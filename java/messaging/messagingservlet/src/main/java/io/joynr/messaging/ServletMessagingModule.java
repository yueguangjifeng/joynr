package io.joynr.messaging;

/*
 * #%L
 * joynr::java::messaging::messagingservlet
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

import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.dispatcher.ServletMessageReceiverImpl;

/**
 * Used in conjunction with DefaultDispatcherModule to inject the application side
 * 
 */
public class ServletMessagingModule extends MessagingModule {

    @Override
    protected void configure() {
        super.configure();
        // bind(String.class).annotatedWith(Names.named("joynr.messaging.channelId")).toInstance(channelId);
        bind(MessageListeners.class).to(MessageListenersImpl.class).asEagerSingleton();
        bind(MessageReceiver.class).to(ServletMessageReceiverImpl.class);
        bind(ServletMessageReceiver.class).to(ServletMessageReceiverImpl.class);
    }

}
