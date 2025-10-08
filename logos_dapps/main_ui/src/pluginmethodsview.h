#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QJsonArray>
#include <QJsonObject>

class PluginMethodsView : public QWidget
{
    Q_OBJECT

public:
    explicit PluginMethodsView(const QString& pluginName, QWidget* parent = nullptr);
    ~PluginMethodsView();

signals:
    void backClicked();

private slots:
    void onBackButtonClicked();
    void onMethodItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    void loadPluginMethods();
    void displayPluginMethods(const QJsonArray& methods);

    QString m_pluginName;
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QTreeWidget* m_methodsTree;
    QPushButton* m_backButton;
}; 