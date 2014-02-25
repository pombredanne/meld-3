/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__UI__FINDBAR_H__
#define __MELD__UI__FINDBAR_H__

#include "gnomeglade.h"
#include <gtksourceview/gtksource.h>

class FindBar : public Component {
public:
    Gtk::TextView* textview;
private:
    Gdk::RGBA orig_base_color;
    Gtk::Arrow *arrow_left;
    Gtk::Arrow *arrow_right;
    Gtk::HBox *wrap_box;
    Gtk::Label *replace_label;
    Gtk::Entry *replace_entry;
    Gtk::HButtonBox *hbuttonbox2;
    Gtk::Entry *find_entry;
    Gtk::Button* find_next_button;
    Gtk::Button* find_previous_button;
    Gtk::CheckButton* match_case;
    Gtk::CheckButton* whole_word;
    Gtk::CheckButton* regex;
    Gtk::Button* replace_button;
    Gtk::Button* replace_all_button;
    GtkSourceSearchContext* search_context;
public:
    FindBar(Gtk::Container &parent);

    void on_focus_child(Gtk::Widget* widget);

    void hide();

    void set_text_view(Gtk::TextView* textview);

    void start_find(Gtk::TextView* textview, const Glib::ustring& text = "");

    void start_find_next(Gtk::TextView* textview);

    void start_find_previous(Gtk::TextView* textview, const Glib::ustring& text = "");

    void start_replace(Gtk::TextView* textview, const Glib::ustring& text = "");

    void on_find_next_button_clicked();

    void on_find_previous_button_clicked();

    void on_replace_button_clicked();

    void on_replace_all_button_clicked();

    void on_find_entry_changed();

    bool _find_text(int start_offset = 1, bool backwards = false, bool wrap = true);
};

#endif
