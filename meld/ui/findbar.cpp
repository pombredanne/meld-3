/* Copyright (C) 2002-2009 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include <cassert>
#include <boost/regex.hpp>

#include "../misc.h"
#include "../util/compat.h"
#include "../conf.h"

#include "findbar.h"

FindBar::FindBar(Gtk::Container &parent) : Component("findbar.ui", "findbar", {"arrow_left", "arrow_right"}) {

    // Initialize member variables
    this->builder->get_widget("find_entry", this->find_entry);
    this->builder->get_widget("arrow_left", this->arrow_left);
    this->builder->get_widget("arrow_right", this->arrow_right);
    this->builder->get_widget("find_next_button", this->find_next_button);
    this->builder->get_widget("find_previous_button", this->find_previous_button);
    this->builder->get_widget("hbuttonbox2", this->hbuttonbox2);
    this->builder->get_widget("match_case", this->match_case);
    this->builder->get_widget("replace_entry", this->replace_entry);
    this->builder->get_widget("replace_label", this->replace_label);
    this->builder->get_widget("whole_word", this->whole_word);
    this->builder->get_widget("wrap_box", this->wrap_box);
    this->builder->get_widget("replace_button", this->replace_button);
    this->builder->get_widget("replace_all_button", this->replace_all_button);

    // Hook up signals
    this->replace_button->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_replace_button_clicked));
    this->replace_all_button->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_replace_all_button_clicked));
    this->find_previous_button->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_find_previous_button_clicked));
    this->find_next_button->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_find_next_button_clicked));
    this->find_entry->signal_changed().connect(sigc::mem_fun(this, &FindBar::on_find_entry_changed));
    this->find_entry->signal_activate().connect(sigc::mem_fun(this, &FindBar::on_find_next_button_clicked));
    this->replace_entry->signal_activate().connect(sigc::mem_fun(this, &FindBar::on_replace_button_clicked));

    this->set_text_view(nullptr);
    Glib::RefPtr<Gtk::StyleContext> context = this->find_entry->get_style_context();
    this->orig_base_color = context->get_background_color(Gtk::STATE_FLAG_NORMAL);
    this->arrow_left->show();
    this->arrow_right->show();
    parent.signal_set_focus_child().connect(sigc::mem_fun(*this, &FindBar::on_focus_child));
}

void FindBar::on_focus_child(Gtk::Widget* widget) {
    if (widget) {
        bool visible = this->widget->is_visible();
        if (widget != this->widget and visible) {
            this->hide();
        }
    }
}

void FindBar::hide() {
    this->set_text_view(nullptr);
    this->wrap_box->set_visible(false);
    this->widget->hide();
}

void FindBar::set_text_view(Gtk::TextView* textview) {
    this->textview = textview;
    if (textview) {
        this->search_context = gtk_source_search_context_new(
                                                             ((GtkSourceBuffer*) textview->get_buffer()->gobj()), nullptr);
    } else {
        this->search_context = nullptr;
    }

}

void FindBar::start_find(Gtk::TextView* textview, const Glib::ustring& text) {
    this->set_text_view(textview);
    this->replace_label->hide();
    this->replace_entry->hide();
    this->hbuttonbox2->hide();
    if (!text.empty()) {
        this->find_entry->set_text(text);
    }
    static_cast<Gtk::Grid*>(this->widget)->set_row_spacing(0);
    this->widget->show();
    this->find_entry->grab_focus();
}

void FindBar::start_find_next(Gtk::TextView* textview) {
    this->set_text_view(textview);
    if (!this->find_entry->get_text().empty()) {
        this->on_find_next_button_clicked();
    } else {
        this->start_find(this->textview);
    }
}

void FindBar::start_find_previous(Gtk::TextView* textview, const Glib::ustring& text) {
    this->set_text_view(textview);
    if (!this->find_entry->get_text().empty()) {
        this->on_find_previous_button_clicked();
    } else {
        this->start_find(this->textview);
    }
}

void FindBar::start_replace(Gtk::TextView* textview, const Glib::ustring& text) {
    this->textview = textview;
    if (!text.empty()) {
        this->find_entry->set_text(text);
    }
    static_cast<Gtk::Grid*>(this->widget)->set_row_spacing(6);
    this->widget->show_all();
    this->find_entry->grab_focus();
    this->wrap_box->set_visible(false);
}

void FindBar::on_find_next_button_clicked() {
    this->_find_text();
}

void FindBar::on_find_previous_button_clicked() {
    this->_find_text(1, true);
}

void FindBar::on_replace_button_clicked() {
    Glib::RefPtr<Gtk::TextBuffer> buf = this->textview->get_buffer();
    Gtk::TextBuffer::iterator oldsel_0, oldsel_1;
    bool oldsel = buf->get_selection_bounds(oldsel_0, oldsel_1);
    bool match = this->_find_text(0);
    Gtk::TextBuffer::iterator newsel_0, newsel_1;
    buf->get_selection_bounds(newsel_0, newsel_1);
    // Only replace if there is an already-selected match at the cursor
    if (match and oldsel and oldsel_0 == newsel_0 and
            oldsel_1 == newsel_1) {
        buf->begin_user_action();
#if 0
        buf->delete_selection(false, false);
        buf->insert_at_cursor(this->replace_entry.get_text());
        this->_find_text(0);
#endif
        buf->end_user_action();
    }
}

void FindBar::on_replace_all_button_clicked() {
    Glib::RefPtr<const Gtk::TextBuffer> buf = this->textview->get_buffer();
#if 0
    Glib::RefPtr<Gtk::TextBuffer::Mark> saved_insert = buf->create_mark(
        None, buf->get_iter_at_mark(buf->get_insert()), true);
    buf->begin_user_action();
    // Seek to the start
    buf->place_cursor(buf->begin());
    // Can't wrap or end up looping.
    while (this->_find_text(0, false, false)) {
        buf->delete_selection(false, false);
        buf->insert_at_cursor(this->replace_entry->get_text());
    }
    buf->end_user_action();
    if (not saved_insert.get_deleted()) {
        buf->place_cursor(buf->get_iter_at_mark(saved_insert));
        this->textview.scroll_to(
            buf->get_insert(), 0.25, 0.5, 0.5);
    }
#endif
}

void FindBar::on_find_entry_changed() {
    this->find_entry->override_background_color(this->orig_base_color,
                                    Gtk::STATE_FLAG_NORMAL);
    this->_find_text(0);
}

bool FindBar::_find_text(int start_offset, bool backwards, bool wrap) {
    bool match_case = this->match_case->get_active();
    bool whole_word = this->whole_word->get_active();
    bool regex = this->regex->get_active();
    assert(this->textview);
    assert(this->search_context);
    Glib::RefPtr<Gtk::TextBuffer> buf = this->textview->get_buffer();
    Gtk::TextBuffer::iterator insert = buf->get_iter_at_mark(buf->get_insert());
    Glib::ustring tofind = this->find_entry->get_text();
    gtk_source_search_settings_set_case_sensitive(gtk_source_search_context_get_settings(this->search_context), match_case);
    gtk_source_search_settings_set_at_word_boundaries(gtk_source_search_context_get_settings(this->search_context), whole_word);
    gtk_source_search_settings_set_regex_enabled(gtk_source_search_context_get_settings(this->search_context), regex);
    gtk_source_search_settings_set_search_text(gtk_source_search_context_get_settings(this->search_context), tofind.c_str());
    gtk_source_search_settings_set_wrap_around(gtk_source_search_context_get_settings(this->search_context), wrap);
    gtk_source_search_context_set_highlight(this->search_context, true);
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    this->wrap_box->set_visible(false);
    bool match;
    GtkTextIter* start_iter;
    GtkTextIter* end_iter;
    if (!backwards) {
        insert.forward_chars (start_offset);
        match = gtk_source_search_context_forward(this->search_context, insert.gobj(), start_iter, end_iter);
        if (match && (gtk_text_iter_get_offset(start_iter) < insert.get_offset())) {
            this->wrap_box->set_visible(true);
        }
    } else {
        match = gtk_source_search_context_backward(this->search_context, insert.gobj(), start_iter, end_iter);
        if (match && (gtk_text_iter_get_offset(start_iter) > insert.get_offset())) {
            this->wrap_box->set_visible(true);
        }
    }
    if (match) {
        buf->place_cursor(Glib::wrap(start_iter));
        buf->move_mark(buf->get_selection_bound(), Glib::wrap(end_iter));
        gtk_text_view_scroll_to_mark(this->textview->gobj(),
            buf->get_insert()->gobj(), 0.25, true, 0.5, 0.5);
        return true;
    } else {
        buf->place_cursor(buf->get_iter_at_mark(buf->get_insert()));
        // FIXME: Even though docs suggest this should work, it does
        // not. It just sets the selection colour on the text, without
        // affecting the entry colour at all.
        Gdk::RGBA color("#ffdddd");
        this->find_entry->override_background_color(
            color, Gtk::STATE_FLAG_NORMAL);
        this->wrap_box->set_visible(false);
    }

    return false;
}
