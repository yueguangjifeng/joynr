/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "MqttReceiver.h"

#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/JsonSerializer.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

INIT_LOGGER(MqttReceiver);

MqttReceiver::MqttReceiver(const MessagingSettings& settings)
        : channelCreatedSemaphore(new joynr::Semaphore(0)),
          isChannelCreated(false),
          channelIdForMqttTopic(),
          channelIdForCapabilitiesDirectory(),
          receiverId(),
          settings(settings),
          channelUrlDirectory(),
          mosquittoSubscriber(settings.getBrokerUrl(),
                              channelIdForCapabilitiesDirectory,
                              channelCreatedSemaphore),
          mqttSettings()
{
    MessagingPropertiesPersistence persist(settings.getMessagingPropertiesPersistenceFilename());

    channelIdForMqttTopic = persist.getChannelId();
    std::string brokerUri =
            "tcp://" + settings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost() + ":" +
            std::to_string(settings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort());
    system::RoutingTypes::MqttAddress receiveMqttAddress(brokerUri, channelIdForMqttTopic);
    channelIdForCapabilitiesDirectory = JsonSerializer::serialize(receiveMqttAddress);
    receiverId = persist.getReceiverId();

    init();
}

MqttReceiver::~MqttReceiver()
{
    mosquittoSubscriber.stop();
}

void MqttReceiver::init()
{
    mosquittoSubscriber.registerChannelId(channelIdForMqttTopic);
}

void MqttReceiver::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory)
{
    std::ignore = channelUrlDirectory;
}

void MqttReceiver::updateSettings()
{
}

void MqttReceiver::startReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "startReceiveQueue");

    mosquittoSubscriber.start();
}

void MqttReceiver::waitForReceiveQueueStarted()
{
    JOYNR_LOG_DEBUG(logger, "waiting for ReceiveQueue to be started.");

    if (!isChannelCreated) {
        channelCreatedSemaphore->wait();
        isChannelCreated = true;
    }
}

void MqttReceiver::stopReceiveQueue()
{
    JOYNR_LOG_DEBUG(logger, "stopReceiveQueue");

    mosquittoSubscriber.stop();
}

const std::string& MqttReceiver::getReceiveChannelId() const
{
    return channelIdForCapabilitiesDirectory;
}

bool MqttReceiver::tryToDeleteChannel()
{
    return true;
}

void MqttReceiver::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    mosquittoSubscriber.registerReceiveCallback(onTextMessageReceived);
}

} // namespace joynr