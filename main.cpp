#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    if (argc == 2) {
        QString path = QString::fromLocal8Bit(argv[1]);
        QFileInfo info(path);
        if (info.exists()) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            w.openPath(path);
            QApplication::restoreOverrideCursor();
        }
    }
    return a.exec();
}
