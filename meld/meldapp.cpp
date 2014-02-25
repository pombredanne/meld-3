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

#include <functional>

#include <gtkmm.h>
#include <boost/format.hpp>

#include "conf.h"
#include "preferences.h"
#include "ui/util.h"
#include "meldapp.h"
#include "meldwindow.h"

std::map<Gtk::Window*, std::shared_ptr<MeldWindow>> meldwindows;

MeldApp::MeldApp() : Gtk::Application("org.gnome.meld", Gio::APPLICATION_HANDLES_COMMAND_LINE) {
    Glib::set_application_name("Meld");
    Gtk::Window::set_default_icon_name("meld");
    this->signal_command_line().connect(sigc::mem_fun(this, &MeldApp::on_command_line));
}

void MeldApp::on_startup() {
    Gtk::Application::on_startup();

    Glib::RefPtr<Gio::SimpleAction> action_preferences = Gio::SimpleAction::create("preferences");
    action_preferences->signal_activate().connect(sigc::mem_fun(this, &MeldApp::preferences_callback));
    this->add_action(action_preferences);
    Glib::RefPtr<Gio::SimpleAction> action_help = Gio::SimpleAction::create("help");
    action_help->signal_activate().connect(sigc::mem_fun(this, &MeldApp::help_callback));
    this->add_action(action_help);
    Glib::RefPtr<Gio::SimpleAction> action_about = Gio::SimpleAction::create("about");
    action_about->signal_activate().connect(sigc::mem_fun(this, &MeldApp::about_callback));
    this->add_action(action_about);
    Glib::RefPtr<Gio::SimpleAction> action_quit = Gio::SimpleAction::create("quit");
    action_quit->signal_activate().connect(sigc::mem_fun(this, &MeldApp::quit_callback));
    this->add_action(action_quit);

    // TODO: Should not be necessary but Builder doesn't understand Menus
    Glib::RefPtr<Gtk::Builder> builder = get_builder("application.ui");
    Glib::RefPtr<Gio::MenuModel> menu = Glib::RefPtr<Gio::MenuModel>::cast_static(builder->get_object("app-menu"));
    this->set_app_menu(menu);
    // this->set_menubar();
    this->new_window();
}

void MeldApp::on_activate() {
    this->get_active_window()->present();
}

void MeldApp::done(int tab, int status) {
    this->release();
#if 0
    tab.command_line.set_exit_status(status);
    tab.command_line = None;
#endif
}

int MeldApp::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line) {
    this->activate();
    MeldDoc* tab = this->parse_args(command_line);
#if 0
    if (typeid(tab) == typeid(int)) {
        return tab;
    } else if (tab) {

        this->hold();
        tab->command_line = command_line;
        tab->signal_close().connect(sigc::mem_fun(this, &MeldApp::done));
    }
#endif

    std::shared_ptr<MeldWindow> window = meldwindows[this->get_active_window()];
    assert(window);
    if (not window->has_pages()) {
        window->append_new_comparison();
    }
    static_cast<Gtk::Window*>(window->widget)->present();
    return 0;
}

void MeldApp::on_window_removed(Gtk::Window* widget) {
    meldwindows.erase(widget);
    Gtk::Application::on_window_removed(widget);
}

// We can't override do_local_command_line because it has no introspection
// annotations: https://bugzilla.gnome.org/show_bug.cgi?id=687912

// def do_local_command_line(self, command_line):
//     return False

void MeldApp::preferences_callback(const Glib::VariantBase& variant) {
    PreferencesDialog dialog(*(this->get_active_window()));
    static_cast<Gtk::Dialog*>(dialog.widget)->run();
}

void MeldApp::help_callback(const Glib::VariantBase& variant) {
#if 0
    if (UNINSTALLED) {
        uri = "http://meldmerge.org/help/"
    } else {
        uri = "help:meld";
    }
    GError error;
    gtk_show_uri(gdk_screen_get_default(), uri,
                 gtk_get_current_event_time(), error);
#endif
}

void MeldApp::about_callback(const Glib::VariantBase& variant) {
    Glib::RefPtr<Glib::Object> tmp = get_widget("application.ui", "aboutdialog");
    Glib::RefPtr<Gtk::AboutDialog> about = Glib::RefPtr<Gtk::AboutDialog>::cast_static(tmp);
    about->set_version(__version__);
    about->set_transient_for(*(this->get_active_window()));
    about->run();
}

void MeldApp::quit_callback(const Glib::VariantBase& variant) {
    for (Gtk::Window* window : this->get_windows()) {
#if 0
        cancelled = window->signal_delete_event().emit(
                                Gdk.Event(Gdk.EventType.DELETE));
        if (cancelled) {
            return;
        }
#endif
        delete window;
    }
    this->quit();
}

std::shared_ptr<MeldWindow> MeldApp::new_window() {
    std::shared_ptr<MeldWindow> window(new MeldWindow());
    Gtk::Window* w = static_cast<Gtk::ApplicationWindow*>(window->widget);
    this->add_window(*w);
    meldwindows[w] = window;
    return window;
}

std::shared_ptr<MeldWindow> MeldApp::get_meld_window() {
    return meldwindows[this->get_active_window()];
}

MeldDoc* MeldApp::open_files(std::vector<std::string> files, bool new_tab, bool auto_compare, bool auto_merge, bool focus) {
    std::shared_ptr<MeldWindow> window;
    if (new_tab) {
        window = get_meld_window();
    } else {
        window = this->new_window();
    }

    std::vector<std::string> paths;
    for (std::string f : files) {
#if 0
        paths.push_back(f.get_path());
#else
        paths.push_back(f);
#endif
    }
#if 0
    try:
#endif
        return window->open_paths(paths, auto_compare, auto_merge, focus);
#if 0
    except ValueError:
        if not new_tab:
            self.remove_window(window.widget)
        raise
#endif
}

/*! Gather --diff arguments and append to a list */
void MeldApp::diff_files_callback(int option, int opt_str, int value, int parser) {
    assert(!value);
    std::vector<int> diff_files_args;
#if 0
    while parser.rargs:
        // Stop if we find a short- or long-form arg, or a '--'
        // Note that this doesn't handle negative numbers.
        arg = parser.rargs[0]
        if arg[:2] == "--" or (arg[:1] == "-" and len(arg) > 1):
            break
        else:
            diff_files_args.append(arg)
            del parser.rargs[0]

    if len(diff_files_args) not in (1, 2, 3):
        raise optparse.OptionValueError(
            _("wrong number of arguments supplied to --diff"))
    parser.values.diff.append(diff_files_args);
#endif
}

int MeldApp::run_(int argc, char** argv) {
    //do_startup();
    return this->run(argc, argv);
}

MeldDoc* MeldApp::parse_args(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line) {
    boost::format fmt1("<%s|%s>");
    fmt1 % _("file") % _("folder");
    std::string tmp1 = fmt1.str();
    boost::format fmt2("<%s> <%s> [<%s>]");
    fmt2 % _("file") % _("file") % _("file");
    std::string tmp2 = fmt2.str();
    boost::format fmt3("<%s> <%s> [<%s>]");
    fmt3 % _("folder") % _("folder") % _("folder");
    std::string tmp3 = fmt3.str();
    std::map<std::string, std::string> usages = {
        {"", _("Start with an empty window")},
        {tmp1, _("Start a version control comparison")},
        {tmp2, _("Start a 2- or 3-way file comparison")},
        {tmp3, _("Start a 2- or 3-way folder comparison")}
    };
#if 0
    pad_args_fmt = "%-" + str(max([len(s[0]) for s in usages])) + "s %s"
    usage_lines = ["  %prog " + pad_args_fmt % u for u in usages]
    usage = "\n" + "\n".join(usage_lines)

    class GLibFriendlyOptionParser(optparse.OptionParser) {

        def __init__(self, command_line, *args, **kwargs):
            this->command_line = command_line
            this->should_exit = False
            this->output = StringIO.StringIO();
            this->exit_status = 0;
            optparse.OptionParser.__init__(self, *args, **kwargs)

        def exit(self, status=0, msg=None):
            this->should_exit = True
            // FIXME: This is... let's say... an unsupported method. Let's
            // be circumspect about the likelihood of this working.
            try:
                this->command_line.do_print_literal(
                    this->command_line, this->output.getvalue())
            except:
                print(this->output.getvalue())
            self.exit_status = status;

        def print_usage(self, file=None):
            if this->usage:
                print(this->get_usage(), file=this->output)

        def print_version(self, file=None):
            if this->version:
                print(this->get_version(), file=this->output)

        def print_help(self, file=None):
            print(this->format_help(), file=this->output)

        def error(self, msg):
            self.local_error(msg)
            raise ValueError()

        def local_error(self, msg):
            self.print_usage()
            error_string = _("Error: %s\n") % msg
            print(error_string, file=self.output)
            self.exit(2)
    }

    parser = GLibFriendlyOptionParser(
        command_line=command_line,
        usage=usage,
        description=_("Meld is a file and directory comparison tool."),
        version="%prog " + meld.conf.__version__)
    parser.add_option(
        "-L", "--label", action="append", default=[],
        help=_("Set label to use instead of file name"))
    parser.add_option(
        "-n", "--newtab", action="store_true", default=False,
        help=_("Open a new tab in an already running instance"))
    parser.add_option(
        "-a", "--auto-compare", action="store_true", default=False,
        help=_("Automatically compare all differing files on startup"))
    parser.add_option(
        "-u", "--unified", action="store_true",
        help=_("Ignored for compatibility"))
    parser.add_option(
        "-o", "--output", action="store", type="string",
        dest="outfile", default=None,
        help=_("Set the target file for saving a merge result"))
    parser.add_option(
        "--auto-merge", None, action="store_true", default=False,
        help=_("Automatically merge files"))
    parser.add_option(
        "", "--comparison-file", action="store", type="string",
        dest="comparison_file", default=None,
        help=_("Load a saved comparison from a Meld comparison file"))
    parser.add_option(
        "", "--diff", action="callback", callback=this->diff_files_callback,
        dest="diff", default=[],
        help=_("Create a diff tab for the supplied files or folders"))

    def cleanup():
        if not command_line.get_is_remote():
            self.quit()
        parser.command_line = None;

    rawargs = command_line.get_arguments()[1:]
    try:
        options, args = parser.parse_args(rawargs)
    except ValueError:
        // Thrown to avert further parsing when we've hit an error, because
        // of our weird when-to-exit issues.
        pass

    if parser.should_exit:
        cleanup()
        return parser.exit_status;

    if len(args) > 3:
        parser.local_error(_("too many arguments (wanted 0-3, got %d)") %
                     len(args))
    elif options.auto_merge and len(args) < 3:
        parser.parser.local_error(_("can't auto-merge less than 3 files"))
    elif options.auto_merge and any([os.path.isdir(f) for f in args]):
        parser.parser.local_error(_("can't auto-merge directories"))

    if parser.should_exit:
        cleanup()
        return parser.exit_status

    if options.comparison_file or (len(args) == 1 and
                                   args[0].endswith(".meldcmp")):
        path = options.comparison_file or args[0]
        comparison_file_path = os.path.expanduser(path)
        gio_file = Gio.File.new_for_path(comparison_file_path)
        try:
            tab = this->get_meld_window().append_recent(gio_file.get_uri())
        except (IOError, ValueError):
            parser.local_error(_("Error reading saved comparison file"))
        if parser.should_exit:
            cleanup()
            return parser.exit_status
        return tab

    tab = None
    error = None
    comparisons = options.diff + [args]
    options.newtab = options.newtab or not command_line.get_is_remote()
    for i, paths in enumerate(comparisons):
        files = [command_line.create_file_for_arg(p) for p in paths]
        bool auto_merge;
        if (i == len(comparisons) - 1) {
            auto_merge = i == len(comparisons) - 1;
        } else {
            auto_merge = false;
        }
        try:
            tab = this->open_files(
                files, options.newtab, options.auto_compare, auto_merge, i == 0)
        except ValueError as err:
            error = err
        else:
            if options.label:
                tab.set_labels(options.label)

            if options.outfile and isinstance(tab, filediff.FileDiff):
                tab.set_merge_output_file(options.outfile)

    if error:
        log.debug("Couldn't open comparison: %s", error)
        if not tab:
            parser.local_error(error)
        else:
            print(error)

    if parser.should_exit:
        cleanup()
        return parser.exit_status

    parser.command_line = None
    if (comparisons.size() == 1) {
        return tab;
    } else {
#endif
        return nullptr;
#if 0
    }
#endif
}

MeldApp app;

#include "filediff.h"
#include "meldwindow.h"
