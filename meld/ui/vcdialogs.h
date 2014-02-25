/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__UI__VCDIALOGS_H__
#define __MELD__UI__VCDIALOGS_H__

#include <libintl.h>
#define _(str) gettext(str)

#include <gtkmm.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>

#include "../misc.h"
#include "../settings.h"
#include "gnomeglade.h"
#include "../vcview.h"
#include "historyentry.h"

class CommitDialog : public Glib::Object, public Component {
private:
    const std::string __gtype_name__ = "CommitDialog";

#if 0
    Glib::Property<bool> break_commit_message(_default=false);
#endif

    VcView* parent;

    Gtk::Label* changedfiles;
    Glib::RefPtr<Gtk::TextView> textview;

    HistoryCombo* previousentry;
    Gtk::ScrolledWindow* scrolledwindow1;
public:
    CommitDialog(VcView* parent);

    void run();

    void on_previousentry_activate(HistoryCombo* gentry);
};


class PushDialog : public Component {
private:
    VcView* parent;
public:
    PushDialog(VcView* parent);
    void run();
};


#endif
