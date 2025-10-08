#include "modulesgenericview.h"
#include <QFont>
#include "mainwindow.h"
#include "modulesview.h"
#include "coremoduleview.h"
#include <QDebug>

ModulesGenericView::ModulesGenericView(QWidget *parent, MainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_modulesView(nullptr)
    , m_coreModuleView(nullptr)
{
    setupUi();
    createTabs();
}

ModulesGenericView::~ModulesGenericView()
{
}

void ModulesGenericView::setupUi()
{
    // Create main layout
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(20);
    m_layout->setContentsMargins(40, 40, 40, 40);
    
    // Create and style the title
    m_titleLabel = new QLabel("Modules", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #ffffff;");
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "   border: 1px solid #3d3d3d;"
        "   background-color: #2d2d2d;"
        "   border-radius: 5px;"
        "}"
        "QTabBar::tab {"
        "   background-color: #1d1d1d;"
        "   color: #cccccc;"
        "   padding: 8px 16px;"
        "   border-top-left-radius: 5px;"
        "   border-top-right-radius: 5px;"
        "   margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "   background-color: #2d2d2d;"
        "   color: #ffffff;"
        "}"
    );
    
    m_layout->addWidget(m_titleLabel);
    m_layout->addWidget(m_tabWidget);
}

void ModulesGenericView::createTabs()
{
    // UI Modules tab (Apps)
    QWidget *uiModulesTab = new QWidget();
    QVBoxLayout *uiModulesLayout = new QVBoxLayout(uiModulesTab);
    uiModulesLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create ModulesView
    m_modulesView = new ModulesView(uiModulesTab, m_mainWindow);
    uiModulesLayout->addWidget(m_modulesView);
    
    // Core Modules tab
    QWidget *coreModulesTab = new QWidget();
    QVBoxLayout *coreModulesLayout = new QVBoxLayout(coreModulesTab);
    coreModulesLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create CoreModuleView
    m_coreModuleView = new CoreModuleView(coreModulesTab);
    coreModulesLayout->addWidget(m_coreModuleView);
    
    // Add tabs to tab widget
    m_tabWidget->addTab(uiModulesTab, "Apps");
    m_tabWidget->addTab(coreModulesTab, "Core Modules");
    
    // Connect tab bar click to refresh the appropriate view
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0 && m_modulesView) {
            // Refresh UI modules tab
            m_modulesView->refreshPluginList();
            qDebug() << "Apps tab selected, refreshing UI modules";
        } else if (index == 1 && m_coreModuleView) {
            // Refresh Core modules tab
            m_coreModuleView->updatePluginList();
            qDebug() << "Core Modules tab selected, refreshing core modules";
        }
    });
}

void ModulesGenericView::refreshModulesView()
{
    if (m_modulesView) {
        m_modulesView->refreshPluginList();
        qDebug() << "ModulesView refreshed";
    }
}