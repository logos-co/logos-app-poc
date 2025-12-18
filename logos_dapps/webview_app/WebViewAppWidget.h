#pragma once

#include <QWidget>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QUrl>

class QPushButton;
class QLabel;
class QQuickView;
class QQuickItem;

class LogosAPI;
class LogosAPIClient;

class WebViewAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit WebViewAppWidget(LogosAPI* logosAPI = nullptr, QWidget* parent = nullptr);
    ~WebViewAppWidget();
    
    Q_INVOKABLE void handleLogosRequest(const QString& moduleName, const QString& methodName, const QVariantList& args, int requestId);
    
    Q_INVOKABLE void qmlReady();

private slots:
    void onWikipediaClicked();
    void onLocalFileClicked();
    void onSendToWebAppClicked();

private:
    QQuickView* m_quickView;
    QQuickItem* m_rootItem;
    QPushButton* m_wikipediaButton;
    QPushButton* m_localFileButton;
    QPushButton* m_sendToWebAppButton;
    QLabel* m_statusLabel;
    QUrl m_pendingUrl;
    bool m_qmlReady;
    LogosAPI* m_logosAPI;
    
    void setupUI();
    void loadURL(const QUrl& url);
    void loadLocalHtml();
    void runJavaScript(const QString& script);
    void sendResponseToJS(int requestId, const QVariant& result = QVariant(), const QString& error = QString());
    void sendEventToJS(const QString& eventName, const QVariantMap& data);
};
