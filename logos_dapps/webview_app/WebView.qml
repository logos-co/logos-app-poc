import QtQuick
import QtWebView

Rectangle {
    id: root
    color: "white"
    property bool requestDrainInFlight: false
    property bool pageLoaded: false
    
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
    
    WebView {
        id: webView
        anchors.fill: parent
        
        onLoadingChanged: function(loadRequest) {
            console.log("WebView loading changed:", loadRequest.status);
            root.pageLoaded = (loadRequest.status === WebView.LoadSucceededStatus);
            root.requestDrainInFlight = false;
            if (loadRequest.status === WebView.LoadSucceededStatus) {
                console.log("Page loaded, injecting logos script");
                webView.runJavaScript(
                    "if (!document.getElementById('logos-bridge-script')) { " +
                    "var script = document.createElement('script'); " +
                    "script.id = 'logos-bridge-script'; " +
                    "script.src = 'qrc:/logos-script.js'; " +
                    "script.onload = function() { console.log('logos-script.js loaded'); }; " +
                    "script.onerror = function(e) { console.error('Failed to load logos-script.js', e); }; " +
                    "document.head.appendChild(script); " +
                    "} else { console.log('logos-script.js already injected'); }"
                );
                if (typeof logosScriptContent === "string" && logosScriptContent.length > 0) {
                    webView.runJavaScript(logosScriptContent);
                } else {
                    console.warn("logosScriptContent missing, cannot inline inject logos bridge");
                }
            }
        }

        onUrlChanged: {
            console.log("WebView URL changed to:", url);
        }
    }

    Timer {
        id: logosRequestPump
        interval: 30
        running: true
        repeat: true
        onTriggered: {
            if (!root.pageLoaded)
                return;
            if (root.requestDrainInFlight)
                return;
            if (webView.loading)
                return;

            root.requestDrainInFlight = true;
            try {
                webView.runJavaScript("(function(){ var d = (typeof _qtDrain === 'function') ? _qtDrain : (window.__logosBridge && window.__logosBridge.drain); return d ? d() : []; })();",
                                      function(result) {
                                          root.requestDrainInFlight = false;
                                          if (!result || !result.length)
                                              return;
                                          try {
                                              for (var i = 0; i < result.length; ++i) {
                                                  var payload = result[i];
                                                  var reqId = payload.requestId || payload.id || 0;
                                                  var moduleName = payload.module || payload.plugin || "";
                                                  var methodName = payload.method || "";
                                                  var args = payload.args || [];
                                                  hostWidget.handleLogosRequest(moduleName, methodName, args, reqId);
                                              }
                                          } catch (e) {
                                              console.log("Failed to process logos request payload:", e);
                                          }
                                      });
            } catch (err) {
                root.requestDrainInFlight = false;
                console.log("logosRequestPump failed to run JS:", err);
            }
        }
    }
    
    // Notify C++ when component is ready
    Component.onCompleted: {
        console.log("QML Component completed, notifying C++");
        hostWidget.qmlReady();
    }
}
