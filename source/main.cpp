#include "draw_ui_view_t.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    hwshqtb::draw_ui_view_t window;
    window.show();

    try {
        return a.exec();
    }
    catch (const std::exception& c) {
        qDebug() << c.what();
    }
}
