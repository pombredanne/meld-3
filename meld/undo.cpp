/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010-2011 Kai Willadsen <kai.willadsen@gmail.com>
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

/*!
 * \file
 * Module to help implement undo functionality.
 *
 * Usage:
 *
 * t = TextWidget()
 * s = UndoSequence()
 * def on_textwidget_text_inserted():
 *     s.begin_group()
 *     if not t.is_modified():
 *         s.add_action( TextWidgetModifiedAction() )
 *     s.add_action( InsertionAction() )
 *     s.end_group()
 *
 * def on_undo_button_pressed():
 *     s.undo()
 */

#include "undo.h"

GroupAction::GroupAction(UndoSequence &seq) : seq(seq) {
    // TODO: If a GroupAction affects more than one sequence, our logic
    // breaks. Currently, this isn't a problem.
    this->buffer = seq.actions[0]->buffer;
}

void GroupAction::undo() {
    while (this->seq.can_undo()) {
        this->seq.undo();
    }
}

void GroupAction::redo() {
    while (this->seq.can_redo()) {
        this->seq.redo();
    }
}

UndoSequence::UndoSequence() : Glib::Object() {
    this->actions.clear();
    this->next_redo = 0;
    this->checkpoints.clear();
    this->group = 0;
    this->busy = false;
}

void UndoSequence::clear() {
    assert(this->group);
    if (this->can_undo()) {
        this->m_signal_can_undo.emit(0);
    }
    if (this->can_redo()) {
        this->m_signal_can_redo.emit(0);
    }
    this->actions.clear();
    this->next_redo = 0;
    this->checkpoints = {};
}

bool UndoSequence::can_undo() {
    return this->next_redo > 0;
}

bool UndoSequence::can_redo() {
    return this->next_redo < (int) this->actions.size();
}

void UndoSequence::add_action(GroupAction *action) {
    if (this->busy) {
        return;
    }

    if (!this->group) {
        if (this->checkpointed(action->buffer)) {
            this->checkpoints[action->buffer].second = this->next_redo;
            this->m_signal_checkpointed.emit(action->buffer, false);
        } else {
            // If we go back in the undo stack before the checkpoint starts,
            // and then modify the buffer, we lose the checkpoint altogether
            int start, end;
            if (this->checkpoints.count(action->buffer) > 0) {
                std::pair<int, int> p = this->checkpoints[action->buffer];
                start = p.first;
                end = p.second;
            } else {
                start = -1;
                end = -1;
            }
            if (start >= 0 and start > this->next_redo) {
                this->checkpoints[action->buffer] = std::pair<int, int>(-1, -1);
            }
        }
        bool could_undo = this->can_undo();
        bool could_redo = this->can_redo();
#if 0
        this->actions[this->next_redo:] = [];
#endif
        this->actions.push_back(action);
        this->next_redo += 1;
        if (not could_undo) {
            this->m_signal_can_undo.emit(1);
        }
        if (could_redo) {
            this->m_signal_can_redo.emit(0);
        }
    } else {
        this->group->add_action(action);
    }
}

void UndoSequence::undo() {
    assert(this->next_redo > 0);
    this->busy = true;
    Glib::RefPtr<MeldBuffer> buf = this->actions[this->next_redo - 1]->buffer;
    if (this->checkpointed(buf)) {
        this->m_signal_checkpointed.emit(buf, false);
    }
    bool could_redo = this->can_redo();
    this->next_redo -= 1;
    this->actions[this->next_redo]->undo();
    this->busy = false;
    if (not this->can_undo()) {
        this->m_signal_can_undo.emit(0);
    }
    if (not could_redo) {
        this->m_signal_can_redo.emit(1);
    }
    if (this->checkpointed(buf)) {
        this->m_signal_checkpointed.emit(buf, true);
    }
}

void UndoSequence::redo() {
    assert(this->next_redo < this->actions.size());
    this->busy = true;
    Glib::RefPtr<MeldBuffer> buf = this->actions[this->next_redo]->buffer;
    if (this->checkpointed(buf)) {
        this->m_signal_checkpointed.emit(buf, false);
    }
    bool could_undo = this->can_undo();
    GroupAction* a = this->actions[this->next_redo];
    this->next_redo += 1;
    a->redo();
    this->busy = false;
    if (not could_undo) {
        this->m_signal_can_undo.emit(1);
    }
    if (not this->can_redo()) {
        this->m_signal_can_undo.emit(0);
    }
    if (this->checkpointed(buf)) {
        this->m_signal_checkpointed.emit(buf, true);
    }
}

void UndoSequence::checkpoint(Glib::RefPtr<MeldBuffer> buf) {
    int start = this->next_redo;
    while (start > 0 and this->actions[start - 1]->buffer != buf) {
        start -= 1;
    }
    int end = this->next_redo;
    while (end < this->actions.size() - 1 and
           this->actions[end + 1]->buffer != buf) {
        end += 1;
    }
    if (end == this->actions.size()) {
        end = -1;
    }
    this->checkpoints[buf] = std::pair<int, int>(start, end);
    this->m_signal_checkpointed.emit(buf, true);
}

bool UndoSequence::checkpointed(Glib::RefPtr<MeldBuffer> buf) {
    // While the main undo sequence should always have checkpoints
    // recorded, grouped subsequences won't.
    int start, end;
    if (this->checkpoints.count(buf) > 0) {
        std::pair<int, int> p = this->checkpoints[buf];
        start = p.first;
        end = p.second;
    } else {
        start = -1;
        end = -1;
    }
    if (start < 0) {
        return false;
    }
    if (end < 0) {
        end = this->actions.size();
    }
    return start <= this->next_redo <= end;
}

void UndoSequence::begin_group() {
    if (this->busy) {
        return;
    }

    if (this->group) {
        this->group->begin_group();
    } else {
        this->group = new UndoSequence();
    }
}

void UndoSequence::end_group() {
    if (this->busy) {
        return;
    }

    assert(this->group);
    if (this->group->group) {
        this->group->end_group();
    } else {
        UndoSequence* group = this->group;
        this->group = 0;
        // Collapse single action groups
        if (group->actions.size() == 1) {
            this->add_action(group->actions[0]);
        } else if (group->actions.size() > 1) {
            this->add_action(new GroupAction(*group));
        }
    }
}

void UndoSequence::abort_group() {
    if (this->busy) {
        return;
    }

    assert(this->group);
    if (this->group->group) {
        this->group->abort_group();
    } else {
        this->group = 0;
    }
}

bool UndoSequence::in_grouped_action() {
    return this->group;
}
