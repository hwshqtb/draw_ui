#ifndef _HWSHQTB_DRAW_UI_VIEW_T
#define _HWSHQTB_DRAW_UI_VIEW_T

#include "draw_ui_controller_t.hpp"
#include "resource.hpp"
#include <QDebug>
#include <iostream>

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QOpenGLWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QScrollArea>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QScrollBar>
#include <QWidgetAction>
#include <QRandomGenerator>
#include <QLabel>

#include <deque>
#include <list>

namespace hwshqtb {
    class edit_metaproperties_view_t: public QWidget {
        Q_OBJECT;

    public:
        edit_metaproperties_view_t(draw_ui_data_t::metatypes_iterator_t metatype_iterator, QWidget* parent):
            QWidget::QWidget(parent),
            _metatype_iterator(metatype_iterator) {
            connect(&_add_button, &QPushButton::clicked, this, [this, parent]() {
                _add_meta_property();
                update();
                parent->repaint();
            });
            connect(&_fold_button, &QPushButton::clicked, this, [this]() {
                _visible = false;
                for (auto& i : _delete_buttons) i.hide();
                for (auto& i : _type_comboboxs) i.hide();
                for (auto& i : _key_edits) i.hide();
                _add_button.hide();
                _fold_button.hide();
                _unfold_button.show();
            });
            connect(&_unfold_button, &QPushButton::clicked, this, [this]() {
                _visible = true;
                for (auto& i : _delete_buttons) i.show();
                for (auto& i : _type_comboboxs) i.show();
                for (auto& i : _key_edits) i.show();
                _add_button.show();
                _unfold_button.hide();
                _fold_button.show();
            });

            if (_metatype_iterator == draw_ui_controller_t::instance().data.metatypes_end()) return;
            _metaproperties_name_edit.setText(QString::fromStdString(_metatype_iterator->first));
            for (const auto& [key, t] : _metatype_iterator->second)
                _add_meta_property(key, t);

        }

        void showEvent(QShowEvent* event) {
            resize(parentWidget()->width(), number() * 50 + 70);
            _delete_button.move(0, 10);
            _delete_button.resize(50, 50);
            _metaproperties_name_edit.move(50, 10);
            _fold_button.resize(50, 50);
            _unfold_button.resize(50, 50);
            _add_button.resize(50, 50);
            _add_button.move(0, _key_edits.size() * 50 + 70);

            _visible = true;
            _unfold_button.hide();
            _fold_button.show();
            for (auto& delete_buuton : _delete_buttons)
                delete_buuton.show();
            for (auto& _type_combobox : _type_comboboxs)
                _type_combobox.show();
            for (auto& key_edit : _key_edits)
                key_edit.show();
        }
        void resizeEvent(QResizeEvent* event) {
            _metaproperties_name_edit.resize(width() - 100, 50);

            _fold_button.move(width() - 50, 10);
            _unfold_button.move(width() - 50, 10);

            for (auto& i : _key_edits) i.resize(width() - 150, 50);
        }
        void hideEvent(QHideEvent* event) {
            if (_metatype_iterator == draw_ui_controller_t::instance().data.metatypes_end()) {
                if (!_metaproperties_name_edit.toPlainText().size()) return;
                draw_ui_data_t::meta_properties_t metatype;
                auto type_iter = _type_comboboxs.cbegin();
                for (const auto& key_edit : _key_edits) {
                    metatype.emplace(std::move(key_edit.toPlainText().toStdString()), std::move(static_cast<draw_ui_data_t::type_t>(type_iter->currentIndex())));
                    ++type_iter;
                }
                _metatype_iterator = draw_ui_controller_t::instance().data.add_metatype(std::move(_metaproperties_name_edit.toPlainText().toStdString()), std::move(metatype));
            }
            else if (!_metaproperties_name_edit.toPlainText().size()) {
                draw_ui_controller_t::instance().data.remove_metatype(_metatype_iterator->first);
                _metatype_iterator = draw_ui_controller_t::instance().data.metatypes_end();
            }
            else if (_metaproperties_name_edit.toPlainText().toStdString() == _metatype_iterator->first) {
                draw_ui_data_t::meta_properties_t metatype;
                auto type_iter = _type_comboboxs.cbegin();
                for (const auto& key_edit : _key_edits) {
                    metatype.emplace(std::move(key_edit.toPlainText().toStdString()), std::move(static_cast<draw_ui_data_t::type_t>(type_iter->currentIndex())));
                    ++type_iter;
                }
                _metatype_iterator->second = metatype;
            }
            else {
                draw_ui_data_t::meta_properties_t metatype;
                auto type_iter = _type_comboboxs.cbegin();
                for (const auto& key_edit : _key_edits) {
                    metatype.emplace(std::move(key_edit.toPlainText().toStdString()), std::move(static_cast<draw_ui_data_t::type_t>(type_iter->currentIndex())));
                    ++type_iter;
                }
                draw_ui_controller_t::instance().data.remove_metatype(_metatype_iterator->first);
                _metatype_iterator = draw_ui_controller_t::instance().data.add_metatype(std::move(_metaproperties_name_edit.toPlainText().toStdString()), std::move(metatype));
            }
        }

        int number()const {
            return  _visible ? _key_edits.size() + 1 : 0;
        }
        void set_self_iterator(std::list<edit_metaproperties_view_t>::iterator self_iterator) {
            connect(&_delete_button, &QPushButton::clicked, this, [this, self_iterator]() {
                _metaproperties_name_edit.clear();
                hide();
                remove(self_iterator);
            });
        }

    signals:
        void remove(std::list<edit_metaproperties_view_t>::iterator iterator);

    private:
        void _add_meta_property() {
            int size = _key_edits.size();
            _delete_buttons.emplace_back(resources_t::instance().sub_icon, "", this);
            _delete_buttons.back().move(0, 70 + size * 50);
            _delete_buttons.back().resize(50, 50);
            _delete_buttons.back().show();
            _type_comboboxs.emplace_back(this);
            _type_comboboxs.back().addItems(resources_t::instance().metatype_name);
            _type_comboboxs.back().move(50, 70 + size * 50);
            _type_comboboxs.back().resize(100, 50);
            _type_comboboxs.back().show();
            _key_edits.emplace_back(this);
            _key_edits.back().move(150, 70 + size * 50);
            _key_edits.back().show();
            _add_button.move(0, size * 50 + 120);

            connect(&_delete_buttons.back(), &QPushButton::clicked, this, [this,
                delete_button_iterator = --_delete_buttons.end(), type_combobox_iterator = --_type_comboboxs.end(), key_edit_iterator = --_key_edits.end()]() {

                std::list<QPushButton>::iterator i1 = delete_button_iterator;
                std::list<QComboBox>::iterator i2 = type_combobox_iterator;
                std::list<QTextEdit>::iterator i3 = key_edit_iterator;

                while (++i1 != _delete_buttons.end()) {
                    ++i2;
                    ++i3;
                    i1->move(0, i1->y() - 50);
                    i2->move(50, i2->y() - 50);
                    i3->move(150, i3->y() - 50);
                }
                _add_button.move(0, _add_button.y() - 50);

                _delete_buttons.erase(delete_button_iterator);
                _type_comboboxs.erase(type_combobox_iterator);
                _key_edits.erase(key_edit_iterator);
            });
        }
        void _add_meta_property(draw_ui_data_t::key_t key, draw_ui_data_t::type_t t) {
            _add_meta_property();
            _type_comboboxs.back().setCurrentIndex(static_cast<int>(t));
            _key_edits.back().setText(QString::fromStdString(key));
        }

        bool _visible = true;
        QPushButton _delete_button{resources_t::instance().sub_icon, "", this};
        QTextEdit _metaproperties_name_edit{this};
        QPushButton _fold_button{resources_t::instance().fold_icon, "", this};
        QPushButton _unfold_button{resources_t::instance().unfold_icon, "", this};
        std::list<QPushButton> _delete_buttons;
        std::list<QComboBox> _type_comboboxs;
        std::list<QTextEdit> _key_edits;
        QPushButton _add_button{resources_t::instance().add_icon, "", this};

        draw_ui_data_t::metatypes_iterator_t _metatype_iterator;
    };

    class edit_metatype_view_t: public QWidget {
        Q_OBJECT;

    public:
        edit_metatype_view_t(QWidget* parent):
            QWidget::QWidget(parent, Qt::Tool),
            _metaproperties_end(_metaproperties.end()) {
            setWindowTitle(tr(u8"edit type(s)"));
            setMinimumSize({300, 500});
            _area.setWidget(&_area_widget);
            _area.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            _area.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            connect(&_add_button, &QPushButton::clicked, this, [this]() {
                if (_metaproperties_end == _metaproperties.end()) {
                    _metaproperties.emplace_back(draw_ui_controller_t::instance().data.metatypes_end(), &_area_widget);
                    _metaproperties.back().set_self_iterator(--_metaproperties.end());
                    connect(&_metaproperties.back(), &edit_metaproperties_view_t::remove, this, [this](std::list<edit_metaproperties_view_t>::iterator iterator) {
                        _metaproperties.splice(_metaproperties.end(), _metaproperties, iterator);
                        --_metaproperties_end;
                    });
                    _metaproperties.back().show();
                }
                else {
                    _metaproperties_end->show();
                    ++_metaproperties_end;
                }
                _lay();
                update();
            });
        }

        void showEvent(QShowEvent* event) {
            resize(300, 500);
            _area.move(0, 0);
            _area_widget.move(0, 0);
            _area_widget.resize(size());
            _add_button.move(0, 450);
            _add_button.resize(50, 50);
        }
        void paintEvent(QPaintEvent* event) {
            _lay();
            update();
        }
        void resizeEvent(QResizeEvent* event) {
            _area.resize(width(), height() - 50);
            _add_button.move(0, height() - 50);
            _lay();
            update();
        }
        void closeEvent(QCloseEvent* event) {
            event->ignore();
            hide();
            closed();
            for (const auto& [name, properties] : draw_ui_controller_t::instance().data.metatypes()) {
                std::cout << name << "\n";
                for (const auto& [key, type] : properties)
                    std::cout << "\t" << static_cast<int>(type) << " => " << key << "\n";
                std::cout << "\n";
            }
        }

    signals:
        void closed();

    private:
        void _lay() {
            int h = 0;
            for (auto iterator = _metaproperties.begin(); iterator != _metaproperties_end; ++iterator) {
                iterator->move(0, h);
                iterator->resize(_area_widget.width(), iterator->number() * 50 + 70);
                h += iterator->number() * 50 + 70 + 15;
            }
            _area_widget.resize(_area.width(), h);
        }

        QScrollArea _area{this};
        QWidget _area_widget{this};
        std::list<edit_metaproperties_view_t> _metaproperties;
        std::list<edit_metaproperties_view_t>::iterator _metaproperties_end;
        QPushButton _add_button{resources_t::instance().add_icon, "", this};
    };

    class edit_view_t: public QOpenGLWidget {
        Q_OBJECT;

    public:
        edit_view_t(QWidget* parent = nullptr):
            QOpenGLWidget::QOpenGLWidget(parent) {
            setAutoFillBackground(true);
            setBackgroundRole(QPalette::Base);
        }

    public slots:
        void paintEvent(QPaintEvent* event) {
            QPainter painter(this);

            painter.setBrush(Qt::white);
            painter.drawRect(rect());

            painter.setTransform({_zoom, 0, 0, _zoom, _offset.x(), _offset.y()});

            painter.drawPixmap(QPointF{0, 0}, draw_ui_controller_t::instance().background);

            for (auto& tile : draw_ui_controller_t::instance().data.tiles()) {
                painter.setBrush({QColor(QRandomGenerator::global()->bounded(100, 245), QRandomGenerator::global()->bounded(100, 245), QRandomGenerator::global()->bounded(100, 245))});
                painter.drawPolygon(tile.data(), tile.size());
            }

            painter.setPen({QColor(0, 0, 255), draw_ui_controller_t::point_radius});
            for (const auto& [point, times] : draw_ui_controller_t::instance().points)
                if (times) painter.drawPoint(point);

            painter.setPen({QColor(255, 0, 0), draw_ui_controller_t::point_radius});
            painter.drawPoints(draw_ui_controller_t::instance().new_tile.data(), draw_ui_controller_t::instance().new_tile.size());
        }
        void mouseReleaseEvent(QMouseEvent* event) {
            if (event->button() == Qt::LeftButton) draw_ui_controller_t::instance().add_point((event->pos() - _offset) / _zoom);
            else if (event->button() == Qt::RightButton) draw_ui_controller_t::instance().delete_point();
            update();
        }
        void keyReleaseEvent(QKeyEvent* event) {
            switch (event->key()) {
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
        QPointF _offset = {0, 0};
        double _zoom = 1.0;

    };

    class edit_properties_view_t: public QWidget {
        Q_OBJECT;

    public:
        edit_properties_view_t(QWidget* parent):
            QWidget::QWidget(parent, Qt::Tool) {
            _metatype.move(0, 0);
            _area.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            _area.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            _area.setWidget(&_area_widget);
            _area.move(0, 50);
            _area_widget.move(0, 0);
        }

        void showEvent(QShowEvent* event) {}
        void resizeEvent(QResizeEvent* event) {
            _area.resize(width(), height() - 50);
            _area_widget.resize(width(), height() - 50);
            for (auto& value_edit : _value_edits)
                value_edit.resize(_area_widget.width() - 200, 50);
        }
        void hideEvent(QHideEvent* event) {}
        void closeEvent(QCloseEvent* event) {
            event->ignore();
            hide();
            _save_metatype();
        }

        void load_tile(int i) {
            _index = i;
            _metatype.clear();
            for (const auto& [name, meta_properties] : draw_ui_controller_t::instance().data.metatypes()) {
                _metatype.addItem(QString::fromStdString(name));
                if (name == draw_ui_controller_t::instance().data.type(i)) {
                    _metatype.setCurrentIndex(_metatype.count() - 1);
                    _load_metatype(meta_properties);
                }
            }
        }

    private:
        void _load_metatype(const draw_ui_data_t::meta_properties_t& meta_properties) {
            // = draw_ui_controller_t::instance().data.metatypes().at(_metatype.currentText().toStdString());
            int size = 0;
            for (const auto& [key, type] : meta_properties) {
                if (size < _types.size()) {
                    _types[size].setText(resources_t::instance().metatype_name[static_cast<int>(type)]);
                    _keys[size].setText(QString::fromStdString(key));
                    _value_edits[size].setPlainText(draw_ui_controller_t::instance().get_property(_index, key));
                }
                else {
                    _types.emplace_back(resources_t::instance().metatype_name[static_cast<int>(type)], &_area_widget);
                    _types.back().move(0, size * 50);
                    _types.back().resize(100, 50);
                    _keys.emplace_back(QString::fromStdString(key), &_area_widget);
                    _keys.back().move(100, size * 50);
                    _keys.back().resize(100, 50);
                    _value_edits.emplace_back(draw_ui_controller_t::instance().get_property(_index, key), &_area_widget);
                    _value_edits.back().move(200, size * 50);
                }
                ++size;
            }
        }
        void _save_metatype() {
            draw_ui_controller_t::instance().change_type(_index, _metatype.currentText());
            for (int i = 0; i < _types.size(); ++i)
                draw_ui_controller_t::instance().change_property(_index, _keys[i].text(), _value_edits[i].toPlainText());
        }

        QScrollArea _area{this};
        QWidget _area_widget{this};

        QComboBox _metatype{this};
        std::deque<QLabel> _types;
        std::deque<QLabel> _keys;
        std::deque<QTextEdit> _value_edits;

        int _index;
    };

    class central_view_t: public QOpenGLWidget {
        Q_OBJECT;

    public:
        central_view_t(QWidget* parent):
            QOpenGLWidget::QOpenGLWidget(parent) {
            setAutoFillBackground(true);
            setBackgroundRole(QPalette::Base);
        }

    public slots:
        void paintEvent(QPaintEvent* event) {
            QPainter painter(this);

            painter.setBrush(Qt::white);
            painter.drawRect(rect());

            painter.setTransform({_zoom, 0, 0, _zoom, _offset.x(), _offset.y()});

            painter.drawPixmap(QPointF{0, 0}, draw_ui_controller_t::instance().background);

            painter.setBrush(Qt::white);
            for (auto& tile : draw_ui_controller_t::instance().data.tiles())
                painter.drawPolygon(tile.data(), tile.size());

            painter.setPen({QColor(0, 0, 255), draw_ui_controller_t::point_radius});
            for (auto& [point, times] : draw_ui_controller_t::instance().points)
                if (times) painter.drawPoint(point);
        }
        void mouseReleaseEvent(QMouseEvent* event) {
            if (event->button() == Qt::LeftButton && !_properties_view.isVisible()) {
                if (int index = draw_ui_controller_t::instance().polygon_of_point(event->pos()); index == -1) return;
                else {
                    _properties_view.load_tile(index);
                    _properties_view.show();
                }
            }
            update();
        }
        void wheelEvent(QWheelEvent* event) {
            _zoom += event->angleDelta().y() > 0 ? 0.1 : (_zoom < 0.11 ? 0 : -0.1);
            update();
        }

    private:
        QPointF _offset = {0, 0};
        double _zoom = 1.0;

        edit_properties_view_t _properties_view{this};

    };

    class draw_ui_view_t: public QMainWindow {
        Q_OBJECT;

    public:
        draw_ui_view_t(QWidget* parent = nullptr):
            QMainWindow::QMainWindow(parent) {

            QMenu* file_menu = menuBar()->addMenu(tr(u8"&File"));
            QAction* open_action = file_menu->addAction(tr(u8"&Open"));
            QAction* save_action = file_menu->addAction(tr(u8"&Save"));
            save_action->setEnabled(false);
            QAction* save_as_action = file_menu->addAction(tr(u8"Save &As"));
            save_as_action->setEnabled(false);
            QAction* close_action = file_menu->addAction(tr(u8"&Close"));
            close_action->setEnabled(false);

            QMenu* edit_menu = menuBar()->addMenu(tr(u8"&Edit"));
            QAction* edit_metatype_action = edit_menu->addAction(tr(u8"edit type(s)"));
            edit_metatype_action->setEnabled(false);
            edit_menu->addSeparator();
            QAction* add_object_action = edit_menu->addAction(tr(u8"Add object(s)"));
            add_object_action->setEnabled(false);
            QAction* finish_add_object_action = edit_menu->addAction(tr(u8"finish"));
            finish_add_object_action->setEnabled(false);
            QWidgetAction* choose_type_action = new QWidgetAction(edit_menu);
            QComboBox* types_combobox = new QComboBox;
            choose_type_action->setDefaultWidget(types_combobox);
            edit_menu->addAction(choose_type_action);
            choose_type_action->setEnabled(false);

            QMenu* about_menu = menuBar()->addMenu(tr(u8"About"));
            QAction* about_action = about_menu->addAction(tr(u8"About the application"));
            QAction* aboutqt_action = about_menu->addAction(tr(u8"About Qt"));

            connect(open_action, &QAction::triggered, this, [save_action, save_as_action, close_action, edit_metatype_action]() {
                if (QString name = QFileDialog::getOpenFileName(nullptr, u8"选择文件"); name.size()) {
                    draw_ui_controller_t::instance().load(name);
                    save_action->setEnabled(true);
                    save_as_action->setEnabled(true);
                    close_action->setEnabled(true);
                    edit_metatype_action->setEnabled(true);
                }
            });
            connect(save_action, &QAction::triggered, this, []() {
                draw_ui_controller_t::instance().save();
            });
            connect(save_as_action, &QAction::triggered, this, []() {
                if (QString name = QFileDialog::getOpenFileName(nullptr, u8"选择文件"); name.size()) {
                    draw_ui_controller_t::instance().change_name(name);
                    draw_ui_controller_t::instance().save();
                }
            });
            connect(close_action, &QAction::triggered, this, [save_action, save_as_action, close_action, edit_metatype_action]() {
                if (draw_ui_controller_t::instance().is_changed) {
                    switch (QMessageBox::warning(nullptr, "", u8"保存吗？", QMessageBox::Ok, QMessageBox::No, QMessageBox::Cancel)) {
                        case QMessageBox::Ok: draw_ui_controller_t::instance().save(); break;
                        default: return;
                    }
                }
                save_action->setEnabled(false);
                save_as_action->setEnabled(false);
                close_action->setEnabled(false);
                edit_metatype_action->setEnabled(false);
            });
            connect(edit_metatype_action, &QAction::triggered, this, [this]() {
                add_type_view.show();
            });
            connect(add_object_action, &QAction::triggered, this, [this, add_object_action, finish_add_object_action, choose_type_action, types_combobox]() {
                central_view.setParent(nullptr);
                setCentralWidget(&edit_view);
                add_object_action->setEnabled(false);
                finish_add_object_action->setEnabled(true);
                choose_type_action->setEnabled(true);

                types_combobox->clear();
                types_combobox->addItem(tr(u8"please choose a type"));
                for (const auto& [key, t] : draw_ui_controller_t::instance().data.metatypes())
                    types_combobox->addItem(QString::fromStdString(key));
            });
            connect(finish_add_object_action, &QAction::triggered, this, [this, add_object_action, finish_add_object_action, choose_type_action]() {
                edit_view.setParent(nullptr);
                setCentralWidget(&central_view);
                add_object_action->setEnabled(true);
                finish_add_object_action->setEnabled(false);
                choose_type_action->setEnabled(false);
            });
            connect(types_combobox, QOverload<int>::of(&QComboBox::activated), this, [types_combobox](int index) {
                if (index == 0) draw_ui_controller_t::instance().new_type = "";
                else draw_ui_controller_t::instance().new_type = types_combobox->currentText().toStdString();
            });
            connect(about_action, &QAction::triggered, this, [this]() {
                QMessageBox::about(this, tr(u8"About"), tr(u8"Qt5.15.12 learning.\n"
                    "Aiming to create a game engine for war strategy games like World Conqueror series from Esay Tech, Europa Universalis series from Paradox Development Studio and other board games like mahjong.\n"
                    "this is version beta0.1."));
            });
            connect(aboutqt_action, &QAction::triggered, this, [this]() {
                QMessageBox::aboutQt(this, tr(u8"About Qt"));
            });
            connect(&add_type_view, &edit_metatype_view_t::closed, this, [add_object_action]() {
                if (draw_ui_controller_t::instance().data.metatypes().size()) add_object_action->setEnabled(true);
                else add_object_action->setEnabled(false);
            });
        }

    private:
        central_view_t central_view{this};
        edit_view_t edit_view{this};
        edit_metatype_view_t add_type_view{this};
    };
}

#endif