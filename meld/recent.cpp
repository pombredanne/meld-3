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

#include "vc/_vc.h"

#include <gtkmm.h>
#include <cassert>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/format.hpp>

#include "misc.h"
#include "recent.h"
#include "util/compat.h"
#include "conf.h"

const std::string TYPE_FILE = "File";
const std::string TYPE_FOLDER = "Folder";
const std::string TYPE_VC = "Version control";
const std::string TYPE_MERGE = "Merge";
const std::vector<std::string> COMPARISON_TYPES = {TYPE_FILE, TYPE_FOLDER, TYPE_VC, TYPE_MERGE};

std::string unicodeify(std::string s) {
#if 0
    if (!s) {
        return nullptr;
    }
    if (isinstance(s, str)) {
        return s.decode(sys.getfilesystemencoding(), 'replace');
    }
#endif
    return s;
}

RecentFiles::RecentFiles(char* argv[]) {
    this->recent_manager = Gtk::RecentManager::get_default();
    this->recent_filter = Gtk::RecentFilter::create();
    this->recent_filter->add_mime_type(this->mime_type);
    this->_stored_comparisons.clear();
    this->app_exec = boost::filesystem::path(argv[0]).string();

    if (not boost::filesystem::exists(this->recent_path)) {
        boost::filesystem::create_directories(this->recent_path);
    }

    this->_clean_recent_files();
    this->_update_recent_files();
    this->recent_manager->signal_changed().connect(sigc::mem_fun(*this, &RecentFiles::_update_recent_files));
}

RecentFiles::~RecentFiles() {
    this->_stored_comparisons.clear();
}

std::string get_home() {
    char const* home = getenv("HOME");
    if (home or (home = getenv("USERPROFILE"))) {
        return home;
    } else {
        char const *drive = getenv("HOMEDRIVE");
        char const *path = getenv("HOMEPATH");
        return std::string(drive) + path;
    }
}

/*!
 * Add a tab to our recently-used comparison list
 *
 * The passed flags are currently ignored. In the future these are to be
 * used for extra initialisation not captured by the tab itself.
 */
void RecentFiles::add(MeldDoc* tab, int* flags) {
    std::pair<std::string, std::vector<std::string>> comparison_key = tab->get_comparison();
    std::string comp_type = comparison_key.first;
    std::vector<std::string> paths = comparison_key.second;

    // While Meld handles comparisons including None, recording these as
    // recently-used comparisons just isn't that sane.
    for (std::string path : paths) {
        if (path.empty()) {
            return;
        }
    }

    for (size_t i = 0; i < paths.size(); i++) {
        paths[i] = unicodeify(paths[i]);
    }
    // If a (type, paths) comparison is already registered, then re-add
    // the corresponding comparison file
    Glib::RefPtr<Gio::File> gio_file;
    if (this->_stored_comparisons.count(comparison_key) > 0) {
        gio_file = Gio::File::create_for_uri(this->_stored_comparisons[comparison_key]);
    } else {
        std::string recent_path = this->_write_recent_file(comp_type, paths);
        gio_file = Gio::File::create_for_path(recent_path);
    }

    std::string display_name;
    if (paths.size() > 1) {
        display_name = boost::algorithm::join(shorten_names(paths), " : ");
    } else {
        Glib::ustring display_path = paths[0];
        std::string userhome = get_home();
        if (boost::starts_with(display_path, userhome)) {
            // FIXME: What should we show on Windows?
            display_path = "~" + display_path.substr(userhome.size());
        }
        display_name = std::string(_("Version control:")) + " " + display_path;
    }
    // FIXME: Should this be translatable? It's not actually used anywhere.
    boost::format fmt("%s comparison\n%s");
    fmt % comp_type % boost::join(paths, ", ");
    Glib::ustring description = fmt.str();

    Gtk::RecentManager::Data recent_metadata;
    recent_metadata.mime_type = this->mime_type;
    recent_metadata.app_name = this->app_name;
    boost::format fmt2("%s --comparison-file %%u");
    fmt2 % this->app_exec;
    recent_metadata.app_exec = fmt.str();
    recent_metadata.display_name = display_name;
    recent_metadata.description = description;
    recent_metadata.is_private = true;
    this->recent_manager->add_item(gio_file->get_uri(), recent_metadata);
}

/*!
 * Read stored comparison from URI
 *
 * Returns the comparison type, the paths involved and the comparison
 * flags.
 */
std::tuple<std::string, std::vector<std::string>, int> RecentFiles::read(const std::string& uri) {
    Glib::RefPtr<Gio::File> gio_file = Gio::File::create_for_uri(uri);
    std::string path = gio_file->get_path();
    if (not gio_file->query_exists() or path.empty()) {
        throw IOError("File does not exist");
    }

    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini(path, config);

        config.get_child("Comparison");
        config.get_child("Comparison.type");
        config.get_child("Comparison.paths");

    } catch (boost::property_tree::ini_parser_error &e) {
        throw ValueError("Invalid recent comparison file");
    } catch (boost::property_tree::ptree_bad_path &e) {
        throw ValueError("Invalid recent comparison file");
    }

    std::string comp_type = config.get<std::string>("Comparison.type");
    std::vector<std::string> paths = split(config.get<std::string>("Comparison.paths"), ';');
    int flags;

    if (std::find(COMPARISON_TYPES.begin(), COMPARISON_TYPES.end(), comp_type) == COMPARISON_TYPES.end()) {
        throw ValueError("Invalid recent comparison file");
    }

    return std::tuple<std::string, std::vector<std::string>, int>(comp_type, paths, flags);
}

std::string RecentFiles::_write_recent_file(std::string comp_type, std::vector<std::string> paths) {
    for (size_t i = 0; i < paths.size(); i++) {
        paths[i] = unicodeify(paths[i]);
    }
    // TODO: Use GKeyFile instead, and return a Gio.File. This is why we're
    // using ';' to join comparison paths.
    std::string name = this->recent_path + "/" + boost::filesystem::unique_path("recent-%%%%-%%%%-%%%%-%%%%" + this->recent_suffix).string();

    boost::property_tree::ptree pt;
    pt.add("Comparison.type", comp_type);
    pt.add("Comparison.paths", boost::algorithm::join(paths, ";"));
    boost::property_tree::ini_parser::write_ini(name, pt);
    return name;
}

void RecentFiles::_clean_recent_files() {
    // Remove from RecentManager any comparisons with no existing file
    std::vector<Glib::RefPtr<Gtk::RecentInfo>> _meld_items = this->_filter_items(this->recent_filter,
                                    this->recent_manager->get_items());
    for (Glib::RefPtr<Gtk::RecentInfo> item : _meld_items) {
        if (not item->exists()) {
            this->recent_manager->remove_item(item->get_uri());
        }
    }

    std::list<Glib::RefPtr<Gtk::RecentInfo>> meld_items;
    for (Glib::RefPtr<Gtk::RecentInfo> item : _meld_items) {
        if (item->exists()) {
            meld_items.push_back(item);
        }
    }

    // Remove any comparison files that are not listed by RecentManager
    std::list<Glib::ustring> item_uris;
    for (Glib::RefPtr<Gtk::RecentInfo> item : meld_items) {
        item_uris.push_back(item->get_uri());
    }
    std::list<std::string> item_paths;
    for (Glib::ustring uri : item_uris) {
        item_paths.push_back(Gio::File::create_for_uri(uri)->get_path());
    }
    std::list<std::string> stored;
    for (std::string p : os_listdir(this->recent_path)) {
        if (boost::algorithm::ends_with(p, this->recent_suffix)) {
            stored.push_back(p);
        }
    }
    for (std::string path : stored) {
        std::string file_path = boost::filesystem::canonical(boost::filesystem::path(path)).string();
        if (std::find(item_paths.begin(), item_paths.end(), file_path) == item_paths.end()) {
            boost::filesystem::remove(boost::filesystem::path(file_path));
        }
    }
}

void RecentFiles::_update_recent_files() {
    std::vector<Glib::RefPtr<Gtk::RecentInfo>> meld_items = this->_filter_items(this->recent_filter, this->recent_manager->get_items());
    std::vector<Glib::ustring> item_uris;
    for (Glib::RefPtr<Gtk::RecentInfo> item : meld_items) {
        if (item->exists()) {
            item_uris.push_back(item->get_uri());
        }
    }
    this->_stored_comparisons.clear();
    for (Glib::ustring uri : item_uris) {
        std::tuple<std::string, std::vector<std::string>, int> comp;
        try {
            comp = this->read(uri);
        } catch (IOError) {
            continue;
        } catch (ValueError) {
            continue;
        }
        // Store and look up comparisons by type and paths, ignoring flags
        std::pair<std::string, std::vector<std::string>> p(std::get<0>(comp), std::get<1>(comp));
        this->_stored_comparisons[p] = uri;
    }
}

std::vector<Glib::RefPtr<Gtk::RecentInfo>> RecentFiles::_filter_items(Glib::RefPtr<Gtk::RecentFilter> recent_filter, std::vector<Glib::RefPtr<Gtk::RecentInfo>> items) {
    static const std::map<Gtk::RecentFilterFlags, std::string> getters = {
            {Gtk::RECENT_FILTER_URI, "uri"},
            {Gtk::RECENT_FILTER_DISPLAY_NAME, "display_name"},
            {Gtk::RECENT_FILTER_MIME_TYPE, "mime_type"},
            {Gtk::RECENT_FILTER_APPLICATION, "applications"},
            {Gtk::RECENT_FILTER_GROUP, "groups"},
            {Gtk::RECENT_FILTER_AGE, "age"}
    };
    Gtk::RecentFilterFlags needed = recent_filter->get_needed();
    std::list<std::string> attrs;
    for (std::pair<Gtk::RecentFilterFlags, std::string> p : getters) {
        Gtk::RecentFilterFlags k = p.first;
        std::string v = p.second;
        if (needed & k) {
            attrs.push_back(v);
        }
    }

    std::vector<Glib::RefPtr<Gtk::RecentInfo>> filtered_items;
    for (Glib::RefPtr<Gtk::RecentInfo> i : items) {
        Gtk::RecentFilter::Info filter_info;
        filter_info.contains = recent_filter->get_needed();
        for (std::string attr : attrs) {
            if (attr == "uri") {
                filter_info.uri = i->get_uri();
            } else if (attr == "display_name") {
                filter_info.display_name = i->get_display_name();
            } else if (attr == "mime_type") {
                filter_info.mime_type = i->get_mime_type();
            } else if (attr == "applications") {
                // https://bugzilla.gnome.org/show_bug.cgi?id=695970
                continue;
            } else if (attr == "groups") {
                // https://bugzilla.gnome.org/show_bug.cgi?id=695970
                continue;
            } else if (attr == "age") {
                filter_info.age = i->get_age();
            }
        }
#if 0
        if (recent_filter->filter(filter_info)) {
            filtered_items.push_back(i);
        }
#endif
    }
    return filtered_items;
}

std::string RecentFiles::__str__() {
    std::vector<Glib::RefPtr<Gtk::RecentInfo>> items = this->recent_manager->get_items();
    std::stringstream descriptions;
    for (Glib::RefPtr<Gtk::RecentInfo> i : this->_filter_items(this->recent_filter, items)) {
        descriptions << i->get_display_name() << "\n" << i->get_uri_display() << "\n";
    }
    return descriptions.str();
}

RecentFiles* recent_comparisons;
