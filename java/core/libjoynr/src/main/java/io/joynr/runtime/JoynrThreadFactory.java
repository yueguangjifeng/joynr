/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.runtime;

import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

public class JoynrThreadFactory implements ThreadFactory {
    private final ThreadFactory defaultThreadFactory = Executors.defaultThreadFactory();
    private AtomicInteger threadCount;
    private String namePrefix;
    private boolean daemon;

    public JoynrThreadFactory() {
        threadCount = new AtomicInteger(0);
        namePrefix = "joynr-";
        daemon = false;
    }

    public JoynrThreadFactory(String name, boolean daemon) {
        this();
        this.namePrefix += name + "-";
        this.daemon = daemon;
    }

    @Override
    public Thread newThread(Runnable runnable) {
        Thread newThread = defaultThreadFactory.newThread(runnable);
        newThread.setName(namePrefix + threadCount.getAndIncrement());
        newThread.setDaemon(daemon);
        return newThread;
    }

}
