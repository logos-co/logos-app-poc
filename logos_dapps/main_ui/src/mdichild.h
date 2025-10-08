#ifndef MDICHILD_H
#define MDICHILD_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class MdiChild : public QWidget
{
    Q_OBJECT

public:
    MdiChild(QWidget *parent = nullptr);
    ~MdiChild();

private:
    QLabel *contentLabel;
    QVBoxLayout *layout;
};

#endif // MDICHILD_H 