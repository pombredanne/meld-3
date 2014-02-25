/* Copyright (C) 2009, 2012 Piotr Piastucki <the_leech@users.berlios.de>
 * Copyright (C) 2012 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__FILEMERGE_H__
#define __MELD__FILEMERGE_H__

#include "filediff.h"
#include "merge.h"

class FileMerge : public FileDiff {
private:
    AutoMergeDiffer differ;
public:
    FileMerge(int num_panes, SchedulerBase& scheduler);
    virtual ~FileMerge();

    virtual void _connect_buffer_handlers();

    virtual std::pair<std::string, std::vector<std::string>> get_comparison();

    virtual void _set_files_internal(std::vector<std::string> files);

    void _merge_files();
};


#endif
