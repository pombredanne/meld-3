/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
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

#ifndef __MELD__LINKMAP_H__
#define __MELD__LINKMAP_H__

#include <array>
#include <gtkmm.h>

#include "filediff.h"
#include "sourceview.h"

class FileDiff;

class LinkMap : public Gtk::DrawingArea {
private:
    bool _setup;
    FileDiff *filediff;
    std::array<MeldSourceView*, 2> views;
    std::array<int, 2> view_indices;
    std::map<Glib::ustring, Gdk::RGBA> fill_colors;
    std::map<Glib::ustring, Gdk::RGBA> line_colors;
    int line_height;

    Glib::RefPtr<Gtk::Builder> m_refGlade;

public:
    LinkMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~LinkMap();

    void associate(FileDiff* filediff, MeldSourceView* left_view, MeldSourceView* right_view);

    void set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map);

    void do_draw(Cairo::RefPtr<Cairo::Context> context);

    void do_scroll_event(GdkEventScroll* event);
};

class ScrollLinkMap : public Gtk::DrawingArea {
private:
    bool _setup;
    MeldDoc* melddoc;

    Glib::RefPtr<Gtk::Builder> m_refGlade;

public:
    ScrollLinkMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~ScrollLinkMap();

    void associate(MeldDoc* melddoc);

    virtual bool on_scroll_event(GdkEventScroll* event);
};

#endif
