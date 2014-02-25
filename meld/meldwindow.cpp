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

#include <gtkmm.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "filemerge.h"
#include "melddoc.h"
#include "recent.h"
#include "task.h"
#include "ui/gnomeglade.h"
#include "ui/notebooklabel.h"
#include "conf.h"
#include "util/compat.h"
#include "recent.h"
#include "settings.h"
#include "meldwindow.h"
#include "newdifftab.h"
#include "meldapp.h"
#include "tab.h"

class DummyDoc {
    void __getattr__(int a) {
#if 0
        return lambda *x: None;
#endif
    }
};

static void app_action(std::string name) {
    app.lookup_action(name)->activate();
}

MeldWindow::MeldWindow() : Component("meldapp.ui", "meldapp") {

    // Initialize member variables
    this->builder->get_widget("meldapp", this->meldapp);
    this->builder->get_widget("notebook", this->notebook);
    this->builder->get_widget("appvbox", this->appvbox);
    this->builder->get_widget("toolbar_holder", this->toolbar_holder);

    // Hook up signals
    this->meldapp->signal_delete_event().connect(sigc::mem_fun(this, &MeldWindow::on_delete_event));
    this->meldapp->signal_configure_event().connect(sigc::mem_fun(this, &MeldWindow::on_configure_event));
    this->notebook->signal_page_reordered().connect(sigc::mem_fun(this, &MeldWindow::_update_notebook_menu));
    this->notebook->signal_page_reordered().connect(sigc::mem_fun(this, &MeldWindow::after_page_reordered), true);
    this->notebook->signal_page_added().connect(sigc::mem_fun(this, &MeldWindow::_update_notebook_menu));
    this->notebook->signal_switch_page().connect(sigc::mem_fun(this, &MeldWindow::on_switch_page));
    this->notebook->signal_switch_page().connect(sigc::mem_fun(this, &MeldWindow::after_switch_page), true);
    this->notebook->signal_page_removed().connect(sigc::mem_fun(this, &MeldWindow::_update_notebook_menu));


    this->widget->set_name("meldapp");

    std::string ui_file = gnomeglade_ui_file("meldapp-ui.xml");
    this->actiongroup = Gtk::ActionGroup::create("MainActions");
#if 0
    this->actiongroup->set_translation_domain("meld");
#endif

    this->actiongroup->add(Gtk::Action::create("FileMenu", _("_File")));
    Glib::RefPtr<Gtk::Action> action_new = Gtk::Action::create("New", Gtk::Stock::NEW, _("_New Comparison..."), _("Start a new comparison"));
    action_new->set_accel_path("<control>N");
    this->actiongroup->add(action_new, sigc::mem_fun(this, &MeldWindow::on_menu_file_new_activate));
    this->actiongroup->add(Gtk::Action::create("Save", Gtk::Stock::SAVE, "", _("Save the current file")), sigc::mem_fun(this, &MeldWindow::on_menu_save_activate));
    Glib::RefPtr<Gtk::Action> action_save_as = Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS, _("Save As..."), _("Save the current file with a different name"));
    action_save_as->set_accel_path("<control><shift>S");
    this->actiongroup->add(action_save_as, sigc::mem_fun(this, &MeldWindow::on_menu_save_as_activate));
    this->actiongroup->add(Gtk::Action::create("Close", Gtk::Stock::CLOSE, "", _("Close the current file")), sigc::mem_fun(this, &MeldWindow::on_menu_close_activate));

    this->actiongroup->add(Gtk::Action::create("EditMenu", _("_Edit")));
    Glib::RefPtr<Gtk::Action> action_undo = Gtk::Action::create("Undo", Gtk::Stock::UNDO, "", _("Undo the last action"));
    action_undo->set_accel_path("<control>Z");
    this->actiongroup->add(action_undo, sigc::mem_fun(this, &MeldWindow::on_menu_undo_activate));
    Glib::RefPtr<Gtk::Action> action_redo = Gtk::Action::create("Redo", Gtk::Stock::REDO, "", _("Redo the last undone action"));
    action_redo->set_accel_path("<control><shift>Z");
    this->actiongroup->add(action_redo, sigc::mem_fun(this, &MeldWindow::on_menu_redo_activate));
    this->actiongroup->add(Gtk::Action::create("Cut", Gtk::Stock::CUT, "", _("Cut the selection")), sigc::mem_fun(this, &MeldWindow::on_menu_cut_activate));
    this->actiongroup->add(Gtk::Action::create("Copy", Gtk::Stock::COPY, "", _("Copy the selection")), sigc::mem_fun(this, &MeldWindow::on_menu_copy_activate));
    this->actiongroup->add(Gtk::Action::create("Paste", Gtk::Stock::PASTE, "", _("Paste the clipboard")), sigc::mem_fun(this, &MeldWindow::on_menu_paste_activate));
    this->actiongroup->add(Gtk::Action::create("Find", Gtk::Stock::FIND, _("Find..."), _("Search for text")), sigc::mem_fun(this, &MeldWindow::on_menu_find_activate));
    Glib::RefPtr<Gtk::Action> action_find_next = Gtk::Action::create("FindNext", _("Find Ne_xt"), _("Search forwards for the same text"));
    action_find_next->set_accel_path("<control>G");
    this->actiongroup->add(action_find_next, sigc::mem_fun(this, &MeldWindow::on_menu_find_next_activate));
    Glib::RefPtr<Gtk::Action> action_find_previous = Gtk::Action::create("FindPrevious", _("Find _Previous"), _("Search backwards for the same text"));
    action_find_previous->set_accel_path("<control><shift>G");
    this->actiongroup->add(action_find_previous, sigc::mem_fun(this, &MeldWindow::on_menu_find_previous_activate));
    Glib::RefPtr<Gtk::Action> action_replace = Gtk::Action::create("Replace", Gtk::Stock::FIND_AND_REPLACE, _("_Replace..."), _("Find and replace text"));
    action_replace->set_accel_path("<control>H");
    this->actiongroup->add(action_replace, sigc::mem_fun(this, &MeldWindow::on_menu_replace_activate));

    this->actiongroup->add(Gtk::Action::create("ChangesMenu", _("_Changes")));
    Glib::RefPtr<Gtk::Action> action_next_change = Gtk::Action::create("NextChange", Gtk::Stock::GO_DOWN, _("Next Change"), _("Go to the next change"));
    action_next_change->set_accel_path("<Alt>Down");
    this->actiongroup->add(action_next_change, sigc::mem_fun(this, &MeldWindow::on_menu_edit_down_activate));
    Glib::RefPtr<Gtk::Action> action_prev_change = Gtk::Action::create("PrevChange", Gtk::Stock::GO_UP, _("Previous Change"), _("Go to the previous change"));
    action_prev_change->set_accel_path("<Alt>Up");
    this->actiongroup->add(action_prev_change, sigc::mem_fun(this, &MeldWindow::on_menu_edit_up_activate));
    this->actiongroup->add(Gtk::Action::create("OpenExternal", _("Open Externally"), _("Open selected file or directory in the default external " "application")), sigc::mem_fun(this, &MeldWindow::on_open_external));

    this->actiongroup->add(Gtk::Action::create("ViewMenu",  _("_View")));
    this->actiongroup->add(Gtk::Action::create("FileStatus",  _("File Status")));
    this->actiongroup->add(Gtk::Action::create("VcStatus",  _("Version Status")));
    this->actiongroup->add(Gtk::Action::create("FileFilters",  _("File Filters")));
    Glib::RefPtr<Gtk::Action> action_stop = Gtk::Action::create("Stop", Gtk::Stock::STOP, "", _("Stop the current action"));
    action_stop->set_accel_path("Escape");
    this->actiongroup->add(action_stop, sigc::mem_fun(this, &MeldWindow::on_toolbar_stop_clicked));
    Glib::RefPtr<Gtk::Action> action_refresh = Gtk::Action::create("Refresh", Gtk::Stock::REFRESH, "", _("Refresh the view"));
    action_refresh->set_accel_path("<control>R");
    this->actiongroup->add(action_refresh, sigc::mem_fun(this, &MeldWindow::on_menu_refresh_activate));

    this->actiongroup->add(Gtk::Action::create("TabMenu", _("_Tabs")));
    Glib::RefPtr<Gtk::Action> action_prev_tab = Gtk::Action::create("PrevTab",  _("_Previous Tab"), _("Activate previous tab"));
    action_prev_tab->set_accel_path("<Ctrl><Alt>Page_Up");
    this->actiongroup->add(action_prev_tab, sigc::mem_fun(this, &MeldWindow::on_prev_tab));
    Glib::RefPtr<Gtk::Action> action_next_tab = Gtk::Action::create("NextTab", _("_Next Tab"), _("Activate next tab"));
    action_next_tab->set_accel_path("<Ctrl><Alt>Page_Down");
    this->actiongroup->add(action_next_tab, sigc::mem_fun(this, &MeldWindow::on_next_tab));
    Glib::RefPtr<Gtk::Action> action_move_tab_prev = Gtk::Action::create("MoveTabPrev", _("Move Tab _Left"), _("Move current tab to left"));
    action_move_tab_prev->set_accel_path("<Ctrl><Alt><Shift>Page_Up");
    this->actiongroup->add(action_move_tab_prev, sigc::mem_fun(this, &MeldWindow::on_move_tab_prev));
    Glib::RefPtr<Gtk::Action> action_move_tab_next = Gtk::Action::create("MoveTabNext", _("Move Tab _Right"), _("Move current tab to right"));
    action_move_tab_next->set_accel_path("<Ctrl><Alt><Shift>Page_Down");
    this->actiongroup->add(action_move_tab_next, sigc::mem_fun(this, &MeldWindow::on_move_tab_next));

    Glib::RefPtr<Gtk::ToggleAction> action_fullscreen = Gtk::ToggleAction::create("Fullscreen", _("Fullscreen"), _("View the comparison in fullscreen"), false);
    action_fullscreen->set_accel_path("F11");
    this->actiongroup->add(action_fullscreen, sigc::bind(sigc::mem_fun(this, &MeldWindow::on_action_fullscreen_toggled), action_fullscreen));
    this->actiongroup->add(Gtk::ToggleAction::create("ToolbarVisible", _("_Toolbar"), _("Show or hide the toolbar"), true));

    Glib::RefPtr<Gtk::RecentAction> recent_action = Gtk::RecentAction::create("Recent",  _("Open Recent"),
                                     _("Open recent files"));
    recent_action->set_show_private(true);
    recent_action->set_filter(recent_comparisons->recent_filter);
    recent_action->set_sort_type(Gtk::RECENT_SORT_MRU);
    recent_action->signal_item_activated().connect(sigc::mem_fun(*this, &MeldWindow::on_action_recent));
    this->actiongroup->add(recent_action);

    this->ui = Gtk::UIManager::create();
    this->ui->insert_action_group(this->actiongroup, 0);
    this->ui->add_ui_from_file(ui_file);

    // Manually handle shells that don't show an application menu
    Glib::RefPtr<Gtk::Settings> gtk_settings = Gtk::Settings::get_default();
    Glib::Value<bool> value;
    value.init(Glib::Value<bool>::value_type());
    gtk_settings->get_property_value("gtk_shell_shows_app_menu", value);
    if (not value.get()) {
        Glib::RefPtr<Gtk::ActionGroup> app_actiongroup = Gtk::ActionGroup::create("AppActions");
#if 0
        app_actiongroup->set_translation_domain("meld");
#endif
        app_actiongroup->add(Gtk::Action::create("AppMenu", "", _("_Meld")));
        app_actiongroup->add(Gtk::Action::create("Quit", Gtk::Stock::QUIT, "", _("Quit the program")), sigc::bind(sigc::ptr_fun(app_action), "quit"));
        app_actiongroup->add(Gtk::Action::create("Preferences", Gtk::Stock::PREFERENCES, _("Prefere_nces"), _("Configure the application")), sigc::bind(sigc::ptr_fun(app_action), "preferences"));
        Glib::RefPtr<Gtk::Action> action_help = Gtk::Action::create("Help", Gtk::Stock::HELP, _("_Contents"), _("Open the Meld manual"));
        action_new->set_accel_path("F1");
        app_actiongroup->add(action_help, sigc::bind(sigc::ptr_fun(app_action), "help"));
        app_actiongroup->add(Gtk::Action::create("About", Gtk::Stock::ABOUT, "", _("About this application")), sigc::bind(sigc::ptr_fun(app_action), "about"));
        this->ui->insert_action_group(app_actiongroup, 0);

        ui_file = gnomeglade_ui_file("appmenu-fallback.xml");
        this->ui->add_ui_from_file(ui_file);
        static_cast<Gtk::ApplicationWindow*>(this->widget)->set_show_menubar(false);
    }

    this->tab_switch_actiongroup.clear();
    this->tab_switch_merge_id = 0;



    this->actiongroup->get_action("Save")->property_is_important() = true;
    this->actiongroup->get_action("Undo")->property_is_important() = true;
    static_cast<Gtk::ApplicationWindow*>(this->widget)->add_accel_group(this->ui->get_accel_group());
    this->menubar = this->ui->get_widget("/Menubar");
    this->toolbar = this->ui->get_widget("/Toolbar");
    this->toolbar->get_style_context()->add_class(
        GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    settings->bind("toolbar-visible",
                  (this->actiongroup->get_action("ToolbarVisible")).operator->(), "active",
                  Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("toolbar-visible", this->toolbar, "visible",
                  Gio::SETTINGS_BIND_DEFAULT);
    interface_settings->bind("toolbar-style", this->toolbar, "toolbar-style",
                  Gio::SETTINGS_BIND_DEFAULT);

    // Add alternate keybindings for Prev/Next Change
    Glib::RefPtr<const Gtk::AccelGroup> accels = this->ui->get_accel_group();
    guint keyval;
    Gdk::ModifierType mask;
    Gtk::AccelGroup::parse("<Ctrl>D", keyval, mask);
#if 0
    accels->connect(keyval, mask, 0, this->on_menu_edit_down_activate);
#endif
    Gtk::AccelGroup::parse("<Ctrl>E", keyval, mask);
#if 0
    accels->connect(keyval, mask, 0, this->on_menu_edit_up_activate);
#endif
    Gtk::AccelGroup::parse("F5", keyval, mask);
#if 0
    accels->connect(keyval, mask, 0, this->on_menu_refresh_activate);
#endif

    // Initialise sensitivity for important actions
    this->actiongroup->get_action("Stop")->set_sensitive(false);
    this->_update_page_action_sensitivity();

    this->appvbox->pack_start(*this->menubar, false, true, 0);
    this->toolbar_holder->pack_start(*this->toolbar, true, true, 0);

    // Double toolbars to work around UIManager integration issues
    this->secondary_toolbar.get_style_context()->add_class(
        GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    this->toolbar_holder->pack_end(this->secondary_toolbar, false, true, 0);

    Gtk::ToolItem toolbutton;
    toolbutton.add(this->spinner);
    this->secondary_toolbar.insert(toolbutton, -1);
    // Set a minimum size because the spinner requests nothing
    this->secondary_toolbar.set_size_request(30, -1);
    this->secondary_toolbar.show_all();

    this->widget->drag_dest_set(
        Gtk::DEST_DEFAULT_MOTION | Gtk::DEST_DEFAULT_HIGHLIGHT |
        Gtk::DEST_DEFAULT_DROP,
        Gdk::ACTION_COPY);
    this->widget->drag_dest_add_uri_targets();
    this->widget->signal_drag_data_received().connect(sigc::mem_fun(*this,
                        &MeldWindow::on_widget_drag_data_received));
    this->idle_hooked.disconnect();
    this->scheduler.signal_runnable().connect(sigc::mem_fun(this, &MeldWindow::on_scheduler_runnable));
    Glib::Variant<std::vector<int>> window_size;
    settings->get_value("window-size", window_size);
    static_cast<Gtk::ApplicationWindow*>(this->widget)->set_default_size(window_size.get()[0], window_size.get()[1]);
    Glib::ustring window_state = settings->get_string("window-state");
    if (window_state == "maximized") {
        static_cast<Gtk::ApplicationWindow*>(this->widget)->maximize();
    }
    this->ui->ensure_update();
    this->diff_handler.disconnect();
    this->widget->signal_focus_in_event().connect(sigc::mem_fun(this, &MeldWindow::on_focus_change));
    this->widget->signal_focus_out_event().connect(sigc::mem_fun(this, &MeldWindow::on_focus_change));
}

bool MeldWindow::on_focus_change(GdkEventFocus* event) {
    for (int idx = 0; idx < this->notebook->get_n_pages(); idx++) {
        Gtk::Widget* w = this->notebook->get_nth_page(idx);
        Component* c = (Component *) pyobject[w];
        if (typeid(c) == typeid(FileDiff*)) {
            static_cast<FileDiff*>(c)->on_focus_change();
        }
    }
    // Let the rest of the stack know about this event
    return false;
}

void MeldWindow::on_widget_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& selection_data,
                                  guint info, guint time) {
    if (!selection_data.get_uris().empty()) {
        std::vector<std::string> paths;
        for (Glib::ustring uri : selection_data.get_uris()) {
            paths.push_back(Gio::File::create_for_uri(uri)->get_path());
        }
        this->open_paths(paths);
    }
}

int MeldWindow::on_idle() {
    int ret = this->scheduler.iteration();
#if 0
    if (ret and isinstance(ret, string_types)) {
        this->spinner.set_tooltip_text(ret);
    }
#endif

    bool pending = this->scheduler.tasks_pending();
    if (not pending) {
        this->spinner.stop();
        this->spinner.hide();
        this->spinner.set_tooltip_text("");
        this->idle_hooked.disconnect();
        this->actiongroup->get_action("Stop")->set_sensitive(false);
    }
    return pending;
}

void MeldWindow::on_scheduler_runnable() {
    if (not this->idle_hooked.connected()) {
        this->spinner.show();
        this->spinner.start();
        this->actiongroup->get_action("Stop")->set_sensitive(true);
        this->idle_hooked = Glib::signal_idle().connect(sigc::mem_fun(this, &MeldWindow::on_idle));
    }
}

bool MeldWindow::on_delete_event(GdkEventAny* extra) {
    // Delete pages from right-to-left.  This ensures that if a version
    // control page is open in the far left page, it will be closed last.
    std::vector<Gtk::Widget*> children = this->notebook->get_children();
    std::reverse(children.begin(), children.end());
    for (Gtk::Widget* c : children) {
        Component* page = (Component*) pyobject[c];
        this->notebook->set_current_page(this->notebook->page_num(*(page->widget)));
        Gtk::ResponseType response = this->try_remove_page(page, 1);
        if (response == Gtk::RESPONSE_CANCEL) {
            return true;
        }
    }
    return false;
}

bool MeldWindow::has_pages() {
    return this->notebook->get_n_pages() > 0;
}

void MeldWindow::_update_page_action_sensitivity() {
    int current_page = this->notebook->get_current_page();
    bool have_prev_tab = current_page > 0;
    bool have_next_tab = current_page < this->notebook->get_n_pages() - 1;
    this->actiongroup->get_action("PrevTab")->set_sensitive(have_prev_tab);
    this->actiongroup->get_action("NextTab")->set_sensitive(have_next_tab);
    this->actiongroup->get_action("MoveTabPrev")->set_sensitive(have_prev_tab);
    this->actiongroup->get_action("MoveTabNext")->set_sensitive(have_next_tab);

    Component* page;
    if (current_page != -1) {
        page = pyobject[this->notebook->get_nth_page(current_page)];
    } else {
        page = nullptr;
    }

    this->actiongroup->get_action("Close")->set_sensitive(bool(page));
    MeldDoc* melddoc = dynamic_cast<MeldDoc*>(page);
    if (not melddoc) {
        for (std::string action : {"PrevChange", "NextChange", "Cut", "Copy", "Paste",
                       "Find", "FindNext", "FindPrevious", "Replace",
                       "Refresh"}) {
            this->actiongroup->get_action(action)->set_sensitive(false);
        }
    } else {
        for (std::string action : {"Find", "Refresh"}) {
            this->actiongroup->get_action(action)->set_sensitive(true);
        }
        bool is_filediff = typeid(page) == typeid(FileDiff*);
        for (std::string action : {"Cut", "Copy", "Paste", "FindNext", "FindPrevious",
                       "Replace"}) {
            this->actiongroup->get_action(action)->set_sensitive(is_filediff);
        }
    }
}

void MeldWindow::on_switch_page(Gtk::Widget*, guint which) {
    int oldidx = this->notebook->get_current_page();
    if (oldidx >= 0) {
        Component* olddoc = (Component *) pyobject[this->notebook->get_nth_page(oldidx)];
        if (this->diff_handler) {
            this->diff_handler.disconnect();
        }
        Tab* oldmelddoc = dynamic_cast<Tab*>(olddoc);
        if (oldmelddoc) {
            oldmelddoc->on_container_switch_out_event(this->ui);
        }
        if (!this->undo_handlers.empty()) {
            if (typeid(olddoc) == typeid(FileDiff*)) {
                for (sigc::connection handler : this->undo_handlers) {
                    handler.disconnect();
                }
            }
            this->undo_handlers.clear();
        }
    }

    Tab* newdoc = (Tab*) pyobject[this->notebook->get_nth_page(which)];
    bool can_undo;
    bool can_redo;
    if (typeid(newdoc) == typeid(FileDiff*)) {
        UndoSequence* undoseq = static_cast<FileDiff*>(newdoc)->undosequence;
        can_undo = undoseq->can_undo();
        can_redo = undoseq->can_redo();
        sigc::connection undo_handler = undoseq->signal_can_undo().connect(sigc::mem_fun(this, &MeldWindow::on_can_undo));
        sigc::connection redo_handler = undoseq->signal_can_redo().connect(sigc::mem_fun(this, &MeldWindow::on_can_redo));
        this->undo_handlers = {undo_handler, redo_handler};
    } else {
        can_undo = false;
        can_redo = false;
    }
    this->actiongroup->get_action("Undo")->set_sensitive(can_undo);
    this->actiongroup->get_action("Redo")->set_sensitive(can_redo);

    // FileDiff handles save sensitivity; it makes no sense for other modes
    if (typeid(newdoc) != typeid(FileDiff*)) {
        this->actiongroup->get_action("Save")->set_sensitive(false);
        this->actiongroup->get_action("SaveAs")->set_sensitive(false);
    } else {
        this->actiongroup->get_action("SaveAs")->set_sensitive(true);
    }

    NotebookLabel* nbl = static_cast<NotebookLabel*>(this->notebook->get_tab_label(*(newdoc->widget)));
    static_cast<Gtk::ApplicationWindow*>(this->widget)->set_title(nbl->get_label_text() + " - Meld");
    newdoc->on_container_switch_in_event(this->ui);
    MeldDoc* d = dynamic_cast<MeldDoc*>(newdoc);
    if (d) {
        this->diff_handler = d->signal_next_diff_changed().connect(sigc::mem_fun(this, &MeldWindow::on_next_diff_changed));
    }
}

void MeldWindow::after_switch_page(Gtk::Widget*, guint which) {
    this->_update_page_action_sensitivity();
    Glib::RefPtr<Gtk::ActionGroup> actiongroup = this->tab_switch_actiongroup;
    if (actiongroup) {
        boost::format fmt("SwitchTab%d");
        fmt % which;
        std::string action_name = fmt.str();

        Glib::RefPtr<Gtk::Action> tmp = actiongroup->get_action(action_name);
        Glib::RefPtr<Gtk::ToggleAction> tmp2 = Glib::RefPtr<Gtk::ToggleAction>::cast_static(tmp);

        tmp2->set_active(true);
    }
}

void MeldWindow::after_page_reordered(Gtk::Widget*, guint) {
    this->_update_page_action_sensitivity();
}

void MeldWindow::on_notebook_label_changed(Component* component, Glib::ustring text, Glib::ustring tooltip) {
    Gtk::Widget* page = component->widget;
    NotebookLabel* nbl = static_cast<NotebookLabel*>(this->notebook->get_tab_label(*page));
    nbl->set_label_text(text);
    nbl->set_tooltip_text(tooltip);

    // Only update the window title if the current page is active
    if (this->notebook->get_current_page() == this->notebook->page_num(*page)) {
        static_cast<Gtk::ApplicationWindow*>(this->widget)->set_title(text + " - Meld");
    }

    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    value.set(text);
    page->set_property_value("menu-label", value);

    Glib::RefPtr<Gtk::ActionGroup> actiongroup = this->tab_switch_actiongroup;
    if (actiongroup) {
        int idx = this->notebook->page_num(*page);
        boost::format fmt("SwitchTab%d");
        fmt % idx;
        std::string action_name = fmt.str();
        actiongroup->get_action(action_name)->set_label(text);
    }
}

void MeldWindow::on_can_undo(bool can) {
    this->actiongroup->get_action("Undo")->set_sensitive(can);
}

void MeldWindow::on_can_redo(bool can) {
    this->actiongroup->get_action("Redo")->set_sensitive(can);
}

void MeldWindow::on_next_diff_changed(bool have_prev, bool have_next) {
    this->actiongroup->get_action("PrevChange")->set_sensitive(have_prev);
    this->actiongroup->get_action("NextChange")->set_sensitive(have_next);
}

bool MeldWindow::on_configure_event(GdkEventConfigure* event) {
    GdkWindowState state = gdk_window_get_state(event->window);
    int nosave = GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_MAXIMIZED;
    if (not (state & nosave)) {
        Glib::Variant<std::pair<int, int>> variant = Glib::Variant<std::pair<int, int>>::create(std::pair<int, int>(event->width, event->height));
        settings->set_value("window-size", variant);
    }
    int maximised = state & GDK_WINDOW_STATE_MAXIMIZED;
    Glib::ustring window_state;
    if (maximised) {
        window_state = "maximized";
    } else {
        window_state = "normal";
    }
    settings->set_string("window-state", window_state);
    return false;
}

void MeldWindow::on_menu_file_new_activate() {
    this->append_new_comparison();
}

void MeldWindow::on_menu_save_activate() {
    this->current_doc()->save();
}

void MeldWindow::on_menu_save_as_activate() {
    this->current_doc()->save_as();
}

void MeldWindow::on_action_recent(/*int action*/) {
#if 0
    uri = action.get_current_uri();
    if (not uri) {
        return;
    }
    try {
        this->append_recent(uri);
    } catch (IOError &e) {
        // FIXME: Need error handling, but no sensible display location
    } catch (ValueError &e) {
        // FIXME: Need error handling, but no sensible display location
    }
#endif
}

void MeldWindow::on_menu_close_activate() {
    int i = this->notebook->get_current_page();
    if (i >= 0) {
        Component* page = (Component*) pyobject[this->notebook->get_nth_page(i)];
        this->try_remove_page(page);
    }
}

void MeldWindow::on_menu_undo_activate() {
    MeldDoc* doc = this->current_doc();
    if (typeid(doc) == typeid(FileDiff*)) {
        static_cast<FileDiff*>(doc)->on_undo_activate();
    }
}

void MeldWindow::on_menu_redo_activate() {
    MeldDoc* doc = this->current_doc();
    if (typeid(doc) == typeid(FileDiff*)) {
        static_cast<FileDiff*>(doc)->on_redo_activate();
    }
}

void MeldWindow::on_menu_refresh_activate() {
    this->current_doc()->on_refresh_activate();
}

void MeldWindow::on_menu_find_activate() {
    this->current_doc()->on_find_activate();
}

void MeldWindow::on_menu_find_next_activate() {
    this->current_doc()->on_find_next_activate();
}

void MeldWindow::on_menu_find_previous_activate() {
    this->current_doc()->on_find_previous_activate();
}

void MeldWindow::on_menu_replace_activate() {
    this->current_doc()->on_replace_activate();
}

void MeldWindow::on_menu_copy_activate() {
    Gtk::Widget* widget = static_cast<Gtk::ApplicationWindow*>(this->widget)->get_focus();
    Gtk::Editable* editable = dynamic_cast<Gtk::Editable*>(widget);
    if (editable) {
        editable->copy_clipboard();
    } else {
        Gtk::TextView* textview = dynamic_cast<Gtk::TextView*>(widget);
        if (textview) {
#if 0
        widget.emit("copy-clipboard");
#endif
        }
    }
}

void MeldWindow::on_menu_cut_activate() {
    Gtk::Widget* widget = static_cast<Gtk::ApplicationWindow*>(this->widget)->get_focus();
    Gtk::Editable* editable = dynamic_cast<Gtk::Editable*>(widget);
    if (editable) {
        editable->cut_clipboard();
    } else {
        Gtk::TextView* textview = dynamic_cast<Gtk::TextView*>(widget);
        if (textview) {
#if 0
            widget.emit("cut-clipboard");
#endif
        }
    }
}

void MeldWindow::on_menu_paste_activate() {
    Gtk::Widget* widget = static_cast<Gtk::ApplicationWindow*>(this->widget)->get_focus();
    Gtk::Editable* editable = dynamic_cast<Gtk::Editable*>(widget);
    if (editable) {
        editable->paste_clipboard();
    } else {
        Gtk::TextView* textview = dynamic_cast<Gtk::TextView*>(widget);
        if (textview) {
#if 0
            widget.emit("paste-clipboard");
#endif
        }
    }
}

void MeldWindow::on_action_fullscreen_toggled(Glib::RefPtr<Gtk::ToggleAction> action_fullscreen) {
    Gdk::WindowState window_state = this->widget->get_window()->get_state();
    bool is_full = window_state & Gdk::WINDOW_STATE_FULLSCREEN;
    if (action_fullscreen->get_active() and not is_full) {
        static_cast<Gtk::ApplicationWindow*>(this->widget)->fullscreen();
    } else if (is_full) {
        static_cast<Gtk::ApplicationWindow*>(this->widget)->unfullscreen();
    }
}

void MeldWindow::on_menu_edit_down_activate() {
#if 0
    this->current_doc()->next_diff(Gdk.ScrollDirection.DOWN);
#endif
}

void MeldWindow::on_menu_edit_up_activate() {
#if 0
    this->current_doc().next_diff(Gdk.ScrollDirection.UP);
#endif
}

void MeldWindow::on_open_external() {
    this->current_doc()->open_external();
}

void MeldWindow::on_toolbar_stop_clicked() {
    this->current_doc()->stop();
}

void MeldWindow::on_prev_tab() {
    this->notebook->prev_page();
}

void MeldWindow::on_next_tab() {
    this->notebook->next_page();
}

void MeldWindow::on_move_tab_prev() {
    int page_num = this->notebook->get_current_page();
    Gtk::Widget* child = this->notebook->get_nth_page(page_num);
    if (page_num > 0) {
        page_num = page_num - 1;
    } else {
        page_num = 0;
    }
    this->notebook->reorder_child(*child, page_num);
}

void MeldWindow::on_move_tab_next() {
    int page_num = this->notebook->get_current_page();
    Gtk::Widget* child = this->notebook->get_nth_page(page_num);
    this->notebook->reorder_child(*child, page_num + 1);
}

void MeldWindow::current_tab_changed_cb(const Glib::RefPtr<Gtk::RadioAction>& action/*, int current*/) {
#if 0
    if (action == current) {
#endif
        this->notebook->set_current_page(action->get_current_value());
#if 0
    }
#endif
}

void MeldWindow::_update_notebook_menu(Gtk::Widget*, guint) {
    if (this->tab_switch_merge_id) {
        this->ui->remove_ui(this->tab_switch_merge_id);
        this->ui->remove_action_group(this->tab_switch_actiongroup);
    }

    this->tab_switch_merge_id = this->ui->new_merge_id();
    this->tab_switch_actiongroup = Gtk::ActionGroup::create("TabSwitchActions");
    this->ui->insert_action_group(this->tab_switch_actiongroup);
    Glib::RefPtr<Gtk::RadioAction> group;
    int current_page = this->notebook->get_current_page();
    for (int i = 0; i < this->notebook->get_n_pages(); i++) {
        Gtk::Widget* page = this->notebook->get_nth_page(i);
        Glib::ustring label = this->notebook->get_menu_label_text(*page);
        boost::format fmt("SwitchTab%d");
        fmt % i;
        Glib::ustring name = fmt.str();
        Glib::ustring tooltip = _("Switch to this tab");
        Glib::RefPtr<Gtk::RadioAction> action = Gtk::RadioAction::create();
        action->set_name(name);
        action->set_label(label);
        action->set_tooltip(tooltip);
        action->property_value() = i;
        action->join_group(group);
        group = action;
        action->set_active(current_page == i);

        action->signal_changed().connect(sigc::mem_fun(this, &MeldWindow::current_tab_changed_cb));
        Gtk::AccelKey accel;
        if (i < 10) {
            boost::format fmt("<Alt>%d");
            fmt % ((i + 1) % 10);
            accel = Gtk::AccelKey(fmt.str());
        }
        this->tab_switch_actiongroup->add(action, accel);
        this->ui->add_ui(this->tab_switch_merge_id,
                       "/Menubar/TabMenu/TabPlaceholder",
                       name, name, Gtk::UI_MANAGER_MENUITEM, false);
    }
}

/*! See if a page will allow itself to be removed */
Gtk::ResponseType MeldWindow::try_remove_page(Component* page, int appquit) {
    Gtk::ResponseType response;
    Tab* d = dynamic_cast<Tab*>(page);
    if (d) {
        response = d->on_delete_event(appquit);
    } else {
        throw new ValueError("Component passed as page not handled");
    }
    if (response != Gtk::RESPONSE_CANCEL) {
        MeldDoc* melddoc = dynamic_cast<MeldDoc*>(d);
        if (melddoc) {
#if 0
            this->scheduler.remove_scheduler(melddoc->scheduler);
#endif
        }
        int page_num = this->notebook->page_num(*(page->widget));
        assert(page_num >= 0);
        // If the page we're removing is the current page, we need to
        // disconnect and clear undo handlers, and trigger a switch out
        if (this->notebook->get_current_page() == page_num) {
            if (this->diff_handler) {
                this->diff_handler.disconnect();
            }
            if (!this->undo_handlers.empty()) {
                for (sigc::connection handler : this->undo_handlers) {
                    handler.disconnect();
                }
            }
            this->undo_handlers.clear();
            if (d) {
                d->on_container_switch_out_event(this->ui);
            }
        }

        this->notebook->remove_page(page_num);
#if 0
        if (this->notebook->get_n_pages() == 0) {
            static_cast<Gtk::ApplicationWindow*>(this->widget)->set_title("Meld");
            this->_update_page_action_sensitivity();
        }
#endif
    }
    return response;
}

void MeldWindow::on_file_changed(Glib::ustring filename, MeldDoc* srcpage) {
    for (Gtk::Widget* c : this->notebook->get_children()) {
        Component* page = (Component *) pyobject[c];
        if (page != srcpage) {
            MeldDoc* p = dynamic_cast<MeldDoc*>(page);
            if (p) {
                p->on_file_changed(filename);
            }
        }
    }
}

void MeldWindow::_append_page(Component* page, std::string icon) {
    // TODO: egore memory eak
    NotebookLabel* nbl = new NotebookLabel(Gtk::StockID(icon), "", [this, page] () { this->try_remove_page(page); });
    this->notebook->append_page(*(page->widget), *nbl);

    // Change focus to the newly created page only if the user is on a
    // DirDiff or VcView page, or if it's a new tab page. This prevents
    // cycling through X pages when X diffs are initiated.
    if (typeid(this->current_doc()) == typeid(DirDiff*) or
        typeid(this->current_doc()) == typeid(VcView*) or
        typeid(page) == typeid(NewDiffTab*)) {
        this->notebook->set_current_page(this->notebook->page_num(*(page->widget)));
    }

    MeldDoc* d = dynamic_cast<MeldDoc*>(page);
    if (d) {
        d->signal_label_changed().connect(sigc::mem_fun(this, &MeldWindow::on_notebook_label_changed));
        d->signal_file_changed().connect(sigc::bind(sigc::mem_fun(this, &MeldWindow::on_file_changed), d));
        d->signal_create_diff().connect(sigc::mem_fun(this, &MeldWindow::append_diff));
    }

    this->notebook->set_tab_reorderable(*(page->widget), true);
}

void MeldWindow::diff_created_cb(Component* newdoc, Component* doc) {
    this->try_remove_page(doc);
    int idx = this->notebook->page_num(*(newdoc->widget));
    this->notebook->set_current_page(idx);
}

NewDiffTab* MeldWindow::append_new_comparison() {
    NewDiffTab* doc = new NewDiffTab(this);
    this->_append_page(doc, "document-new");
    this->on_notebook_label_changed(doc, _("New comparison"), "");

    doc->signal_diff_created().connect(sigc::bind(sigc::mem_fun(this, &MeldWindow::diff_created_cb), doc));
    return doc;
}

DirDiff* MeldWindow::append_dirdiff(std::vector<std::string> dirs, bool auto_compare) {
    for (std::string asd : dirs) {
        std::cout << asd << std::endl;
    }
    assert(dirs.size() >= 1 && dirs.size() <= 3);
    DirDiff* doc = new DirDiff(dirs.size(), this->scheduler);
    this->_append_page(doc, "folder");
    doc->set_locations(dirs);
    // FIXME: This doesn't work, as dirdiff behaves differently to vcview
    if (auto_compare) {
        doc->on_button_diff_clicked();
    }
    return doc;
}

FileDiff* MeldWindow::append_filediff(std::vector<std::string> files, std::string merge_output, std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta) {
    assert(files.size() >= 1 && files.size() <= 3);
    FileDiff* doc = new FileDiff(files.size(), this->scheduler);
    this->_append_page(doc, "text-x-generic");
    doc->set_files(files);
    if (!merge_output.empty()) {
        doc->set_merge_output_file(merge_output);
    }
    if (!meta.empty()) {
        doc->set_meta(meta);
    }
    return doc;
}

FileMerge* MeldWindow::append_filemerge(std::vector<std::string> files, std::string merge_output) {
    if (files.size() != 3) {
        boost::format fmt(_("Need three files to auto-merge, got: %r"));
        fmt % files.size();
        throw new ValueError(fmt.str());
    }
    FileMerge* doc = new FileMerge(files.size(), this->scheduler);
    this->_append_page(doc, "text-x-generic");
    doc->set_files(files);
    if (!merge_output.empty()) {
        doc->set_merge_output_file(merge_output);
    }
    return doc;
}

MeldDoc* MeldWindow::append_diff(std::vector<std::string> paths, bool auto_compare, bool auto_merge, std::string merge_output, std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta) {
    std::vector<std::string> dirslist;
    std::vector<std::string> fileslist;
    for (std::string p : paths) {
        if (boost::filesystem::is_directory(p)) {
            dirslist.push_back(p);
        }
        if (boost::filesystem::is_regular(p)) {
            fileslist.push_back(p);
        }
    }
    if (!dirslist.empty() and !fileslist.empty()) {
        throw ValueError(
            _("Cannot compare a mixture of files and directories"));
    } else if (!dirslist.empty()) {
        return this->append_dirdiff(paths, auto_compare);
    } else if (auto_merge) {
        return this->append_filemerge(paths, merge_output);
    } else {
        return this->append_filediff(paths, merge_output, meta);
    }
}

VcView* MeldWindow::append_vcview(std::string location, bool auto_compare) {
    VcView* doc = new VcView(this->scheduler);
    this->_append_page(doc, "meld-version-control");
    doc->set_location(location);
    if (auto_compare) {
        doc->on_button_diff_clicked();
    }
    return doc;
}

MeldDoc* MeldWindow::append_recent(const std::string& uri) {
    std::tuple<std::string, std::vector<std::string>, int> tmp = recent_comparisons->read(uri);
    std::string comparison_type = std::get<0>(tmp);
    std::vector<std::string> files = std::get<1>(tmp);
    int flags = std::get<2>(tmp);
    MeldDoc* tab;
    if (comparison_type == TYPE_MERGE) {
        tab = this->append_filemerge(files);
    } else if (comparison_type == TYPE_FOLDER) {
        tab = this->append_dirdiff(files);
    } else if (comparison_type == TYPE_VC) {
        // Files should be a single-element iterable
        tab = this->append_vcview(files[0]);
    } else {  // comparison_type == recent.TYPE_FILE:
        tab = this->append_filediff(files);
    }
    this->notebook->set_current_page(this->notebook->page_num(*(dynamic_cast<Component*>(tab)->widget)));
    recent_comparisons->add(tab);
    return tab;
}

void MeldWindow::_single_file_open(std::string path) {
    VcView* doc = new VcView(scheduler);
    path = boost::filesystem::path(path).string();
    doc->set_location(path);
    doc->signal_create_diff().connect(sigc::mem_fun(this, &MeldWindow::append_diff));
    doc->run_diff(path);
    delete doc;
}

MeldDoc* MeldWindow::open_paths(std::vector<std::string> paths, bool auto_compare, bool auto_merge,
               bool focus) {
    MeldDoc* tab = nullptr;
    if (paths.size() == 1) {
        std::string a = paths[0];
        if (boost::filesystem::is_regular(a)) {
            this->_single_file_open(a);
        } else {
            tab = this->append_vcview(a, auto_compare);
        }
    } else if (paths.size() == 2 || paths.size() == 3) {
        tab = this->append_diff(paths, auto_compare, auto_merge);
    }
    if (tab) {
        recent_comparisons->add(tab);
        if (focus) {
            this->notebook->set_current_page(
                this->notebook->page_num(*(tab->widget)));
        }
    }

    return tab;
}

/*! Get the current doc or a dummy object if there is no current */
MeldDoc* MeldWindow::current_doc() {
    int index = this->notebook->get_current_page();
    if (index >= 0) {
        Component* page = (Component *) pyobject[this->notebook->get_nth_page(index)];
        MeldDoc* doc = dynamic_cast<MeldDoc*>(page);
        if (doc) {
            return doc;
        }
    }

#if 0
    return DummyDoc();
#else
    return nullptr;
#endif
}
