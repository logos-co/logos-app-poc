#ifndef DASHBOARDVIEW_H
#define DASHBOARDVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

class DashboardView : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardView(QWidget *parent = nullptr);
    ~DashboardView();

private:
    void setupUi();

    QVBoxLayout *m_layout;
    QLabel *m_title;
    QLabel *m_contentLabel;
};

#endif // DASHBOARDVIEW_H 