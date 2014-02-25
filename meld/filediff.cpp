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

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>

#include <gtkmm.h>

#include "diffutil.h"
#include "matchers.h"
#include "merge.h"
#include "misc.h"
#include "patchdialog.h"
#include "recent.h"
#include "ui/findbar.h"
#include "ui/gnomeglade.h"
#include "settings.h"
#include "util/compat.h"
#include "sourceview.h"
#include "filediff.h"
#include "gutterrendererchunk.h"
#include "conf.h"
#include "vcview.h"
#include "const.h"

int process_pool = 0;
CachedSequenceMatcher::CachedSequenceMatcher() {
    if (!this->process_pool) {
#if MULTITHREADING
#ifdef _WINDOWS
        CachedSequenceMatcher::process_pool = ThreadPool(None);
#else
        // maxtasksperchild is new in Python 2.7; this is for 2.6 compat
        try {
            CachedSequenceMatcher::process_pool = Pool(
                None, matchers.init_worker, maxtasksperchild=1);
        } catch (TypeError &e) {
            CachedSequenceMatcher::process_pool = Pool(
                None, matchers.init_worker);
        }
#endif
#endif
    }
    this->cache.clear();
}

void CachedSequenceMatcher::match(Glib::ustring text1, Glib::ustring textn, std::function<void(difflib::chunk_list_t)> cb) {
    if (this->cache.count(std::pair<Glib::ustring, Glib::ustring>(text1, textn)) > 0) {
        this->cache[std::pair<Glib::ustring, Glib::ustring>(text1, textn)].second = time(0);
        // FIXME: This idle should be totally unnecessary, and yet nothing
        // gets highlighted without it, even though everything in the
        // callback appears to run identically.
#if MULTITHREADING
        GLib.idle_add(lambda: cb(this->cache[std::pair<Glib::ustring, Glib::ustring>(text1, textn)].first));
#else
        cb(this->cache[std::pair<Glib::ustring, Glib::ustring>(text1, textn)].first);
    #endif
    } else {
#if MULTITHREADING
        def inline_cb(opcodes):
            this->cache[(text1, textn)] = [opcodes, time.time()]
            GLib.idle_add(lambda: cb(opcodes))
        this->process_pool.apply_async(matchers.matcher_worker,
                                      (text1, textn),
                                      callback=inline_cb);
#else
        difflib::chunk_list_t opcodes = matcher_worker(text1, textn);
        this->cache[std::pair<Glib::ustring, Glib::ustring>(text1, textn)] = std::pair<difflib::chunk_list_t, time_t>(opcodes, time(0));
        cb(opcodes);
#endif
    }
}

void CachedSequenceMatcher::clean(size_t size_hint) {
    if (this->cache.size() < size_hint * 3) {
        return;
    }
#if 0
    items = this->cache.items();
    items.sort(key=lambda it: it[1][1]);
    for (int item : items[:-size_hint * 2]) {
        del this->cache[item[0]];
    }
#endif
}

struct TaskEntry {
    std::string filename;
    int file;
    Glib::RefPtr<MeldBuffer> buf;
    std::vector<Glib::ustring> codec;
    size_t pane;
    bool was_cr;
};

/*! Start up an filediff with num_panes empty contents. */
FileDiff::FileDiff(int num_panes, SchedulerBase& scheduler) : MeldDoc(scheduler, "filediff.ui", "filediff") {

    bind_settings(this, __gsettings_bindings__);

    // Initialize member variables
    this->builder->get_widget_derived("diffmap0", this->diffmap0);
    this->builder->get_widget_derived("diffmap1", this->diffmap1);
    this->diffmap = { this->diffmap0, this->diffmap1 };

    this->builder->get_widget("file_save_button0", this->file_save_button0);
    this->builder->get_widget("file_save_button1", this->file_save_button1);
    this->builder->get_widget("file_save_button2", this->file_save_button2);
    this->file_save_button = { this->file_save_button0, this->file_save_button1, this->file_save_button2 };

    this->builder->get_widget("file_toolbar0", this->file_toolbar0);
    this->builder->get_widget("file_toolbar1", this->file_toolbar1);
    this->builder->get_widget("file_toolbar2", this->file_toolbar2);
    this->file_toolbar = { this->file_toolbar0, this->file_toolbar1, this->file_toolbar2 };

    this->builder->get_widget("fileentry0", this->fileentry0);
    this->builder->get_widget("fileentry1", this->fileentry1);
    this->builder->get_widget("fileentry2", this->fileentry2);
    this->fileentry = { this->fileentry0, this->fileentry1, this->fileentry2 };

    this->builder->get_widget_derived("linkmap0", this->linkmap0);
    this->builder->get_widget_derived("linkmap1", this->linkmap1);
    this->linkmap = { this->linkmap0, this->linkmap1 };

    this->builder->get_widget_derived("msgarea_mgr0", this->msgarea_mgr0);
    this->builder->get_widget_derived("msgarea_mgr1", this->msgarea_mgr1);
    this->builder->get_widget_derived("msgarea_mgr2", this->msgarea_mgr2);
    this->msgarea_mgr = { this->msgarea_mgr0, this->msgarea_mgr1, this->msgarea_mgr2 };

    this->builder->get_widget("readonlytoggle0", this->readonlytoggle0);
    this->builder->get_widget("readonlytoggle1", this->readonlytoggle1);
    this->builder->get_widget("readonlytoggle2", this->readonlytoggle2);
    this->readonlytoggle = { this->readonlytoggle0, this->readonlytoggle1, this->readonlytoggle2 };

    this->builder->get_widget("scrolledwindow0", this->scrolledwindow0);
    this->builder->get_widget("scrolledwindow1", this->scrolledwindow1);
    this->builder->get_widget("scrolledwindow2", this->scrolledwindow2);
    this->scrolledwindow = { this->scrolledwindow0, this->scrolledwindow1, this->scrolledwindow2 };

    this->builder->get_widget_derived("textview0", this->textview0);
    this->builder->get_widget_derived("textview1", this->textview1);
    this->builder->get_widget_derived("textview2", this->textview2);
    this->textview = { this->textview0, this->textview1, this->textview2 };

    this->builder->get_widget("vbox0", this->vbox0);
    this->builder->get_widget("vbox1", this->vbox1);
    this->builder->get_widget("vbox2", this->vbox2);
    this->vbox = { this->vbox0, this->vbox1, this->vbox2 };

    this->builder->get_widget("dummy_toolbar_linkmap0", this->dummy_toolbar_linkmap0);
    this->builder->get_widget("dummy_toolbar_linkmap1", this->dummy_toolbar_linkmap1);
    this->dummy_toolbar_linkmap = { this->dummy_toolbar_linkmap0, this->dummy_toolbar_linkmap1 };

    this->builder->get_widget("filelabel_toolitem0", this->filelabel_toolitem0);
    this->builder->get_widget("filelabel_toolitem1", this->filelabel_toolitem1);
    this->builder->get_widget("filelabel_toolitem2", this->filelabel_toolitem2);
    this->filelabel_toolitem = { this->filelabel_toolitem0, this->filelabel_toolitem1, this->filelabel_toolitem2 };

    this->builder->get_widget("filelabel0", this->filelabel0);
    this->builder->get_widget("filelabel1", this->filelabel1);
    this->builder->get_widget("filelabel2", this->filelabel2);
    this->filelabel = { this->filelabel0, this->filelabel1, this->filelabel2 };

    this->builder->get_widget("fileentry_toolitem0", this->fileentry_toolitem0);
    this->builder->get_widget("fileentry_toolitem1", this->fileentry_toolitem1);
    this->builder->get_widget("fileentry_toolitem2", this->fileentry_toolitem2);
    this->fileentry_toolitem = { this->fileentry_toolitem0, this->fileentry_toolitem1, this->fileentry_toolitem2 };

    this->builder->get_widget("dummy_toolbar_diffmap0", this->dummy_toolbar_diffmap0);
    this->builder->get_widget("dummy_toolbar_diffmap1", this->dummy_toolbar_diffmap1);
    this->fileentry_toolitem = { this->dummy_toolbar_diffmap0, this->dummy_toolbar_diffmap1 };

    this->builder->get_widget_derived("grid", this->grid);
    this->builder->get_widget_derived("statusbar", this->statusbar);

    // Hook up signals
#if 0
    this->container->signal_destroy().connect(sigc::mem_fun(this, &FileDiff::on_delete_event));
#endif
    this->widget->signal_key_press_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->widget->signal_key_release_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->readonlytoggle2->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_readonly_button_toggled), this->readonlytoggle2));
    this->file_save_button2->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_file_save_button_clicked), this->file_save_button2));
    this->fileentry2->signal_file_set().connect(sigc::mem_fun(this, &FileDiff::on_fileentry_file_set));
    this->readonlytoggle1->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_readonly_button_toggled), this->readonlytoggle1));
    this->file_save_button1->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_file_save_button_clicked), this->file_save_button1));
    this->fileentry1->signal_file_set().connect(sigc::mem_fun(this, &FileDiff::on_fileentry_file_set));
    this->readonlytoggle0->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_readonly_button_toggled), this->readonlytoggle0));
    this->file_save_button0->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_file_save_button_clicked), this->file_save_button0));
    this->fileentry0->signal_file_set().connect(sigc::mem_fun(this, &FileDiff::on_fileentry_file_set));
    this->scrolledwindow0->signal_size_allocate().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_scrolledwindow_size_allocate), this->scrolledwindow0));
    this->textview0->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_button_press_event), this->textview0));
    this->textview0->signal_draw().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_draw), this->textview0));
    this->textview0->signal_key_release_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview0->signal_key_press_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview0->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_focus_in_event), this->textview0));
    this->textview0->signal_focus_out_event().connect(sigc::mem_fun(this, &FileDiff::on_textview_focus_out_event));
    this->textview0->signal_popup_menu().connect(sigc::mem_fun(this, &FileDiff::on_textview_popup_menu));
    this->scrolledwindow2->signal_size_allocate().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_scrolledwindow_size_allocate), this->scrolledwindow2));
    this->textview2->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_button_press_event), this->textview2));
    this->textview2->signal_draw().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_draw), this->textview2));
    this->textview2->signal_key_release_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview2->signal_key_press_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview2->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_focus_in_event), this->textview2));
    this->textview2->signal_focus_out_event().connect(sigc::mem_fun(this, &FileDiff::on_textview_focus_out_event));
    this->textview2->signal_popup_menu().connect(sigc::mem_fun(this, &FileDiff::on_textview_popup_menu));
    this->scrolledwindow1->signal_size_allocate().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_scrolledwindow_size_allocate), this->scrolledwindow1));
    this->textview1->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_button_press_event), this->textview1));
    this->textview1->signal_draw().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_draw), this->textview1));
    this->textview1->signal_key_release_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview1->signal_key_press_event().connect(sigc::mem_fun(this, &FileDiff::on_key_event));
    this->textview1->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_focus_in_event), this->textview1));
    this->textview1->signal_focus_out_event().connect(sigc::mem_fun(this, &FileDiff::on_textview_focus_out_event));
    this->textview1->signal_popup_menu().connect(sigc::mem_fun(this, &FileDiff::on_textview_popup_menu));

    // This SizeGroup isn't actually necessary for FileDiff; it's for
    // handling non-homogenous selectors in FileComp. It's also fragile.
    Glib::RefPtr<Gtk::SizeGroup> column_sizes = Gtk::SizeGroup::create(Gtk::SizeGroupMode::SIZE_GROUP_HORIZONTAL);
    column_sizes->set_ignore_hidden(true);

    this->warned_bad_comparison = false;
    for (MeldSourceView* v : this->textview) {
        Glib::RefPtr<MeldBuffer> buf(new MeldBuffer());
        buf->signal_begin_user_action().connect(sigc::mem_fun(this, &FileDiff::on_textbuffer_begin_user_action));
        buf->signal_end_user_action().connect(sigc::mem_fun(this, &FileDiff::on_textbuffer_end_user_action));
        v->set_buffer(buf);
        buf->data->signal_file_changed().connect(sigc::mem_fun(this, &FileDiff::notify_file_changed));
    }
    this->keymask = 0;
    this->load_font();
    this->deleted_lines_pending = -1;
    this->textview_overwrite = 0;
    this->focus_pane = 0;
    for (MeldSourceView* t : this->textview) {
        this->textview_overwrite_handlers.push_back(t->signal_toggle_overwrite().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_toggle_overwrite), t)));
        Glib::RefPtr<MeldBuffer> buf = Glib::RefPtr<MeldBuffer>::cast_static(t->get_buffer());
        this->textbuffer.push_back(buf);
        this->buffer_texts.push_back(new BufferLines(buf));
        this->buffer_filtered.push_back(new BufferLines(buf, std::bind(&FileDiff::_filter_text, this, std::placeholders::_1)));
    }
    this->undosequence = new UndoSequence();
    this->create_text_filters();
    this->settings_handlers = {
        meldsettings->signal_text_filters_changed().connect(sigc::mem_fun(this, &FileDiff::on_text_filters_changed))
    };
    for (size_t i = 0; i < this->scrolledwindow.size(); i++) {
        Gtk::ScrolledWindow* w = this->scrolledwindow[i];
        w->get_vadjustment()->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::_sync_vscroll), w->get_vadjustment(), i));
        w->get_hadjustment()->signal_value_changed().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::_sync_hscroll), w->get_hadjustment()));
    }
    this->_connect_buffer_handlers();
    this->_sync_vscroll_lock = false;
    this->_sync_hscroll_lock = false;
    this->_scroll_lock = false;
    this->linediffer = new _Differ();
    this->force_highlight = false;
    this->in_nested_textview_gutter_expose = false;
    this->_cached_match = new CachedSequenceMatcher();
    for (Glib::RefPtr<Gtk::TextBuffer> buf : this->textbuffer) {
        this->anim_source_id.push_back(-1);
        this->animating_chunks.push_back(std::vector<TextviewLineAnimation*>());
        buf->create_tag("inline");
#if 0
        buf->connect("notify::has-selection",
                    this->update_text_actions_sensitivity);
#endif
    }

    this->ui_file = gnomeglade_ui_file("filediff-ui.xml");
    this->actiongroup = Gtk::ActionGroup::create("FilediffPopupActions");
#if 0
    this->actiongroup->set_translation_domain("meld");
#endif

    this->actiongroup->add(Gtk::Action::create("MakePatch", _("Format as Patch..."), _("Create a patch using differences between files")), sigc::mem_fun(this, &FileDiff::make_patch));
    Glib::RefPtr<Gtk::Action> action_save_all = Gtk::Action::create("SaveAll", _("Save A_ll"), _("Save all files in the current comparison"));
    action_save_all->set_accel_path("<Ctrl><Shift>L");
    this->actiongroup->add(action_save_all, sigc::mem_fun(this, &FileDiff::on_save_all_activate));
    this->actiongroup->add(Gtk::Action::create("Revert", Gtk::Stock::REVERT_TO_SAVED, "", _("Revert files to their saved versions")), sigc::mem_fun(this, &FileDiff::on_revert_activate));
    this->actiongroup->add(Gtk::Action::create("SplitAdd", _("Add Synchronization Point"), _("Add a manual point for synchronization of changes between files")), sigc::mem_fun(this, &FileDiff::add_sync_point));
    this->actiongroup->add(Gtk::Action::create("SplitClear", _("Clear Synchronization Points"), _("Clear manual change sychronization points")), sigc::mem_fun(this, &FileDiff::clear_sync_points));
    Glib::RefPtr<Gtk::Action> action_prev_conflict = Gtk::Action::create("PrevConflict", _("Previous Conflict"), _("Go to the previous conflict"));
    action_prev_conflict->set_accel_path("<Ctrl>I");
    this->actiongroup->add(action_prev_conflict, sigc::mem_fun(this, &FileDiff::on_next_conflict_up));
    Glib::RefPtr<Gtk::Action> action_next_conflict = Gtk::Action::create("NextConflict", _("Next Conflict"), _("Go to the next conflict"));
    action_next_conflict->set_accel_path("<Ctrl>K");
    this->actiongroup->add(action_next_conflict, sigc::mem_fun(this, &FileDiff::on_next_conflict_down));
    Glib::RefPtr<Gtk::Action> action_push_left = Gtk::Action::create("PushLeft", Gtk::Stock::GO_BACK, _("Push to Left"), _("Push current change to the left"));
    action_push_left->set_accel_path("<Alt>Left");
    this->actiongroup->add(action_push_left, sigc::mem_fun(this, &FileDiff::push_change_left));
    Glib::RefPtr<Gtk::Action> action_push_right = Gtk::Action::create("PushRight", Gtk::Stock::GO_FORWARD, _("Push to Right"), _("Push current change to the right"));
    action_push_right->set_accel_path("<Alt>Right");
    this->actiongroup->add(action_push_right, sigc::mem_fun(this, &FileDiff::push_change_right));
    // FIXME: using LAST and FIRST is terrible and unreliable icon abuse
    Glib::RefPtr<Gtk::Action> action_pull_left = Gtk::Action::create("PullLeft", Gtk::Stock::GOTO_LAST, _("Pull from Left"), _("Pull change from the left"));
    action_pull_left->set_accel_path("<Alt><Shift>Right");
    this->actiongroup->add(action_pull_left, sigc::mem_fun(this, &FileDiff::pull_change_left));
    Glib::RefPtr<Gtk::Action> action_pull_right = Gtk::Action::create("PullRight", Gtk::Stock::GOTO_FIRST, _("Pull from Right"), _("Pull change from the right"));
    action_pull_right->set_accel_path("<Alt><Shift>Left");
    this->actiongroup->add(action_pull_right, sigc::mem_fun(this, &FileDiff::pull_change_right));
    Glib::RefPtr<Gtk::Action> action_copy_left_up = Gtk::Action::create("CopyLeftUp", _("Copy Above Left"), _("Copy change above the left chunk"));
    action_copy_left_up->set_accel_path("<Alt>bracketleft");
    this->actiongroup->add(action_copy_left_up, sigc::mem_fun(this, &FileDiff::copy_change_left_up));
    Glib::RefPtr<Gtk::Action> action_copy_left_down = Gtk::Action::create("CopyLeftDown", _("Copy Below Left"), _("Copy change below the left chunk"));
    action_copy_left_down->set_accel_path("<Alt>semicolon");
    this->actiongroup->add(action_copy_left_down, sigc::mem_fun(this, &FileDiff::copy_change_left_down));
    Glib::RefPtr<Gtk::Action> action_copy_right_up = Gtk::Action::create("CopyRightUp", _("Copy Above Right"), _("Copy change above the right chunk"));
    action_copy_right_up->set_accel_path("<Alt>bracketright");
    this->actiongroup->add(action_copy_right_up, sigc::mem_fun(this, &FileDiff::copy_change_right_up));
    Glib::RefPtr<Gtk::Action> action_copy_right_down = Gtk::Action::create("CopyRightDown", _("Copy Below Right"), _("Copy change below the right chunk"));
    action_copy_right_down->set_accel_path("<Alt>quoteright");
    this->actiongroup->add(action_copy_right_down, sigc::mem_fun(this, &FileDiff::copy_change_right_down));
    Glib::RefPtr<Gtk::Action> action_delete = Gtk::Action::create("Delete", Gtk::Stock::DELETE, _("Delete"), _("Delete change"));
    action_delete->set_accel_path("<Alt>Delete");
    this->actiongroup->add(action_delete, sigc::mem_fun(this, &FileDiff::delete_change));
    this->actiongroup->add(Gtk::Action::create("MergeFromLeft", _("Merge All from Left"), _("Merge all non-conflicting changes from the left")), sigc::mem_fun(this, &FileDiff::pull_all_non_conflicting_changes_left));
    this->actiongroup->add(Gtk::Action::create("MergeFromRight", _("Merge All from Right"), _("Merge all non-conflicting changes from the right")), sigc::mem_fun(this, &FileDiff::pull_all_non_conflicting_changes_right));
    this->actiongroup->add(Gtk::Action::create("MergeAll", _("Merge All"), _("Merge all non-conflicting changes from left and right panes")), sigc::mem_fun(this, &FileDiff::merge_all_non_conflicting_changes));
    Glib::RefPtr<Gtk::Action> action_cycle_documents = Gtk::Action::create("CycleDocuments", _("Cycle Through Documents"), _("Move keyboard focus to the next document in this comparison"));
    action_cycle_documents->set_accel_path("<control>Escape");
    this->actiongroup->add(action_cycle_documents, sigc::mem_fun(this, &FileDiff::action_cycle_documents));

    Glib::RefPtr<Gtk::ToggleAction> toggle_action_lock_scrolling = Gtk::ToggleAction::create("LockScrolling", _("Lock Scrolling"), _("Lock scrolling of all panes"));
    toggle_action_lock_scrolling->activate();
    this->actiongroup->add(toggle_action_lock_scrolling, sigc::bind(sigc::mem_fun(this, &FileDiff::on_action_lock_scrolling_toggled), toggle_action_lock_scrolling));

    this->findbar = new FindBar(*this->grid);
    this->grid->attach(*(this->findbar->widget), 1, 2, 5, 1);

    this->set_num_panes(num_panes);

    // Must be called after set_num_panes() because it depends on the value
    this->on_style_updated();
    this->widget->signal_style_updated().connect(sigc::mem_fun(this, &FileDiff::on_style_updated));

    this->cursor = new CursorDetails();
    this->signal_current_diff_changed().connect(sigc::mem_fun(this, &FileDiff::on_current_diff_changed));
    for (MeldSourceView* t : this->textview) {
        t->signal_focus_in_event().connect(sigc::mem_fun(this, &FileDiff::on_current_diff_changed));
        t->signal_focus_out_event().connect(sigc::mem_fun(this, &FileDiff::on_current_diff_changed));
    }
    this->linediffer->signal_diffs_changed().connect(sigc::mem_fun(this, &FileDiff::on_diffs_changed));
    this->undosequence->signal_checkpointed().connect(sigc::mem_fun(this, &FileDiff::on_undo_checkpointed));
    this->signal_next_conflict_changed().connect(sigc::mem_fun(this, &FileDiff::on_next_conflict_changed));

    for (DiffMap* diffmap : this->diffmap) {
        this->linediffer->signal_diffs_changed().connect(sigc::mem_fun(diffmap, &DiffMap::on_diffs_changed));
    }

    Gtk::Label *overwrite_label = Gtk::manage(new Gtk::Label());
    overwrite_label->show();
    Gtk::Label* cursor_label = Gtk::manage(new Gtk::Label());
    cursor_label->show();
    this->status_info_labels = {overwrite_label, cursor_label};
    this->statusbar->set_info_box(this->status_info_labels);

    // Prototype implementation

    for (size_t pane = 0; pane < this->textview.size(); pane++) {
        MeldSourceView* t = this->textview[pane];
        // FIXME: set_num_panes will break this good
        if (pane == 0 or (pane == 1 and this->num_panes == 3)) {
            Gtk::TextWindowType window = Gtk::TEXT_WINDOW_RIGHT;
            std::pair<Gtk::TextView*, Gtk::TextView*> views = {this->textview[pane], this->textview[pane + 1]};
            GutterRendererChunkAction* renderer = new GutterRendererChunkAction(pane, pane + 1, views, this, this->linediffer);
            Glib::RefPtr<Gsv::Gutter> gutter = t->get_gutter(window);
#if 0
SIGSEGV
            gutter->insert(renderer, 10);
#endif
        }
        if (pane == 1 || pane == 2) {
            Gtk::TextWindowType window = Gtk::TEXT_WINDOW_LEFT;
            std::pair<Gtk::TextView*, Gtk::TextView*> views = {this->textview[pane], this->textview[pane - 1]};
            GutterRendererChunkAction* renderer = new GutterRendererChunkAction(pane, pane - 1, views, this, this->linediffer);
            Glib::RefPtr<Gsv::Gutter> gutter = t->get_gutter(window);
#if 0
SIGSEGV
            gutter->insert(renderer, 10);
#endif
        }
    }

#if 0
    this->connect("notify::ignore-blank-lines", this->refresh_comparison)
#endif
    meldsettings->signal_changed().connect(sigc::mem_fun(this, &FileDiff::on_setting_changed));
}

FileDiff::~FileDiff() {
}

int FileDiff::get_keymask() {
    return this->keymask;
}

void FileDiff::set_keymask(int value) {
    int mode;
    if (value & MASK_SHIFT) {
        mode = MODE_DELETE;
    } else if (value & MASK_CTRL) {
        mode = MODE_INSERT;
    } else {
        mode = MODE_REPLACE;
    }
    this->keymask = value;
    this->signal_action_mode_changed().emit(mode);
}

bool FileDiff::on_key_event(GdkEventKey* event) {
    GdkKeymap* keymap = gdk_keymap_get_default();

    guint keyval;
    gint group;
    gint lvl;
    GdkModifierType consumed;
    gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode, (GdkModifierType)0, event->group,
                                        &keyval, &group, &lvl, &consumed);

    int mod_key = 0;
    if (this->keylookup.count(keyval) > 0) {
        mod_key = this->keylookup[keyval];
    }
    if (event->type == GDK_KEY_PRESS) {
        this->keymask |= mod_key;
        if (event->keyval == GDK_KEY_Escape) {
            this->findbar->hide();
        }
    } else if (event->type == GDK_KEY_RELEASE) {
        if (event->keyval == GDK_KEY_Return and this->keymask & MASK_SHIFT) {
            this->findbar->start_find_previous(this->focus_pane);
        }
        this->keymask &= ~mod_key;
    }
    return false;
}

Gdk::RGBA lookup(Glib::RefPtr<Gtk::StyleContext> style, const Glib::ustring& name, const Glib::ustring& _default) {
    Gdk::RGBA colour;
    bool found = style->lookup_color(name, colour);
    if (not found) {
        colour.set(_default);
    }
    return colour;
}

void FileDiff::on_style_updated() {
    Glib::RefPtr<Gtk::StyleContext> style = this->widget->get_style_context();


    for (Glib::RefPtr<Gtk::TextBuffer> buf : this->textbuffer) {
        Glib::RefPtr<Gtk::TextTag> tag = buf->get_tag_table()->lookup("inline");
        const Gdk::RGBA value = lookup(style, "inline-bg", "LightSteelBlue2");
        tag->property_background_rgba().set_value(value);
    }

    this->fill_colors.clear();
    this->fill_colors["insert"] =                  lookup(style, "insert-bg", "DarkSeaGreen1");
    this->fill_colors["delete"] =                  lookup(style, "insert-bg", "DarkSeaGreen1"),
    this->fill_colors["conflict"] =                lookup(style, "conflict-bg", "Pink"),
    this->fill_colors["replace"] =                 lookup(style, "replace-bg", "#ddeeff"),
    this->fill_colors["current-chunk-highlight"] = lookup(style, "current-chunk-highlight", "#ffffff");
    this->line_colors["insert"] =                  lookup(style, "insert-outline", "#77f077"),
    this->line_colors["delete"] =                  lookup(style, "insert-outline", "#77f077"),
    this->line_colors["conflict"] =                lookup(style, "conflict-outline", "#f0768b"),
    this->line_colors["replace"] =                 lookup(style, "replace-outline", "#8bbff3");
    this->highlight_color = lookup(style, "current-line-highlight", "#ffff00");
    this->syncpoint_color = lookup(style, "syncpoint-outline", "#555555");

    for (DiffMap* associated : this->diffmap) {
        associated->set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>>(this->fill_colors, this->line_colors));
    }
    for (LinkMap* associated : this->linkmap) {
        associated->set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>>(this->fill_colors, this->line_colors));
    }

    this->queue_draw();
}

void FileDiff::on_focus_change() {
    this->keymask = 0;
}

void FileDiff::on_text_filters_changed() {
    bool relevant_change = this->create_text_filters();
    if (relevant_change) {
        this->refresh_comparison();
    }
}

bool FileDiff::create_text_filters() {
    // In contrast to file filters, ordering of text filters can matter
    std::vector<FilterEntry*> old_active;
    for (FilterEntry* f : this->text_filters) {
        if (f->active) {
            old_active.push_back(f);
        }
    }
    std::vector<FilterEntry*> new_active;
    for (FilterEntry* f : meldsettings->text_filters) {
        if (f->active) {
            new_active.push_back(f);
        }
    }
    bool active_filters_changed = old_active != new_active;

    this->text_filters.clear();
    for (FilterEntry* f : meldsettings->text_filters) {
        this->text_filters.push_back(new FilterEntry(*f));
    }

    return active_filters_changed;
}

void FileDiff::_disconnect_buffer_handlers() {
    for (MeldSourceView* textview : this->textview) {
        textview->set_editable(0);
    }
    for (Glib::RefPtr<MeldBuffer> buf : this->textbuffer) {
        for (sigc::connection h : buf->handlers) {
            h.disconnect();
        }
    }
}

void FileDiff::_connect_buffer_handlers() {
    assert(this->textview.size() == this->textbuffer.size());
    for (unsigned int i = 0; i < this->textview.size(); i++) {
        MeldSourceView* textview = this->textview[i];
        Glib::RefPtr<MeldBuffer> buf = this->textbuffer[i];
        textview->set_editable(buf->data->editable);
    }
    for (Glib::RefPtr<MeldBuffer> buf : this->textbuffer) {
        sigc::connection id0 = buf->signal_insert().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_text_insert_text), buf));
#if 0
        sigc::connection id1 = buf->signal_delete_range().connect(sigc::mem_fun(this, &FileDiff::on_text_delete_range));
#else
        sigc::connection id1;
#endif
        sigc::connection id2 = buf->signal_insert().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::after_text_insert_text), buf), true);
#if 0
        sigc::connection id3 = buf->signal_delete_range().connect(sigc::mem_fun(this, &FileDiff::after_text_delete_range), true);
        sigc::connection id4 = buf->signal_connect("notify::cursor-position", this->on_cursor_position_changed);
#else
        sigc::connection id3;
        sigc::connection id4;
#endif
        buf->handlers = {id0, id1, id2, id3, id4};
    }
}

void FileDiff::on_cursor_position_changed(Glib::RefPtr<Gtk::TextBuffer> buf, int pspec, bool force) {
    int pane = -1;
    for (unsigned int i = 0; i < this->textbuffer.size(); i++) {
        Glib::RefPtr<Gtk::TextBuffer> b = this->textbuffer[i];
        if (buf == b) {
            pane = i;
            break;
        }
    }
    int pos = buf->property_cursor_position().get_value();
    if (pane == this->cursor->pane and pos == this->cursor->pos and not force) {
        return;
    }
    this->cursor->pane = pane;
    this->cursor->pos = pos;

    Gtk::TextBuffer::iterator cursor_it = buf->get_iter_at_offset(pos);
    int offset = cursor_it.get_line_offset();
    int line = cursor_it.get_line();

    std::string insert_overwrite = this->_insert_overwrite_text[this->textview_overwrite];
    boost::format fmt(this->_line_column_text);
    fmt % (line + 1) % (offset + 1);
    std::string line_column = fmt.str();
    this->status_info_labels[0]->set_text(insert_overwrite);
    this->status_info_labels[1]->set_text(line_column);

    if (line != this->cursor->line or force) {
        std::array<int, 3> tmp = this->linediffer->locate_chunk(pane, line);
        int chunk = tmp[0];
        int prev = tmp[1];
        int next_ = tmp[2];
        if (chunk != this->cursor->chunk or force) {
            this->cursor->chunk = chunk;
            this->m_signal_current_diff_changed.emit(nullptr);
        }
        if (prev != this->cursor->prev or next_ != this->cursor->next or force) {
            this->m_signal_next_diff_changed.emit(prev >= 0, next_ >= 0);
        }

        int prev_conflict = -1;
        int next_conflict = -1;
        for (int conflict : this->linediffer->conflicts) {
            if (prev >= 0 and conflict <= prev) {
                prev_conflict = conflict;
            }
            if (next_ >= 0 and conflict >= next_) {
                next_conflict = conflict;
                break;
            }
        }
        if (prev_conflict != this->cursor->prev_conflict or
           next_conflict != this->cursor->next_conflict or force) {
            this->m_signal_next_conflict_changed.emit(prev_conflict >= 0, next_conflict >= 0);
        }

        this->cursor->prev = prev;
        this->cursor->next = next_;
        this->cursor->prev_conflict = prev_conflict;
        this->cursor->next_conflict = next_conflict;
    }
    this->cursor->line = line;
    this->cursor->offset = offset;
}

bool FileDiff::on_current_diff_changed(GdkEventFocus* event) {
    int pane = this->_get_focused_pane();
    int chunk_id = 0;
    if (pane != -1) {
        // While this *should* be redundant, it's possible for focus pane
        // and cursor pane to be different in several situations.
        pane = this->cursor->pane;
        chunk_id = this->cursor->chunk;
    }

    bool push_left;
    bool push_right;
    bool pull_left;
    bool pull_right;
    bool _delete;
    bool copy_left;
    bool copy_right;

    if (pane == -1 or chunk_id < 0) {
        push_left = false;
        push_right = false;
        pull_left = false;
        pull_right = false;
        _delete = false;
        copy_left = false;
        copy_right = false;
    } else {
        push_left = true;
        push_right = true;
        pull_left = true;
        pull_right = true;
        _delete = true;
        copy_left = true;
        copy_right = true;

        // Push and Delete are active if the current pane has something to
        // act on, and the target pane exists and is editable. Pull is
        // sensitive if the source pane has something to get, and the
        // current pane is editable. Copy actions are sensitive if the
        // conditions for push are met, *and* there is some content in the
        // target pane.
        bool editable = this->textview[pane]->get_editable();
        bool editable_left = pane > 0 and this->textview[pane - 1]->get_editable();
        bool editable_right = pane < this->num_panes - 1 and
                         this->textview[pane + 1]->get_editable();
        if (pane == 0 or pane == 2) {
            difflib::chunk_t chunk = this->linediffer->get_chunk(chunk_id, pane);
            bool insert_chunk = std::get<1>(chunk) == std::get<2>(chunk);
            bool delete_chunk = std::get<3>(chunk) == std::get<4>(chunk);
            push_left = editable_left and not insert_chunk;
            push_right = editable_right and not insert_chunk;
            pull_left = pane == 2 and editable and not delete_chunk;
            pull_right = pane == 0 and editable and not delete_chunk;
            _delete = editable and not insert_chunk;
            copy_left = push_left and not delete_chunk;
            copy_right = push_right and not delete_chunk;
        } else if (pane == 1) {
            difflib::chunk_t chunk0 = this->linediffer->get_chunk(chunk_id, 1, 0);
            difflib::chunk_t chunk2 = difflib::EMPTY_CHUNK;
            if (this->num_panes == 3) {
                chunk2 = this->linediffer->get_chunk(chunk_id, 1, 2);
            }
            bool left_mid_exists = chunk0 != difflib::EMPTY_CHUNK and std::get<1>(chunk0) != std::get<2>(chunk0);
            bool left_exists = chunk0 != difflib::EMPTY_CHUNK and std::get<3>(chunk0) != std::get<4>(chunk0);
            bool right_mid_exists = chunk2 != difflib::EMPTY_CHUNK and std::get<1>(chunk2) != std::get<2>(chunk2);
            bool right_exists = chunk2 != difflib::EMPTY_CHUNK and std::get<3>(chunk2) != std::get<4>(chunk2);
            push_left = editable_left and left_mid_exists;
            push_right = editable_right and right_mid_exists;
            pull_left = editable and left_exists;
            pull_right = editable and right_exists;
            _delete = editable and (left_mid_exists or right_mid_exists);
            copy_left = push_left and left_exists;
            copy_right = push_right and right_exists;
        }
    }
    this->actiongroup->get_action("PushLeft")->set_sensitive(push_left);
    this->actiongroup->get_action("PushRight")->set_sensitive(push_right);
    this->actiongroup->get_action("PullLeft")->set_sensitive(pull_left);
    this->actiongroup->get_action("PullRight")->set_sensitive(pull_right);
    this->actiongroup->get_action("Delete")->set_sensitive(_delete);
    this->actiongroup->get_action("CopyLeftUp")->set_sensitive(copy_left);
    this->actiongroup->get_action("CopyLeftDown")->set_sensitive(copy_left);
    this->actiongroup->get_action("CopyRightUp")->set_sensitive(copy_right);
    this->actiongroup->get_action("CopyRightDown")->set_sensitive(copy_right);
    // FIXME: don't queue_draw() on everything... just on what changed
    this->queue_draw();

    return false;
}

void FileDiff::on_next_conflict_changed(bool have_prev, bool have_next) {
    this->actiongroup->get_action("PrevConflict")->set_sensitive(have_prev);
    this->actiongroup->get_action("NextConflict")->set_sensitive(have_next);
}

void FileDiff::on_next_conflict_up() {
    this->on_next_conflict(GDK_SCROLL_UP);
}

void FileDiff::on_next_conflict_down() {
    this->on_next_conflict(GDK_SCROLL_DOWN);
}

void FileDiff::on_next_conflict(GdkScrollDirection direction) {
    int target = 0;
    if (direction == GDK_SCROLL_DOWN) {
        target = this->cursor->next_conflict;
    } else { // direction == Gdk.ScrollDirection.UP
        target = this->cursor->prev_conflict;
    }

    if (!target) {
        return;
    }

    Glib::RefPtr<Gtk::TextBuffer> buf = this->textbuffer[this->cursor->pane];
    difflib::chunk_t chunk = this->linediffer->get_chunk(target, this->cursor->pane);
    buf->place_cursor(buf->get_iter_at_line(std::get<1>(chunk)));
    this->textview[this->cursor->pane]->scroll_to(
        buf->get_insert(), 0.1, 0.5, 0.5);
}

void FileDiff::push_change_left() {
    this->push_change(-1);
}

void FileDiff::push_change_right() {
    this->push_change(1);
}

void FileDiff::push_change(int direction) {
    int src = this->_get_focused_pane();
    int dst = src + direction;
    difflib::chunk_t chunk = this->linediffer->get_chunk(this->cursor->chunk, src, dst);
    assert(src != -1 and this->cursor->chunk);
    assert(dst == 0 || dst == 1 || dst == 2);
    assert(chunk != difflib::EMPTY_CHUNK);
    this->replace_chunk(src, dst, chunk);
}

void FileDiff::pull_change_left() {
    this->pull_change(-1);
}

void FileDiff::pull_change_right() {
    this->pull_change(1);
}

void FileDiff::pull_change(int direction) {
    int dst = this->_get_focused_pane();
    int src = dst + direction;
    difflib::chunk_t chunk = this->linediffer->get_chunk(this->cursor->chunk, src, dst);
    assert(dst != -1 and this->cursor->chunk);
    assert(src == 0 || src == 1 || src == 2);
    assert(chunk != difflib::EMPTY_CHUNK);
    this->replace_chunk(src, dst, chunk);
}

void FileDiff::copy_change_left_up() {
    this->copy_change(-1, -1);
}

void FileDiff::copy_change_left_down() {
    this->copy_change(-1, 1);
}

void FileDiff::copy_change_right_up() {
    this->copy_change(1, -1);
}

void FileDiff::copy_change_right_down() {
    this->copy_change(1, 1);
}

void FileDiff::copy_change(int direction, int copy_direction) {
    int src = this->_get_focused_pane();
    int dst = src + direction;
    difflib::chunk_t chunk = this->linediffer->get_chunk(this->cursor->chunk, src, dst);
    assert(src != -1 and this->cursor->chunk);
    assert(dst == 0 || dst == 1 || dst == 2);
    assert(chunk != difflib::EMPTY_CHUNK);
    bool copy_up;
    if (copy_direction < 0) {
        copy_up = true;
    } else {
        copy_up = false;
    }
    this->copy_chunk(src, dst, chunk, copy_up);
}

void FileDiff::pull_all_non_conflicting_changes_left() {
    this->pull_all_non_conflicting_changes(-1);
}

void FileDiff::pull_all_non_conflicting_changes_right() {
    this->pull_all_non_conflicting_changes(1);
}

void FileDiff::pull_all_non_conflicting_changes(int direction) {
    assert(direction == -1 || direction == 1);
    int dst = this->_get_focused_pane();
    int src = dst + direction;
    assert(src >= 0 && src < this->num_panes);
    Merger merger;
    merger.differ = this->linediffer;
    merger.texts = this->buffer_texts;
    std::string mergedfile = merger.merge_2_files(src, dst);
    this->_sync_vscroll_lock = true;
    this->on_textbuffer_begin_user_action();
    this->textbuffer[dst]->set_text(mergedfile);
    this->on_textbuffer_end_user_action();
    std::function<void()> resync = [this, src] () {
        this->_sync_vscroll_lock = false;
        this->_sync_vscroll(this->scrolledwindow[src]->get_vadjustment(), src);
    };
    this->scheduler.add_task(resync);
}

void FileDiff::merge_all_non_conflicting_changes() {
    int dst = 1;
    Merger merger;
    merger.differ = this->linediffer;
    merger.texts = this->buffer_texts;
    Glib::ustring mergedfile = merger.merge_3_files(false);
    this->_sync_vscroll_lock = true;
    this->on_textbuffer_begin_user_action();
    this->textbuffer[dst]->set_text(mergedfile);
    this->on_textbuffer_end_user_action();
    std::function<void()> resync = [this] () {
        this->_sync_vscroll_lock = false;
        this->_sync_vscroll(this->scrolledwindow[0]->get_vadjustment(), 0);
    };
    this->scheduler.add_task(resync);
}

void FileDiff::delete_change() {
    int pane = this->_get_focused_pane();
    difflib::chunk_t chunk = this->linediffer->get_chunk(this->cursor->chunk, pane);
    assert(pane != -1 and this->cursor->chunk);
    assert(chunk != difflib::EMPTY_CHUNK);
    this->delete_chunk(pane, chunk);
}

/*!
 * Returns the Same chunk that would exist at
 * the given location if we didn't remove Same chunks
 */
difflib::chunk_t FileDiff::_synth_chunk(int pane0, int pane1, int line) {

    // This method is a hack around our existing diffutil data structures;
    // getting rid of the Same chunk removal is difficult, as several places
    // have baked in the assumption of only being given changed blocks.

    Glib::RefPtr<Gtk::TextBuffer> buf0 = this->textbuffer[pane0];
    Glib::RefPtr<Gtk::TextBuffer> buf1 = this->textbuffer[pane1];
    int start0 = 0;
    int end0 = buf0->get_line_count() - 1;
    int start1 = 0;
    int end1 = buf1->get_line_count() - 1;

    // This hack is required when pane0's prev/next chunk doesn't exist
    // (i.e., is Same) between pane0 and pane1.
    difflib::chunk_t prev_chunk0 = difflib::EMPTY_CHUNK;
    difflib::chunk_t prev_chunk1 = difflib::EMPTY_CHUNK;
    difflib::chunk_t next_chunk0 = difflib::EMPTY_CHUNK;
    difflib::chunk_t next_chunk1 = difflib::EMPTY_CHUNK;
    std::array<int, 3> t = this->linediffer->locate_chunk(pane0, line);
    int prev = t[1];
    int next_ = t[2];
    if (prev) {
        while (prev >= 0) {
            prev_chunk0 = this->linediffer->get_chunk(prev, pane0, pane1);
            prev_chunk1 = this->linediffer->get_chunk(prev, pane1, pane0);
            if (prev_chunk0 != difflib::EMPTY_CHUNK && prev_chunk1 != difflib::EMPTY_CHUNK) {
                start0 = std::get<2>(prev_chunk0);
                start1 = std::get<2>(prev_chunk1);
                break;
            }
            prev -= 1;
        }
    }

    if (next_) {
        while (next_ < this->linediffer->diff_count()) {
            next_chunk0 = this->linediffer->get_chunk(next_, pane0, pane1);
            next_chunk1 = this->linediffer->get_chunk(next_, pane1, pane0);
            if (next_chunk0 != difflib::EMPTY_CHUNK && next_chunk1 != difflib::EMPTY_CHUNK) {
                end0 = std::get<1>(next_chunk0);
                end1 = std::get<1>(next_chunk1);
                break;
            }
            next_ += 1;
        }
    }

    return difflib::chunk_t("Same", start0, end0, start1, end1);
}

int FileDiff::_corresponding_chunk_line(int chunk, int line, int pane, int new_pane) {

    Glib::RefPtr<Gtk::TextBuffer> old_buf = this->textbuffer[pane];
    Glib::RefPtr<Gtk::TextBuffer> new_buf = this->textbuffer[new_pane];

    // Special-case cross-pane jumps
    if ((pane == 0 and new_pane == 2) or (pane == 2 and new_pane == 0)) {
        int proxy = this->_corresponding_chunk_line(chunk, line, pane, 1);
        return this->_corresponding_chunk_line(chunk, proxy, 1, new_pane);
    }

    // Either we are currently in a identifiable chunk, or we are in a Same
    // chunk; if we establish the start/end of that chunk in both panes, we
    // can figure out what our new offset should be.
    difflib::chunk_t cur_chunk = difflib::EMPTY_CHUNK;
    if (chunk) {
        cur_chunk = this->linediffer->get_chunk(chunk, pane, new_pane);
    }

    if (cur_chunk == difflib::EMPTY_CHUNK) {
        cur_chunk = this->_synth_chunk(pane, new_pane, line);
    }
    int cur_start = std::get<1>(cur_chunk);
    int cur_end = std::get<2>(cur_chunk);
    int new_start = std::get<3>(cur_chunk);
    int new_end = std::get<4>(cur_chunk);

    // If the new buffer's current cursor is already in the correct chunk,
    // assume that we have in-progress editing, and don't move it.
    Gtk::TextBuffer::iterator cursor_it = new_buf->get_iter_at_mark(new_buf->get_insert());
    int cursor_line = cursor_it.get_line();

    int cursor_chunk = std::get<0>(this->linediffer->locate_chunk(new_pane, cursor_line));
    bool already_in_chunk;
    if (cursor_chunk) {
        already_in_chunk = cursor_chunk == chunk;
    } else {
        difflib::chunk_t cursor_chunk = this->_synth_chunk(pane, new_pane, cursor_line);
        already_in_chunk = std::get<3>(cursor_chunk) == new_start and
                           std::get<4>(cursor_chunk) == new_end;
    }

    int new_line;
    if (already_in_chunk) {
        new_line = cursor_line;
    } else {
        // Guess where to put the cursor: in the same chunk, at about the
        // same place within the chunk, calculated proportionally by line.
        // Insert chunks and one-line chunks are placed at the top.
        float chunk_offset;
        if (cur_end == cur_start) {
            chunk_offset = 0.0;
        } else {
            chunk_offset = (line - cur_start) / float(cur_end - cur_start);
        }
        new_line = new_start + int(chunk_offset * (new_end - new_start));
    }

    return new_line;
}

void FileDiff::action_cycle_documents() {
    int pane = this->_get_focused_pane();
    int new_pane = (pane + 1) % this->num_panes;
    int chunk = this->cursor->chunk;
    int line = this->cursor->line;

    int new_line = this->_corresponding_chunk_line(chunk, line, pane, new_pane);

    Glib::RefPtr<Gtk::TextBuffer> new_buf = this->textbuffer[new_pane];
    this->textview[new_pane]->grab_focus();
    new_buf->place_cursor(new_buf->get_iter_at_line(new_line));
    this->textview[new_pane]->scroll_to(
        new_buf->get_insert(), 0.1, 0.5, 0.5);
}

void FileDiff::_set_external_action_sensitivity() {
    bool have_file = this->focus_pane;
    if (this->main_actiongroup && this->main_actiongroup->get_action("OpenExternal")) {
        this->main_actiongroup->get_action("OpenExternal")->set_sensitive(have_file);
    }
}

bool FileDiff::on_textview_focus_in_event(GdkEventFocus* event, Gtk::TextView* view) {
    this->focus_pane = view;
    this->findbar->textview = view;
    this->on_cursor_position_changed(view->get_buffer(), 0, true);
    this->_set_save_action_sensitivity();
    this->_set_merge_action_sensitivity();
    this->_set_external_action_sensitivity();
    this->update_text_actions_sensitivity();
    return false;
}

bool FileDiff::on_textview_focus_out_event(GdkEventFocus* event) {
    this->_set_merge_action_sensitivity();
    this->_set_external_action_sensitivity();
    return false;
}

void FileDiff::_after_text_modified(Glib::RefPtr<MeldBuffer> buffer, int startline, int sizechange) {
    if (this->num_panes > 1) {
#if 0
        int pane = find(this->textbuffer.begin(), this->textbuffer.end(), &buffer) - this->textbuffer.begin();
#endif
        if (this->linediffer->syncpoints.empty()) {
#if 0
            this->linediffer->change_sequence(pane, startline, sizechange,
                                             this->buffer_filtered);
#endif
        }
        // FIXME: diff-changed signal for the current buffer would be cleaner
        int focused_pane = this->_get_focused_pane();
        if (focused_pane != -1) {
            this->on_cursor_position_changed(this->textbuffer[focused_pane],
                                             0, true);
        }
        this->queue_draw();
    }
}

std::string FileDiff::_filter_text(std::string txt) {

    std::function<std::string(std::ssub_match)> killit = [&] (std::ssub_match sm) {
        if (!sm.matched) {
            return sm.str();
        } else {
#if 0
            assert(m.group().count("\n") == 0);
            if (len(m.groups())) {
                s = m.group();
                for (int g : m.groups()) {
                    if (g) {
                        s = s.replace(g,"");
                    }
                }
                return s;
            } else {
                return "";
            }
#else
            return sm.str();
#endif
        }
    };

    for (FilterEntry* filt : this->text_filters) {
#if 0
        try {
#endif
            if (filt->active) {
                const std::regex re = std::regex("\\s+");
                std::for_each(std::sregex_token_iterator(txt.begin(), txt.end(), re, {0,-1}),
                              std::sregex_token_iterator(),
                              killit);
            }
#if 0
        } catch (AssertionError &e) {
            if (not this->warned_bad_comparison) {
                boost::format fmt(_("Filter '%s' changed the number of lines in the file. "
                                    "Comparison will be incorrect. See the user manual for more details."));
                fmt % filt->label;
                run_dialog(fmt.str());
                this->warned_bad_comparison = true;
            }
            break;
        }
#endif
    }
    return txt;
}

void FileDiff::after_text_insert_text(const Gtk::TextBuffer::iterator& it, const Glib::ustring& newtext, int textlen, Glib::RefPtr<MeldBuffer> buf) {
    Glib::RefPtr<Gtk::TextBuffer::Mark> start_mark = buf->get_mark("insertion-start");
    int starting_at = buf->get_iter_at_mark(start_mark).get_line();
    buf->delete_mark(start_mark);
    int lines_added = it.get_line() - starting_at;
    this->_after_text_modified(buf, starting_at, lines_added);
}

void FileDiff::after_text_delete_range(Glib::RefPtr<MeldBuffer> buffer, Gtk::TextBuffer::iterator it0, Gtk::TextBuffer::iterator it1) {
    int starting_at = it0.get_line();
    assert(this->deleted_lines_pending != -1);
    this->_after_text_modified(buffer, starting_at, -this->deleted_lines_pending);
    this->deleted_lines_pending = -1;
}

void FileDiff::load_font() {
    Glib::RefPtr<Pango::Context> context = this->textview[0]->get_pango_context();
    Pango::FontMetrics metrics = context->get_metrics(meldsettings->font(),
                                       context->get_language());
    int line_height_points = metrics.get_ascent() + metrics.get_descent();
    this->pixels_per_line = line_height_points; // 1024
    for (int i = 0; i < 3; i++) {
        this->textview[i]->override_font(meldsettings->font());
    }
    for (int i = 0; i < 2; i++) {
        this->linkmap[i]->queue_draw();
    }
}

void FileDiff::on_setting_changed(const Glib::ustring& key) {
    if (key == "font") {
        this->load_font();
    }
}

Gtk::ResponseType FileDiff::check_save_modified(Glib::ustring label) {
    Gtk::ResponseType response = Gtk::RESPONSE_OK;
    std::vector<bool> modified;
    for (Glib::RefPtr<MeldBuffer> b : this->textbuffer) {
        modified.push_back(b->data->modified);
    }
    if (std::find(modified.begin(), modified.end(), true) != modified.end()) {
        Component dialog("filediff.ui", "check_save_dialog");
        Gtk::MessageDialog* message_dialog = ((Gtk::MessageDialog*) dialog.widget);
        message_dialog->set_transient_for(*(static_cast<Gtk::Window*>(this->widget->get_toplevel())));
        if (!label.empty()) {
            Glib::Value<Glib::ustring> value;
            value.init(Glib::Value<Glib::ustring>::value_type());
            value.set(label);
            message_dialog->set_property_value("text", value);
        }
        // FIXME: Should be packed into dialog.widget.get_message_area(),
        // but this is unbound on currently required PyGTK.
        std::vector<Gtk::CheckButton*> buttons;
        Gtk::VBox* extra_vbox = nullptr;
        dialog.builder->get_widget("extra_vbox", extra_vbox);
        for (int i = 0; i < this->num_panes; i++) {
            Gtk::CheckButton* button = Gtk::manage(new Gtk::CheckButton(this->textbuffer[i]->data->label()));
            button->set_use_underline(false);
            button->set_sensitive(modified[i]);
            button->set_active(modified[i]);
            extra_vbox->pack_start(*button, true, true, 0);
            buttons.push_back(button);
        }
        extra_vbox->show_all();
        int response = message_dialog->run();
        std::vector<bool> try_save;
        for (Gtk::CheckButton* b : buttons) {
            try_save.push_back(b->get_active());
        }
        delete extra_vbox;
        delete message_dialog;
        if (response == Gtk::RESPONSE_OK) {
            for (int i = 0; i < this->num_panes; i++) {
                if (try_save[i]) {
                    if (not this->save_file(i)) {
                        return Gtk::RESPONSE_CANCEL;
                    }
                }
            }
        } else if (response == Gtk::RESPONSE_DELETE_EVENT) {
            response = Gtk::RESPONSE_CANCEL;
        }
    }

    if (response == Gtk::RESPONSE_OK and !this->meta.empty()) {
        VcView* parent = nullptr;
        if (this->meta.count("parent") > 0) {
            parent = boost::get<VcView*>(this->meta["parent"]);
        }
        bool saved = false;
        if (this->meta.count("middle_saved") > 0) {
            saved = boost::get<bool>(this->meta["middle_saved"]);
        }
        bool prompt_resolve = false;
        if (this->meta.count("prompt_resolve") > 0) {
            prompt_resolve = boost::get<bool>(this->meta["prompt_resolve"]);
        }
        if (prompt_resolve and saved and parent->has_command("resolve")) {
            std::string primary = _("Mark conflict as resolved?");
            std::string secondary = _(
                "If the conflict was resolved successfully, you may mark "
                "it as resolved now.");
            std::map<const Glib::ustring, Gtk::ResponseType> buttons = {
                {_("Cancel"), Gtk::RESPONSE_CANCEL},
                {_("Mark _Resolved"), Gtk::RESPONSE_OK}
            };
            int resolve_response = modal_dialog(
                primary, secondary, buttons, static_cast<Gtk::Window*>(this->widget),
                Gtk::MESSAGE_QUESTION);

            if (resolve_response == Gtk::RESPONSE_OK) {
                std::string conflict_file = this->textbuffer[1]->data->filename();
                parent->command("resolve", {conflict_file});
            }
        }
    }


    return response;
}

Gtk::ResponseType FileDiff::on_delete_event(int appquit) {
    Gtk::ResponseType response = this->check_save_modified();
    if (response == Gtk::RESPONSE_OK) {
        for (sigc::connection h : this->settings_handlers) {
            h.disconnect();
        }
    }
    // TODO: Base the return code on something meaningful for VC tools
    this->signal_close().emit(0);
    return response;
}

//
// text buffer undo/redo
//

void FileDiff::on_undo_activate() {
    if (this->undosequence->can_undo()) {
        this->undosequence->undo();
    }
}

void FileDiff::on_redo_activate() {
    if (this->undosequence->can_redo()) {
        this->undosequence->redo();
    }
}

void FileDiff::on_textbuffer_begin_user_action() {
    this->undosequence->begin_group();
}

void FileDiff::on_textbuffer_end_user_action() {
    this->undosequence->end_group();
}

void FileDiff::on_text_insert_text(const Gtk::TextBuffer::iterator& it, const Glib::ustring& text, int textlen, Glib::RefPtr<MeldBuffer> buf) {
#if 0
    this->undosequence->add_action(
        static_cast<GroupAction *>(new BufferInsertionAction(buf, it.get_offset(), text)));
#endif
    buf->create_mark("insertion-start", it, true);
}

void FileDiff::on_text_delete_range(Gtk::TextBuffer* buf, Gtk::TextBuffer::iterator it0, Gtk::TextBuffer::iterator it1) {
    Glib::ustring text = buf->get_text(it0, it1, false);
    assert(this->deleted_lines_pending == -1);
    this->deleted_lines_pending = it1.get_line() - it0.get_line();
#if 0
    this->undosequence->add_action(
        static_cast<GroupAction *>(new BufferDeletionAction(buf, it0.get_offset(), text)));
#endif
}

void FileDiff::on_undo_checkpointed(Glib::RefPtr<MeldBuffer> buf, bool checkpointed) {
    this->set_buffer_modified(buf, not checkpointed);
}

//
//
//

void FileDiff::open_external() {
    int pane = this->_get_focused_pane();
    if (pane >= 0) {
        Glib::RefPtr<MeldBuffer> tmp = this->textbuffer[pane];
        if (!tmp->data->filename().empty()) {
            int pos = tmp->property_cursor_position().get_value();
            Gtk::TextBuffer::iterator cursor_it = tmp->get_iter_at_offset(pos);
            int line = cursor_it.get_line() + 1;
            this->_open_files({tmp->data->filename()}, line);
        }
    }
}

void FileDiff::update_text_actions_sensitivity() {
    Gtk::TextView* widget = this->focus_pane;
    bool cut, copy, paste;
    if (not widget) {
        cut = false;
        copy = false;
        paste = false;
    } else {
        cut = copy = widget->get_buffer()->get_has_selection();
        // Ideally, this would check whether the clipboard included
        // something pasteable. However, there is no changed signal.
        // widget.get_clipboard(
        //    Gdk.SELECTION_CLIPBOARD).wait_is_text_available()
        paste = widget->get_editable();
    }
    if (this->main_actiongroup) {
        this->main_actiongroup->get_action("Cut")->set_sensitive(cut);
        this->main_actiongroup->get_action("Copy")->set_sensitive(copy);
        this->main_actiongroup->get_action("Paste")->set_sensitive(paste);
    }
}

Glib::ustring FileDiff::get_selected_text() {
    int pane = this->_get_focused_pane();
    if (pane != -1) {
        Glib::RefPtr<Gtk::TextBuffer> buf = this->textbuffer[pane];
        Gtk::TextBuffer::iterator start;
        Gtk::TextBuffer::iterator end;
        bool sel = buf->get_selection_bounds(start, end);
        if (sel) {
            return buf->get_text(start, end, false);
        }
    }
    return "";
}

void FileDiff::on_find_activate() {
    Glib::ustring selected_text = this->get_selected_text();
    this->findbar->start_find(this->focus_pane, selected_text);
    this->keymask = 0;
}

void FileDiff::on_replace_activate() {
    Glib::ustring selected_text = this->get_selected_text();
    this->findbar->start_replace(this->focus_pane, selected_text);
    this->keymask = 0;
}

void FileDiff::on_find_next_activate() {
    this->findbar->start_find_next(this->focus_pane);
}

void FileDiff::on_find_previous_activate() {
    this->findbar->start_find_previous(this->focus_pane);
}

void FileDiff::on_scrolledwindow_size_allocate(Gtk::Allocation& allocation, Gtk::ScrolledWindow* scrolledwindow) {
    int index = -1;
    for (int i = 0; i < this->scrolledwindow.size(); i++) {
        if (this->scrolledwindow[i] == scrolledwindow) {
            index = i;
            break;
        }
    }
    if (index == 0 or index == 1) {
        this->linkmap[0]->queue_draw();
    }
    if (index == 1 or index == 2) {
        this->linkmap[1]->queue_draw();
    }
}

bool FileDiff::on_textview_popup_menu() {
#if 0
    this->popup_menu->popup(None, None, None, None, 0,
                          Gtk.get_current_event_time());
#endif
    return true;
}

bool FileDiff::on_textview_button_press_event(GdkEventButton* event, MeldSourceView* textview) {
    if (event->button == 3) {
        textview->grab_focus();
        this->popup_menu->popup(0, event->button); // TODO: egore: event->time was passed as the last argument
        return true;
    }
    return false;
}

void FileDiff::on_textview_toggle_overwrite(MeldSourceView* view) {
    this->textview_overwrite = not this->textview_overwrite;
    for (size_t i = 0; i < this->textview.size(); i++) {
        MeldSourceView* v = this->textview[i];
        sigc::connection h = this->textview_overwrite_handlers[i];
        h.disconnect();
        if (v != view) {
            v->signal_toggle_overwrite().emit();
        }
    }
    this->textview_overwrite_handlers.clear();
    for (MeldSourceView* t : this->textview) {
        sigc::connection tmp = t->signal_toggle_overwrite().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_textview_toggle_overwrite), t));
        this->textview_overwrite_handlers.push_back(tmp);
    }
    this->on_cursor_position_changed(view->get_buffer(), 0, true);
}

//
// text buffer loading/saving
//

void FileDiff::set_labels(std::vector<Glib::ustring> labels) {
    for (size_t i = 0; i < this->textbuffer.size(); i++) {
        Glib::ustring label = labels[i];
        Glib::RefPtr<MeldBuffer> buf = this->textbuffer[i];
        if (!label.empty()) {
            buf->data->label(label);
        }
    }
}

void FileDiff::set_merge_output_file(Glib::ustring filename) {
    if (this->textbuffer.size() < 2) {
        return;
    }
    Glib::RefPtr<MeldBuffer> buf = this->textbuffer[1];
    buf->data->savefile = boost::filesystem::absolute(boost::filesystem::path(filename)).string();
    buf->data->label(filename);
    bool writable = true;
    if (boost::filesystem::exists(buf->data->savefile)) {
        boost::filesystem::path p(buf->data->savefile);
        boost::filesystem::file_status s = status(p);
        writable = (s.permissions() & (boost::filesystem::perms::owner_write | boost::filesystem::perms::group_write | boost::filesystem::perms::others_write)) != 0;
    }
    this->set_buffer_writable(buf, writable);
    this->fileentry[1]->set_filename(boost::filesystem::path(filename).string());
    this->recompute_label();
}

void FileDiff::_set_save_action_sensitivity() {
    int pane = this->_get_focused_pane();
    bool modified;
    if (pane == -1) {
        modified = false;
    } else {
        modified = this->textbuffer[pane]->data->modified;
    }
    if (this->main_actiongroup) {
        this->main_actiongroup->get_action("Save")->set_sensitive(modified);
    }
    bool any_modified = false;
    for (Glib::RefPtr<MeldBuffer> b : this->textbuffer) {
        if (b->data->modified) {
            any_modified = true;
            break;
        }
    }
    this->actiongroup->get_action("SaveAll")->set_sensitive(any_modified);
}

void FileDiff::recompute_label() {
    this->_set_save_action_sensitivity();
    std::vector<std::string> filenames;
    for (int i = 0; i < this->num_panes; i++) {
        Glib::RefPtr<MeldBuffer> b = this->textbuffer[i];
        filenames.push_back(b->data->label());
    }
    std::vector<std::string> shortnames = shorten_names(filenames);
    for (int i = 0; i < this->num_panes; i++) {
        Glib::RefPtr<MeldBuffer> buf = this->textbuffer[i];
        if (buf->data->modified) {
            shortnames[i] += "*";
        }
        this->file_save_button[i]->set_sensitive(buf->data->modified);
        if (buf->data->writable) {
            this->file_save_button[i]->set_stock_id(Gtk::Stock::SAVE);
        } else {
            this->file_save_button[i]->set_stock_id(Gtk::Stock::SAVE_AS);
        }
    }

    std::string label;
    if (this->meta.count("tablabel") > 0) {
        label = boost::get<std::string>(this->meta["tablabel"]);
    }

    if (!label.empty()) {
        this->label_text = label;
    } else {
        this->label_text = boost::join(shortnames, " — ");
    }
    this->tooltip_text = this->label_text;
    this->label_changed();
}

/*!
 * Set num panes to len(files) and load each file given.
 * If an element is None, the text of a pane is left as is.
 */
void FileDiff::set_files(std::vector<std::string> files) {
    this->_disconnect_buffer_handlers();
    for (size_t i = 0; i < files.size(); i++) {
        std::string f = files[i];
        if (f.empty()) {
            continue;
        }
        std::string absfile = boost::filesystem::path(f).string();
        this->fileentry[i]->set_filename(absfile);
        this->textbuffer[i]->reset_buffer(absfile);
        this->msgarea_mgr[i]->clear();
    }

    this->recompute_label();
    this->textview[files.size() >= 2 ? 1 : 0]->grab_focus();
    this->_connect_buffer_handlers();
    this->scheduler.add_task([this, files] () { this->_set_files_internal(files); });
}

std::pair<std::string, std::vector<std::string>> FileDiff::get_comparison() {
    std::vector<std::string> files;
    for (int i = 0; i < this->num_panes; i++) {
        Glib::RefPtr<MeldBuffer> b = this->textbuffer[i];
        files.push_back(b->data->filename());
    }
    return std::pair<std::string, std::vector<std::string>>(TYPE_FILE, files);
}

void FileDiff::add_dismissable_msg_on_response(int response_id, int pane) {
    this->msgarea_mgr[pane]->clear();
}

Gtk::InfoBar* FileDiff::add_dismissable_msg(int pane, const Gtk::BuiltinStockID icon, std::string primary, std::string secondary) {
    Gtk::InfoBar* msgarea = this->msgarea_mgr[pane]->new_from_text_and_icon(
                    icon, primary, secondary);
    msgarea->add_button(_("Hi_de"), Gtk::RESPONSE_CLOSE);
    msgarea->signal_response().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::add_dismissable_msg_on_response), pane));
    msgarea->show_all();
    return msgarea;
}

void FileDiff::_load_files(std::vector<std::string> files, std::vector<Glib::RefPtr<MeldBuffer>> textbuffers) {
    this->undosequence->clear();
#if 0
    yield _("[%s] Set num panes") % this->label_text;
#endif
    this->set_num_panes(files.size());
    this->_disconnect_buffer_handlers();
    this->linediffer->clear();
    this->queue_draw();
    Glib::Variant<std::vector<Glib::ustring>> try_codecs;
    settings->get_value("detect-encodings", try_codecs);
#if 0
    try_codecs.append('latin1')
    yield _("[%s] Opening files") % this->label_text;
#endif
    std::vector<TaskEntry> tasks;

    for (size_t pane = 0; pane < files.size(); pane++) {
        std::string filename = files[pane];
        Glib::RefPtr<MeldBuffer> buf = textbuffers[pane];
        if (!filename.empty()) {
#if 0
            try {
                handle = io.open(filename, "r", encoding=try_codecs[0]);
#else
                int handle = 0;
#endif
                TaskEntry task = {filename, handle, buf, try_codecs.get(), pane, false};
                tasks.push_back(task);
#if 0
            } catch (IOError &e) {
                buf.delete(*buf.get_bounds())
                add_dismissable_msg(pane, Gtk.STOCK_DIALOG_ERROR,
                                    _("Could not read file"), str(e))
            } catch (LookupError &e) {
                buf.delete(*buf.get_bounds())
                add_dismissable_msg(pane, Gtk.STOCK_DIALOG_ERROR,
                                    _("Could not read file"), str(e))
            }
#endif
        }
    }
#if 0
    yield _("[%s] Reading files") % this->label_text
#endif
#if 0
    while (!tasks.empty()) {
        for (TaskEntry t : tasks) {
            std::string nextbit;
            try {
                nextbit = t.file.read(4096);
                if (nextbit.find("\x00") != -1) {
                    Gtk::TextBuffer::iterator begin, end;
                    t.buf->get_bounds(begin, end);
                    t.buf->erase(begin, end);
                    Glib::ustring filename = Glib::Markup::escape_text(t.filename);
                    boost::format fmt(_("%s appears to be a binary file."));
                    fmt % filename;
                    add_dismissable_msg(t.pane, Gtk::Stock::DIALOG_ERROR,
                                        _("Could not read file"), fmt.str());
                    tasks.erase(std::find(tasks.begin(), tasks.end(), t));
                    continue;
                }
            } catch (ValueError &err) {
                t.codec.erase(t.codec.begin());
                if (t.codec.size() > 0) {
                    Gtk::TextBuffer::iterator begin, end;
                    t.buf->get_bounds(begin, end);
                    t.buf->erase(begin, end);
                    t.file = io.open(t.filename, "r", encoding=t.codec[0]);
                } else {
                    Gtk::TextBuffer::iterator begin, end;
                    t.buf->get_bounds(begin, end);
                    t.buf->erase(begin, end);
                    Glib::ustring filename = Glib::Markup::escape_text(t.filename);
                    boost::format fmt(_("%s is not in encodings: %s"));
                    fmt % filename % try_codecs;
                    add_dismissable_msg(t.pane, Gtk::Stock::DIALOG_ERROR,
                                        _("Could not read file"), fmt.str());
                    tasks.erase(std::find(tasks.begin(), tasks.end(), t));
                }
            } catch (IOError &ioerr) {
                add_dismissable_msg(t.pane, Gtk::Stock::DIALOG_ERROR,
                                _("Could not read file"), ioerr.what());
                tasks.erase(std::find(tasks.begin(), tasks.end(), t));
            }
            // The handling here avoids inserting split CR/LF pairs into
            // GtkTextBuffers; this is relevant only when universal
            // newline support is unavailable or broken.
            if (t.was_cr) {
                nextbit = "\r" + nextbit;
                t.was_cr = false;
            }
            if (nextbit.length() > 0) {
                if (boost::algorithm::ends_with(nextbit, "\r") and nextbit.length() > 1) {
                    t.was_cr = true;
                    nextbit = nextbit.substr(0, nextbit.length() - 1);
                }
                t.buf->insert(t.buf->end(), nextbit);
            } else {
                bool writable = false;
                if (!t.buf->data->savefile.empty()) {
                    writable = true;
                    if (boost::filesystem::exists(t.buf->data->savefile)) {
                        boost::filesystem::path p(t.buf->data->savefile);
                        boost::filesystem::file_status s = status(p);
                        writable = (s.permissions() & (boost::filesystem::perms::owner_write | boost::filesystem::perms::group_write | boost::filesystem::perms::others_write)) != 0;
                    }
                } else {
                    boost::filesystem::path p(t.filename);
                    boost::filesystem::file_status s = status(p);
                    writable = (s.permissions() & (boost::filesystem::perms::owner_write | boost::filesystem::perms::group_write | boost::filesystem::perms::others_write)) != 0;
                }
                this->set_buffer_writable(t.buf, writable);
                t.buf->data->encoding = t.codec[0];
#if 0
                if (hasattr(t.file, "newlines")) {
                    t.buf->data->newlines = t.file->newlines;
                }
#endif
                tasks.erase(std::find(tasks.begin(), tasks.end(), t));
            }
        }
#if 0
        yield 1;
#endif
    }
#endif
    for (Glib::RefPtr<MeldBuffer> b : this->textbuffer) {
        this->undosequence->checkpoint(b);
        b->data->update_mtime();
    }
}

void FileDiff::_diff_files(bool refresh) {
#if 0
    yield _("[%s] Computing differences") % this->label_text;
#endif
    std::vector<BufferLines*> texts(this->buffer_filtered.begin(), this->buffer_filtered.begin() + this->num_panes);
    this->linediffer->ignore_blanks = settings->get_boolean("ignore-blank-lines");
    this->linediffer->set_sequences_iter(texts);

    if (not refresh) {
        std::array<int, 3> tmp = this->linediffer->locate_chunk(1, 0);
        int chunk = tmp[0];
        int prev = tmp[1];
        int next_ = tmp[2];
        this->cursor->next = chunk;
        if (this->cursor->next) {
            this->cursor->next = next_;
        }
        for (Glib::RefPtr<Gtk::TextBuffer> buf : this->textbuffer) {
            buf->place_cursor(buf->begin());
        }

        if (this->cursor->next) {
            this->scheduler.add_task([this] () { this->next_diff(GDK_SCROLL_DOWN, true); }, true);
        } else {
            Glib::RefPtr<Gtk::TextBuffer> buf;
            if (this->num_panes > 1) {
                buf = this->textbuffer[1];
            } else {
                buf = this->textbuffer[0];
            }
            this->on_cursor_position_changed(buf, 0, true);
        }
    }

    this->queue_draw();
    this->_connect_buffer_handlers();
    this->_set_merge_action_sensitivity();

    std::vector<Glib::RefPtr<Gsv::Language>> langs;
    for (int i = 0; i < this->num_panes; i++) {
        Glib::ustring filename = this->textbuffer[i]->data->filename();
        if (!filename.empty()) {
            langs.push_back(LanguageManager::get_language_from_file(filename));
        } else {
            langs.push_back(Glib::RefPtr<Gsv::Language>());
        }
    }

    // If we have only one identified language then we assume that all of
    // the files are actually of that type.
    std::vector<Glib::RefPtr<Gsv::Language>> real_langs;
    for (Glib::RefPtr<Gsv::Language> l : langs) {
        if (l) {
            real_langs.push_back(l);
        }
    }
#if 0
    if (!real_langs.empty() and real_langs.count(real_langs[0]) == real_langs.size()) {
        langs = (real_langs[0],) * langs.size();
    }
#endif

    for (int i = 0; i < this->num_panes; i++) {
        this->textbuffer[i]->set_language(langs[i]);
    }
}

void FileDiff::_set_files_internal(std::vector<std::string> files) {
    this->_load_files(files, this->textbuffer);
    this->_diff_files();
}

void FileDiff::on_file_changed_response(int /*Gtk::ResponseType*/ response_id, int pane) {
    this->msgarea_mgr[pane]->clear();
    if (response_id == Gtk::RESPONSE_ACCEPT) {
        this->on_revert_activate();
    }
}

void FileDiff::set_meta(std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta) {
    this->meta = meta;

    std::vector<std::string> labels;
    labels = boost::get<std::vector<std::string>>(meta["labels"]);
    if (!labels.empty()) {
        for (size_t i = 0; i < labels.size(); i++) {
            std::string l = labels[i];
            if (!l.empty()) {
                this->filelabel[i]->set_text(l);
                this->filelabel_toolitem[i]->set_visible(true);
                this->fileentry_toolitem[i]->set_visible(false);
            }
        }
    }
}

void FileDiff::notify_file_changed(MeldBufferData* data) {
    int pane = -1;
    for (size_t i = 0; i < this->textbuffer.size(); i++) {
        Glib::RefPtr<MeldBuffer> b = this->textbuffer[i];
        if (b->data == data) {
            pane = i;
            break;
        }
    }
    if (pane < 0) {
        // Notification for unknown buffer
        return;
    }
    Glib::RefPtr<Gio::File> gfile = Gio::File::create_for_path(data->filename());

    boost::format fmt(_("File %s has changed on disk"));
    fmt % gfile->get_parse_name();
    std::string primary = fmt.str();
    std::string secondary = _("Do you want to reload the file?");
    Gtk::InfoBar* msgarea = this->msgarea_mgr[pane]->new_from_text_and_icon(
                    Gtk::Stock::DIALOG_WARNING, primary, secondary);
    msgarea->add_button(_("_Reload"), Gtk::RESPONSE_ACCEPT);
    msgarea->add_button(_("Hi_de"), Gtk::RESPONSE_CLOSE);

    msgarea->signal_response().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_file_changed_response), pane));
    msgarea->show_all();
}

void FileDiff::refresh_comparison() {
    this->_disconnect_buffer_handlers();
    this->linediffer->clear();

    for (Glib::RefPtr<Gtk::TextBuffer> buf : this->textbuffer) {
        Glib::RefPtr<Gtk::TextTag> tag = buf->get_tag_table()->lookup("inline");
        buf->remove_tag(tag, buf->begin(), buf->end());
    }

    this->queue_draw();
    this->scheduler.add_task([this] () { this->_diff_files(true); });
}

void FileDiff::_set_merge_action_sensitivity() {
    int pane = this->_get_focused_pane();
    bool editable;
    std::pair<bool, bool> mergeable;
    if (pane != -1) {
        editable = this->textview[pane]->get_editable();
        mergeable = this->linediffer->has_mergeable_changes(pane);
    } else {
        editable = false;
        mergeable = std::pair<bool, bool>(false, false);
    }
    this->actiongroup->get_action("MergeFromLeft")->set_sensitive(std::get<0>(mergeable) and editable);
    this->actiongroup->get_action("MergeFromRight")->set_sensitive(std::get<1>(mergeable) and editable);
    if (this->num_panes == 3 and this->textview[1]->get_editable()) {
        mergeable = this->linediffer->has_mergeable_changes(1);
    } else {
        mergeable = std::pair<bool, bool>(false, false);
    }
    this->actiongroup->get_action("MergeAll")->set_sensitive(std::get<0>(mergeable) or std::get<1>(mergeable));
}

bool process_matches(difflib::chunk_t match, std::vector<int> offsets) {
    if (std::get<0>(match) != "equal") {
        return true;
    }
    // Always keep matches occurring at the start or end
    bool start_or_end = (
        (std::get<1>(match) == 0 and std::get<3>(match) == 0) or
        (std::get<2>(match) == offsets[0] and std::get<4>(match) == offsets[1]));
    if (start_or_end) {
        return false;
    }
    // Remove equal matches of size less than 3
    bool too_short = ((std::get<2>(match) - std::get<1>(match) < 3) or
                      (std::get<4>(match) - std::get<3>(match) < 3));
    return too_short;
}

void apply_highlight(std::pair<Glib::RefPtr<MeldBuffer>, Glib::RefPtr<MeldBuffer>> bufs,
                     std::pair<Glib::RefPtr<Gtk::TextBuffer::Tag>, Glib::RefPtr<Gtk::TextBuffer::Tag>> tags,
                     std::pair<Glib::RefPtr<Gtk::TextBuffer::Mark>, Glib::RefPtr<Gtk::TextBuffer::Mark>> start_marks,
                     std::pair<Glib::RefPtr<Gtk::TextBuffer::Mark>, Glib::RefPtr<Gtk::TextBuffer::Mark>> end_marks,
                     std::pair<std::string, std::string> texts, std::vector<difflib::chunk_t> matches_) {
    std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> starts = {bufs.first->get_iter_at_mark(start_marks.first),
                                                                              bufs.second->get_iter_at_mark(start_marks.second)};
    std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> ends = {bufs.first->get_iter_at_mark(end_marks.first),
                                                                           bufs.second->get_iter_at_mark(end_marks.second)};
    Glib::ustring text1 = bufs.first->get_text(starts.first, ends.first, false);
    Glib::ustring textn = bufs.second->get_text(starts.second, ends.second, false);

    bufs.first->delete_mark(start_marks.first);
    bufs.first->delete_mark(end_marks.first);
    bufs.second->delete_mark(start_marks.second);
    bufs.second->delete_mark(end_marks.second);

    if (texts != std::pair<std::string, std::string>(text1, textn)) {
        return;
    }

    std::vector<int> offsets = {ends.first.get_offset() - starts.first.get_offset(),
                                ends.second.get_offset() - starts.second.get_offset()};

    std::vector<difflib::chunk_t> matches;
    for (difflib::chunk_t m : matches_) {
        if (process_matches(m, offsets)) {
            matches.push_back(m);
        }
    }

    {
        Gtk::TextBuffer::iterator start = starts.first;
        Gtk::TextBuffer::iterator end = starts.first;
        int offset = start.get_offset();
        for (difflib::chunk_t o : matches) {
            start.set_offset(offset + std::get<1 + 2 * 0>(o));
            end.set_offset(offset + std::get<2 + 2 * 0>(o));
            bufs.first->apply_tag(tags.first, start, end);
        }
    }
    {
        Gtk::TextBuffer::iterator start = starts.second;
        Gtk::TextBuffer::iterator end = starts.second;
        int offset = start.get_offset();
        for (difflib::chunk_t o : matches) {
            start.set_offset(offset + std::get<1 + 2 * 1>(o));
            end.set_offset(offset + std::get<2 + 2 * 1>(o));
            bufs.second->apply_tag(tags.second, start, end);
        }
    }
}

void FileDiff::on_diffs_changed(std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>> chunk_changes) {
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> removed_chunks = std::get<0>(chunk_changes);
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> added_chunks = std::get<1>(chunk_changes);
    std::pair<difflib::chunk_t, difflib::chunk_t> modified_chunks = std::get<2>(chunk_changes);

    // We need to clear removed and modified chunks, and need to
    // re-highlight added and modified chunks.
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> need_clearing(removed_chunks.begin(), removed_chunks.end());
    std::sort(need_clearing.begin(), need_clearing.end());
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> need_highlighting;
    need_highlighting.push_back(modified_chunks);
    std::sort(need_highlighting.begin(), need_highlighting.end());

    std::vector<Glib::RefPtr<Gtk::TextBuffer::Tag>> alltags;
    for (Glib::RefPtr<MeldBuffer> b : this->textbuffer) {
        alltags.push_back(b->get_tag_table()->lookup("inline"));
    }

    for (std::pair<difflib::chunk_t, difflib::chunk_t> chunk : need_clearing) {
        for (int i = 0; i < 2; i++) {
            difflib::chunk_t c = (i == 0) ? chunk.first : chunk.second;
            if (c == difflib::EMPTY_CHUNK or std::get<0>(c) != "replace") {
                continue;
            }
            int to_idx;
            if (i == 1) {
                to_idx = 2;
            } else {
                to_idx = 0;
            }
            std::pair<Glib::RefPtr<MeldBuffer>, Glib::RefPtr<MeldBuffer>> bufs(this->textbuffer[1], this->textbuffer[to_idx]);
            std::pair<Glib::RefPtr<Gtk::TextBuffer::Tag>, Glib::RefPtr<Gtk::TextBuffer::Tag>> tags(alltags[1], alltags[to_idx]);

            std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> starts(bufs.first->get_iter_at_line_or_eof(std::get<1>(c)), bufs.second->get_iter_at_line_or_eof(std::get<3>(c)));
            std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> ends(bufs.first->get_iter_at_line_or_eof(std::get<2>(c)), bufs.second->get_iter_at_line_or_eof(std::get<4>(c)));

            bufs.first->remove_tag(tags.first, starts.first, ends.first);
            bufs.second->remove_tag(tags.second, starts.second, ends.second);
        }
    }

    for (std::pair<difflib::chunk_t, difflib::chunk_t> chunk : need_highlighting) {
        bool clear = chunk == modified_chunks;
        for (int i = 0; i < 2; i++) {
            difflib::chunk_t c;
            if (i == 0) {
                c = chunk.first;
            } else {
                c = chunk.second;
            }
            if (c == difflib::EMPTY_CHUNK or std::get<0>(c) != "replace") {
                continue;
            }
            int to_idx;
            if (i == 1) {
                to_idx = 2;
            } else {
                to_idx = 0;
            }
            std::pair<Glib::RefPtr<MeldBuffer>, Glib::RefPtr<MeldBuffer>> bufs (this->textbuffer[1], this->textbuffer[to_idx]);
            std::pair<Glib::RefPtr<Gtk::TextBuffer::Tag>, Glib::RefPtr<Gtk::TextBuffer::Tag>> tags(alltags[1], alltags[to_idx]);

            std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> starts(bufs.first->get_iter_at_line_or_eof(std::get<1>(c)), bufs.second->get_iter_at_line_or_eof(std::get<3>(c)));
            std::pair<Gtk::TextBuffer::iterator, Gtk::TextBuffer::iterator> ends(bufs.first->get_iter_at_line_or_eof(std::get<2>(c)), bufs.second->get_iter_at_line_or_eof(std::get<4>(c)));

            // We don't use this->buffer_texts here, as removing line
            // breaks messes with inline highlighting in CRLF cases
            Glib::ustring text1 = bufs.first->get_text(starts.first, ends.first, false);
            Glib::ustring textn = bufs.second->get_text(starts.second, ends.second, false);

            // Bail on long sequences, rather than try a slow comparison
            static const int inline_limit = 10000;
            if (text1.size() + textn.size() > inline_limit and
                    not this->force_highlight) {
                bufs.first->apply_tag(tags.first, starts.first, ends.first);
                bufs.second->apply_tag(tags.second, starts.second, ends.second);
                this->_prompt_long_highlighting();
                continue;
            }


            if (clear) {
                bufs.first->remove_tag(tags.first, starts.first, ends.first);
                bufs.second->remove_tag(tags.second, starts.second, ends.second);
            }

            std::pair<Glib::RefPtr<Gtk::TextBuffer::Mark>, Glib::RefPtr<Gtk::TextBuffer::Mark>> starts_(bufs.first->create_mark("", starts.first, true),
                                                                                                        bufs.second->create_mark("", starts.second, true));
            std::pair<Glib::RefPtr<Gtk::TextBuffer::Mark>, Glib::RefPtr<Gtk::TextBuffer::Mark>> ends_(bufs.first->create_mark("", ends.first, true),
                                                                                                      bufs.second->create_mark("", ends.second, true));
            std::function<void(difflib::chunk_list_t)> match_cb = [this, bufs, tags, starts_, ends_, text1, textn] (difflib::chunk_list_t chunks) {
                apply_highlight(bufs, tags, starts_, ends_, std::pair<std::string, std::string>(text1, textn), chunks);
            };
            this->_cached_match->match(text1, textn, match_cb);
        }
    }

    this->_cached_match->clean(this->linediffer->diff_count());

    this->_set_merge_action_sensitivity();
    if (this->linediffer->sequences_identical()) {
        bool error_message = false;
        for (MsgAreaController* m : this->msgarea_mgr) {
            if (m->has_message()) {
                error_message = true;
                break;
            }
        }
        if (this->num_panes == 1 or error_message) {
            return;
        }
        for (size_t index = 0; index < this->msgarea_mgr.size(); index++) {
            MsgAreaController* mgr = this->msgarea_mgr[index];
            Glib::ustring secondary_text = "";
            // TODO: Currently this only checks to see whether text filters
            // are active, and may be altering the comparison. It would be
            // better if we only showed this message if the filters *did*
            // change the text in question.
            bool active_filters = false;
            for (FilterEntry* f : this->text_filters) {
                if (f->active) {
                    active_filters = true;
                }
            }
            if (active_filters) {
                secondary_text = _("Text filters are being used, and may "
                                   "be masking differences between files. "
                                   "Would you like to compare the "
                                   "unfiltered files?");
            }

            Gtk::InfoBar* msgarea = mgr->new_from_text_and_icon(Gtk::Stock::INFO,
                                                 _("Files are identical"),
                                                 secondary_text);
            mgr->set_msg_id(FileDiff::MSG_SAME);
            Gtk::Button* button = msgarea->add_button(_("Hide"), Gtk::RESPONSE_CLOSE);
            if (index == 0) {
                button->property_label() = _("Hi_de");
            }

            if (active_filters) {
                msgarea->add_button(_("Show without filters"),
                                   Gtk::RESPONSE_OK);
            }

            msgarea->signal_response().connect(sigc::mem_fun(this, &FileDiff::on_msgarea_identical_response));
            msgarea->show_all();
        }
    } else {
        for (MsgAreaController* m : this->msgarea_mgr) {
            if (m->get_msg_id() == FileDiff::MSG_SAME) {
                m->clear();
            }
        }
    }
}

void FileDiff::on_msgarea_highlighting_response(int /*Gtk::ResponseType*/ respid) {
    for (MsgAreaController* mgr : this->msgarea_mgr) {
        mgr->clear();
    }
    if (respid == Gtk::RESPONSE_OK) {
        this->force_highlight = true;
        this->refresh_comparison();
    }
}

void FileDiff::_prompt_long_highlighting() {
    for (size_t index = 0; index < this->msgarea_mgr.size(); index++) {
        MsgAreaController* mgr = this->msgarea_mgr[index];
        Gtk::InfoBar* msgarea = mgr->new_from_text_and_icon(
            Gtk::Stock::INFO,
            _("Change highlighting incomplete"),
            _("Some changes were not highlighted because they were too "
              "large. You can force Meld to take longer to highlight "
              "larger changes, though this may be slow."));
        mgr->set_msg_id(FileDiff::MSG_SLOW_HIGHLIGHT);
        Gtk::Button* button = msgarea->add_button(_("Hi_de"), Gtk::RESPONSE_OK);
        if (index == 0) {
            Glib::Value<Glib::ustring> value;
            value.init(Glib::Value<Glib::ustring>::value_type());
            value.set(_("Hi_de"));
            button->set_property_value("label", value);
        }
        button = msgarea->add_button(
            _("Keep highlighting"), Gtk::RESPONSE_OK);
        if (index == 0) {
            Glib::Value<Glib::ustring> value;
            value.init(Glib::Value<Glib::ustring>::value_type());
            value.set(_("_Keep highlighting"));
            button->set_property("label", value);
        }
        msgarea->signal_response().connect(sigc::mem_fun(this, &FileDiff::on_msgarea_highlighting_response));
        msgarea->show_all();
    }
}

void FileDiff::on_msgarea_identical_response(int /*Gtk::ResponseType*/ respid) {
    for (MsgAreaController* mgr : this->msgarea_mgr) {
        mgr->clear();
    }
    if (respid == Gtk::RESPONSE_OK) {
        this->text_filters.clear();
        this->refresh_comparison();
    }
}

int anim_cb(void* userdata) {
    GtkTextView* textview = (GtkTextView*) userdata;
    gtk_widget_queue_draw(GTK_WIDGET(textview));
    return 1;
}

bool FileDiff::on_textview_draw(const Cairo::RefPtr<Cairo::Context>& context, MeldSourceView* textview) {
    if (this->num_panes == 1) {
        return false;
    }

    // FIXME: Update to use gtk_cairo_should_draw_window()

    // if event.window != textview.get_window(Gtk.TextWindowType.TEXT)
    //     and event.window != textview.get_window(Gtk.TextWindowType.LEFT):
    //     return

    // // Hack to redraw the line number gutter used by post-2.10 GtkSourceView
    // if event.window == textview.get_window(Gtk.TextWindowType.LEFT) and
    //    this->in_nested_textview_gutter_expose:
    //     this->in_nested_textview_gutter_expose = False
    //     return

    Gdk::Rectangle visible;
    textview->get_visible_rect(visible);
    int pane = -1;
    for (size_t i = 0; i < this->textview.size(); i++) {
        MeldSourceView* tmp = this->textview[i];
        if (tmp == textview) {
            pane = i;
            break;
        }
    }
    Glib::RefPtr<Gtk::TextBuffer> textbuffer = textview->get_buffer();
    int x, y;
    textview->window_to_buffer_coords(Gtk::TEXT_WINDOW_WIDGET, 0, 0, x, y);
    Gtk::Allocation view_allocation = textview->get_allocation();
    std::pair<int, int> bounds (textview->get_line_num_for_y(y),
                                textview->get_line_num_for_y(y + view_allocation.get_height() + 1));

    int width = view_allocation.get_width();
    int height = view_allocation.get_height();
    context->set_line_width(1.0);

    for (difflib::chunk_t change : this->linediffer->single_changes(pane, bounds)) {
        int ypos0 = textview->get_y_for_line_num(std::get<1>(change)) - visible.get_y();
        int ypos1 = textview->get_y_for_line_num(std::get<2>(change)) - visible.get_y();

        context->rectangle(-0.5, ypos0 - 0.5, width + 1, ypos1 - ypos0);
        if (std::get<1>(change) != std::get<2>(change)) {
            Gdk::RGBA tmp = this->fill_colors[std::get<0>(change)];
            context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
            context->fill_preserve();
            if (std::get<0>(this->linediffer->locate_chunk(pane, std::get<1>(change))) == this->cursor->chunk) {
                Gdk::RGBA highlight = this->fill_colors["current-chunk-highlight"];
                context->set_source_rgba(highlight.get_red(), highlight.get_green(), highlight.get_blue(),
                                         highlight.get_alpha());
                context->fill_preserve();
            }
        }

        Gdk::RGBA tmp = this->line_colors[std::get<0>(change)];
        context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
        context->stroke();
    }

    if (settings->get_boolean("highlight_current_line") and textview->is_focus() and
            this->cursor->line) {
        Gtk::TextBuffer::iterator it = textbuffer->get_iter_at_line(this->cursor->line);
        int ypos, line_height;
        textview->get_line_yrange(it, ypos, line_height);
        context->save();
        context->rectangle(0, ypos - visible.get_y(), width, line_height);
        context->clip();
        Gdk::RGBA tmp = this->highlight_color;
        context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
        context->paint_with_alpha(0.25);
        context->restore();
    }

    for (std::vector<Glib::RefPtr<Gtk::TextBuffer::Mark>> p : this->syncpoints) {
        Glib::RefPtr<Gtk::TextBuffer::Mark> syncpoint = p[pane];
        if (not syncpoint) {
            continue;
        }
        int syncline = textbuffer->get_iter_at_mark(syncpoint).get_line();
        if (std::get<0>(bounds) <= syncline && syncline <= std::get<1>(bounds)) {
            int ypos = textview->get_y_for_line_num(syncline) - visible.get_y();
            context->rectangle(-0.5, ypos - 0.5, width + 1, 1);
            Gdk::RGBA tmp = this->syncpoint_color;
            context->set_source_rgba(tmp.get_red(), tmp.get_green(), tmp.get_blue(), tmp.get_alpha());
            context->stroke();
        }
    }

    std::vector<TextviewLineAnimation*> new_anim_chunks;
    for (TextviewLineAnimation* c : this->animating_chunks[pane]) {
        gint64 current_time = g_get_monotonic_time();
        float percent = std::min(1.0f, (current_time - c->start_time) / float(c->duration));
        Gdk::RGBA rgba;
        rgba.set_rgba(c->start_rgba.get_red()   + (c->end_rgba.get_red()   - c->start_rgba.get_red())   * percent,
                      c->start_rgba.get_green() + (c->end_rgba.get_green() - c->start_rgba.get_green()) * percent,
                      c->start_rgba.get_blue()  + (c->end_rgba.get_blue()  - c->start_rgba.get_blue())  * percent,
                      c->start_rgba.get_alpha() + (c->end_rgba.get_alpha() - c->start_rgba.get_alpha()) * percent);

        Gtk::TextBuffer::iterator it = textbuffer->get_iter_at_mark(c->start_mark);
        int ystart, unused;
        textview->get_line_yrange(it, ystart, unused);
        it = textbuffer->get_iter_at_mark(c->end_mark);
        int yend;
        textview->get_line_yrange(it, yend, unused);
        if (ystart == yend) {
            ystart -= 1;
        }

        context->set_source_rgba(rgba.get_red(), rgba.get_green(), rgba.get_blue(), rgba.get_alpha());
        context->rectangle(0, ystart - visible.get_y(), width, yend - ystart);
        context->fill();

        if (current_time <= c->start_time + c->duration) {
            new_anim_chunks.push_back(c);
        } else {
            textbuffer->delete_mark(c->start_mark);
            textbuffer->delete_mark(c->end_mark);
        }
    }
    this->animating_chunks[pane] = new_anim_chunks;

    if (!this->animating_chunks[pane].empty() and !this->anim_source_id[pane]) {
        // Using timeout_add interferes with recalculation of inline
        // highlighting; this mechanism could be improved.
        this->anim_source_id[pane] = g_idle_add(anim_cb, (gpointer)textbuffer->gobj());
    } else if (this->animating_chunks[pane].empty() and this->anim_source_id[pane]) {
        g_source_remove(this->anim_source_id[pane]);
        this->anim_source_id[pane] = -1;
    }

    // if event.window == textview.get_window(Gtk.TextWindowType.LEFT):
    //     this->in_nested_textview_gutter_expose = True
    //     textview.emit("expose-event", event)
    return false;
}

Glib::ustring FileDiff::_get_filename_for_saving(Glib::ustring title) {
    Gtk::Window* w = static_cast<Gtk::Window*>(this->widget->get_toplevel());
    Gtk::FileChooserDialog* dialog = new Gtk::FileChooserDialog(*w,
                                                                title,
                                                                Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog->set_default_response(Gtk::RESPONSE_OK);
    int response = dialog->run();
    Glib::ustring filename = "";
    if (response == Gtk::RESPONSE_OK) {
        filename = dialog->get_filename();
    }
    delete dialog;
    if (!filename.empty()) {
        if (boost::filesystem::exists(filename.c_str())) {
            boost::format fmt(_("\"%s\" exists!\nOverwrite?"));
            fmt % boost::filesystem::path(filename.c_str()).parent_path().string();
            int response = run_dialog(
                fmt.str(),
                static_cast<Gtk::Window*>(this->widget->get_toplevel()),
                Gtk::MESSAGE_WARNING,
                Gtk::BUTTONS_YES_NO);
            if (response == Gtk::RESPONSE_NO) {
                return "";
            }
        }
        return filename;
    }
    return "";
}

bool FileDiff::_save_text_to_filename(Glib::ustring filename, Glib::ustring text) {
    try {
        std::ofstream f;
        f.exceptions(std::ios::failbit);
        f.open(filename.c_str());
        f << text;
        f.close();
    } catch (const std::exception & e) {
        boost::format fmt(_("Error writing to %s\n\n%s."));
        fmt % filename % e.what();
        run_dialog(
            fmt.str(),
            static_cast<Gtk::Window*>(this->widget->get_toplevel()),
            Gtk::MESSAGE_ERROR,
            Gtk::BUTTONS_OK);
        return false;
    }
    return true;
}

// FIXME egore: check parameter order
void FileDiff::on_file_changed_response_2(int /*Gtk::ResponseType*/ response_id, int pane, bool saveas) {
    this->msgarea_mgr[pane]->clear();
    if (response_id == Gtk::RESPONSE_ACCEPT) {
        this->save_file(pane, saveas, true);
    }
}

bool FileDiff::save_file(int pane, bool saveas, bool force_overwrite) {
    Glib::RefPtr<MeldBuffer> buf = this->textbuffer[pane];
    MeldBufferData* bufdata = buf->data;
    if (saveas or not (!bufdata->filename().empty() or !bufdata->savefile.empty())
            or not bufdata->writable) {
        Glib::ustring prompt;
        if (pane == 0) {
            prompt = _("Save Left Pane As");
        } else if (pane == 1 and this->num_panes == 3) {
            prompt = _("Save Middle Pane As");
        } else {
            prompt = _("Save Right Pane As");
        }
        Glib::ustring filename = this->_get_filename_for_saving(prompt);
        if (!filename.empty()) {
            std::string tmp = boost::filesystem::absolute(boost::filesystem::path(filename)).string();
            bufdata->filename(tmp);
            bufdata->label(tmp);
            bufdata->savefile = nullptr;
            this->fileentry[pane]->set_filename(bufdata->filename());
            this->filelabel_toolitem[pane]->set_visible(false);
            this->fileentry_toolitem[pane]->set_visible(true);
        } else {
            return false;
        }
    }

    if (not force_overwrite and not bufdata->current_on_disk()) {
        Glib::RefPtr<Gio::File> gfile = Gio::File::create_for_path(bufdata->filename());
        boost::format fmt(_("File %s has changed on disk since it was opened"));
        fmt % gfile->get_parse_name();
        Glib::ustring primary = fmt.str();
        Glib::ustring secondary = _("If you save it, any external changes will be lost.");
        Gtk::InfoBar* msgarea = this->msgarea_mgr[pane]->new_from_text_and_icon(
                        Gtk::Stock::DIALOG_WARNING, primary, secondary);
        msgarea->add_button(_("Save Anyway"), Gtk::RESPONSE_ACCEPT);
        msgarea->add_button(_("Don't Save"), Gtk::RESPONSE_CLOSE);

        msgarea->signal_response().connect(sigc::bind(sigc::mem_fun(this, &FileDiff::on_file_changed_response_2), pane, saveas));
        msgarea->show_all();
        return false;
    }


    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    Glib::ustring text = buf->get_text(start, end, false);
    if (!bufdata->newlines.empty()) {
#if 0
        if (isinstance(bufdata->newlines, basestring)) {
            if (bufdata->newlines != '\n') {
                text = text.replace("\n", bufdata.newlines);
            }
        } else {
            static std::map<Glib::ustring, std::pair<Glib::ustring, int>> buttons = {
                {"\n", {"UNIX (LF)", 0}},
                {"\r\n", {"DOS/Windows (CR-LF)", 1}},
                {"\r", {"Mac OS (CR)", 2}}
            };
            boost::format fmt(_("This file '%s' contains a mixture of line endings.\n\nWhich format would you like to use?"));
            fmt % bufdata->label;
            int newline = run_dialog(fmt.str(),
                this, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_CANCEL,
                extrabuttons=[ buttons[b] for b in bufdata.newlines ] );
            if (newline < 0) {
                return false;
            }
            for (std::pair<Glib::ustring, std::pair<Glib::ustring, int>> tmp : buttons.items()) {
                Glib::ustring k = tmp.first;
                std::pair<Glib::ustring, int> v = tmp.second;
                if (v.second == newline) {
                    bufdata->newlines = k;
                    if (k != "\n") {
                        text = text.replace("\n", k);
                    }
                    break;
                }
            }
        }
#endif
    }
#if 0
    if (bufdata->encoding) {
        try {
            text = text.encode(bufdata.encoding);
        } catch (UnicodeEncodeError &e) {
            boost::format fmt(_("'%s' contains characters not encodable with '%s'\nWould you like to save as UTF-8?"));
            fmt  % bufdata->label % bufdata->encoding;
            if (run_dialog(
                fmt.str(),
                this, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_YES_NO) != Gtk::RESPONSE_YES) {
                return false;
            }
        }
    }
#endif

    std::string save_to;
    if (!bufdata->savefile.empty()) {
        save_to = bufdata->savefile;
    } else {
        save_to = bufdata->filename();
    }
    if (this->_save_text_to_filename(save_to, text)) {
        this->m_signal_file_changed.emit(save_to);
        this->undosequence->checkpoint(buf);
        if (pane == 1 and this->num_panes == 3) {
            this->meta["middle_saved"] = true;
        }
        bufdata->update_mtime();
        return true;
    } else {
        return false;
    }
}

void FileDiff::make_patch() {
    PatchDialog dialog(this);
    dialog.run();
}

void FileDiff::set_buffer_writable(Glib::RefPtr<MeldBuffer> buf, bool writable) {
    buf->data->writable = writable;
    this->recompute_label();
    int index = -1;
    for (size_t i = 0; i < this->textbuffer.size(); i++) {
        Glib::RefPtr<MeldBuffer> b = this->textbuffer[i];
        if (b == buf) {
            index = i;
            break;
        }
    }
    this->readonlytoggle[index]->set_visible(not writable);
    this->set_buffer_editable(buf, writable);
}

void FileDiff::set_buffer_modified(Glib::RefPtr<MeldBuffer> buf, bool yesno) {
    buf->data->modified = yesno;
    this->recompute_label();
}

void FileDiff::set_buffer_editable(Glib::RefPtr<MeldBuffer> buf, bool editable) {
    buf->data->editable = editable;
    int index = -1;
    for (size_t i = 0; i < this->textbuffer.size(); i++) {
        Glib::RefPtr<MeldBuffer> b = this->textbuffer[i];
        if (b == buf) {
            index = i;
            break;
        }
    }
    this->readonlytoggle[index]->set_active(not editable);
    this->textview[index]->set_editable(editable);
    this->on_cursor_position_changed(buf, -1, true);
    for (Gtk::DrawingArea* linkmap : this->linkmap) {
        linkmap->queue_draw();
    }
}

void FileDiff::save() {
    int pane = this->_get_focused_pane();
    if (pane >= 0) {
        this->save_file(pane);
    }
}

void FileDiff::save_as() {
    int pane = this->_get_focused_pane();
    if (pane >= 0) {
        this->save_file(pane, true);
    }
}

void FileDiff::on_save_all_activate() {
    for (int i = 0; i < this->num_panes; i++) {
        if (this->textbuffer[i]->data->modified) {
            this->save_file(i);
        }
    }
}

void FileDiff::on_file_save_button_clicked(Gtk::ToolButton* button) {
    int idx = -1;
    for (size_t i = 0; i < this->file_save_button.size(); i++) {
        Gtk::ToolButton* b = this->file_save_button[i];
        if (b == button) {
            idx = i;
            break;
        }
    }
    this->save_file(idx);
}

void FileDiff::on_fileentry_file_set() {
    if (this->check_save_modified() != Gtk::RESPONSE_CANCEL) {
        std::vector<Glib::RefPtr<const Gio::File>> files;
        for (int i = 0; i < this->num_panes; i++) {
            Gtk::FileChooserButton* e = this->fileentry[i];
            files.push_back(e->get_file());
        }
        std::vector<std::string> paths;
        for (Glib::RefPtr<const Gio::File> f : files) {
            if (f) {
                paths.push_back(f->get_path());
            } else {
                paths.push_back("");
            }
        }
        this->set_files(paths);
    }
}

int FileDiff::_get_focused_pane() {
    for (int i = 0; i < this->num_panes; i++) {
        if (this->textview[i]->is_focus()) {
            return i;
        }
    }
    return -1;
}

void FileDiff::on_revert_activate() {
    Gtk::ResponseType response = Gtk::RESPONSE_OK;
    std::vector<bool> unsaved;
    for (Glib::RefPtr<MeldBuffer> b : this->textbuffer) {
        if (b->data->modified) {
            unsaved.push_back(b->data->modified);
        }
    }
    if (!unsaved.empty()) {
        Component dialog("filediff.ui", "revert_dialog");
        Gtk::Window* w = static_cast<Gtk::Window*>(static_cast<Gtk::VBox*>(this->widget)->get_toplevel());
        static_cast<Gtk::MessageDialog*>(dialog.widget)->set_transient_for(*w);
        // FIXME: Should be packed into dialog.widget.get_message_area(),
        // but this is unbound on currently required PyGTK.
        std::vector<std::string> tmp;
        for (auto f : unsaved) {
            tmp.push_back("\t" + f);
        }
        std::string filelist = boost::join(tmp, "\n");
        static_cast<Gtk::MessageDialog*>(dialog.widget)->set_secondary_text(static_cast<Gtk::MessageDialog*>(dialog.widget)->property_secondary_text().get_value() + filelist);
        response = static_cast<Gtk::ResponseType>(static_cast<Gtk::MessageDialog*>(dialog.widget)->run());
        delete dialog.widget;
    }

    if (response == Gtk::RESPONSE_OK) {
        std::vector<std::string> files;
        for (int i = 0; i < this->num_panes; i++) {
            auto b = this->textbuffer[i];
            files.push_back(b->data->filename());
        }
        this->set_files(files);
    }
}

void FileDiff::on_refresh_activate() {
    this->refresh_comparison();
}

void FileDiff::queue_draw(int* junk) {
    for (MeldSourceView* t : this->textview) {
        t->queue_draw();
    }
    for (int i = 0; i < this->num_panes-1; i++) {
        this->linkmap[i]->queue_draw();
    }
    this->diffmap0->queue_draw();
    this->diffmap1->queue_draw();
}

void FileDiff::on_action_lock_scrolling_toggled(Glib::RefPtr<Gtk::ToggleAction> action) {
    this->toggle_scroll_lock(action->get_active());
}

void FileDiff::toggle_scroll_lock(bool locked) {
    Glib::RefPtr<Gtk::ToggleAction>::cast_static(this->actiongroup->get_action("LockScrolling"))->set_active(locked);
    this->_scroll_lock = not locked;
}

void FileDiff::on_readonly_button_toggled(Gtk::ToggleToolButton* button) {
    int index = -1;
    for (size_t i = 0; i < this->readonlytoggle.size(); i++) {
        Gtk::ToggleToolButton* b = this->readonlytoggle[i];
        if (b == button) {
            index = i;
            break;
        }
    }
    Glib::RefPtr<MeldBuffer> buf = this->textbuffer[index];
    this->set_buffer_editable(buf, not button->get_active());
}

//
// scrollbars
//
void FileDiff::_sync_hscroll(Glib::RefPtr<Gtk::Adjustment> adjustment) {
    if (this->_sync_hscroll_lock or this->_scroll_lock) {
        return;
    }

    this->_sync_hscroll_lock = true;
    double val = adjustment->get_value();
    for (int i = 0; i < this->num_panes; i++) {
        Gtk::ScrolledWindow* sw = this->scrolledwindow[i];
        Glib::RefPtr<Gtk::Adjustment> adj = sw->get_hadjustment();
        if (adj != adjustment) {
            adj->set_value(val);
        }
    }
    this->_sync_hscroll_lock = false;
}

void FileDiff::_sync_vscroll(Glib::RefPtr<const Gtk::Adjustment> adjustment, int master) {
    // only allow one scrollbar to be here at a time
    if (this->_sync_vscroll_lock) {
        return;
    }

    if (not this->_scroll_lock and (this->keymask & MASK_SHIFT) == 0) {
        this->_sync_vscroll_lock = true;
        float syncpoint = 0.5;

        // the line to search for in the 'master' text
        float master_y = (adjustment->get_value() + adjustment->get_page_size() *
                    syncpoint);

        Gtk::TextBuffer::iterator it;
        int line_top;
        this->textview[master]->get_line_at_y(it, int(master_y), line_top);
        int line_y;
        int height;
        this->textview[master]->get_line_yrange(it, line_y, height);
        int line = it.get_line() + ((master_y-line_y)/height);

        // scrollbar influence 0->1->2 or 0<-1->2 or 0<-1<-2
        int scrollbar_influence[3][2] = {{1, 2}, {0, 2}, {1, 0}};

        for (int j = 0; j < this->num_panes - 1; j++) {
            int i = scrollbar_influence[master][j];
            Glib::RefPtr<Gtk::Adjustment> adj = this->scrolledwindow[i]->get_vadjustment();
            int mbegin = 0;
            int mend = this->textbuffer[master]->get_line_count();
            int obegin = 0;
            int oend = this->textbuffer[i]->get_line_count();
            // look for the chunk containing 'line'
            for (difflib::chunk_t c : this->linediffer->pair_changes(master, i)) {
                if (std::get<1>(c) >= line) {
                    mend = std::get<1>(c);
                    oend = std::get<3>(c);
                    break;
                } else if (std::get<2>(c) >= line) {
                    mbegin = std::get<1>(c);
                    mend = std::get<2>(c);
                    obegin = std::get<3>(c);
                    oend = std::get<4>(c);
                    break;
                } else {
                    mbegin = std::get<2>(c);
                    obegin = std::get<4>(c);
                }
            }
            int fraction = (line - mbegin) / ((mend - mbegin) or 1);
            int other_line = (obegin + fraction * (oend - obegin));
            Gtk::TextBuffer::iterator it = this->textbuffer[i]->get_iter_at_line(int(other_line));
            int val;
            int height;
            this->textview[i]->get_line_yrange(it, val, height);
            val -= (adj->get_page_size()) * syncpoint;
            val += (other_line-int(other_line)) * height;
            val = std::min(std::max((double)val, adj->get_lower()),
                      adj->get_upper() - adj->get_page_size());
            adj->set_value(val);

            // If we just changed the central bar, make it the master
            if (i == 1) {
                master = 1;
                line = other_line;
            }
        }
        this->_sync_vscroll_lock = false;
    }

    for (Gtk::DrawingArea* lm : this->linkmap) {
        lm->queue_draw();
    }
}

std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> FileDiff::coords_iter(int i) {
    int buf_index;
    if (i == 1 and this->num_panes == 3) {
        buf_index = 2;
    } else {
        buf_index = i;
    }

    std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> coords_by_chunk = [this, buf_index, i] () {
        int y, h;
        this->textview[buf_index]->get_line_yrange(this->textbuffer[buf_index]->end(), y, h);
        float max_y = float(y + h);
        std::vector<std::tuple<Glib::ustring, int, int>> result;
        for (difflib::chunk_t c : this->linediffer->single_changes(i)) {
            int y0, _dummy;
            this->textview[buf_index]->get_line_yrange(this->textbuffer[buf_index]->get_iter_at_line(std::get<1>(c)), y0, _dummy);
            if (std::get<1>(c) == std::get<2>(c)) {
                y = y0;
                h = 0;
            } else {
                this->textview[buf_index]->get_line_yrange(this->textbuffer[buf_index]->get_iter_at_line(std::get<2>(c) - 1), y, h);
            }
            result.push_back(std::tuple<Glib::ustring, int, int>(std::get<0>(c), y0 / max_y, (y + h) / max_y));
        }
        return result;
    };
    return coords_by_chunk;
}

void FileDiff::set_num_panes(int n) {
    if (n != this->num_panes and (n == 1 || n == 2 || n == 3)) {
        this->num_panes = n;
        for (int i = 0; i < n; i++) {
            this->vbox[i]->show();
            this->file_toolbar[i]->show();
        }
        for (int i = 0; i < n - 1; i++) {
            this->diffmap[i]->show();
            this->linkmap[i]->show();
            this->dummy_toolbar_linkmap[i]->show();
#if I_KNOW_MORE
            this->dummy_toolbar_diffmap[i]->show();
#endif
        }
        for (int i = n; i < 3; i++) {
            this->vbox[i]->hide();
            this->file_toolbar[i]->hide();
        }
        for (int i = n - 1; i < 2; i++) {
            this->diffmap[i]->hide();
            this->linkmap[i]->hide();
            this->dummy_toolbar_linkmap[i]->hide();
#if I_KNOW_MORE
            this->dummy_toolbar_diffmap[i]->hide();
#endif
        }

        this->actiongroup->get_action("MakePatch")->set_sensitive(n > 1);
        this->actiongroup->get_action("CycleDocuments")->set_sensitive(n > 1);

        for (int i = 0; i < this->num_panes - 1; i++) {
            DiffMap* w = this->diffmap[0];
            Gtk::Scrollbar* scroll = this->scrolledwindow[i]->get_vscrollbar();
            std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> tmp = coords_iter(i);
            w->setup(scroll, tmp, std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>>(this->fill_colors, this->line_colors));
        }

        for (int i = 0; i < this->num_panes - 2; i++) {
            LinkMap* w = this->linkmap[i];
            w->associate(this, this->textview[i], this->textview[i + 1]);
        }

        for (int i = 0; i < this->num_panes; i++) {
            this->file_save_button[i]->set_sensitive(
                this->textbuffer[i]->data->modified);
        }

        this->queue_draw();
        this->recompute_label();
    }
}

void FileDiff::next_diff(GdkScrollDirection direction, bool centered) {
    int target;
    if (direction == GDK_SCROLL_DOWN) {
        target = this->cursor->next;
    } else {
        target = this->cursor->prev;
    }
    if (!target) {
        return;
    }

    int pane = this->_get_focused_pane();
    if (pane == -1) {
        if (!this->textview.empty()) {
            pane = 1;
        } else {
            pane = 0;
        }
    }

    difflib::chunk_t chunk = this->linediffer->get_chunk(target, pane);
    if (chunk == difflib::EMPTY_CHUNK) {
        return;
    }

    // Warp the cursor to the first line of next chunk
    Glib::RefPtr<MeldBuffer> buf = this->textbuffer[pane];
    if (this->cursor->line != std::get<1>(chunk)) {
        buf->place_cursor(buf->get_iter_at_line(std::get<1>(chunk)));
    }
    float tolerance = centered ? 0.0 : 0.2;
    this->textview[pane]->scroll_to(
        buf->get_insert(), tolerance, 0.5, 0.5);
}

void FileDiff::copy_chunk(int src, int dst, const difflib::chunk_t& chunk, bool copy_up) {
    Glib::RefPtr<MeldBuffer> b0 = this->textbuffer[src];
    Glib::RefPtr<MeldBuffer> b1 = this->textbuffer[dst];
    Gtk::TextBuffer::iterator start = b0->get_iter_at_line_or_eof(std::get<1>(chunk));
    Gtk::TextBuffer::iterator end = b0->get_iter_at_line_or_eof(std::get<2>(chunk));
    Glib::ustring t0 = b0->get_text(start, end, false);

    Gtk::TextBuffer::iterator  dst_start;
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark0;
    Gtk::TextBuffer::iterator new_end;
    if (copy_up) {
        if (std::get<2>(chunk) >= b0->get_line_count() and
            std::get<3>(chunk) < b1->get_line_count()) {
            // TODO: We need to insert a linebreak here, but there is no
            // way to be certain what kind of linebreak to use.
            t0 = t0 + "\n";
        }
        dst_start = b1->get_iter_at_line_or_eof(std::get<3>(chunk));
        mark0 = b1->create_mark("", dst_start, true);
        new_end = b1->insert_at_line(std::get<3>(chunk), t0);
    } else { // copy down
        dst_start = b1->get_iter_at_line_or_eof(std::get<4>(chunk));
        mark0 = b1->create_mark("", dst_start, true);
        new_end = b1->insert_at_line(std::get<4>(chunk), t0);
    }

    Glib::RefPtr<Gtk::TextBuffer::Mark> mark1 = b1->create_mark("", new_end, true);
    // FIXME: If the inserted chunk ends up being an insert chunk, then
    // this animation is not visible; this happens often in three-way diffs
    Gdk::RGBA rgba0 = this->fill_colors["insert"];
    Gdk::RGBA rgba1 = this->fill_colors["insert"];
    rgba0.set_alpha(1.0);
    rgba1.set_alpha(0.0);
    TextviewLineAnimation* anim = new TextviewLineAnimation(mark0, mark1, rgba0, rgba1, 500000);
    this->animating_chunks[dst].push_back(anim);
}

void FileDiff::replace_chunk(int src, int dst, const difflib::chunk_t& chunk) {
    Glib::RefPtr<MeldBuffer> b0 = this->textbuffer[src];
    Glib::RefPtr<MeldBuffer> b1 = this->textbuffer[dst];
    Gtk::TextBuffer::iterator src_start = b0->get_iter_at_line_or_eof(std::get<1>(chunk));
    Gtk::TextBuffer::iterator src_end = b0->get_iter_at_line_or_eof(std::get<2>(chunk));
    Gtk::TextBuffer::iterator dst_start = b1->get_iter_at_line_or_eof(std::get<3>(chunk));
    Gtk::TextBuffer::iterator dst_end = b1->get_iter_at_line_or_eof(std::get<4>(chunk));
    Glib::ustring t0 = b0->get_text(src_start, src_end, false);
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark0 = b1->create_mark("", dst_start, true);
    this->on_textbuffer_begin_user_action();
    b1->erase(dst_start, dst_end);
    Gtk::TextBuffer::iterator new_end = b1->insert_at_line(std::get<3>(chunk), t0);
    this->on_textbuffer_end_user_action();
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark1 = b1->create_mark("", new_end, true);
    // FIXME: If the inserted chunk ends up being an insert chunk, then
    // this animation is not visible; this happens often in three-way diffs
    Gdk::RGBA rgba0 = this->fill_colors["insert"];
    Gdk::RGBA rgba1 = this->fill_colors["insert"];
    rgba0.set_alpha(1.0);
    rgba1.set_alpha(0.0);
    TextviewLineAnimation* anim = new TextviewLineAnimation(mark0, mark1, rgba0, rgba1, 500000);
    this->animating_chunks[dst].push_back(anim);
}

void FileDiff::delete_chunk(int src, const difflib::chunk_t& chunk) {
    Glib::RefPtr<MeldBuffer> b0 = this->textbuffer[src];
    Gtk::TextBuffer::iterator it = b0->get_iter_at_line_or_eof(std::get<1>(chunk));
    if (std::get<2>(chunk) >= b0->get_line_count()) {
        it.backward_char();
    }
    b0->erase(it, b0->get_iter_at_line_or_eof(std::get<2>(chunk)));
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark0 = b0->create_mark("", it, true);
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark1 = b0->create_mark("", it, true);
    // TODO: Need a more specific colour here; conflict is wrong
    Gdk::RGBA rgba0 = this->fill_colors["conflict"];
    Gdk::RGBA rgba1 = this->fill_colors["conflict"];
    rgba0.set_alpha(1.0);
    rgba1.set_alpha(0.0);
    TextviewLineAnimation* anim = new TextviewLineAnimation(mark0, mark1, rgba0, rgba1, 500000);
    this->animating_chunks[src].push_back(anim);
}

int FileDiff::make_line_retriever(int pane, std::vector<Glib::RefPtr<Gtk::TextBuffer::Mark>> marks) {
    Glib::RefPtr<MeldBuffer> buf = this->textbuffer[pane];
    Glib::RefPtr<Gtk::TextBuffer::Mark> mark = marks[pane];

    return buf->get_iter_at_mark(mark).get_line();
}

void FileDiff::add_sync_point() {
    int pane = this->_get_focused_pane();
    if (pane == -1) {
        return;
    }

#if 0
    // Find a non-complete syncpoint, or create a new one
    if (this->syncpoints and None in this->syncpoints[-1]) {
        syncpoint = this->syncpoints.pop();
    } else {
        syncpoint = [None] * this->num_panes;
    }
    cursor_it = this->textbuffer[pane].get_iter_at_mark(
        this->textbuffer[pane].get_insert());
    syncpoint[pane] = this->textbuffer[pane].create_mark(None, cursor_it);
    this->syncpoints.append(syncpoint);

    valid_points = [p for p in this->syncpoints if all(p)]
    if (valid_points and this->num_panes == 2) {
        this->linediffer.syncpoints = [
            ((make_line_retriever(1, p), make_line_retriever(0, p)), )
            for p in valid_points
        ];
    } else if (valid_points and this->num_panes == 3) {
        this->linediffer.syncpoints = [
            ((make_line_retriever(1, p), make_line_retriever(0, p)),
             (make_line_retriever(1, p), make_line_retriever(2, p)))
            for p in valid_points
        ];
    }

    if (valid_points) {
        for (MsgAreaController* mgr : this->msgarea_mgr) {
            Gtk::InfoBar* msgarea = mgr->new_from_text_and_icon(
                Gtk::Stock::DIALOG_INFO,
                _("Live comparison updating disabled"),
                _("Live updating of comparisons is disabled when "
                  "synchronization points are active. You can still "
                  "manually refresh the comparison, and live updates will "
                  "resume when synchronization points are cleared."));
            mgr->set_msg_id(MSG_SYNCPOINTS);
            msgarea->show_all();
        }
    }
#endif

    this->refresh_comparison();
}

void FileDiff::clear_sync_points() {
    this->syncpoints.clear();
    this->linediffer->syncpoints.clear();
    for (MsgAreaController* mgr : this->msgarea_mgr) {
        if (mgr->get_msg_id() == MSG_SYNCPOINTS) {
            mgr->clear();
        }
    }
    this->refresh_comparison();
}
