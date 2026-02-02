#include "MainContainer.h"
#include "MainUIBackend.h"
#include "mdiview.h"

#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QVBoxLayout>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QQuickItem>
#include <QProcessEnvironment>
#include <QColor>
#include <QPalette>

MainContainer::MainContainer(LogosAPI* logosAPI, QWidget* parent)
    : QWidget(parent)
    , m_logosAPI(logosAPI)
    , m_backend(nullptr)
    , m_sidebarWidget(nullptr)
    , m_contentStack(nullptr)
    , m_mdiView(nullptr)
    , m_contentWidget(nullptr)
{
    // Set QML style
    QQuickStyle::setStyle("Basic");
    
    // Create backend
    m_backend = new MainUIBackend(m_logosAPI, this);
    
    setupUi();
    
    // Connect section index changes
    connect(m_backend, &MainUIBackend::currentActiveSectionIndexChanged, 
            this, &MainContainer::onViewIndexChanged);
    connect(m_backend, &MainUIBackend::navigateToApps, 
            this, &MainContainer::onNavigateToApps);
    
    // Connect plugin window signals to MdiView
    connect(m_backend, &MainUIBackend::pluginWindowRequested,
            this, &MainContainer::onPluginWindowRequested);
    connect(m_backend, &MainUIBackend::pluginWindowRemoveRequested,
            this, &MainContainer::onPluginWindowRemoveRequested);
    connect(m_backend, &MainUIBackend::pluginWindowActivateRequested,
            this, &MainContainer::onPluginWindowActivateRequested);

    // When user closes a plugin window (tab/window X), notify backend to unload
    connect(m_mdiView, &MdiView::pluginWindowClosed,
            m_backend, &MainUIBackend::onPluginWindowClosed);

    // Connect to QML signals from SidebarPanel
    QObject* sidebarRoot = m_sidebarWidget->rootObject();
    if (sidebarRoot) {
        connect(sidebarRoot, SIGNAL(launchUIModule(QString)),
                m_backend, SLOT(onAppLauncherClicked(QString)));
        connect(sidebarRoot, SIGNAL(updateLauncherIndex(int)),
                m_backend, SLOT(setCurrentActiveSectionIndex(int)));
    }

    qDebug() << "MainContainer created";
}

MainContainer::~MainContainer()
{
    qDebug() << "MainContainer destroyed";
}

// Using this function to load qml files from local path instead of qrc
QUrl MainContainer::resolveQmlUrl(const QString& qmlFile)
{
    QString qmlUiPath =  QProcessEnvironment::systemEnvironment().value("QML_UI", "");

    if (!qmlUiPath.isEmpty()) {
        QDir qmlDir(qmlUiPath);
        QString fullPath = qmlDir.absoluteFilePath(qmlFile);

        if (QFile::exists(fullPath)) {
            qDebug() << "Loading from filesystem " << fullPath;
            return QUrl::fromLocalFile(fullPath);
        }
    }

    qDebug() << "Loading from resources " << qmlFile;
    QString resourcePath = "qrc:/" + qmlFile;
    return QUrl(resourcePath);
}

void MainContainer::setupUi()
{
    // We would likely move this to qml and use Logos.DesignSystem instead
    QColor bgColor("#171717");
    // set background color
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(QPalette::Window, bgColor);
    setPalette(p);

    // Create main horizontal layout
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(8, 4, 8, 4);
    // When QML_UI is set, add it to each QML engine's import path so nested
    // components (e.g. SidebarIconButton) load from disk — no rebuild for UI changes.
    QString qmlUiPath = QProcessEnvironment::systemEnvironment().value("QML_UI", "");

    // === SIDEBAR (QML) ===
    m_sidebarWidget = new QQuickWidget(this);
    m_sidebarWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    if (!qmlUiPath.isEmpty()) {
        QString absPath = QDir(qmlUiPath).absolutePath();
        m_sidebarWidget->engine()->addImportPath(absPath + "/qml");
        m_sidebarWidget->engine()->addImportPath(absPath);
        qDebug() << "DEV MODE: Added QML import paths:" << absPath + "/qml" << absPath;
    } else {
        m_sidebarWidget->engine()->addImportPath("qrc:/qml");
    }
    qDebug() << "Sidebar engine import paths:" << m_sidebarWidget->engine()->importPathList();
    m_sidebarWidget->rootContext()->setContextProperty("backend", m_backend);
    m_sidebarWidget->setSource(resolveQmlUrl("qml/panels/SidebarPanel.qml"));
    m_sidebarWidget->setMinimumWidth(80);
    m_sidebarWidget->setMaximumWidth(80);
    // set clear color to sidebar so that rounded corners don't show white
    m_sidebarWidget->setClearColor(bgColor);

    // === CONTENT AREA (vertical layout with stack + app launcher) ===
    QWidget* contentArea = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(8, 0, 0, 0);
    // Create content stack
    m_contentStack = new QStackedWidget(contentArea);
    m_contentStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Index 0: MdiView (C++ widget)
    m_mdiView = new MdiView(m_contentStack);
    m_contentStack->addWidget(m_mdiView);
    
    // Index 1: QML content views (Dashboard, Modules, PackageManager, Settings)
    m_contentWidget = new QQuickWidget(m_contentStack);
    m_contentWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    if (!qmlUiPath.isEmpty()) {
        QString absPath = QDir(qmlUiPath).absolutePath();
        m_contentWidget->engine()->addImportPath(absPath + "/qml");
        m_contentWidget->engine()->addImportPath(absPath);
    } else {
        m_contentWidget->engine()->addImportPath("qrc:/qml");
    }
    m_contentWidget->rootContext()->setContextProperty("backend", m_backend);
    m_contentWidget->setSource(resolveQmlUrl("qml/views/ContentViews.qml"));
    m_contentStack->addWidget(m_contentWidget);
    
    // Add widgets to content layout
    contentLayout->addWidget(m_contentStack, 1);

    // Add widgets to main layout
    m_mainLayout->addWidget(m_sidebarWidget);
    m_mainLayout->addWidget(contentArea, 1);
    
    // Set initial state
    m_contentStack->setCurrentIndex(0); // Show MdiView by default
    
    // Set reasonable minimum size
    setMinimumSize(800, 600);
}

void MainContainer::onViewIndexChanged()
{
    int sectionIndex = m_backend->currentActiveSectionIndex();
    
    qDebug() << "MainContainer: Active section index changed to" << sectionIndex;
    
    // Index 0 = Apps (show MdiView), Indices 1-3 = Dashboard/Modules/Settings (show QML)
    if (sectionIndex == 0) {
        // Apps workspace - show MdiView (C++ widget)
        m_contentStack->setCurrentIndex(0);
    } else {
        // Dashboard, Modules, or Settings - show QML content
        m_contentStack->setCurrentIndex(1);
    }
}

void MainContainer::onNavigateToApps()
{
    // This is called when an app is loaded and we need to switch to Apps view
    m_backend->setCurrentActiveSectionIndex(0);
}

void MainContainer::onPluginWindowRequested(QWidget* widget, const QString& title)
{
    if (m_mdiView && widget) {
        m_mdiView->addPluginWindow(widget, title);
        qDebug() << "MainContainer: Added plugin window to MdiView:" << title;
    }
}

void MainContainer::onPluginWindowRemoveRequested(QWidget* widget)
{
    if (m_mdiView && widget) {
        m_mdiView->removePluginWindow(widget);
        qDebug() << "MainContainer: Removed plugin window from MdiView";
    }
}

void MainContainer::onPluginWindowActivateRequested(QWidget* widget)
{
    if (m_mdiView && widget) {
        m_mdiView->activatePluginWindow(widget);
        qDebug() << "MainContainer: Activated plugin window in MdiView";
    }
}

