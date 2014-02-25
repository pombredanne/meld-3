/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2011-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__MELDDOC_H__
#define __MELD__MELDDOC_H__

#include <boost/variant.hpp>

#include "task.h"
#include "melddoc.h"
#include "tab.h"

class VcView;

/*! Base class for documents in the meld application. */
class MeldDoc : public Tab {
public:
    typedef sigc::signal<void, Glib::ustring> type_signal_file_changed;
    type_signal_file_changed signal_file_changed() {
        return m_signal_file_changed;
    }
    type_signal_file_changed m_signal_file_changed;
    typedef sigc::signal<MeldDoc*, std::vector<std::string>, bool, bool, std::string, std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>>> type_signal_create_diff;
    type_signal_create_diff signal_create_diff() {
        return m_signal_create_diff;
    }
protected:
    type_signal_create_diff m_signal_create_diff;
public:
    typedef sigc::signal<bool, GdkEventFocus*> type_signal_current_diff_changed;
    type_signal_current_diff_changed signal_current_diff_changed() {
        return m_signal_current_diff_changed;
    }
    type_signal_current_diff_changed m_signal_current_diff_changed;
    typedef sigc::signal<void, bool, bool> type_signal_next_diff_changed;
    type_signal_next_diff_changed signal_next_diff_changed() {
        return m_signal_next_diff_changed;
    }
    type_signal_next_diff_changed m_signal_next_diff_changed;
    typedef sigc::signal<void, bool> type_signal_close;
    type_signal_close signal_close() {
        return m_signal_close;
    }
    type_signal_close m_signal_close;

    typedef sigc::signal<void, Component*, Glib::ustring, Glib::ustring> type_signal_label_changed;
    type_signal_label_changed signal_label_changed() {
        return m_signal_label_changed;
    }
    type_signal_label_changed m_signal_label_changed;

    SchedulerBase& scheduler;
    int num_panes;
    Glib::ustring label_text;
    Glib::ustring tooltip_text;
private:
    Glib::ustring ui_file;
    Gtk::UIManager::ui_merge_id ui_merge_id;
public:
    Glib::RefPtr<Gtk::ActionGroup> DirdiffActions;
    Glib::RefPtr<Gtk::ActionGroup> main_actiongroup;
    Glib::RefPtr<Gtk::ActionGroup> actiongroup;
    Gtk::Menu* popup_menu;

    MeldDoc(SchedulerBase& scheduler, std::string filename, std::string root, std::list<std::string> extra = {});
    virtual ~MeldDoc();

    /*! Get the comparison type and path(s) being compared */
    virtual std::pair<std::string, std::vector<std::string>> get_comparison() = 0;

    virtual void save();

    virtual void save_as();

    void stop();

    void os_open(std::string path, std::string uri);

    void open_cb(const Glib::RefPtr<Gio::AsyncResult>& result, Glib::RefPtr<Gio::File> source, int line);

    void _open_files(std::list<std::string> selected, int line = 0);

    void open_external();

    virtual void on_refresh_activate();

    virtual void on_find_activate();

    virtual void on_find_next_activate();

    virtual void on_find_previous_activate();

    virtual void on_replace_activate();

    void on_file_changed(std::string filename);

    void label_changed();

    virtual void set_labels(std::vector<Glib::ustring> lst);

    /*! Called when the container app switches to this tab. */
    virtual void on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> uimanager);

    /*! Called when the container app switches away from this tab. */
    virtual void on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> uimanager);

    /*!
     * Called when the docs container is about to close.
     *
     * A doc normally returns Gtk.ResponseType.OK, but may instead return
     * Gtk.ResponseType.CANCEL to request that the container not delete it.
     */
    virtual Gtk::ResponseType on_delete_event(int appquit = 0);

    virtual void next_diff(GdkScrollDirection direction, bool centered = false) = 0;
};

#endif
