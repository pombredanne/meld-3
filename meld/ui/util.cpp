/* Copyright (C) 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "util.h"

#include <iostream>

#include "../conf.h"
#include "../util/compat.h"

std::string ui_file(std::string filename) {
    return DATADIR + "/ui/" + filename;
}


Glib::RefPtr<Glib::Object> get_widget(std::string filename, const Glib::ustring& widget) {
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
    builder->set_translation_domain(__package__);
    builder->add_from_file(ui_file(filename), widget);
    return builder->get_object(widget);
}


Glib::RefPtr<Gtk::Builder> get_builder(std::string filename) {
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
    builder->set_translation_domain(__package__);

    try {
        builder->add_from_file(ui_file(filename));
    } catch(const Glib::FileError& ex) {
        std::cerr << "FileError: " << ex.what() << std::endl;
    } catch(const Glib::MarkupError& ex) {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
    } catch(const Gtk::BuilderError& ex) {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
    }

    return builder;
}
