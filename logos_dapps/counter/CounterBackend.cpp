#include "CounterBackend.h"

CounterBackend::CounterBackend(QObject* parent)
    : QObject(parent)
{
}

int CounterBackend::count() const
{
    return m_count;
}

void CounterBackend::increment()
{
    m_count++;
    emit countChanged();
}
