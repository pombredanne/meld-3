/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
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

#include <cmath>

#include "linkmap.h"

LinkMap::LinkMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::DrawingArea(cobject), m_refGlade(refGlade) {
    this->_setup = false;
}

LinkMap::~LinkMap() {
}

void LinkMap::associate(FileDiff* filediff, MeldSourceView* left_view, MeldSourceView* right_view) {
    this->filediff = filediff;
    this->views = { left_view, right_view };
    this->view_indices[0] = 0;
    this->view_indices[1] = 1;
    if (this->get_direction() == Gtk::TEXT_DIR_RTL) {
        std::reverse(views.begin(), views.end());
        this->view_indices[0] = 1;
        this->view_indices[1] = 0;
    }

    this->set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>>(filediff->fill_colors, filediff->line_colors));

    this->line_height = filediff->pixels_per_line;

    this->_setup = true;
}

void LinkMap::set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>> color_map) {
    this->fill_colors = color_map.first;
    this->line_colors = color_map.second;
    this->queue_draw();
}

void LinkMap::do_draw(Cairo::RefPtr<Cairo::Context> context) {
    if (not this->_setup) {
        return;
    }

    context->set_line_width(1.0);
    Gtk::Allocation allocation = this->get_allocation();
    Glib::RefPtr<Gtk::StyleContext> style = this->get_style_context();

    std::vector<int> pix_start;
    std::vector<int> y_offset;
    int tmp = 0;
    for (Gtk::TextView* t : this->views) {
        Gdk::Rectangle visible_rect;
        t->get_visible_rect(visible_rect);
        pix_start.push_back(visible_rect.get_y());
        int x, y;
        t->translate_coordinates(static_cast<Gtk::Widget&>(*this), 0, 0, x, y);
        y_offset.push_back(y);
        tmp = std::max(tmp, visible_rect.get_height());
    }
    int clip_height = tmp + 2;
    style->render_frame(context, 0.0d, 0.0d, (double)allocation.get_width(), (double)clip_height);
    context->rectangle(0, -1, allocation.get_width(), clip_height);
    context->clip();

    int height = allocation.get_height();
    std::vector<int> visible = {this->views[0]->get_line_num_for_y(pix_start[0]),
                      this->views[0]->get_line_num_for_y(pix_start[0] + height),
                      this->views[1]->get_line_num_for_y(pix_start[1]),
                      this->views[1]->get_line_num_for_y(pix_start[1] + height)};

    int wtotal = allocation.get_width();
    // For bezier control points
    double x_steps[4] = {-0.5, (1. / 3) * wtotal, (2. / 3) * wtotal, wtotal + 0.5};
    // Rounded rectangle corner radius for culled changes display
    float radius = this->line_height / 3;
    double q_rad = M_PI / 2;

    int left = this->view_indices[0];
    int right = this->view_indices[1];
    for (difflib::chunk_t c : this->filediff->linediffer->pair_changes(left, right, visible)) {
        // f and t are short for "from" and "to"
        int f0 = (this->views[0]->get_y_for_line_num(1) - pix_start[0] + y_offset[0]);
        int f1 = (this->views[0]->get_y_for_line_num(2) - pix_start[0] + y_offset[0]);
        int t0 = (this->views[1]->get_y_for_line_num(3) - pix_start[1] + y_offset[1]);
        int t1 = (this->views[1]->get_y_for_line_num(4) - pix_start[1] + y_offset[1]);

        // If either endpoint is completely off-screen, we cull for clarity
        if ((t0 < 0 and t1 < 0) or (t0 > height and t1 > height)) {
            if (f0 == f1) {
                continue;
            }
            context->arc(x_steps[0], f0 - 0.5 + radius, radius, -q_rad, 0);
            context->arc(x_steps[0], f1 - 0.5 - radius, radius, 0, q_rad);
            context->close_path();
        } else if ((f0 < 0 and f1 < 0) or (f0 > height and f1 > height)) {
            if (t0 == t1) {
                continue;
            }
            context->arc_negative(x_steps[3], t0 - 0.5 + radius, radius,
                                 -q_rad, q_rad * 2);
            context->arc_negative(x_steps[3], t1 - 0.5 - radius, radius,
                                 q_rad * 2, q_rad);
            context->close_path();
        } else {
            context->move_to(x_steps[0], f0 - 0.5);
            context->curve_to(x_steps[1], f0 - 0.5,
                             x_steps[2], t0 - 0.5,
                             x_steps[3], t0 - 0.5);
            context->line_to(x_steps[3], t1 - 0.5);
            context->curve_to(x_steps[2], t1 - 0.5,
                             x_steps[1], f1 - 0.5,
                             x_steps[0], f1 - 0.5);
            context->close_path();
        }

        Gdk::RGBA tmp = this->fill_colors[std::get<0>(c)];
        context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
        context->fill_preserve();

        int chunk_idx = this->filediff->linediffer->locate_chunk(left, std::get<1>(c))[0];
        if (chunk_idx == this->filediff->cursor->chunk) {
            Gdk::RGBA highlight = this->fill_colors["current-chunk-highlight"];
            context->set_source_rgba(highlight.get_red(), highlight.get_green(), highlight.get_blue(),
                                     highlight.get_alpha());
            context->fill_preserve();
        }

        tmp = this->line_colors[std::get<0>(c)];
        context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
        context->stroke();
    }
}

void LinkMap::do_scroll_event(GdkEventScroll* event) {
    this->filediff->next_diff(event->direction);
}

ScrollLinkMap::ScrollLinkMap(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::DrawingArea(cobject), m_refGlade(refGlade) {
    this->_setup = false;
}

ScrollLinkMap::~ScrollLinkMap() {
}

void ScrollLinkMap::associate(MeldDoc* melddoc) {
    this->melddoc = melddoc;
    this->_setup = true;
}

bool ScrollLinkMap::on_scroll_event(GdkEventScroll* event) {
    if (not this->_setup) {
        return false;
    }
    this->melddoc->next_diff(event->direction);
    return false;
}
