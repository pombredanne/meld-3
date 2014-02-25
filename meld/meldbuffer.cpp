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

#include <cassert>

#include "util/compat.h"
#include "conf.h"

#include "meldbuffer.h"
#include "settings.h"

MeldBuffer::MeldBuffer(std::string filename) : Gsv::Buffer() {
    bind_settings(this, __gsettings_bindings__);
    this->data = new MeldBufferData(filename);
    this->user_action_count = 0;
}

void MeldBuffer::do_begin_user_action(int *args) {
    this->user_action_count += 1;
}

void MeldBuffer::do_end_user_action(int *args) {
    this->user_action_count -= 1;
}

void MeldBuffer::apply_tag(const Glib::RefPtr<Tag>& tag, const iterator& start, const iterator& end) {
    // Filthy, evil, horrible hack. What we're doing here is trying to
    // figure out if a tag apply has come from a paste action, in which
    // case GtkTextBuffer will 'helpfully' apply the existing tags in the
    // copied selection. There appears to be no way to override this
    // behaviour, or to hook in to the necessary paste mechanics to just
    // request that we only get plain text or something. We're abusing the
    // user_action notion here, because we only apply the tags we actually
    // want in a callback.
    if (tag->property_name() == "inline" and this->user_action_count > 0) {
        return;
    }
    Gsv::Buffer::apply_tag(tag, start, end);
}

/*! Clear the contents of the buffer and reset its metadata */
void MeldBuffer::reset_buffer(std::string filename) {
    Gtk::TextBuffer::iterator range_begin, range_end;
    this->get_bounds(range_begin, range_end);
    this->erase(range_begin, range_end);
    std::string label;
    if (this->data->filename() == filename) {
        label = this->data->label();
    } else {
        label = filename;
    }
    this->data->reset();
    this->data->filename(filename);
    this->data->label(label);
}

/*! Return a Gtk.TextIter at the given line, or the end of the buffer.
 *
 * This method is like get_iter_at_line, but if asked for a position past
 * the end of the buffer, this returns the end of the buffer; the
 * get_iter_at_line behaviour is to return the start of the last line in
 * the buffer.
 */
Gtk::TextBuffer::iterator MeldBuffer::get_iter_at_line_or_eof(int line) {
    if (line >= this->get_line_count()) {
        return this->end();
    }
    return this->get_iter_at_line(line);
}

/*!
 * Insert text at the given line, or the end of the buffer.
 *
 * This method is like insert, but if asked to insert something past the
 * last line in the buffer, this will insert at the end, and will add a
 * linebreak before the inserted text. The last line in a Gtk.TextBuffer
 * is guaranteed never to have a newline, so we need to handle this.
 */
Gtk::TextBuffer::iterator MeldBuffer::insert_at_line(int line, Glib::ustring text) {
    if (line >= this->get_line_count()) {
        // TODO: We need to insert a linebreak here, but there is no
        // way to be certain what kind of linebreak to use.
        text = "\n" + text;
    }
    Gtk::TextBuffer::iterator it = this->get_iter_at_line_or_eof(line);
    this->insert(it, text);
    return it;
}


MeldBufferData::MeldBufferData(std::string filename) : Glib::Object() {
    this->reset();
    this->_label = filename;
    this->filename(filename);
}

void MeldBufferData::reset() {
    this->modified = false;
    this->writable = true;
    this->editable = true;
    this->_monitor = 0;
#if 0
    this->_mtime = 0;
    this->_disk_mtime = 0;
#endif
    this->filename("");
    this->savefile = "";
    this->_label = "";
    this->encoding = "";
    this->newlines = "";
}

MeldBufferData::~MeldBufferData() {
    this->_disconnect_monitor();
}

std::string MeldBufferData::label() {
    if (this->_label.empty()) {
        //TRANSLATORS: This is the label of a new, currently-unnamed file.
        return _("<unnamed>");
    }
    return this->_label;
}

void MeldBufferData::label(std::string value) {
    this->_label = value;
}

void MeldBufferData::_connect_monitor() {
    if (!this->_filename.empty()) {
        Glib::RefPtr<Gio::FileMonitor> monitor = Gio::File::create_for_path(this->_filename)->monitor_file(
            Gio::FILE_MONITOR_NONE);
#if 0
        sigc::connection handler_id = monitor->signal_changed().connect(sigc::mem_fun(*this, &MeldBufferData::_handle_file_change));
        this->_monitor = new std::pair<Glib::RefPtr<Gio::FileMonitor>, sigc::connection*>(monitor, &handler_id);
#endif
    }
}

void MeldBufferData::_disconnect_monitor() {
    if (this->_monitor) {
        Glib::RefPtr<Gio::FileMonitor> monitor = this->_monitor->first;
        sigc::connection* handler_id = this->_monitor->second;
        handler_id->disconnect();
        monitor->cancel();
    }
}

std::unique_ptr<std::pair<glong, glong>> MeldBufferData::_query_mtime(const Glib::RefPtr<Gio::File>& gfile) {
    Glib::RefPtr<Gio::FileInfo> info;
    try {
        std::string time_query = std::string(G_FILE_ATTRIBUTE_TIME_MODIFIED) + "," +
                               G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC;
        info = gfile->query_info(time_query, Gio::FILE_QUERY_INFO_NONE);
    } catch (Glib::Error &e) {
        return 0;
    }
    Glib::TimeVal mtime = info->modification_time();
    return std::unique_ptr<std::pair<glong, glong>>(new std::pair<glong, glong>(mtime.tv_sec, mtime.tv_usec));
}

void MeldBufferData::_handle_file_change(const Glib::RefPtr<Gio::File>& f, const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent event_type) {
    std::unique_ptr<std::pair<glong, glong>> mtime = this->_query_mtime(f);
    if (this->_disk_mtime and mtime->first > this->_disk_mtime->first and mtime->second > this->_disk_mtime->second) {
        this->signal_file_changed().emit(this);
    }
    this->_disk_mtime = std::move(mtime);
}

std::string MeldBufferData::filename() {
    return this->_filename;
}

void MeldBufferData::filename(std::string value) {
    this->_disconnect_monitor();
    this->_filename = value;
    this->update_mtime();
    this->_connect_monitor();
}

void MeldBufferData::update_mtime() {
    if (!this->_filename.empty()) {
        Glib::RefPtr<Gio::File> gfile = Gio::File::create_for_path(this->_filename);
        this->_disk_mtime = this->_query_mtime(gfile);
        this->_mtime = this->_query_mtime(gfile);
    }
}

bool MeldBufferData::current_on_disk() {
    return this->_mtime == this->_disk_mtime;
}

BufferLines::BufferLines(Glib::RefPtr<MeldBuffer> buf, std::function<std::string(std::string)> textfilter) {
    this->buf = buf;
    if (textfilter) {
        this->textfilter = textfilter;
    } else {
        this->textfilter = [] (std::string in) { return in; };
    }
}

std::vector<Glib::ustring> BufferLines::__getitem__(std::vector<int> key) {
#if 0
    lo, hi, _ = key.indices(this->buf.get_line_count());
#else
    int lo;
    int hi;
#endif

    // FIXME: If we ask for arbitrary slices past the end of the buffer,
    // this will return the last line.
    Gtk::TextBuffer::iterator start = this->buf->get_iter_at_line_or_eof(lo);
    Gtk::TextBuffer::iterator end = this->buf->get_iter_at_line_or_eof(hi);
    Glib::ustring txt = this->buf->get_text(start, end, false);

    Glib::ustring filter_txt = this->textfilter(txt);
#if 0
    lines = filter_txt.splitlines();
    ends = filter_txt.splitlines(true);
#else
    std::vector<Glib::ustring> lines;
    std::vector<Glib::ustring> ends;
#endif

    // The last line in a Gtk.TextBuffer is guaranteed never to end in a
    // newline. As splitlines() discards an empty line at the end, we
    // need to artificially add a line if the requested slice is past
    // the end of the buffer, and the last line in the slice ended in a
    // newline.
    if (hi >= this->buf->get_line_count() and
       lo < this->buf->get_line_count() and
       (lines.empty() or lines[-1].length() != ends[-1].length())) {
        lines.push_back("");
        ends.push_back("");
    }

    if (hi == std::numeric_limits<int>::max()) {
        hi = this->buf->get_line_count();
    }
    if (hi - lo != ((int) lines.size())) {
        // These codepoints are considered line breaks by Python, but
        // not by GtkTextStore.
        std::set<char> additional_breaks = {'\x0c', '\x85'};
        unsigned int i = 0;
        while (i < ends.size()) {
            Glib::ustring line = lines[i];
            Glib::ustring end0 = ends[i];
            // It's possible that the last line in a file would end in a
            // line break character, which requires no joining.
            if (!end0.empty() and std::find(additional_breaks.begin(), additional_breaks.end(), end0[-1]) != additional_breaks.end() and
               (line.empty() or std::find(additional_breaks.begin(), additional_breaks.end(), line[-1]) == additional_breaks.end())) {
                assert(ends.size() >= i + 1);
#if 0
                lines[i:i + 2] = [line + end0[-1] + lines[i + 1]];
                ends[i:i + 2] = [end0 + ends[i + 1]];
#endif
            }
            i += 1;
        }
    }

    return lines;
}

Glib::ustring BufferLines::__getitem__(int key) {
    if (key >= __len__()) {
        throw IndexError();
    }
    Gtk::TextBuffer::iterator line_start = this->buf->get_iter_at_line_or_eof(key);
    Gtk::TextBuffer::iterator line_end = line_start;
    if (not line_end.ends_line()) {
        line_end.forward_to_line_end();
    }
    Glib::ustring txt = this->buf->get_text(line_start, line_end, false);
    return this->textfilter(txt);
}

int BufferLines::__len__() {
    return this->buf->get_line_count();
}

BufferAction::BufferAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text) {
    this->buffer = buf;
    this->offset = offset;
    this->text = text;
}

void BufferAction::del() {
    Gtk::TextBuffer::iterator start = this->buffer->get_iter_at_offset(this->offset);
    Gtk::TextBuffer::iterator end = this->buffer->get_iter_at_offset(this->offset + this->text.length());
    this->buffer->erase(start, end);
}

void BufferAction::ins() {
    Gtk::TextBuffer::iterator start = this->buffer->get_iter_at_offset(this->offset);
    this->buffer->insert(start, this->text);
}



BufferInsertionAction::BufferInsertionAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text) : BufferAction(buf, offset, text) {
}

void BufferInsertionAction::undo() {
    BufferAction::del();
}

void BufferInsertionAction::redo() {
    BufferAction::ins();
}


BufferDeletionAction::BufferDeletionAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text) : BufferAction(buf, offset, text) {
}

void BufferDeletionAction::undo() {
    BufferAction::ins();
}

void BufferDeletionAction::redo() {
    BufferAction::del();
}

