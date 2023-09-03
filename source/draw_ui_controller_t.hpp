#ifndef _HWSHQTB_DRAW_UI_CONTROLLER_T
#define _HWSHQTB_DRAW_UI_CONTROLLER_T

#include <QMap>
#include <QString>
#include <QStringLiteral>
#include "draw_ui_data_t.hpp"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

namespace hwshqtb {
    class draw_ui_controller_t {
    public:
        constexpr inline static double point_radius = 5.0;
        static bool point_eq(const QPointF& a, const QPointF& b) {
            constexpr double e = point_radius * point_radius;
            QPointF c = a - b;
            return QPointF::dotProduct(c, c) - e <= std::numeric_limits<qreal>::epsilon();
        }

    private:
        draw_ui_controller_t()
            :data(points) {}

    public:
        static draw_ui_controller_t& instance() {
            static draw_ui_controller_t controller;
            return controller;
        }

        bool load() {
            std::string name = QFileDialog::getOpenFileName(nullptr, u8"选择文件").toStdString();
            if (name.empty()) return false;
            data.load(std::move(name));
            return true;
        }
        void exit() {
            if (!is_changed) return;
            switch (QMessageBox::warning(nullptr, "", u8"保存吗？", QMessageBox::Ok, QMessageBox::No, QMessageBox::Cancel)) {
                case QMessageBox::Ok: data.save(); break;
                default: break;
            }
        }

        void add_point(QPointF point) {
            point = _get_point(point);
            if (!new_tile.empty() && point == new_tile.front() && new_type.size()) {
                data.add(new_type, std::move(new_tile));
                if(!new_tile.empty()) new_tile.clear();
            }
            else {
                points[point]++;
                new_tile.push_back(point);
            }
        }
        void delete_point() {
            if (new_tile.size()) {
                --points[new_tile.back()];
                new_tile.pop_back();
            }
        }
        
        int polygon_of_point(const QPointF& point) {
            for (int i = 0; i < data.tiles().size(); ++i)
                if (_winding_number(data.tile(i), point)) return i;
            return -1;
        }

    private:
        QPointF _get_point(QPointF point) {
            for (const auto& [p, _] : points)
                if (point_eq(point, p)) return p;
            return point;
        }

        qreal _check_edge(const QPointF& from, const QPointF& to, const QPointF& point) {
            return (from.y() - point.y()) * (to.x() - point.x()) - (to.y() - point.y()) * (from.x() - point.x());
        }
        int _winding_number(const std::vector<QPointF>& tile, const QPointF& point) {
            int ret = 0;
            for (int i = 0; i < tile.size() - 1; ++i) {
                if (qreal_less(tile[i].y(), point.y()) && qreal_less(point.y(), tile[i + 1].y()) && _check_edge(tile[i], tile[i + 1], point) > 0) ++ret;
                else if (qreal_less(tile[i + 1].y(), point.y()) && qreal_less(point.y(), tile[i].y()) && _check_edge(tile[i], tile[i + 1], point) < 0) --ret;
            }
            if (qreal_less(tile.back().y(), point.y()) && qreal_less(point.y(), tile.front().y()) && _check_edge(tile.back(), tile.front(), point) > 0) ++ret;
            else if (qreal_less(tile.front().y(), point.y()) && qreal_less(point.y(), tile.back().y()) && _check_edge(tile.back(), tile.front(), point) < 0) --ret;
            return ret;
        }

    public:
        draw_ui_data_t data;

        bool is_changed = false;
        QPixmap background{"map.png"};
        std::map<QPointF, int> points;
        std::vector<QPointF> new_tile;
        std::string new_type;
        std::map<std::string, draw_ui_data_t::type_t> new_metatype;
    };

}

#endif