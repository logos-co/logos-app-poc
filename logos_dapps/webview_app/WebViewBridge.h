#pragma once

#include <QObject>
#include <QVariantMap>
#include <QString>

class WebViewBridge : public QObject {
    Q_OBJECT

public:
    explicit WebViewBridge(QObject* parent = nullptr);
    
    // Handle request from JavaScript
    void handleJavaScriptRequest(const QString& method, const QVariantMap& params);
    
    // Emit event to JavaScript
    void emitEvent(const QString& eventName, const QVariantMap& data);
    
    // Generate the JavaScript code for the logos object (public for platform implementations)
    static QString generateLogosObjectScript();

signals:
    // Emitted when JavaScript sends a request
    void requestReceived(const QString& method, const QVariantMap& params);
    
    // Emitted when C++ wants to send an event to JavaScript
    void eventEmitted(const QString& eventName, const QVariantMap& data);
};
