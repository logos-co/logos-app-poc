#pragma once

#include <QObject>

class CounterBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit CounterBackend(QObject* parent = nullptr);

    int count() const;

public slots:
    void increment();

signals:
    void countChanged();

private:
    int m_count = 0;
};
