/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__MELDAPP_H__
#define __MELD__MELDAPP_H__

#include <gtkmm.h>

#include "meldwindow.h"

class MeldApp : public Gtk::Application {
public:
    MeldApp();

    virtual void on_startup();

    virtual void on_activate();

    void done(int tab, int status);

    virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line);

    virtual void on_window_removed(Gtk::Window* window);

    void preferences_callback(const Glib::VariantBase& variant);

    void help_callback(const Glib::VariantBase& variant);

    void about_callback(const Glib::VariantBase& variant);

    void quit_callback(const Glib::VariantBase& variant);

    std::shared_ptr<MeldWindow> new_window();

    std::shared_ptr<MeldWindow> get_meld_window();

    MeldDoc* open_files(std::vector<std::string> files, bool new_tab, bool auto_compare, bool auto_merge, bool focus);

    /*! Gather --diff arguments and append to a list */
    void diff_files_callback(int option, int opt_str, int value, int parser);

    int run_(int argc, char** argv);

    MeldDoc* parse_args(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line);

};

extern MeldApp app;

#endif
