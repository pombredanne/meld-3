/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
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

#ifndef __MELD__PREFERENCES_H__
#define __MELD__PREFERENCES_H__

#include "ui/listwidget.h"
#include "filters.h"

class FilterList : public ListWidget {
private:
    const Glib::ustring& key;
    FilterEntry::Type filter_type;

    Gtk::TreeViewColumn *pattern_column;
    Gtk::CellRendererPixbuf *validity_renderer;

public:

    FilterList(const Glib::ustring& key, FilterEntry::Type filter_type);

    void valid_icon_celldata(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& it);

    void on_name_edited(int ren, int path, int text);

    void on_cellrenderertoggle_toggled(int ren, int path);

    void on_pattern_edited(int ren, int path, int text);

    void _update_filter_string(const Gtk::TreeModel::Path&,const Gtk::TreeModel::iterator&,int*);
};


class ColumnList : public ListWidget {
private:

    std::map<std::string, std::string> available_columns = {
        {"size", _("Size")},
        {"modification time",  _("Modification time")},
        {"permissions", _("Permissions")}
    };

    std::string key;

public:

    ColumnList(std::string key);

    void on_cellrenderertoggle_toggled(int ren, int path);

    void _update_columns(const Gtk::TreeModel::Path&,const Gtk::TreeModel::iterator&,int*);

};


class GSettingsComboBox : public Gtk::ComboBox {
public:

    GSettingsComboBox();

    void bind_to(int key);

    void _setting_changed(int obj, int val);

    void _active_changed(int obj, int val);
};

#if 0
class GSettingsIntComboBox : public GSettingsComboBox {

    const std::string __gtype_name__ = "GSettingsIntComboBox";

    Glib::Object o1;
    Glib::Property<int> gsettings_column(o1, "gsettings-column", 0);
    Glib::Object o2;
    Glib::Property<int> gsettings_value(o2);
};


class GSettingsBoolComboBox : public GSettingsComboBox {

    const std::string __gtype_name__ = "GSettingsBoolComboBox";

    Glib::Object o1;
    Glib::Property<int> gsettings_column(o1, "gsettings-column", 0);
    Glib::Object o2;
    Glib::Property<bool> gsettings_value(o2, false);
};

class GSettingsStringComboBox : public GSettingsComboBox {

    const std::string __gtype_name__ = "GSettingsStringComboBox";

    gsettings_column = GObject.property(type=int, default=0);
    gsettings_value = GObject.property(type=str, default="");
};
#endif

class PreferencesDialog : public Component {
private:
    Gtk::CheckButton* checkbutton_default_font;
    Gtk::FontButton* fontpicker;
    Gtk::SpinButton* spinbutton_tabsize;
    Gtk::CheckButton* checkbutton_spaces_instead_of_tabs;
    Gtk::CheckButton* checkbutton_highlight_current_line;
    Gtk::CheckButton* checkbutton_show_line_numbers;
    Gtk::CheckButton* checkbutton_use_syntax_highlighting;
    Gtk::CheckButton* system_editor_checkbutton;
    Gtk::Entry* custom_edit_command_entry;
    Gtk::CheckButton* checkbutton_shallow_compare;
    Gtk::CheckButton* checkbutton_ignore_symlinks;
    Gtk::CheckButton* checkbutton_show_commit_margin;
    Gtk::SpinButton* spinbutton_commit_margin;
    Gtk::CheckButton* checkbutton_break_commit_lines;
    Gtk::CheckButton* checkbutton_ignore_blank_lines;
    Gtk::CheckButton* checkbutton_wrap_text;
    Gtk::Dialog* preferencesdialog;
    Gtk::CheckButton* checkbutton_show_whitespace;
    Gtk::CheckButton* checkbutton_wrap_word;
    Gtk::VBox *file_filters_vbox;
    Gtk::VBox *text_filters_vbox;
    Gtk::VBox *column_list_vbox;
    Gtk::Label *combo_timestamp;
    Gtk::ComboBox *combo_file_order;
#if 0
    Gio::IntComboBox
#endif
public:
    PreferencesDialog(Gtk::Window& parent);

    void on_checkbutton_wrap_text_toggled();

    void on_checkbutton_show_whitespace_toggled(Gtk::CheckButton* widget);

    void on_response(int response_id);

};

#endif
