#include "WebViewAppWidget.h"
#include "WebViewBridge.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QWidget>
#include <QtWebView/QtWebView>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>
#include <QTimer>

namespace {
    class LinuxWebViewWidget : public QWidget {
    public:
        LinuxWebViewWidget(QWidget* parent) : QWidget(parent), m_webViewObject(nullptr), m_bridge(nullptr) {
            // Initialize QtWebView
            QtWebView::initialize();
            
            // Create QQuickWidget for embedding QML WebView
            m_quickWidget = new QQuickWidget(this);
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            
            // Create QML content as a string with message handling
            QString qmlContent = R"(
                import QtQuick 2.15
                import QtWebView 1.15
                
                WebView {
                    id: webView
                    anchors.fill: parent
                    
                    onMessageReceived: function(message) {
                        // Forward messages to C++ via external.postMessage
                        if (typeof external !== 'undefined' && external.postMessage) {
                            external.postMessage(JSON.stringify(message));
                        }
                    }
                }
            )";
            
            // Create QML component from string
            QQmlEngine* engine = m_quickWidget->engine();
            QQmlComponent component(engine);
            component.setData(qmlContent.toUtf8(), QUrl("qrc:/webview.qml"));
            
            if (component.isReady()) {
                QObject* object = component.create(m_quickWidget->rootContext());
                if (object) {
                    m_quickWidget->setContent(QUrl(), "WebViewApp", qmlContent.toUtf8());
                    m_webViewObject = object->findChild<QObject*>("webView");
                } else {
                    qDebug() << "Failed to create QML component:" << component.errorString();
                }
            } else {
                qDebug() << "QML component not ready:" << component.errorString();
                // Fallback: try to set source directly
                m_quickWidget->setSource(QUrl("qrc:/webview.qml"));
            }
            
            // Set up layout
            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(m_quickWidget);
        }
        
        void loadURL(const QUrl& url) {
            if (m_webViewObject) {
                QMetaObject::invokeMethod(m_webViewObject, "load", Q_ARG(QUrl, url));
            } else {
                // Try to find the webView object from root
                QObject* rootObject = m_quickWidget->rootObject();
                if (rootObject) {
                    m_webViewObject = rootObject->findChild<QObject*>("webView");
                    if (m_webViewObject) {
                        QMetaObject::invokeMethod(m_webViewObject, "load", Q_ARG(QUrl, url));
                    } else {
                        // Try setting url property directly
                        rootObject->setProperty("url", url);
                    }
                }
            }
            
            // Inject bridge after a short delay to ensure page is loading
            QTimer::singleShot(500, this, [this]() {
                if (m_bridge) {
                    injectBridge();
                }
            });
        }
        
        void executeJavaScript(const QString& script) {
            if (m_webViewObject) {
                QMetaObject::invokeMethod(m_webViewObject, "runJavaScript", Q_ARG(QString, script));
            } else {
                // Try to find the webView object
                QObject* rootObject = m_quickWidget->rootObject();
                if (rootObject) {
                    m_webViewObject = rootObject->findChild<QObject*>("webView");
                    if (m_webViewObject) {
                        QMetaObject::invokeMethod(m_webViewObject, "runJavaScript", Q_ARG(QString, script));
                    }
                }
            }
        }
        
        void injectBridge(WebViewBridge* bridge) {
            m_bridge = bridge;
            injectBridge();
        }
        
        void injectBridge() {
            if (!m_bridge) return;
            
            QString script = WebViewBridge::generateLogosObjectScript();
            executeJavaScript(script);
        }
        
        void handleMessage(const QString& messageJson) {
            if (!m_bridge) return;
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(messageJson.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug() << "Failed to parse message:" << error.errorString();
                return;
            }
            
            QJsonObject obj = doc.object();
            QString type = obj["type"].toString();
            
            if (type == "logos_request") {
                QString method = obj["method"].toString();
                int requestId = obj["requestId"].toInt();
                QJsonObject paramsObj = obj["params"].toObject();
                
                QVariantMap qParams;
                for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it) {
                    qParams[it.key()] = it.value().toVariant();
                }
                
                m_bridge->handleJavaScriptRequest(method, qParams);
                
                // Send response back
                QJsonObject responseObj;
                responseObj["type"] = "logos_response";
                responseObj["requestId"] = requestId;
                QJsonObject resultObj;
                resultObj["success"] = true;
                responseObj["result"] = resultObj;
                
                QJsonDocument responseDoc(responseObj);
                QString responseScript = QString("window.postMessage(%1, '*');")
                    .arg(QString::fromUtf8(responseDoc.toJson()));
                executeJavaScript(responseScript);
            }
        }
        
    private:
        QQuickWidget* m_quickWidget;
        QObject* m_webViewObject;
        WebViewBridge* m_bridge;
    };
}

// These functions match the declarations in WebViewAppWidget_platform.h
QWidget* createLinuxWebViewWidget(QWidget* parent) {
    return new LinuxWebViewWidget(parent);
}

void loadURLInLinuxWebViewWidget(QWidget* widget, const QUrl& url) {
    LinuxWebViewWidget* linuxWidget = qobject_cast<LinuxWebViewWidget*>(widget);
    if (linuxWidget) {
        linuxWidget->loadURL(url);
    }
}

void injectJavaScriptBridgeInLinuxWebView(QWidget* widget, WebViewBridge* bridge) {
    LinuxWebViewWidget* linuxWidget = qobject_cast<LinuxWebViewWidget*>(widget);
    if (linuxWidget) {
        linuxWidget->injectBridge(bridge);
    }
}

void executeJavaScriptInLinuxWebView(QWidget* widget, const QString& script) {
    LinuxWebViewWidget* linuxWidget = qobject_cast<LinuxWebViewWidget*>(widget);
    if (linuxWidget) {
        linuxWidget->executeJavaScript(script);
    }
}
