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

#include <gtkmm.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "melddoc.h"
#include "misc.h"
#include "recent.h"
#include "tree.h"
#include "ui/emblemcellrenderer.h"
#include "ui/gnomeglade.h"
#include "ui/vcdialogs.h"
#include "vcview.h"
#include "vc/__init__.h"
#include "conf.h"

#include "settings.h"

#if 0
log = logging.getLogger(__name__)
#endif

static std::string _commonprefix(std::vector<std::string> files) {
    std::string workdir;
    if (files.empty()) {
        workdir = ".";
    } else if (files.size() != 1) {
        workdir = commonprefix(files);
    } else {
        workdir = boost::filesystem::path(files[0]).parent_path().string();
    }
    return workdir;
}

std::list<std::string> _temp_dirs;
std::list<std::string> _temp_files;

void cleanup_temp() {
#if 0
    std::string temp_location = boost::filesystem::unique_path().string;
    // The strings below will probably end up as debug log, and are deliberately
    // not marked for translation.
    for (std::string f : _temp_files) {
        try {
            assert (boost::filesystem::exists(f) and os.path.isabs(f) and
                    os.path.dirname(f) == temp_location);
            // Windows throws permissions errors if we remove read-only files
#ifdef _WIN32
                os.chmod(f, stat.S_IWRITE)
#endif
            os.remove(f);
        } catch (...) {
            except_str = "{0[0]}: \"{0[1]}\"".format(sys.exc_info());
            print("File \"{0}\" not removed due to".format(f), except_str,
                  file=sys.stderr);
        }
    }
    for (std::string f : _temp_dirs) {
        try {
            assert (boost::filesystem::exists(f) and os.path.isabs(f) and
                    os.path.dirname(f) == temp_location);
            shutil.rmtree(f, ignore_errors=1);
        } catch (...) {
            except_str = "{0[0]}: \"{0[1]}\"".format(sys.exc_info());
            print("Directory \"{0}\" not removed due to".format(f), except_str,
                  file=sys.stderr);
        }
    }
#endif
}

#if 0
atexit.register(cleanup_temp)
#endif


ConsoleStream::ConsoleStream(Gtk::TextView* textview) {
    this->textview = textview;
    Glib::RefPtr<Gtk::TextBuffer> buf = textview->get_buffer();
    this->command_tag = buf->create_tag("command");
    this->command_tag->set_property("weight", Pango::WEIGHT_BOLD);
    this->output_tag = buf->create_tag("output");
    this->error_tag = buf->create_tag("error");
    // FIXME: Need to add this to the gtkrc?
    Glib::Value<Glib::ustring> property_value;
    property_value.init(Glib::Value<Glib::ustring>::value_type());
    property_value.set("#cc0000");
    this->error_tag->set_property_value("foreground", property_value);
    this->end_mark = buf->create_mark(buf->end(), false);
}

void ConsoleStream::command(const Glib::ustring& message) {
    this->write(message, this->command_tag);
}

void ConsoleStream::output(const Glib::ustring& message) {
    this->write(message, this->output_tag);
}

void ConsoleStream::error(const Glib::ustring& message) {
    this->write(message, this->error_tag);
}

void ConsoleStream::write(const Glib::ustring& message, Glib::RefPtr<Gtk::TextBuffer::Tag> tag) {
    if (message.empty()) {
        return;
    }
    Glib::RefPtr<Gtk::TextBuffer> buf = this->textview->get_buffer();
    std::vector<Glib::RefPtr<Gtk::TextBuffer::Tag>> tags{tag};
    buf->insert_with_tags(buf->end(), message, tags);
    this->textview->scroll_to(this->end_mark);
}

const int COL_LOCATION = COL_END;
const int COL_STATUS = COL_END + 1;
const int COL_REVISION = COL_END + 2;
const int COL_OPTIONS = COL_END + 3;


VcTreeStore::VcTreeStore() : DiffTreeStore(1, std::vector<const std::type_info*>{&typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string)}) {
}

//###############################################################################
// filters
//###############################################################################
std::function<bool(Entry* x)> entry_modified = [] (Entry* x) { return (x->state >= STATE_NEW) or (x->isdir and (x->state > STATE_NONE)); };
std::function<bool(Entry* x)> entry_normal   = [] (Entry* x) { return (x->state == STATE_NORMAL); };
std::function<bool(Entry* x)> entry_nonvc    = [] (Entry* x) { return (x->state == STATE_NONE) or (x->isdir and (x->state > STATE_IGNORED)); };
std::function<bool(Entry* x)> entry_ignored  = [] (Entry* x) { return (x->state == STATE_IGNORED) or x->isdir; };

Gtk::TreeViewColumn* VcView::addCol(const Glib::ustring&name, int num, Data data_name) {
    Gtk::TreeViewColumn* column = Gtk::manage(new Gtk::TreeViewColumn(name));
    column->set_resizable(true);
    Gtk::CellRendererText rentext;
    column->pack_start(rentext, true);
#if 0
    column->set_attributes(rentext,
                          markup=this->model->column_index(num, 0));
#endif
    int column_index = this->treeview->append_column(*column) - 1;
    if (data_name != DATA_INVALID) {
        this->column_name_map[data_name] = column_index;
    }
    return column;
}



VcView::VcView(SchedulerBase& scheduler) : MeldDoc(scheduler, "vcview.ui", "vcview",
                                {"VcviewActions", "liststore_vcs"}) {

    bind_settings(this, __gsettings_bindings__);

    this->builder->get_widget("VcviewActions", this->VcviewActions);

    this->ui_file = gnomeglade_ui_file("vcview-ui.xml");
    this->actiongroup = Glib::RefPtr<Gtk::ActionGroup>(this->VcviewActions);
#if 0
    this->actiongroup->set_translation_domain("meld");
#endif

    // Hook up singals
    this->actiongroup->get_action("VcCompare")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_diff_clicked));
    this->actiongroup->get_action("VcCommit")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_commit_clicked));
    this->actiongroup->get_action("VcUpdate")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_update_clicked));
    this->actiongroup->get_action("VcPush")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_push_clicked));
    this->actiongroup->get_action("VcAdd")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_add_clicked));
    this->actiongroup->get_action("VcRemove")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_remove_clicked));
    this->actiongroup->get_action("VcResolved")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_resolved_clicked));
    this->actiongroup->get_action("VcRevert")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_revert_clicked));
    this->actiongroup->get_action("VcDeleteLocally")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_delete_clicked));
    this->actiongroup->get_action("VcFlatten")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_button_flatten_toggled));
    this->actiongroup->get_action("VcShowModified")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_filter_state_toggled));
    this->actiongroup->get_action("VcShowNormal")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_filter_state_toggled));
    this->actiongroup->get_action("VcShowNonVC")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_filter_state_toggled));
    this->actiongroup->get_action("VcShowIgnored")->signal_activate().connect(sigc::mem_fun(this, &VcView::on_filter_state_toggled));

    this->model = Glib::RefPtr<VcTreeStore>(new VcTreeStore());
    this->widget->signal_style_updated().connect(sigc::bind(sigc::mem_fun(*this->model.operator->(), &VcTreeStore::on_style_updated), this->widget));
    this->model->on_style_updated(this->widget);
    this->treeview->set_model(this->model);
    Glib::RefPtr<Gtk::TreeSelection> selection = this->treeview->get_selection();
    selection->set_mode(Gtk::SELECTION_MULTIPLE);
    selection->signal_changed().connect(sigc::mem_fun(this, &VcView::on_treeview_selection_changed));
    this->treeview->set_headers_visible(1);
#if 0
    this->treeview->set_search_equal_func(sigc::mem_fun(this->model, &DiffTreeStore::treeview_search_cb));
#endif
    this->current_path.clear();
    this->prev_path.clear();
    this->next_path.clear();

    Gtk::TreeViewColumn column(_("Name"));
    column.set_resizable(true);
    EmblemCellRenderer renicon;
    column.pack_start(renicon, false);
#if 0
    column.set_attributes(renicon,
                          icon_name=this->model->column_index(tree.COL_ICON, 0),
                          icon_tint=this->model->column_index(tree.COL_TINT, 0));
#endif
    Gtk::CellRendererText rentext;
    column.pack_start(rentext, true);
#if 0
    column.set_attributes(rentext,
                          text=this->model->column_index(tree.COL_TEXT, 0),
                          foreground=this->model->column_index(tree.COL_FG, 0),
                          style=this->model->column_index(tree.COL_STYLE, 0),
                          weight=this->model->column_index(tree.COL_WEIGHT, 0),
                          strikethrough=this->model->column_index(tree.COL_STRIKE, 0));
#endif
    int column_index = this->treeview->append_column(column) - 1;
    this->column_name_map[DATA_NAME] = column_index;

    this->treeview_column_location = addCol(_("Location"), COL_LOCATION);
    addCol(_("Status"), COL_STATUS, DATA_STATE);
    addCol(_("Revision"), COL_REVISION, DATA_REVISION);
    addCol(_("Options"), COL_OPTIONS, DATA_OPTIONS);

    this->consolestream = new ConsoleStream(this->consoleview);
    this->location = nullptr;
    Glib::RefPtr<Gtk::Action> tmp = this->actiongroup->get_action("VcFlatten");
    Glib::RefPtr<Gtk::ToggleAction> tmp2 = Glib::RefPtr<Gtk::ToggleAction>::cast_static(tmp);
    this->treeview_column_location->set_visible(tmp2->get_active());
    this->vc = nullptr;

    Gtk::CellRendererText cell;
    this->combobox_vcs->pack_start(cell, false);
    this->combobox_vcs->add_attribute(cell, "text", 0);
    this->combobox_vcs->add_attribute(cell, "sensitive", 2);
#if 0
    this->combobox_vcs->lock = false;
#endif

#if 0
    settings.bind("vc-console-visible",
                  this->actiongroup.get_action("VcConsoleVisible"),
                  "active", Gio.SettingsBindFlags.DEFAULT)
    settings.bind("vc-console-visible", this->console_vbox, "visible",
                  Gio.SettingsBindFlags.DEFAULT)
    settings.bind("vc-console-pane-position", this->vc_console_vpaned,
                  "position", Gio.SettingsBindFlags.DEFAULT)
#endif

#if 0
    for (int s : this->state_actions) {
        if (std::find(this->props.status_filters.begin(), this->props.status_filters.end(), s) != this->props.status_filters.end()) {
            action_name = this->state_actions[s][0];
            this->state_filters.append(s);
            this->actiongroup.get_action(action_name).set_active(True);
        }
    }
#endif
}

void VcView::_set_external_action_sensitivity(bool focused) {
    if (this->main_actiongroup && this->main_actiongroup->get_action("OpenExternal")) {
        this->main_actiongroup->get_action("OpenExternal")->set_sensitive(focused);
    }
}

void VcView::on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui) {
    MeldDoc::on_container_switch_in_event(ui);
    this->_set_external_action_sensitivity(true);
#if 0
    this->scheduler.add_task([this] () { this->on_treeview_cursor_changed; });
#endif
}

void VcView::on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui) {
    this->_set_external_action_sensitivity(false);
    MeldDoc::on_container_switch_out_event(ui);
}

void VcView::update_visible_columns() {
    for (std::pair<Data, int> data_id : this->column_name_map) {
        Gtk::TreeViewColumn* col = this->treeview->get_column(this->column_name_map[data_id.first]);
        bool visible = std::find(this->vc->VC_COLUMNS.begin(), this->vc->VC_COLUMNS.end(), data_id.first) != this->vc->VC_COLUMNS.end();
        col->set_visible(visible);
    }
}

/*! Disable actions that use not implemented VC plugin methods */
void VcView::update_actions_sensitivity() {
    std::vector<Glib::ustring> valid_vc_actions {"VcDeleteLocally"};
    for (std::pair<std::string, std::pair<std::string, std::vector<std::string>>> tmp : this->action_vc_cmds_map) {
        std::string action_name = tmp.first;
        std::string meth_name = tmp.second.first;
        std::vector<std::string> args = tmp.second.second;
        Glib::RefPtr<Gtk::Action> action = this->actiongroup->get_action(action_name);
        try {
#if 0
            getattr(this->vc, meth_name)(*args);
            action.props.sensitive = True;
            valid_vc_actions.push_back(action_name);
#endif
        } catch (NotImplementedError) {
            Glib::Value<bool> value;
            value.init(Glib::Value<bool>::value_type());
            value.set(false);
            action->set_property_value("sensitive", value);
        }
    }
    this->valid_vc_actions = valid_vc_actions;
}

/*! Display VC plugin(s) that can handle the location */
void VcView::choose_vc(std::string location) {
#if 0
    this->combobox_vcs.lock = true;
#endif
    Glib::RefPtr<Gtk::TreeModel> vcs_model = this->combobox_vcs->get_model();
#if 0
    vcs_model->clear();
#endif
    int default_active = -1;
    std::vector<int> valid_vcs;
    if (location.empty()) {
        location = boost::filesystem::path(".").string();
    } else {
        location = boost::filesystem::path(location).string();
    }

    // VC systems work at the directory level, so make sure we're checking
    // for VC support there instead of on a specific file.
    if (boost::filesystem::is_regular(location)) {
        location = boost::filesystem::path(location).parent_path().string();
    }
    std::vector<Vc*> vcs = get_vcs(location);
    // Try to keep the same VC plugin active on refresh()
    for (unsigned int idx = 0; idx < vcs.size(); idx++) {
        Vc* avc = vcs[idx];
        // See if the necessary version control command exists.  If so,
        // make sure what we're diffing is a valid respository.  If either
        // check fails don't let the user select the that version control
        // tool and display a basic error message in the drop-down menu.
        std::string err_str = "";

        if (not avc->is_installed(*avc)) {
            // TRANSLATORS: this is an error message when a version control
            // application isn't installed or can't be found
            boost::format fmt(_("%s not installed"));
            fmt % avc->CMD;
            err_str = fmt.str();
        } else if (not avc->valid_repo(*avc, location)) {
            // TRANSLATORS: this is an error message when a version
            // controlled repository is invalid or corrupted
            err_str = _("Invalid repository");
        } else {
            valid_vcs.push_back(idx);
            if (this->vc and (typeid(this->vc) == typeid(avc))) {
                default_active = idx;
            }
        }

        if (!err_str.empty()) {
#if 0
            vcs_model.append(
                [_("%s (%s)") % (avc.NAME, err_str), avc, False]);
#endif
        } else {
#if 0
            name = avc.NAME or _("None");
            vcs_model.append([name, avc(location), True]);
#endif
        }
    }

    if (valid_vcs.empty()) {
        // If we didn't get any valid vcs then fallback to null
#if 0
        null_vcs = _null.Vc(location);
        vcs.append(null_vcs);
        vcs_model.insert(
            0, [_("None"), null_vcs, True]);
        default_active = 0;
#endif
    }

    if (default_active == -1) {
        if (!valid_vcs.empty()) {
            default_active = *std::min_element(valid_vcs.begin(), valid_vcs.end());
        } else {
            default_active = 0;
        }
    }

    // If we only have the null VC, give a better error message.
    std::string tooltip;
    if ((vcs.size() == 1 and vcs[0]->CMD.empty()) or (valid_vcs.empty())) {
        tooltip = _("No valid version control system found in this folder");
    } else if (vcs.size() == 1) {
        tooltip = _("Only one version control system found in this folder");
    } else {
        tooltip = _("Choose which version control system to use");
    }

    this->combobox_vcs->set_tooltip_text(tooltip);
    this->combobox_vcs->set_sensitive(vcs.size() > 1);
#if 0
    this->combobox_vcs->lock = false;
#endif
    this->combobox_vcs->set_active(default_active);
}

void VcView::on_vc_change(Gtk::ComboBox* cb) {
#if 0
    if (not cb->lock) {
        this->vc = cb.get_model()[cb.get_active_iter()][1];
        this->_set_location(this->vc.location);
        this->update_actions_sensitivity();
        this->update_visible_columns();
    }
#endif
}

void VcView::set_location(std::string location) {
    this->choose_vc(location);
}

void VcView::_set_location(std::string location) {
#if 0
    this->location = location;
    this->current_path.clear();
    this->model->clear();
    this->fileentry[0]->set_filename(location);
    Gtk::TreeStore::iterator it = this->model->add_entries(nullptr, {location});
    this->treeview->grab_focus();
    this->treeview->get_selection()->select_iter(it);
    this->model->set_path_state(it, 0, STATE_NORMAL, 1);
    this->recompute_label();
    this->scheduler.remove_all_tasks();

    // If the user is just diffing a file (ie not a directory), there's no
    // need to scan the rest of the repository
    if (boost::filesystem::is_directory(this->vc->location)) {
        Gtk::TreeModel::iterator root = this->model->get_iter_first();

        try {
            int col = this->model->column_index(COL_OPTIONS, 0);
            this->model->set_value(root, col,
                                 this->vc.get_commits_to_push_summary())
        } catch (NotImplementedError &e) {
            // pass
        }

        this->scheduler.add_task([this] () { this->_search_recursively_iter(root); });
        this->scheduler.add_task([this] () { this->on_treeview_selection_changed; });
        this->scheduler.add_task([this] () { this->on_treeview_cursor_changed; });
    }
#endif
}

std::pair<std::string, std::vector<std::string>> VcView::get_comparison() {
    return std::pair<std::string, std::vector<std::string>>(TYPE_VC, {this->location});
}

void VcView::recompute_label() {
    std::string location = this->location;
#if 0
    if isinstance(location, str):
        location = location.decode(sys.getfilesystemencoding(), 'replace');
#endif
    this->label_text = boost::filesystem::path(location).filename().string();
    // TRANSLATORS: This is the location of the directory the user is diffing
    boost::format fmt(_("%s: %s"));
    fmt % (_("Location"), location);
    this->tooltip_text = fmt.str();
    this->label_changed();
}

void VcView::_search_recursively_iter(Gtk::TreeModel::iterator iterstart) {
    Glib::ustring rootname = this->model->value_path(iterstart, 0);
    int prefixlen = this->location.size() + 1;
    std::set<std::pair<__dev_t, __ino_t>> symlinks_followed;
    std::vector<std::pair<Gtk::TreeModel::Path, Glib::ustring>> todo = {std::pair<Gtk::TreeModel::Path, Glib::ustring>(this->model->get_path(iterstart), rootname)};

    Glib::RefPtr<Gtk::Action> tmp = this->actiongroup->get_action("VcFlatten");
    Glib::RefPtr<Gtk::ToggleAction> tmp2 = Glib::RefPtr<Gtk::ToggleAction>::cast_static(tmp);
    bool flattened = tmp2->get_active();
    std::vector<Glib::ustring>filters;

    for (std::pair<Glib::ustring, Glib::ustring> a : this->state_actions) {
        Glib::RefPtr<Gtk::Action> tmp = this->actiongroup->get_action(a.first);
        Glib::RefPtr<Gtk::ToggleAction> tmp2 = Glib::RefPtr<Gtk::ToggleAction>::cast_static(tmp);
        if (tmp2->get_active() and !a.second.empty()) {
            filters.push_back(a.second);
        }
    }

#if 0
    yield _("Scanning %s") % rootname
#endif
    this->vc->cache_inventory(rootname);
    while (!todo.empty()) {
        // This needs to happen sorted and depth-first in order for our row
        // references to remain valid while we traverse.
#if 0
        todo.sort();
#endif
        std::string treepath;
        std::string path;
#if 0
        treepath, path = todo.pop(0);
#endif
        Gtk::TreeModel::iterator it = this->model->get_iter(treepath);
#if 0
        yield _("Scanning %s") % path[prefixlen:];
#endif

        std::vector<Entry*> entries = this->vc->listdir(path);
#if 0
        entries = [e for e in entries if any(f(e) for f in filters)];
#endif
        for (Entry* e : entries) {
            if (e->isdir and e->state != STATE_REMOVED) {
                struct stat st;
                int retval = lstat(e->path.c_str(), &st);
                // Covers certain unreadable symlink cases; see bgo#585895
                if (retval != 0) {
                    boost::format fmt("%s: %s");
                    fmt % e->path % strerror(errno);
                    Glib::ustring error_string = fmt.str();
                    Gtk::TreeRow row = *it;
#if 0
                    this->model->add_error(row, error_string, 0);
#endif
                    continue;
                }

                if (S_ISLNK(st.st_mode)) {
                    std::pair<__dev_t, __ino_t> key(st.st_dev, st.st_ino);
                    if (std::find(symlinks_followed.begin(), symlinks_followed.end(), key) != symlinks_followed.end()) {
                        continue;
                    }
                    symlinks_followed.insert(key);
                }

                if (flattened) {
                    if (e->state != STATE_IGNORED) {
                        // If directory state is changed, render it in
                        // in flattened mode.
                        if (e->state != STATE_NORMAL) {
#if 0
                            Gtk::TreeStore::iterator child = this->model->add_entries(*it, {e->path});
                            this->_update_item_state(child, e, path.substr(prefixlen));
#endif
                        }
                        todo.push_back(std::pair<Gtk::TreeModel::Path, Glib::ustring>(Gtk::TreePath(), e->path));
                    }
                    continue;
                }
            }

#if 0
            Gtk::TreeStore::iterator child = this->model->add_entries(*it, {e->path});
#else
            Gtk::TreeStore::iterator child;
#endif
            if (e->isdir and e->state != STATE_IGNORED) {
                todo.push_back(std::pair<Gtk::TreeModel::Path, Glib::ustring>(this->model->get_path(child), e->path));
            }
#if 0
            this->_update_item_state(child, e, path[prefixlen:]);
#endif
        }

        if (flattened) {
            Gtk::TreePath root;
            this->treeview->expand_row(root, false);
        } else {
            if (entries.empty()) {
                this->model->add_empty((*it)->children(), _("(Empty)"));
            }
#if 0
            if (any(e.state != tree.STATE_NORMAL for e in entries)) {
                this->treeview.expand_to_path(treepath);
            }
#endif
        }
    }
}

// TODO: This doesn't fire when the user selects a shortcut folder
void VcView::on_fileentry_file_set(Gtk::FileChooserButton* fileentry) {
    Glib::RefPtr<Gio::File> directory = fileentry->get_file();
    std::string path = directory->get_path();
    this->set_location(path);
}

Gtk::ResponseType VcView::on_delete_event(int appquit) {
    this->scheduler.remove_all_tasks();
#if 0
    this->signal_close().emit(0);
#endif
    return Gtk::RESPONSE_OK;
}

void VcView::on_row_activated(const Gtk::TreeModel::Path& path) {
    Gtk::TreeModel::iterator it = this->model->get_iter(path);
#if 0
    if (this->model->iter_has_child(it)) {
        if (this->treeview->row_expanded(path)) {
            this->treeview->collapse_row(path);
        } else {
            this->treeview->expand_row(path, false);
        }
    } else {
        Glib::ustring path = this->model->value_path(it, 0);
        if (not this->model->is_folder(it, 0, path)) {
            this->run_diff(path);
        }
    }
#endif
}

void VcView::run_diff(std::string path) {
    if (boost::filesystem::is_directory(path)) {
        this->m_signal_create_diff.emit({path}, false, false, "", {});
        return;
    }

    Glib::Value<bool> value;
    value.init(Glib::Value<bool>::value_type());
    this->get_property_value("left_is_local", value);
    bool left_is_local = value.get();
    std::string basename;
#if 0
    basename = os.path.basename(path);
#endif
    std::map<std::string, boost::variant<bool, std::string, int, std::vector<std::string>, VcView*>> meta = {
        {"parent", this},
        {"prompt_resolve", false}
    };

    std::vector<std::string> temps;
    bool auto_compare = false;
    bool auto_merge = false;
    std::string merge_output;
    std::vector<std::string> diffs;
    // May have removed directories in list.
    Entry* vc_entry = this->vc->get_entry(path);
    if (vc_entry && vc_entry->state == STATE_CONFLICT and
            this->vc->supports_get_path_for_conflict()) {
        boost::format fmt_local_label(_("%s — local"));
        fmt_local_label % basename;
        std::string local_label = fmt_local_label.str();
        boost::format fmt_remote_label(_("%s — remote"));
        fmt_remote_label % basename;
        std::string remote_label = fmt_remote_label.str();
        // We create new temp files for other, base and this, and
        // then set the output to the current file.
        std::vector<ConflictType> conflicts;
#if 0
        if (this->props.merge_file_order == "local-merge-remote") {
#else
        if (true) {
#endif
            conflicts = {CONFLICT_THIS, CONFLICT_MERGED, CONFLICT_OTHER};
            std::vector<std::string> tmp = { local_label, "", remote_label };
            meta["labels"] = tmp;
            boost::format fmt_tablabel (_("%s (local, merge, remote)"));
            fmt_tablabel % basename;
            meta["tablabel"] = fmt_tablabel.str();
        } else {
            conflicts = {CONFLICT_OTHER, CONFLICT_MERGED, CONFLICT_THIS};
            std::vector<std::string> tmp = { remote_label, "", local_label };
            meta["labels"] = tmp;
            boost::format fmt_tablabel (_("%s (remote, merge, local)"));
            fmt_tablabel % basename;
            meta["tablabel"] = fmt_tablabel.str();
        }
#if 0
        diffs = [this->vc.get_path_for_conflict(path, conflict=c)
                 for c in conflicts];
        temps = [p for p, is_temp in diffs if is_temp];
        diffs = [p for p, is_temp in diffs];
#endif
        meta["prompt_resolve"] = true;
        merge_output = path;
    } else {
        boost::format fmt_remote_label(_("%s — repository"));
        fmt_remote_label % basename;
        std::string remote_label = fmt_remote_label.str();
        std::string comp_path = this->vc->get_path_for_repo_file(path);
        std::vector<std::string> temps = {comp_path};
        if (left_is_local) {
#if 0
            diffs = [path, comp_path];
#endif
            std::vector<std::string> tmp = { "", remote_label };
            meta["labels"] = tmp;
            boost::format fmt_tablabel(_("%s (working, repository)"));
            fmt_tablabel % basename;
            meta["tablabel"] = fmt_tablabel.str();
        } else {
#if 0
            diffs = [comp_path, path];
#endif
            std::vector<std::string> tmp = { remote_label, "" };
            meta["labels"] = tmp;
            boost::format fmt_tablabel(_("%s (repository, working)"));
            fmt_tablabel % basename;
            meta["tablabel"] = fmt_tablabel.str();
        }
    }

    for (std::string temp_file : temps) {
#if 0
        chmod(temp_file.c_str(), 0o444);
        _temp_files.append(temp_file);
#endif
    }

    this->signal_create_diff().emit(diffs, auto_compare, auto_merge, merge_output, meta);
}

bool VcView::on_treeview_popup_menu(int treeview) {
    guint32 time = gtk_get_current_event_time();
#if 0
    this->popup_menu->popup(None, None, None, None, 0, time);
#endif
    return true;
}

bool VcView::on_button_press_event(Gtk::TreeView* treeview, GdkEventButton* event) {
    if (event->button == 3) {
        Gtk::TreeModel::Path path;
        bool b = treeview->get_path_at_pos(int(event->x), int(event->y), path);
        if (!b) {
            return false;
        }
        Glib::RefPtr<Gtk::TreeSelection> selection = treeview->get_selection();
        Glib::RefPtr<Gtk::TreeModel> tmp = static_cast<Glib::RefPtr<Gtk::TreeModel>>(model);
        std::vector<Gtk::TreeModel::Path> rows = selection->get_selected_rows(tmp);

#if 0
        if (path[0] not in rows) {
            selection.unselect_all();
            selection.select_path(path[0]);
            treeview.set_cursor(path[0]);
        }
#endif

#if 0
        this->popup_menu.popup(None, None, None, None, event.button,
                              event.time);
#endif
        return true;
    }
    return false;
}

void VcView::on_button_flatten_toggled() {
#if 0
    Glib::RefPtr<const Gtk::ToggleAction> action = dynamic_cast<Glib::RefPtr<const Gtk::ToggleAction>>(this->actiongroup->get_action("VcFlatten"));
    this->treeview_column_location->set_visible(action->get_active());
#endif
    this->on_filter_state_toggled();
}

void VcView::on_filter_state_toggled() {
    std::vector<Glib::ustring> active_filters;
    for (std::pair<Glib::ustring, Glib::ustring> a : this->state_actions) {
#if 0
        if (dynamic_cast<Glib::RefPtr<Gtk::ToggleAction>>(this->actiongroup->get_action(a.first))->get_active()) {
            active_filters.push_back(a.first);
        }
#endif
    }

    std::set<Glib::ustring> set_active_filters(active_filters.begin(), active_filters.end());
    std::set<Glib::ustring> set_state_filters(this->state_filters.begin(), this->state_filters.end());
    if (set_active_filters == set_state_filters) {
        return;
    }

    this->state_filters = active_filters;
#if 0
    this->props.status_filters = active_filters;
#endif
    this->refresh();
}

void VcView::on_treeview_selection_changed() {

    Glib::RefPtr<Gtk::TreeSelection> selection = this->treeview->get_selection();
    Glib::RefPtr<Gtk::TreeModel> tmp = static_cast<Glib::RefPtr<Gtk::TreeModel>>(model);
    std::vector<Gtk::TreeModel::Path> rows = selection->get_selected_rows(tmp);

    if (this->vc->supports_update_actions_for_paths()) {
        std::map<std::string, FileState> path_states;
        for (Gtk::TreeModel::Path r : rows) {
            path_states[this->model->value_path(model->get_iter(r), 0)] = this->model->get_state(model->get_iter(r), 0);
        }
        std::map<std::string, bool> action_sensitivity = {
            {"VcCompare", false},
            {"VcCommit", false},
            {"VcUpdate", false},
            {"VcPush", false},
            {"VcAdd", false},
            {"VcResolved", false},
            {"VcRemove", false},
            {"VcRevert", false},
            {"VcDeleteLocally", path_states.count(this->vc->root) == 0}
        };
        this->vc->update_actions_for_paths(path_states, action_sensitivity);
        for (std::pair<std::string, bool> tmp : action_sensitivity) {
            std::string action = tmp.first;
            bool sensitivity = tmp.second;
            this->actiongroup->get_action(action)->set_sensitive(sensitivity);
        }
    } else {
        bool have_selection = !rows.empty();
        for (Glib::ustring action : this->valid_vc_actions) {
            this->actiongroup->get_action(action)->set_sensitive(have_selection);
        }
    }
}

std::vector<std::string> VcView::_get_selected_files() {
    Glib::RefPtr<Gtk::TreeModel> tmp = static_cast<Glib::RefPtr<Gtk::TreeModel>>(model);
    std::vector<Gtk::TreeModel::Path> rows = this->treeview->get_selection()->get_selected_rows(tmp);
    std::vector<Glib::ustring> sel;
    for (Gtk::TreeModel::Path r : rows) {
        sel.push_back(this->model->value_path(this->model->get_iter(r), 0));
    }
    // Remove empty entries and trailing slashes
    std::vector<std::string> result;
    for (Glib::ustring x : sel) {
        if (!x.empty()) {
            if (boost::algorithm::ends_with(x, "/")) {
                result.push_back(x.substr(0, x.size() - 1));
            } else {
                result.push_back(x);
            }
        }
    }
    return result;
}

/*! Run 'command' on 'files'. Return a tuple of the directory the
 * command was executed in and the output of the command. */
void VcView::_command_iter(int command, int files, int refresh, int *working_dir) {
#if 0
    def shelljoin(command):
        def quote(s):
            return "\"%s\"" % s if len(s.split()) > 1 else s
        return " ".join(quote(tok) for tok in command)

    msg = shelljoin(command)
    yield "[%s] %s" % (this->label_text, msg.replace("\n", "\t"))
    def relpath(pbase, p):
        kill = 0
        if len(pbase) and p.startswith(pbase):
            kill = len(pbase) + 1
        return p[kill:] or "."
    if working_dir:
        workdir = this->vc.get_working_directory(working_dir)
    elif len(files) == 1 and os.path.isdir(files[0]):
        workdir = this->vc.get_working_directory(files[0])
    else:
        workdir = this->vc.get_working_directory(_commonprefix(files))
    files = [relpath(workdir, f) for f in files]
    r = None
    this->consolestream.command(shelljoin(command + files) + " (in %s)\n" % workdir)
    readiter = misc.read_pipe_iter(command + files, this->consolestream,
                                   workdir=workdir)
    try:
        while r is None:
            r = next(readiter)
            this->consolestream.output(r)
            yield 1
    except IOError as err:
        misc.error_dialog(
            "Error running command",
            "While running '%s'\nError: %s" % (msg, err))
    this->consolestream.output("\n")

    returncode = next(readiter)
    if returncode:
        this->console_vbox.show()

    if refresh:
        this->refresh_partial(workdir)
    yield workdir, r;
#endif
}

bool VcView::has_command(std::string command) {
#if 0
    return std::find(this->command_map.begin(), this->command_map().end(), command) != this->command_map.end();
#else
    return false;
#endif
}

void VcView::command(std::string command, std::vector<std::string> files) {
    if (not this->has_command(command)) {
        std::cerr << "Couldn't understand command " << command << std::endl;
    }

#if 0
    command = getattr(self.vc, self.command_map[command])
    command(self._command, files)
#endif
}

/*! Run 'command' on 'files'. */
void VcView::_command(int command, int files, int refresh, int *working_dir) {
#if 0
    this->scheduler.add_task(this->_command_iter(command, files, refresh,
                                               working_dir));
#endif
}

void VcView::_command_on_selected(int command, int refresh) {
#if 0
    std::vector<std::string> files = this->_get_selected_files();
    if (!files.empty()) {
        this->_command(command, files, refresh);
    }
#endif
}

void VcView::on_button_update_clicked() {
#if 0
    try {
        this->vc->update(this->_command, this->_get_selected_files());
    } catch (NotImplementedError&) {
        this->_command_on_selected(this->vc->update_command());
    }
#endif
}

void VcView::on_button_push_clicked() {
    PushDialog(this).run();
}

void VcView::on_button_commit_clicked() {
    CommitDialog(this).run();
}

void VcView::on_button_add_clicked() {
#if 0
    try {
        this->vc.add(this->_command, this->_get_selected_files())
    } catch (NotImplementedError&) {
        this->_command_on_selected(this->vc.add_command())
    }
#endif
}

void VcView::on_button_remove_clicked() {
#if 0
    std::vector<std::string> selected = this->_get_selected_files();
    if (any(os.path.isdir(p) for p in selected)) {
        // TODO: Improve and reuse this dialog for the non-VC delete action
        Gtk::MessageDialog dialog(*this->widget->get_toplevel(), _("Remove folder and all its files?"),
                                  false, Gtk::MESSAGE_WARNING, true);
        dialog.set_secondary_text(
            _("This will remove all selected files and folders, and all "
              "files within any selected folders, from version control."));

        dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        dialog.add_button(_("_Remove"), Gtk::RESPONSE_OK);
        int response = dialog.run();
        dialog.close();
        if (response != Gtk::RESPONSE_OK) {
            return;
        }
    }

    try {
        this->vc->remove(this->_command, selected)
    } catch (NotImplementedError &e) {
        this->_command_on_selected(this->vc->remove_command());
    }
#endif
}

void VcView::on_button_resolved_clicked() {
#if 0
    try {
        this->vc.resolve(this->_command, this->_get_selected_files())
    } catch (NotImplementedError &e) {
        this->_command_on_selected(this->vc.resolved_command());
    }
#endif
}

void VcView::on_button_revert_clicked() {
#if 0
    try {
        this->vc.revert(this->_command, this->_get_selected_files())
    } catch (NotImplementedError &e) {
        this->_command_on_selected(this->vc.revert_command());
    }
#endif
}

void VcView::on_button_delete_clicked() {
#if 0
    files = this->_get_selected_files()
    for name in files:
        try:
            gfile = Gio.File.new_for_path(name)
            gfile.trash(None)
        except GLib.GError as e:
            misc.error_dialog(_("Error removing %s") % name, str(e))
    workdir = _commonprefix(files)
    this->refresh_partial(workdir);
#endif
}

void VcView::on_button_diff_clicked() {
    std::vector<std::string> files = this->_get_selected_files();
    for (std::string f : files) {
        this->run_diff(f);
    }
}

void VcView::open_external() {
#if 0
    this->_open_files(this->_get_selected_files());
#endif
}

void VcView::refresh() {
#if 0
    this->set_location(this->model.value_path(this->model.get_iter_first(), 0));
#endif
}

void VcView::refresh_partial(int where) {
#if 0
    if not this->actiongroup.get_action("VcFlatten").get_active():
        it = this->find_iter_by_name(where)
        if it:
            newiter = this->model.insert_after(None, it)
            this->model.set_value(
                newiter, this->model.column_index(tree.COL_PATH, 0), where)
            this->model.set_path_state(newiter, 0, tree.STATE_NORMAL, True)
            this->model.remove(it)
            this->treeview.grab_focus()
            this->treeview.get_selection().select_iter(newiter)
            this->scheduler.add_task(this->_search_recursively_iter(newiter))
            this->scheduler.add_task(this->on_treeview_selection_changed)
            this->scheduler.add_task(this->on_treeview_cursor_changed)
    else:
        // XXX fixme
        this->refresh();
#endif
}

void VcView::_update_item_state(Gtk::TreeModel::iterator& it, int vcentry, int location) {
#if 0
    e = vcentry
    this->model.set_path_state(it, 0, e.state, e.isdir)

    def setcol(col, val):
        this->model.set_value(it, this->model.column_index(col, 0), val)
    setcol(COL_LOCATION, location)
    setcol(COL_STATUS, e.get_status())
    setcol(COL_REVISION, e.rev)
    setcol(COL_OPTIONS, e.options);
#endif
}

void VcView::on_file_changed(std::string filename) {
#if 0
    it = this->find_iter_by_name(filename)
    if it:
        path = this->model.value_path(it, 0)
        this->vc.update_file_state(path)
        files = this->vc.lookup_files([], [(os.path.basename(path), path)])[1]
        for e in files:
            if e.path == path:
                prefixlen = 1 + len( this->model.value_path( this->model.get_iter_first(), 0 ) )
                this->_update_item_state( it, e, e.parent[prefixlen:])
                return;
#endif
}

void VcView::find_iter_by_name(int name) {
#if 0
    it = this->model.get_iter_first()
    path = this->model.value_path(it, 0)
    while it:
        if name == path:
            return it
        elif name.startswith(path):
            child = this->model.iter_children( it )
            while child:
                path = this->model.value_path(child, 0)
                if name == path:
                    return child
                elif name.startswith(path):
                    break
                else:
                    child = this->model.iter_next( child )
            it = child
        else:
            break
    return None;
#endif
}

void VcView::on_consoleview_populate_popup(int textview, int menu) {
#if 0
    buf = textview.get_buffer()
    clear_cb = lambda *args: buf.delete(*buf.get_bounds())
    clear_action = Gtk.ImageMenuItem(Gtk.STOCK_CLEAR)
    clear_action.connect("activate", clear_cb)
    menu.insert(clear_action, 0)
    menu.insert(Gtk.SeparatorMenuItem(), 1)
    menu.show_all();
#endif
}

void VcView::on_treeview_cursor_changed(int *args) {
    Gtk::TreeModel::Path cursor_path;
    Gtk::TreeViewColumn* cursor_col;
    this->treeview->get_cursor(cursor_path, cursor_col);
    if (not cursor_path) {
#if 0
        this->emit("next-diff-changed", false, false);
#endif
        this->current_path = cursor_path;
        return;
    }

    // If invoked directly rather than through a callback, we always check
    bool skip;
    if (not args) {
        skip = false;
    } else {
        Gtk::TreeModel::iterator old_cursor = this->model->get_iter(this->current_path);
        if (old_cursor == Gtk::TreeModel::iterator()) {
            // An invalid path gives ValueError
            skip = false;
        } else {
            // We can skip recalculation if the new cursor is between
            // the previous/next bounds, and we weren't on a changed row
            int state = this->model->get_state(old_cursor, 0);
            if (state != STATE_NORMAL && state != STATE_EMPTY) {
                skip = false;
            } else {
                if (!this->prev_path and !this->next_path) {
                    skip = true;
                } else if (!this->prev_path) {
                    skip = cursor_path < this->next_path;
                } else if (!this->next_path) {
                    skip = this->prev_path < cursor_path;
                } else {
                    skip = this->prev_path < cursor_path && cursor_path < this->next_path;
                }
            }
        }
    }

    if (not skip) {
        std::pair<Gtk::TreeModel::Path, Gtk::TreeModel::Path> tmp = this->model->_find_next_prev_diff(cursor_path);
        Gtk::TreeModel::Path prev = tmp.first;
        Gtk::TreeModel::Path next = tmp.second;
        this->prev_path = prev;
        this->next_path = next;
#if 0
        std::pair<bool, bool>have_next_diffs = (!!prev, !!next);
        this->emit("next-diff-changed", *have_next_diffs);
#endif
    }
    this->current_path = cursor_path;
}

void VcView::next_diff(GdkScrollDirection direction, bool centered) {
    Gtk::TreePath path;
    if (direction == GDK_SCROLL_UP) {
        path = this->prev_path;
    } else {
        path = this->next_path;
    }
    if (path) {
        this->treeview->expand_to_path(path);
        this->treeview->set_cursor(path);
    }
}

void VcView::on_refresh_activate() {
    this->on_fileentry_file_set(this->fileentry[0]);
}

void VcView::on_find_activate() {
#if 0
    this->treeview->signal_start_interactive_search().emit();
#endif
}
