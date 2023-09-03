#ifndef _HWSHQTB_PROVINCE_HPP
#define _HWSHQTB_PROVINCE_HPP

#include <QDebug>
#include <QWidget>

#include <QPolygon>
#include <QLine>
#include <QPainter>
#include <QImage>

#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

#include <filesystem>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>

namespace hwshqtb {
    class province: public QWidget {
        Q_OBJECT;

        struct line_compare {
            bool operator()(const QLine& a, const QLine& b)const {
                auto f = [](const QPoint& p1, const QPoint& p2) {
                    return p1.x() != p2.x() ? p1.x() < p2.x() : p1.y() < p2.y();
                };
                return a.p1() != b.p1() ? f(a.p1(), b.p1()) : f(a.p2(), b.p2());
            }
        };

    public:
        province(QWidget* parent = nullptr):
            QWidget::QWidget(parent), _offset(), _image(".\\bg.png") {
            setObjectName(QString::fromUtf8("province"));
            resize(800, 600);
            setWindowTitle(QString::fromUtf8("province"));
            setAutoFillBackground(true);
            setBackgroundRole(QPalette::Base);

            if (!std::filesystem::exists(std::filesystem::path(".\\link.txt"))) _load_from_map();
            else _load_from_link();

            QPainter painter(&_image);
            painter.setPen({QColor(0, 0, 0)});
            for (const QPolygon& polygon : _polygons)
                painter.drawPolygon(polygon);
        }
        ~province() = default;

    public slots:
        void paintEvent(QPaintEvent*) {
            _draw();
        }
        void mouseReleaseEvent(QMouseEvent* event) {
            if (event->button() == Qt::LeftButton) {
                _now_polygon = _get_polygon(event->pos() / _zoom - _offset);
            }
            else if (event->button() == Qt::RightButton) {
                if (_now_polygon == -1) return;
                int to = _get_polygon(event->pos() / _zoom - _offset);
                if (to == _now_polygon) return;
                int i = _get_link(_now_polygon, to);
                if (i == -1) _links[_now_polygon].push_back(to);
                else _links[_now_polygon].erase(_links[_now_polygon].begin() + i);
            }
            update();
        }
        void keyReleaseEvent(QKeyEvent* event) {
            switch (event->key()) {
                case Qt::Key_Return: if (!event->isAutoRepeat()) _save(); return;
                case Qt::Key_Up: _offset.ry() += 10; break;
                case Qt::Key_Down: _offset.ry() -= 10; break;
                case Qt::Key_Left: _offset.rx() += 10; break;
                case Qt::Key_Right: _offset.rx() -= 10; break;
            }

            update();
        }
        void wheelEvent(QWheelEvent* event) {
            _zoom += event->angleDelta().y() > 0 ? 0.1 : (_zoom < 0.11 ? 0 : -0.1);
            update();
        }

    private:
        void _draw() {
            QPainter painter(this);

            painter.drawPixmap(QRect{{0, 0}, size()}, _image, QRect{-_offset, size() / _zoom});

            if (_now_polygon != -1) {
                for (auto to : _links[_now_polygon]) {
                    painter.setPen(_get_link(to, _now_polygon) == -1 ? Qt::DashLine : Qt::SolidLine);
                    painter.drawLine((_polygon_centers[_now_polygon] + _offset) * _zoom, (_polygon_centers[to] + _offset) * _zoom);
                }
            }
        }
        std::vector<QLine> _get_lines(const QPolygon& polygon) {
            std::vector<QLine> ret(polygon.size());
            for (int i = 0; i < polygon.size() - 1; ++i)
                ret[i] = {polygon[i], polygon[i + 1]};
            ret.back() = {polygon.back(), polygon.front()};
            return ret;
        }
        int _get_polygon(const QPoint& point) {
            for (int i = 0; i < _polygons.size(); ++i)
                if (_polygons[i].containsPoint(point, Qt::WindingFill)) return i;
            return -1;
        }
        int _get_link(int from, int to) {
            for (int i = 0; i < _links[from].size(); ++i)
                if (_links[from][i] == to) return i;
            return -1;
        }
        void _load_from_map() {
            std::ifstream file(".\\map.txt");

            std::multimap<QLine, int, line_compare> lines;
            while (file.good()) {
                QPolygon polygon;
                QPoint center;
                while (file.get() != '\n' && file.good()) {
                    file.unget();
                    int x, y;
                    file >> x >> y;
                    polygon.push_back({x, y});
                    center += {x, y};
                    while (file.get() == ' ');
                    file.unget();
                }
                if (polygon.size()) {
                    _polygons.push_back(polygon);
                    _polygon_centers.push_back(center / polygon.size());
                    _links.push_back({});
                    for (const QLine& line : _get_lines(polygon)) {
                        auto range = lines.equal_range(line);
                        for (auto i = range.first; i != range.second; ++i) {
                            _links.back().push_back(i->second);
                            _links[i->second].push_back(_links.size() - 1);
                        }
                        lines.insert({line, _polygons.size() - 1});
                        lines.insert({QLine{line.p2(), line.p1()}, _polygons.size() - 1});
                    }
                }
            }

            for (auto& polygon : _links) {
                std::sort(polygon.begin(), polygon.end());
                auto last = std::unique(polygon.begin(), polygon.end());
                polygon.erase(last, polygon.end());
            }
        }
        void _load_from_link() {
            std::ifstream file(".\\map.txt");

            while (file.good()) {
                QPolygon polygon;
                QPoint center;
                while (file.get() != '\n' && file.good()) {
                    file.unget();
                    int x, y;
                    file >> x >> y;
                    polygon.push_back({x, y});
                    center += {x, y};
                    while (file.get() == ' ');
                    file.unget();
                }
                if (polygon.size()) {
                    _polygons.emplace_back(std::move(polygon));
                    _polygon_centers.emplace_back(std::move(center / polygon.size()));
                }
            }

            std::ifstream file2(".\\link.txt");
            while (file2.good() && _links.size() < _polygons.size()) {
                std::vector<int> link;
                while (file2.get() != '\n' && file2.good()) {
                    file2.unget();
                    int to;
                    file2 >> to;
                    link.emplace_back(to);
                    while (file2.get() == ' ');
                    file2.unget();
                }
                _links.push_back(link);
            }
        }
        void _save() {
            std::ofstream file(".\\link.txt");
            for (auto& link : _links) {
                std::sort(link.begin(), link.end());
                for (auto to : link)
                    file << to << ' ';
                file << '\n';
            }
        }

    private:
        std::vector<QPolygon> _polygons;
        std::vector<QPoint> _polygon_centers;
        std::vector<std::vector<int> > _links;
        int _now_polygon = -1;

        QPixmap _image;

        QPoint _offset;
        double _zoom = 1;
    };
}

#endif