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

#ifndef __MELD__RECENT_H__
#define __MELD__RECENT_H__

/*!
 * \file
 * Recent files integration for Meld's multi-element comparisons
 *
 * The GTK+ recent files mechanism is designed to take only single files with a
 * limited set of metadata. In Meld, we almost always need to enter pairs or
 * triples of files or directories, along with some information about the
 * comparison type. The solution provided by this module is to create fake
 * single-file registers for multi-file comparisons, and tell the recent files
 * infrastructure that that's actually what we opened.
 */

#include <gtkmm.h>

#include "ui/gnomeglade.h"
#include "melddoc.h"

extern const std::string TYPE_FILE;
extern const std::string TYPE_FOLDER;
extern const std::string TYPE_VC;
extern const std::string TYPE_MERGE;


class RecentFiles {
private:
    const std::string mime_type = "application/x-meld-comparison";
    std::string recent_path = Glib::get_user_data_dir() + "/meld";
#if 0
    recent_path = recent_path.decode('utf8');
#endif
    const std::string recent_suffix = ".meldcmp";

    // Recent data
    const std::string app_name = "Meld";

    Glib::RefPtr<Gtk::RecentManager> recent_manager;
public:
    Glib::RefPtr<Gtk::RecentFilter> recent_filter;
private:
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> _stored_comparisons;
    std::string app_exec;

public:
    RecentFiles(char* argv[]);
    ~RecentFiles();

    /*!
     * Add a tab to our recently-used comparison list
     *
     * The passed flags are currently ignored. In the future these are to be
     * used for extra initialisation not captured by the tab itself.
     */
    void add(MeldDoc* tab, int* flags = 0);

    /*!
     * Read stored comparison from URI
     *
     * Returns the comparison type, the paths involved and the comparison
     * flags.
     */
    std::tuple<std::string, std::vector<std::string>, int> read(const std::string& uri);

    std::string _write_recent_file(std::string comp_type, std::vector<std::string> paths);

    void _clean_recent_files();

    void _update_recent_files();

    std::vector<Glib::RefPtr<Gtk::RecentInfo>> _filter_items(Glib::RefPtr<Gtk::RecentFilter> recent_filter, std::vector<Glib::RefPtr<Gtk::RecentInfo>> items);

    std::string __str__();
};

extern RecentFiles* recent_comparisons;

#endif
