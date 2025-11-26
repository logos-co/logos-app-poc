#pragma once

#include <QWidget>

class QPushButton;

class WebViewAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit WebViewAppWidget(QWidget* parent = nullptr);

private slots:
    void onWikipediaClicked();
    void onLocalFileClicked();

private:
    QWidget* webView;
    QPushButton* m_wikipediaButton;
    QPushButton* m_localFileButton;
    void setupWebView();
    void loadLocalHTML();
    void loadURLInWebView(const QUrl& url);
};
