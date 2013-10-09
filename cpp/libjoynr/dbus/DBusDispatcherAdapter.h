/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef DBUSDISPATCHERADAPTER_H
#define DBUSDISPATCHERADAPTER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/IDispatcher.h"
#include "joynr/IDbusSkeletonWrapper.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/IMessaging.h"

#include "joynr/JoynrExport.h"

namespace joynr {

class JOYNR_EXPORT DBusDispatcherAdapter : public IMessaging
{
public:
    DBusDispatcherAdapter(IDispatcher& dispatcher, QString dbusAddress);

    ~DBusDispatcherAdapter();

    virtual void transmit(JoynrMessage& message, const MessagingQos& qos);

private:
    DISALLOW_COPY_AND_ASSIGN(DBusDispatcherAdapter);
    IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>* dbusSkeletonWrapper;
    IDispatcher& dispatcher;
};


} // namespace joynr
#endif // DBUSDISPATCHERADAPTER_H
