#pragma once

#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>

class DenyAllNAMFactory : public QQmlNetworkAccessManagerFactory {
public:
    QNetworkAccessManager* create(QObject* parent) override;
};
