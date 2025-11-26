#pragma once

#include <QWidget>
#include <QVariantMap>

class QPushButton;
class QLabel;
class WebViewBridge;

class WebViewAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit WebViewAppWidget(QWidget* parent = nullptr);
    ~WebViewAppWidget();

private slots:
    void onWikipediaClicked();
    void onLocalFileClicked();
    void onBridgeRequest(const QString& method, const QVariantMap& params);
    void onQtButtonClicked();

private:
    QWidget* webView;
    QPushButton* m_wikipediaButton;
    QPushButton* m_localFileButton;
    QPushButton* m_qtButton;
    QLabel* m_statusLabel;
    WebViewBridge* m_bridge;
    
    void setupWebView();
    void loadLocalHTML();
    void loadURLInWebView(const QUrl& url);
    void executeJavaScript(const QString& script);
    void sendEventToWebApp(const QString& eventName, const QVariantMap& data);
    void injectJavaScriptBridge();
};
