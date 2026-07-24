#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QLineEdit>

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(const QString &currentPath, QWidget *parent = nullptr);
    QString filePath() const;

private:
    QLineEdit *pathEdit;
};
#endif // PREFERENCESDIALOG_H
