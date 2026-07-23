
#include "head.h"
#include "qapplication.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMenuBar>
#include <QSettings>
#include <QFileInfo>
#include <algorithm>
#include <QClipboard>

// --------------------- EnvViewer 实现 ---------------------
EnvViewer::EnvViewer(QWidget *parent)
    : QWidget(parent)
{
    if(!QFileInfo::exists(QDir::homePath() + "/.config/EnvViewer/EnvViewer.conf"))
    {
        saveSettings();
    }
    loadSettings();
     // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);

    QMenu *prefMenu = menuBar->addMenu("偏好设置");
    QAction *prefAction = prefMenu->addAction("偏好设置");
    connect(prefAction, &QAction::triggered, this, [this]() {
        PreferencesDialog dlg(pathFilePath, this);
        if (dlg.exec() == QDialog::Accepted) {
            pathFilePath = dlg.filePath();
            saveSettings();
            updateBashrcSource(pathFilePath);
            // 重新加载变量（因为文件可能变化）
            loadCustomVarsFromFile();
            updateTableDisplay();
        }
    });


    setWindowTitle("Linux 系统环境变量查看器");
    resize(750, 500);

    // 表格：三列
    table = new QTableWidget(0, 3, this);
    table->setHorizontalHeaderLabels(QStringList() << "变量名" << "值" << "定义者");
    // 设置列宽模式：前两列自动拉伸，第三列固定宽度
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QWidget::customContextMenuRequested,
            this, &EnvViewer::onTableContextMenu);

    // 按钮
    addBtn = new QPushButton("添加变量", this);
    deleteBtn = new QPushButton("删除变量", this);
    editBtn = new QPushButton("编辑变量", this);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(deleteBtn);   // 新增
    //btnLayout->addWidget(refreshBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMenuBar(menuBar);
    mainLayout->addWidget(table);
    mainLayout->addLayout(btnLayout);
   // mainLayout->addWidget(menuBar);
    //搜索功能
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("搜索变量名...");
    searchEdit->setClearButtonEnabled(true);
    mainLayout->insertWidget(0, searchEdit);  // 插入到表格之后、按钮之前（索引根据实际调整）

    connect(searchEdit, &QLineEdit::textChanged, this, &EnvViewer::filterTable);
    setLayout(mainLayout);

    process = new QProcess(this);
    // 信号连接
    //connect(refreshBtn, &QPushButton::clicked, this, &EnvViewer::loadEnvironment);
    connect(addBtn, &QPushButton::clicked, this, &EnvViewer::addVariable);
    connect(deleteBtn, &QPushButton::clicked, this, &EnvViewer::deleteVariable);
        connect(editBtn, &QPushButton::clicked, this, &EnvViewer::editVariable);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &EnvViewer::readProcessOutput);

    //双击触发编辑操作
    connect(table, &QTableWidget::cellDoubleClicked,this, &EnvViewer::onTableCellDoubleClicked);
    // 启动时自动加载
    //loadCustomVarsFromFile();   // 先加载 ~/.path 中的自定义变量，不阻塞
    loadEnvironment();
}

EnvViewer::~EnvViewer()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(3000);
    }
}



void EnvViewer::loadEnvironment()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(1000);
    }
    //refreshBtn->setEnabled(false);
    addBtn->setEnabled(false);       // 加载期间禁用添加按钮
    process->start("env");
}

void EnvViewer::readProcessOutput()
{
    //refreshBtn->setEnabled(true);
    addBtn->setEnabled(true);

    if (process->exitStatus() != QProcess::NormalExit || process->exitCode() != 0) {
        QMessageBox::warning(this, "错误", "执行 env 命令失败");
        return;
    }

    QByteArray output = process->readAllStandardOutput();
    QString text = QString::fromUtf8(output);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    systemVars.clear();
    for (const QString &line : lines) {
        int idx = line.indexOf('=');
        if (idx == -1) continue;
        QString key = line.left(idx);
        QString value = line.mid(idx + 1);
        systemVars.insert(key, value);
    }

    // 统一刷新表格
    loadCustomVarsFromFile();
    updateTableDisplay();
    sortPathFile();   // 启动/刷新时自动排序整理文件
}

void EnvViewer::addVariable()
{
    AddVarDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.varName();
        QString value = dlg.varValue();

        if (name.isEmpty()) {
            QMessageBox::warning(this, "错误", "变量名不能为空");
            return;
        }

        // 检查名称是否与已有变量重复（系统变量 + 自定义变量）
        if (systemVars.contains(name) || customVars.contains(name)) {
            QMessageBox::warning(this, "错误",
                                 QString("变量 \"%1\" 已存在，不能重复添加").arg(name));
            return;
        }

        // 保存到自定义变量集
        customVars.insert(name, value);

        // 追加写入 ~/.path 文件
        QFile file(pathFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << "export " << name << "=" << value << "\n";
            file.close();
            QMessageBox::information(this, "成功", "变量添加成功");   // 新增
        } else {
            QMessageBox::warning(this, "警告", "无法写入 ~/.path 文件，变量仅显示在列表中");
        }

        // 立即刷新显示
        updateTableDisplay();
        sortPathFile();          // 新增：排序文件
    }
}

void EnvViewer::editVariable()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要编辑的行");
        return;
    }

    QString name = table->item(row, 0)->text();
    QString oldValue = table->item(row, 1)->text();   // 表格当前显示的值
    QString source = table->item(row, 2)->text();

    QStringList allPaths = oldValue.split(':', Qt::SkipEmptyParts);
    bool isMultiValue = (allPaths.size() > 2 );

    if (isMultiValue) {
        // 获取系统原始路径
        QStringList sysPaths;
        if (systemVars.contains(name))
            sysPaths = systemVars.value(name).split(':', Qt::SkipEmptyParts);
        else
            sysPaths = allPaths; // 退路：当作全部系统路径

        // 多值变量：弹出带有添加功能的对话框
        bool hasCustom = customVars.contains(name);   // 有自定义记录
        PathViewDialog dlg(name,sysPaths,allPaths, hasCustom, this);
        connect(&dlg, &PathViewDialog::resetRequested,
                this, &EnvViewer::resetSystemVariable);
        connect(&dlg, &PathViewDialog::pathAdded,
                this, &EnvViewer::onPathAdded);
        connect(&dlg, &PathViewDialog::customPathsChanged,
                this, &EnvViewer::onCustomPathsChanged);  // 新增
        dlg.exec();
        // 对话框关闭时表格可能已通过 pathAdded 更新，但若重置则需刷新
        updateTableDisplay();
        return;
    }

    // 单值变量：使用原有编辑对话框
    AddVarDialog dlg(this);
    dlg.setWindowTitle("编辑环境变量");
    dlg.setVarName(name);
    dlg.setValue(oldValue);
    dlg.setDefinitionSource(source);
    dlg.setResetEnabled(hasExportInFile(name));

    connect(&dlg, &AddVarDialog::resetRequested,
            this, &EnvViewer::resetSystemVariable);

    if (dlg.exec() == QDialog::Accepted) {
        QString newValue = dlg.varValue();
        if (newValue == oldValue)
            return;

        bool isSystem = (source == "system");
        bool alreadyCustomized = customVars.contains(name);

        if (isSystem && !alreadyCustomized) {
            customVars.insert(name, newValue);
            QFile file(pathFilePath);
            if (file.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&file);
                out << "export " << name << "=" << newValue << "\n";
                file.close();
            }
        }
        else {
            customVars[name] = newValue;
            QFile file(pathFilePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QStringList lines;
                QTextStream in(&file);
                bool found = false;
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    QString trimmed = line.trimmed();
                    if (trimmed.startsWith("export ")) {
                        QString rest = trimmed.mid(7).trimmed();
                        int eqIdx = rest.indexOf('=');
                        if (eqIdx != -1) {
                            QString key = rest.left(eqIdx).trimmed();
                            if (key == name) {
                                lines.append("export " + name + "=" + newValue);
                                found = true;
                                continue;
                            }
                        }
                    }
                    lines.append(line);
                }
                file.close();

                if (!found) {
                    // 如果找不到旧行（可能被手动删除），追加新行
                    lines.append("export " + name + "=" + newValue);
                }

                // 写回文件
                if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    QTextStream out(&file);
                    for (const QString &l : lines)
                        out << l << "\n";
                    file.close();
                }
            } else {
                // 文件不存在，直接创建
                QFile newFile(pathFilePath);
                if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&newFile);
                    out << "export " + name + "=" + newValue + "\n";
                    newFile.close();
                }
            }
            QMessageBox::information(this, "成功", "变量修改成功");   // 新增
        }
        updateTableDisplay();
        sortPathFile();          // 新增：排序文件
    }
}
void EnvViewer::deleteVariable()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中要删除的行");
        return;
    }

    // 检查是否自定义变量
    QTableWidgetItem *sourceItem = table->item(row, 2);
    if (!sourceItem || sourceItem->text() != "custom") {
        QMessageBox::warning(this, "错误", "只能删除自定义（custom）变量，系统变量不可删除");
        return;
    }

    QString name = table->item(row, 0)->text();

    // 从自定义变量集移除
    customVars.remove(name);

    // 同步更新 ~/.path 文件：移除对应的 export 行
    QString filePath = pathFilePath;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            // 解析是否是我们要删除的 export 行
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("export ")) {
                QString rest = trimmedLine.mid(7).trimmed();
                int eqIdx = rest.indexOf('=');
                if (eqIdx != -1) {
                    QString key = rest.left(eqIdx).trimmed();
                    if (key == name) {
                        continue;   // 跳过这一行，即删除
                    }
                }
            }
            lines.append(line);   // 保留其他行
        }
        file.close();

        // 重写文件
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            for (const QString &l : lines)
                out << l << "\n";
            file.close();
        } else {
            QMessageBox::warning(this, "警告", "无法更新 ~/.path 文件，但已从列表移除");
        }
    } else {
        // 文件不存在或无法读取，直接从列表移除即可
        // （可能文件被手动删除，此处忽略即可）
    }

    // 刷新表格显示
    updateTableDisplay();
    sortPathFile();          // 新增：排序文件
    QMessageBox::information(this, "成功", "变量删除成功");   // 新增
}

void EnvViewer::updateTableDisplay()
{
    table->setRowCount(0);

    // 1. 填充系统变量
    QMapIterator<QString, QString> sys(systemVars);
    while (sys.hasNext()) {
        sys.next();
        int row = table->rowCount();
        table->insertRow(row);

        // 变量名列（第0列）
        QTableWidgetItem *nameItem = new QTableWidgetItem(sys.key());
        nameItem->setToolTip(sys.key());          // 添加 tooltip
        table->setItem(row, 0, nameItem);

        // 值列（第1列）
        QTableWidgetItem *valueItem = new QTableWidgetItem(sys.value());
        valueItem->setToolTip(sys.value());       // 添加 tooltip
        table->setItem(row, 1, valueItem);

        // 定义者列（第2列）不需要 tooltip
        table->setItem(row, 2, new QTableWidgetItem("system"));
    }

    // 2. 填充自定义变量（覆盖或新增）
    QMapIterator<QString, QString> cust(customVars);
    while (cust.hasNext()) {
        cust.next();
        bool found = false;
        for (int r = 0; r < table->rowCount(); ++r) {
            if (table->item(r, 0)->text() == cust.key()) {
                // 覆盖已存在的行
                QTableWidgetItem *valueItem = table->item(r, 1);
                valueItem->setText(cust.value());
                valueItem->setToolTip(cust.value());   // 更新 tooltip
                table->item(r, 2)->setText("custom");
                found = true;
                break;
            }
        }
        if (!found) {
            int row = table->rowCount();
            table->insertRow(row);

            QTableWidgetItem *nameItem = new QTableWidgetItem(cust.key());
            nameItem->setToolTip(cust.key());
            table->setItem(row, 0, nameItem);

            QTableWidgetItem *valueItem = new QTableWidgetItem(cust.value());
            valueItem->setToolTip(cust.value());
            table->setItem(row, 1, valueItem);

            //设置颜色
            table->setItem(row, 2, new QTableWidgetItem("custom"));
        }
    }
    for (int i = 0; i < table->rowCount(); ++i) {
        QTableWidgetItem *sourceItem = table->item(i, 2);
        if (!sourceItem) continue;
        if (sourceItem->text() == "custom")
            sourceItem->setBackground(QColor(144, 238, 144));

    }

    filterTable(searchEdit->text());
    filterTable(searchEdit->text());

    //自动调整宽度
    //table->resizeColumnsToContents();
}

void EnvViewer::loadCustomVarsFromFile()
{
    customVars.clear();

    QFile file(pathFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // 用系统变量作为初始值，用于展开 $VAR
    QMap<QString, QString> currentValues = systemVars;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.startsWith("export "))
            continue;

        QString rest = line.mid(7).trimmed();       // 去掉 "export "
        int eqIdx = rest.indexOf('=');
        if (eqIdx == -1) continue;
        QString key = rest.left(eqIdx).trimmed();
        QString value = rest.mid(eqIdx + 1).trimmed();

        // 去除外层引号（双引号或单引号）
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\'')))
            value = value.mid(1, value.length() - 2);

        // 展开 $VAR 引用（仅当 currentValues 中存在该变量时才展开）
        QString placeholder = "$" + key;
        if (value.contains(placeholder)) {
            if (currentValues.contains(key)) {
                QString base = currentValues.value(key);
                value.replace(placeholder, base);
            } // 否则保持原样，不展开
        }

        // 更新 currentValues 和 customVars
        currentValues[key] = value;
        customVars[key] = value;
    }
    file.close();
}
void EnvViewer::resetSystemVariable(const QString &name)
{
    // 从内存中移除自定义记录
    customVars.remove(name);

    // 从 ~/.path 文件中删除对应的 export 行
    QFile file(pathFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("export ")) {
                QString rest = trimmed.mid(7).trimmed();
                int eqIdx = rest.indexOf('=');
                if (eqIdx != -1) {
                    QString key = rest.left(eqIdx).trimmed();
                    if (key == name)
                        continue;   // 跳过该行，即删除
                }
            }
            lines.append(line);
        }
        file.close();

        // 写回文件
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            for (const QString &l : lines)
                out << l << "\n";
            file.close();
        }
    }

    // 刷新表格显示（系统变量恢复为 env 命令提供的原始值，若无则消失）
    updateTableDisplay();
    sortPathFile();          // 新增：排序文件
}

void EnvViewer::onPathAdded(const QString &varName, const QString &path)
{
    // 写入 ~/.path 文件
    QFile file(pathFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "export " << varName << "=\"$" << varName << ":" << path << "\"\n";
        file.close();
    } else {
        QMessageBox::warning(this, "错误", "无法写入 ~/.path 文件");
        return;
    }

    // 更新内存中的 customVars（覆盖该变量的值）
    // 重新从文件加载该变量的最终值并更新表格
    loadCustomVarsFromFile();   // 简单重建所有自定义变量
    updateTableDisplay();
    sortPathFile();          // 新增：排序文件
}

void EnvViewer::onCustomPathsChanged(const QString &varName, const QStringList &customPaths)
{
    // 移除文件中所有该变量的旧 export 行，重新写入新自定义路径
    QFile file(pathFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("export ")) {
            QString rest = trimmed.mid(7).trimmed();
            int eqIdx = rest.indexOf('=');
            if (eqIdx != -1) {
                QString key = rest.left(eqIdx).trimmed();
                if (key == varName)
                    continue;   // 跳过该变量的所有旧行
            }
        }
        lines.append(line);
    }
    file.close();

    // 追加新的自定义路径行
    for (const QString &path : customPaths) {
        lines.append("export " + varName + "=\"$" + varName + ":" + path + "\"");
    }

    // 写回文件
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        for (const QString &l : lines)
            out << l << "\n";
        file.close();
    }

    // 重新加载并刷新表格
    loadCustomVarsFromFile();
    updateTableDisplay();
    sortPathFile();          // 新增：排序文件
}

bool EnvViewer::hasExportInFile(const QString &varName)
{
    QFile file(pathFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("export ")) {
            QString rest = line.mid(7).trimmed();
            int eqIdx = rest.indexOf('=');
            if (eqIdx != -1 && rest.left(eqIdx).trimmed() == varName) {
                file.close();
                return true;
            }
        }
    }
    file.close();
    return false;
}

void EnvViewer::loadSettings()
{
    QSettings settings("EnvViewer", "EnvViewer");
    // 默认路径为 ~/.path
    QString defaultPath = QDir::homePath() + "/.path";
    pathFilePath = settings.value("userVarFile").toString();
    // 确保目录存在（可选）
    QFileInfo fi(pathFilePath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        QDir().mkpath(dir.absolutePath());
    }
}

void EnvViewer::saveSettings()
{
    QSettings settings("EnvViewer", "EnvViewer");
    settings.setValue("userVarFile", QDir::homePath() + "/.path");
}

void EnvViewer::updateBashrcSource(const QString &filePath)
{
    QString bashrcPath = QDir::homePath() + "/.bashrc";
    QString marker = "# EnvViewer custom env vars";

    QFile file(bashrcPath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

    // 读取所有行
    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }

    // 移除所有包含标记的行
    QStringList newLines;
    for (const QString &line : lines) {
        if (!line.contains(marker))
            newLines.append(line);
    }

    // 追加新行
    newLines.append(QString("source %1  %2").arg(filePath, marker));

    // 写回文件
    file.resize(0);
    QTextStream out(&file);
    for (const QString &line : newLines)
        out << line << "\n";
    file.close();
}
//变量搜索功能
void EnvViewer::filterTable(const QString &text)
{
    for (int i = 0; i < table->rowCount(); ++i) {
        QTableWidgetItem *item = table->item(i, 0);
        if (item) {
            bool match = item->text().contains(text, Qt::CaseInsensitive);
            table->setRowHidden(i, !match);
        }
    }
}
//双击触发编辑操作
void EnvViewer::onTableCellDoubleClicked(int row, int /*column*/)
{
    table->selectRow(row);   // 确保当前行为双击的行
    editVariable();          // 调用已有编辑功能
}

void EnvViewer::sortPathFile()
{
    QFile file(pathFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QStringList exportLines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("export "))
            exportLines.append(line);
    }
    file.close();

    if (exportLines.isEmpty())
        return;

    // 判断是否为多值变量：值中引用自身（$VAR 或 ${VAR}）
    auto isMultiValue = [](const QString &varName, const QString &value) -> bool {
        return value.contains("$" + varName) || value.contains("${" + varName + "}");
    };

    QStringList singleValueLines, multiValueLines;
    for (const QString &line : exportLines) {
        QString rest = line.mid(7).trimmed();   // 去掉 "export "
        int eqIdx = rest.indexOf('=');
        if (eqIdx == -1) continue;
        QString key = rest.left(eqIdx).trimmed();
        QString value = rest.mid(eqIdx + 1);
        if (isMultiValue(key, value))
            multiValueLines.append(line);
        else
            singleValueLines.append(line);
    }

    // 按变量名排序（不区分大小写）
    auto sortByVarName = [](QStringList &list) {
        std::sort(list.begin(), list.end(), [](const QString &a, const QString &b) {
            auto varName = [](const QString &s) -> QString {
                QString r = s.mid(7).trimmed();
                int eq = r.indexOf('=');
                return (eq == -1) ? r : r.left(eq).trimmed();
            };
            return QString::compare(varName(a), varName(b), Qt::CaseInsensitive) < 0;
        });
    };
    sortByVarName(singleValueLines);
    sortByVarName(multiValueLines);

    // 合并：单值在上，多值在下
    QStringList sortedLines = singleValueLines + multiValueLines;

    // 写回文件
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        for (const QString &l : sortedLines)
            out << l << "\n";
        file.close();
    }
}

void EnvViewer::onTableContextMenu(const QPoint &pos)
{
    // 获取鼠标点击位置对应的行
    QTableWidgetItem *item = table->itemAt(pos);
    int row = item ? item->row() : -1;

    QMenu menu(this);
    QAction *addAction = menu.addAction("添加变量");
    QAction *editAction = menu.addAction("编辑变量");
    QAction *deleteAction = menu.addAction("删除变量");
    menu.addSeparator();
    QAction *copyAction = menu.addAction("复制变量名");

    // 如果没有选中行，禁用编辑、删除、复制
    bool hasSelection = (row >= 0);
    editAction->setEnabled(hasSelection);
    deleteAction->setEnabled(hasSelection);
    copyAction->setEnabled(hasSelection);

    // 连接动作（使用 exec 返回选择的动作）
    QAction *selectedAction = menu.exec(table->viewport()->mapToGlobal(pos));
    if (selectedAction == addAction) {
        addVariable();
    } else if (selectedAction == editAction) {
        table->selectRow(row);
        editVariable();
    } else if (selectedAction == deleteAction) {
        table->selectRow(row);
        deleteVariable();
    } else if (selectedAction == copyAction) {
        table->selectRow(row);
        copyVariableName();
    }
}
void EnvViewer::copyVariableName()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先选中要复制的行。");
        return;
    }

    QString name = table->item(row, 0)->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "变量名为空，无法复制。");
        return;
    }

    QApplication::clipboard()->setText(name);
    QMessageBox::information(this, "成功", "变量名已复制到剪贴板。");
}
