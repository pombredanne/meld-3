/* Copyright (C) 2011-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__NEWDIFFTAB_H__
#define __MELD__NEWDIFFTAB_H__

#include "meldwindow.h"
#include "tab.h"

class MeldWindow;

class NewDiffTab : public Tab {
public:
    typedef sigc::signal<void, Component*> type_signal_diff_created;
    type_signal_diff_created signal_diff_created() {
        return m_signal_diff_created;
    }
    type_signal_diff_created m_signal_diff_created;

private:
    Gtk::ToggleButton* button_type_file;
    Gtk::ToggleButton* button_type_dir;
    Gtk::ToggleButton* button_type_vc;
    std::vector<Gtk::ToggleButton*> button_types;
    std::vector<std::function<MeldDoc* (std::vector<std::string>)>> diff_methods;
    int diff_type;
    Gtk::FileChooserButton* file_chooser0;
    Gtk::FileChooserButton* file_chooser1;
    Gtk::FileChooserButton* file_chooser2;
    std::vector<Gtk::FileChooserButton*> file_chooser;
    Gtk::FileChooserButton* dir_chooser0;
    Gtk::FileChooserButton* dir_chooser1;
    Gtk::FileChooserButton* dir_chooser2;
    std::vector<Gtk::FileChooserButton*> dir_chooser;
    Gtk::FileChooserButton* vc_chooser0;
    std::vector<Gtk::FileChooserButton*> vc_chooser;
    Gtk::Notebook* choosers_notebook;
    Gtk::Button* button_new_blank;
    Gtk::Button* button_compare;
    Gtk::CheckButton* file_three_way_checkbutton;
    Gtk::CheckButton* dir_three_way_checkbutton;
public:
    NewDiffTab(MeldWindow *parentapp);

    void on_button_type_toggled(Gtk::ToggleButton* button);

    void on_three_way_checkbutton_toggled(Gtk::CheckButton* button);

    void on_file_set(Gtk::FileChooserButton* button);

    int _get_num_paths();

    void on_button_compare_clicked();

    void on_button_new_blank_clicked();

    virtual void on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui);

    virtual void on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui);

    virtual Gtk::ResponseType on_delete_event(int appquit = 0);
};

#endif
