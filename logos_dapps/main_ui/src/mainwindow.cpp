#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QSpacerItem>
#include <QFont>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QLabel>
#include <QDebug>
#include "modulesview.h"
#include "dashboardview.h"
#include "coremoduleview.h"
#include "packagemanagerview.h"
#include "modulesgenericview.h"
#include "applauncher.h"
#include "logos_api.h"
#include "token_manager.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_logosAPI(nullptr)
{
    // Create own LogosAPI instance if none provided
    m_logosAPI = new LogosAPI("core", this);
    
    setupUi();
    createSidebar();
    createContentPages();
    
    // Set the first button as active by default
    if (!m_sidebarButtons.isEmpty()) {
        m_sidebarButtons[0]->setActive(true);
        m_contentStack->setCurrentIndex(0);
    }

    qDebug() << "===> mainwindow: LogosAPI: printing keys";
    QList<QString> keys = m_logosAPI->getTokenManager()->getTokenKeys();
    for (const QString& key : keys) {
       qDebug() << "===> mainwindow: LogosAPI: Token key:" << key << "value:" << m_logosAPI->getTokenManager()->getToken(key);
    }
}

MainWindow::MainWindow(LogosAPI* logosAPI, QWidget *parent)
    : QWidget(parent)
    , m_logosAPI(logosAPI)
{
    setupUi();
    createSidebar();
    createContentPages();
    
    // Set the first button as active by default
    if (!m_sidebarButtons.isEmpty()) {
        m_sidebarButtons[0]->setActive(true);
        m_contentStack->setCurrentIndex(0);
    }

    if (m_logosAPI) {
        qDebug() << "===> mainwindow: LogosAPI: printing keys";
        QList<QString> keys = m_logosAPI->getTokenManager()->getTokenKeys();
        for (const QString& key : keys) {
           qDebug() << "===> mainwindow: LogosAPI: Token key:" << key << "value:" << m_logosAPI->getTokenManager()->getToken(key);
        }
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    // Create main layout
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create sidebar frame
    m_sidebar = new QFrame(this);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFrameShape(QFrame::NoFrame);
    m_sidebar->setMinimumWidth(80);
    m_sidebar->setMaximumWidth(80);
    
    // Use a dark background for the sidebar
    QString sidebarStyle = QString("QFrame#sidebar { "
                                  "background-color: #2D2D2D; "
                                  "border: none; "
                                  "}");
    m_sidebar->setStyleSheet(sidebarStyle);
    
    m_sidebarLayout = new QVBoxLayout(m_sidebar);
    m_sidebarLayout->setSpacing(15);
    m_sidebarLayout->setContentsMargins(0, 20, 0, 20);
    m_sidebarLayout->setAlignment(Qt::AlignCenter);
    
    // Create content area with vertical layout (content stack + app launcher)
    QWidget* contentArea = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create content stack
    m_contentStack = new QStackedWidget(contentArea);
    
    // Enable resizing for the widget and contentStack
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_contentStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create app launcher
    m_appLauncher = new AppLauncher(contentArea, this);
    
    // Add content stack and launcher to content layout
    contentLayout->addWidget(m_contentStack);
    contentLayout->addWidget(m_appLauncher);
    
    // Add widgets to main layout
    m_mainLayout->addWidget(m_sidebar);
    m_mainLayout->addWidget(contentArea);
    
    // Set a reasonable minimum size for the whole window
    setMinimumSize(800, 600);
}

void MainWindow::createSidebar()
{
    // Create sidebar buttons with icons
    struct ButtonInfo {
        QString text;
        QString iconPath;
    };
    
    ButtonInfo buttonInfos[] = {
        {"Apps", ":/icons/home.png"},
        {"Dashboard", ":/icons/chart.png"},
        {"Modules", ":/icons/modules.png"},
        {"Package Manager", ":/icons/modules.png"}, // Reusing modules icon for now
        {"Settings", ":/icons/settings.png"}
    };
    
    for (const auto &info : buttonInfos) {
        SidebarButton *button = new SidebarButton(info.text, QIcon(info.iconPath), m_sidebar);
        connect(button, &SidebarButton::clicked, this, &MainWindow::onSidebarButtonClicked);
        m_sidebarLayout->addWidget(button);
        m_sidebarButtons.append(button);
    }
    
    // Add spacer at the bottom to push buttons to the top
    m_sidebarLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void MainWindow::createContentPages()
{
    // Create MDI View as the first page
    m_mdiView = new MdiView();
    m_contentStack->addWidget(m_mdiView);
    
    // Dashboard page with plugin list
    DashboardView *dashboardView = new DashboardView();
    m_contentStack->addWidget(dashboardView);
    
    // Modules page (contains both Apps and Core Modules tabs)
    m_modulesGenericView = new ModulesGenericView(nullptr, this);
    m_contentStack->addWidget(m_modulesGenericView);
    
    // Connect ModulesView signals to AppLauncher and set ModulesView reference
    if (m_modulesGenericView && m_modulesGenericView->getModulesView() && m_appLauncher) {
        ModulesView* modulesView = m_modulesGenericView->getModulesView();
        m_appLauncher->setModulesView(modulesView);
        connect(modulesView, &ModulesView::appStateChanged, m_appLauncher, &AppLauncher::updateAppState);
    }
    
    // Package Manager page
    PackageManagerView *packageManagerView = new PackageManagerView(m_logosAPI);
    packageManagerView->setMainWindow(this);
    m_contentStack->addWidget(packageManagerView);
    
    // Settings page
    QWidget *settingsPage = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    
    QLabel *settingsTitle = new QLabel("Settings", settingsPage);
    QFont titleFont = settingsTitle->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    settingsTitle->setFont(titleFont);
    settingsTitle->setAlignment(Qt::AlignCenter);
    
    QLabel *settingsContent = new QLabel("This is the Settings content area.", settingsPage);
    settingsContent->setAlignment(Qt::AlignCenter);
    
    settingsLayout->addWidget(settingsTitle);
    settingsLayout->addWidget(settingsContent);
    settingsLayout->addStretch();
    
    m_contentStack->addWidget(settingsPage);
}

void MainWindow::refreshCoreModuleView()
{
    // Get the CoreModuleView from ModulesGenericView
    if (m_modulesGenericView && m_modulesGenericView->getCoreModuleView()) {
        m_modulesGenericView->getCoreModuleView()->updatePluginList();
        qDebug() << "CoreModuleView refreshed from ModulesGenericView";
    }
}

void MainWindow::refreshModulesView()
{
    if (m_modulesGenericView) {
        // Call the refreshModulesView method which refreshes the ModulesView tab
        m_modulesGenericView->refreshModulesView();
        qDebug() << "ModulesView refreshed";
    }
}

void MainWindow::onSidebarButtonClicked()
{
    SidebarButton *clickedButton = qobject_cast<SidebarButton*>(sender());
    if (!clickedButton)
        return;
    
    // Deactivate all buttons
    for (SidebarButton *button : m_sidebarButtons) {
        button->setActive(false);
    }
    
    // Activate the clicked button
    clickedButton->setActive(true);
    
    // Switch to the corresponding content page
    int index = m_sidebarButtons.indexOf(clickedButton);
    if (index >= 0 && index < m_contentStack->count()) {
        m_contentStack->setCurrentIndex(index);
        
        // If Modules view is selected, refresh it
        if (clickedButton->text() == "Modules") {
            refreshModulesView();
            
            // Check if there's a CoreModuleView and refresh it too since it's on one of the tabs
            refreshCoreModuleView();
        }
    }
} 
