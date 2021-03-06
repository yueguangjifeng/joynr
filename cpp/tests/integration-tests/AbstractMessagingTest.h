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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ClusterControllerSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MutableMessage.h"
#include "joynr/MessageQueue.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MessageSender.h"
#include "joynr/CcMessageRouter.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/Request.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/IPlatformSecurityManager.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockMessageSender.h"
#include "tests/mock/MockInProcessMessagingSkeleton.h"
#include "tests/mock/MockTransportMessageSender.h"
#include "tests/mock/MockTransportMessageReceiver.h"
#include "tests/mock/MockDispatcher.h"

using namespace ::testing;
using namespace joynr;

class AbstractMessagingTest : public ::testing::Test {
public:
    std::string settingsFileName;
    Settings settings;
    MessagingSettings messagingSettings;
    ClusterControllerSettings clusterControllerSettings;
    std::string senderId;
    std::string globalClusterControllerAddress;
    std::string receiverId;
    Request request;
    std::string requestId;
    MessagingQos qos;
    std::shared_ptr<MockDispatcher> dispatcher;
    std::shared_ptr<MockInProcessMessagingSkeleton> inProcessMessagingSkeleton;
    Semaphore semaphore;
    const bool isLocalMessage;
    const bool enablePersistency;

    MutableMessageFactory messageFactory;
    std::shared_ptr<MockTransportMessageReceiver> mockMessageReceiver;
    std::shared_ptr<MockTransportMessageSender> mockMessageSender;
    std::shared_ptr<MessagingStubFactory> messagingStubFactory;
    std::shared_ptr<SingleThreadedIOService> singleThreadedIOService;
    std::shared_ptr<CcMessageRouter> messageRouter;
    AbstractMessagingTest() :
        settingsFileName("MessagingTest.settings"),
        settings(settingsFileName),
        messagingSettings(settings),
        clusterControllerSettings(settings),
        senderId("senderParticipantId"),
        globalClusterControllerAddress("senderChannelId"),
        receiverId("receiverParticipantId"),
        request(),
        requestId("requestId"),
        qos(),
        dispatcher(),
        inProcessMessagingSkeleton(std::make_shared<MockInProcessMessagingSkeleton>(dispatcher)),
        semaphore(0),
        isLocalMessage(false),
        enablePersistency(true),
        messageFactory(),
        mockMessageReceiver(new MockTransportMessageReceiver()),
        mockMessageSender(new MockTransportMessageSender()),
        messagingStubFactory(std::make_shared<MessagingStubFactory>()),
        singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
        messageRouter()
    {
        const std::string globalCCAddress("globalAddress");
        const std::string messageNotificationProviderParticipantId("messageNotificationProviderParticipantId");

        messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());
        messageRouter = std::make_shared<CcMessageRouter>(messagingSettings,
                                                          clusterControllerSettings,
                                                          messagingStubFactory,
                                                          std::make_shared<MulticastMessagingSkeletonDirectory>(),
                                                          nullptr,
                                                          singleThreadedIOService->getIOService(),
                                                          nullptr,
                                                          globalCCAddress,
                                                          messageNotificationProviderParticipantId,
                                                          enablePersistency,
                                                          std::vector<std::shared_ptr<ITransportStatus>>{},
                                                          std::make_unique<MessageQueue<std::string>>(),
                                                          std::make_unique<MessageQueue<std::shared_ptr<ITransportStatus>>>());
        messageRouter->init();
        qos.setTtl(10000);
    }

    void WaitXTimes(std::uint64_t x)
    {
        for(std::uint64_t i = 0; i<x; ++i) {
            ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
        }
    }

    ~AbstractMessagingTest(){
        messageRouter->shutdown();
        std::remove(settingsFileName.c_str());
    }

    void sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(
            std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        // Test Outline: send message from JoynrMessageSender to ICommunicationManager
        // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
        //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
        // - InProcessMessagingStub.transmit (IMessaging)
        // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
        // - MessageRouter.route
        // - MessageRunnable.run
        // - *MessagingStub.transmit (IMessaging)
        // - MessageSender.send

        auto mockDispatcher = std::make_shared<MockDispatcher>();
        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(_,_))
                .Times(0);

        // MessageSender should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,_,_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        EXPECT_CALL(*mockDispatcher, addReplyCaller(_,_,_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        MessageSender messageSender(messageRouter, nullptr);
        std::shared_ptr<IReplyCaller> replyCaller;
        messageSender.registerDispatcher(mockDispatcher);

        // local messages
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        messageSender.sendRequest(senderId, receiverId, qos, request, replyCaller, isLocalMessage);

        WaitXTimes(2);
    }

    void routeMsgWithInvalidParticipantId()
    {
        std::string invalidReceiverId("invalidReceiverId");
        MutableMessage mutableMessage = messageFactory.createRequest(
                    senderId,
                    invalidReceiverId,
                    qos,
                    request,
                    isLocalMessage);

        messageRouter->route(mutableMessage.getImmutableMessage());
        SUCCEED();
    }

    void routeMsgToInProcessMessagingSkeleton()
    {
        MutableMessage mutableMessage = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);

        // We must set the reply address here. Otherwise the message router will
        // set it and the message which was created will differ from the message
        // which is passed to the messaging-skeleton.
        mutableMessage.setReplyTo(globalClusterControllerAddress);

        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        // InProcessMessagingSkeleton should receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(immutableMessage),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // MessageSender should not receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,_,_))
                .Times(0);

        EXPECT_CALL(*mockMessageReceiver, getGlobalClusterControllerAddress())
                .Times(0);

        auto messagingSkeletonEndpointAddr = std::make_shared<InProcessMessagingAddress>(inProcessMessagingSkeleton);
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;

        messageRouter->addNextHop(receiverId, messagingSkeletonEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        messageRouter->route(immutableMessage);

        WaitXTimes(1);
    }

    void routeMsgToCommunicationManager(std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        MutableMessage mutableMessage = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);
        mutableMessage.setReplyTo(globalClusterControllerAddress);
        std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
        // InProcessMessagingSkeleton should not receive the message
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(immutableMessage),_))
                .Times(0);

        // *CommunicationManager should receive the message
        EXPECT_CALL(*mockMessageSender, sendMessage(_,Eq(immutableMessage),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        messageRouter->route(immutableMessage);

        WaitXTimes(1);
    }

    void routeMultipleMessages(std::shared_ptr<system::RoutingTypes::Address> joynrMessagingEndpointAddr)
    {
        MutableMessage mutableMessage1 = messageFactory.createRequest(
                    senderId,
                    receiverId,
                    qos,
                    request,
                    isLocalMessage);
        mutableMessage1.setReplyTo(globalClusterControllerAddress);

        std::string receiverId2("receiverId2");
        MutableMessage mutableMessage2 = messageFactory.createRequest(
                    senderId,
                    receiverId2,
                    qos,
                    request,
                    isLocalMessage);
        mutableMessage2.setReplyTo(globalClusterControllerAddress);

        std::shared_ptr<ImmutableMessage> immutableMessage1 = mutableMessage1.getImmutableMessage();
        std::shared_ptr<ImmutableMessage> immutableMessage2 = mutableMessage2.getImmutableMessage();

        // MessageSender should receive message
        EXPECT_CALL(*mockMessageSender, sendMessage(_, Eq(immutableMessage1),_))
                .Times(1).WillRepeatedly(ReleaseSemaphore(&semaphore));

        // InProcessMessagingSkeleton should receive twice message2
        EXPECT_CALL(*inProcessMessagingSkeleton, transmit(Eq(immutableMessage2),_))
                .Times(2).WillRepeatedly(ReleaseSemaphore(&semaphore));

        EXPECT_CALL(*mockMessageReceiver, getGlobalClusterControllerAddress())
                .WillRepeatedly(ReturnRefOfCopy(globalClusterControllerAddress));

        auto messagingSkeletonEndpointAddr = std::make_shared<InProcessMessagingAddress>(inProcessMessagingSkeleton);
        const bool isGloballyVisible = false;
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;

        messageRouter->addNextHop(receiverId2, messagingSkeletonEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);
        messageRouter->addNextHop(receiverId, joynrMessagingEndpointAddr, isGloballyVisible, expiryDateMs, isSticky);

        messageRouter->route(immutableMessage1);
        messageRouter->route(immutableMessage2);
        messageRouter->route(immutableMessage2);

        WaitXTimes(3);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(AbstractMessagingTest);
};
