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
const Typing = require("../../util/Typing");

/**
 * @constructor MqttMessagingSkeleton
 * @param {Object} settings
 * @param {SharedMqttClient} settings.client the mqtt client to be used to transmit messages
 * @param {MessageRouter} settings.messageRouter the message router
 * @param {MqttAddress} settings.address own address of joynr client
 */
const MqttMessagingSkeleton = function MqttMessagingSkeleton(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.client, "SharedMqttClient", "settings.client");
    Typing.checkProperty(settings.messageRouter, "MessageRouter", "settings.messageRouter");
    Typing.checkProperty(settings.address, "MqttAddress", "settings.address");

    this._multicastSubscriptionCount = {};
    this._settings = settings;

    settings.client.onmessage = function(topic, message) {
        message.isReceivedFromGlobal = true;
        settings.messageRouter.route(message);
    };

    settings.client.subscribe(settings.address.topic + "/#");
};

MqttMessagingSkeleton.prototype._translateWildcard = function(multicastId) {
    if (multicastId.match(/[\w\W]*\/[*]$/)) {
        return multicastId.replace(/\/\*/g, "/#");
    }
    return multicastId;
};

MqttMessagingSkeleton.prototype.registerMulticastSubscription = function(multicastId) {
    if (this._multicastSubscriptionCount[multicastId] === undefined) {
        this._multicastSubscriptionCount[multicastId] = 0;
    }
    this._settings.client.subscribe(this._translateWildcard(multicastId));
    this._multicastSubscriptionCount[multicastId] = this._multicastSubscriptionCount[multicastId] + 1;
};

MqttMessagingSkeleton.prototype.unregisterMulticastSubscription = function(multicastId) {
    let subscribersCount = this._multicastSubscriptionCount[multicastId];
    if (subscribersCount !== undefined) {
        subscribersCount--;
        if (subscribersCount === 0) {
            this._settings.client.unsubscribe(this._translateWildcard(multicastId));
            delete this._multicastSubscriptionCount[multicastId];
        } else {
            this._multicastSubscriptionCount[multicastId] = subscribersCount;
        }
    }
};

/**
 * @function MqttMessagingSkeleton#shutdown
 */
MqttMessagingSkeleton.prototype.shutdown = function shutdown() {
    this._settings.client.close();
};

module.exports = MqttMessagingSkeleton;
