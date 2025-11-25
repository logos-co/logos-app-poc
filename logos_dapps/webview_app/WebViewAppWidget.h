#pragma once

#include <QWidget>

class WebViewAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit WebViewAppWidget(QWidget* parent = nullptr);

private:
    QWidget* webView;
    void setupWebView();
};
