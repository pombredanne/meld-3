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

#include "settings.h"
#include "util/compat.h"
#include "sourceview.h"
#include "conf.h"

#include "patchdialog.h"

PatchDialog::PatchDialog(FileDiff* filediff) : Component("patch-dialog.ui", "patchdialog") {

    // Initialize member variables
    this->builder->get_widget("left_radiobutton", this->left_radiobutton);
    this->builder->get_widget("right_radiobutton", this->right_radiobutton);
    this->builder->get_widget("reverse_checkbutton", this->reverse_checkbutton);

    // Hook up signals
    this->left_radiobutton->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &PatchDialog::on_buffer_selection_changed), this->left_radiobutton));
    this->right_radiobutton->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &PatchDialog::on_buffer_selection_changed), this->right_radiobutton));
    this->reverse_checkbutton->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &PatchDialog::on_reverse_checkbutton_toggled), this->reverse_checkbutton));

    // Rest of construction
    static_cast<Gtk::Dialog*>(this->widget)->set_transient_for(*(static_cast<Gtk::Window*>(static_cast<Gtk::VBox*>(filediff->widget)->get_toplevel())));
    this->filediff = filediff;

    this->textview->set_buffer(buf);
#if 0
    Glib::RefPtr<const Gsv::Language> lang = LanguageManager::get_language_from_mime_type("text/x-diff");
    buf->set_language(lang);
#endif
    buf->set_highlight_syntax(true);

#if 0
    this->textview->modify_font(meldsettings->font());
#endif
    this->textview->set_editable(false);

    this->index_map = {{this->left_radiobutton, std::pair<int, int>(0, 1)},
                       {this->right_radiobutton, std::pair<int, int>(1, 2)}};
    this->left_patch = true;
    this->reverse_patch = this->reverse_checkbutton->get_active();

    if (this->filediff->num_panes < 3) {
        this->side_selection_label->hide();
        this->side_selection_box->hide();
    }

#if 0
    meldsettings->connect("changed", this->on_setting_changed);
#endif
}

void PatchDialog::on_setting_changed(int setting, std::string key) {
    if (key == "font") {
#if 0
        this->textview->modify_font(meldsettings->font());
#endif
    }
}

void PatchDialog::on_buffer_selection_changed(Gtk::RadioButton* radiobutton) {
    if (not radiobutton->get_active()) {
        return;
    }
    this->left_patch = radiobutton == this->left_radiobutton;
    this->update_patch();
}

void PatchDialog::on_reverse_checkbutton_toggled(Gtk::CheckButton* checkbutton) {
    this->reverse_patch = checkbutton->get_active();
    this->update_patch();
}

void PatchDialog::update_patch() {
    std::pair<int, int> indices = {0, 1};
    if (not this->left_patch) {
        indices = {1, 2};
    }
    if (this->reverse_patch) {
        indices = {indices.second, indices.first};
    }

    std::vector<std::list<std::string>> texts;
    for (Glib::RefPtr<Gtk::TextBuffer> b : this->filediff->textbuffer) {
        Gtk::TextBuffer::iterator start, end;
        b->get_bounds(start, end);
#if 0
        Glib::ustring text = text_type(b->get_text(start, end, false), "utf8");
#else
        Glib::ustring text = b->get_text(start, end, false);
#endif
        std::istringstream stream(text);
        std::string line;
        std::list<std::string> lines;
        while(std::getline(stream, line)) {
            lines.push_back(line + '\n');
        }

        texts.push_back(lines);
    }

#if 0
    std::list<std::string> _names;
    for (int i = 0; i < 3; i++) {
        _names.push_back(this->filediff->textbuffer[i]->data.label);
    }
    std::string prefix = os.path.commonprefix(_names);
    std::vector<std::string> names;
    for (std::string n : _names) {
        names.push_back(n[prefix.rfind("/") + 1:]);
    }

    Glib::RefPtr<const Gtk::TextBuffer> buf = this->textview->get_buffer();
    std::list<std::string> text0 = texts[indices[0]];
    std::list<std::string> text1 = texts[indices[1]];
    std::string name0 = names[indices[0]];
    std::string name1 = names[indices[1]];
    std::string diff_text = "".join(difflib.unified_diff(text0, text1, name0, name1));
    buf->set_text(diff_text);
#endif
}

void PatchDialog::run() {
    this->update_patch();

    while (1) {
        int result = static_cast<Gtk::Dialog*>(this->widget)->run();
        if (result < 0) {
            break;
        }

        Glib::RefPtr<Gtk::TextBuffer> buf = this->textview->get_buffer();
        Gtk::TextBuffer::iterator start, end;
        buf->get_bounds(start, end);
#if 0
        txt = text_type(buf.get_text(start, end, false), "utf8");
#else
        Glib::ustring txt = buf->get_text(start, end, false);
#endif
        // Copy patch to clipboard
        if (result == 1) {
            Glib::RefPtr<Gtk::Clipboard> clip = Gtk::Clipboard::get();
            clip->set_text(txt);
            clip->store();
            break;
        // Save patch as a file
        } else {
            // FIXME: These filediff methods are actually general utility.
            std::string filename = this->filediff->_get_filename_for_saving(
                _("Save Patch"));
            if (!filename.empty()) {
                this->filediff->_save_text_to_filename(filename, txt);
                break;
            }
        }
    }

    this->widget->hide();
}
