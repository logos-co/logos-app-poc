import QtQuick
import QtWebView

Rectangle {
    id: root
    color: "white"
    
    // Functions callable from C++
    function loadUrl(url) {
        console.log("QML loadUrl called:", url);
        webView.url = url;
    }
    
    function loadHtmlContent(html) {
        console.log("QML loadHtmlContent called, html length:", html.length);
        webView.loadHtml(html);
    }
    
    function runScript(script) {
        console.log("QML runScript called");
        webView.runJavaScript(script);
    }
    
    // JavaScript to inject the logos object
    readonly property string logosScript: '
        (function() {
            if (typeof window.logos !== "undefined") return;
            
            let requestIdCounter = 0;
            const pendingRequests = new Map();
            const eventListeners = new Map();
            
            window.addEventListener("message", function(event) {
                if (!event.data) return;
                
                if (event.data.type === "logos_response") {
                    const promise = pendingRequests.get(event.data.requestId);
                    if (promise) {
                        pendingRequests.delete(event.data.requestId);
                        if (event.data.error) {
                            promise.reject(new Error(event.data.error));
                        } else {
                            promise.resolve(event.data.result);
                        }
                    }
                } else if (event.data.type === "logos_event") {
                    const listeners = eventListeners.get(event.data.eventName);
                    if (listeners) {
                        listeners.forEach(function(cb) {
                            try { cb(event.data.data); } catch(e) { console.error(e); }
                        });
                    }
                }
            });
            
            window.logos = {
                request: function(options) {
                    return new Promise(function(resolve, reject) {
                        const requestId = ++requestIdCounter;
                        const method = options.method || options;
                        const params = options.params || {};
                        
                        pendingRequests.set(requestId, { resolve: resolve, reject: reject });
                        
                        // Use title change to communicate with Qt
                        var msg = "LOGOS_REQUEST:" + requestId + ":" + method + ":" + JSON.stringify(params);
                        document.title = msg;
                        
                        setTimeout(function() {
                            if (pendingRequests.has(requestId)) {
                                pendingRequests.delete(requestId);
                                reject(new Error("Request timeout"));
                            }
                        }, 30000);
                    });
                },
                
                on: function(eventName, callback) {
                    if (!eventListeners.has(eventName)) {
                        eventListeners.set(eventName, []);
                    }
                    eventListeners.get(eventName).push(callback);
                },
                
                removeListener: function(eventName, callback) {
                    const listeners = eventListeners.get(eventName);
                    if (listeners) {
                        const idx = listeners.indexOf(callback);
                        if (idx > -1) listeners.splice(idx, 1);
                    }
                }
            };
            
            window.dispatchEvent(new Event("logos#initialized"));
        })();
    '
    
    WebView {
        id: webView
        anchors.fill: parent
        
        onLoadingChanged: function(loadRequest) {
            console.log("WebView loading changed:", loadRequest.status);
            if (loadRequest.status === WebView.LoadSucceededStatus) {
                console.log("Page loaded, injecting logos script");
                webView.runJavaScript(root.logosScript);
            }
        }
        
        onTitleChanged: {
            if (title && title.indexOf("LOGOS_REQUEST:") === 0) {
                console.log("Intercepted logos request via title");
                var content = title.substring(14);
                var firstColon = content.indexOf(":");
                var secondColon = content.indexOf(":", firstColon + 1);
                
                if (firstColon > 0 && secondColon > firstColon) {
                    var requestId = parseInt(content.substring(0, firstColon));
                    var method = content.substring(firstColon + 1, secondColon);
                    var paramsJson = content.substring(secondColon + 1);
                    
                    try {
                        var params = JSON.parse(paramsJson);
                        hostWidget.handleLogosRequest(method, params, requestId);
                    } catch (e) {
                        console.log("Failed to parse params:", e);
                    }
                }
            }
        }
        
        onUrlChanged: {
            console.log("WebView URL changed to:", url);
        }
    }
    
    // Notify C++ when component is ready
    Component.onCompleted: {
        console.log("QML Component completed, notifying C++");
        hostWidget.qmlReady();
    }
}
