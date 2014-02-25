/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010-2011, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__UI__LISTWIDGET_H__
#define __MELD__UI__LISTWIDGET_H__

#include "gnomeglade.h"

class ListWidget : public Component {

protected:
    int *new_row_data;
    Gtk::TreeView* list;
    Glib::RefPtr<Gtk::TreeModel> model;

    Gtk::ToolButton *remove;
    Gtk::ToolButton *move_up;
    Gtk::ToolButton *move_down;

public:
    ListWidget(std::string ui_file, std::string root, std::list<std::string> store, const Glib::ustring& treeview, int *new_row_data = 0);

    void _update_sensitivity();

    std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> _get_selected();

    void on_add_clicked(int button);

    void on_remove_clicked(int button);

    void on_move_up_clicked(int button);

    int on_move_down_clicked(int button);

};

#endif
