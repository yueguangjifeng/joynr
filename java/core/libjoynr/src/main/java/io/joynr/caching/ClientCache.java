package io.joynr.caching;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.qos.QualityOfService;

import java.util.NoSuchElementException;

/**
 * This cache is on a per provider basis. 
 */
public interface ClientCache {

    /**
     * Sets the quality of service.
     */
    void setQoS(QualityOfService qos);

    /**
     * Checks if current cached value conforms to the QoS constraints.  
     * If the attribute does not appear in the cache, then it will return false.
     */
    boolean isCacheValueValid(String attributeId);

    /**
     * Returns the value stored for the attribute id, if none exists, throws NoSuchElemntException.
     */
    Object lookUp(String attributeId) throws NoSuchElementException;

    /**
     * Inserts the key (attributeId) and value into the cache.  If the attributeId already has a value, then this overwrites the previous value.
     * Note, this insert does not perform any validation on the value.
     */
    void insert(String attributeId, Object value);

    /**
     * Checks if the entry is still compatible with the requirements of the QoS.
     */
    public void cleanUp();

}
