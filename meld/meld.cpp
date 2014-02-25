/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2014 Kai Willadsen <kai.willadsen@gmail.com>
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
#include <boost/filesystem.hpp>
#include <libintl.h>

#include "conf.h"
#include "settings.h"
#include "meldapp.h"
#include "recent.h"

boost::filesystem::path get_meld_dir(boost::filesystem::path self_path) {
    // Support running from an uninstalled version
#if 0
    if os.path.basename(__file__) == "meld":
        self_path = os.path.realpath(__file__)
#endif
    return self_path.parent_path().parent_path();
}

bool uninstalled = false;
bool devel = false;
bool frozen = false;
void setup_uninstalled(char* argv[]) {
    // Support running from an uninstalled version
    boost::filesystem::path self_path = boost::filesystem::path(argv[0]);
    boost::filesystem::path melddir = get_meld_dir(self_path);

    if (boost::filesystem::exists(melddir.string() + "/meld.doap")) {
        uninstalled = true;
    }
    devel = boost::filesystem::exists(melddir.string() + "/.git");
    if (uninstalled) {
        conf_uninstalled();
    } else if (frozen) {
        conf_frozen();
    }
}

void setup_gettext() {
    // TODO: Possibly move to elib.intl
    const Glib::ustring locale_domain = __package__;
    std::string locale_dir = LOCALEDIR;

    bindtextdomain(locale_domain.c_str(), locale_dir.c_str());
    setlocale(LC_ALL, "");
    textdomain(locale_domain.c_str());
#if 0
    trans = gettext.translation(locale_domain, localedir=locale_dir, fallback=True)
    try:
        _ = meld.conf._ = trans.ugettext
        meld.conf.ngettext = trans.ungettext
    except AttributeError:
        // py3k
        _ = meld.conf._ = trans.gettext
        meld.conf.ngettext = trans.ngettext

    try:
        if os.name == 'nt':
            from ctypes import cdll
            if frozen:
                libintl = cdll['libintl-8']
            else:
                libintl = cdll.intl
            libintl.bindtextdomain(locale_domain, locale_dir)
            libintl.bind_textdomain_codeset(locale_domain, 'UTF-8')
            del libintl
        else:
            locale.bindtextdomain(locale_domain, locale_dir)
            locale.bind_textdomain_codeset(locale_domain, 'UTF-8')
    except AttributeError as e:
        // Python builds linked without libintl (i.e., OSX) don't have
        // bindtextdomain(), which causes Gtk.Builder translations to fail.
        print("Couldn't bind the translation domain. Some translations won't work.")
        print(e)
    except locale.Error as e:
        print("Couldn't bind the translation domain. Some translations won't work.")
        print(e)
    except WindowsError as e:
        // Accessing cdll.intl sometimes fails on Windows for unknown reasons.
        // Let's just continue, as translations are non-essential.
        print("Couldn't bind the translation domain. Some translations won't work.")
        print(e)
#endif
}

#if 0
void check_requirements() {

    pyver = (2, 7)
    gtk_requirement = (3, 6)
    glib_requirement = (2, 34, 0)
    gtksourceview_requirement = (3, 6, 0)

    def missing_reqs(mod, ver, exception=None):
        if isinstance(exception, ImportError):
            print(_("Cannot import: ") + mod + "\n" + str(e))
        else:
            modver = mod + " " + ".".join(map(str, ver))
            print(_("Meld requires %s or higher.") % modver)
        sys.exit(1)

    if sys.version_info[0] == 3:
        print(_("Meld does not support Python 3."))
        sys.exit(1)

    if sys.version_info[:2] < pyver:
        missing_reqs("Python", pyver)

    # gtk+ and related imports
    try:
        # FIXME: Extra clause for gi
        import gi
        from gi.repository import Gtk
        gi.require_version("Gtk", "3.0")
        version = (Gtk.get_major_version(), Gtk.get_minor_version())
        assert version >= gtk_requirement
    except (ImportError, AssertionError) as e:
        missing_reqs("GTK+", gtk_requirement, e)

    try:
        from gi.repository import GObject
        assert GObject.glib_version >= glib_requirement
    except (ImportError, AssertionError) as e:
        missing_reqs("GLib", glib_requirement, e)

    try:
        from gi.repository import GtkSource
        // TODO: There is no way to get at GtkSourceView's actual version
    except (ImportError, AssertionError) as e:
        missing_reqs("GtkSourceView", gtksourceview_requirement, e)
}
#endif

void setup_resources() {
    Glib::thread_init();
    std::string icon_dir = DATADIR + "/icons";
    Gtk::IconTheme::get_default()->append_search_path(icon_dir);

    Glib::RefPtr<Gtk::Settings> gtk_settings = Gtk::Settings::get_default();
    Glib::Value<bool> dark_theme;
    dark_theme.init(Glib::Value<bool>::value_type());
    gtk_settings->get_property_value("gtk-application-prefer-dark-theme", dark_theme);
    std::string css_name;
    if (dark_theme.get()) {
        css_name = "meld-dark";
    } else {
        css_name = "meld";
    }
    std::string css_file = DATADIR + "/" + css_name + ".css";
    Glib::RefPtr<Gtk::CssProvider> provider = Gtk::CssProvider::create();
    try {
        provider->load_from_path(css_file);
    } catch (const Glib::Error& err) {
        printf(_("Couldn't load Meld-specific CSS (%s)\n%s"), css_file, err.what());
    }

    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(), provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

#if 0
    if (Gtk::get_minor_version() >= 13) {
#endif
        Glib::RefPtr<Gtk::CssProvider> fixes_provider = Gtk::CssProvider::create();
        fixes_provider->load_from_data(
                    "MeldSourceView { background-color: rgba(0.0, 0.0, 0.0, 0.0); }");
        Gtk::StyleContext::add_provider_for_screen(
                    Gdk::Screen::get_default(), provider,
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#if 0
    }
#endif
}


void setup_settings() {
#if 0
    std::string schema_path = DATADIR + "gschemas.compiled";
    if (uninstalled and not boost::filesystem::exists(schema_path)) {
        subprocess.call(["glib-compile-schemas", meld.conf.DATADIR],
                        cwd=melddir);
    }
#endif
    create_settings(uninstalled or frozen);
}


void setup_logging() {
#if 0
    log = logging.getLogger();

    // If we're running uninstalled and from Git, turn up the logging level
    if (uninstalled and devel) {
        log.setLevel(logging.INFO);
    } else {
        log.setLevel(logging.CRITICAL);
    }

    handler = logging.StreamHandler();
    formatter = logging.Formatter("%(asctime)s %(levelname)s "
                                  "%(name)s: %(message)s");
    handler.setFormatter(formatter);
    log.addHandler(handler);
#endif
}

int main(int argc, char* argv[]) {
    Glib::init();
    setup_uninstalled(argv);
    recent_comparisons = new RecentFiles(argv);
    setup_gettext();
    setup_logging();
#if 0
    check_requirements();
#endif
    setup_settings();
    setup_resources();

    int status = app.run_(argc, argv);
    delete recent_comparisons;
    exit(status);
}
