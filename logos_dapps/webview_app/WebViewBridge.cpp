#include "WebViewBridge.h"
#include <QDebug>

WebViewBridge::WebViewBridge(QObject* parent)
    : QObject(parent)
{
}

void WebViewBridge::handleJavaScriptRequest(const QString& method, const QVariantMap& params) {
    qDebug() << "WebViewBridge: Received request - method:" << method << "params:" << params;
    emit requestReceived(method, params);
}

void WebViewBridge::emitEvent(const QString& eventName, const QVariantMap& data) {
    qDebug() << "WebViewBridge: Emitting event - name:" << eventName << "data:" << data;
    emit eventEmitted(eventName, data);
}

QString WebViewBridge::generateLogosObjectScript() {
    return R"(
        (function() {
            if (typeof window.logos !== 'undefined') {
                return; // Already injected
            }
            
            let requestIdCounter = 0;
            const pendingRequests = new Map();
            const eventListeners = new Map();
            
            // Message handler for receiving messages from C++
            window.addEventListener('message', function(event) {
                if (event.data && event.data.type === 'logos_response') {
                    const { requestId, result, error } = event.data;
                    const promise = pendingRequests.get(requestId);
                    if (promise) {
                        pendingRequests.delete(requestId);
                        if (error) {
                            promise.reject(new Error(error));
                        } else {
                            promise.resolve(result);
                        }
                    }
                } else if (event.data && event.data.type === 'logos_event') {
                    const { eventName, data } = event.data;
                    const listeners = eventListeners.get(eventName);
                    if (listeners) {
                        listeners.forEach(callback => {
                            try {
                                callback(data);
                            } catch (e) {
                                console.error('Error in logos event listener:', e);
                            }
                        });
                    }
                }
            });
            
            // Send message to C++ via postMessage
            function sendToNative(message) {
                if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.logos) {
                    // macOS WKWebView - convert to NSDictionary-compatible format
                    const nsMessage = {
                        type: message.type,
                        requestId: message.requestId,
                        method: message.method,
                        params: message.params || {}
                    };
                    window.webkit.messageHandlers.logos.postMessage(nsMessage);
                } else if (window.chrome && window.chrome.webview && window.chrome.webview.postMessage) {
                    // Windows WebView2
                    window.chrome.webview.postMessage(JSON.stringify(message));
                } else if (window.external && window.external.postMessage) {
                    // Linux/QML WebView
                    window.external.postMessage(JSON.stringify(message));
                } else {
                    console.warn('Logos bridge not available');
                }
            }
            
            // Create the logos object
            window.logos = {
                // Request method - similar to ethereum.request()
                request: function(options) {
                    return new Promise((resolve, reject) => {
                        const requestId = ++requestIdCounter;
                        const method = options.method || options;
                        const params = options.params || (typeof options === 'string' ? [] : options);
                        
                        pendingRequests.set(requestId, { resolve, reject });
                        
                        sendToNative({
                            type: 'logos_request',
                            requestId: requestId,
                            method: method,
                            params: params
                        });
                        
                        // Timeout after 30 seconds
                        setTimeout(() => {
                            if (pendingRequests.has(requestId)) {
                                pendingRequests.delete(requestId);
                                reject(new Error('Request timeout'));
                            }
                        }, 30000);
                    });
                },
                
                // Listen to events - similar to ethereum.on()
                on: function(eventName, callback) {
                    if (typeof callback !== 'function') {
                        throw new Error('Callback must be a function');
                    }
                    
                    if (!eventListeners.has(eventName)) {
                        eventListeners.set(eventName, []);
                    }
                    eventListeners.get(eventName).push(callback);
                },
                
                // Remove event listener
                removeListener: function(eventName, callback) {
                    const listeners = eventListeners.get(eventName);
                    if (listeners) {
                        const index = listeners.indexOf(callback);
                        if (index > -1) {
                            listeners.splice(index, 1);
                        }
                    }
                },
                
                // Remove all listeners for an event
                removeAllListeners: function(eventName) {
                    eventListeners.delete(eventName);
                }
            };
            
            // Dispatch ready event
            if (document.readyState === 'loading') {
                document.addEventListener('DOMContentLoaded', function() {
                    window.dispatchEvent(new Event('logos#initialized'));
                });
            } else {
                window.dispatchEvent(new Event('logos#initialized'));
            }
        })();
    )";
}
