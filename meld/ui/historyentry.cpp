/* Copyright (C) 2008-2011, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

/* This file started off as a Python translation of:
 *  * gedit/gedit/gedit-history-entry.c
 *  * libgnomeui/libgnomeui/gnome-file-entry.c
 * roughly based on Colin Walters' Python translation of msgarea.py from Hotwire
 */

#define MIN_ITEM_LEN 3
#define HISTORY_ENTRY_HISTORY_LENGTH_DEFAULT 10


bool _remove_item(Glib::RefPtr<const Gtk::TreeModel> store, Glib::ustring &text) {
    if (text.empty()) {
        return false;
    }

    for (const Gtk::TreeRow row : store->children()) {
#if 0
        if (row[1] == text) {
            store->remove(row.iter);
            return true;
        }
#endif
    }
    return false;
}


void _clamp_list_store(Glib::RefPtr<Gtk::TreeModel> liststore, int max_items) {
    // -1 because TreePath counts from 0
    Gtk::TreeModel::iterator it = liststore->get_iter(std::to_string(max_items - 1));
    if (!it) {
        return;
    }
    bool valid = true;
    while (valid) {
#if 0
        valid = liststore->remove(it);
#endif
    }
}


class HistoryCombo : public Gtk::ComboBox {
    const std::string __gtype_name__ = "HistoryCombo";

    typedef sigc::signal<void> type_signal_history_id;
    type_signal_history_id signal_history_id();
    typedef sigc::signal<void> type_signal_history_length;
    type_signal_history_length signal_history_length();

#if 0
    Glib::Property<std::string> history_id(
        nick="History ID",
        blurb="Identifier associated with entry's history store",
        default_=None,
        flags=GObject.PARAM_READWRITE,
    );
#endif
    Glib::PropertyProxy<std::string> property_history_id();
    Glib::PropertyProxy_ReadOnly<std::string> property_history_id() const;

#if 0
    Glib::Property<int> history_length(
        nick="History length",
        blurb="Number of history items to display in the combo",
        minimum=1, maximum=20,
        default_=HISTORY_ENTRY_HISTORY_LENGTH_DEFAULT,
    );
#endif
    Glib::PropertyProxy<int> property_history_length();
    Glib::PropertyProxy_ReadOnly<int> property_history_length() const;

    std::string history_file;
    boost::property_tree::ptree config;

    HistoryCombo(int **kwargs) : Gtk::ComboBox(**kwargs) {

#if _WIN32
        std::string pref_dir = getenv("APPDATA") + "/Meld";
#else
        std::string pref_dir = Glib::get_user_config_dir() + "/meld";
#endif

        if (not boost::filesystem::exists(pref_dir)) {
            boost::filesystem::create_directories(pref_dir);
        }

        this->history_file = pref_dir + "/history.ini";
        if (boost::filesystem::exists(this->history_file)) {
            boost::property_tree::ini_parser::read_ini(this->history_file, this->config);
        }

#if 0
        this->set_model(new Gtk::ListStore(str, str));
#endif
        Gtk::CellRendererText rentext;
        rentext.property_width_chars() = 60;
        rentext.property_ellipsize() = Pango::ELLIPSIZE_END;
        this->pack_start(rentext, true);
        this->add_attribute(rentext, "text", 0);

        this->signal_history_id().connect(sigc::mem_fun(*this, &HistoryCombo::_load_history));
        this->signal_history_length().connect(sigc::mem_fun(*this, &HistoryCombo::_load_history));
    }

    void prepend_history(Glib::ustring &text) {
        this->_insert_history_item(text, true);
    }

    void append_history(Glib::ustring &text) {
        this->_insert_history_item(text, false);
    }

    void clear() {
        this->get_model().clear();
        this->_save_history();
    }

    void _insert_history_item(Glib::ustring &text, bool prepend) {
        if (text.length() <= MIN_ITEM_LEN) {
            return;
        }

        Glib::RefPtr<Gtk::TreeModel> store = this->get_model();
        if (not _remove_item(store, text)) {
            this->property_active();
            _clamp_list_store(store, this->property_history_length() - 1);
        }
#if 0
        row = (text.splitlines()[0], text);

        if (prepend) {
            store->insert(0, row);
        } else {
            store->append(row);
        }
#endif
        this->_save_history();
    }

    void _load_history() {
        Glib::PropertyProxy<std::string> section_key = this->property_history_id();
#if 0
        if (section_key.get_value().empty() or not this->config.has_section(section_key)) {
            return;
        }
#endif

        Glib::RefPtr<const Gtk::TreeModel> store = this->get_model();
#if 0
        store->clear();
        const boost::property_tree::basic_ptree<std::string>::self_type paths = this->config.get_child(section_key);
        std::sort(paths.begin(), paths.end());
        for (std::pair<int, int> p : paths[:this->props.history_length - 1]) {
            key = p.first;
            path = p.second;
            path = path.decode("string-escape");
            firstline = path.splitlines()[0];
            store.append((firstline, path));
        }
#endif
    }

    void _save_history() {
        Glib::PropertyProxy<std::string> section_key = this->property_history_id();
        if (section_key.get_value().empty()) {
            return;
        }
#if 0
        this->config.erase(section_key);
        for (int i = 0; i < this->get_model()->children().size(); i++) {
            Gtk::TreeRow row =this->get_model()->children()[i];
            message = row[1].encode("string-escape");
            this->config.set(section_key + ".item" + i, message);
        }
#endif
        boost::property_tree::write_ini(this->history_file, this->config);
    }
};
