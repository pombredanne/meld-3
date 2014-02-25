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

/*! Module of commonly used helper classes and functions */

#include <errno.h>

#include "misc.h"
#include "conf.h"
#include "meldapp.h"

#include <boost/algorithm/string.hpp>

#ifndef _WIN32
#if 0
    from select import select
#endif
#else
    import time

    def select(rlist, wlist, xlist, timeout):
        time.sleep(timeout)
        return rlist, wlist, xlist
#endif

/*! A common error dialog handler for Meld.
 *
 * This should only ever be used as a last resort, and for errors that
 * a user is unlikely to encounter. If you're tempted to use this,
 * think twice.
 *
 * Primary must be plain text. Secondary must be valid markup.
 */
void error_dialog(const Glib::ustring& primary, const Glib::ustring& secondary, Gtk::Window *parent, Gtk::MessageType messagetype) {
    if (not parent) {
        parent = app.get_active_window();
    }

    Gtk::MessageDialog dialog(
                *parent,
                primary, true,
                messagetype,
                Gtk::BUTTONS_CLOSE,
                true);

    dialog.set_secondary_text(secondary, true);
    dialog.run();
}

int modal_dialog(const Glib::ustring& primary, const Glib::ustring& secondary, std::map<const Glib::ustring, Gtk::ResponseType> buttons, Gtk::Window* parent, Gtk::MessageType messagetype) {
#if 0
    if (not parent) {
        parent = app.window.widget;
    } else if (not isinstance(parent, Gtk.Window)) {
        parent = parent.get_toplevel();
    }
#endif

    Gtk::MessageDialog dialog(*parent, primary, true, messagetype,  Gtk::BUTTONS_NONE, true);
    dialog.set_secondary_text(secondary, true);

    for (std::pair<const Glib::ustring, int> tmp : buttons) {
        const Glib::ustring label = tmp.first;
        int response_id = tmp.second;
        dialog.add_button(label, response_id);
    }

    int response = dialog.run();
    return response;
}

int run_dialog(const Glib::ustring& text, Gtk::Window *parent, Gtk::MessageType messagetype, Gtk::ButtonsType buttonstype, std::map<const Gtk::StockID*, int> extrabuttons) {
    Glib::ustring escaped = Glib::Markup::escape_text(text);

    Gtk::MessageDialog d("<span weight=\"bold\" size=\"larger\">" + escaped + "</span>",
                         true,
                         messagetype,
                         buttonstype,
                         false);
    if (parent) {
        d.set_transient_for(*static_cast<Gtk::Window*>(parent->get_toplevel()));
    }
    for (std::pair<const Gtk::StockID*, int> pair : extrabuttons) {
        const Gtk::StockID* b = pair.first;
        int rid = pair.second;
        d.add_button(*b, rid);
    }
    d.get_message_area()->set_spacing(12);
    Gtk::HBox* hbox = static_cast<Gtk::HBox*>(d.get_message_area()->get_children()[0]);
    hbox->set_spacing(12);
#if 0
    try:
        d.props.image.set_alignment(0.5, 0);
        d.props.image.set_padding(12, 12);
    except AttributeError:
        # FIXME: This is ridiculous. Possibly distribution-specific patches,
        # or just... bad things. This needs to go away.
        pass
#endif
    int ret = d.run();
    return ret;
}

std::tuple<int, int, bool> position_menu_under_widget(Gtk::Menu &menu, Gtk::Widget &widget) {
    Gtk::Widget* container = widget.get_ancestor(GTK_TYPE_CONTAINER);

    int widget_width = widget.get_allocation().get_width();
    int menu_width = menu.get_allocation().get_width();
    int menu_height = menu.get_allocation().get_height();

    Glib::RefPtr<const Gdk::Screen> screen = menu.get_screen();
    int monitor_num = screen->get_monitor_at_window(widget.get_window());
    if (monitor_num < 0) {
        monitor_num = 0;
    }
    Gdk::Rectangle monitor;
    screen->get_monitor_geometry(monitor_num, monitor);

    int x, y;
    widget.get_window()->get_origin(x, y);
    Gtk::Allocation allocation = widget.get_allocation();
    if (not widget.get_has_window()) {
        x += allocation.get_x();
        y += allocation.get_y();
    }

    if (container->get_direction() == Gtk::TEXT_DIR_LTR) {
        x += allocation.get_width() - widget_width;
    } else {
        x += widget_width - menu_width;
    }

    if ((y + allocation.get_height() + menu_height) <= monitor.get_y() + monitor.get_height()) {
        y += allocation.get_height();
    } else if ((y - menu_height) >= monitor.get_y()) {
        y -= menu_height;
    } else if (monitor.get_y() + monitor.get_height() - (y + allocation.get_height()) > y) {
        y += allocation.get_height();
    } else {
        y -= menu_height;
    }

    return std::tuple<int, int, bool>(x, y, false);
}

std::unique_ptr<Gtk::HBox> make_tool_button_widget(const Glib::ustring& label) {
    Gtk::Arrow arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE);
    Gtk::Label _label(label);
    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox(false, 3));
    hbox->pack_end(arrow, true, true, 0);
    hbox->pack_end(_label, true, true, 0);
    hbox->show_all();
    return std::unique_ptr<Gtk::HBox>(hbox);
}

Gdk::Color gdk_to_cairo_color(Gdk::Color color) {
    Gdk::Color c;
    c.set_rgb_p(color.get_red() / 65535., color.get_green() / 65535., color.get_blue() / 65535.);
    return c;
}

int all_equal(std::vector<int> alist) {
    if (!alist.empty()) {
        int first = alist[0];
        for (size_t n = 1; n < alist.size(); n++) {
            if (n != first) {
                return 0;
            }
        }
    }
    return 1;
}

std::vector<std::string> shorten_names(std::vector<std::string> names) {
    // TODO: Update for different path separators
#if 0
    prefix = os.path.commonprefix(names);
    prefixslash = prefix.rfind("/") + 1;

    names = [n[prefixslash:] for n in names];
    paths = [n.split("/") for n in names];

    try {
        basenames = [p[-1] for p in paths];
    } catch (IndexError &e) {
        //pass
    } else {
        if (all_equal(basenames)) {
            def firstpart(alist) {
                if len(alist) > 1:
                    return "[%s] " % alist[0]
                else:
                    return "";
            }
            roots = [firstpart(p) for p in paths];
            base = basenames[0].strip();
            return [r + base for r in roots];
        }
    }
    // no common path. empty names get changed to "[None]"
    return [name or _("[None]") for name in basenames];
#else
    return names;
#endif
}

sentinel::sentinel() {
#if 0
    this->proc = 0;
#endif
}

sentinel::~sentinel() {
#if 0
    if (this->proc) {
        errorstream.error("killing '%s'\n" % command[0]);
        self.proc.terminate();
        errorstream.error("killed (status was '%i')\n" %
                          self.proc.wait());
    }
#endif
}

void sentinel::__call__() {
#if 0
    self.proc = subprocess.Popen(command, cwd=workdir,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE);
    self.proc.stdin.close();
    childout, childerr = self.proc.stdout, self.proc.stderr;
    bits = [];
    while (len(bits) == 0 or bits[-1] != "") {
        state = select([childout, childerr], [], [childout, childerr],
                       yield_interval);
        if (len(state[0]) == 0) {
            if (len(state[2]) == 0) {
                yield None;
            } else {
                raise Exception("Error reading pipe");
            }
        }
        if (childout in state[0]) {
            try {
                // get buffer size
                bits.append(childout.read(4096));
            } catch (IOError &e) {
                // FIXME: ick need to fix
                break;
            }
        }
        if (childerr in state[0]) {
            try {
                // how many chars?
                errorstream.error(childerr.read(1));
            } catch (IOError &e) {
                // FIXME: ick need to fix
                break;
            }
        }
    }
    status = self.proc.wait();
    errorstream.error(childerr.read());
    self.proc = None;
    if (status) {
        errorstream.error("Exit code: %i\n" % status);
    }
    yield "".join(bits);
    yield status;
#endif
}


int read_pipe_iter(int command, int errorstream, double yield_interval, int* workdir) {
#if 0
    if (workdir == "") {
        workdir = None
    }
    return sentinel()();
#else
    return 1;
#endif
}

void write_pipe(int command, int text, int *error) {
#if 0
    proc = subprocess.Popen(command, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=error);
    proc.communicate(text);
    return proc.wait();
#endif
}

std::string commonprefix(std::vector<std::string> dirs) {
    if (dirs.empty()) {
        return "";
    }
#if 0
    std::vector<std::string> n;
    for (std::string d : dirs) {
        std::vector<std::string> tokens;
        boost::split(tokens, d, boost::is_any_of("/"));
        n.insert(n.end(), tokens.begin(), tokens.end());
    }
    std::vector<std::string> prefix;
    prefix.push_back(n[0]);
    for (std::string item : n) {
        for (int i = 0; i < prefix.length(); i++) {
            if (prefix[:i+1] != item[:i+1]) {
                prefix = prefix[:i];
                if (i == 0) {
                    return "";
                }
                break;
            }
        }
    }
    return os.sep.join(prefix);
#else
    return "";
#endif
}

void copy2(int src, int dst) {
#if 0
    if (os.path.isdir(dst)) {
        dst = os.path.join(dst, os.path.basename(src));
    }

    if (os.path.islink(src) and os.path.isfile(src)) {
        if (os.path.lexists(dst)) {
            os.unlink(dst);
        }
        os.symlink(os.readlink(src), dst);
    } else if (os.path.isfile(src)) {
        shutil.copyfile(src, dst);
    } else {
        raise OSError("Not a file");
    }

    try {
        shutil.copystat(src, dst);
    } catch (OSError &e) {
        if (e.errno != errno.EPERM) {
            raise;
        }
    }
#endif
}

void copytree(int src, int dst) {
#if 0
    // If the source tree is a symlink, duplicate the link and we're done.
    if (os.path.islink(src)) {
        os.symlink(os.readlink(src), dst);
        return;
    }

    try {
        os.mkdir(dst);
    } catch (OSError &e) {
        if (e.errno != errno.EEXIST) {
            raise;
        }
    }
    names = os.listdir(src);
    for (name in names) {
        srcname = os.path.join(src, name);
        dstname = os.path.join(dst, name);
        if (os.path.islink(srcname)) {
            os.symlink(os.readlink(srcname), dstname);
        } else if (os.path.isdir(srcname)) {
            copytree(srcname, dstname);
        } else {
            copy2(srcname, dstname);
        }
    }

    try {
        shutil.copystat(src, dst);
    } catch (OSError &e) {
        if (e.errno != errno.EPERM) {
            raise;
        }
    }
#endif
}

void shell_escape(int glob_pat) {
#if 0
    // TODO: handle all cases
    assert(not re.compile(r"[][*?]").findall(glob_pat));
    return glob_pat.replace("{", "[{]").replace("}", "[}]");
#endif
}

std::string shell_to_regex(std::string pat) {
#if 0
    i, n = 0, len(pat);
    res = "";
    while (i < n) {
        c = pat[i];
        i += 1;
        if (c == '\\') {
            try {
                c = pat[i];
            } catch (IndexError &e) {
                // pass
            } else {
                i += 1;
                res += re.escape(c);
            }
        } else if (c == '*') {
            res += ".*";
        } else if (c == '?') {
            res += ".";
        } else if (c == '[') {
            try {
                j = pat.index(']', i);
            } catch (ValueError &e) {
                res += r'\[';
            } else {
                stuff = pat[i:j];
                i = j+1;
                if (stuff[0] == '!') {
                    stuff = "^%s" % stuff[1:];
                } else if (stuff[0] == '^') {
                    stuff = r"\^%s" % stuff[1:];
                }
                res += "[%s]" % stuff;
            }
        } else if (c == '{') {
            try {
                j = pat.index("}", i);
            } catch (ValueError &e) {
                res += "\\{";
            } else {
                stuff = pat[i:j];
                i = j+1;
                res += "(%s)" % "|".join([shell_to_regex(p)[:-1] for p in stuff.split(",")]);
            }
        } else {
            res += re.escape(c);
        }
    }
    return res + "$";
#else
    return "";
#endif
}
