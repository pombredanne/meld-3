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

#include "listwidget.h"

ListWidget::ListWidget(std::string ui_file, std::string root, std::list<std::string> store, const Glib::ustring& treeview, int *new_row_data) : Component(ui_file, root, store) {
    this->new_row_data = new_row_data;
    this->builder->get_widget(treeview, this->list);
    this->model = this->list->get_model();
    Glib::RefPtr<Gtk::TreeSelection> selection = this->list->get_selection();
    selection->signal_changed().connect(sigc::mem_fun(*this, &ListWidget::_update_sensitivity));
}


void ListWidget::_update_sensitivity() {
    std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> t = this->_get_selected();
    Glib::RefPtr<Gtk::TreeModel> model = std::get<0>(t);
    Gtk::TreeRow it = std::get<1>(t);
    int path = std::get<2>(t);
    if (not it) {
        this->remove->set_sensitive(false);
        this->move_up->set_sensitive(false);
        this->move_down->set_sensitive(false);
    } else {
        this->remove->set_sensitive(true);
        this->move_up->set_sensitive(path > 0);
        this->move_down->set_sensitive(path < model->children().size() - 1);
    }
}


std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> ListWidget::_get_selected() {
    Gtk::TreeModel::iterator p = this->list->get_selection()->get_selected();
    Glib::RefPtr<Gtk::TreeModel> model = this->model;
    Gtk::TreeRow it = *p;
    int path;
    if (it) {
        path = model->get_path(it).front();
    } else {
        path = -1;
    }
    return std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int>(model, it, path);
}


void ListWidget::on_add_clicked(int button) {
#if 0
    this->model->append(this->new_row_data);
#endif
}


void ListWidget::on_remove_clicked(int button) {
    std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> t = this->_get_selected();
    Glib::RefPtr<Gtk::TreeModel> model = std::get<0>(t);
    Gtk::TreeRow it = std::get<1>(t);
    int path = std::get<2>(t);
#if 0
    model->remove(it);
#endif
}


void ListWidget::on_move_up_clicked(int button) {
    std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> t = this->_get_selected();
    Glib::RefPtr<Gtk::TreeModel> model = std::get<0>(t);
    Gtk::TreeRow it = std::get<1>(t);
    int path = std::get<2>(t);
#if 0
    model.swap(it, model.get_iter(path - 1));
#endif
}


int ListWidget::on_move_down_clicked(int button) {
    std::tuple<Glib::RefPtr<Gtk::TreeModel>, Gtk::TreeRow, int> t = this->_get_selected();
    Glib::RefPtr<Gtk::TreeModel> model = std::get<0>(t);
    Gtk::TreeRow it = std::get<1>(t);
    int path = std::get<2>(t);
#if 0
    model.swap(it, model.get_iter(path + 1));
#endif
}
