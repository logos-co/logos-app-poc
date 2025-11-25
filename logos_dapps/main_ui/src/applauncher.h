#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QPixmap>

class MainWindow;
class ModulesView;

class AppLauncher : public QWidget
{
    Q_OBJECT

public:
    explicit AppLauncher(QWidget *parent = nullptr, MainWindow *mainWindow = nullptr);
    ~AppLauncher();
    
    // Refresh the app list from ModulesView
    void refreshAppList();
    
    // Set the ModulesView reference (called after ModulesView is created)
    void setModulesView(ModulesView* modulesView);
    
    // Update the state of a specific app (show/hide dot indicator)
    void updateAppState(const QString& appName, bool isOpen);

private slots:
    void onAppClicked(const QString& appName);

private:
    void setupUi();
    void clearAppList();
    QPixmap loadAppIcon(const QString& appName);
    
    MainWindow* m_mainWindow;
    ModulesView* m_modulesView;
    
    QHBoxLayout* m_layout;
    QMap<QString, QPushButton*> m_appButtons;
    QMap<QString, QLabel*> m_dotIndicators;
    QMap<QString, QWidget*> m_appContainers;
};
