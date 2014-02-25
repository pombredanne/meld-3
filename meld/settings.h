/* Copyright (C) 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__SETTINGS_H__
#define __MELD__SETTINGS_H__

#include <gtkmm.h>

#include "filters.h"

class MeldSettings;

extern Glib::RefPtr<Gio::Settings> settings;
extern Glib::RefPtr<Gio::Settings> interface_settings;
extern std::shared_ptr<MeldSettings> meldsettings;

/*! Handler for settings that can't easily be bound to object properties */
class MeldSettings : public Glib::Object {
public:

    typedef sigc::signal<void> type_signal_file_filters_changed;
    type_signal_file_filters_changed signal_file_filters_changed() {
        return m_signal_file_filters_changed;
    }
    type_signal_file_filters_changed m_signal_file_filters_changed;
    typedef sigc::signal<void> type_signal_text_filters_changed;
    type_signal_text_filters_changed signal_text_filters_changed() {
        return m_signal_text_filters_changed;
    }
    type_signal_text_filters_changed m_signal_text_filters_changed;
    typedef sigc::signal<void, const Glib::ustring&> type_signal_changed;
    type_signal_changed signal_changed() {
        return m_signal_changed;
    }
    type_signal_changed m_signal_changed;

    std::vector<FilterEntry *> file_filters;
    std::vector<FilterEntry*> text_filters;
private:
    Pango::FontDescription _font;

public:

    MeldSettings();

    void on_setting_changed(const Glib::ustring& key);

    std::vector<FilterEntry *> _filters_from_gsetting(const Glib::ustring&  key, FilterEntry::Type filt_type);

    Pango::FontDescription _current_font_from_gsetting();

    Pango::FontDescription font();

};


extern void create_settings(bool uninstalled = false);

void bind_settings(Glib::ObjectBase* obj, std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__);

#endif
