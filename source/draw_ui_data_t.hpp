#ifndef _HWSHQTB_DRAW_UI_DATA_T
#define _HWSHQTB_DRAW_UI_DATA_T

#include <QPointF>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <variant>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>

template <class CharT, class Traits>
inline std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& in, QPointF& point) {
    qreal x, y;
    in >> x >> y;
    point = {x, y};
    return in;
}
template <class CharT, class Traits>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, const QPointF& point) {
    out << point.x() << " " << point.y();
    return out;
}
inline bool qreal_equal(qreal a, qreal b) {
    return std::abs(a - b) <= std::numeric_limits<qreal>::epsilon();
}
inline bool qreal_less(qreal a, qreal b) {
    return !qreal_equal(a, b) && a < b;
}
inline bool operator<(const QPointF& a, const QPointF& b) {
    return qreal_equal(a.x(), b.x()) ? qreal_less(a.y(), b.y()) : qreal_less(a.x(), b.x());
}

namespace hwshqtb {
    class draw_ui_data_t {
    public:
        enum class type_t {
            number,
            color,
            string,
            external,
            fix,
        };
        struct value_t {
            type_t type;
            std::variant<std::monostate, double, std::uint32_t, std::string> value;
            QPointF position;
            int number;

            void change(const std::string& str, const QPointF& p, int n) {
                switch (type) {
                    case type_t::number: value = std::stod(str); break;
                    case type_t::color: value = (std::uint32_t)std::stoll(str); break;
                    case type_t::string: value = str.substr(1, str.size() - 2); break;
                    case type_t::external: value = str, position = p; break;
                    case type_t::fix: value = str, position = p, number = n; break;
                }
            }
            void change(type_t t, const std::string& str, const QPoint& p, int n) {
                type = t;
                change(str, p, n);
            }

            template <class CharT, class Traits>
            friend inline std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& in, value_t& value) {
                std::string str;
                QPointF p;
                int n;
                in >> str;
                if (value.type == type_t::external) in >> p;
                else if (value.type == type_t::fix) in >> p >> n;
                value.change(str, p, n);
                return in;
            }
            template <class CharT, class Traits>
            friend inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, const value_t& value) {
                std::visit([&out, &value](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::monostate>);
                    else if constexpr (std::is_same_v<T, double>) out << arg;
                    else if constexpr (std::is_same_v<T, uint32_t>) out << "#" << std::hex << arg << std::dec;
                    else if (value.type == draw_ui_data_t::type_t::string) out << "\"" << arg << "\"";
                    else if (value.type == draw_ui_data_t::type_t::external) out << arg << " " << value.position;
                    else out << arg << " " << value.number << " " << value.position;
                }, value.value);
                return out;
            }
        };
        using key_t = std::string;
        using meta_properties_t = std::map<key_t, type_t>;
        using meta_properties_name_t = std::string;
        using meta_properties_iterator_t = std::map<key_t, type_t>::iterator;
        using metatypes_t = std::map<meta_properties_name_t, meta_properties_t>;
        using metatypes_iterator_t = std::map<meta_properties_name_t, meta_properties_t>::iterator;
        struct meta_properties_iterator_less_t {
            inline bool operator()(const meta_properties_iterator_t& a, const meta_properties_iterator_t& b) const {
                return a->first < b->first;
            }
        };
        using properties_t = std::map<meta_properties_iterator_t, value_t, meta_properties_iterator_less_t>;
        using tile_t = std::vector<QPointF>;

    public:
        draw_ui_data_t(std::map<QPointF, int>& points):
            _points(points) {}
        ~draw_ui_data_t() = default;

        void load(std::string&& name) {
            _name = std::move(name);

            {
                std::ifstream input_meta(_name + "_meta");
                std::string type;
                int t;
                std::string property;
                while (!input_meta.eof()) {
                    input_meta >> type;
                    std::map<std::string, type_t> metatype;
                    while ((input_meta >> t).good()) {
                        input_meta >> property;
                        metatype.emplace(std::move(property), static_cast<type_t>(t));
                    }
                    add_metatype(std::move(type), std::move(metatype));
                }
            }
            {
                std::ifstream input(_name);
                input >> _width >> _height;
                std::string type;
                QPointF point;
                while (!input.eof()) {
                    input >> type;
                    add(std::move(type), {});
                    for (auto& [property, value] : _properties.back()) input >> value;
                    std::vector<QPointF>& tile = _tiles.back();
                    while ((input >> point).good()) {
                        ++_points[point];
                        tile.emplace_back(std::move(point));
                    }
                }
            }
            {
                std::ifstream input_link(_name + "_link");
                std::string str;
                while (!input_link.eof()) {
                    std::getline(input_link, str);
                    std::istringstream line{str};
                    std::vector<int> link;
                    while (!line.eof()) {
                        int to;
                        line >> to;
                        link.emplace_back(to);
                    }
                    _links.emplace_back(std::move(link));
                }
            }
        }
        void save()const {
            {
                std::ofstream output_meta(_name + "_meta");
                for (auto& [type, properties]: _metatypes) {
                    output_meta << type << " ";
                    for (auto& [property, t] : properties) {
                        output_meta << static_cast<uint8_t>(t) << " " << property << " ";
                    }
                    output_meta << "\n";
                }
            }
            {
                std::ofstream output(_name);
                output << _width << " " << _height << "\n";
                for (int i = 0; i < _types.size(); ++i) {
                    output << _types[i]->first << " ";
                    for (auto& [_, property] : _properties[i])
                        output << property << " ";
                    for (const QPointF& point : _tiles[i])
                        output << point << " ";
                    output << "\n";
                }
            }
            {
                std::ofstream output_link(_name + "_link");
                for (const std::vector<int>& link : _links) {
                    for (int to : link)
                        output_link << to << " ";
                    output_link << "\n";
                }
            }
        }

        metatypes_iterator_t add_metatype(meta_properties_name_t&& properties_name, meta_properties_t&& meta_properties) {
            return _metatypes.insert({std::move(properties_name), std::move(meta_properties)}).first;
        }
        void add(const meta_properties_name_t& properties_name, std::vector<QPointF>&& tile) {
            metatypes_iterator_t metatype_iterator = _metatypes.find(properties_name);
            properties_t metatype;
            for (meta_properties_iterator_t iterator = metatype_iterator->second.begin(); iterator != metatype_iterator->second.end(); ++iterator)
                metatype.try_emplace(iterator, std::move(value_t{iterator->second}));
            _properties.emplace_back(std::move(metatype));
            _types.emplace_back(std::move(metatype_iterator));
            _tiles.emplace_back(std::move(tile));
        }
        void remove_metatype(const meta_properties_name_t& properties_name) {
            for (int i = 0; i < _types.size(); ++i)
                if (_types[i]->first == properties_name) {
                    _types.erase(_types.begin() + i);
                    _properties.erase(_properties.begin() + i);
                    _tiles.erase(_tiles.begin() + i);
                }
            _metatypes.erase(properties_name);
        }
        void remove(int i) {
            _types.erase(_types.begin() + i);
            _properties.erase(_properties.begin() + i);
            _tiles.erase(_tiles.begin() + i);
        }

        metatypes_iterator_t metatypes_end() {
            return _metatypes.end();
        }
        const metatypes_t& metatypes()const {
            return _metatypes;
        }
        void width(qreal&& w) {
            _width = std::move(w);
        }
        const qreal& width()const {
            return _width;
        }
        void height(qreal&& h) {
            _height = std::move(h);
        }
        const qreal& height()const {
            return _height;
        }
        void type(int i, meta_properties_name_t&& properties_name) {
            metatypes_iterator_t metatype_iterator = _metatypes.find(properties_name);
            _types[i] = metatype_iterator;
            _properties[i].clear();
            for (meta_properties_iterator_t iterator = metatype_iterator->second.begin(); iterator != metatype_iterator->second.end(); ++iterator)
                _properties[i].try_emplace(iterator, std::move(value_t{iterator->second}));
        }
        const meta_properties_name_t& type(int i)const {
            return _types[i]->first;
        }
        void value(int i, const key_t& s, value_t&& v) {
            properties_t& properties = _properties[i];
            std::lower_bound(properties.begin(), properties.end(), s, [](const properties_t::value_type& a, const key_t& b) {
                return a.first->first < b;
            })->second = std::move(v);
        }
        const value_t& value(int i, const std::string& s)const {
            const properties_t& properties = _properties[i];
            return std::lower_bound(properties.cbegin(), properties.cend(), s, [](const properties_t::value_type& a, const key_t& b) {
                return a.first->first < b;
            })->second;
        }
        void tile(int i, tile_t&& t) {
            _tiles[i] = std::move(t);
        }
        const tile_t& tile(int i)const {
            return _tiles[i];
        }
        const std::vector<tile_t>& tiles()const {
            return _tiles;
        }
        void link(int i, int j, bool is_connected) {
            std::vector<int>::const_iterator iterator = std::find(_links[i].cbegin(), _links[i].cend(), j);
            if (iterator == _links[i].cend()) {
                if (is_connected) _links[i].emplace_back(j);
            }
            else {
                if (!is_connected) _links[i].erase(iterator);
            }
        }
        const std::vector<int>& link(int i)const {
            return _links[i];
        }

    private:
        std::string _name;
        // file1 type properties{property t}
        metatypes_t _metatypes;
        // file2
        double _width, _height;
        std::vector<metatypes_iterator_t> _types;
        std::vector<properties_t> _properties;
        std::vector<tile_t> _tiles;
        // file3
        std::vector<std::vector<int> > _links;

        std::map<QPointF, int>& _points;
    };
}

#endif