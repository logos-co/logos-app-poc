#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class LogosAPI;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);
    explicit Window(LogosAPI* logosAPI, QWidget *parent = nullptr);
    ~Window();

private:
    void setupUi();
    LogosAPI* m_logosAPI;
};

#endif // WINDOW_H 