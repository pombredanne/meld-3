/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2011-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#if 0
import logging
import shlex
import string
import subprocess
import sys
#endif

#include <gtkmm.h>

#include "melddoc.h"

#include <libintl.h>
#define _(str) gettext(str)
#include "settings.h"

#if 0
log = logging.getLogger(__name__)
#endif


static std::string make_custom_editor_command(std::string path, int line = 0) {
#if 0
    Glib::ustring custom_command = settings->get_string("custom-editor-command");
    fmt = string.Formatter()
    replacements = [tok[1] for tok in fmt.parse(custom_command)]

    if not any(replacements):
        return [custom_command, path]
    elif not all(r in (None, 'file', 'line') for r in replacements):
        log.error("Unsupported fields found", )
        return [custom_command, path]
    else:
        cmd = custom_command.format(file=pipes.quote(path), line=line)
    return shlex.split(cmd);
#else
    return "";
#endif
}

MeldDoc::MeldDoc(SchedulerBase& scheduler, std::string filename, std::string root, std::list<std::string> extra) : Tab(filename, root, extra), scheduler(scheduler) {
    this->num_panes = 0;
    this->label_text = _("untitled");
    this->tooltip_text = _("untitled");
    this->main_actiongroup.clear();
}

MeldDoc::~MeldDoc() {
}

void MeldDoc::save() {
}

void MeldDoc::save_as() {
}

void MeldDoc::stop() {
    if (this->scheduler.tasks_pending()) {
        this->scheduler.remove_task(this->scheduler.get_current_task());
    }
}

void MeldDoc::os_open(std::string path, std::string uri) {
    if (path.empty()) {
        return;
    }
    std::stringstream ss;
#ifdef _WIN32
    ss << "start";
    ss << " " << path;
    system(ss.str().c_str());
#elif __APPLE__
    ss << "open";
    ss << " " << path;
    system(ss.str().c_str());
#elif __unix__ || __linux__
    gtk_show_uri(gdk_screen_get_default(), uri.c_str(),
                 gtk_get_current_event_time(), 0);
#else
#error "Unknown system!"
#endif
}

void MeldDoc::open_cb(const Glib::RefPtr<Gio::AsyncResult>& result, Glib::RefPtr<Gio::File> source, int line) {
    Glib::RefPtr<Gio::FileInfo> info = source->query_info_finish(result);
    Gio::FileType file_type = info->get_file_type();
    std::string path = source->get_path();
    std::string uri = source->get_uri();
    if (file_type == Gio::FILE_TYPE_DIRECTORY) {
        os_open(path, uri);
    } else if (file_type == Gio::FILE_TYPE_REGULAR) {
        std::string content_type = info->get_content_type();
        // FIXME: Content types are broken on Windows with current gio
        if (Gio::content_type_is_a(content_type, "text/plain") or
#ifdef _WIN32
            true
#else
            false
#endif
        ) {
            if (settings->get_boolean("use-system-editor")) {
                Glib::RefPtr<Gio::File> gfile = Gio::File::create_for_path(path);
#ifdef _WIN32
                auto handler = gfile->query_default_handler();
                result = handler.launch({gfile});
#else
                uri = gfile->get_uri();
                Gio::AppInfo::launch_default_for_uri(uri);
#endif
            } else {
                std::string editor = make_custom_editor_command(path, line);
                if (!editor.empty()) {
                    // TODO: If the editor is badly set up, this fails
                    // silently
                    system(editor.c_str());
                } else {
                    os_open(path, uri);
                }
            }
        } else {
            os_open(path, uri);
        }
    } else {
        // TODO: Add some kind of 'failed to open' notification
    }
}

void MeldDoc::_open_files(std::list<std::string> selected, int line) {
    static std::string query_attrs = std::string(G_FILE_ATTRIBUTE_STANDARD_TYPE) + "," +
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE;

    for (std::string s : selected) {
        Glib::RefPtr<Gio::File> f = Gio::File::create_for_path(s);
        f->query_info_async(sigc::bind(sigc::mem_fun(this, &MeldDoc::open_cb), f, line), query_attrs, Gio::FILE_QUERY_INFO_NONE, Glib::PRIORITY_LOW);
    }
}

void MeldDoc::open_external() {
}

void MeldDoc::on_refresh_activate() {
}

void MeldDoc::on_find_activate() {
}

void MeldDoc::on_find_next_activate() {
}

void MeldDoc::on_find_previous_activate() {
}

void MeldDoc::on_replace_activate() {
}

void MeldDoc::on_file_changed(std::string filename) {
}

void MeldDoc::label_changed() {
    this->signal_label_changed().emit(this, this->label_text, this->tooltip_text);
}

void MeldDoc::set_labels(std::vector<Glib::ustring> lst) {
}

/*! Called when the container app switches to this tab. */
void MeldDoc::on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> uimanager) {
    this->ui_merge_id = uimanager->add_ui_from_file(this->ui_file);
    uimanager->insert_action_group(this->actiongroup, -1);
    this->popup_menu = static_cast<Gtk::Menu*>(uimanager->get_widget("/Popup"));
    this->main_actiongroup.clear();
    std::vector<Glib::RefPtr<Gtk::ActionGroup>> action_groups = uimanager->get_action_groups();
    for (Glib::RefPtr<Gtk::ActionGroup> a : action_groups) {
        if (a->get_name() == "MainActions") {
            this->main_actiongroup = a;
            break;
        }
    }
    uimanager->ensure_update();
#if 0
    if (hasattr(self, "focus_pane") and this->focus_pane) {
        this->scheduler.add_task(this->focus_pane.grab_focus);
    }
#endif
}

/*! Called when the container app switches away from this tab. */
void MeldDoc::on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> uimanager) {
    uimanager->remove_action_group(this->actiongroup);
    uimanager->remove_ui(this->ui_merge_id);
    this->main_actiongroup.clear();
}

/*!
 * Called when the docs container is about to close.
 *
 * A doc normally returns Gtk.ResponseType.OK, but may instead return
 * Gtk.ResponseType.CANCEL to request that the container not delete it.
 */
Gtk::ResponseType MeldDoc::on_delete_event(int appquit) {
    return Gtk::RESPONSE_OK;
}
