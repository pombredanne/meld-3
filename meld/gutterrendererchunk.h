/* Copyright (C) 2013-2014 Kai Willadsen <kai.willadsen@gmail.com>
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
#include <gtksourceviewmm.h>

#include "filediff.h"
#include "const.h"

// Fixed size of the renderer. Ideally this would be font-dependent and
// would adjust to other textview attributes, but that's both quite difficult
// and not necessarily desirable.
const int LINE_HEIGHT = 16;

extern Glib::RefPtr<Gdk::Pixbuf> load(std::string icon_name);

class GutterRendererChunkAction : public Gsv::GutterRendererPixbuf {
public:
    std::map<std::string, std::map<int, Glib::RefPtr<Gdk::Pixbuf>>> ACTION_MAP = {
        {"LTR", {
            { MODE_REPLACE, load("meld-change-apply-right") },
            { MODE_DELETE, load("meld-change-delete") },
            { MODE_INSERT, load("meld-change-copy") }
        }},
        {"RTL", {
            { MODE_REPLACE, load("meld-change-apply-left") },
            { MODE_DELETE, load("meld-change-delete") },
            { MODE_INSERT, load("meld-change-copy") }
        }}
    };
private:

    int from_pane;
    int to_pane;
    std::pair<Gtk::TextView*, Gtk::TextView*> views;
    _Differ* linediffer;
    int mode;
    std::map<int, Glib::RefPtr<Gdk::Pixbuf>> action_map;
    FileDiff* filediff;
public:
    GutterRendererChunkAction(int from_pane, int to_pane, std::pair<Gtk::TextView*, Gtk::TextView*> views, FileDiff* filediff, _Differ* linediffer);
    virtual void on_activate(const Gtk::TextIter& start, const Gdk::Rectangle& area, GdkEvent* event);
    void copy_chunk(const difflib::chunk_t& chunk, bool copy_up);
    Gtk::Menu* _make_copy_menu(const difflib::chunk_t& chunk);
    virtual bool on_query_activatable(const Gtk::TextIter& start, const Gdk::Rectangle& area, GdkEvent* event);
    virtual void on_query_data(const Gtk::TextIter& start, const Gtk::TextIter& end, Gsv::GutterRendererState state);
    void on_container_mode_changed(int mode);
    /*!
     * Classify possible actions for the given change
     *
     * Returns the action that can be performed given the content and
     * context of the change.
     */
    int _classify_change_actions(const difflib::chunk_t& change);
};
