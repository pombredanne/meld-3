/* This file is part of the Hotwire Shell user interface.
 *
 * Copyright (C) 2007,2008 Colin Walters <walters@verbum.org>
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
 *
 * Additional modifications made for use in Meld and adaptations for
 * newer GTK+.
 * Copyright (C) 2013 Kai Willadsen <kai.willadsen@gmail.com>
 */

#ifndef __MELD__UI__MSGAREA_H__
#define __MELD__UI__MSGAREA_H__

#include <gtkmm.h>

extern Gtk::HBox* layout_text_and_icon(const Gtk::StockID& stockid, const Glib::ustring& primary_text, const Glib::ustring& secondary_text = "");

class MsgAreaController : public Gtk::HBox {
private:
    Gtk::InfoBar *__msgarea;
    int __msgid;

    Glib::RefPtr<Gtk::Builder> m_refGlade;
public:
    MsgAreaController(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~MsgAreaController();

    bool has_message();

    int get_msg_id();

    void set_msg_id(int msgid);

    void clear() ;

    Gtk::InfoBar* new_from_text_and_icon(const Gtk::StockID& stockid, const Glib::ustring& primary, const Glib::ustring& secondary = "");
};

#endif
