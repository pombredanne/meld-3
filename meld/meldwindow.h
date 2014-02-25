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

#ifndef __MELD__MELDWINDOW_H__
#define __MELD__MELDWINDOW_H__

#include "ui/gnomeglade.h"
#include "dirdiff.h"
#include "filediff.h"
#include "filemerge.h"
#include "vcview.h"
#include "task.h"

class NewDiffTab;

class MeldWindow : public Component {
protected:
    Glib::RefPtr<Gtk::ActionGroup> actiongroup;
    Glib::RefPtr<Gtk::UIManager> ui;
    Gtk::Toolbar secondary_toolbar;
    Gtk::Spinner spinner;
    LifoScheduler scheduler;
    Glib::RefPtr<Gtk::ActionGroup> tab_switch_actiongroup;
    Gtk::UIManager::ui_merge_id tab_switch_merge_id;
    Gtk::ApplicationWindow* meldapp;
    Gtk::Widget* menubar;
    Gtk::Widget* toolbar;
    Gtk::Notebook* notebook;
    Gtk::VBox* appvbox;
    Gtk::Box* toolbar_holder;
    sigc::connection idle_hooked;
    sigc::connection diff_handler;
    std::vector<sigc::connection> undo_handlers;

public:

    MeldWindow();

    bool on_focus_change(GdkEventFocus* event);

    void on_widget_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& selection_data,
                                      guint info, guint time);

    int on_idle();

    void on_scheduler_runnable();

    bool on_delete_event(GdkEventAny* extra);

    bool has_pages();

    void _update_page_action_sensitivity();

    void on_switch_page(Gtk::Widget*, guint which);

    void after_switch_page(Gtk::Widget*, guint which);

    void after_page_reordered(Gtk::Widget*, guint);

    void on_notebook_label_changed(Component* component, Glib::ustring text, Glib::ustring tooltip);

    void on_can_undo(bool can);

    void on_can_redo(bool can);

    void on_next_diff_changed(bool have_prev, bool have_next);

    virtual bool on_configure_event(GdkEventConfigure* event);

    void on_menu_file_new_activate();

    void on_menu_save_activate();

    void on_menu_save_as_activate();

    void on_action_recent(/*int action*/);

    void on_menu_close_activate();

    void on_menu_undo_activate();

    void on_menu_redo_activate();

    void on_menu_refresh_activate();

    void on_menu_find_activate();

    void on_menu_find_next_activate();

    void on_menu_find_previous_activate();

    void on_menu_replace_activate();

    void on_menu_copy_activate();

    void on_menu_cut_activate();

    void on_menu_paste_activate();

    void on_action_fullscreen_toggled(Glib::RefPtr<Gtk::ToggleAction> action_fullscreen);

    void on_menu_edit_down_activate();

    void on_menu_edit_up_activate();

    void on_open_external();

    void on_toolbar_stop_clicked();

    void on_prev_tab();

    void on_next_tab();

    void on_move_tab_prev();

    void on_move_tab_next();

    void current_tab_changed_cb(const Glib::RefPtr<Gtk::RadioAction>& action/*, int current*/);

    void _update_notebook_menu(Gtk::Widget*, guint);

    Gtk::ResponseType try_remove_page(Component* page, int appquit = 0);

    void on_file_changed(Glib::ustring filename, MeldDoc* srcpage);

    void _append_page(Component* page, std::string icon);

    void diff_created_cb(Component* newdoc, Component* doc);

    NewDiffTab* append_new_comparison();

    DirDiff* append_dirdiff(std::vector<std::string> dirs, bool auto_compare = false);

    FileDiff* append_filediff(std::vector<std::string> files, std::string merge_output = "", std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta = {});

    FileMerge* append_filemerge(std::vector<std::string> files, std::string merge_output = "");

    MeldDoc* append_diff(std::vector<std::string> paths, bool auto_compare = false, bool auto_merge = false,
                         std::string merge_output = "", std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta = {});

    VcView* append_vcview(std::string location, bool auto_compare = false);

    MeldDoc* append_recent(const std::string& uri);

    void _single_file_open(std::string path);

    MeldDoc* open_paths(std::vector<std::string> paths, bool auto_compare = false, bool auto_merge = false,
                   bool focus = false);

    /*! Get the current doc or a dummy object if there is no current */
    MeldDoc* current_doc();
};


#endif
