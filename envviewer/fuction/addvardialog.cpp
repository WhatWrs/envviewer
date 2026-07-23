#include "addvardialog.h"
// --------------------- AddVarDialog 实现 ---------------------
AddVarDialog::AddVarDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("添加/编辑环境变量");
    nameEdit = new QLineEdit(this);
    valueEdit = new QLineEdit(this);
    sourceLabel = new QLabel(this);

    QFormLayout *form = new QFormLayout;
    form->addRow("变量名:", nameEdit);
    nameEdit->adjustSize();
    form->addRow("值:", valueEdit);
    //valueEdit->setMinimumWidth(300);
    form->addRow("定义者:", sourceLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    resetBtn = new QPushButton("重置", this);
    resetBtn->setEnabled(false);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    buttonBox->addButton(resetBtn, QDialogButtonBox::ActionRole);
    connect(resetBtn, &QPushButton::clicked, this, &AddVarDialog::onResetClicked);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setMinimumWidth(400);
}

QString AddVarDialog::varName() const { return nameEdit->text().trimmed(); }
QString AddVarDialog::varValue() const { return valueEdit->text(); }

void AddVarDialog::setVarName(const QString &name)
{
    nameEdit->setText(name);
    nameEdit->setReadOnly(true);
}

void AddVarDialog::setDefinitionSource(const QString &src)
{
    sourceLabel->setText(src);
}

void AddVarDialog::setValue(const QString &val)
{
    valueEdit->setText(val);
}
void AddVarDialog::setResetEnabled(bool enabled)
{
    resetBtn->setEnabled(enabled);
}

void AddVarDialog::onResetClicked()
{
    // 发出重置信号，携带当前变量名
    emit resetRequested(varName());
    accept();   // 关闭对话框
}
