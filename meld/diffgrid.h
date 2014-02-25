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

#include <gtkmm.h>

class HandleWindow;

class DiffGrid : public Gtk::Grid {
private:
    bool _in_drag;
    int _drag_pos;
    HandleWindow* _drag_handle;
    HandleWindow* _handle1;
    HandleWindow* _handle2;

    Glib::RefPtr<Gtk::Builder> m_refGlade;
public:
    DiffGrid(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~DiffGrid();

    virtual void on_realize();

    virtual void on_unrealize();

    virtual void on_map();

    virtual void on_unmap();

    bool _handle_set_prelight(GdkWindow* window, int flag);

    virtual bool on_enter_notify_event(GdkEventCrossing* event);

    virtual bool on_leave_notify_event(GdkEventCrossing* event);

    virtual bool on_button_press_event(GdkEventButton* event);

    virtual bool on_button_release_event(GdkEventButton* event);

    virtual bool on_motion_notify_event(GdkEventMotion* event);

    std::pair<int, int> _calculate_positions(int xmin, int xmax, int wlink1, int wlink2,
                              int wpane1, int wpane2, int wpane3);

private:
    int get_child_prop_int(Gtk::Widget* child, std::string name);

    std::vector<int> get_child_attach(Gtk::Widget* child);

    void child_allocate(const std::vector<int>& columns, const std::vector<int>& yrows, Gtk::Widget* child);

public:
    void do_size_allocate(const Gtk::Allocation& allocation);

    std::pair<std::vector<int>, std::vector<int>> _get_min_sizes();

    virtual bool on_draw(const ::Cairo::RefPtr< ::Cairo::Context>& context);

};


class HandleWindow {
private:
    Gtk::Widget* _widget;
    Glib::RefPtr<Gdk::Window> _window;
    int _area_x;
    int _area_y;
    int _area_width;
    int _area_height;
    bool _prelit;
    float _pos;
    std::pair<int, int> _transform;
public:
    HandleWindow();

    float get_position(int width, int xtrans);

    void set_position(int pos);

    void realize(Gtk::Widget* widget);

    void unrealize();

    void set_visible(bool visible);

    void move_resize(int x, int y, int width, int height);

    void set_prelight(bool flag);

    void draw(const ::Cairo::RefPtr< ::Cairo::Context>& cairocontext);
};
