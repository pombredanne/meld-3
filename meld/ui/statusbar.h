/* Copyright (C) 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__STATUSBAR_H__
#define __MELD__STATUSBAR_H__

#include <gtkmm.h>

class MeldStatusBar : public Gtk::Statusbar {
private:
    Gtk::HBox* info_box;
    Glib::RefPtr<Gtk::Builder> m_refGlade;
public:
    MeldStatusBar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~MeldStatusBar();
    void set_info_box(std::vector<Gtk::Label*> widgets);
};

#endif
