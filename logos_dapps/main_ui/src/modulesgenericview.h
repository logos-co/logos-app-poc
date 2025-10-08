#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>

class MainWindow;
class ModulesView;
class CoreModuleView;

class ModulesGenericView : public QWidget
{
    Q_OBJECT

public:
    explicit ModulesGenericView(QWidget *parent = nullptr, MainWindow *mainWindow = nullptr);
    ~ModulesGenericView();
    
    // Getter for the CoreModuleView
    CoreModuleView* getCoreModuleView() const { return m_coreModuleView; }
    
    // Getter for the ModulesView
    ModulesView* getModulesView() const { return m_modulesView; }
    
    // Set the current tab index
    void setCurrentTab(int index) { m_tabWidget->setCurrentIndex(index); }
    
    // Refresh the ModulesView
    void refreshModulesView();

private:
    void setupUi();
    void createTabs();

    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    QTabWidget *m_tabWidget;
    MainWindow *m_mainWindow;
    
    ModulesView *m_modulesView;
    CoreModuleView *m_coreModuleView;
}; 