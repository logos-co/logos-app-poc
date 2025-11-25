#include "WebViewAppWidget.h"
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

namespace {
    class LinuxWebViewWidget : public QWidget {
    public:
        LinuxWebViewWidget(QWidget* parent) : QWidget(parent), m_webViewObject(nullptr) {
            // Initialize QtWebView
            QtWebView::initialize();
            
            // Create QQuickWidget for embedding QML WebView
            m_quickWidget = new QQuickWidget(this);
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            
            // Create QML content as a string
            QString qmlContent = R"(
                import QtQuick 2.15
                import QtWebView 1.15
                
                WebView {
                    id: webView
                    anchors.fill: parent
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
        }
        
    private:
        QQuickWidget* m_quickWidget;
        QObject* m_webViewObject;
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
