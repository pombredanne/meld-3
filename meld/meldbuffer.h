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

#ifndef __MELD__MELDBUFFER_H__
#define __MELD__MELDBUFFER_H__

#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include <functional>

#include "undo.h"

class MeldBufferData;

class MeldBuffer : public Gsv::Buffer {

    const std::string __gtype_name__ = "MeldBuffer";

    std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__ = {
        {"highlight-syntax", "highlight-syntax"}
    };

    int user_action_count;

public:
    std::vector<sigc::connection> handlers;
    MeldBufferData* data;
    MeldBuffer(std::string filename = "");

    void do_begin_user_action(int *args);

    void do_end_user_action(int *args);

    virtual void apply_tag(const Glib::RefPtr<Tag>& tag, const iterator& start, const iterator& end);

    /*! Clear the contents of the buffer and reset its metadata */
    void reset_buffer(std::string filename);

    /*! Return a Gtk.TextIter at the given line, or the end of the buffer.
     *
     * This method is like get_iter_at_line, but if asked for a position past
     * the end of the buffer, this returns the end of the buffer; the
     * get_iter_at_line behaviour is to return the start of the last line in
     * the buffer.
     */
    Gtk::TextBuffer::iterator get_iter_at_line_or_eof(int line);

    /*!
     * Insert text at the given line, or the end of the buffer.
     *
     * This method is like insert, but if asked to insert something past the
     * last line in the buffer, this will insert at the end, and will add a
     * linebreak before the inserted text. The last line in a Gtk.TextBuffer
     * is guaranteed never to have a newline, so we need to handle this.
     */
    Gtk::TextBuffer::iterator insert_at_line(int line, Glib::ustring text);
};


class MeldBufferData : Glib::Object {
public:
    typedef sigc::signal<void, MeldBufferData*> type_signal_file_changed;
    type_signal_file_changed signal_file_changed() {
        return m_signal_file_changed;
    }
    type_signal_file_changed m_signal_file_changed;
private:
    std::string _label;
    std::string _filename;
public:
    bool modified;
    bool writable;
    bool editable;
private:
    std::pair<Glib::RefPtr<Gio::FileMonitor>, sigc::connection*>* _monitor;
    std::unique_ptr<std::pair<glong, glong>> _mtime;
    std::unique_ptr<std::pair<glong, glong>> _disk_mtime;
public:
    std::string savefile;
    Glib::ustring encoding;
    std::string newlines;

    MeldBufferData(std::string filename = "");

    void reset();

    ~MeldBufferData();

    std::string label();

    void label(std::string value);

    void _connect_monitor();

    void _disconnect_monitor();

    std::unique_ptr<std::pair<glong, glong>> _query_mtime(const Glib::RefPtr<Gio::File>& gfile);

    void _handle_file_change(const Glib::RefPtr<Gio::File>& f, const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent event_type);

    std::string filename();

    void filename(std::string value);

    void update_mtime();

    bool current_on_disk();
};

// TODO stub
class IndexError : public std::exception {
};

/*!
 * Gtk.TextBuffer shim with line-based access and optional filtering
 *
 * This class allows a Gtk.TextBuffer to be treated as a list of lines of
 * possibly-filtered text. If no filter is given, the raw output from the
 * Gtk.TextBuffer is used.
 *
 * The logic here (and in places in FileDiff) requires that Python's
 * unicode splitlines() implementation and Gtk.TextBuffer agree on where
 * linebreaks occur. Happily, this is usually the case.
 */
class BufferLines {
private:
    Glib::RefPtr<MeldBuffer> buf;
    std::function<std::string(std::string)> textfilter;
public:
    BufferLines(Glib::RefPtr<MeldBuffer> buf, std::function<std::string(std::string)> textfilter = nullptr);

    std::vector<Glib::ustring> __getitem__(std::vector<int> key);
    std::vector<Glib::ustring> operator[] (std::vector<int> key) {
        return __getitem__(key);
    }
    Glib::ustring __getitem__(int key);
    Glib::ustring operator[] (int key) {
        return __getitem__(key);
    }

    int __len__();
};


/*! A helper to undo/redo text insertion/deletion into/from a text buffer */
class BufferAction {
private:
    int offset;
    Glib::ustring text;
    Glib::RefPtr<MeldBuffer> buffer;
public:

    BufferAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text);

    virtual void undo() = 0;
    virtual void redo() = 0;

protected:

    void del();

    void ins();

};


class BufferInsertionAction : public BufferAction {
public:

    BufferInsertionAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text);

    virtual void undo();
    virtual void redo();

};


class BufferDeletionAction : public BufferAction {
public:

    BufferDeletionAction(Glib::RefPtr<MeldBuffer> buf, int offset, Glib::ustring text);

    virtual void undo();
    virtual void redo();

};


#endif
