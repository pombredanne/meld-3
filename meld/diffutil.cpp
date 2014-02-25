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

#include <gtkmm.h>
#include <set>
#include <cassert>

#include "matchers.h"
#include "meldbuffer.h"
#include "diffutil.h"
#include "conf.h"

static std::map<std::string, std::string> opcode_reverse = {
    {"replace", "replace"},
    {"insert", "delete"},
    {"delete", "insert"},
    {"conflict", "conflict"},
    {"equal", "equal"}
};


static difflib::chunk_t reverse_chunk(const difflib::chunk_t& chunk) {
    std::string tag = opcode_reverse[std::get<0>(chunk)];
    return difflib::chunk_t(tag, std::get<3>(chunk), std::get<4>(chunk), std::get<1>(chunk), std::get<2>(chunk));
}


static std::pair<size_t, size_t> _find_blank_lines(std::string txt, const size_t& lo_, const size_t& hi_) {
    size_t lo = lo_;
    size_t hi = hi_;
    while (lo < hi and not txt[lo]) {
        lo += 1;
    }
    while (lo < hi and not txt[hi - 1]) {
        hi -= 1;
    }
    return std::pair<size_t, size_t>(lo, hi);
}


static difflib::chunk_t consume_blank_lines(const difflib::chunk_t chunk, std::vector<std::string> texts, int pane1, int pane2) {
    if (chunk == difflib::EMPTY_CHUNK) {
        return difflib::EMPTY_CHUNK;
    }

    std::string tag = std::get<0>(chunk);
    std::pair<size_t, size_t> tmp;
    tmp = _find_blank_lines(texts[pane1], std::get<1>(chunk), std::get<2>(chunk));
    int c1 = tmp.first;
    int c2 = tmp.second;
    tmp = _find_blank_lines(texts[pane2], std::get<3>(chunk), std::get<4>(chunk));
    int c3 = tmp.first;
    int c4 = tmp.second;

    if (c1 == c2 and c3 == c4) {
        return difflib::EMPTY_CHUNK;
    }
    if (c1 == c2 and tag == "replace") {
        tag = "insert";
    } else if (c3 == c4 and tag == "replace") {
        tag = "delete";
    }
    return difflib::chunk_t(tag, c1, c2, c3, c4);
}


_Differ::_Differ() : Glib::Object() {
    // Internally, diffs are stored from text1 -> text0 and text1 -> text2.
    this->num_sequences = 0;
    this->seqlength = {0, 0, 0};
    this->ignore_blanks = false;
    this->_initialised = false;
    this->_has_mergeable_changes = {false, false, false, false};
    for (size_t seq = 0; seq < 3; seq++) {
        _line_cache.push_back({});
    }
}

_Differ::~_Differ() {
}

void _Differ::_update_merge_cache(std::vector<std::string> texts) {
    this->_merge_cache.clear();
    if (this->num_sequences == 3) {
        for (std::pair<difflib::chunk_t, difflib::chunk_t> c : this->_merge_diffs(this->diffs.first, this->diffs.second, texts)) {
             this->_merge_cache.push_back(c);
        }
    } else {
        for (difflib::chunk_t c :this->diffs.first) {
            this->_merge_cache.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(c, difflib::EMPTY_CHUNK));
        }
    }

    if (this->ignore_blanks) {
        // We don't handle altering the chunk-type of conflicts in three-way
        // comparisons where e.g., pane 1 and 3 differ in blank lines
        for (size_t i = 0; i < this->_merge_cache.size(); i++) {
            std::pair<difflib::chunk_t, difflib::chunk_t> c = this->_merge_cache[i];
            this->_merge_cache[i] = std::pair<difflib::chunk_t, difflib::chunk_t>(consume_blank_lines(c.first, texts, 1, 0),
                                                                                  consume_blank_lines(c.second, texts, 1, 2));
        }
        std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> tmp;
        for (std::pair<difflib::chunk_t, difflib::chunk_t> x : this->_merge_cache) {
            if (x.first != difflib::EMPTY_CHUNK || x.second != difflib::EMPTY_CHUNK) {
                tmp.push_back(x);
            }
        }
        this->_merge_cache = tmp;
    }

    // Calculate chunks that were added (in the new but not the old merge
    // cache), removed (in the old but not the new merge cache) and changed
    // (where the edit actually occurred, *and* the chunk is still around).
    // This information is used by the inline highlighting mechanism to
    // avoid re-highlighting existing chunks.
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> tmp(this->_merge_cache.begin(), this->_merge_cache.end());
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> removed_chunks = this->_old_merge_cache;
    for (auto x : tmp) {
        std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>::iterator it;
        for (it = removed_chunks.begin(); it != removed_chunks.end(); ++it) {
            auto y = *it;
            if (x == y) {
                removed_chunks.erase(it);
                break;
            }
        }
    }
    std::set<std::pair<difflib::chunk_t, difflib::chunk_t>> added_chunks = tmp;
    for (auto x : this->_old_merge_cache) {
        std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>::iterator it;
        for (it = added_chunks.begin(); it != added_chunks.end(); ++it) {
            auto y = *it;
            if (x == y) {
                added_chunks.erase(it);
                break;
            }
        }
    }
    std::pair<difflib::chunk_t, difflib::chunk_t> modified_chunks = this->_changed_chunks;
    if (std::find(removed_chunks.begin(), removed_chunks.end(), modified_chunks) != removed_chunks.end()) {
        modified_chunks = { difflib::EMPTY_CHUNK,  difflib::EMPTY_CHUNK };
    }
    std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>> chunk_changes(removed_chunks, added_chunks, modified_chunks);

    bool mergeable0 = false;
    bool mergeable1 = false;
    for (std::pair<difflib::chunk_t, difflib::chunk_t> p : this->_merge_cache) {
        difflib::chunk_t c0 = p.first;
        difflib::chunk_t c1 = p.second;
        mergeable0 = mergeable0 or (c0 != difflib::EMPTY_CHUNK and std::get<0>(c0) != "conflict");
        mergeable1 = mergeable1 or (c1 != difflib::EMPTY_CHUNK and std::get<0>(c1) != "conflict");
        if (mergeable0 and mergeable1) {
            break;
        }
    }
    this->_has_mergeable_changes = {false, mergeable0, mergeable1, false};

    // Conflicts can only occur when there are three panes, and will always
    // involve the middle pane.
    this->conflicts.clear();
    for (size_t i = 0; i < this->_merge_cache.size(); i++) {
        std::pair<difflib::chunk_t, difflib::chunk_t> p = this->_merge_cache[i];
        difflib::chunk_t c1 = p.first;
        difflib::chunk_t c2 = p.second;
        if ((c1 != difflib::EMPTY_CHUNK and std::get<0>(c1) == "conflict") or
           (c2 != difflib::EMPTY_CHUNK and std::get<0>(c2) == "conflict")) {
            this->conflicts.push_back(i);
        }
    }

    this->_update_line_cache();
    this->signal_diffs_changed().emit(chunk_changes);
}

int _Differ::_find_next(const int last_chunk, int diff, int seq, int current) {
    int next_chunk = -1;
    if (seq == 1 and current + 1 < last_chunk) {
        next_chunk = current + 1;
    } else {
        for (int j = current + 1; j < last_chunk; j++) {
            if ((diff == 0 ? this->_merge_cache[j].first : this->_merge_cache[j].second) != difflib::EMPTY_CHUNK) {
                next_chunk = j;
                break;
            }
        }
    }
    return next_chunk;
}

void _Differ::_update_line_cache() {
    for (size_t i = 0; i < this->seqlength.size(); i++) {
        int l = this->seqlength[i];
        // seqlength + 1 for after-last-line requests, which we do
        std::vector<std::array<int, 3>> tmp;
        for (int i = 0; i < l + 1; i++) {
            tmp.push_back(std::array<int, 3>());
        }
        this->_line_cache[i] = tmp;
    }

    int last_chunk = this->_merge_cache.size();

    std::vector<int> prev = {-1, -1, -1};
    std::vector<int> next = {_find_next(last_chunk, 0, 0, -1), _find_next(last_chunk, 0, 1, -1), _find_next(last_chunk, 1, 2, -1)};
    std::vector<int> old_end = {0, 0, 0};

    for (size_t i = 0; i < this->_merge_cache.size(); i++) {
        std::pair<difflib::chunk_t, difflib::chunk_t> c = this->_merge_cache[i];
        std::vector<std::vector<int>> seq_params = {{0, 0, 3, 4}, {0, 1, 1, 2}, {1, 2, 3, 4}};
        for (std::vector<int> _x : seq_params) {
            int diff = _x[0];
            int seq = _x[1];
            int lo = _x[2];
            int hi = _x[3];
            if ((diff == 0 ? c.first : c.second) == difflib::EMPTY_CHUNK) {
                if (seq == 1) {
                    diff = 1;
                } else {
                    continue;
                }
            }

            int start;
            if (lo == 1) {
                start = std::get<1>(diff == 0 ? c.first : c.second);
            } else if (lo == 2) {
                start = std::get<2>(diff == 0 ? c.first : c.second);
            } else if (lo == 3) {
                start = std::get<3>(diff == 0 ? c.first : c.second);
            } else if (lo == 4) {
                start = std::get<4>(diff == 0 ? c.first : c.second);
            }
            int end;
            if (hi == 1) {
                end = std::get<1>(diff == 0 ? c.first : c.second);
            } else if (hi == 2) {
                end = std::get<2>(diff == 0 ? c.first : c.second);
            } else if (hi == 3) {
                end = std::get<3>(diff == 0 ? c.first : c.second);
            } else if (hi == 4) {
                end = std::get<4>(diff == 0 ? c.first : c.second);
            }
            int last = old_end[seq];
            if (start > last) {
                for (int i = last; i < start; i++) {
                    this->_line_cache[seq][i] = { 0, prev[seq], next[seq] };
                }
            }

            // For insert chunks, claim the subsequent line.
            if (start == end) {
                end += 1;
            }

            next[seq] = _find_next(last_chunk, diff, seq, i);
            for (int i = start; i < end; i++) {
                this->_line_cache[seq][i] = { i, prev[seq], next[seq] };
            }
            prev[seq] = i;
            old_end[seq] = end;
        }
    }

    for (int seq = 0; seq < 3; seq++) {
        int last = old_end[seq];
        int end = this->_line_cache[seq].size();
        if (last < end) {
            for (int i = last; i < end; i++) {
                this->_line_cache[seq][i] = { 0, prev[seq], next[seq] };
            }
        }
    }
}

/*! Offset a chunk by o1/o2 if it's after the inserted lines */
difflib::chunk_t _Differ::offset(const difflib::chunk_t& c, int start, int o1, int o2) {
    if (c == difflib::EMPTY_CHUNK) {
        return difflib::EMPTY_CHUNK;
    }
    size_t start_a = std::get<1>(c) + (std::get<1>(c) > start ? o1 : 0);
    size_t end_a = std::get<2>(c) + (std::get<2>(c) > start ? o1 : 0);
    size_t start_b = std::get<3>(c) + (std::get<3>(c) > start ? o2 : 0);
    size_t end_b = std::get<4>(c) + (std::get<4>(c) > start ? o2 : 0);
    return difflib::chunk_t(std::get<0>(c), start_a, end_a, start_b, end_b);
}

void _Differ::change_sequence(int sequence, int startidx, int sizechange, std::vector<std::string> texts) {
    assert(sequence == 0 || sequence == 1 || sequence == 2);
    if (sequence == 0 or sequence == 1) {
        this->_change_sequence(0, sequence, startidx, sizechange, texts);
    }
    if (sequence == 2 or (sequence == 1 and this->num_sequences == 3)) {
        this->_change_sequence(1, sequence, startidx, sizechange, texts);
    }
    this->seqlength[sequence] += sizechange;

    // Calculate the expected differences in the chunk set if no cascading
    // changes occur, making sure to not include the changed chunk itself
    this->_old_merge_cache.clear();
    this->_changed_chunks = std::pair<difflib::chunk_t, difflib::chunk_t>(difflib::EMPTY_CHUNK, difflib::EMPTY_CHUNK);
    bool chunk_changed = false;
    for (std::pair<difflib::chunk_t, difflib::chunk_t> _x : this->_merge_cache) {
        difflib::chunk_t c1 = _x.first;
        difflib::chunk_t c2 = _x.second;
        if (sequence == 0) {
            if (c1 != difflib::EMPTY_CHUNK and std::get<3>(c1) <= startidx && startidx < std::get<4>(c1)) {
                chunk_changed = true;
            }
            c1 = offset(c1, startidx, 0, sizechange);
        } else if (sequence == 2) {
            if (c2 != difflib::EMPTY_CHUNK and std::get<3>(c2) <= startidx && startidx < std::get<4>(c2)) {
                chunk_changed = true;
            }
            c2 = offset(c2, startidx, 0, sizechange);
        } else {
            // Middle sequence changes alter both chunks
            if (c1 != difflib::EMPTY_CHUNK and std::get<1>(c1) <= startidx && startidx < std::get<2>(c1)) {
                chunk_changed = true;
            }
            c1 = offset(c1, startidx, sizechange, 0);
            if (this->num_sequences == 3) {
                c2 = offset(c2, startidx, sizechange, 0);
            }
        }
        if (chunk_changed) {
#if 0
            assert(not this->_changed_chunks);
#endif
            this->_changed_chunks = std::pair<difflib::chunk_t, difflib::chunk_t>(c1, c2);
            chunk_changed = false;
        }
        this->_old_merge_cache.insert(std::pair<difflib::chunk_t, difflib::chunk_t>(c1, c2));
    }

    this->_update_merge_cache(texts);
}

/*! Find the index of the chunk which contains line. */
int _Differ::_locate_chunk(int whichdiffs, int sequence, int line) {
    int high_index = 2 + 2 * int(sequence != 1);
    difflib::chunk_list_t _x = whichdiffs == 0 ? this->diffs.first : this->diffs.second;
    for (size_t i = 0; i < _x.size(); i++) {
        difflib::chunk_t c = _x[i];
        int tmp;
        if (high_index == 1) {
            tmp = std::get<1>(c);
        } else if (high_index == 2) {
            tmp = std::get<2>(c);
        } else if (high_index == 3) {
            tmp = std::get<3>(c);
        } else if (high_index == 4) {
            tmp = std::get<4>(c);
        }
        if (line < tmp) {
            return i;
        }
    }
    return (whichdiffs == 0 ? this->diffs.first : this->diffs.second).size();
}

/*!
    Return the index-th change in from_pane

    If to_pane is provided, then only changes between from_pane and to_pane
    are considered, otherwise all changes starting at from_pane are used.
 */
difflib::chunk_t _Differ::get_chunk(int index, int from_pane, int to_pane) {
    int sequence = int(from_pane == 2 or to_pane == 2);
    difflib::chunk_t chunk = sequence == 0 ? this->_merge_cache[index].first : this->_merge_cache[index].second;
    if (from_pane == 0 || from_pane == 2) {
        if (chunk == difflib::EMPTY_CHUNK) {
            return difflib::EMPTY_CHUNK;
        }
        return reverse_chunk(chunk);
    } else {
        if (!to_pane and chunk == difflib::EMPTY_CHUNK) {
            chunk = this->_merge_cache[index].second;
        }
        return chunk;
    }
}

/*! Find the index of the chunk which contains line. */
std::array<int, 3> _Differ::locate_chunk(int pane, int line) {
    if (pane >= 0 && pane < 3 && line < this->_line_cache[pane].size()) {
        return this->_line_cache[pane][line];
    } else {
        return {-1, -1, -1};
    }
}

int _Differ::diff_count() {
    return this->_merge_cache.size();
}

std::pair<bool, bool> _Differ::has_mergeable_changes(int which) {
    return std::pair<bool, bool>(this->_has_mergeable_changes[which], this->_has_mergeable_changes[which + 1]);
}

difflib::chunk_t _Differ::offset(const difflib::chunk_t& c, int o1, int o2) {
    return difflib::chunk_t(std::get<0>(c), std::get<1>(c) + o1, std::get<2>(c) + o1,
                                            std::get<3>(c) + o2, std::get<4>(c) + o2);
}

void _Differ::_change_sequence(int which, int sequence, int startidx, int sizechange, std::vector<std::string> texts) {
    difflib::chunk_list_t diffs = which == 0 ? this->diffs.first : this->diffs.second;
    std::array<int, 3> lines_added = {0, 0, 0};
    lines_added[sequence] = sizechange;
    int loidx = this->_locate_chunk(which, sequence, startidx);
    int hiidx;
    if (sizechange < 0) {
        hiidx = this->_locate_chunk(which, sequence, startidx - sizechange);
    } else {
        hiidx = loidx;
    }
    std::pair<int, int> lorange;
    if (loidx > 0) {
        loidx -= 1;
        lorange = std::pair<int, int>(std::get<3>(diffs[loidx]), std::get<1>(diffs[loidx]));
    } else {
        lorange = std::pair<int, int>(0, 0);
    }
    int x = which * 2;
    std::pair<int, int> hirange;
    if (hiidx < diffs.size()) {
        hiidx += 1;
        hirange = std::pair<int, int>(std::get<4>(diffs[hiidx - 1]), std::get<2>(diffs[hiidx - 1]));
    } else {
        hirange = std::pair<int, int>(this->seqlength[x], this->seqlength[1]);
    }
    std::pair<int, int> rangex = std::pair<int, int>(lorange.first, hirange.first + lines_added[x]);
    std::pair<int, int> range1 = std::pair<int, int>(lorange.second, hirange.second + lines_added[1]);
    assert(rangex.first <= rangex.second and range1.first <= range1.second);
    std::string linesx = texts[x].substr(rangex.first, rangex.second);
    std::string lines1 = texts[1].substr(range1.first, range1.second);

    MyersSequenceMatcher tmp(lines1, linesx);
    difflib::chunk_list_t newdiffs = tmp.get_difference_opcodes();
    for (size_t i = 0; i < newdiffs.size(); i++) {
        difflib::chunk_t c = newdiffs[i];
        newdiffs[i] = offset(c, range1.first, rangex.first);
    }

#if 0
    if (hiidx < len(this->diffs[which])) {
        offset_diffs = [offset(c, lines_added[1], lines_added[x]) for c
                                              in this->diffs[which][hiidx:]];
        this->diffs[which][hiidx:] = offset_diffs;
    }
    this->diffs[which][loidx:hiidx] = newdiffs;
#endif
}

std::pair<int, int> _Differ::_range_from_lines(int textindex, std::pair<int, int> lines) {
    int lo_line = lines.first;
    int hi_line = lines.second;
    std::array<int, 3> top_chunk = this->locate_chunk(textindex, lo_line);
    int start = top_chunk[0];
    if (!start) {
        start = top_chunk[2];
    }
    std::array<int, 3> bottom_chunk = this->locate_chunk(textindex, hi_line);
    int end = bottom_chunk[0];
    if (!end) {
        end = bottom_chunk[1];
    }
    return std::pair<int, int>(start, end);
}

std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _Differ::all_changes() {
    return this->_merge_cache;
}

/*! Give all changes between file1 and either file0 or file2. */
difflib::chunk_list_t _Differ::pair_changes(int fromindex, int toindex, std::vector<int> lines) {
    difflib::chunk_list_t result;

    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> merge_cache;
    if (std::find(lines.begin(), lines.end(), 0) == lines.end()) {
        std::pair<int, int> tmp = this->_range_from_lines(fromindex, std::pair<int, int>(lines[0], lines[1]));
        int start1 = tmp.first;
        int end1 = tmp.second;
        tmp = this->_range_from_lines(toindex, std::pair<int, int>(lines[2], lines[3]));
        int start2 = tmp.first;
        int end2 = tmp.second;
        if ((start1 < 0 or end1 < 0) and
            (start2 < 0 or end2 < 0)) {
            return result;
        }
#if 0
        start = min([x for x in (start1, start2) if x is not None]);
        end = max([x for x in (end1, end2) if x is not None]);
        merge_cache = this->_merge_cache[start:end + 1];
#endif
    } else {
        merge_cache = this->_merge_cache;
    }

    if (fromindex == 1) {
        int seq = toindex; // 2
        for (std::pair<difflib::chunk_t, difflib::chunk_t> c : merge_cache) {
            if ((seq == 0 ? c.first : c.second) != difflib::EMPTY_CHUNK) {
                result.push_back(seq == 0 ? c.first : c.second);
            }
        }
    } else {
        int seq = fromindex; // 2
        for (std::pair<difflib::chunk_t, difflib::chunk_t> c : merge_cache) {
            if ((seq == 0 ? c.first : c.second) != difflib::EMPTY_CHUNK) {
                result.push_back(reverse_chunk(seq == 0 ? c.first : c.second));
            }
        }
    }
    return result;
}

/*! Give changes for single file only. do not return 'equal' hunks. */
difflib::chunk_list_t _Differ::single_changes(int textindex, std::pair<int, int> lines) {
    difflib::chunk_list_t result;
#if 0
    if (None not in lines) {
        std::pair<int, int> tmp = this->_range_from_lines(textindex, lines);
        start = tmp.first;
        end = tmp.second;
        if (start < 0 or end < 0) {
            return difflib::EMPTY_CHUNK;
        }
        merge_cache = this->_merge_cache[start:end + 1];
    } else {
        merge_cache = this->_merge_cache;
    }
    if (textindex == 0 || textindex == 2) {
        int seq = textindex; // 2
        for (int cs : merge_cache) {
            if (cs[seq]) {
                result.push_back(reverse_chunk(cs[seq]));
            }
        }
    } else {
        for (int cs : merge_cache) {
            result.push_back(cs[0] or cs[1]);
        }
    }
#endif
    return result;
}

bool _Differ::sequences_identical() {
    // check so that we don't call an uninitialised comparison 'identical';
    return this->diffs.first.empty() && this->diffs.second.empty() and this->_initialised;
}

std::array<int, 6> _Differ::_merge_blocks(std::array<difflib::chunk_list_t, 2> _using) {
    const int LO = 1;
    const int HI = 2;
    int lowc = std::min(std::get<LO>(_using[0][0]), std::get<LO>(_using[1][0]));
    int highc = std::max(std::get<HI>(_using[0][-1]), std::get<HI>(_using[1][-1]));
    std::vector<int> low;
    std::vector<int> high;
    for (int i = 0; i < 2; i++) {
        difflib::chunk_t d = _using[i][0];
        low.push_back(lowc - std::get<LO>(d) + std::get<2 + LO>(d));
        d = _using[i][-1];
        high.push_back(highc - std::get<HI>(d) + std::get<2 + HI>(d));
    }
    return std::array<int, 6>{low[0], high[0], lowc, highc, low[1], high[1]};
}

/*! Automatically merge two sequences of change blocks */
std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _Differ::_auto_merge(std::array<difflib::chunk_list_t, 2> _using, std::vector<std::string> texts) {
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> result;
    for (;;) { // FIXME: unknown because yield
        int l0, h0, l1, h1, l2, h2;
        std::array<int, 6> tmp = this->_merge_blocks(_using);
        l0 = tmp[0];
        h0 = tmp[1];
        l1 = tmp[2];
        h1 = tmp[3];
        l2 = tmp[4];
        h2 = tmp[5];
        std::string tag;
#if 0
        if (h0 - l0 == h2 - l2 and texts[0][l0:h0] == texts[2][l2:h2]) {
            if (l1 != h1 and l0 == h0) {
                tag = "delete";
            } else if (l1 != h1) {
                tag = "replace";
            } else {
                tag = "insert";
            }
        } else {
#endif
            tag = "conflict";
#if 0
        }
#endif
        difflib::chunk_t out0(tag, l1, h1, l0, h0);
        difflib::chunk_t out1(tag, l1, h1, l2, h2);
        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
    }
    return result;
}

std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _Differ::_merge_diffs(difflib::chunk_list_t seq0, difflib::chunk_list_t seq1, std::vector<std::string> texts) {
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> result;
    std::vector<difflib::chunk_list_t>seq = {seq0, seq1};
#if 0
    while (seq0.size() or seq1.size()) {
        int high_seq;
        if (seq0.empty()) {
            high_seq = 1;
        } else if (seq1.empty()) {
            high_seq = 0;
        } else {
            high_seq = int(std::get<1>(seq0[0]) > std::get<1>(seq1[0]));
            if (std::get<1>(seq0[0]) == std::get<1>(seq1[0])) {
                if (std::get<0>(seq0[0]) == "insert") {
                    high_seq = 0;
                } else if (std::get<0>(seq1[0]) == "insert") {
                    high_seq = 1;
                }
            }
        }

        high_diff = seq[high_seq].pop(0);
        int high_mark = std::get<2>(high_diff);
        int other_seq;
        if (high_seq == 1) {
            other_seq = 0;
        } else {
            other_seq = 1;
        }

        std::array<difflib::chunk_list_t, 2> _using;
        _using[high_seq].push_back(high_diff);

        while (seq[other_seq]) {
            difflib::chunk_t other_diff = seq[other_seq][0];
            if (high_mark < std::get<1>(other_diff)) {
                break;
            }
            if (high_mark == std::get<1>(other_diff) and
               not (std::get<0>(high_diff) == std::get<0>(other_diff) == "insert")) {
                break;
            }

            _using[other_seq].push_back(other_diff);
            seq[other_seq].pop(0);

            if (high_mark < std::get<2>(other_diff)) {
                high_seq, other_seq = other_seq, high_seq;
                high_mark = std::get<2>(other_diff);
            }
        }

        if (_using[0].size() == 0) {
            assert(_using[1].size() == 1);
            result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(nullptr, _using[1][0]);
        } else if (_using[1].size() == 0) {
            assert(_using[0].size() == 1);
            result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(_using[0][0], nullptr));
        } else {
            for (std::pair<difflib::chunk_t, difflib::chunk_t> c : this->_auto_merge(_using, texts)) {
                result.push_back(c);
            }
        }
    }
#endif
    return result;
}

void _Differ::set_sequences_iter(std::vector<BufferLines*> sequences) {
    assert(0 <= sequences.size() && sequences.size() <= 3);
    this->diffs.first.clear();
    this->diffs.second.clear();
    this->num_sequences = sequences.size();
    this->seqlength.clear();
#if 0
    for (std::string s : sequences) {
        this->seqlength.push_back(s.length());
    }

    for (int i = 0; i < this->num_sequences - 1; i++) {
        MyersSequenceMatcher<std::string>* matcher;
        if (!this->syncpoints.empty()) {
            std::vector<std::pair<int, int>> syncpoints;
            for (int s : this->syncpoints) {
                syncpoints.push_back(std::pair<int, int>(s[i][0](), s[i][1]()));
            }
            matcher = new SyncPointMyersSequenceMatcher<MeldBuffer*>(0,
                                         sequences[1], sequences[i * 2],
                                         syncpoints);
        } else {
            matcher = new MyersSequenceMatcher<MeldBuffer*>(0, sequences[1], sequences[i * 2]);
        }
        matcher->initialise();
        this->diffs[i] = matcher.get_difference_opcodes();
    }
    this->_initialised = true;
    this->_update_merge_cache(sequences);
#endif
}

void _Differ::clear() {
    this->diffs.first.clear();
    this->diffs.second.clear();
    this->seqlength.clear();
    for (int i = 0; i < this->num_sequences; i++) {
        this->seqlength.push_back(0);
    }
    this->_initialised = false;
    this->_old_merge_cache.clear();
    std::vector<std::string> tmp;
    for (int i = 0; i < this->num_sequences; i++) {
        tmp.push_back("");
    }
    this->_update_merge_cache(tmp);
}
