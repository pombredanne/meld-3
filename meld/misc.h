/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009 Vincent Legoll <vincent.legoll@gmail.com>
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

#ifndef __MELD__MISC_H__
#define __MELD__MISC_H__

#include <gtkmm.h>
#include "ui/gnomeglade.h"

/*! A common error dialog handler for Meld.
 *
 * This should only ever be used as a last resort, and for errors that
 * a user is unlikely to encounter. If you're tempted to use this,
 * think twice.
 *
 * Primary must be plain text. Secondary must be valid markup.
 */
extern void error_dialog(const Glib::ustring& primary, const Glib::ustring& secondary, Gtk::Window *parent = 0, Gtk::MessageType messagetype = Gtk::MESSAGE_ERROR);

/*!
 * A common message dialog handler for Meld
 *
 * This should only ever be used for interactions that must be resolved
 * before the application flow can continue.
 *
 * Primary must be plain text. Secondary must be valid markup.
 */
extern int modal_dialog(const Glib::ustring& primary, const Glib::ustring& secondary, std::map<const Glib::ustring, Gtk::ResponseType> buttons, Gtk::Window* parent = nullptr,
                        Gtk::MessageType messagetype = Gtk::MESSAGE_WARNING);

/*!
 * Run a dialog with text 'text'.
 *
 * Extra buttons are passed as tuples of (button label, response id).
 */
extern int run_dialog(const Glib::ustring& text, Gtk::Window *parent = 0, Gtk::MessageType messagetype = Gtk::MESSAGE_WARNING, Gtk::ButtonsType buttonstype = Gtk::BUTTONS_OK, std::map<const Gtk::StockID*, int> extrabuttons = {});

/* Taken from epiphany */
extern std::tuple<int, int, bool> position_menu_under_widget(Gtk::Menu &menu, Gtk::Widget &widget);

/*!
 * Make a GtkToolButton label-widget suggestive of a menu dropdown
 */
extern std::unique_ptr<Gtk::HBox> make_tool_button_widget(const Glib::ustring& label);

extern Gdk::Color gdk_to_cairo_color(Gdk::Color color);

/*!
 * Return true if all members of the list are equal to the first.
 *
 * An empty list is considered to have all elements equal.
 */
extern void all_equal(int alist);

/*!
 * Remove redunant parts of a list of names (e.g. /tmp/foo{1,2} -> foo{1,2}
 */
extern std::vector<std::string> shorten_names(std::vector<std::string> names);

class sentinel {
public:
    sentinel();

    ~sentinel();

    void __call__();
};

/*!
 * Read the output of a shell command iteratively.
 *
 * Each time 'callback_interval' seconds pass without reading any data,
 * this function yields None.
 * When all the data is read, the entire string is yielded.
 * If 'workdir' is specified the command is run from that directory.
 */
extern int read_pipe_iter(int command, int errorstream, double yield_interval = 0.1, int *workdir = 0);

/*!
 * Write 'text' into a shell command and discard its stdout output.
 */
extern void write_pipe(int command, int text, int *error = 0);

/*!
 * Given a list of pathnames, returns the longest common leading component.
 */
extern std::string commonprefix(std::vector<std::string> dirs);

/*!
 * Like shutil.copy2 but ignores chmod errors, and copies symlinks as links
 *
 * See [Bug 568000] Copying to NTFS fails
 */
extern void copy2(int src, int dst);

/*!
 * Similar to shutil.copytree, but always copies symlinks and doesn't
 * error out if the destination path already exists.
 */
extern void copytree(int src, int dst);

extern void shell_escape(int glob_pat);

/*!
 * Translate a shell PATTERN to a regular expression.
 *
 * Based on fnmatch.translate(). We also handle {a,b,c} where fnmatch does not.
 */
extern std::string shell_to_regex(std::string pat);


#endif
