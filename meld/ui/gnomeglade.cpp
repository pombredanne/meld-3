/* Copyright (C) 2002-2008 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "gnomeglade.h"

std::map<Gtk::Widget*, Component*> pyobject;

std::string gnomeglade_ui_file(std::string filename) {
    return DATADIR + "/ui/" + filename;
}

Component::Component(std::string filename, std::string root, std::list<std::string> extra) {
    this->builder = Gtk::Builder::create();
    this->builder->set_translation_domain(__package__);
    std::vector<Glib::ustring> objects;
    objects.push_back(root);
    objects.insert(objects.end(), extra.begin(), extra.end());
    filename = gnomeglade_ui_file(filename);
    this->builder->add_from_file(filename, objects);
#if 0
    this->builder->connect_property_changedect_signals(this);
#endif
    this->builder->get_widget(root, this->widget);
    pyobject[this->widget] = this;
}

Component::~Component() {
}
