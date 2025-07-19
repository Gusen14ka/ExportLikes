#include "exportlikes.hpp"
#include <QApplication>

int main(int argc, char* argv[]) {
    QCoreApplication::setApplicationName("ExportLikes");
    QApplication a(argc, argv);
    ExportLikes w;
    w.show();
    return a.exec();
}




