#ifndef ADDVARDIALOG_H
#define ADDVARDIALOG_H


#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QProcess>
#include <QMap>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QListWidget>
#include <QStackedWidget>

// 用于添加自定义变量的对话框
class AddVarDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddVarDialog(QWidget *parent = nullptr);

    QString varName() const;
    QString varValue() const;

    void setVarName(const QString &name);
    void setValue(const QString &val);
    void setDefinitionSource(const QString &src);
    void setResetEnabled(bool enabled);           // 新增：启用/禁用重置按钮

signals:
    void resetRequested(const QString &varName);  // 新增：重置按钮被点击时发出

private slots:
    void onResetClicked();                        // 新增：重置按钮点击槽

private:
    QLineEdit   *nameEdit;
    QLineEdit   *valueEdit;
    QLabel      *sourceLabel;
    QPushButton *resetBtn;                        // 新增：重置按钮
};
#endif // ADDVARDIALOG_H
