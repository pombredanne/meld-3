/* Copyright (C) 2013-2014 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "gutterrendererchunk.h"
#include "diffutil.h"
#include "merge.h"
#include "conf.h"

Glib::RefPtr<Gdk::Pixbuf> load(std::string icon_name) {
    const Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
    return icon_theme->load_icon(icon_name, LINE_HEIGHT, (Gtk::IconLookupFlags)0);
}

GutterRendererChunkAction::GutterRendererChunkAction(int from_pane, int to_pane, std::pair<Gtk::TextView*, Gtk::TextView*> views, FileDiff* filediff, _Differ* linediffer) : Gsv::GutterRendererPixbuf() {
    this->from_pane = from_pane;
    this->to_pane = to_pane;
    // FIXME: Views are needed only for editable checking; connect to this
    // in Filediff instead?
    this->views = views;
    // FIXME: Don't pass in the linediffer; pass a generator like elsewhere
    this->linediffer = linediffer;
    this->mode = MODE_REPLACE;
    this->set_size(LINE_HEIGHT);
    std::string direction;
    if (from_pane < to_pane) {
        direction = "LTR";
    } else {
        direction = "RTL";
    }
    this->action_map = GutterRendererChunkAction::ACTION_MAP[direction];
    this->filediff = filediff;
    this->filediff->signal_action_mode_changed().connect(sigc::mem_fun(this, &GutterRendererChunkAction::on_container_mode_changed));
}

void GutterRendererChunkAction::on_activate(const Gtk::TextIter& start, const Gdk::Rectangle& area, GdkEvent* event) {
    int line = start.get_line();
    int chunk_index = this->linediffer->locate_chunk(this->from_pane, line)[0];
    if (!chunk_index) {
        return;
    }

    // FIXME: This is all chunks, not just those shared with to_pane
    difflib::chunk_t chunk = this->linediffer->get_chunk(chunk_index, this->from_pane);
    if (std::get<1>(chunk) != line) {
        return;
    }

    int action = this->_classify_change_actions(chunk);
    if (action == MODE_DELETE) {
        this->filediff->delete_chunk(this->from_pane, chunk);
    } else if (action == MODE_INSERT) {
        Gtk::Menu* copy_menu = this->_make_copy_menu(chunk);
        // TODO: Need a custom GtkMenuPositionFunc to position this next to
        // the clicked gutter, not where the cursor is
        copy_menu->popup(0, 0); // TODO: egore: this was passing event.time as the last parameter
    } else {
        this->filediff->replace_chunk(this->from_pane, this->to_pane, chunk);
    }
}

void GutterRendererChunkAction::copy_chunk(const difflib::chunk_t& chunk, bool copy_up) {
    this->filediff->copy_chunk(this->from_pane, this->to_pane, chunk,
                             copy_up);
}

Gtk::Menu* GutterRendererChunkAction::_make_copy_menu(const difflib::chunk_t& chunk) {
    Gtk::Menu* copy_menu = Gtk::manage(new Gtk::Menu());
    Gtk::MenuItem copy_up(_("Copy _up"), true);
    Gtk::MenuItem copy_down(_("Copy _down"), true);
    copy_menu->append(copy_up);
    copy_menu->append(copy_down);
    copy_menu->show_all();

    // FIXME: This is horrible
    Gtk::VBox *widget = static_cast<Gtk::VBox*>(this->filediff->widget);
    copy_menu->attach_to_widget(*widget);

    copy_up.signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GutterRendererChunkAction::copy_chunk), chunk, true));
    copy_down.signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GutterRendererChunkAction::copy_chunk), chunk, false));

    return copy_menu;
}

bool GutterRendererChunkAction::on_query_activatable(const Gtk::TextIter& start, const Gdk::Rectangle& area, GdkEvent* event) {
    int line = start.get_line();
    int chunk_index = this->linediffer->locate_chunk(this->from_pane, line)[0];
    if (chunk_index >= 0) {
        // FIXME: This is all chunks, not just those shared with to_pane
        difflib::chunk_t chunk = this->linediffer->get_chunk(chunk_index, this->from_pane);
        if (std::get<1>(chunk) == line) {
            return true;
        }
    }
    return false;
}

void GutterRendererChunkAction::on_query_data(const Gtk::TextIter& start, const Gtk::TextIter& end, Gsv::GutterRendererState state) {
    int line = start.get_line();
    int chunk_index = this->linediffer->locate_chunk(this->from_pane, line)[0];

    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    if (chunk_index >= 0) {
        difflib::chunk_t chunk = this->linediffer->get_chunk(
                    chunk_index, this->from_pane, this->to_pane);
        if (chunk != difflib::EMPTY_CHUNK and std::get<1>(chunk) == line) {
            int action = this->_classify_change_actions(chunk);
            pixbuf = this->action_map[action];
        }
    }
    if (pixbuf) {
        this->set_pixbuf(pixbuf);
    } else {
        this->property_pixbuf() = pixbuf;
    }
}

void GutterRendererChunkAction::on_container_mode_changed(int mode) {
    this->mode = mode;
    this->queue_draw();
}

/*!
 * Classify possible actions for the given change
 *
 * Returns the action that can be performed given the content and
 * context of the change.
 */
int GutterRendererChunkAction::_classify_change_actions(const difflib::chunk_t& change) {
    Gtk::TextView* editable = this->views.first;
    Gtk::TextView* other_editable = this->views.second;

    if (not editable and not other_editable) {
        return -1;
    }

    // Reclassify conflict changes, since we treat them the same as a
    // normal two-way change as far as actions are concerned
    std::string change_type = std::get<0>(change);
    if (change_type == "conflict") {
        if (std::get<1>(change) == std::get<2>(change)) {
            change_type = "insert";
        } else if (std::get<3>(change) == std::get<4>(change)) {
            change_type = "delete";
        } else {
            change_type = "replace";
        }
    }

    int action = -1;
    if (change_type == "delete") {
        if (editable and (this->mode == MODE_DELETE or not other_editable)) {
            action = MODE_DELETE;
        } else if (other_editable) {
            action = MODE_REPLACE;
        }
    } else if (change_type == "replace") {
        if (not editable) {
            if (this->mode == MODE_INSERT || this->mode == MODE_REPLACE) {
                action = this->mode;
            }
        } else if (not other_editable) {
            action = MODE_DELETE;
        } else {
            action = this->mode;
        }
    }

    return action;
}
