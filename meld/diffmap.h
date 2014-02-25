/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__DIFFMAP_H__
#define __MELD__DIFFMAP_H__

#include <gtkmm.h>
#include <functional>
#include <set>

#include "difflib/src/difflib.h"

class DiffMap : public Gtk::DrawingArea {
private:

    Glib::RefPtr<Gtk::Adjustment> _scrolladj;
    std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> _difffunc;
    std::list<sigc::connection> _handlers;
    int _y_offset;
    int _h_offset;
    int _scroll_y;
    int _scroll_height;
    int _setup;
    int _width;
    Cairo::RefPtr<Cairo::Surface> _cached_map;
    std::map<Glib::ustring, Gdk::RGBA> fill_colors;
    std::map<Glib::ustring, Gdk::RGBA> line_colors;

    Glib::RefPtr<Gtk::Builder> m_refGlade;

public:

    DiffMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~DiffMap();

    void setup(Gtk::Scrollbar* scrollbar, std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> change_chunk_fn, std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map);

    void on_diffs_changed(std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>>);

    void set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map);

    void on_scrollbar_style_updated(Gtk::Scrollbar* scrollbar);

    void on_scrollbar_size_allocate(Gtk::Allocation &allocation);

    void do_draw(Cairo::Context& context);

    bool do_button_press_event(GdkEventButton *event);

    std::tuple<int, int> do_get_preferred_width();
};

#endif
