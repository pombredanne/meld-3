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

#ifndef __MELD__CONF_H__
#define __MELD__CONF_H__

#include <string>
#include <gtkmm.h>

extern const Glib::ustring& __package__;
extern const Glib::ustring& __version__;

extern std::string DATADIR;
extern std::string LOCALEDIR;
extern bool UNINSTALLED;

// Installed from main script
#include <libintl.h>
#define _(str) gettext(str)

extern void conf_uninstalled();
extern void conf_frozen();

#endif
