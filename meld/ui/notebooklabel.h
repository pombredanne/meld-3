/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2008-2009, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__UI__NOTEBOOKLABEL_H__
#define __MELD__UI__NOTEBOOKLABEL_H__

#include <gtkmm.h>
#include <functional>

class NotebookLabel : public Gtk::HBox {
private:
    std::string __gtype_name__ = "NotebookLabel";

    std::string css = "\
    * {\
        -GtkButton-default-border : 0;\
        -GtkButton-default-outside-border : 0;\
        -GtkButton-inner-border: 0;\
        -GtkWidget-focus-line-width : 0;\
        -GtkWidget-focus-padding : 0;\
        padding : 0;\
    }\
    ";
    int tab_width_in_chars = 30;

    Gtk::Label* __label;
    Gtk::Image* __icon1;
    Gtk::Image* __image;
    Gtk::EventBox* __label_box;
    Gtk::Button* __button;
    std::function<void()> __onclose;
public:
    void __on_close();

    NotebookLabel(const Gtk::StockID& iconname, const Glib::ustring& text, std::function<void()> onclose);
    virtual ~NotebookLabel();

    bool on_label_clicked(GdkEventButton* event);

    Glib::ustring get_label_text();

    void set_label_text(const Glib::ustring & text);
};

#endif
