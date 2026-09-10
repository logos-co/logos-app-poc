#include "FixtureShellHost.h"
#include "ShellSections.h"

#include <QDebug>

FixtureShellHost::FixtureShellHost(const QJsonObject& fixture)
    : m_backend(fixture)
{
    // The sidebar writes the section index straight into the backend; the
    // content stack only follows if the observer hears about it.
    QObject::connect(&m_backend, &FixtureBackend::currentActiveSectionIndexChanged, &m_backend, [this] {
        if (m_observer) m_observer->onSectionIndexChanged(m_backend.currentActiveSectionIndex());
    });
}

QObject* FixtureShellHost::backendObject() { return &m_backend; }

int  FixtureShellHost::currentSectionIndex() const { return m_backend.currentActiveSectionIndex(); }

void FixtureShellHost::setCurrentSectionIndex(int index)
{
    m_backend.setCurrentActiveSectionIndex(index);
    if (m_observer) m_observer->onSectionIndexChanged(index);
}

void FixtureShellHost::loadUiModule(const QString& name)
{
    qInfo() << "FixtureShellHost: loadUiModule" << name
            << "— no plugin loading in the preview host";
}

void FixtureShellHost::unloadUiModule(const QString& name)
{
    qInfo() << "FixtureShellHost: unloadUiModule" << name;
}

void FixtureShellHost::setCurrentVisibleApp(const QString& name)
{
    m_backend.setCurrentVisibleApp(name);
}

QString FixtureShellHost::displayNameFor(const QString& name) const
{
    return m_backend.displayNameFor(name);
}

void FixtureShellHost::setObserver(IShellObserver* observer) { m_observer = observer; }

void FixtureShellHost::replaySection()
{
    // The shell starts on the workspace and only moves on a callback; the
    // fixture's section was set before any observer could hear the signal.
    if (m_observer && m_backend.currentActiveSectionIndex() != ShellSection::Workspace)
        m_observer->onSectionIndexChanged(m_backend.currentActiveSectionIndex());
}
