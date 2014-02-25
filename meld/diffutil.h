/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009, 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__DIFFUTIL_H__
#define __MELD__DIFFUTIL_H__

#include <gtkmm.h>
#include <set>
#include <array>
#include "difflib/src/difflib.h"
#include "meldbuffer.h"

/*! Utility class to hold diff2 or diff3 chunks */
class _Differ : public Glib::Object {
public:
    std::vector<int> unresolved;
    typedef sigc::signal<void, std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>>> type_signal_diffs_changed;
    type_signal_diffs_changed signal_diffs_changed() {
        return m_signal_diffs_changed;
    }
    type_signal_diffs_changed m_signal_diffs_changed;
private:
    int num_sequences;
    std::vector<int> seqlength;
    std::pair<difflib::chunk_list_t, difflib::chunk_list_t> diffs;
public:
    std::vector<int> syncpoints;
    std::vector<int> conflicts;
private:
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> _old_merge_cache;
    std::pair<difflib::chunk_t, difflib::chunk_t> _changed_chunks;
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _merge_cache;
    std::vector<std::vector<std::array<int, 3>>> _line_cache;
public:
    bool ignore_blanks;
private:
    bool _initialised;
    std::array<bool, 4> _has_mergeable_changes;

public:

    _Differ();
    virtual ~_Differ();

    void _update_merge_cache(std::vector<std::string> texts);

    int _find_next(const int last_chunk, int diff, int seq, int current);

    void _update_line_cache();

    /*! Offset a chunk by o1/o2 if it's after the inserted lines */
    difflib::chunk_t offset(const difflib::chunk_t& c, int start, int o1, int o2);

    void change_sequence(int sequence, int startidx, int sizechange, std::vector<std::string> texts);

    /*! Find the index of the chunk which contains line. */
    int _locate_chunk(int whichdiffs, int sequence, int line);

    /*!
        Return the index-th change in from_pane

        If to_pane is provided, then only changes between from_pane and to_pane
        are considered, otherwise all changes starting at from_pane are used.
     */
    difflib::chunk_t get_chunk(int index, int from_pane, int to_pane = -1);

    /*! Find the index of the chunk which contains line. */
    std::array<int, 3> locate_chunk(int pane, int line);

    int diff_count();

    std::pair<bool, bool> has_mergeable_changes(int which);

    difflib::chunk_t offset(const difflib::chunk_t& c, int o1, int o2);

    void _change_sequence(int which, int sequence, int startidx, int sizechange, std::vector<std::string> texts);

    std::pair<int, int> _range_from_lines(int textindex, std::pair<int, int> lines);

    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> all_changes();

    /*! Give all changes between file1 and either file0 or file2. */
    difflib::chunk_list_t pair_changes(int fromindex, int toindex, std::vector<int> lines = {0, 0, 0, 0});

    /*! Give changes for single file only. do not return 'equal' hunks. */
    difflib::chunk_list_t single_changes(int textindex, std::pair<int, int> lines = {0, 0});

    bool sequences_identical();

    std::array<int, 6> _merge_blocks(std::array<difflib::chunk_list_t, 2> _using);

    /*! Automatically merge two sequences of change blocks */
    virtual std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _auto_merge(std::array<difflib::chunk_list_t, 2> _using, std::vector<std::string> texts);

    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _merge_diffs(difflib::chunk_list_t seq0, difflib::chunk_list_t seq1, std::vector<std::string> texts);

    void set_sequences_iter(std::vector<BufferLines*> sequences);

    void clear();
};

#endif
