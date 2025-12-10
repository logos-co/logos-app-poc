#include "MainContainer.h"
#include "MainUIBackend.h"
#include "mdiview.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickStyle>
#include <QVBoxLayout>
#include <QDebug>

MainContainer::MainContainer(LogosAPI* logosAPI, QWidget* parent)
    : QWidget(parent)
    , m_logosAPI(logosAPI)
    , m_backend(nullptr)
    , m_sidebarWidget(nullptr)
    , m_contentStack(nullptr)
    , m_mdiView(nullptr)
    , m_contentWidget(nullptr)
    , m_appLauncherWidget(nullptr)
{
    // Set QML style
    QQuickStyle::setStyle("Basic");
    
    // Create backend
    m_backend = new MainUIBackend(m_logosAPI, this);
    
    setupUi();
    
    // Connect view index changes
    connect(m_backend, &MainUIBackend::currentViewIndexChanged, 
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
    
    qDebug() << "MainContainer created";
}

MainContainer::~MainContainer()
{
    qDebug() << "MainContainer destroyed";
}

void MainContainer::setupUi()
{
    // Create main horizontal layout
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // === SIDEBAR (QML) ===
    m_sidebarWidget = new QQuickWidget(this);
    m_sidebarWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_sidebarWidget->rootContext()->setContextProperty("backend", m_backend);
    m_sidebarWidget->setSource(QUrl("qrc:/Sidebar.qml"));
    m_sidebarWidget->setMinimumWidth(80);
    m_sidebarWidget->setMaximumWidth(80);
    
    // === CONTENT AREA (vertical layout with stack + app launcher) ===
    QWidget* contentArea = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create content stack
    m_contentStack = new QStackedWidget(contentArea);
    m_contentStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Index 0: MdiView (C++ widget)
    m_mdiView = new MdiView(m_contentStack);
    m_contentStack->addWidget(m_mdiView);
    
    // Index 1: QML content views (Dashboard, Modules, PackageManager, Settings)
    m_contentWidget = new QQuickWidget(m_contentStack);
    m_contentWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_contentWidget->rootContext()->setContextProperty("backend", m_backend);
    m_contentWidget->setSource(QUrl("qrc:/ContentViews.qml"));
    m_contentStack->addWidget(m_contentWidget);
    
    // App Launcher (QML dock at bottom, only visible when on Apps view)
    m_appLauncherWidget = new QQuickWidget(contentArea);
    m_appLauncherWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_appLauncherWidget->rootContext()->setContextProperty("backend", m_backend);
    m_appLauncherWidget->setSource(QUrl("qrc:/AppLauncher.qml"));
    m_appLauncherWidget->setMinimumHeight(90);
    m_appLauncherWidget->setMaximumHeight(90);
    
    // Add widgets to content layout
    contentLayout->addWidget(m_contentStack, 1);
    contentLayout->addWidget(m_appLauncherWidget);
    
    // Add widgets to main layout
    m_mainLayout->addWidget(m_sidebarWidget);
    m_mainLayout->addWidget(contentArea, 1);
    
    // Set initial state
    m_contentStack->setCurrentIndex(0); // Show MdiView by default
    m_appLauncherWidget->setVisible(true); // Show app launcher on Apps view
    
    // Set reasonable minimum size
    setMinimumSize(800, 600);
}

void MainContainer::onViewIndexChanged()
{
    int viewIndex = m_backend->currentViewIndex();
    
    qDebug() << "MainContainer: View index changed to" << viewIndex;
    
    if (viewIndex == 0) {
        // Apps view - show MdiView (C++ widget)
        m_contentStack->setCurrentIndex(0);
        m_appLauncherWidget->setVisible(true);
    } else {
        // Other views - show QML content
        m_contentStack->setCurrentIndex(1);
        m_appLauncherWidget->setVisible(false);
    }
}

void MainContainer::onNavigateToApps()
{
    // This is called when an app is loaded and we need to switch to Apps view
    m_backend->setCurrentViewIndex(0);
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

