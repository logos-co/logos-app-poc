#pragma once

#include <QWidget>
#include <QVariantMap>
#include <QUrl>

class QPushButton;
class QLabel;
class QQuickView;
class QQuickItem;

class WebViewAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit WebViewAppWidget(QWidget* parent = nullptr);
    ~WebViewAppWidget();
    
    // Invokable method for QML to call when logos request is received
    Q_INVOKABLE void handleLogosRequest(const QString& method, const QVariantMap& params, int requestId);
    
    // Called by QML when ready
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
    
    void setupUI();
    void loadURL(const QUrl& url);
    void loadLocalHtml();
    void runJavaScript(const QString& script);
    void sendResponseToJS(int requestId, const QVariantMap& result);
    void sendEventToJS(const QString& eventName, const QVariantMap& data);
};
