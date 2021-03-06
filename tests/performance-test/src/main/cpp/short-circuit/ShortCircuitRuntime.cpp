/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "ShortCircuitRuntime.h"

#include <chrono>
#include <limits>

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Dispatcher.h"
#include "joynr/IKeychain.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/MessageSender.h"
#include "joynr/MessageQueue.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/Settings.h"
#include "joynr/SubscriptionManager.h"
#include "libjoynr/in-process/InProcessMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynrclustercontroller/include/joynr/CcMessageRouter.h"

namespace joynr
{

class ITransportStatus;

ShortCircuitRuntime::ShortCircuitRuntime(std::unique_ptr<Settings> settings,
                                         std::shared_ptr<IKeychain> keyChain)
        : JoynrRuntimeImpl(*settings),
          keyChain(std::move(keyChain)),
          clusterControllerSettings(*settings),
          enablePersistency(true)
{
    auto messagingStubFactory = std::make_unique<MessagingStubFactory>();
    requestCallerDirectory = std::make_shared<DummyRequestCallerDirectory>();

    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());

    const std::string multicastTopicPrefix = "";

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
            std::make_unique<MqttMulticastAddressCalculator>(nullptr, multicastTopicPrefix);

    const std::string& globalClusterControllerAddress("globalAddress");
    const std::string messageNotificationProviderParticipantId(
            "messageNotificationProviderParticipantId");

    messageRouter = std::make_shared<CcMessageRouter>(
            messagingSettings,
            clusterControllerSettings,
            std::move(messagingStubFactory),
            nullptr,
            nullptr,
            singleThreadedIOService.getIOService(),
            std::move(addressCalculator),
            globalClusterControllerAddress,
            messageNotificationProviderParticipantId,
            enablePersistency,
            std::vector<std::shared_ptr<ITransportStatus>>{},
            std::make_unique<MessageQueue<std::string>>(),
            std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());

    messageSender = std::make_shared<MessageSender>(messageRouter, keyChain);
    joynrDispatcher =
            std::make_shared<Dispatcher>(messageSender, singleThreadedIOService.getIOService());
    messageSender->registerDispatcher(joynrDispatcher);

    dispatcherMessagingSkeleton = std::make_shared<InProcessMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = std::make_shared<PublicationManager>(
            singleThreadedIOService.getIOService(), messageSender, enablePersistency);
    subscriptionManager = std::make_shared<SubscriptionManager>(
            singleThreadedIOService.getIOService(), messageRouter);
    inProcessDispatcher =
            std::make_shared<InProcessDispatcher>(singleThreadedIOService.getIOService());

    inProcessPublicationSender = std::make_shared<InProcessPublicationSender>(subscriptionManager);
    auto inProcessConnectorFactory = std::make_unique<InProcessConnectorFactory>(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            std::dynamic_pointer_cast<IRequestCallerDirectory>(inProcessDispatcher));
    auto joynrMessagingConnectorFactory =
            std::make_unique<JoynrMessagingConnectorFactory>(messageSender, subscriptionManager);
    auto connectorFactory = std::make_unique<ConnectorFactory>(
            std::move(inProcessConnectorFactory), std::move(joynrMessagingConnectorFactory));
    proxyFactory = std::make_unique<ProxyFactory>(std::move(connectorFactory));

    std::string persistenceFilename = "dummy.txt";
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_shared<DummyDiscovery>();
    capabilitiesRegistrar =
            std::make_unique<CapabilitiesRegistrar>(dispatcherList,
                                                    discoveryProxy,
                                                    participantIdStorage,
                                                    dispatcherAddress,
                                                    messageRouter,
                                                    std::numeric_limits<std::int64_t>::max(),
                                                    publicationManager,
                                                    globalClusterControllerAddress);

    maximumTtlMs = std::chrono::milliseconds(std::chrono::hours(24) * 30).count();
}

} // namespace joynr
