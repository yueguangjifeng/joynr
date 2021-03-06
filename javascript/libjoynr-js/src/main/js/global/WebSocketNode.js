/*eslint global-require: "off"*/
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

/**
 * Adapts the nodejs websocket lib WebSocket-Node to the WebSocket client API.
 * See: http://dev.w3.org/html5/websockets/#the-websocket-interface
 *
 */
const JoynrRuntimeException = require("../joynr/exceptions/JoynrRuntimeException");
const UtilInternal = require("../joynr/util/UtilInternal.js");
const MessageSerializer = require("../joynr/messaging/MessageSerializer");

function useWebSocketNode() {
    if (typeof Buffer !== "function") {
        throw new JoynrRuntimeException("Decoding of binary websocket messages not possible. Buffer not available.");
    }

    /*
     * try to load the native C++ websocket implementation first; only if this fails
     * fall back to JS implementation. Temporarily silence error output for first
     * load attempt.
     */
    let ws;
    try {
        ws = require("wscpp");
    } catch (e) {
        ws = require("ws");
    }

    function WebSocketNodeWrapper(remoteUrl, keychain, useUnencryptedTls) {
        const clientOptions = keychain
            ? {
                  cert: keychain.tlsCert,
                  key: keychain.tlsKey,
                  ca: keychain.tlsCa,
                  rejectUnauthorized: true,
                  useUnencryptedTls
              }
            : undefined;

        const webSocketObj = new ws(remoteUrl, clientOptions);

        webSocketObj.encodeString = function(string) {
            return Buffer.from(string);
        };
        webSocketObj.decodeEventData = function(data) {
            return data;
        };

        webSocketObj.marshalJoynrMessage = function(data) {
            return MessageSerializer.stringify(data);
        };
        webSocketObj.unmarshalJoynrMessage = function(event, callback) {
            const joynrMessage = MessageSerializer.parse(event.data);
            if (joynrMessage) {
                callback(joynrMessage);
            }
        };

        return webSocketObj;
    }

    UtilInternal.extend(WebSocketNodeWrapper, ws);

    return WebSocketNodeWrapper;
}

module.exports = global.window !== undefined ? require("./WebSocket") : useWebSocketNode();
