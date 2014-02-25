/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010, 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "emblemcellrenderer.h"
#include "../util/compat.h"

EmblemCellRenderer::EmblemCellRenderer() : Gtk::CellRenderer() {
    this->_icon_name = "text-x-generic";
    this->_emblem_name = "";
    this->_icon_tint = "";
    this->_tint_color = 0;
    this->_state = 0;
    // FIXME: hardcoded sizes
    this->_icon_size = 16;
    this->_emblem_size = 8;
}

void EmblemCellRenderer::do_set_property(GParamSpec* pspec, std::string value) {
    std::string tmp = pspec->name;
    if (tmp == "icon-name") {
        this->_icon_name = value;
    } else if (tmp == "emblem-name") {
        this->_emblem_name = value;
    } else if (tmp == "icon-tint") {
        this->_icon_tint = value;
        if (!this->_icon_tint.empty()) {
            this->_tint_color = new Gdk::RGBA(value);
        } else {
            delete this->_tint_color;
            this->_tint_color = 0;
        }
    } else {
        throw AttributeError(std::string("unknown property ") + pspec->name);
    }
}

std::string EmblemCellRenderer::do_get_property(GParamSpec* pspec) {
    std::string tmp = pspec->name;
    if (tmp == "icon-name") {
        return this->_icon_name;
    } else if (tmp == "emblem-name") {
        return this->_emblem_name;
    } else if (tmp == "icon-tint") {
        return this->_icon_tint;
    } else {
        throw AttributeError(std::string("unknown property ") + pspec->name);
    }
}

Glib::RefPtr<Gdk::Pixbuf> EmblemCellRenderer::_get_pixbuf(std::string name, int size) {
    std::pair<std::string, int> key(name, size);
    if (this->icon_cache.count(key) == 0) {
        Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = icon_theme->load_icon(name, size, 0)->copy();
        this->icon_cache[key] = pixbuf;
    }
    return this->icon_cache[key];
}

void EmblemCellRenderer::do_render(const Cairo::RefPtr<Cairo::Context>& context, int widget, int background_area, Gdk::Rectangle &cell_area, int flags) {
    context->translate(cell_area.get_x(), cell_area.get_y());
    context->rectangle(0, 0, cell_area.get_width(), cell_area.get_height());
    context->clip();

    // TODO: Incorporate padding
    context->push_group();
    if (!this->_icon_name.empty()) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = this->_get_pixbuf(this->_icon_name, this->_icon_size);
        context->set_operator(Cairo::OPERATOR_SOURCE);
        // Assumes square icons; may break if we don't get the requested size
        int height_offset = int((cell_area.get_height() - pixbuf->get_height())/2);
        Gdk::Cairo::set_source_pixbuf(context, pixbuf, 0, height_offset);
        context->rectangle(0, height_offset,
                          pixbuf->get_width(), pixbuf->get_height());
        context->fill();

        if (this->_tint_color) {
            Gdk::RGBA* c = this->_tint_color;
            gushort r = c->get_red();
            gushort g = c->get_green();
            gushort b = c->get_blue();
            // Figure out the difference between our tint colour and an
            // empirically determined (i.e., guessed) satisfying luma and
            // adjust the base colours accordingly
            double luma = (r + r + b + g + g + g) / 6.;
            double extra_luma = (1.2 - luma) / 3.;
            r = std::min(r + extra_luma, 1.);
            g = std::min(g + extra_luma, 1.);
            b = std::min(b + extra_luma, 1.);
            context->set_source_rgba(r, g, b, 0.4);
            context->set_operator(Cairo::OPERATOR_ATOP);
            context->paint();
        }

        if (!this->_emblem_name.empty()) {
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = this->_get_pixbuf(this->_emblem_name, this->_emblem_size);
            int x_offset = this->_icon_size - this->_emblem_size;
            context->set_operator(Cairo::OPERATOR_OVER);
            Gdk::Cairo::set_source_pixbuf(context, pixbuf, x_offset, 0);
            context->rectangle(x_offset, 0,
                              cell_area.get_width(), this->_emblem_size);
            context->fill();
        }
    }

    context->pop_group_to_source();
    context->set_operator(Cairo::OPERATOR_OVER);
    context->paint();
}

std::tuple<int, int, int, int> EmblemCellRenderer::do_get_size(int widget, Gdk::Rectangle &cell_area) {
    // TODO: Account for cell_area if we have alignment set
    int x_offset = 0;
    int y_offset = 0;
    int width = this->_icon_size;
    int height = this->_icon_size;
    // TODO: Account for padding
    return std::tuple<int, int, int, int>(x_offset, y_offset, width, height);
}

