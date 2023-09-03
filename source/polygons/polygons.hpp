#ifndef _HWSHQTB_POLYGONS_HPP
#define _HWSHQTB_POLYGONS_HPP

#include <QDebug>
#include <QWidget>
#include <QRandomGenerator>

#include <QPoint>
#include <QLine>
#include <QPolygon>
#include <QPainter>
#include <QImage>

#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

#include <vector>
#include <map>
#include <optional>
#include <fstream>

namespace hwshqtb {
    class polygons: public QWidget {
        Q_OBJECT;

        struct spolygon_compare {
            bool operator()(const QPoint& l, const QPoint& r)const {
                return l.x() == r.x() ? l.y() < r.y() : l.x() < r.x();
            }
        };
        using spoint = std::map<QPoint, int, spolygon_compare>::iterator;
        struct spolygon {
            std::vector<spoint> _points;

            operator QPolygon()const {
                QPolygon ret;
                for (const spoint& iter : _points)
                    ret.push_back(iter->first);
                return ret;
            }
            bool push_back(const spoint& new_point) {
                if (_points.size() > 0 && _points.front() == new_point) return false;
                _points.push_back(new_point);
                return true;
            }
            std::optional<spoint> pop_back() {
                if (_points.size() == 0) return std::nullopt;
                spoint point = _points.back();
                _points.pop_back();
                return point;
            }
        };

    public:
        polygons(QWidget* parent = nullptr):
            QWidget::QWidget(parent), _offset(), _image(".\\map.png") {
            setObjectName(QString::fromUtf8("polygons"));
            resize(800, 600); 
            setWindowTitle(QString::fromUtf8("polygons"));
            setAutoFillBackground(true);
            setBackgroundRole(QPalette::Base);

            _load();
        }
        ~polygons() = default;

    public slots:
        void paintEvent(QPaintEvent*) {
            _draw();
        }
        void mouseReleaseEvent(QMouseEvent* event) {
            if (event->button() == Qt::LeftButton) {
                spoint new_point = _get_spoint(event->pos() / _zoom - _offset);
                if (!_new_polygon.push_back(new_point)) {
                    --new_point->second;
                    _polygons.push_back(_new_polygon);
                    _new_polygon._points.clear();
                }
            }
            else if (event->button() == Qt::RightButton) {
                std::optional<spoint> pp = _new_polygon.pop_back();
                if (pp == std::nullopt) {
                    if (_polygons.size()) {
                        for (auto& point : _polygons.back())
                            if (!--_points[point]) _points.erase(point);
                        _polygons.pop_back();
                    }
                }
                else {
                    spoint point = *pp;
                    if (!--point->second)
                        _points.erase(point->first);
                }
            }
            update();
        }
        void keyReleaseEvent(QKeyEvent* event) {
            switch (event->key()) {
                case Qt::Key_Return: if (!event->isAutoRepeat()) _save(); return;
                case Qt::Key_Up: _offset.ry() += 1; break;
                case Qt::Key_Down: _offset.ry() -= 1; break;
                case Qt::Key_Left: _offset.rx() -= 1; break;
                case Qt::Key_Right: _offset.rx() += 1; break;
            }

            update();
        }
        void wheelEvent(QWheelEvent* event) {
            _zoom += event->angleDelta().y() > 0 ? 0.1 : (_zoom < 0.11 ? 0 : -0.1);
            update();
        }

    private:
        int _distance(const QPoint& a, const QPoint& b) {
            return (a.x() - b.x()) * (a.x() - b.x()) + (a.y() - b.y()) * (a.y() - b.y());
        }

        void _draw() {
            QPainter painter(this);

            painter.drawPixmap(QRect{{0, 0}, size()}, _image, QRect{-_offset, size() / _zoom});

            painter.setPen({QColor(0, 0, 0)});
            for (const QPolygon& _polygon : _polygons){
                QPolygon polygon = _polygon;
                for (QPoint& point : polygon) point += _offset, point *= _zoom;
                painter.setBrush({QColor(QRandomGenerator::global()->bounded(100, 245),QRandomGenerator::global()->bounded(100, 245),QRandomGenerator::global()->bounded(100, 245))});
                painter.drawPolygon(polygon);
            }

            painter.setPen({QColor(0, 0, 255), 5});
            painter.setBrush({});
            for (const auto& [point, _] : _points) {
                painter.drawPoint((point + _offset) * _zoom);
            }

            if (_new_polygon._points.size()) {
                painter.setPen({QColor(0, 255, 0), 5});
                painter.drawPoint((_new_polygon._points.front()->first + _offset) * _zoom);

                painter.setPen({QColor(255, 0, 0)});
                for (int i = 1; i < _new_polygon._points.size(); ++i) {
                    painter.drawLine((_new_polygon._points[i - 1]->first + _offset) * _zoom, (_new_polygon._points[i]->first + _offset) * _zoom);
                }
            }
        }
        spoint _get_spoint(const QPoint& point) {
            for (spoint iter = _points.begin(); iter != _points.end(); ++iter) {
                if (_distance(point, iter->first) <= 25) {
                    ++iter->second;
                    return iter;
                }
            }
            return _points.insert({point, 1}).first;
        }
        void _load() {
            std::ifstream file(".\\map.txt");

            while (file.good()) {
                QPolygon polygon;
                while (file.get() != '\n' && file.good()) {
                    file.unget();
                    int x, y;
                    file >> x >> y;
                    _get_spoint({x, y});
                    polygon.push_back({x, y});
                    while (file.get() == ' ');
                    file.unget();
                }
                if (polygon.size()) _polygons.push_back(polygon);
            }
        }
        void _save() {
            std::ofstream file(".\\map.txt");
            for (const auto& polygon : _polygons) {
                for (const auto& point : polygon)
                    file << point.x() << ' ' << point.y() << ' ';
                file << '\n';
            }
        }

    private:
        std::map<QPoint, int, spolygon_compare> _points;
        std::vector<QPolygon> _polygons;
        spolygon _new_polygon;

        QPixmap _image;

        QPoint _offset;
        double _zoom = 1;
    };
}

#endif