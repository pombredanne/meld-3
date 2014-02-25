/* Copyright (C) 2014 Marco Brito <bcaza@null.net>
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

#include "diffgrid.h"
#include <map>

std::map<GdkWindow*, HandleWindow*> handles;

DiffGrid::DiffGrid(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gtk::Grid(cobject), m_refGlade(refGlade) {
    this->_in_drag = false;
    this->_drag_pos = -1;
    this->_drag_handle = nullptr;
    this->_handle1 = new HandleWindow();
    this->_handle2 = new HandleWindow();
}

DiffGrid::~DiffGrid() {
    delete _handle1;
    delete _handle2;
}

void DiffGrid::on_realize() {
    Gtk::Grid::on_realize();
    this->_handle1->realize(this);
    this->_handle2->realize(this);
}

void DiffGrid::on_unrealize() {
    this->_handle1->unrealize();
    this->_handle2->unrealize();
    Gtk::Grid::on_unrealize();
}

void DiffGrid::on_map() {
    Gtk::Grid::on_map();
    const Gtk::Widget* drag = this->get_child_at(2, 0);
    if (drag and drag->get_visible()) {
        this->_handle1->set_visible(true);
    }

    drag = this->get_child_at(4, 0);
    if (drag and drag->get_visible()) {
        this->_handle2->set_visible(true);
    }
}

void DiffGrid::on_unmap() {
    this->_handle1->set_visible(false);
    this->_handle2->set_visible(false);
    Gtk::Grid::on_unmap();
}

bool DiffGrid::_handle_set_prelight(GdkWindow* window, int flag) {
    if (handles.count(window) > 0) {
        handles[window]->set_prelight(flag);
        return true;
    }
    return false;
}

bool DiffGrid::on_enter_notify_event(GdkEventCrossing* event) {
    return this->_handle_set_prelight(event->window, true);
}

bool DiffGrid::on_leave_notify_event(GdkEventCrossing* event) {
    if (not this->_in_drag) {
        return this->_handle_set_prelight(event->window, false);
    }
    return false;
}

bool DiffGrid::on_button_press_event(GdkEventButton* event) {
    if (event->button & GDK_BUTTON_PRIMARY) {
        this->_drag_pos = event->x;
        this->_in_drag = true;
        return true;
    }
    return false;
}

bool DiffGrid::on_button_release_event(GdkEventButton* event) {
    if (event->button & GDK_BUTTON_PRIMARY) {
        this->_in_drag = false;
        return true;
    }
    return false;
}

bool DiffGrid::on_motion_notify_event(GdkEventMotion* event) {
    if (event->state & Gdk::BUTTON1_MASK) {
        if (handles.count(event->window) > 0) {
            gint x, y;
            gdk_window_get_position(event->window, &x, &y);
            int pos = std::round(x + event->x - this->_drag_pos);
            handles[event->window]->set_position(pos);
            this->_drag_handle = handles[event->window];
            this->queue_resize_no_redraw();
            return true;
        }
    }
    return false;
}

std::pair<int, int> DiffGrid::_calculate_positions(int xmin, int xmax, int wlink1, int wlink2,
                                                   int wpane1, int wpane2, int wpane3) {
    int wremain = std::max(0, xmax - xmin - wlink1 - wlink2);
    float pos1 = this->_handle1->get_position(wremain, xmin);
    float pos2 = this->_handle2->get_position(wremain, xmin + wlink1);

    if (not this->_drag_handle) {
        int npanes = 0;
        if (wpane1 > 0) {
            npanes += 1;
        }
        if (wpane2 > 0) {
            npanes += 1;
        }
        if (wpane3 > 0) {
            npanes += 1;
        }
        float wpane = float(wremain) / std::max(1, npanes);
        if (wpane1 > 0) {
            wpane1 = wpane;
        }
        if (wpane2 > 0) {
            wpane2 = wpane;
        }
        if (wpane3 > 0) {
            wpane3 = wpane;
        }
    }

    int xminlink1 = xmin + wpane1;
    int xmaxlink2 = xmax - wpane3 - wlink2;
    int wlinkpane = wlink1 + wpane2;

    if (wpane1 == 0) {
        pos1 = xminlink1;
    }
    if (wpane3 == 0) {
        pos2 = xmaxlink2;
    }
    if (wpane2 == 0) {
        if (wpane3 == 0) {
            pos1 = pos2 - wlink2;
        } else {
            pos2 = pos1 + wlink1;
        }
    }

    if (this->_drag_handle == this->_handle2) {
        int xminlink2 = xminlink1 + wlinkpane;
        pos2 = std::min(std::max((float)xminlink2, pos2), (float)xmaxlink2);
        int xmaxlink1 = pos2 - wlinkpane;
        pos1 = std::min(std::max((float)xminlink1, pos1), (float)xmaxlink1);
    } else {
        int xmaxlink1 = xmaxlink2 - wlinkpane;
        pos1 = std::min(std::max((float)xminlink1, pos1), (float)xmaxlink1);
        int xminlink2 = pos1 + wlinkpane;
        pos2 = std::min(std::max((float)xminlink2, pos2), (float)xmaxlink2);
    }

    this->_handle1->set_position(pos1);
    this->_handle2->set_position(pos2);
    return std::pair<int, int>(int(round(pos1)), int(round(pos2)));
}

int DiffGrid::get_child_prop_int(Gtk::Widget* child, std::string name) {
    GValue prop = G_VALUE_INIT;
    g_value_init(&prop, G_TYPE_INT);
    gtk_container_child_get_property(static_cast<Gtk::Container*>(this)->gobj(), child->gobj(), name.c_str(), &prop);
    return g_value_get_int(&prop);
}

std::vector<int> DiffGrid::get_child_attach(Gtk::Widget* child) {
    std::vector<int> attach = {
        get_child_prop_int(child, "left-attach"),
        get_child_prop_int(child, "top-attach"),
        get_child_prop_int(child, "width"),
        get_child_prop_int(child, "height")
    };
    return attach;
}

void DiffGrid::child_allocate(const std::vector<int>& columns, const std::vector<int>& yrows, Gtk::Widget* child) {
    if (not child->get_visible()) {
        return;
    }
    std::vector<int> attach = get_child_attach(child);
    int left = attach[0];
    int top = attach[1];
    int width = attach[2];
    int height = attach[3];
    Gtk::Allocation alloc = this->get_allocation();
    alloc.set_x(columns[left]);
    alloc.set_y(yrows[top]);
    alloc.set_width(columns[left + width] - columns[left]);
    alloc.set_height(yrows[top + height] - yrows[top]);
    child->size_allocate(alloc);
}

void DiffGrid::do_size_allocate(const Gtk::Allocation& allocation) {
    this->set_allocation(allocation);
    std::pair<std::vector<int>, std::vector<int>> tmp = this->_get_min_sizes();
    std::vector<int> wcols = tmp.first;
    std::vector<int> hrows = tmp.second;
    std::vector<int> yrows = {allocation.get_y(),
                              allocation.get_y() + hrows[0],
                              // Roughly equivalent to hard-coding row 1 to expand=True
                              allocation.get_y() + (allocation.get_height() - hrows[2]),
                              allocation.get_y() + allocation.get_height()};

    int wmap1 = wcols[0];
    int wpane1 = wcols[1];
    int wlink1 = wcols[2];
    int wpane2 = wcols[3];
    int wlink2 = wcols[4];
    int wpane3 = wcols[5];
    int wmap2 = wcols[6];
    int xmin = allocation.get_x() + wmap1;
    int xmax = allocation.get_x() + allocation.get_width() - wmap2;
    std::pair<int, int> tmp2 = this->_calculate_positions(xmin, xmax,
                                           wlink1, wlink2,
                                           wpane1, wpane2, wpane3);
    int pos1 = tmp2.first;
    int pos2 = tmp2.second;

    wpane1 = pos1 - (allocation.get_x() + wmap1);
    wpane2 = pos2 - (pos1 + wlink1);
    wpane3 = xmax - (pos2 + wlink2);
    wcols = { allocation.get_x(), wmap1, wpane1, wlink1, wpane2, wlink2, wpane3, wmap2 };
    std::vector<int> columns;
    for (size_t i = 0; i < wcols.size(); i++) {
        int sum;
        for (size_t j = 0; j < i + 1; j++) {
            sum += wcols[j];
        }
        columns.push_back(sum);
    }

    for (Gtk::Widget* child : this->get_children()) {
        child_allocate(columns, yrows, child);
    }

    if (this->get_realized()) {
        bool mapped = this->get_mapped();
        int ydrag = yrows[0];
        int hdrag = yrows[1] - yrows[0];
        this->_handle1->set_visible(mapped and wlink1 > 0);
        this->_handle1->move_resize(pos1, ydrag, wlink1, hdrag);
        this->_handle2->set_visible(mapped and wlink2 > 0);
        this->_handle2->move_resize(pos2, ydrag, wlink2, hdrag);
    }
}

std::pair<std::vector<int>, std::vector<int>> DiffGrid::_get_min_sizes() {
    std::vector<int> hrows = {0, 0, 3};
    std::vector<int> wcols = {0, 0, 0, 0, 0, 0, 0};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 7; col++) {
            Gtk::Widget* child = this->get_child_at(col, row);
            if (child and child->get_visible()) {
                Gtk::Requisition msize, nsize;
                child->get_preferred_size(msize, nsize);
                // Ignore spanning columns in width calculations; we should
                // do this properly, but it's difficult.
                GValue spanning = G_VALUE_INIT;
                g_value_init(&spanning, G_TYPE_INT);
                gtk_container_child_get_property(static_cast<Gtk::Container*>(this)->gobj(), child->gobj(), "width", &spanning);
                int _spanning = g_value_get_int(&spanning);
                if (_spanning == 1) {
                    wcols[col] = std::max(std::max(wcols[col], msize.width), nsize.width);
                }
                hrows[row] = std::max(std::max(hrows[row], msize.height), nsize.height);
            }
        }
    }
    return std::pair<std::vector<int>, std::vector<int>>(wcols, hrows);
}

bool DiffGrid::on_draw(const ::Cairo::RefPtr< ::Cairo::Context>& context) {
    bool result = Gtk::Grid::on_draw(context);
    this->_handle1->draw(context);
    this->_handle2->draw(context);
    return result;
}

HandleWindow::HandleWindow() {
    this->_widget = nullptr;
    this->_window.clear();
    this->_area_x = -1;
    this->_area_y = -1;
    this->_area_width = 1;
    this->_area_height = 1;
    this->_prelit = false;
    this->_pos = 0.0;
    this->_transform = std::pair<int, int>(0, 0);
}

float HandleWindow::get_position(int width, int xtrans) {
    this->_transform = std::pair<int, int>(width, xtrans);
    return float(this->_pos * width) + xtrans;
}

void HandleWindow::set_position(int pos) {
    int width = this->_transform.first;
    int xtrans = this->_transform.second;
    this->_pos = float(pos - xtrans) / width;
}

void HandleWindow::realize(Gtk::Widget* widget) {
    GdkWindowAttr attr;
    attr.window_type = GDK_WINDOW_CHILD;
    attr.x = this->_area_x;
    attr.y = this->_area_y;
    attr.width = this->_area_width;
    attr.height = this->_area_height;
    attr.wclass = GDK_INPUT_OUTPUT;
    attr.event_mask = (widget->get_events() |
                       Gdk::BUTTON_PRESS_MASK |
                       Gdk::BUTTON_RELEASE_MASK |
                       Gdk::ENTER_NOTIFY_MASK |
                       Gdk::LEAVE_NOTIFY_MASK |
                       Gdk::POINTER_MOTION_MASK);
    attr.cursor = gdk_cursor_new_for_display(widget->get_display()->gobj(),
                                             GDK_SB_H_DOUBLE_ARROW);
    int attr_mask = (Gdk::WA_X |
                 Gdk::WA_Y |
                 Gdk::WA_CURSOR);

    Glib::RefPtr<Gdk::Window> parent = widget->get_parent_window();
    this->_window = Gdk::Window::create(parent, &attr, attr_mask);
    handles[this->_window->gobj()] = this;
    this->_widget = widget;
    this->_widget->register_window(this->_window);
}

void HandleWindow::unrealize() {
    this->_widget->unregister_window(this->_window);
}

void HandleWindow::set_visible(bool visible) {
    if (visible) {
        this->_window->show();
    } else {
        this->_window->hide();
    }
}

void HandleWindow::move_resize(int x, int y, int width, int height) {
    this->_window->move(x, y);
    this->_window->resize(width, height);
    this->_area_x = x;
    this->_area_y = y;
    this->_area_width = width;
    this->_area_height = height;
}

void HandleWindow::set_prelight(bool flag) {
    this->_prelit = flag;
    this->_widget->queue_draw_area(this->_area_x, this->_area_y,
                                 this->_area_width, this->_area_height);
}

void HandleWindow::draw(const ::Cairo::RefPtr< ::Cairo::Context>& cairocontext) {
    Gtk::Allocation alloc = this->_widget->get_allocation();
    const int padding = 5;
    int x = this->_area_x - alloc.get_x() + padding;
    int y = this->_area_y - alloc.get_y() + padding;
    int width = std::max(0, this->_area_width - 2 * padding);
    int height = std::max(0, this->_area_height - 2 * padding);

    if (width == 0 or height == 0) {
        return;
    }

    Glib::RefPtr<Gtk::StyleContext> stylecontext = this->_widget->get_style_context();
    Gtk::StateFlags state = this->_widget->get_state_flags();
    if (this->_widget->is_focus()) {
        state |= Gtk::STATE_FLAG_SELECTED;
    }
    if (this->_prelit) {
        state |= Gtk::STATE_FLAG_PRELIGHT;
    }


    if (gtk_cairo_should_draw_window(cairocontext->cobj(), this->_window->gobj())) {
        gtk_style_context_save(stylecontext->gobj());
        stylecontext->set_state(state);
        stylecontext->add_class(GTK_STYLE_CLASS_PANE_SEPARATOR);
        Gdk::RGBA color = stylecontext->get_background_color(state);
        if (color.get_alpha() > 0.0) {
            stylecontext->render_handle(cairocontext,
                              x, y, width, height);
        } else {
            float xcenter = x + width / 2.0;
            stylecontext->render_line(cairocontext,
                            xcenter, y, xcenter, y + height);
        }
        gtk_style_context_restore(stylecontext->gobj());
    }
}
