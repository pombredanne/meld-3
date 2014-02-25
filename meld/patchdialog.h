/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2010, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__PATCHDIALOG_H__
#define __MELD__PATCHDIALOG_H__

#include <gtksourceviewmm.h>

#include "ui/gnomeglade.h"
#include "filediff.h"

class PatchDialog : public Component {
private:
    FileDiff* filediff;
    Glib::RefPtr<Gsv::Buffer> buf;
    Gtk::TextView* textview;
    Gtk::RadioButton* left_radiobutton;
    Gtk::RadioButton* right_radiobutton;
    std::map<Gtk::RadioButton*, std::pair<int, int>> index_map;
    bool left_patch;
    Gtk::CheckButton* reverse_checkbutton;
    bool reverse_patch;
    Glib::RefPtr<Gtk::Label> side_selection_label;
    Glib::RefPtr<Gtk::HBox> side_selection_box;
public:
    PatchDialog(FileDiff* filediff);

    void on_setting_changed(int setting, std::string key);

    void on_buffer_selection_changed(Gtk::RadioButton* radiobutton);

    void on_reverse_checkbutton_toggled(Gtk::CheckButton* checkbutton);

    void update_patch();

    void run();
};

#endif
