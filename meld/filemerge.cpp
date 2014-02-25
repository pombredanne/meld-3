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

#include "filemerge.h"
#include "recent.h"
#include "meldbuffer.h"
#include "conf.h"

FileMerge::FileMerge(int num_panes, SchedulerBase& scheduler) : FileDiff(num_panes, scheduler) {
}

FileMerge::~FileMerge() {
}

void FileMerge::_connect_buffer_handlers() {
    FileDiff::_connect_buffer_handlers();
    this->textview[0]->set_editable(0);
    this->textview[2]->set_editable(0);
}

std::pair<std::string, std::vector<std::string>> FileMerge::get_comparison() {
    std::pair<std::string, std::vector<std::string>> comp = FileDiff::get_comparison();
    return std::pair<std::string, std::vector<std::string>>(TYPE_MERGE, comp.second);
}

void FileMerge::_set_files_internal(std::vector<std::string> files) {
    Glib::RefPtr<MeldBuffer> tmp(new MeldBuffer());
    this->textview[1]->set_buffer(tmp);
    this->_load_files(files, this->textbuffer);
    this->_merge_files();
    this->textview[1]->set_buffer(this->textbuffer[1]);
    this->_diff_files();
}

void FileMerge::_merge_files() {
#if 0
    yield _("[%s] Merging files") % this->label_text;
#endif
    Merger merger;
    merger.initialize(this->buffer_filtered, this->buffer_texts);
#if 0
    while (next(step) is None) {
        yield 1;
    }
#endif
    Glib::ustring merged_text = merger.merge_3_files();
    this->linediffer->unresolved = merger.unresolved;
    this->textbuffer[1]->set_text(merged_text);
    this->textbuffer[1]->data->modified = true;
    this->recompute_label();
}
