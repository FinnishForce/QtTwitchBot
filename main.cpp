#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.setWindowTitle("Twitch Chat & Bot v1.0 by Force");

    QString path = QFileInfo(".").absoluteFilePath();
    path = path + "/myicon.png";

    w.setWindowIcon(QIcon(path));

    return a.exec();
}
