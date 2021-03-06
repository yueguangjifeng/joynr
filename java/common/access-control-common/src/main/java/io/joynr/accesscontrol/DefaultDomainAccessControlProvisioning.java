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
package io.joynr.accesscontrol;

import java.util.Collection;
import java.util.Collections;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;

public class DefaultDomainAccessControlProvisioning implements DomainAccessControlProvisioning {

    @Override
    public Collection<DomainRoleEntry> getDomainRoleEntries() {
        return Collections.emptyList();
    }

    @Override
    public Collection<MasterAccessControlEntry> getMasterAccessControlEntries() {
        return Collections.emptyList();
    }

}
