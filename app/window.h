#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class LogosAPI;
class QMenu;
class QAction;
class QCloseEvent;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);
    explicit Window(LogosAPI* logosAPI, QWidget *parent = nullptr);
    ~Window();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void showHideWindow();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void quitApplication();

private:
    void setupUi();
    void createTrayIcon();
    void setIcon();
    
    LogosAPI* m_logosAPI;
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayIconMenu;
    QAction* m_showHideAction;
    QAction* m_quitAction;
};

#endif // WINDOW_H 