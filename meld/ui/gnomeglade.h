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

#ifndef __MELD__UI__GNOMEGLADE_H__
#define __MELD__UI__GNOMEGLADE_H__

#include <gtkmm.h>

#include "../conf.h"
#include "../util/compat.h"
#include "../ui/gnomeglade.h"

class Component;

extern std::map<Gtk::Widget*, Component*> pyobject;

extern std::string gnomeglade_ui_file(std::string filename);

/*! Base class for all Gtk.Builder created objects
 *
 * This class loads the UI file, autoconnects signals, and makes
 * widgets available as attributes. The toplevel widget is stored as
 * 'this->widget'.
 *
 * The python object can be accessed from the widget itself via
 * widget.pygobject, which is sadly sometimes necessary.
 */
class Component {
public:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Widget* widget;

    /*! Load the widgets from the node 'root' in file 'filename' */
    Component(std::string filename, std::string root, std::list<std::string> extra = {});
    virtual ~Component();

};

#endif
