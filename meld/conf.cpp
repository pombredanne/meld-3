/* Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
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

#include <cstdlib>
#include <boost/filesystem.hpp>

#include "conf.h"

const Glib::ustring& __package__ = "meld";
const Glib::ustring& __version__ = "3.13.0";

// START; these paths are clobbered on install by meld.build_helpers
std::string DATADIR = std::string(PREFIX) + "/share/meld";
std::string LOCALEDIR = std::string(PREFIX) + "/share/locale";
// END
bool UNINSTALLED = false;

void conf_uninstalled() {
    boost::filesystem::path p(__FILE__);
    boost::filesystem::path absolute = boost::filesystem::absolute(p);
    boost::filesystem::path parent = absolute.parent_path();
    boost::filesystem::path parent_parent = parent.parent_path();
    std::string melddir = boost::filesystem::absolute(parent_parent).string();

    DATADIR = melddir + "/data";
    LOCALEDIR = melddir + "/build/mo";
    UNINSTALLED = true;

    // This first bit should be unnecessary, but some things (GTK icon theme
    // location, GSettings schema location) don't fall back correctly.
    std::string data_dir;
    char* tmp = getenv("XDG_DATA_DIRS");
    if (tmp) {
        data_dir = tmp;
    }
    if (data_dir.empty()) {
        data_dir = "/usr/local/share/:/usr/share/";
    }
    data_dir = melddir + ":" + data_dir;
    setenv("XDG_DATA_DIRS", data_dir.c_str(), 1);
}

void conf_frozen() {
#if 0
    melddir = os.path.dirname(sys.executable)

    DATADIR = os.path.join(melddir, "share", "meld")
    LOCALEDIR = os.path.join(melddir, "share", "mo")

    // This first bit should be unnecessary, but some things (GTK icon theme
    // location, GSettings schema location) don't fall back correctly.
    data_dir = os.environ.get('XDG_DATA_DIRS', "/usr/local/share/:/usr/share/")
    data_dir = ":".join((melddir, data_dir))
    os.environ['XDG_DATA_DIRS'] = data_dir
#endif
}
