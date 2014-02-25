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

#include "vcdialogs.h"
#include "../conf.h"

// FIXME: Duplication from vcview
static std::string _commonprefix(std::vector<std::string> files) {
    std::string workdir;
    if (files.size() != 1) {
        workdir = commonprefix(files);
    } else {
        workdir = boost::filesystem::path(files[0]).parent_path().string();
    }
    return workdir;
}


CommitDialog::CommitDialog(VcView* parent) : Glib::Object(), Component("vcview.ui", "commitdialog") {

    this->previousentry->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &CommitDialog::on_previousentry_activate), this->previousentry));

    this->parent = parent;
#if 0
    this->widget->set_transient_for(parent->widget->get_toplevel());
#endif
    std::vector<std::string> selected = parent->_get_selected_files();

    std::vector<std::string> to_commit;
    std::string topdir;
    try {
        to_commit = parent->vc->get_files_to_commit(selected);
        topdir = parent->vc->root;
#if 0
        if (to_commit) {
            to_commit = ["\t" + s for s in to_commit];
        } else {
            to_commit = ["\t" + _("No files will be committed")];
        }
#endif
    } catch (NotImplementedError &e) {
        topdir = _commonprefix(selected);
#if 0
        to_commit = ["\t" + s[len(topdir) + 1:] for s in selected];
#endif
    }
    this->changedfiles->set_text("(in " + topdir + ")\n" + boost::algorithm::join(to_commit, ", "));

#if 0
    this->textview->modify_font(meldsettings->font);
    std::string commit_prefill = this->parent->vc->get_commit_message_prefill();
    if (commit_prefill) {
        buf = this->textview.get_buffer();
        buf.set_text(commit_prefill);
        buf.place_cursor(buf.begin());
    }
#endif

    // Try and make the textview wide enough for a standard 80-character
    // commit message.
    Glib::RefPtr<Pango::Context> context = this->textview->get_pango_context();
    Pango::FontMetrics metrics = context->get_metrics(meldsettings->font(),
                                  context->get_language());
    double char_width = metrics.get_approximate_char_width() / Pango::SCALE;
    this->scrolledwindow1->set_size_request(80 * char_width, -1);

#if 0
    settings->bind("vc-show-commit-margin", this->textview,
                  "show-right-margin", Gio::SETTINGS_BIND_DEFAULT);
    settings->bind("vc-commit-margin", this->textview,
                  "right-margin-position", Gio::SETTINGS_BIND_DEFAULT);
#endif
    settings->bind("vc-break-commit-message", this,
                  "break-commit-message", Gio::SETTINGS_BIND_DEFAULT);
    this->widget->show_all();
}

void CommitDialog::run() {
    this->previousentry->set_active(-1);
    this->textview->grab_focus();
    int response = static_cast<Gtk::Dialog*>(this->widget)->run();
    if (response == Gtk::RESPONSE_OK) {
#if 0
        show_margin = this->textview->get_show_right_margin();
        margin = this->textview->get_right_margin_position();
#endif
        Glib::RefPtr<const Gtk::TextBuffer> buf = this->textview->get_buffer();
        Gtk::TextBuffer::iterator range_begin;
        Gtk::TextBuffer::iterator range_end;
#if 0
        buf->get_bounds(range_begin, range_end);
        Glib::ustring msg = buf->get_text(range_begin, range_end, false);
        // This is a dependent option because of the margin column
        if (show_margin and this->props.break_commit_message) {
            paragraphs = msg.split("\n\n");
            msg = "\n\n".join(textwrap.fill(p, margin) for p in paragraphs);
        }
        this->parent._command_on_selected(
            this->parent.vc.commit_command(msg));
        if (msg.strip()) {
            this->previousentry.prepend_history(msg);
        }
#endif
    }
#if 0
    this->widget.destroy();
#endif
}

void CommitDialog::on_previousentry_activate(HistoryCombo* gentry) {
    Gtk::TreeModel::iterator idx = gentry->get_active();
    if (idx) {
        Glib::RefPtr<const Gtk::TreeModel> model = gentry->get_model();
        Glib::RefPtr<const Gtk::TextBuffer> buf = this->textview->get_buffer();
#if 0
        buf->set_text(model[idx][1]);
#endif
    }
}


PushDialog::PushDialog(VcView* parent) : Component("vcview.ui", "pushdialog") {
    this->parent = parent;
#if 0
    this->widget->set_transient_for(parent->widget->get_toplevel());
#endif
    this->widget->show_all();
}

void PushDialog::run() {
    // TODO: Ask the VC for a more informative label for what will happen.
    // In git, this is probably the parsed output of push --dry-run.

    int response = static_cast<Gtk::Dialog*>(this->widget)->run();
    if (response == Gtk::RESPONSE_OK) {
#if 0
        this->parent->vc->push(this->parent->_command);
#endif
    }
#if 0
    this->widget->destroy();
#endif
}
