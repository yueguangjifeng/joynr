/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
const Promise = require("../../../global/Promise");
const MessagingQos = require("../../messaging/MessagingQos");
const MulticastWildcardRegexFactory = require("../../messaging/util/MulticastWildcardRegexFactory");
const defaultMessagingSettings = require("../../start/settings/defaultMessagingSettings");
const SubscriptionQos = require("../../proxy/SubscriptionQos");
const SubscriptionStop = require("../types/SubscriptionStop");
const SubscriptionRequest = require("../types/SubscriptionRequest");
const MulticastSubscriptionRequest = require("../types/MulticastSubscriptionRequest");
const BroadcastSubscriptionRequest = require("../types/BroadcastSubscriptionRequest");
const SubscriptionListener = require("./SubscriptionListener");
const SubscriptionUtil = require("./util/SubscriptionUtil");
const LongTimer = require("../../util/LongTimer");
const LoggingManager = require("../../system/LoggingManager");
const uuid = require("uuid/v4");
const UtilInternal = require("../../util/UtilInternal");
const Typing = require("../../util/Typing");
const TypeRegistrySingleton = require("../../../joynr/types/TypeRegistrySingleton");
const PublicationMissedException = require("../../exceptions/PublicationMissedException");
const JSONSerializer = require("../../util/JSONSerializer");
/**
 * @name SubscriptionManager
 * @constructor
 * @param {Dispatcher}
 *            dispatcher
 */
function SubscriptionManager(dispatcher) {
    const multicastWildcardRegexFactory = new MulticastWildcardRegexFactory();
    const log = LoggingManager.getLogger("joynr.dispatching.subscription.SubscriptionManager");
    const typeRegistry = TypeRegistrySingleton.getInstance();
    if (!(this instanceof SubscriptionManager)) {
        // in case someone calls constructor without new keyword (e.g. var c =
        // Constructor({..}))
        return new SubscriptionManager(dispatcher);
    }

    // stores subscriptionId - SubscriptionInfo pairs
    const subscriptionInfos = {};
    // stores subscriptionId - SubscriptionListener
    const subscriptionListeners = {};
    // stores the object which is returned by setTimeout mapped to the subscriptionId
    let publicationCheckTimerIds = {};
    let subscriptionReplyCallers = {};
    let started = true;

    const multicastSubscribers = {};

    function isReady() {
        return started;
    }

    /**
     * @param {String}
     *            subscriptionId Id of the subscription to check
     * @returns {Number} time of last received publication
     */
    function getLastPublicationTime(subscriptionId) {
        return subscriptionInfos[subscriptionId].lastPublicationTime_ms;
    }

    function setLastPublicationTime(subscriptionId, time_ms) {
        subscriptionInfos[subscriptionId].lastPublicationTime_ms = time_ms;
    }

    /**
     * @param {String}
     *            subscriptionId Id of the subscription to check
     * @param {Number}
     *            delay_ms Delay to the next publication check.
     * @returns {Boolean} true if subscription is expired, false if end date is not reached.
     */
    function subscriptionEnds(subscriptionId, delay_ms) {
        if (subscriptionInfos[subscriptionId] === undefined) {
            log.warn('subscriptionEnds has been called with unresolved subscriptionId "' + subscriptionId + '"');
            return true;
        }
        const expiryDateMs = subscriptionInfos[subscriptionId].qos.expiryDateMs;
        // log.debug("Checking subscription end for subscriptionId: " + subscriptionId + "
        // expiryDateMs: " + expiryDateMs + "
        // current time: " + Date.now());
        const ends = expiryDateMs <= Date.now() + delay_ms;
        // if (ends === true) {
        // log.info("Subscription end date reached for id: " + subscriptionId);
        // }
        return ends;
    }

    /**
     * @param {String}
     *            subscriptionId Id of the subscription to check
     * @param {Number}
     *            alertAfterIntervalMs maximum delay between two incoming publications
     */
    function checkPublication(subscriptionId, alertAfterIntervalMs) {
        const subscriptionListener = subscriptionListeners[subscriptionId];
        const timeSinceLastPublication = Date.now() - getLastPublicationTime(subscriptionId);
        // log.debug("timeSinceLastPublication : " + timeSinceLastPublication + "
        // alertAfterIntervalMs: " + alertAfterIntervalMs);
        if (alertAfterIntervalMs > 0 && timeSinceLastPublication >= alertAfterIntervalMs) {
            // log.warn("publication missed for subscription id: " + subscriptionId);
            if (subscriptionListener.onError) {
                const publicationMissedException = new PublicationMissedException({
                    detailMessage: "alertAfterIntervalMs period exceeded without receiving publication",
                    subscriptionId
                });
                subscriptionListener.onError(publicationMissedException);
            }
        }

        let delay_ms;
        if (timeSinceLastPublication > alertAfterIntervalMs) {
            delay_ms = alertAfterIntervalMs;
        } else {
            delay_ms = alertAfterIntervalMs - timeSinceLastPublication;
        }

        if (!subscriptionEnds(subscriptionId, delay_ms)) {
            // log.debug("Rescheduling checkPublication with delay: " + delay_ms);
            publicationCheckTimerIds[subscriptionId] = LongTimer.setTimeout(
                checkPublication,
                delay_ms,
                subscriptionId,
                alertAfterIntervalMs
            );
        }
    }

    function calculateTtl(subscriptionQos) {
        if (subscriptionQos.expiryDateMs === SubscriptionQos.NO_EXPIRY_DATE) {
            return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
        }
        const ttl = subscriptionQos.expiryDateMs - Date.now();
        if (ttl > defaultMessagingSettings.MAX_MESSAGING_TTL_MS) {
            return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
        }
        return ttl;
    }

    function storeSubscriptionRequest(settings, subscriptionRequest) {
        let onReceiveWrapper;
        if (settings.attributeType !== undefined) {
            onReceiveWrapper = function(response) {
                settings.onReceive(Typing.augmentTypes(response[0], typeRegistry, settings.attributeType));
            };
        } else {
            onReceiveWrapper = function(response) {
                let responseKey;
                for (responseKey in response) {
                    if (response.hasOwnProperty(responseKey)) {
                        response[responseKey] = Typing.augmentTypes(
                            response[responseKey],
                            typeRegistry,
                            settings.broadcastParameter[responseKey].type
                        );
                    }
                }
                settings.onReceive(response);
            };
        }
        subscriptionListeners[subscriptionRequest.subscriptionId] = new SubscriptionListener({
            onReceive: onReceiveWrapper,
            onError: settings.onError,
            onSubscribed: settings.onSubscribed
        });
        const subscriptionInfo = UtilInternal.extend(
            {
                proxyId: settings.proxyId,
                providerDiscoveryEntry: settings.providerDiscoveryEntry,
                lastPublicationTime_ms: 0
            },
            subscriptionRequest
        );

        subscriptionInfos[subscriptionRequest.subscriptionId] = subscriptionInfo;

        const alertAfterIntervalMs = subscriptionRequest.qos.alertAfterIntervalMs;
        if (alertAfterIntervalMs !== undefined && alertAfterIntervalMs > 0) {
            publicationCheckTimerIds[subscriptionRequest.subscriptionId] = LongTimer.setTimeout(
                checkPublication,
                alertAfterIntervalMs,
                subscriptionRequest.subscriptionId,
                alertAfterIntervalMs
            );
        }
    }

    function removeRequestFromMulticastSubscribers(multicastId, subscriptionId) {
        let i, multicastIdPattern, subscribers;
        for (multicastIdPattern in multicastSubscribers) {
            if (multicastSubscribers.hasOwnProperty(multicastIdPattern)) {
                subscribers = multicastSubscribers[multicastIdPattern];
                for (i = 0; i < subscribers.length; i++) {
                    if (subscribers[i] === subscriptionId) {
                        subscribers.splice(i, 1);
                        if (subscribers.length === 0) {
                            delete multicastSubscribers[multicastIdPattern];
                        }
                    }
                }
            }
        }
    }

    function cleanupSubscription(subscriptionId) {
        if (subscriptionInfos[subscriptionId] !== undefined) {
            const subscriptionInfo = subscriptionInfos[subscriptionId];
            if (subscriptionInfo.multicastId !== undefined) {
                removeRequestFromMulticastSubscribers(subscriptionInfo.multicastId, subscriptionId);
            }
            delete subscriptionInfos[subscriptionId];
        }
        if (subscriptionListeners[subscriptionId] !== undefined) {
            delete subscriptionListeners[subscriptionId];
        }
        if (subscriptionReplyCallers[subscriptionId] !== undefined) {
            delete subscriptionReplyCallers[subscriptionId];
        }
    }

    /**
     * This callback is called when a publication is received
     * @callback SubscriptionManager~onReceive
     * @param {Object} publication received
     */
    /**
     * This callback is called if there was an error with the subscription
     * @callback SubscriptionManager~onError
     * @param {Error} error
     */

    /**
     * @name SubscriptionManager#registerSubscription
     * @function
     * @param {Object}
     *            settings
     * @param {String}
     *            settings.proxyId participantId of the sender
     * @param {DiscoveryEntryWithMetaInfo}
     *            settings.providerDiscoveryEntry DiscoveryEntry of the receiver
     * @param {String}
     *            settings.attributeType the type of the subscribing attribute
     * @param {String}
     *            settings.attributeName the attribute name to subscribe to
     * @param {SubscriptionQos}
     *            [settings.qos] the subscriptionQos
     * @param {String}
     *            settings.subscriptionId optional parameter subscriptionId to reuse a
     *            preexisting identifier for this concrete subscription request
     * @param {SubscriptionManager~onReceive}
     *            settings.onReceive the callback for received publications.
     * @param {SubscriptionManager~onError}
     *            settings.onError the callback for missing publication alerts or when an
     *            error occurs.
     * @param {SubscriptionManager~onSubscribed}
     *            settings.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @returns an A promise object which provides the subscription token upon success and
     *          an error upon failure
     */
    this.registerSubscription = function registerSubscription(settings) {
        if (!isReady()) {
            return Promise.reject(new Error("SubscriptionManager is already shut down"));
        }
        const subscriptionId = settings.subscriptionId || uuid();
        // log.debug("Registering Subscription Id " + subscriptionId);

        if (settings.attributeName === undefined) {
            return Promise.reject(
                new Error(
                    "Error: attributeName not provided in call to registerSubscription, settings = " +
                        JSON.stringify(settings)
                )
            );
        }
        if (settings.attributeType === undefined) {
            return Promise.reject(
                new Error(
                    "Error: attributeType not provided in call to registerSubscription, settings = " +
                        JSON.stringify(settings)
                )
            );
        }

        if (settings.onError === undefined) {
            log.warn(
                'Warning: subscription for attribute "' +
                    settings.attributeName +
                    '" has been done without error callback function. You will not be informed about missed publications. Please specify the "onError" parameter while subscribing!'
            );
        }
        if (settings.onReceive === undefined) {
            log.warn(
                'Warning: subscription for attribute "' +
                    settings.attributeName +
                    '" has been done without receive callback function. You will not be informed about incoming publications. Please specify the "onReceive" parameter while subscribing!'
            );
        }
        const subscriptionRequest = new SubscriptionRequest({
            subscriptionId,
            subscribedToName: settings.attributeName,
            qos: settings.qos
        });

        const messagingQos = new MessagingQos({
            ttl: calculateTtl(subscriptionRequest.qos)
        });

        const deferred = UtilInternal.createDeferred();

        subscriptionReplyCallers[subscriptionId] = {
            resolve: deferred.resolve,
            reject: deferred.reject
        };

        storeSubscriptionRequest(settings, subscriptionRequest);

        dispatcher.sendSubscriptionRequest({
            from: settings.proxyId,
            toDiscoveryEntry: settings.providerDiscoveryEntry,
            messagingQos,
            subscriptionRequest
        });

        return deferred.promise;
    };

    function addRequestToMulticastSubscribers(multicastId, subscriptionId) {
        const multicastIdPattern = multicastWildcardRegexFactory.createIdPattern(multicastId);
        if (multicastSubscribers[multicastIdPattern] === undefined) {
            multicastSubscribers[multicastIdPattern] = [];
        }
        const subscribers = multicastSubscribers[multicastIdPattern];
        for (let i = 0; i < subscribers.length; i++) {
            if (subscribers[i] === subscriptionId) {
                return;
            }
        }
        subscribers.push(subscriptionId);
    }

    function createBroadcastSubscriptionRequest(parameters) {
        let request;
        if (parameters.selective) {
            request = new BroadcastSubscriptionRequest({
                subscriptionId: parameters.subscriptionId || uuid(),
                subscribedToName: parameters.broadcastName,
                qos: parameters.subscriptionQos,
                filterParameters: parameters.filterParameters
            });
        } else {
            request = new MulticastSubscriptionRequest({
                multicastId: SubscriptionUtil.createMulticastId(
                    parameters.providerDiscoveryEntry.participantId,
                    parameters.broadcastName,
                    parameters.partitions
                ),
                subscriptionId: parameters.subscriptionId || uuid(),
                subscribedToName: parameters.broadcastName,
                qos: parameters.subscriptionQos
            });
            addRequestToMulticastSubscribers(request.multicastId, request.subscriptionId);
        }
        return request;
    }

    /**
     * @name SubscriptionManager#registerBroadcastSubscription
     * @function
     * @param {Object}
     *            parameters
     * @param {String}
     *            parameters.proxyId participantId of the sender
     * @param {DiscoveryEntryWithMetaInfo}
     *            parameters.providerDiscoveryEntry DiscoveryEntry of the receiver
     * @param {String}
     *            parameters.broadcastName the name of the broadcast being subscribed to
     * @param {String[]}
     *            parameters.broadcastParameter the parameter meta information of the broadcast being subscribed to
     * @param {SubscriptionQos}
     *            [parameters.subscriptionQos] the subscriptionQos
     * @param {BroadcastFilterParameters}
     *            [parameters.filterParameters] filter parameters used to indicate interest in
     *            only a subset of broadcasts that might be sent.
     * @param {Boolean}
     *            parameters.selective true if broadcast is selective
     * @param {String[]}
     *            [parameters.partitions] partitions for multicast requests
     * @param {String}
     *            parameters.subscriptionId optional parameter subscriptionId to reuse a
     *            pre-existing identifier for this concrete subscription request
     * @param {SubscriptionManager~onReceive}
     *            parameters.onReceive is called when a broadcast is received.
     * @param {SubscriptionManager~onError}
     *            parameters.onError is called when an error occurs with the broadcast
     * @param {SubscriptionManager~onSubscribed}
     *            parameters.onSubscribed the callback to inform once the subscription request has
     *            been delivered successfully
     * @returns a promise object which provides the subscription token upon success and an error
     *          upon failure
     */
    this.registerBroadcastSubscription = function(parameters) {
        if (!isReady()) {
            return Promise.reject(new Error("SubscriptionManager is already shut down"));
        }

        const deferred = UtilInternal.createDeferred();
        const subscriptionRequest = createBroadcastSubscriptionRequest(parameters);

        const messagingQos = new MessagingQos({
            ttl: calculateTtl(subscriptionRequest.qos)
        });

        subscriptionReplyCallers[subscriptionRequest.subscriptionId] = {
            resolve: deferred.resolve,
            reject: deferred.reject
        };

        storeSubscriptionRequest(parameters, subscriptionRequest);

        function sendBroadcastSubscriptionRequestOnError(error) {
            cleanupSubscription(subscriptionRequest.subscriptionId);
            if (parameters.onError) {
                parameters.onError(error);
            }
            deferred.reject(error);
            return deferred.promise;
        }

        dispatcher
            .sendBroadcastSubscriptionRequest({
                from: parameters.proxyId,
                toDiscoveryEntry: parameters.providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest
            })
            .catch(sendBroadcastSubscriptionRequestOnError);

        return deferred.promise;
    };

    /**
     * @name SubscriptionManager#handleSubscriptionReply
     * @function
     * @param subscriptionReply
     *            {SubscriptionReply} incoming subscriptionReply
     */
    this.handleSubscriptionReply = function handleSubscriptionReply(subscriptionReply) {
        const subscriptionReplyCaller = subscriptionReplyCallers[subscriptionReply.subscriptionId];
        const subscriptionListener = subscriptionListeners[subscriptionReply.subscriptionId];

        if (subscriptionReplyCaller === undefined && subscriptionListener === undefined) {
            log.error(
                "error handling subscription reply, because subscriptionReplyCaller and subscriptionListener could not be found: " +
                    JSONSerializer.stringify(subscriptionReply, undefined, 4)
            );
            return;
        }

        try {
            if (subscriptionReply.error) {
                if (!(subscriptionReply.error instanceof Error)) {
                    subscriptionReply.error = Typing.augmentTypes(subscriptionReply.error, typeRegistry);
                }
                if (subscriptionReplyCaller !== undefined) {
                    subscriptionReplyCaller.reject(subscriptionReply.error);
                }
                if (subscriptionListener !== undefined && subscriptionListener.onError !== undefined) {
                    subscriptionListener.onError(subscriptionReply.error);
                }
                cleanupSubscription(subscriptionReply.subscriptionId);
            } else {
                if (subscriptionReplyCaller !== undefined) {
                    subscriptionReplyCaller.resolve(subscriptionReply.subscriptionId);
                }
                if (subscriptionListener !== undefined && subscriptionListener.onSubscribed !== undefined) {
                    subscriptionListener.onSubscribed(subscriptionReply.subscriptionId);
                }
                delete subscriptionReplyCallers[subscriptionReply.subscriptionId];
            }
        } catch (e) {
            log.error(
                "exception thrown during handling subscription reply " +
                    JSONSerializer.stringify(subscriptionReply, undefined, 4) +
                    ":\n" +
                    e.stack
            );
            delete subscriptionReplyCallers[subscriptionReply.subscriptionId];
        }
    };

    /**
     * @name SubscriptionManager#handleMulticastPublication
     * @function
     * @param publication
     *            {MulticastPublication} incoming multicast publication
     */
    this.handleMulticastPublication = function handleMulticastPublication(publication) {
        let i,
            multicastIdPattern,
            subscribers,
            subscribersFound = false;
        for (multicastIdPattern in multicastSubscribers) {
            if (multicastSubscribers.hasOwnProperty(multicastIdPattern)) {
                if (publication.multicastId.match(new RegExp(multicastIdPattern)) !== null) {
                    subscribers = multicastSubscribers[multicastIdPattern];
                    if (subscribers !== undefined) {
                        subscribersFound = true;
                        for (i = 0; i < subscribers.length; i++) {
                            const subscriptionListener = subscriptionListeners[subscribers[i]];
                            if (publication.error) {
                                if (subscriptionListener.onError) {
                                    subscriptionListener.onError(publication.error);
                                } else {
                                    log.debug(
                                        'subscriptionListener with Id "' +
                                            subscribers[i] +
                                            '" has no onError callback. Skipping error publication'
                                    );
                                }
                            } else if (publication.response) {
                                if (subscriptionListener.onReceive) {
                                    subscriptionListener.onReceive(publication.response);
                                } else {
                                    log.debug(
                                        'subscriptionListener with Id "' +
                                            subscribers[i] +
                                            '" has no onReceive callback. Skipping multicast publication'
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!subscribersFound) {
            throw new Error(
                "Publication cannot be handled, as no subscription with " +
                    "multicastId " +
                    publication.multicastId +
                    " is known."
            );
        }
    };

    /**
     * @name SubscriptionManager#handlePublication
     * @function
     * @param publication
     *            {SubscriptionPublication} incoming publication
     */
    this.handlePublication = function handlePublication(publication) {
        if (subscriptionInfos[publication.subscriptionId] === undefined) {
            throw new Error(
                "Publication cannot be handled, as no subscription with " +
                    "subscriptionId " +
                    publication.subscriptionId +
                    " is known."
            );
        }
        setLastPublicationTime(publication.subscriptionId, Date.now());
        const subscriptionListener = subscriptionListeners[publication.subscriptionId];
        if (publication.error) {
            if (subscriptionListener.onError) {
                subscriptionListener.onError(publication.error);
            } else {
                log.debug(
                    'subscriptionListener with Id "' +
                        publication.subscriptionId +
                        '" has no onError callback. Skipping error publication'
                );
            }
        } else if (publication.response) {
            if (subscriptionListener.onReceive) {
                subscriptionListener.onReceive(publication.response);
            } else {
                log.debug(
                    'subscriptionListener with Id "' +
                        publication.subscriptionId +
                        '" has no onReceive callback. Skipping publication'
                );
            }
        }
    };

    /**
     * @name SubscriptionManager#unregisterSubscription
     * @function
     * @param {Object}
     *            settings
     * @param {MessagingQos}
     *            settings.messagingQos the messaging Qos object for the ttl
     * @param {String}
     *            settings.subscriptionId of the subscriptionId to stop
     * @returns {Object} A promise object
     */
    this.unregisterSubscription = function unregisterSubscription(settings) {
        if (!isReady()) {
            throw new Error("SubscriptionManager is already shut down");
        }

        const subscriptionInfo = subscriptionInfos[settings.subscriptionId];
        let errorMessage;
        if (subscriptionInfo === undefined) {
            errorMessage = "Cannot find subscription with id: " + settings.subscriptionId;
            log.error(errorMessage);
            return Promise.reject(new Error(errorMessage));
        }

        const subscriptionStop = new SubscriptionStop({
            subscriptionId: settings.subscriptionId
        });

        let promise;
        if (subscriptionInfo.multicastId !== undefined) {
            promise = dispatcher.sendMulticastSubscriptionStop({
                from: subscriptionInfo.proxyId,
                toDiscoveryEntry: subscriptionInfo.providerDiscoveryEntry,
                messagingQos: settings.messagingQos,
                multicastId: subscriptionInfo.multicastId,
                subscriptionStop
            });
        } else {
            promise = dispatcher.sendSubscriptionStop({
                from: subscriptionInfo.proxyId,
                toDiscoveryEntry: subscriptionInfo.providerDiscoveryEntry,
                messagingQos: settings.messagingQos,
                subscriptionStop
            });
        }

        if (publicationCheckTimerIds[settings.subscriptionId] !== undefined) {
            LongTimer.clearTimeout(publicationCheckTimerIds[settings.subscriptionId]);
            delete publicationCheckTimerIds[settings.subscriptionId];
        }

        cleanupSubscription(settings.subscriptionId);

        return promise;
    };

    this.hasMulticastSubscriptions = function() {
        return Object.keys(multicastSubscribers).length > 0;
    };

    this.hasOpenSubscriptions = function() {
        const hasSubscriptionInfos = Object.keys(subscriptionInfos).length > 0;
        const hasSubscriptionListeners = Object.keys(subscriptionListeners).length > 0;
        const hasPublicationCheckTimerIds = Object.keys(publicationCheckTimerIds).length > 0;
        const hasSubscriptionReplyCallers = Object.keys(subscriptionReplyCallers).length > 0;
        return (
            hasSubscriptionInfos ||
            hasSubscriptionListeners ||
            hasPublicationCheckTimerIds ||
            hasSubscriptionReplyCallers ||
            this.hasMulticastSubscriptions()
        );
    };

    /**
     * This method is meant to be called by the runtime before shutdown is called.
     * It turns out that there is a necessary shutdown order and Subscriptionmanager can't be shutdown first.
     *
     * @function
     * @param {number} clearSubscriptionsTimeoutMs
     */
    this.terminateSubscriptions = function(clearSubscriptionsTimeoutMs) {
        const cleanUpPromises = [];
        let activeSubscriptionId;
        for (activeSubscriptionId in subscriptionInfos) {
            if (Object.prototype.hasOwnProperty.call(subscriptionInfos, activeSubscriptionId)) {
                const promise = this.unregisterSubscription({
                    subscriptionId: activeSubscriptionId,
                    messagingQos: new MessagingQos({ ttl: clearSubscriptionsTimeoutMs })
                });
                cleanUpPromises.push(promise);
            }
        }
        return UtilInternal.timeoutPromise(Promise.all(cleanUpPromises), clearSubscriptionsTimeoutMs);
    };

    /**
     * Shutdown the subscription manager
     *
     * @function
     */
    this.shutdown = function shutdown() {
        let subscriptionId;
        for (subscriptionId in publicationCheckTimerIds) {
            if (publicationCheckTimerIds.hasOwnProperty(subscriptionId)) {
                const timerId = publicationCheckTimerIds[subscriptionId];
                if (timerId !== undefined) {
                    LongTimer.clearTimeout(timerId);
                }
            }
        }
        publicationCheckTimerIds = {};
        for (subscriptionId in subscriptionReplyCallers) {
            if (subscriptionReplyCallers.hasOwnProperty(subscriptionId)) {
                const subscriptionReplyCaller = subscriptionReplyCallers[subscriptionId];
                if (subscriptionReplyCaller) {
                    subscriptionReplyCaller.reject(new Error("Subscription Manager is already shut down"));
                }
            }
        }
        subscriptionReplyCallers = {};
        started = false;
    };
}

module.exports = SubscriptionManager;
