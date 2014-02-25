/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__DIRDIFF_H__
#define __MELD__DIRDIFF_H__

#include "ui/gnomeglade.h"
#include "ui/msgarea.h"
#include "melddoc.h"
#include "vc/_vc.h"
#include "filters.h"
#include "diffmap.h"
#include "tree.h"
#include "linkmap.h"

/*! Multi-pane lists with canonicalised matching and error detection */
class CanonicalListing : Glib::Object {
private:
    std::vector<std::vector<std::vector<Glib::ustring> >> items;
    std::function<std::string(std::string)> canonicalize;
    std::vector<std::tuple<int, int, int>> errors;
public:
    std::function<void(int,std::vector<Glib::ustring>)> add;

    CanonicalListing(int n, std::function<std::string(std::string)> canonicalize = nullptr);
    void add_noncanon(int pane, std::vector<Glib::ustring> item);
    void add_canon(int pane, std::vector<Glib::ustring> item);
    void get();
};


/*! Two or three way folder comparison */
class DirDiff : public MeldDoc {
private:

    std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__ = {
        {"folder-ignore-symlinks", "ignore-symlinks"},
        {"folder-shallow-comparison", "shallow-comparison"},
        {"folder-time-resolution", "time-resolution"},
        {"folder-status-filters", "status-filters"},
        {"ignore-blank-lines", "ignore-blank-lines"}
    };

#if 0
    ignore_blank_lines = GObject.property(
        type=bool,
        nick="Ignore blank lines",
        blurb="Whether to ignore blank lines when comparing file contents",
        default=False,
    )
    ignore_symlinks = GObject.property(
        type=bool,
        nick="Ignore symbolic links",
        blurb="Whether to follow symbolic links when comparing folders",
        default=False,
    )
    shallow_comparison = GObject.property(
        type=bool,
        nick="Use shallow comparison",
        blurb="Whether to compare files based solely on size and mtime",
        default=False,
    )
    status_filters = GObject.property(
        type=GObject.TYPE_STRV,
        nick="File status filters",
        blurb="Files with these statuses will be shown by the comparison.",
    )
    time_resolution = GObject.property(
        type=int,
        nick="Time resolution",
        blurb="When comparing based on mtime, the minimum difference in "
              "nanoseconds between two files before they're considered to "
              "have different mtimes.",
        default=100,
    )
#endif

    /*! Dictionary mapping tree states to corresponding difflib-like terms */
    const std::map<FileState, Glib::ustring> chunk_type_map = {
        {STATE_NORMAL, ""},
        {STATE_NOCHANGE, ""},
        {STATE_NEW, "insert"},
        {STATE_ERROR, "error"},
        {STATE_EMPTY, ""},
        {STATE_MODIFIED, "replace"},
        {STATE_MISSING, "delete"},
        {STATE_NONEXIST, "delete"}
    };

    const std::map<FileState, std::pair<Glib::ustring, Glib::ustring>> state_actions = {
        {STATE_NORMAL, std::pair<Glib::ustring, Glib::ustring>("normal", "ShowSame")},
        {STATE_NEW, std::pair<Glib::ustring, Glib::ustring>("new", "ShowNew")},
        {STATE_MODIFIED, std::pair<Glib::ustring, Glib::ustring>("modified", "ShowModified")}
    };

    Glib::ustring ui_file;
    std::vector<FilterEntry*> name_filters;
    std::vector<FilterEntry*> text_filters;
    std::vector<sigc::connection> settings_handlers;
    std::vector<int> custom_labels;
    std::vector<sigc::connection> focus_in_events;
    std::vector<sigc::connection> focus_out_events;
    std::vector<Gtk::TreeView*> treeview;
    Gtk::TreeView* treeview0;
    Gtk::TreeView* treeview1;
    Gtk::TreeView* treeview2;
    std::vector<Gtk::FileChooserButton*> fileentry;
    Gtk::FileChooserButton* fileentry0;
    Gtk::FileChooserButton* fileentry1;
    Gtk::FileChooserButton* fileentry2;
    std::vector<Gtk::ScrolledWindow*> scrolledwindow;
    Gtk::ScrolledWindow* scrolledwindow0;
    Gtk::ScrolledWindow* scrolledwindow1;
    Gtk::ScrolledWindow* scrolledwindow2;
    std::vector<DiffMap*> diffmap;
    DiffMap* diffmap0;
    DiffMap* diffmap1;
    std::vector<ScrollLinkMap*> linkmap;
    ScrollLinkMap* linkmap0;
    ScrollLinkMap* linkmap1;
    std::vector<MsgAreaController*> msgarea_mgr;
    MsgAreaController* msgarea_mgr0;
    MsgAreaController* msgarea_mgr1;
    MsgAreaController* msgarea_mgr2;
    std::vector<Gtk::VBox*> vbox;
    Gtk::VBox* vbox0;
    Gtk::VBox* vbox1;
    Gtk::VBox* vbox2;
    std::vector<Gtk::Toolbar*> dummy_toolbar_linkmap;
    Gtk::Toolbar* dummy_toolbar_linkmap0;
    Gtk::Toolbar* dummy_toolbar_linkmap1;
    std::vector<Gtk::Toolbar*> file_toolbar;
    Gtk::Toolbar* file_toolbar0;
    Gtk::Toolbar* file_toolbar1;
    Gtk::Toolbar* file_toolbar2;
    Gtk::TreeModel::Path current_path;
    Gtk::TreeModel::Path prev_path;
    Gtk::TreeModel::Path next_path;
    Gtk::TreeView* focus_pane;
    std::vector<std::pair<FileState, std::pair<Glib::ustring, Glib::ustring>>> state_filters;
    std::map<Glib::ustring, Gdk::RGBA> fill_colors;
    std::map<Glib::ustring, Gdk::RGBA> line_colors;
    std::pair<std::vector<int>, std::vector<int>> linediffs;
    std::array<std::map<std::string, Gtk::TreeViewColumn*>, 3> columns_dict;
    const Gtk::MenuShell* custom_popup;
    sigc::connection popup_deactivate_id;
    Gtk::UIManager::ui_merge_id custom_merge_id;
    Glib::RefPtr<Gtk::ActionGroup> filter_actiongroup;
    std::vector<std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Gtk::UIManagerItemType, bool>> filter_ui;
    const Gtk::ToggleAction* filter_menu_button;
    Glib::RefPtr<Gtk::UIManager> ui_manager;
    DiffTreeStore* model;

public:
    DirDiff(int num_panes, SchedulerBase& scheduler);

    Gdk::RGBA lookup(Glib::RefPtr<Gtk::StyleContext> style, const Glib::ustring& name, const Glib::ustring& _default);

    void on_style_updated();

    void queue_draw();

    void update_comparator();

    /*! Update the visibility and order of columns */
    void update_treeview_columns(int settings, Glib::ustring key);

    void on_custom_filter_menu_toggled(Gtk::ToggleAction* item);

    void _cleanup_filter_menu_button(Glib::RefPtr<Gtk::UIManager> ui);

    void _create_filter_menu_button(Glib::RefPtr<Gtk::UIManager> ui);

    virtual void on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui);

    virtual void on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui);

    void on_file_filters_changed();

    bool create_name_filters();

    void on_text_filters_changed();

    bool create_text_filters();

    void _sync_vscroll();

    void _sync_hscroll();

    int _get_focused_pane();

    void file_deleted(const char* path, int pane);

    void file_created(const char* path, int pane);

    void on_fileentry_file_set(Gtk::FileChooserButton* entry);

    void set_locations(std::vector<std::string> locations);

    virtual std::pair<std::string, std::vector<std::string>> get_comparison();

    /*! Recursively update from tree path 'path'. */
    void recursively_update(Gtk::TreePath path);

    void _search_recursively_iter(Gtk::TreePath rootpath);

    void _show_tree_wide_errors(std::vector<std::tuple<int, std::string, std::string>> invalid_filenames, std::vector<std::tuple<int, std::string, std::string, std::string>> shadowed_entries);

    Gtk::InfoBar* add_dismissable_msg(int pane, const Gtk::BuiltinStockID icon, std::string primary, std::string secondary);

    void copy_selected(int direction);

    /*! Delete all selected files/folders recursively. */
    void delete_selected();

    // TODO: Move this and path tools to new tree helper module
    void refocus_deleted_path(int model, int path);

    void on_treemodel_row_deleted(int model, int path);

    void on_treeview_selection_changed(Glib::RefPtr<Gtk::TreeView::Selection> selection, int pane);

    void on_treeview_cursor_changed();

    bool on_treeview_key_press_event(Gtk::TreeView* view, GdkEventKey* event);

    void on_treeview_row_activated(Gtk::TreeView* view, Gtk::TreeModel::Path path, int column);

    void on_treeview_row_expanded(Gtk::TreeView* view, int it, Gtk::TreeModel::Path path);

    void on_treeview_row_collapsed(Gtk::TreeView* view, int me, Gtk::TreeModel::Path path);

    void on_popup_deactivate_event();

    bool on_treeview_focus_in_event(GdkEventFocus* event, Gtk::TreeView* tree);

    bool on_treeview_focus_out_event(GdkEventFocus* event);

    void on_button_diff_clicked();

    void on_button_copy_left_clicked(int button);

    void on_button_copy_right_clicked(int button);
    void on_button_delete_clicked(int button);

    void open_external();

    void on_button_ignore_case_toggled(int button);

    void on_filter_state_toggled(int button);

    void _update_name_filter(Gtk::ToggleButton* button, int idx);

    void on_filter_hide_current_clicked(int button);

    //
    // Selection
    //
    std::vector<Gtk::TreeModel::Path> _get_selected_paths(int pane);

    //
    // Filtering
    //
    /*!
     * Get state of 'files' for filtering purposes.
     * Returns STATE_NORMAL, STATE_NEW or STATE_MODIFIED
     *
     * roots - array of root directories
     * fileslist - array of filename tuples of length len(roots)
     */
    std::vector<int> _filter_on_state(std::vector<Glib::ustring> roots, const CanonicalListing& fileslist);

    /*! Update the state of the item at 'it' */
    void _update_item_state(Gtk::TreeModel::iterator& it);

    void popup_in_pane(int pane, int event);

    bool on_treeview_popup_menu(int treeview);

    bool on_treeview_button_press_event(Gtk::TreeView* treeview, GdkEventButton* event);

    void recurse_tree_states(Gtk::TreeView* treeview, std::vector<FileState>& row_states, Gtk::TreeModel::iterator rowiter, int treeindex);

    std::vector<std::tuple<Glib::ustring, int, int>> tree_state_iter(int diffmapindex);

    std::vector<std::tuple<Glib::ustring, int, int>> get_state_traversal(int diffmapindex);

    void set_num_panes(int n);

    void refresh();

    void recompute_label();

    virtual void set_labels(std::vector<Glib::ustring> labels);

    void _update_diffmaps();

    /*!
     * When a file has changed, try to find it in our tree
     * and update its status if necessary
     */
    void on_file_changed(Glib::ustring changed_filename);

    virtual void next_diff(GdkScrollDirection direction, bool centered = false);

    virtual void on_refresh_activate();

    virtual Gtk::ResponseType on_delete_event(int appquit = 0);

    virtual void on_find_activate();
};

#endif
