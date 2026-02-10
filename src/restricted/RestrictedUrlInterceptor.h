#pragma once

#include <QQmlAbstractUrlInterceptor>
#include <QStringList>
#include <QUrl>

class RestrictedUrlInterceptor : public QQmlAbstractUrlInterceptor {
public:
    explicit RestrictedUrlInterceptor(const QStringList& allowedRoots);
    QUrl intercept(const QUrl& url, DataType type) override;

private:
    QStringList m_allowedRoots;
};
