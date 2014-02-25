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

#include <gtkmm.h>
#include <gtksourceviewmm.h>

#include "filters.h"
#include "settings.h"
#include "ui/listwidget.h"
#include "preferences.h"
#include "conf.h"

FilterList::FilterList(const Glib::ustring& key, FilterEntry::Type filter_type) : ListWidget("EditableList.ui",
                        "list_vbox", {"EditableListStore"},
                        "EditableList"
#if 0
                        , {_("label"), false, _("pattern"), true}
#endif
                       ), key(key) {
    this->filter_type = filter_type;

#if 0
    this->pattern_column->set_cell_data_func(this->validity_renderer,
                                           sigc::mem_fun(*this, &FilterList::valid_icon_celldata));

    Glib::Variant<std::vector<std::tuple<std::string, bool, std::string>>> filter_params_;
    settings->get_value(this->key, filter_params_);
    std::vector<std::tuple<std::string, bool, std::string>> tmp = filter_params_.get();
    for (std::tuple<std::string, bool, std::string> filter_params : tmp) {
        FilterEntry* filt = FilterEntry::new_from_gsetting(filter_params, filter_type);
        if (!filt) {
            continue;
        }
        bool valid = filt->filter;
        this->model->append({filt->label, filt->active,
                           filt->filter_string, valid});
    }
#endif

#if 0
    this->model->signal_row_changed().connect(sigc::mem_fun(*this, &FilterList::_update_filter_string));
    this->model->signal_row_deleted().connect(sigc::mem_fun(*this, &FilterList::_update_filter_string));
    this->model->signal_row_inserted().connect(sigc::mem_fun(*this, &FilterList::_update_filter_string));
    this->model->signal_rows_reordered().connect(sigc::mem_fun(*this, &FilterList::_update_filter_string));
#endif

    this->_update_sensitivity();
}

void FilterList::valid_icon_celldata(Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& it) {
#if 0
    bool is_valid = model->get_value(it, 3);
#else
    bool is_valid = true;
#endif
    std::string icon_name;
    if (not is_valid) {
        icon_name = "gtk-dialog-warning";
    }
    cell->set_property("stock-id", icon_name);
}

void FilterList::on_name_edited(int ren, int path, int text) {
#if 0
    this->model[path][0] = text;
#endif
}

void FilterList::on_cellrenderertoggle_toggled(int ren, int path) {
#if 0
    this->model[path][1] = not ren.get_active();
#endif
}

void FilterList::on_pattern_edited(int ren, int path, int text) {
#if 0
    std::regex* filt = FilterEntry::compile_filter(text, this->filter_type);
    bool valid = filt;
    this->model[path][2] = text;
    this->model[path][3] = valid;
#endif
}

void FilterList::_update_filter_string(const Gtk::TreeModel::Path&,const Gtk::TreeModel::iterator&,int*) {
#if 0
    value = [(row[0], row[1], row[2]) for row in this->model];
    settings.set_value(this->key, GLib.Variant("a(sbs)", value));
#endif
}


ColumnList::ColumnList(std::string key) : ListWidget("EditableList.ui",
                        "columns_ta", {"ColumnsListStore"},
                        "columns_treeview") {
    this->key = key;

    // Unwrap the variant
#if 0
    prefs_columns = [(k, v) for k, v in settings->get_value(this->key)]
    std::vector<int> column_vis;
    std::vector<int> column_order;
    for (sort_key, (column_name, visibility) in enumerate(prefs_columns)) {
        column_vis[column_name] = bool(int(visibility));
        column_order[column_name] = sort_key;
    }

    columns = [(column_vis.get(name, True), name, label) for
                name, label in self.available_columns.items()];
    columns = sorted(columns, key=lambda c: column_order.get(c[1], 0));

    for (visibility, name, label in columns) {
        self.model.append([visibility, name, label]);
    }
#endif

#if 0
    this->model->signal_row_changed().connect(sigc::mem_fun(*this, &ColumnList::_update_columns));
    this->model->signal_row_deleted().connect(sigc::mem_fun(*this, &ColumnList::_update_columns));
    this->model->signal_row_inserted().connect(sigc::mem_fun(*this, &ColumnList::_update_columns));
    this->model->signal_rows_reordered().connect(sigc::mem_fun(*this, &ColumnList::_update_columns));
#endif

    this->_update_sensitivity();
}

void ColumnList::on_cellrenderertoggle_toggled(int ren, int path) {
#if 0
    this->model[path][0] = not ren.get_active();
#endif
}

void ColumnList::_update_columns(const Gtk::TreeModel::Path&,const Gtk::TreeModel::iterator&,int*) {
#if 0
    value = [(c[1].lower(), c[0]) for c in this->model];
    settings.set_value(this->key, Glib::Variant("a(sb)", value));
#endif
}


GSettingsComboBox::GSettingsComboBox() : Gtk::ComboBox() {
#if 0
    this->connect("notify::gsettings-value", this->_setting_changed);
    this->connect("notify::active", this->_active_changed);
#endif
}

void GSettingsComboBox::bind_to(int key) {
#if 0
    settings.bind(
        key, this, "gsettings-value", Gio.SettingsBindFlags.DEFAULT);
#endif
}

void GSettingsComboBox::_setting_changed(int obj, int val) {
#if 0
    const Gtk::TreeModelColumn<int> column;
    this->get_property("gsettings-column", column);
    std::string value;
    this->get_property("gsettings-value", value);

    int idx = 0;
    for (const Gtk::TreeRow row : this->get_model()->children()) {
        if (value == row[column]) {
            idx = row.path[0];
            break;
        }
    }

    int active;
    this->get_property("active", active);
    if (active != idx) {
        this->set_property("active", idx);
    }
#endif
}

void GSettingsComboBox::_active_changed(int obj, int val) {
    int column;
    this->get_property("gsettings-column", column);
#if 0
    int value = this->get_model()[this->get_active_iter()][column];
    this->set_property("gsettings-value", value);
#endif
}


PreferencesDialog::PreferencesDialog(Gtk::Window& parent) : Component("preferences.ui", "preferencesdialog",
                       {"adjustment1", "adjustment2", "fileorderstore",
                        "sizegroup_editor", "timestampstore",
                        "mergeorderstore", "sizegroup_file_order_labels",
                        "sizegroup_file_order_combos"}) {

    // Hook up signals
    this->preferencesdialog->signal_response().connect(sigc::mem_fun(this, &PreferencesDialog::on_response));
    this->checkbutton_wrap_text->signal_toggled().connect(sigc::mem_fun(this, &PreferencesDialog::on_checkbutton_wrap_text_toggled));
    this->checkbutton_wrap_word->signal_toggled().connect(sigc::mem_fun(this, &PreferencesDialog::on_checkbutton_wrap_text_toggled));
    this->checkbutton_show_whitespace->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &PreferencesDialog::on_checkbutton_show_whitespace_toggled), this->checkbutton_show_whitespace));

    static_cast<Gtk::Dialog*>(this->widget)->set_transient_for(parent);

    settings->bind("use-system-font", this->checkbutton_default_font, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("custom-font", this->fontpicker, "font", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("indent-width", this->spinbutton_tabsize, "value", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("insert-spaces-instead-of-tabs", this->checkbutton_spaces_instead_of_tabs, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("highlight-current-line", this->checkbutton_highlight_current_line, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("show-line-numbers", this->checkbutton_show_line_numbers, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("highlight-syntax", this->checkbutton_use_syntax_highlighting, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("use-system-editor", this->system_editor_checkbutton, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("custom-editor-command", this->custom_edit_command_entry, "text", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("folder-shallow-comparison", this->checkbutton_shallow_compare, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("folder-ignore-symlinks", this->checkbutton_ignore_symlinks, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("vc-show-commit-margin", this->checkbutton_show_commit_margin, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("vc-commit-margin", this->spinbutton_commit_margin, "value", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("vc-break-commit-message", this->checkbutton_break_commit_lines, "active", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("ignore-blank-lines", this->checkbutton_ignore_blank_lines, "active", Gio::SETTINGS_BIND_DEFAULT);
    // Sensitivity bindings must come after value bindings, or the key
    // writability in gsettings overrides manual sensitivity setting.
    settings->bind("vc-show-commit-margin", this->spinbutton_commit_margin, "sensitive", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("vc-show-commit-margin", this->checkbutton_break_commit_lines, "sensitive", Gio::SETTINGS_BIND_DEFAULT);

    settings->bind(
        "use-system-editor", this->custom_edit_command_entry, "sensitive",
        Gio::SETTINGS_BIND_DEFAULT | Gio::SETTINGS_BIND_INVERT_BOOLEAN);
    settings->bind(
        "use-system-font", this->fontpicker, "sensitive",
        Gio::SETTINGS_BIND_DEFAULT | Gio::SETTINGS_BIND_INVERT_BOOLEAN);

#if 0
    this->checkbutton_wrap_text->bind_property(
        "active", this->checkbutton_wrap_word, "sensitive",
        Gio::SETTINGS_BIND_DEFAULT);
#endif

    // TODO: Fix once bind_with_mapping is available
    this->checkbutton_show_whitespace->set_active(
        bool(settings->get_flags("draw-spaces")));

    int wrap_mode = settings->get_enum("wrap-mode");
    this->checkbutton_wrap_text->set_active(wrap_mode != Gtk::WRAP_NONE);
    this->checkbutton_wrap_word->set_active(wrap_mode == Gtk::WRAP_WORD);

    FilterList filefilter("filename-filters", FilterEntry::SHELL);
    this->file_filters_vbox->pack_start(*filefilter.widget, true, true, 0);

    FilterList textfilter("text-filters", FilterEntry::REGEX);
    this->text_filters_vbox->pack_start(*textfilter.widget, true, true, 0);

    ColumnList columnlist("folder-columns");
    this->column_list_vbox->pack_start(*columnlist.widget, true, true, 0);

#if 0
    this->combo_timestamp->bind_to("folder-time-resolution");
    this->combo_file_order->bind_to("vc-left-is-local");
    this->combo_merge_order->bind_to("vc-merge-file-order");
#endif

    this->widget->show();
}

void PreferencesDialog::on_checkbutton_wrap_text_toggled() {
    Gtk::WrapMode wrap_mode;
    if (not this->checkbutton_wrap_text->get_active()) {
        wrap_mode = Gtk::WRAP_NONE;
    } else if (this->checkbutton_wrap_word->get_active()) {
        wrap_mode = Gtk::WRAP_WORD;
    } else {
        wrap_mode = Gtk::WRAP_CHAR;
    }
#if 0
    settings->set_enum("wrap-mode", wrap_mode);
#endif
}

void PreferencesDialog::on_checkbutton_show_whitespace_toggled(Gtk::CheckButton *widget) {
    int value;
    if (widget->get_active()) {
        value = Gsv::DRAW_SPACES_ALL;
    } else {
        value = 0;
    }
#if 0
    settings->set_flags("draw-spaces", value);
#endif
}

void PreferencesDialog::on_response(int response_id) {
    delete this->widget;
}

