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

#ifndef WEBSOCKETMULTICASTADDRESSCALCULATOR_H
#define WEBSOCKETMULTICASTADDRESSCALCULATOR_H

#include <memory>

#include "joynr/IMulticastAddressCalculator.h"

namespace joynr
{
namespace system
{
namespace RoutingTypes
{

class WebSocketAddress;
class Address;

} // RoutingTypes
} // system

class JoynrMessage;
}

namespace joynr
{

class WebSocketMulticastAddressCalculator : public IMulticastAddressCalculator
{
public:
    explicit WebSocketMulticastAddressCalculator(
            std::shared_ptr<const system::RoutingTypes::WebSocketAddress> clusterControllerAddress);

    std::shared_ptr<const system::RoutingTypes::Address> compute(
            const JoynrMessage& message) override;

private:
    std::shared_ptr<const system::RoutingTypes::WebSocketAddress> clusterControllerAddress;
};

} // namespace joynr
#endif // WEBSOCKETMULTICASTADDRESSCALCULATOR_H
