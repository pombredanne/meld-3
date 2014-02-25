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

#ifndef __MELD__UI__HISTORYENTRY_H__
#define __MELD__UI__HISTORYENTRY_H__


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


extern bool _remove_item(Glib::RefPtr<const Gtk::TreeModel> store, Glib::ustring &text);


extern void _clamp_list_store(Glib::RefPtr<Gtk::TreeModel> liststore, int max_items);


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

    HistoryCombo(int **kwargs);

    void prepend_history(Glib::ustring &text);

    void append_history(Glib::ustring &text);

    void clear();

    void _insert_history_item(Glib::ustring &text, bool prepend);

    void _load_history();

    void _save_history();
};

#endif
