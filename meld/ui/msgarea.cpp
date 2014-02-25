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

#include "wraplabel.h"
#include "msgarea.h"

Gtk::HBox* layout_text_and_icon(const Gtk::StockID& stockid, const Glib::ustring& primary_text, const Glib::ustring& secondary_text) {
    Gtk::HBox* hbox_content = new Gtk::HBox(false, 8);
    hbox_content->show();

    Gtk::Image* image = new Gtk::Image(stockid, Gtk::ICON_SIZE_DIALOG);
    image->show();
    hbox_content->pack_start(*image, false, false, 0);
    image->set_alignment(0.5, 0.5);

    Gtk::VBox* vbox = new Gtk::VBox(false, 6);
    vbox->show();
    hbox_content->pack_start(*vbox, true, true, 0);

    std::string primary_markup = "<b>" + primary_text + "</b>";
    WrapLabel* primary_label = new WrapLabel(primary_markup);
    primary_label->show();
    vbox->pack_start(*primary_label, true, true, 0);
    primary_label->set_use_markup(true);
    primary_label->set_line_wrap(true);
    primary_label->set_alignment(0, 0.5);
    primary_label->set_can_focus(true);
    primary_label->set_selectable(true);

    if (!secondary_text.empty()) {
        std::string secondary_markup = "<small>" + secondary_text + "</small>";
        WrapLabel* secondary_label = new WrapLabel(secondary_markup);
        secondary_label->show();
        vbox->pack_start(*secondary_label, true, true, 0);
        secondary_label->set_can_focus(true);
        secondary_label->set_use_markup(true);
        secondary_label->set_line_wrap(true);
        secondary_label->set_selectable(true);
        secondary_label->set_alignment(0, 0.5);
    }

    return hbox_content;
}


MsgAreaController::MsgAreaController(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::HBox(cobject), m_refGlade(refGlade) {
    this->__msgarea = 0;
    this->__msgid = -1;
}

MsgAreaController::~MsgAreaController() {
}

bool MsgAreaController::has_message() {
    return this->__msgarea >= 0;
}

int MsgAreaController::get_msg_id() {
    return this->__msgid;
}

void MsgAreaController::set_msg_id(int msgid) {
    this->__msgid = msgid;
}

void MsgAreaController::clear() {
    if (this->__msgarea) {
        this->remove(*this->__msgarea);
        delete this->__msgarea;
        this->__msgarea = 0;
    }
    this->__msgid = -1;
}

Gtk::InfoBar* MsgAreaController::new_from_text_and_icon(const Gtk::StockID& stockid, const Glib::ustring& primary, const Glib::ustring& secondary) {
    this->clear();
    Gtk::InfoBar* msgarea = this->__msgarea = Gtk::manage(new Gtk::InfoBar());

    Gtk::HBox* content = layout_text_and_icon(stockid, primary, secondary);

    Gtk::Container* content_area = dynamic_cast<Gtk::Container*>(msgarea->get_content_area());
    for (Gtk::Widget* w : content_area->get_children()) {
        delete w;
    }
    content_area->get_children().clear();
    content_area->add(*content);

    this->pack_start(*msgarea, true, true, 0);
    return msgarea;
}
