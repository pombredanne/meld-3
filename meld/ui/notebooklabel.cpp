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

#include "notebooklabel.h"

#include "../util/compat.h"
#include "../conf.h"

void NotebookLabel::__on_close() {
    __onclose();
}

NotebookLabel::NotebookLabel(const Gtk::StockID& iconname, const Glib::ustring& text, std::function<void()> onclose) : Gtk::HBox(false, 4) {

    Gtk::Label* label = Gtk::manage(new Gtk::Label(text));
    // FIXME: ideally, we would use custom ellipsization that ellipsized the
    // two paths separately, but that requires significant changes to label
    // generation in many different parts of the code
    label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
    label->set_single_line_mode(true);
    label->set_alignment(0.0, 0.5);
    label->set_padding(0, 0);

    Glib::RefPtr<Pango::Context> context = this->get_pango_context();
    Pango::FontDescription font_desc = this->get_style_context()->get_font(Gtk::STATE_FLAG_NORMAL);
    Pango::FontMetrics metrics = context->get_metrics(font_desc, context->get_language());
    int char_width = metrics.get_approximate_char_width() / Pango::SCALE;
    int w, h;
    Gtk::IconSize::lookup(Gtk::ICON_SIZE_MENU, w, h, this->get_settings());
    // FIXME: PIXELS replacement
    this->set_size_request(this->tab_width_in_chars * char_width + 2 * w, -1);

    Gtk::Button* button = Gtk::manage(new Gtk::Button());
    button->set_relief(Gtk::RELIEF_NONE);
    button->set_focus_on_click(false);
    Glib::RefPtr<Gio::ThemedIcon> icon = Gio::ThemedIcon::create("window-close-symbolic");

    Gtk::Image* image = Gtk::manage(new Gtk::Image());
    image->set((Glib::RefPtr<const Gio::Icon>&)icon, Gtk::ICON_SIZE_MENU);
    image->set_tooltip_text(_("Close tab"));
    button->add(*image);
    button->set_name("meld-tab-close-button");
    button->signal_clicked().connect(sigc::mem_fun(this, &NotebookLabel::__on_close));

    Glib::RefPtr<Gtk::StyleContext> context1 = button->get_style_context();
    Glib::RefPtr<Gtk::CssProvider> provider = Gtk::CssProvider::create();
    provider->load_from_data(this->css);
    context1->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    Gtk::Image* icon1 = Gtk::manage(new Gtk::Image(iconname, Gtk::ICON_SIZE_MENU));

    Gtk::EventBox* label_box = Gtk::manage(new Gtk::EventBox());
    label_box->add_events(Gdk::EventMask::BUTTON_PRESS_MASK);
    label_box->property_visible_window() = false;
    label_box->signal_button_press_event().connect(sigc::mem_fun(*this, &NotebookLabel::on_label_clicked));
    label_box->add(*label);

    this->pack_start(*icon1, false, true, 0);
    this->pack_start(*label_box, true, true, 0);
    this->pack_start(*button, false, true, 0);
    this->set_tooltip_text(text);
    this->show_all();

    this->__label = label;
    this->__icon1 = icon1;
    this->__label_box = label_box;
    this->__image = image;
    this->__button = button;
    this->__onclose = onclose;
}

NotebookLabel::~NotebookLabel() {
    delete __label;
    delete __icon1;
    delete __label_box;
    delete __image;
    delete __button;
}

bool NotebookLabel::on_label_clicked(GdkEventButton* event) {
    if (event->type == GDK_BUTTON_PRESS and event->button == 2) {
        this->__onclose();
    }
    return false;
}

Glib::ustring NotebookLabel::get_label_text() {
    return this->__label->get_text();
}

void NotebookLabel::set_label_text(const Glib::ustring & text) {
    this->__label->set_text(text);
    this->set_tooltip_text(text);
}
