#include "pathviewdialog.h"
#include <QMessageBox>

PathViewDialog::PathViewDialog(const QString &varName,
                               const QStringList &systemPaths,
                               const QStringList &allPaths,
                               bool hasCustomized,
                               QWidget *parent)
    : QDialog(parent), m_varName(varName), m_systemPaths(systemPaths), m_hasCustomized(hasCustomized)
{
    setWindowTitle("编辑多值变量 - " + varName);
    setMinimumWidth(500);
    setMinimumHeight(350);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 标题
    QLabel *title = new QLabel("变量名: " + varName + "\n定义者: " +
                                   (hasCustomized ? "custom" : "system"), this);
    QFont boldFont = title->font();
    boldFont.setBold(true);
    title->setFont(boldFont);
    mainLayout->addWidget(title);

    // 路径列表
    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->addItems(allPaths);
    // 区分系统路径和自定义路径
    QSet<QString> sysSet(systemPaths.begin(), systemPaths.end());
    for (int i = 0; i < allPaths.size(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        if (sysSet.contains(item->text())) {
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            item->setForeground(Qt::gray);
        }
        // 否则保持可选、黑色
    }
    mainLayout->addWidget(listWidget);

    // 新增路径输入区
    QHBoxLayout *addLayout = new QHBoxLayout;
    newPathEdit = new QLineEdit(this);
    QPushButton *addBtn = new QPushButton("添加路径", this);
    QPushButton *delBtn = new QPushButton("删除选中", this);   // 新增
    QPushButton *editBtn = new QPushButton("编辑选中", this);  // 新增

    addLayout->addWidget(newPathEdit);
    addLayout->addWidget(addBtn);
    addLayout->addWidget(editBtn);   // 新增
    addLayout->addWidget(delBtn);   // 新增
    mainLayout->addLayout(addLayout);

    // 按钮区（关闭 + 重置）
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *resetBtn = new QPushButton("重置", this);
    resetBtn->setEnabled(hasCustomized);
    buttonBox->addButton(resetBtn, QDialogButtonBox::ActionRole);
    mainLayout->addWidget(buttonBox);

    connect(addBtn, &QPushButton::clicked, this, &PathViewDialog::onAddClicked);
    connect(delBtn, &QPushButton::clicked, this, &PathViewDialog::onDeleteClicked);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认重置",
            "确定要重置该变量的所有自定义路径吗？\n此操作将删除所有自定义添加的路径，恢复系统原始值。",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            emit resetRequested(m_varName);
            accept();
        }
    });
    connect(editBtn, &QPushButton::clicked, this, &PathViewDialog::onEditClicked);  // 新增
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}



void PathViewDialog::onAddClicked()
{
    QString path = newPathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "无效路径", "请输入有效的路径。");
        return;
    }

    // 添加到列表（正常黑色）
    listWidget->addItem(path);
    newPathEdit->clear();
    newPathEdit->setFocus();

    // 通知主窗口写入文件
    emit pathAdded(m_varName, path);
    QMessageBox::information(this, "成功", "路径添加成功");   // 可选
}

void PathViewDialog::onEditClicked()
{
    QList<QListWidgetItem*> selected = listWidget->selectedItems();
    if (selected.isEmpty()) return;
    QListWidgetItem *item = selected.first();
    if (!(item->flags() & Qt::ItemIsSelectable)) return;  // 仅允许编辑自定义项

    QString oldPath = item->text();
    bool ok;
    QString newPath = QInputDialog::getText(this, "编辑路径",
                                            "修改路径：", QLineEdit::Normal,
                                            oldPath, &ok);
    if (!ok || newPath.trimmed().isEmpty() || newPath == oldPath)
        return;

    // 更新列表项文本
    item->setText(newPath.trimmed());

    // 收集所有自定义路径（非系统路径）
    QStringList customPaths;
    QSet<QString> sysSet(m_systemPaths.begin(), m_systemPaths.end());
    for (int i = 0; i < listWidget->count(); ++i) {
        QString text = listWidget->item(i)->text();
        if (!sysSet.contains(text))
            customPaths.append(text);
    }

    // 通知主窗口重写文件
    emit customPathsChanged(m_varName, customPaths);
}
void PathViewDialog::onDeleteClicked()
{
    QList<QListWidgetItem*> selected = listWidget->selectedItems();
    if (selected.isEmpty()) return;

    for (QListWidgetItem *item : selected) {
        if (!(item->flags() & Qt::ItemIsSelectable)) continue;
        delete listWidget->takeItem(listWidget->row(item));
    }

    // 收集剩余的非系统路径
    QStringList customPaths;
    QSet<QString> sysSet(m_systemPaths.begin(), m_systemPaths.end());
    for (int i = 0; i < listWidget->count(); ++i) {
        QString text = listWidget->item(i)->text();
        if (!sysSet.contains(text))
            customPaths.append(text);
    }

    emit customPathsChanged(m_varName, customPaths);
    QMessageBox::information(this, "成功", "选中路径已删除");   // 新增
}

