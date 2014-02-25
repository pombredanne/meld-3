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

#include "statusbar.h"

#if 0
Gtk::rc_parse_string(
    ""
    "style \"meld-statusbar-style\" {"
    "    GtkStatusbar::shadow-type = GTK_SHADOW_NONE"
    "}"
    "class \"MeldStatusBar\" style \"meld-statusbar-style\""
    "");
#endif

MeldStatusBar::MeldStatusBar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::Statusbar(cobject), m_refGlade(refGlade) {
    this->property_spacing() = 6;

    Gtk::Box* hbox = const_cast<Gtk::Box*>(dynamic_cast<const Gtk::Box*>(this->get_message_area()));
    Gtk::Label* label = const_cast<Gtk::Label*>(dynamic_cast<const Gtk::Label *>(hbox->get_children()[0]));
    hbox->property_spacing() = 6;
    label->property_ellipsize() = Pango::ELLIPSIZE_NONE;
    hbox->remove(*label);
    hbox->pack_start(*label, true, true, 0);

    Gtk::Alignment alignment(1.0, 0.5, 1.0, 1.0);
    this->info_box = Gtk::manage(new Gtk::HBox(false, 12));
    this->info_box->show();
    alignment.add(*this->info_box);
    this->pack_start(alignment, true, true, 0);
    alignment.show();
}

MeldStatusBar::~MeldStatusBar() {
}

void MeldStatusBar::set_info_box(std::vector<Gtk::Label*> widgets) {
    for (Gtk::Widget* child : this->info_box->get_children()) {
        this->info_box->remove(*child);
    }
    for (Gtk::Label* widget : widgets) {
        this->info_box->pack_end(*widget, false, true, 0);
    }
}
