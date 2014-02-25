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

#ifndef __MELD__FILEDIFF_H__
#define __MELD__FILEDIFF_H__

#include <functional>
#include <boost/variant.hpp>

#include "ui/gnomeglade.h"
#include "ui/findbar.h"
#include "ui/msgarea.h"
#include "melddoc.h"
#include "undo.h"
#include "meldbuffer.h"
#include "sourceview.h"
#include "diffutil.h"
#include "merge.h"
#include "ui/statusbar.h"
#include "filters.h"
#include "diffmap.h"
#include "linkmap.h"
#include "diffgrid.h"

#include <time.h>
#include <libintl.h>
#define _(str) gettext(str)

class LinkMap;

/*!
 * Simple class for caching diff results, with LRU-based eviction
 *
 * Results from the SequenceMatcher are cached and timestamped, and
 * subsequently evicted based on least-recent generation/usage. The LRU-based
 * eviction is overly simplistic, but is okay for our usage pattern.
 */
class CachedSequenceMatcher {
private:
    static Glib::Threads::Thread* process_pool;
    std::map<std::pair<Glib::ustring, Glib::ustring>, std::pair<difflib::chunk_list_t, time_t>> cache;
public:
    CachedSequenceMatcher();
    void match(Glib::ustring text1, Glib::ustring textn, std::function<void(difflib::chunk_list_t)> cb);
    /*!
     * Clean the cache if necessary
     *
     * @param size_hint: the recommended minimum number of cache entries
     */
    void clean(size_t size_hint);
};

class TextviewLineAnimation : Glib::Object {
public:
    Glib::RefPtr<Gtk::TextBuffer::Mark> start_mark;
    Glib::RefPtr<Gtk::TextBuffer::Mark> end_mark;
    Gdk::RGBA start_rgba;
    Gdk::RGBA end_rgba;
    gint64 start_time;
    int duration;

    TextviewLineAnimation(Glib::RefPtr<Gtk::TextBuffer::Mark> mark0, Glib::RefPtr<Gtk::TextBuffer::Mark> mark1, Gdk::RGBA rgba0, Gdk::RGBA rgba1, int duration) {
        this->start_mark = mark0;
        this->end_mark = mark1;
        this->start_rgba = rgba0;
        this->end_rgba = rgba1;
        this->start_time = g_get_monotonic_time();
        this->duration = duration;
    }
};

class CursorDetails : public Glib::Object {
public:
    int pane;
    int pos;
    int line;
    int offset;
    int chunk;
    int prev;
    int next;
    int prev_conflict;
    int next_conflict;
};

const int MASK_SHIFT = 1;
const int MASK_CTRL = 2;

/*! Two or three way comparison of text files */
class FileDiff : public MeldDoc {
public:
    typedef sigc::signal<void, int> type_signal_action_mode_changed;
    type_signal_action_mode_changed signal_action_mode_changed() {
        return m_signal_action_mode_changed;
    }
    type_signal_action_mode_changed m_signal_action_mode_changed;
private:
    bool warned_bad_comparison;
    int keymask;
    std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta;
    int deleted_lines_pending;
    int textview_overwrite;
    Gtk::TextView* focus_pane;
    std::vector<sigc::connection> textview_overwrite_handlers;
public:
    std::vector<BufferLines*> buffer_texts;
    UndoSequence* undosequence;
private:
    std::vector<FilterEntry*> text_filters;
    std::vector<sigc::connection> settings_handlers;
public:
    std::vector<BufferLines*> buffer_filtered;
private:
    bool _sync_vscroll_lock;
    bool _sync_hscroll_lock;
    bool _scroll_lock;
    bool force_highlight;
    std::vector<std::vector<Glib::RefPtr<Gtk::TextBuffer::Mark>>> syncpoints;
    bool in_nested_textview_gutter_expose;
    CachedSequenceMatcher* _cached_match;
    std::vector<int> anim_source_id;
    std::vector<std::vector<TextviewLineAnimation*>> animating_chunks;
    std::string ui_file;
    Glib::RefPtr<Gtk::ActionGroup> actiongroup;
    FindBar* findbar;
    DiffGrid* grid;
public:
    CursorDetails* cursor;
private:
    std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__ = {
        {"highlight-current-line", "highlight-current-line"},
        {"ignore-blank-lines", "ignore-blank-lines"}
    };
#if 0

    differ = diffutil.Differ
#endif

    std::map<guint, int> keylookup = {
        {GDK_KEY_Shift_L, MASK_SHIFT},
        {GDK_KEY_Shift_R, MASK_SHIFT},
        {GDK_KEY_Control_L, MASK_CTRL},
        {GDK_KEY_Control_R, MASK_CTRL}
    };

public:
    // Identifiers for MsgArea messages
    static const int MSG_SAME = 0;
    static const int MSG_SLOW_HIGHLIGHT = 1;
    static const int MSG_SYNCPOINTS = 2;

    typedef sigc::signal<void, bool, bool> type_signal_next_conflict_changed;
    type_signal_next_conflict_changed signal_next_conflict_changed() {
        return m_signal_next_conflict_changed;
    }
    type_signal_next_conflict_changed m_signal_next_conflict_changed;

private:

    // Abbreviations for insert and overwrite that fit in the status bar
    std::string _insert_overwrite_text[2] = {_("INS"), _("OVR")};
    // Abbreviation for line, column so that it will fit in the status bar
    std::string _line_column_text = _("Ln %i, Col %i");

    DiffMap* diffmap0;
    DiffMap* diffmap1;
    std::vector<DiffMap*> diffmap;
    Gtk::ToolButton* file_save_button0;
    Gtk::ToolButton* file_save_button1;
    Gtk::ToolButton* file_save_button2;
    std::vector<Gtk::ToolButton*> file_save_button;
    Gtk::Toolbar* file_toolbar0;
    Gtk::Toolbar* file_toolbar1;
    Gtk::Toolbar* file_toolbar2;
    std::vector<Gtk::Toolbar*> file_toolbar;
    Gtk::FileChooserButton* fileentry0;
    Gtk::FileChooserButton* fileentry1;
    Gtk::FileChooserButton* fileentry2;
    std::vector<Gtk::FileChooserButton*> fileentry;
    LinkMap* linkmap0;
    LinkMap* linkmap1;
    std::vector<LinkMap*> linkmap;
    MsgAreaController* msgarea_mgr0;
    MsgAreaController* msgarea_mgr1;
    MsgAreaController* msgarea_mgr2;
    std::vector<MsgAreaController*> msgarea_mgr;
    Gtk::ToggleToolButton* readonlytoggle0;
    Gtk::ToggleToolButton* readonlytoggle1;
    Gtk::ToggleToolButton* readonlytoggle2;
    std::vector<Gtk::ToggleToolButton*> readonlytoggle;
    Gtk::ScrolledWindow* scrolledwindow0;
    Gtk::ScrolledWindow* scrolledwindow1;
    Gtk::ScrolledWindow* scrolledwindow2;
    std::vector<Gtk::ScrolledWindow*> scrolledwindow;
    MeldSourceView* textview0;
    MeldSourceView* textview1;
    MeldSourceView* textview2;
public:
    std::vector<MeldSourceView*> textview;
private:
    Gtk::VBox* vbox0;
    Gtk::VBox* vbox1;
    Gtk::VBox* vbox2;
    std::vector<Gtk::VBox*> vbox;
    Gtk::Toolbar* dummy_toolbar_linkmap0;
    Gtk::Toolbar* dummy_toolbar_linkmap1;
    std::vector<Gtk::Toolbar*> dummy_toolbar_linkmap;
    Gtk::ToolItem* filelabel_toolitem0;
    Gtk::ToolItem* filelabel_toolitem1;
    Gtk::ToolItem* filelabel_toolitem2;
    std::vector<Gtk::ToolItem*> filelabel_toolitem;
    Gtk::Label* filelabel0;
    Gtk::Label* filelabel1;
    Gtk::Label* filelabel2;
    std::vector<Gtk::Label*> filelabel;
    Gtk::ToolItem* fileentry_toolitem0;
    Gtk::ToolItem* fileentry_toolitem1;
    Gtk::ToolItem* fileentry_toolitem2;
    std::vector<Gtk::ToolItem*> fileentry_toolitem;
    Gtk::ToolItem* dummy_toolbar_diffmap0;
    Gtk::ToolItem* dummy_toolbar_diffmap1;
    std::vector<Gtk::ToolItem*> dummy_toolbar_diffmap;

    std::vector<Gtk::Label*> status_info_labels;
    MeldStatusBar* statusbar;

public:
    int num_panes;
    std::map<Glib::ustring, Gdk::RGBA> fill_colors;
    std::map<Glib::ustring, Gdk::RGBA> line_colors;
    Gdk::RGBA highlight_color;
    Gdk::RGBA syncpoint_color;
    int pixels_per_line;
    _Differ* linediffer;

    FileDiff(int num_panes, SchedulerBase& scheduler);
    virtual ~FileDiff();
    std::vector<Glib::RefPtr<MeldBuffer>> textbuffer;
    Glib::ustring _get_filename_for_saving(Glib::ustring);

    int get_keymask();
    void set_keymask(int value);
    bool on_key_event(GdkEventKey* event);
    void on_style_updated();
    void on_focus_change();
    void on_text_filters_changed();
    bool create_text_filters();
    void _disconnect_buffer_handlers();
    virtual void _connect_buffer_handlers();
    void on_cursor_position_changed(Glib::RefPtr<Gtk::TextBuffer> buf, int pspec, bool force = false);
    bool on_current_diff_changed(GdkEventFocus* event);
    void on_next_conflict_changed(bool have_prev, bool have_next);
    void on_next_conflict_up();
    void on_next_conflict_down();
    void on_next_conflict(GdkScrollDirection direction);
    void push_change_left();
    void push_change_right();
    void push_change(int direction);
    void pull_change_left();
    void pull_change_right();
    void pull_change(int direction);
    void copy_change_left_up();
    void copy_change_left_down();
    void copy_change_right_up();
    void copy_change_right_down();
    void copy_change(int direction, int copy_direction);
    void pull_all_non_conflicting_changes_left();
    void pull_all_non_conflicting_changes_right();
    void pull_all_non_conflicting_changes(int direction);
    void merge_all_non_conflicting_changes();
    void delete_change();
    difflib::chunk_t _synth_chunk(int pane0, int pane1, int line);
    /*! Approximates the corresponding line between panes */
    int _corresponding_chunk_line(int chunk, int line, int pane, int new_pane);
    void action_cycle_documents();
    void _set_external_action_sensitivity();
    bool on_textview_focus_in_event(GdkEventFocus* event, Gtk::TextView* view);
    bool on_textview_focus_out_event(GdkEventFocus* event);
    void _after_text_modified(Glib::RefPtr<MeldBuffer> buffer, int startline, int sizechange);
    std::string _filter_text(std::string txt);
    void after_text_insert_text(const Gtk::TextBuffer::iterator& it, const Glib::ustring& newtext, int textlen, Glib::RefPtr<MeldBuffer> buf);
    void after_text_delete_range(Glib::RefPtr<MeldBuffer> buffer, Gtk::TextBuffer::iterator it0, Gtk::TextBuffer::iterator it1);
    void load_font();
    void on_setting_changed(const Glib::ustring& key);
    Gtk::ResponseType check_save_modified(Glib::ustring label = "");
    virtual Gtk::ResponseType on_delete_event(int appquit = 0);
    void on_undo_activate();
    void on_redo_activate();
    void on_textbuffer_begin_user_action();
    void on_textbuffer_end_user_action();
    void on_text_insert_text(const Gtk::TextBuffer::iterator& it, const Glib::ustring& text, int textlen, Glib::RefPtr<MeldBuffer> buf);
    void on_text_delete_range(Gtk::TextBuffer* buf, Gtk::TextBuffer::iterator it0, Gtk::TextBuffer::iterator it1);
    void on_undo_checkpointed(Glib::RefPtr<MeldBuffer> buf, bool checkpointed);
    void open_external();
    void update_text_actions_sensitivity();
    /*! Returns selected text of active pane */
    Glib::ustring get_selected_text();
    virtual void on_find_activate();
    virtual void on_replace_activate();
    virtual void on_find_next_activate();
    virtual void on_find_previous_activate();
    void on_scrolledwindow_size_allocate(Gtk::Allocation& allocation, Gtk::ScrolledWindow* scrolledwindow);
    bool on_textview_popup_menu();
    bool on_textview_button_press_event(GdkEventButton* event, MeldSourceView* textview);
    void on_textview_toggle_overwrite(MeldSourceView* view);
    virtual void set_labels(std::vector<Glib::ustring> labels);
    void set_merge_output_file(Glib::ustring filename);
    void _set_save_action_sensitivity();
    void recompute_label();
    void set_files(std::vector<std::string> files);
    virtual std::pair<std::string, std::vector<std::string>> get_comparison();
    void add_dismissable_msg_on_response(int response_id, int pane);
    Gtk::InfoBar* add_dismissable_msg(int pane, const Gtk::BuiltinStockID icon, std::string primary, std::string secondary);
    void _load_files(std::vector<std::string> files, std::vector<Glib::RefPtr<MeldBuffer>> textbuffers);
    void _diff_files(bool refresh = false);
    virtual void _set_files_internal(std::vector<std::string> files);
    void on_file_changed_response(int /*Gtk::ResponseType*/ response_id, int pane);
    void set_meta(std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta);
    void notify_file_changed(MeldBufferData* data);
    /*! Refresh the view by clearing and redoing all comparisons */
    void refresh_comparison();
    void _set_merge_action_sensitivity();
    void on_diffs_changed(std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>> chunk_changes);
    void on_msgarea_highlighting_response(int /*Gtk::ResponseType*/ respid);
    void _prompt_long_highlighting();
    void on_msgarea_identical_response(int /*Gtk::ResponseType*/ respid);
    bool on_textview_draw(const Cairo::RefPtr<Cairo::Context>& context, MeldSourceView* textview);
    void _get_filename_for_saving(int title);
    bool _save_text_to_filename(Glib::ustring filename, Glib::ustring text);
    void on_file_changed_response_2(int /*Gtk::ResponseType*/ response_id, int pane, bool saveas);
    bool save_file(int pane, bool saveas = false, bool force_overwrite = false);
    void make_patch();
    void set_buffer_writable(Glib::RefPtr<MeldBuffer> buf, bool writable);
    void set_buffer_modified(Glib::RefPtr<MeldBuffer> buf, bool yesno);
    void set_buffer_editable(Glib::RefPtr<MeldBuffer> buf, bool editable);
    void save();
    virtual void save_as();
    void on_save_all_activate();
    void on_file_save_button_clicked(Gtk::ToolButton* button);
    void on_fileentry_file_set();
    int _get_focused_pane();
    void on_revert_activate();
    virtual void on_refresh_activate();
    void queue_draw(int *junk = 0);
    void on_action_lock_scrolling_toggled(Glib::RefPtr<Gtk::ToggleAction> action);
    void toggle_scroll_lock(bool locked);
    void on_readonly_button_toggled(Gtk::ToggleToolButton* button);
    void _sync_hscroll(Glib::RefPtr<Gtk::Adjustment> adjustment);
    void _sync_vscroll(Glib::RefPtr<const Gtk::Adjustment> adjustment, int master);
    std::function<std::vector<std::tuple<Glib::ustring, int, int>>()> coords_iter(int i);
    void set_num_panes(int n);
    virtual void next_diff(GdkScrollDirection direction, bool centered = false);
    void copy_chunk(int from_pane, int to_pane, const difflib::chunk_t& chunk, bool copy_up);
    void replace_chunk(int src, int dst, const difflib::chunk_t& chunk);
    void delete_chunk(int src, const difflib::chunk_t& chunk);
    int make_line_retriever(int pane, std::vector<Glib::RefPtr<Gtk::TextBuffer::Mark>> marks);
    void add_sync_point();
    void clear_sync_points();

};

#endif
