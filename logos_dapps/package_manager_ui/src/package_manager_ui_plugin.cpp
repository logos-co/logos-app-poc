#include "package_manager_ui_plugin.h"
#include "packagemanagerview.h"

PackageManagerUIPlugin::PackageManagerUIPlugin(QObject* parent)
    : QObject(parent)
    , m_packageManagerView(nullptr)
    , m_logosAPI(nullptr)
{
}

PackageManagerUIPlugin::~PackageManagerUIPlugin()
{
    delete m_packageManagerView;
}

QWidget* PackageManagerUIPlugin::createWidget(LogosAPI* logosAPI)
{
    m_logosAPI = logosAPI;
    if (!m_packageManagerView) {
        m_packageManagerView = new PackageManagerView(m_logosAPI);
    }
    return m_packageManagerView;
}

void PackageManagerUIPlugin::destroyWidget(QWidget* widget)
{
    if (widget == m_packageManagerView) {
        delete m_packageManagerView;
        m_packageManagerView = nullptr;
    }
}
