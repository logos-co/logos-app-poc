#ifndef SIDEBARBUTTON_H
#define SIDEBARBUTTON_H

#include <QPushButton>
#include <QIcon>
#include <QSize>

class SidebarButton : public QPushButton
{
    Q_OBJECT

public:
    explicit SidebarButton(const QString &text, const QIcon &icon, QWidget *parent = nullptr);
    
    // Set whether this button is the active one
    void setActive(bool active);
    bool isActive() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    bool m_active;
};

#endif // SIDEBARBUTTON_H 