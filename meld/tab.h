/* Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
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

#ifndef __MELD__TAB_H__
#define __MELD__TAB_H__

#include <gtkmm.h>
#include "ui/gnomeglade.h"

class Tab : public Glib::Object, public Component {
public:
    Tab(std::string filename, std::string root, std::list<std::string> extra = {});
    ~Tab();

    /*! Called when the container app switches to this tab. */
    virtual void on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> uimanager) = 0;

    /*! Called when the container app switches away from this tab. */
    virtual void on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> uimanager) = 0;

    virtual Gtk::ResponseType on_delete_event(int appquit = 0) = 0;
};

#endif
