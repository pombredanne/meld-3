/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__TASK_H__
#define __MELD__TASK_H__

/*! \file Classes to implement scheduling for cooperative threads. */

#include <deque>
#include <vector>
#include <functional>
#include <sigc++/signal.h>

/*!
 * Base class with common functionality for schedulers
 *
 * Derived classes must implement get_current_task.
 */
class SchedulerBase {
protected:

    std::deque<std::function<void()>> tasks;
    std::vector<std::function<void(SchedulerBase*)>> callbacks;

public:

    typedef sigc::signal<void> type_signal_runnable;
    type_signal_runnable signal_runnable() {
        return m_signal_runnable;
    }
    type_signal_runnable m_signal_runnable;

    SchedulerBase();

#if 0
    std::string __repr__();
#endif

    void connect(std::string signal, std::function<void(SchedulerBase*)> action);

    /*!
     * Add a task to the scheduler's task list
     *
     * The task may be a function, generator or scheduler, and is
     * deemed to have finished when it returns a false value or raises
     * StopIteration.
     */
    void add_task(std::function<void()> task, bool atfront = false);

    /*! Remove a single task from the scheduler */
    void remove_task(std::function<void()> task);

    /*! Remove all tasks from the scheduler */
    void remove_all_tasks();

    /*! Overridden function returning the next task to run */
    virtual std::function<void()> get_current_task() = 0;

    virtual void remove_current_task() = 0;

    /*! Run an iteration of the current task */
    int __call__();

    /*! Run all of the scheduler's current tasks to completion */
    void complete_tasks();

    bool tasks_pending();

    /*! Perform one iteration of the current task */
    int iteration();
};


/*! Scheduler calling most recently added tasks first */
class LifoScheduler : public SchedulerBase {
public:
    virtual std::function<void()> get_current_task();
    virtual void remove_current_task();
};


/*! Scheduler calling tasks in the order they were added */
class FifoScheduler : public SchedulerBase {
public:
    virtual std::function<void()> get_current_task();
    virtual void remove_current_task();
};

#endif
