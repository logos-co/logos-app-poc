#pragma once

#include <QWidget>
#include <QUrl>
#include <QString>

class WebViewBridge;

// Platform-specific webview creation and URL loading functions
#if defined(Q_OS_MAC)
    QWidget* createMacWebViewWidget(QWidget* parent);
    void loadURLInMacWebViewWidget(QWidget* widget, const QUrl& url);
    void injectJavaScriptBridgeInMacWebView(QWidget* widget, WebViewBridge* bridge);
    void executeJavaScriptInMacWebView(QWidget* widget, const QString& script);
#elif defined(Q_OS_WIN)
    QWidget* createWinWebViewWidget(QWidget* parent);
    void loadURLInWinWebViewWidget(QWidget* widget, const QUrl& url);
    void injectJavaScriptBridgeInWinWebView(QWidget* widget, WebViewBridge* bridge);
    void executeJavaScriptInWinWebView(QWidget* widget, const QString& script);
#else
    // Linux
    QWidget* createLinuxWebViewWidget(QWidget* parent);
    void loadURLInLinuxWebViewWidget(QWidget* widget, const QUrl& url);
    void injectJavaScriptBridgeInLinuxWebView(QWidget* widget, WebViewBridge* bridge);
    void executeJavaScriptInLinuxWebView(QWidget* widget, const QString& script);
#endif
