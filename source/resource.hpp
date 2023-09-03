#ifndef _HWSHQTB_DRAW_UI_RESOURCE
#define _HWSHQTB_DRAW_UI_RESOURCE

#include <QObject>
#include <QPixmap>
#include <QIcon>
#include <QStringList>

namespace hwshqtb {
    class resources_t {
    public:
        static const resources_t& instance() {
            static resources_t resources;
            return resources;
        }

    private:
        resources_t() = default;

    public:
        // QPixmap
        const QPixmap undetermined_pic{":/buttons/undetermined.png"};
        const QPixmap add_pic{":/buttons/add.png"};
        const QPixmap sub_pic{":/buttons/sub.png"};
        const QPixmap fold_pic{":/buttons/fold.png"};
        const QPixmap unfold_pic{":/buttons/unfold.png"};
        //QIcon
        const QIcon undetermined_icon{":/buttons/undetermined.png"};
        const QIcon add_icon{":/buttons/add.png"};
        const QIcon sub_icon{":/buttons/sub.png"};
        const QIcon fold_icon{":/buttons/fold.png"};
        const QIcon unfold_icon{":/buttons/unfold.png"};
        //String
        const QStringList metatype_name{{QObject::tr(u8"number")},
            {QObject::tr(u8"color")},
            {QObject::tr(u8"string")},
            {QObject::tr(u8"external")},
            {QObject::tr(u8"fix")}};
    };
}

#endif