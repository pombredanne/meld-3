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

#ifndef __MELD__VCVIEW_H__
#define __MELD__VCVIEW_H__

#include <functional>

#include "melddoc.h"
#include "tree.h"
#include "ui/gnomeglade.h"
#include "vc/_vc.h"

class ConsoleStream {
private:
    Gtk::TextView* textview;
    Glib::RefPtr<Gtk::TextBuffer::Tag> command_tag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> output_tag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> error_tag;
    Glib::RefPtr<Gtk::TextBuffer::Mark> end_mark;
public:
    ConsoleStream(Gtk::TextView* textview);

    void command(const Glib::ustring& message);

    void output(const Glib::ustring& message);

    void error(const Glib::ustring& message);

    void write(const Glib::ustring& message, Glib::RefPtr<Gtk::TextBuffer::Tag> tag);

};

class VcTreeStore : public DiffTreeStore {
public:
    VcTreeStore();
};

extern std::function<bool(Entry* x)> entry_modified;
extern std::function<bool(Entry* x)> entry_normal;
extern std::function<bool(Entry* x)> entry_nonvc;
extern std::function<bool(Entry* x)> entry_ignored;

class VcView : public MeldDoc {
private:

    std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__ = {
        {"vc-status-filters", "status-filters"},
        {"vc-left-is-local", "left-is-local"},
        {"vc-merge-file-order", "merge-file-order"}
    };

    Gtk::ActionGroup* VcviewActions;

    Gtk::TreeViewColumn* addCol(const Glib::ustring&name, int num, Data data = DATA_INVALID);

#if 0
    status_filters = GObject.property(
        type=GObject.TYPE_STRV,
        nick="File status filters",
        blurb="Files with these statuses will be shown by the comparison.",
    )
    left_is_local = GObject.property(type=bool, default=False)
    merge_file_order = GObject.property(type=str, default="local-merge-remote")
#endif

    // Map action names to VC commands and required arguments list
    std::map<std::string, std::pair<std::string, std::vector<std::string>>> action_vc_cmds_map = {
        {"VcCommit", {"commit_command", {""}}},
        {"VcUpdate", {"update_command", {}}},
#if 0
        {"VcPush", {"push", (lambda *args, **kwargs: None, )}},
#endif
        {"VcAdd", {"add_command", {}}},
        {"VcResolved", {"resolved_command", {}}},
        {"VcRemove", {"remove_command", {}}},
        {"VcRevert", {"revert_command", {}}}
    };

    // Map for inter-tab command() calls
#if 0
    std::map<std::string, std::function(void(int, int)> command_map = {
        {"resolve", Vc::resolve}
    };

    std::map<Glib::ustring, std::pair<Glib::ustring, std::function<bool(Entry* x)>> state_actions = {
        {"flatten", {"VcFlatten", nullptr}},
        {"modified", {"VcShowModified", entry_modified}},
        {"normal", {"VcShowNormal", entry_normal}},
        {"unknown", {"VcShowNonVC", entry_nonvc}},
        {"ignored", {"VcShowIgnored", entry_ignored}}
    };
#else
    std::map<Glib::ustring, Glib::ustring> state_actions = {
        {"flatten", "VcFlatten"},
        {"modified", "VcShowModified"},
        {"normal", "VcShowNormal"},
        {"unknown", "VcShowNonVC"},
        {"ignored", "VcShowIgnored"}
    };
#endif
    Gtk::TreeView* treeview;
    std::string ui_file;
    Glib::RefPtr<VcTreeStore> model;
    std::map<Data, int> column_name_map;
    ConsoleStream* consolestream;
    Gtk::ComboBox* combobox_vcs;
    std::vector<Glib::ustring> state_filters;
    Gtk::TreeModel::Path current_path;
    Gtk::TreeModel::Path prev_path;
    Gtk::TreeModel::Path next_path;
    std::vector<Gtk::FileChooserButton*> fileentry;
    std::string location;
    Gtk::TreeViewColumn* treeview_column_location;
    Gtk::TextView* consoleview;
    std::vector<Glib::ustring> valid_vc_actions;
public:

    Vc *vc;

    VcView(SchedulerBase& scheduler);

    void _set_external_action_sensitivity(bool focused);

    virtual void on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui);

    virtual void on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui);

    void update_visible_columns();

    void update_actions_sensitivity();

    void choose_vc(std::string location);

    void on_vc_change(Gtk::ComboBox* cb);

    void set_location(std::string location);

    void _set_location(std::string location);

    virtual std::pair<std::string, std::vector<std::string>> get_comparison();

    void recompute_label();

    void _search_recursively_iter(Gtk::TreeModel::iterator iterstart);

    void on_fileentry_file_set(Gtk::FileChooserButton* fileentry);

    Gtk::ResponseType on_delete_event(int appquit = 0);

    void on_row_activated(const Gtk::TreeModel::Path& path);

    void run_diff(std::string path);

    bool on_treeview_popup_menu(int treeview);

    bool on_button_press_event(Gtk::TreeView* treeview, GdkEventButton* event);

    void on_button_flatten_toggled();

    void on_filter_state_toggled();

    void on_treeview_selection_changed();

    std::vector<std::string> _get_selected_files();

    void _command_iter(int command, int files, int refresh, int *working_dir = 0);

    bool has_command(std::string command);

    void command(std::string command, std::vector<std::string> files);

    void _command(int command, int files, int refresh = 1, int *working_dir = 0);

    void _command_on_selected(int command, int refresh = 1);

    void on_button_update_clicked();

    void on_button_push_clicked();

    void on_button_commit_clicked();

    void on_button_add_clicked();

    void on_button_remove_clicked();

    void on_button_resolved_clicked();

    void on_button_revert_clicked();

    void on_button_delete_clicked();

    void on_button_diff_clicked();

    void open_external();

    void refresh();

    void refresh_partial(int where);

    void _update_item_state(Gtk::TreeModel::iterator& it, int vcentry, int location);

    void on_file_changed(std::string filename);

    void find_iter_by_name(int name);

    void on_consoleview_populate_popup(int textview, int menu);

    void on_treeview_cursor_changed(int *args);

    virtual void next_diff(GdkScrollDirection direction, bool centered = false);

    virtual void on_refresh_activate();

    virtual void on_find_activate();
};

#endif
