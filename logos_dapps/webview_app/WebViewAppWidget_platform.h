#pragma once

#include <QWidget>
#include <QUrl>

// Platform-specific webview creation and URL loading functions
#if defined(Q_OS_MAC)
    QWidget* createMacWebViewWidget(QWidget* parent);
    void loadURLInMacWebViewWidget(QWidget* widget, const QUrl& url);
#elif defined(Q_OS_WIN)
    QWidget* createWinWebViewWidget(QWidget* parent);
    void loadURLInWinWebViewWidget(QWidget* widget, const QUrl& url);
#else
    // Linux
    QWidget* createLinuxWebViewWidget(QWidget* parent);
    void loadURLInLinuxWebViewWidget(QWidget* widget, const QUrl& url);
#endif
