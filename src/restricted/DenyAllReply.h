#pragma once

#include <QNetworkReply>
#include <QNetworkRequest>

// Network reply that immediately fails to prevent any QML network usage.
class DenyAllReply : public QNetworkReply {
public:
    DenyAllReply(const QNetworkRequest& request, QObject* parent);

    void abort() override;
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;
};
