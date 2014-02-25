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

#ifndef __MELD__UNDO_H__
#define __MELD__UNDO_H__

#include <gtkmm.h>

#include "meldbuffer.h"

class UndoSequence;
class MeldBuffer;

/*! A group action combines several actions into one logical action. */
class GroupAction {
    friend class UndoSequence;
private:
    Glib::RefPtr<MeldBuffer> buffer;
    UndoSequence &seq;
public:
    GroupAction(UndoSequence &seq);

    void undo();

    void redo();
};

/*! A manager class for operations which can be undone/redone. */
class UndoSequence : Glib::Object {
    friend class GroupAction;
public:
    typedef sigc::signal<void, bool> type_signal_can_undo;
    type_signal_can_undo signal_can_undo() {
        return m_signal_can_undo;
    }
    type_signal_can_undo m_signal_can_undo;
    typedef sigc::signal<void, bool> type_signal_can_redo;
    type_signal_can_redo signal_can_redo() {
        return m_signal_can_redo;
    }
    type_signal_can_redo m_signal_can_redo;
    typedef sigc::signal<void, Glib::RefPtr<MeldBuffer>, bool> type_signal_checkpointed;
    type_signal_checkpointed signal_checkpointed() {
        return m_signal_checkpointed;
    }
    type_signal_checkpointed m_signal_checkpointed;
private:
    std::vector<GroupAction *> actions;
    int next_redo;
    std::map<Glib::RefPtr<MeldBuffer>, std::pair<int, int> > checkpoints;
    UndoSequence *group;
    bool busy;

public:
    /*! Create an empty UndoSequence. */
    UndoSequence();

    /*!
     * Remove all undo and redo actions from this sequence
     *
     * If the sequence was previously able to undo and/or redo, the
     * 'can-undo' and 'can-redo' signals are emitted.
     *
     * Raises an AssertionError if a group is in progress.
     */
    void clear();

    /*! Return if an undo is possible. */
    bool can_undo();

    /*! Return if a redo is possible. */
    bool can_redo();

    /*!
     * Add an action to the undo list.
     *
     * Arguments:
     *
     * action -- A class with two callable attributes: 'undo' and 'redo'
     *           which are called by this sequence during an undo or redo.
     */
    void add_action(GroupAction *action);

    /*!
     * Undo an action.
     *
     * Raises an AssertionError if the sequence is not undoable.
     */
    void undo();

    /*!
     * Redo an action.
     *
     * Raises and AssertionError if the sequence is not undoable.
     */
    void redo();

    void checkpoint(Glib::RefPtr<MeldBuffer> buf);

    bool checkpointed(Glib::RefPtr<MeldBuffer> buf);

    /*!
     * Group several actions into a single logical action.
     *
     * When you wrap several calls to add_action() inside begin_group()
     * and end_group(), all the intervening actions are considered
     * one logical action. For instance a 'replace' action may be
     * implemented as a pair of 'delete' and 'create' actions, but
     * undoing should undo both of them.
     */
    void begin_group();

    /*!
     * End a logical group action. See also begin_group().
     *
     * Raises an AssertionError if there was not a matching call to
     * begin_group().
     */
    void end_group();

    /*!
     * Revert the sequence to the state before begin_group() was called.
     *
     * Raises an AssertionError if there was no a matching call to begin_group().
     */
    void abort_group();

    bool in_grouped_action();

private:
    void emit(std::string, int, bool = false);
};

#endif
