/*
* This file was generated by the CommonAPI Generators.
*
*/
#ifndef JOYNR_MESSAGING_I_Capabilities_PROXY_BASE_H_
#define JOYNR_MESSAGING_I_Capabilities_PROXY_BASE_H_

#include "ICapabilities.h"
#include <cstdint>
#include <joynr/messaging/types/Types.h>
#include <vector>
#include <CommonAPI/InputStream.h>
#include <string>
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/OutputStream.h>
#include <CommonAPI/Proxy.h>
#include <functional>
#include <future>

namespace joynr {
namespace messaging {

class ICapabilitiesProxyBase: virtual public CommonAPI::Proxy {
 public:
    typedef std::function<void(const CommonAPI::CallStatus&)> AddAsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&)> AddEndPointAsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&, const types::Types::CapabilityEntryList&)> Lookup1AsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&, const types::Types::CapabilityEntryList&)> Lookup2AsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&)> RemoveAsyncCallback;




    virtual void add(const std::string& domain, const std::string& interfaceName, const std::string& participantId, const types::Types::ProviderQos& qos, const types::Types::EndpointAddressList& endpointAddressList, const types::Types::EndpointAddressBase& messagingStubAddress, const int64_t& timeout_ms, CommonAPI::CallStatus& callStatus) = 0;
    virtual std::future<CommonAPI::CallStatus> addAsync(const std::string& domain, const std::string& interfaceName, const std::string& participantId, const types::Types::ProviderQos& qos, const types::Types::EndpointAddressList& endpointAddressList, const types::Types::EndpointAddressBase& messagingStubAddress, const int64_t& timeout_ms, AddAsyncCallback callback) = 0;

    virtual void addEndPoint(const std::string& participantId, const types::Types::EndpointAddressBase& messagingStubAddress, const int64_t& timeout_ms, CommonAPI::CallStatus& callStatus) = 0;
    virtual std::future<CommonAPI::CallStatus> addEndPointAsync(const std::string& participantId, const types::Types::EndpointAddressBase& messagingStubAddress, const int64_t& timeout_ms, AddEndPointAsyncCallback callback) = 0;

    virtual void lookup1(const std::string& domain, const std::string& interfaceName, const types::Types::ProviderQosRequirement& qos, const types::Types::DiscoveryQos& discoveryQos, CommonAPI::CallStatus& callStatus, types::Types::CapabilityEntryList& result) = 0;
    virtual std::future<CommonAPI::CallStatus> lookup1Async(const std::string& domain, const std::string& interfaceName, const types::Types::ProviderQosRequirement& qos, const types::Types::DiscoveryQos& discoveryQos, Lookup1AsyncCallback callback) = 0;

    virtual void lookup2(const std::string& participandId, const types::Types::DiscoveryQos& discoveryQos, CommonAPI::CallStatus& callStatus, types::Types::CapabilityEntryList& result) = 0;
    virtual std::future<CommonAPI::CallStatus> lookup2Async(const std::string& participandId, const types::Types::DiscoveryQos& discoveryQos, Lookup2AsyncCallback callback) = 0;

    virtual void remove(const std::string& participantId, const int64_t& timeout_ms, CommonAPI::CallStatus& callStatus) = 0;
    virtual std::future<CommonAPI::CallStatus> removeAsync(const std::string& participantId, const int64_t& timeout_ms, RemoveAsyncCallback callback) = 0;
};


} // namespace messaging
} // namespace joynr

#endif // JOYNR_MESSAGING_I_Capabilities_PROXY_BASE_H_
