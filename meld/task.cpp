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

#include <cassert>
#include <string>
#include <algorithm>

#include "util/compat.h"

#include "task.h"

#if 0
from __future__ import print_function

import traceback
#endif

SchedulerBase::SchedulerBase() {
}

#if 0
std::string SchedulerBase::__repr__() {
    std::stringstream ss;
    for (int task : this->tasks) {
        ss << task << ", ";
    }
    return ss.str();
}
#endif

void SchedulerBase::connect(std::string signal, std::function<void(SchedulerBase*)> action) {
    assert(signal == "runnable");
#if 0
    if (std::find(this->callbacks.begin(), this->callbacks.end(), action) == this->callbacks.end()) {
        this->callbacks.push_back(action);
    }
#endif
}

void SchedulerBase::add_task(std::function<void()> task, bool atfront) {
    this->remove_task(task);

    if (atfront) {
        this->tasks.push_front(task);
    } else {
        this->tasks.push_back(task);
    }

    for (std::function<void(SchedulerBase*)> callback : this->callbacks) {
        callback(this);
    }
}

void SchedulerBase::remove_task(std::function<void()> task) {
#if 0
    auto i = std::find(this->tasks.begin(), this->tasks.end(), task);
    if (i != this->tasks.end()) {
        this->tasks.erase(i);
    }
#endif
}

void SchedulerBase::remove_all_tasks() {
    this->tasks.clear();
}

int SchedulerBase::__call__() {
    if (this->tasks.size()) {
        int r = this->iteration();
        if (r) {
            return r;
        }
    }
    return this->tasks_pending();
}

void SchedulerBase::complete_tasks() {
    while (this->tasks_pending()) {
        this->iteration();
    }
}

bool SchedulerBase::tasks_pending() {
    return !this->tasks.empty();
}

int SchedulerBase::iteration() {
    std::function<void()> task;
    try {
        task = this->get_current_task();
    } catch (StopIteration &e) {
        return 0;
    }
    task();

    this->remove_current_task();
    return 0;
}


std::function<void()> LifoScheduler::get_current_task() {
    if (this->tasks.empty()) {
        throw StopIteration();
    }
    return this->tasks.back();
}

void LifoScheduler::remove_current_task() {
    if (this->tasks.empty()) {
        return;
    }
    this->tasks.erase(this->tasks.begin() + this->tasks.size());
}


std::function<void()> FifoScheduler::get_current_task() {
    if (this->tasks.empty()) {
        throw StopIteration();
    }
    return this->tasks[0];
}

void FifoScheduler::remove_current_task() {
    if (this->tasks.empty()) {
        return;
    }
    this->tasks.erase(this->tasks.begin());
}
