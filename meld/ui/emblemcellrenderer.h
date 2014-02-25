/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010, 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
 * Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MELD__UI__EMBLEMCELLRENDERER_H__
#define __MELD__UI__EMBLEMCELLRENDERER_H__

#include <cairomm/cairomm.h>
#include <gtkmm.h>

class EmblemCellRenderer : public Gtk::CellRenderer {

    std::string __gtype_name__ = "EmblemCellRenderer";

#if 0
    std::map<std::string, std::tuple<std::string, std::string, std::string, int>> __gproperties__ = {
        {"icon-name",   {"Named icon",
                        "Name for base icon",
                        "text-x-generic", GObject.PARAM_READWRITE}},
        {"emblem-name", {"Named emblem icon",
                        "Name for emblem icon to overlay",
                        "", GObject::PARAM_READWRITE}},
        {"icon-tint",   {"Icon tint",
                        "GDK-parseable color to be used to tint icon",
                        "", GObject.PARAM_READWRITE}}
    };
#endif

    std::map<std::pair<std::string, int>, Glib::RefPtr<Gdk::Pixbuf>> icon_cache;

    std::string _icon_name;
    std::string _emblem_name;
    std::string _icon_tint;
    Gdk::RGBA* _tint_color;
    int *_state;
    int _icon_size;
    int _emblem_size;
public:
    EmblemCellRenderer();

    void do_set_property(GParamSpec* pspec, std::string value);

    std::string do_get_property(GParamSpec* pspec);

    Glib::RefPtr<Gdk::Pixbuf> _get_pixbuf(std::string name, int size);

    void do_render(const Cairo::RefPtr<Cairo::Context>& context, int widget, int background_area, Gdk::Rectangle &cell_area, int flags);

    std::tuple<int, int, int, int> do_get_size(int widget, Gdk::Rectangle &cell_area);

};


#endif
