#ifndef ENVVIEWER_H
#define ENVVIEWER_H

#include <QWidget>
#include <QMainWindow>
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





// 主窗口
class EnvViewer : public QWidget
{
    Q_OBJECT

public:
    explicit EnvViewer(QWidget *parent = nullptr);
    ~EnvViewer();

private slots:
    void loadEnvironment();
    void readProcessOutput();
    void addVariable();                // 新增：弹出添加变量对话框
    void deleteVariable();   // 新增：删除选中的自定义变量
    void editVariable();
    void resetSystemVariable(const QString &name);   // 新增
    void onPathAdded(const QString &varName, const QString &path);
    void onCustomPathsChanged(const QString &varName, const QStringList &customPaths);
    void filterTable(const QString &text);   // 搜索过滤槽
    void onTableCellDoubleClicked(int row, int column);//双击操作
    void onTableContextMenu(const QPoint &pos);
    void copyVariableName();
    void copyVariableValue();
    void exportCustomVars();
    void importCustomVars();


private:
    void updateTableDisplay();         // 新增：根据系统变量和自定义变量刷新表格
    void loadCustomVarsFromFile();   // 从 ~/.path 加载用户自定义变量
    bool hasExportInFile(const QString &varName);
    void loadSettings();                  // 从配置文件加载 pathFilePath
    void saveSettings();                  // 保存当前 pathFilePath
    void updateBashrcSource(const QString &filePath);
    void sortPathFile();   // 对 .path 文件进行排序和分类


    QTableWidget *table;
    //QPushButton  *refreshBtn;
    QPushButton  *addBtn;              // 新增：添加变量按钮
    QPushButton *deleteBtn;  // 新增：删除按钮
    QPushButton *editBtn;
    QProcess     *process;
    QMap<QString, QString> systemVars; // 新增：保存从 env 获取的系统变量
    QMap<QString, QString> customVars; // 新增：用户自定义的变量
    QString pathFilePath;   // 存储 ~/.path 的完整路径
    QLineEdit *searchEdit;                   // 搜索框
};

#endif // ENVVIEWER_H
