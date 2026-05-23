#include <QApplication>
#include "envviewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    EnvViewer viewer;
    viewer.show();

    return app.exec();
}