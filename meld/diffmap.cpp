/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "diffmap.h"
#include <cmath>

DiffMap::DiffMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::DrawingArea(cobject), m_refGlade(refGlade) {
    this->add_events(Gdk::EventMask::BUTTON_PRESS_MASK);
    this->_scrolladj.clear();
    this->_difffunc = 0;
    this->_handlers.clear();
    this->_y_offset = 0;
    this->_h_offset = 0;
    this->_scroll_y = 0;
    this->_scroll_height = 0;
    this->_setup = false;
    this->_width = 10;
}

DiffMap::~DiffMap() {
}

void DiffMap::setup(Gtk::Scrollbar* scrollbar, std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> change_chunk_fn, std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map) {
    for (sigc::connection h : this->_handlers) {
        h.disconnect();
    }

    this->_scrolladj = scrollbar->get_adjustment();
    this->on_scrollbar_style_updated(scrollbar);
    Gtk::Allocation allocation = scrollbar->get_allocation();
    this->on_scrollbar_size_allocate(allocation);
    sigc::connection scroll_style_hid = scrollbar->signal_style_updated().connect(
                                         sigc::bind(sigc::mem_fun(this, &DiffMap::on_scrollbar_style_updated), scrollbar));
    sigc::connection scroll_size_hid = scrollbar->signal_size_allocate().connect(
                                         sigc::mem_fun(this, &DiffMap::on_scrollbar_size_allocate));
    sigc::connection adj_change_hid = this->_scrolladj->signal_changed().connect(
                                         sigc::mem_fun(this, &DiffMap::queue_draw));
    sigc::connection adj_val_hid = this->_scrolladj->signal_value_changed().connect(
                                         sigc::mem_fun(this, &DiffMap::queue_draw));
    this->_handlers = {scroll_style_hid,
                       scroll_size_hid,
                       adj_change_hid,
                       adj_val_hid};

    this->_difffunc = change_chunk_fn;
    this->set_color_scheme(color_map);
    this->_setup = true;
    this->_cached_map.clear();
    this->queue_draw();
}

void DiffMap::on_diffs_changed(std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>>) {
    this->_cached_map.clear();
}

void DiffMap::set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map) {
    this->fill_colors = color_map.first;
    this->line_colors = color_map.second;
    this->queue_draw();
}

void DiffMap::on_scrollbar_style_updated(Gtk::Scrollbar* scrollbar) {
    int stepper_size, stepper_spacing;
    scrollbar->get_style_property("stepper-size", stepper_size);
    scrollbar->get_style_property("stepper-spacing", stepper_spacing);

    bool has_backward, has_secondary_forward, has_secondary_backward, has_foreward;
    scrollbar->get_style_property("has-backward-stepper", has_backward);
    scrollbar->get_style_property("has-secondary-forward-stepper", has_secondary_forward);
    scrollbar->get_style_property("has-secondary-backward-stepper", has_secondary_backward);
    scrollbar->get_style_property("has-forward-stepper", has_foreward);
    std::vector<bool> steppers = { has_backward, has_secondary_forward, has_secondary_backward, has_foreward };

    int offset = 0;
    for (int i = 0; i < 2; i++) {
        offset += stepper_size;
    }
    int shorter;
    for (int i = 0; i < 4; i++) {
        shorter += stepper_size;
    }
    if (steppers[0] or steppers[1]) {
        offset += stepper_spacing;
        shorter += stepper_spacing;
    }
    if (steppers[2] or steppers[3]) {
        shorter += stepper_spacing;
    }
    this->_y_offset = offset;
    this->_h_offset = shorter;
    this->queue_draw();
}

void DiffMap::on_scrollbar_size_allocate(Gtk::Allocation &allocation) {
    this->_scroll_y = allocation.get_y();
    this->_scroll_height = allocation.get_height();
    this->_width = std::max(allocation.get_width(), 10);
    this->_cached_map.clear();
    this->queue_resize();
}

void DiffMap::do_draw(Cairo::Context& context) {
    if (not this->_setup) {
        return;
    }
    int height = this->_scroll_height - this->_h_offset - 1;
    int y_start = this->_scroll_y - this->get_allocation().get_y() - this->_y_offset + 1;
    int width = this->get_allocated_width();
    double xpad = 2.5;
    double x0 = xpad;
    double x1 = width - 2 * xpad;

    context.translate(0, y_start);
    context.set_line_width(1);
    context.rectangle(x0 - 3, -1, x1 + 6, height + 1);
    context.clip();

    if (!this->_cached_map) {
        const Cairo::RefPtr<Cairo::Surface> surface = Cairo::Surface::create(
            context.get_target(), Cairo::CONTENT_COLOR_ALPHA,
            width, height);
        Cairo::RefPtr<Cairo::Context> cache_ctx = Cairo::Context::create(surface);
        cache_ctx->set_line_width(1);

        std::map<Glib::ustring, std::vector<std::pair<int, int>>> tagged_diffs;
        for (std::tuple<Glib::ustring, int, int> t : this->_difffunc()) {
            Glib::ustring c = std::get<0>(t);
            int y0 = std::get<1>(t);
            int y1 = std::get<2>(t);
            if (tagged_diffs.count(c) == 0) {
                tagged_diffs[c] = std::vector<std::pair<int, int>>();
            }
            tagged_diffs[c].push_back(std::pair<int, int>(y0, y1));
        }
        for (std::pair<Glib::ustring, std::vector<std::pair<int, int>>> p : tagged_diffs) {
            Glib::ustring tag = p.first;
            std::vector<std::pair<int, int>> diffs = p.second;
            Gdk::RGBA tmp = this->fill_colors[tag];
            cache_ctx->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
            for (std::pair<int, int> q : diffs) {
                int y0 = q.first;
                int y1 = q.second;
                y0 = round(y0 * height) - 0.5;
                y1 = round(y1 * height) - 0.5;
                cache_ctx->rectangle(x0, y0, x1, y1 - y0);
            }
            cache_ctx->fill_preserve();
            tmp = this->line_colors[tag];
            cache_ctx->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
            cache_ctx->stroke();
        }
        this->_cached_map = surface;
    }

#if 0
    context.set_source_surface(this->_cached_map, 0., 0.);
#endif
    context.paint();

    static const double page_color[4] = {0., 0., 0., 0.1};
    static const double page_outline_color[4] = {0.0, 0.0, 0.0, 0.3};
    Glib::RefPtr<const Gtk::Adjustment> adj = this->_scrolladj;
    double s = round(height * (adj->get_value() / adj->get_upper())) - 0.5;
    double e = round(height * (adj->get_page_size() / adj->get_upper()));
    context.set_source_rgba(page_color[0], page_color[1], page_color[2], page_color[3]);
    context.rectangle(x0 - 2, s, x1 + 4, e);
    context.fill_preserve();
    context.set_source_rgba(page_outline_color[0], page_outline_color[1], page_outline_color[2], page_outline_color[3]);
    context.stroke();
}

bool DiffMap::do_button_press_event(GdkEventButton *event) {
    if (event->button == 1) {
        int y_start = this->get_allocation().get_y() - this->_scroll_y - this->_y_offset;
        int total_height = this->_scroll_height - this->_h_offset;
        double fraction = (event->y + y_start) / total_height;

        Glib::RefPtr<Gtk::Adjustment> adj = this->_scrolladj;
        double val = fraction * adj->get_upper() - adj->get_page_size() / 2;
        double upper = adj->get_upper() - adj->get_page_size();
        adj->set_value(std::max(std::min(upper, val), adj->get_lower()));
        return true;
    }
    return false;
}

std::tuple<int, int> DiffMap::do_get_preferred_width() {
    return std::tuple<int, int>(this->_width, this->_width);
}
