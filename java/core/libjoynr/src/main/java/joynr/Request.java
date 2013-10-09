package joynr;

/*
 * #%L
 * joynr::java::messaging::messagingcommon
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

import java.util.Arrays;
import java.util.List;
import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonIgnore;

/**
 * This is a value class that represents a JoynRPC function call as JSON. The class also offers a function to
 * deserialize the parameters as an Object[] using meta information.
 */
public class Request implements JoynrMessageType {

    private String methodName;
    private String requestReplyId;
    private String[] paramDatatypes;
    private Object[] params;

    public Request() {

    }

    public Request(String methodName, Object[] params, String[] paramDatatypes, String requestReplyId) {

        this.params = params;
        if (methodName == null) {
            throw new IllegalArgumentException("cannot create JsonRequest with null method name. requestReplyId: "
                    + requestReplyId);
        }
        this.methodName = methodName;
        if (requestReplyId == null) {
            requestReplyId = UUID.randomUUID().toString();
        }
        this.requestReplyId = requestReplyId;

        this.paramDatatypes = paramDatatypes;

    }

    public Request(String name, Object[] params, Class<?>[] parameterTypes) {
        this(name, params, toDatatypeNames(parameterTypes), null);
    }

    private static String[] toDatatypeNames(Class<?>[] types) {
        if (types == null) {
            return null;
        }

        String[] strings = new String[types.length];
        for (int i = 0; i < types.length; i++) {
            Class<?> type = types[i];
            String typeName = null;
            if (type == null) {
                continue;
            } else if (Boolean.class.isAssignableFrom(type)) {
                typeName = "Boolean";
            } else if (Byte.class.isAssignableFrom(type)) {
                typeName = "Byte";
            } else if (Short.class.isAssignableFrom(type)) {
                typeName = "Short";
            } else if (Integer.class.isAssignableFrom(type)) {
                typeName = "Integer";
            } else if (Long.class.isAssignableFrom(type)) {
                typeName = "Long";
            } else if (Float.class.isAssignableFrom(type)) {
                typeName = "Float";
            } else if (Double.class.isAssignableFrom(type)) {
                typeName = "Double";
            } else if (String.class.isAssignableFrom(type)) {
                typeName = "String";
            } else if (List.class.isAssignableFrom(type)) {
                typeName = "List";
            } else {
                typeName = type.getCanonicalName();
            }
            strings[i] = typeName;
        }
        return strings;
    }

    @JsonIgnore
    public List<String> getFullyQualifiedParamDatatypes() {
        String[] names = paramDatatypes;
        if (names == null) {
            return null;
        }

        String[] fullyQualifiedNames = new String[names.length];
        for (int i = 0; i < names.length; i++) {
            String typeName = names[i];
            String type = fullyQualifiedNameFor(typeName);
            fullyQualifiedNames[i] = type;
        }
        return Arrays.asList(fullyQualifiedNames);
    }

    private String fullyQualifiedNameFor(String typeName) {
        if (typeName == null) {
            return null;
        }

        String fullyQualifiedName = null;
        if (typeName.equals("Boolean")) {
            fullyQualifiedName = Boolean.class.getCanonicalName();
        } else if (typeName.equals("Byte")) {
            fullyQualifiedName = Byte.class.getCanonicalName();
        } else if (typeName.equals("Short")) {
            fullyQualifiedName = Short.class.getCanonicalName();
        } else if (typeName.equals("Integer")) {
            fullyQualifiedName = Integer.class.getCanonicalName();
        } else if (typeName.equals("Long")) {
            fullyQualifiedName = Long.class.getCanonicalName();
        } else if (typeName.equals("Float")) {
            fullyQualifiedName = Float.class.getCanonicalName();
        } else if (typeName.equals("Double")) {
            fullyQualifiedName = Double.class.getCanonicalName();
        } else if (typeName.equals("String")) {
            fullyQualifiedName = String.class.getCanonicalName();
        } else if (typeName.equals("List")) {
            fullyQualifiedName = List.class.getCanonicalName();
        } else {
            fullyQualifiedName = typeName;
        }
        return fullyQualifiedName;
    }

    @JsonIgnore
    public boolean hasParams() {
        return params != null && params.length != 0;
    }

    public String getMethodName() {
        return methodName;
    }

    public Object[] getParams() {
        return params;
    }

    public String[] getParamDatatypes() {
        return paramDatatypes;
    }

    public String getRequestReplyId() {
        return requestReplyId;
    }

    @Override
    public String toString() {
        return "JsonRequest: " + this.methodName + ", requestReplyId: " + requestReplyId + ", params: "
                + Arrays.toString(this.params);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;

        Request other = (Request) obj;

        // always non null
        if (!requestReplyId.equals(other.requestReplyId)) {
            return false;
        }

        // always non-null
        if (!methodName.equals(other.methodName)) {
            return false;
        }

        if (params == null) {
            if (other.params != null) {
                return false;
            }
        } else if (!Arrays.deepEquals(params, other.params)) {
            return false;
        }

        if (paramDatatypes == null) {
            if (other.paramDatatypes != null) {
                return false;
            }
        } else if (!Arrays.deepEquals(paramDatatypes, other.paramDatatypes)) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((requestReplyId == null) ? 0 : requestReplyId.hashCode());
        result = prime * result + ((methodName == null) ? 0 : methodName.hashCode());
        result = prime * result + ((params == null) ? 0 : Arrays.deepHashCode(params));
        result = prime * result + ((paramDatatypes == null) ? 0 : Arrays.deepHashCode(paramDatatypes));
        return result;
    }

    public boolean hasParamDatatypes() {
        if (paramDatatypes == null) {
            return false;
        }

        return paramDatatypes.length > 0;
    }

}
