/* Copyright (C) 2009-2010 Piotr Piastucki <the_leech@users.berlios.de>
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

#ifndef __MELD__MERGE_H__
#define __MELD__MERGE_H__

#include "diffutil.h"
#include "meldbuffer.h"
#include <vector>
#include <string>

class Merger;

class AutoMergeDiffer : public _Differ {
    friend class Merger;
protected:
    bool auto_merge;
public:
    AutoMergeDiffer();
    virtual ~AutoMergeDiffer();

    virtual std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> _auto_merge(std::array<difflib::chunk_list_t, 2> _using, std::vector<std::string> texts);

    virtual void change_sequence(int sequence, int startidx, int sizechange, std::vector<std::string> texts);

    int get_unresolved_count();
};

class Merger : public _Differ {
public:
    _Differ* differ;
    std::vector<BufferLines*> texts;

    Merger();
    virtual ~Merger();

    void initialize(std::vector<BufferLines*> sequences, std::vector<BufferLines*> texts);

    int _apply_change(BufferLines* text, difflib::chunk_t change, std::vector<Glib::ustring>& mergedtext);

    Glib::ustring merge_3_files(bool mark_conflicts = true);

    Glib::ustring merge_2_files(int fromindex, int toindex);
};

#endif
