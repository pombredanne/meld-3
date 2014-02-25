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

#include <boost/filesystem.hpp>

#include "newdifftab.h"
#include "recent.h"
#include "tab.h"

NewDiffTab::NewDiffTab(MeldWindow *parentapp) : Tab("tab-placeholder.ui", "new_comparison_tab") {

    // Initialize member variables
    this->builder->get_widget("file_chooser0", this->file_chooser0);
    this->builder->get_widget("file_chooser1", this->file_chooser1);
    this->builder->get_widget("file_chooser2", this->file_chooser2);
    this->file_chooser = { this->file_chooser0, this->file_chooser1, this->file_chooser2 };

    this->builder->get_widget("dir_chooser0", this->dir_chooser0);
    this->builder->get_widget("dir_chooser1", this->dir_chooser1);
    this->builder->get_widget("dir_chooser2", this->dir_chooser2);
    this->dir_chooser = { this->dir_chooser0, this->dir_chooser1, this->dir_chooser2 };

    this->builder->get_widget("vc_chooser0", this->vc_chooser0);
    this->vc_chooser = { this->vc_chooser0 };

    this->builder->get_widget("button_type_file", this->button_type_file);
    this->builder->get_widget("button_type_dir", this->button_type_dir);
    this->builder->get_widget("button_type_vc", this->button_type_vc);
    this->builder->get_widget("file_three_way_checkbutton", this->file_three_way_checkbutton);
    this->builder->get_widget("dir_three_way_checkbutton", this->dir_three_way_checkbutton);
    this->builder->get_widget("button_new_blank", this->button_new_blank);
    this->builder->get_widget("button_compare", this->button_compare);

    this->builder->get_widget("choosers_notebook", this->choosers_notebook);

    // Hook up signals
    this->button_type_file->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_button_type_toggled), this->button_type_file));
    this->button_type_dir->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_button_type_toggled), this->button_type_dir));
    this->button_type_vc->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_button_type_toggled), this->button_type_vc));
    this->file_three_way_checkbutton->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_three_way_checkbutton_toggled), this->file_three_way_checkbutton));
    this->file_chooser0->signal_file_set().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_file_set), this->file_chooser0));
    this->file_chooser1->signal_file_set().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_file_set), this->file_chooser1));
    this->file_chooser2->signal_file_set().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_file_set), this->file_chooser2));
    this->dir_three_way_checkbutton->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &NewDiffTab::on_three_way_checkbutton_toggled), this->dir_three_way_checkbutton));
    this->button_new_blank->signal_clicked().connect(sigc::mem_fun(this, &NewDiffTab::on_button_new_blank_clicked));
    this->button_compare->signal_clicked().connect(sigc::mem_fun(this, &NewDiffTab::on_button_compare_clicked));

    // Rest of Construction
    this->button_types = {this->button_type_file, this->button_type_dir,
                         this->button_type_vc};
    this->diff_methods = {[parentapp] (std::vector<std::string> files) { return parentapp->append_filediff(files); },
                          [parentapp] (std::vector<std::string> dirs) { return parentapp->append_dirdiff(dirs); },
                          [parentapp] (std::vector<std::string> location) { return parentapp->append_vcview(location[0]); } };
    this->diff_type = -1;

    // FIXME: cross platform?
    std::string default_path = getenv("HOME");
    this->file_chooser0->set_current_folder(default_path);
    this->file_chooser1->set_current_folder(default_path);
    this->file_chooser2->set_current_folder(default_path);

    this->widget->show();
}

void NewDiffTab::on_button_type_toggled(Gtk::ToggleButton* button) {
    if (not button->get_active()) {
        bool any_active = false;
        for (Gtk::ToggleButton* b : this->button_types) {
            if (b->get_active()) {
                any_active = true;
            }
        }
        if (not any_active) {
            button->set_active(true);
        }
        return;
    }

    for (Gtk::ToggleButton* b : this->button_types) {
        if (b != button) {
            b->set_active(false);
        }
    }

    this->diff_type = std::find(this->button_types.begin(), this->button_types.end(), button) - this->button_types.begin();
    this->choosers_notebook->set_current_page(this->diff_type + 1);
    // FIXME: Add support for new blank for DirDiff and VcView
    this->button_new_blank->set_sensitive(this->diff_type == 0);
    this->button_compare->set_sensitive(true);
}

void NewDiffTab::on_three_way_checkbutton_toggled(Gtk::CheckButton* button) {
    if (button == this->file_three_way_checkbutton) {
        this->file_chooser2->set_sensitive(button->get_active());
    } else {  // button is this->dir_three_way_checkbutton
        this->dir_chooser2->set_sensitive(button->get_active());
    }
}

void NewDiffTab::on_file_set(Gtk::FileChooserButton* button) {
    std::string filename = button->get_filename();
    if (filename.empty()) {
        return;
    }

    std::string parent = boost::filesystem::path(filename).parent_path().string();
    if (boost::filesystem::is_directory(parent)) {
        if (this->file_chooser0->get_filename().empty()) {
            this->file_chooser0->set_current_folder(parent);
        }
        if (this->file_chooser1->get_filename().empty()) {
            this->file_chooser1->set_current_folder(parent);
        }
        if (this->file_chooser2->get_filename().empty()) {
            this->file_chooser2->set_current_folder(parent);
        }
    }

    // TODO: We could do checks here to prevent errors: check to see if
    // we've got binary files; check for null file selections; sniff text
    // encodings; check file permissions.
}

int NewDiffTab::_get_num_paths() {
    int num_paths;
    if (this->diff_type == 0) {
        bool three_way = this->file_three_way_checkbutton->get_active();
        num_paths = three_way ? 3 : 2;
    } else if (this->diff_type == 1) {
        bool three_way = this->dir_three_way_checkbutton->get_active();
        num_paths = three_way ? 3 : 2;
    } else {  // this->diff_type == 2
        num_paths = 1;
    }
    return num_paths;
}

void NewDiffTab::on_button_compare_clicked() {
    static const std::vector<std::vector<Gtk::FileChooserButton*>> type_choosers = {{this->file_chooser0, this->file_chooser1, this->file_chooser2}, {this->dir_chooser0, this->dir_chooser1, this->dir_chooser2}, {this->vc_chooser0}};

    std::vector<std::string> compare_paths;
    int num_paths = this->_get_num_paths();
    for (int i = 0; i < num_paths; i++) {
        Gtk::FileChooserButton* chooser = type_choosers[this->diff_type][i];
        Glib::RefPtr<const Gio::File> gfile = chooser->get_file();
        std::string path = gfile ? gfile->get_path() : "";
#if 0
        path = path.decode("utf8")
#endif
        compare_paths.push_back(path);
    }

    MeldDoc* tab = this->diff_methods[this->diff_type](compare_paths);
    recent_comparisons->add(tab);
    this->signal_diff_created().emit(tab);
}

void NewDiffTab::on_button_new_blank_clicked() {
    // TODO: This doesn't work the way I'd like for DirDiff and VCView.
    // It should do something similar to FileDiff; give a tab with empty
    // file entries and no comparison done.
    std::vector<std::string> compare_paths;
    for (int i = 0; i < this->_get_num_paths(); i++) {
        compare_paths.push_back("");
    }
    MeldDoc *tab = this->diff_methods[this->diff_type](compare_paths);
    this->signal_diff_created().emit(tab);
}

void NewDiffTab::on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui) {
}

void NewDiffTab::on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui) {
}

Gtk::ResponseType NewDiffTab::on_delete_event(int appquit) {
    return Gtk::RESPONSE_OK;
}
