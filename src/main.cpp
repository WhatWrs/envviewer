#include <QApplication>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include "envviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 单实例锁：防止同时运行多个实例
    QString lockPath = QDir::homePath() + "/.config/EnvViewer/EnvViewer.lock";
    QLockFile lockFile(lockPath);
    if (!lockFile.tryLock(100)) {
        QMessageBox::warning(nullptr, "提示",
                             "EnvViewer 已在运行中，不能同时打开多个实例。");
        return 1;
    }

    EnvViewer viewer;
    viewer.show();

    // 首次运行：显示欢迎界面（模态，与主界面同时可见）
    QSettings settings("EnvViewer", "EnvViewer");
    bool firstRun = settings.value("firstRun", true).toBool();
    if (firstRun) {
        QMessageBox welcomeBox(&viewer);
        welcomeBox.setWindowTitle("欢迎");
        welcomeBox.setIcon(QMessageBox::Information);
        welcomeBox.setText("<h3>Linux 系统环境变量查看器</h3>"
                         "<p>版本：1.0</p>"
                         "<p>一个基于 Qt 的 Linux 环境变量管理工具，"
                         "支持查看、添加、编辑和删除系统环境变量。</p>"
                         "<hr>"
                         "<p>项目地址：<br>"
                         "<a href='https://gitee.com/WRSwhat/envviewer.git'>"
                         "https://gitee.com/WRSwhat/envviewer.git</a></p>");
        welcomeBox.setTextFormat(Qt::RichText);
        welcomeBox.exec();
        settings.setValue("firstRun", false);
    }

    int ret = app.exec();

    // 程序退出时自动释放锁文件
    lockFile.unlock();

    return ret;
}
