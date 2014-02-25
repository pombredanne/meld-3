/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2011-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__TREE_H__
#define __MELD__TREE_H__

#include <gtkmm.h>
#include <pangomm.h>

enum Col {
    COL_PATH = 0, COL_STATE, COL_TEXT, COL_ICON, COL_TINT, COL_FG, COL_STYLE,
    COL_WEIGHT, COL_STRIKE, COL_END
};

extern std::vector<const std::type_info*> COL_TYPES;

#include "vc/_vc.h"

class TextAttribute {
public:
    Glib::ustring fg;
    Pango::Style style;
    Pango::Weight weight;
    bool strike;
    TextAttribute(Glib::ustring fg, Pango::Style style, Pango::Weight weight, bool strike);
};

class DiffTreeStore : public Gtk::TreeStore {
public:
    int ntree;
private:
    std::vector<TextAttribute *> text_attributes;
    std::vector<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>> icon_details;

public:
    DiffTreeStore(int ntree, std::vector<const std::type_info*> types);

    void on_style_updated(Gtk::Widget* widget);

    Glib::ustring lookup(Glib::RefPtr<Gtk::StyleContext> style, std::string name, const Glib::ustring& _default);

    void _setup_default_styles(Glib::RefPtr<Gtk::StyleContext> style);

    std::vector<Glib::ustring> value_paths(Gtk::TreeModel::iterator it);

    Glib::ustring value_path(Gtk::TreeModel::iterator it, int pane);

    bool is_folder(const Gtk::TreeModel::iterator& it, int pane, std::string path);

    int column_index(Col col, int pane);

    Gtk::TreeStore::iterator add_entries(const Gtk::TreeNodeChildren* parent, std::vector<std::string> names);

    void add_empty(const Gtk::TreeNodeChildren& parent, std::string text = "empty folder");

    void add_error(const Gtk::TreeNodeChildren& parent, Glib::ustring msg, int pane);

    void set_path_state(const Gtk::TreeModel::iterator& it, int pane, FileState state, int isdir = 0);

    void set_state(const Gtk::TreeModel::iterator& it, int pane, FileState state, Glib::ustring label, int isdir = 0);

    FileState get_state(const Gtk::TreeModel::iterator& it, int pane);

    std::vector<Gtk::TreeModel::iterator> inorder_search_down(Gtk::TreeModel::iterator& it);

    std::vector<Gtk::TreeModel::iterator> inorder_search_up(Gtk::TreeModel::iterator& it);

    std::pair<Gtk::TreeModel::Path, Gtk::TreeModel::Path> _find_next_prev_diff(Gtk::TreeModel::Path start_path);

    bool treeview_search_cb(const Glib::RefPtr<TreeModel>& model, int column, const Glib::ustring& key, const Gtk::TreeModel::iterator& iter);

};

#endif
