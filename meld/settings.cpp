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

#include <boost/filesystem.hpp>

#include "conf.h"
#include "filters.h"
#include "settings.h"

const std::string MELD_SCHEMA = "org.gnome.meld";

Glib::RefPtr<Gio::Settings> settings;
Glib::RefPtr<Gio::Settings> interface_settings;
std::shared_ptr<MeldSettings> meldsettings;

MeldSettings::MeldSettings() : Glib::Object() {
    this->on_setting_changed("filename-filters");
    this->on_setting_changed("text-filters");
    this->on_setting_changed("use-system-font");
    settings->signal_changed().connect(sigc::mem_fun(this, &MeldSettings::on_setting_changed));
}

void MeldSettings::on_setting_changed(const Glib::ustring& key) {
    if (key == "filename-filters") {
        this->file_filters = this->_filters_from_gsetting(
            "filename-filters", FilterEntry::SHELL);
        this->m_signal_file_filters_changed.emit();
    } else if (key == "text-filters") {
        this->text_filters = this->_filters_from_gsetting(
            "text-filters", FilterEntry::REGEX);
        this->m_signal_text_filters_changed.emit();
    } else if (key == "use-system-font" || key == "custom-font") {
        this->_font = this->_current_font_from_gsetting();
        this->m_signal_changed.emit("font");
    }
}

std::vector<FilterEntry *> MeldSettings::_filters_from_gsetting(const Glib::ustring&  key, FilterEntry::Type filt_type) {
    Glib::Variant<std::vector<std::tuple<std::string, bool, std::string>>> filter_params;
    settings->get_value(key, filter_params);
    std::vector<FilterEntry *> filters;
    std::vector<std::tuple<std::string, bool, std::string>> tmp;
#if 0
    tmp = filter_params.get();
#endif
    for (std::tuple<std::string, bool, std::string> params : tmp) {
        filters.push_back(FilterEntry::new_from_gsetting(params, filt_type));
    }
    return filters;
}

Pango::FontDescription MeldSettings::_current_font_from_gsetting() {
    Glib::ustring font_string;
    if (settings->get_boolean("use-system-font")) {
        font_string = interface_settings->get_string("monospace-font-name");
    } else {
        font_string = settings->get_string("custom-font");
    }
    return Pango::FontDescription(font_string);
}

Pango::FontDescription MeldSettings::font() {
    return this->_font;
}


#if 0
static std::string find_schema() {
    schema_source = Gio::SettingsSchemaSource::new_from_directory(
        DATADIR,
        Gio::SettingsSchemaSource::get_default(),
        false
    );
    return schema_source.lookup(MELD_SCHEMA, false);
}
#endif

#if 0
static Glib::RefPtr<Glib::SettingsBackend> check_backend() {
    bool force_ini = boost::filesystem::exists(
        Glib::get_user_config_dir() + "/meld/use-rc-prefs");
    if (force_ini) {
        // TODO: Use GKeyfileSettingsBackend once available (see bgo#682702)
        std::cout << "Using a flat-file settings backend is not yet supported" << std::endl;
        return nullptr;
    }
    return nullptr;
}
#endif

void create_settings(bool uninstalled) {

#if 0
    Glib::RefPtr<Glib::SettingsBackend> backend = check_backend();
#endif
    if (uninstalled) {
#if 0
        std::string schema = "";
        settings = Gio::Settings::new_full(schema, backend, "");
    } else if (!backend.empty()) {
        settings = Gio::Settings::new_with_backend(MELD_SCHEMA, backend);
#else
        settings = Gio::Settings::create(MELD_SCHEMA);
#endif
    } else {
        settings = Gio::Settings::create(MELD_SCHEMA);
    }

    interface_settings = Gio::Settings::create("org.gnome.desktop.interface");
    meldsettings = std::shared_ptr<MeldSettings>(new MeldSettings());
}

void bind_settings(Glib::ObjectBase* obj, std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__) {
    for (std::pair<Glib::ustring, Glib::ustring> binding : __gsettings_bindings__) {
        std::string settings_id = binding.first;
        std::string property_id = binding.second;
        settings->bind(settings_id, obj, property_id, Gio::SETTINGS_BIND_DEFAULT);
    }
}
