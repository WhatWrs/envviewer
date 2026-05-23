#ifndef PATHVIEWDIALOG_H
#define PATHVIEWDIALOG_H

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
#include <QInputDialog>

class PathViewDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PathViewDialog(const QString &varName,
                            const QStringList &originalPaths,   // 系统原始路径
                            const QStringList &allPaths,        // 当前所有路径
                            bool hasCustomized,
                            QWidget *parent = nullptr);
signals:
    void resetRequested(const QString &varName);
    void pathAdded(const QString &varName, const QString &path);
    void customPathsChanged(const QString &varName, const QStringList &customPaths);


private slots:
    void onAddClicked();
    void onDeleteClicked();   // 新增：删除选中路径
    void onEditClicked();

private:
    QString     m_varName;
    QStringList m_systemPaths;   // 系统原始路径
    //QStringList m_originalPaths;          // 打开对话框时的路径列表
    QListWidget *listWidget;
    QLineEdit   *newPathEdit;
    bool         m_hasCustomized;
};

#endif // PATHVIEWDIALOG_H
