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
#ifndef ICAPABILITIESCLIENT_H
#define ICAPABILITIESCLIENT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/ProxyBuilder.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{

class ICapabilitiesClient
{
public:
    virtual ~ICapabilitiesClient() = default;
    virtual void add(
            const std::vector<types::GlobalDiscoveryEntry>& capabilitiesInformationList) = 0;
    virtual void remove(const std::string& participantId) = 0;
    virtual void remove(std::vector<std::string> capabilitiesInformationList) = 0;
    virtual std::vector<types::GlobalDiscoveryEntry> lookup(const std::string& domain,
                                                            const std::string& interfaceName,
                                                            const std::int64_t messagingTtl) = 0;
    virtual void lookup(
            const std::string& domain,
            const std::string& interfaceName,
            const std::int64_t messagingTtl,
            std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)>
                    onError = nullptr) = 0;
    virtual void lookup(
            const std::string& participantId,
            std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)>
                    onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& error)>
                    onError = nullptr) = 0;
    virtual std::string getLocalChannelId() const = 0;

    virtual void setProxyBuilder(
            std::unique_ptr<IProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
                    capabilitiesProxyBuilder) = 0;
};

} // namespace joynr
#endif // ICAPABILITIESCLIENT_H
