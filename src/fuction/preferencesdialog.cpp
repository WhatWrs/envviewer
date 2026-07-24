#include "preferencesdialog.h"
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLabel>

PreferencesDialog::PreferencesDialog(const QString &currentPath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("偏好设置");
    setMinimumSize(400, 150);

    QTabWidget *tabWidget = new QTabWidget(this);
    QWidget *userVarTab = new QWidget();
    QFormLayout *form = new QFormLayout(userVarTab);

    pathEdit = new QLineEdit(currentPath, userVarTab);

    QPushButton *browseBtn = new QPushButton("更改", userVarTab);
    QPushButton *reSetBtn = new QPushButton("重置", userVarTab);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            "选择用户变量文件",
            pathEdit->text(),
            "所有文件 (*)",
            nullptr,
            QFileDialog::DontConfirmOverwrite  // 不弹出覆盖确认
            );
        if (!filePath.isEmpty())
            pathEdit->setText(filePath);
    });
    connect(reSetBtn, &QPushButton::clicked, this, [this]() {
        pathEdit->setText(QDir::homePath() + "/.path");
    });

    QHBoxLayout *pathLayout = new QHBoxLayout();
    // pathEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    pathLayout->addWidget(pathEdit);      // 拉伸
    pathLayout->addWidget(browseBtn);
    pathLayout->addWidget(reSetBtn);
    form->addRow("文件路径:", pathLayout);

    tabWidget->addTab(userVarTab, "用户变量");
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setMinimumWidth(500);
}

QString PreferencesDialog::filePath() const
{
    return pathEdit->text().trimmed();
}