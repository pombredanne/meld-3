/* Copyright (C) 2002-2006 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2009-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include <cassert>
#include <gtkmm.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>
#include <functional>

#include "melddoc.h"
#include "tree.h"
#include "misc.h"
#include "recent.h"
#include "diffmap.h"
#include "ui/gnomeglade.h"
#include "ui/emblemcellrenderer.h"
#include "dirdiff.h"
#include "settings.h"

#include "settings.h"

#if 0
//###############################################################################
//
// Local Functions
//
//###############################################################################

class StatItem(namedtuple('StatItem', 'mode size time')):
    __slots__ = ()

    static StatItem _make(int cls, int stat_result) {
        return StatItem(stat.S_IFMT(stat_result.st_mode),
                        stat_result.st_size, stat_result.st_mtime);
    }

    def shallow_equal(self, other, time_resolution_ns):
        if this->size != other.size:
            return False

        # Shortcut to avoid expensive Decimal calculations. 2 seconds is our
        # current accuracy threshold (for VFAT), so should be safe for now.
        if abs(this->time - other.time) > 2:
            return False

        dectime1 = Decimal(str(this->time)).scaleb(Decimal(9)).quantize(1)
        dectime2 = Decimal(str(other.time)).scaleb(Decimal(9)).quantize(1)
        mtime1 = dectime1 // time_resolution_ns
        mtime2 = dectime2 // time_resolution_ns

        return mtime1 == mtime2


CacheResult = namedtuple('CacheResult', 'stats result')


_cache = {}
Same, SameFiltered, DodgySame, DodgyDifferent, Different, FileError = \
    list(range(6))
# TODO: Get the block size from os.stat
CHUNK_SIZE = 4096


def all_same(lst):
    return not lst or lst.count(lst[0]) == len(lst)


def remove_blank_lines(text):
    splits = text.splitlines()
    lines = text.splitlines(True)
    blanks = set([i for i, l in enumerate(splits) if not l])
    lines = [l for i, l in enumerate(lines) if i not in blanks]
    return ''.join(lines)

/*!
 * Determine whether a list of files are the same.
 *
 * Possible results are:
 * Same: The files are the same
 * SameFiltered: The files are identical only after filtering with 'regexes'
 * DodgySame: The files are superficially the same (i.e., type, size, mtime)
 * DodgyDifferent: The files are superficially different
 * FileError: There was a problem reading one or more of the files
 */
def _files_same(files, regexes, comparison_args):

    # One file is the same as itself
    if len(files) < 2:
        return Same

    files = tuple(files)
    regexes = tuple(regexes)
    stats = tuple([StatItem._make(os.stat(f)) for f in files])

    shallow_comparison = comparison_args['shallow-comparison']
    time_resolution_ns = comparison_args['time-resolution']
    ignore_blank_lines = comparison_args['ignore_blank_lines']

    need_contents = regexes or ignore_blank_lines

    # If all entries are directories, they are considered to be the same
    if all([stat.S_ISDIR(s.mode) for s in stats]):
        return Same

    # If any entries are not regular files, consider them different
    if not all([stat.S_ISREG(s.mode) for s in stats]):
        return Different

    # Compare files superficially if the options tells us to
    if shallow_comparison:
        if all(s.shallow_equal(stats[0], time_resolution_ns) for s in stats[1:]):
            return DodgySame
        else:
            return Different

    # If there are no text filters, unequal sizes imply a difference
    if not need_contents and not all_same([s.size for s in stats]):
        return Different

    # Check the cache before doing the expensive comparison
    cache_key = (files, regexes, ignore_blank_lines)
    cache = _cache.get(cache_key)
    if cache and cache.stats == stats:
        return cache.result

    # Open files and compare bit-by-bit
    contents = [[] for f in files]
    result = None

    try:
        handles = [open(f, "rb") for f in files]
        try:
            data = [h.read(CHUNK_SIZE) for h in handles]

            // Rough test to see whether files are binary. If files are guessed
            // to be binary, we don't examine contents for speed and space.
            if any(["\0" in d for d in data]):
                need_contents = False

            while True:
                if all_same(data):
                    if not data[0]:
                        break
                else:
                    result = Different
                    if not need_contents:
                        break

                if need_contents:
                    for i in range(len(data)):
                        contents[i].append(data[i])

                data = [h.read(CHUNK_SIZE) for h in handles]

        // Files are too large; we can't apply filters
        except (MemoryError, OverflowError):
            result = DodgySame if all_same(stats) else DodgyDifferent
        finally:
            for h in handles:
                h.close()
    except IOError:
        // Don't cache generic errors as results
        return FileError

    if result is None:
        result = Same

    if result == Different and need_contents:
        contents = ["".join(c) for c in contents]
        for r in regexes:
            contents = [re.sub(r, "", c) for c in contents]
        if ignore_blank_lines:
            contents = [remove_blank_lines(c) for c in contents]
        result = SameFiltered if all_same(contents) else Different

    _cache[cache_key] = CacheResult(stats, result)
    return result

#endif
const int COL_EMBLEM = COL_END;
const int COL_SIZE = COL_END + 1;
const int COL_TIME = COL_END + 2;
const int COL_PERMS = COL_END + 3;

class DirDiffTreeStore : public DiffTreeStore {
public:
    DirDiffTreeStore(int ntree) : DiffTreeStore(ntree, std::vector<const std::type_info*>{&typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string)}) {
    }
};


CanonicalListing::CanonicalListing(int n, std::function<std::string(std::string)> canonicalize) {
#if 0
    this->items = collections.defaultdict(lambda: [None] * n);
#endif
    if (canonicalize) {
        this->canonicalize = canonicalize;
        this->add = std::bind(&CanonicalListing::add_canon, this, std::placeholders::_1, std::placeholders::_2);
    } else {
        this->add = std::bind(&CanonicalListing::add_noncanon, this, std::placeholders::_1, std::placeholders::_2);
    }
}

void CanonicalListing::add_noncanon(int pane, std::vector<Glib::ustring> item) {
#if 0
    this->items[item][pane] = item;
#endif
}

void CanonicalListing::add_canon(int pane, std::vector<Glib::ustring> item) {
#if 0
    Glib::ustring ci = this->canonicalize(item);
    if (!this->items[ci][pane]) {
        this->items[ci][pane] = item;
    } else {
        this->errors.push_back(std::tuple<int, std::string, int>(pane, item, this->items[ci][pane]));
    }
#endif
}

void CanonicalListing::get() {
#if 0
    first = lambda seq: next(s for s in seq if s);
    filled = lambda seq: tuple([s or first(seq) for s in seq]);
    return sorted(filled(v) for v in this->items.values());
#endif
}


//###############################################################################
//
// DirDiff
//
//###############################################################################

DirDiff::DirDiff(int num_panes, SchedulerBase& scheduler) : MeldDoc(scheduler, "dirdiff.ui", "dirdiff", {"DirdiffActions"}) {

    bind_settings(this, __gsettings_bindings__);

    this->ui_file = gnomeglade_ui_file("dirdiff-ui.xml");
    this->DirdiffActions = Glib::RefPtr<Gtk::ActionGroup>::cast_static(this->builder->get_object("DirdiffActions"));
    this->actiongroup = this->DirdiffActions;
#if 0
    this->actiongroup->set_translation_domain("meld");
#endif

    this->create_name_filters();
    this->create_text_filters();

    this->settings_handlers = {
        meldsettings->signal_file_filters_changed().connect(sigc::mem_fun(this, &DirDiff::on_file_filters_changed)),
        meldsettings->signal_text_filters_changed().connect(sigc::mem_fun(this, &DirDiff::on_text_filters_changed))
    };

    this->builder->get_widget("treeview0", this->treeview0);
    this->builder->get_widget("treeview1", this->treeview1);
    this->builder->get_widget("treeview2", this->treeview2);
    this->treeview = { this->treeview0, this->treeview1, this->treeview2 };

    this->builder->get_widget("fileentry0", this->fileentry0);
    this->builder->get_widget("fileentry1", this->fileentry1);
    this->builder->get_widget("fileentry2", this->fileentry2);
    this->fileentry = { this->fileentry0, this->fileentry1, this->fileentry2 };

    this->builder->get_widget("scrolledwindow0", this->scrolledwindow0);
    this->builder->get_widget("scrolledwindow1", this->scrolledwindow1);
    this->builder->get_widget("scrolledwindow2", this->scrolledwindow2);
    this->scrolledwindow = { this->scrolledwindow0, this->scrolledwindow1, this->scrolledwindow2 };

    this->builder->get_widget_derived("diffmap0", this->diffmap0);
    this->builder->get_widget_derived("diffmap1", this->diffmap1);
    this->diffmap = { this->diffmap0, this->diffmap1 };

    this->builder->get_widget_derived("linkmap0", this->linkmap0);
    this->builder->get_widget_derived("linkmap1", this->linkmap1);
    this->linkmap = { this->linkmap0, this->linkmap1 };

    this->builder->get_widget("msgarea_mgr0", this->msgarea_mgr0);
    this->builder->get_widget("msgarea_mgr1", this->msgarea_mgr1);
    this->builder->get_widget("msgarea_mgr2", this->msgarea_mgr2);
    this->msgarea_mgr = { this->msgarea_mgr0, this->msgarea_mgr1, this->msgarea_mgr2 };

    this->builder->get_widget("vbox0", this->vbox0);
    this->builder->get_widget("vbox1", this->vbox1);
    this->builder->get_widget("vbox2", this->vbox2);
    this->vbox = { this->vbox0, this->vbox1, this->vbox2 };

    this->builder->get_widget("dummy_toolbar_linkmap0", this->dummy_toolbar_linkmap0);
    this->builder->get_widget("dummy_toolbar_linkmap1", this->dummy_toolbar_linkmap1);
    this->dummy_toolbar_linkmap = { this->dummy_toolbar_linkmap0, this->dummy_toolbar_linkmap1 };

    this->builder->get_widget("file_toolbar0", this->file_toolbar0);
    this->builder->get_widget("file_toolbar1", this->file_toolbar1);
    this->builder->get_widget("file_toolbar2", this->file_toolbar2);
    this->file_toolbar = { this->file_toolbar0, this->file_toolbar1, this->file_toolbar2 };

    assert(this->widget);
    this->on_style_updated();
    this->widget->signal_style_updated().connect(sigc::mem_fun(this, &DirDiff::on_style_updated));

    this->set_num_panes(num_panes);

    this->widget->signal_style_updated().connect(sigc::bind(sigc::mem_fun(this->model, &DiffTreeStore::on_style_updated), this->widget));
#if 0
    this->model->on_style_updated(this->widget);
#endif

    for (Gtk::TreeView* treeview : this->treeview) {
        sigc::connection handler_id_in = treeview->signal_focus_in_event().connect(sigc::bind(sigc::mem_fun(this, &DirDiff::on_treeview_focus_in_event), treeview));
        this->focus_in_events.push_back(handler_id_in);
        sigc::connection handler_id_out = treeview->signal_focus_out_event().connect(sigc::mem_fun(this, &DirDiff::on_treeview_focus_out_event));
        this->focus_out_events.push_back(handler_id_out);
#if 0
        treeview->set_search_equal_func(this->model->treeview_search_cb);
#endif
    }
    this->on_treeview_focus_out_event(nullptr);
    this->focus_pane = nullptr;

    // One column-dict for each treeview, for changing visibility and order
    for (int i = 0; i < 3; i++) {
        // Create icon and filename CellRenderer
        Gtk::TreeViewColumn* column1 = Gtk::manage(new Gtk::TreeViewColumn(_("Name")));
        column1->set_resizable(true);
        Gtk::CellRendererText rentext1;
        EmblemCellRenderer renicon;
        column1->pack_start(renicon, false);
        column1->pack_start(rentext1, true);
#if 0
        column1->set_attributes(rentext1, markup=this->model->column_index(tree.COL_TEXT, i),
                              foreground=this->model->column_index(tree.COL_FG, i),
                              style=this->model->column_index(tree.COL_STYLE, i),
                              weight=this->model->column_index(tree.COL_WEIGHT, i),
                              strikethrough=this->model->column_index(tree.COL_STRIKE, i));
        column1->set_attributes(renicon,
                              icon_name=this->model->column_index(tree.COL_ICON, i),
                              emblem_name=this->model->column_index(COL_EMBLEM, i),
                              icon_tint=this->model->column_index(tree.COL_TINT, i));
#endif
        this->treeview[i]->append_column(*column1);
        this->columns_dict[i]["name"] = column1;
        // Create file size CellRenderer
        Gtk::TreeViewColumn* column2 = Gtk::manage(new Gtk::TreeViewColumn(_("Size")));
        column2->set_resizable(true);
        Gtk::CellRendererText rentext2;
        column2->pack_start(rentext2, true);
#if 0
        column2->set_attributes(rentext2, markup=this->model->column_index(COL_SIZE, i));
#endif
        this->treeview[i]->append_column(*column2);
        this->columns_dict[i]["size"] = column2;
        // Create date-time CellRenderer
        Gtk::TreeViewColumn* column3 = Gtk::manage(new Gtk::TreeViewColumn(_("Modification time")));
        column3->set_resizable(true);
        Gtk::CellRendererText rentext3;
        column3->pack_start(rentext3, true);
#if 0
        column3->set_attributes(rentext3, markup=this->model->column_index(COL_TIME, i));
#endif
        this->treeview[i]->append_column(*column3);
        this->columns_dict[i]["modification time"] = column3;
        // Create permissions CellRenderer
        Gtk::TreeViewColumn* column4 = Gtk::manage(new Gtk::TreeViewColumn(_("Permissions")));
        column4->set_resizable(true);
        Gtk::CellRendererText rentext4;
        column4->pack_start(rentext4, false);
#if 0
        column4->set_attributes(rentext4, markup=this->model->column_index(COL_PERMS, i));
#endif
        this->treeview[i]->append_column(*column4);
        this->columns_dict[i]["permissions"] = column4;
    }

    for (int i = 0; i < 3; i++) {
        Glib::RefPtr<Gtk::TreeSelection> selection = this->treeview[i]->get_selection();
        selection->set_mode(Gtk::SELECTION_MULTIPLE);
#if 0
        selection->signal_changed().connect(sigc::mem_fun(this, &DirDiff::on_treeview_selection_changed));
#endif
        this->scrolledwindow[i]->get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DirDiff::_sync_vscroll));
        this->scrolledwindow[i]->get_hadjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DirDiff::_sync_hscroll));
    }

    this->update_treeview_columns(settings, "folder-columns");
#if 0
    settings.connect("changed::folder-columns",
                     this->update_treeview_columns);
#endif

    this->update_comparator();
#if 0
    this->connect("notify::shallow-comparison", this->update_comparator);
    this->connect("notify::time-resolution", this->update_comparator);
    this->connect("notify::ignore-blank-lines", this->update_comparator);
#endif

    for (std::pair<FileState, std::pair<Glib::ustring, Glib::ustring>> s : this->state_actions) {
#if 0
        if (std::find(this->props.status_filters.begin(), this->props.status_filters.end(), this->state_actions[s].first) != this->props.status_filters.end()) {
            this->state_filters.push_back(s);
            action_name = this->state_actions[s].second;
            this->actiongroup.get_action(action_name).set_active(true);
        }
#endif
    }
}

Gdk::RGBA DirDiff::lookup(Glib::RefPtr<Gtk::StyleContext> style, const Glib::ustring& name, const Glib::ustring& _default) {
    Gdk::RGBA colour;
    bool found = style->lookup_color(name, colour);
    if (not found) {
        colour = Gdk::RGBA(_default);
    }
    return colour;
}

void DirDiff::on_style_updated() {
    Glib::RefPtr<Gtk::StyleContext> style = this->widget->get_style_context();

    this->fill_colors = {{"insert", lookup(style, "insert-bg", "DarkSeaGreen1")},
                         {"delete", lookup(style, "delete-bg", "White")},
                         {"replace", lookup(style, "replace-bg", "#ddeeff")},
                         {"error", lookup(style, "error-bg", "#fce94f")}};
    this->line_colors = {{"insert", lookup(style, "insert-outline", "#77f077")},
                         {"delete", lookup(style, "delete-outline", "Grey")},
                         {"replace", lookup(style, "replace-outline", "#8bbff3")},
                         {"error", lookup(style, "error-outline", "#edd400")}};

    for (DiffMap* diffmap : this->diffmap) {
        diffmap->set_color_scheme(std::pair<std::map<Glib::ustring, Gdk::RGBA>, std::map<Glib::ustring, Gdk::RGBA>>(this->fill_colors, this->line_colors));
    }
    this->queue_draw();
}

void DirDiff::queue_draw() {
    for (Gtk::TreeView* treeview : this->treeview) {
        treeview->queue_draw();
    }
    for (DiffMap* diffmap : this->diffmap) {
        diffmap->queue_draw();
    }
}

void DirDiff::update_comparator() {
#if 0
    comparison_args = {
        "shallow-comparison": this->props.shallow_comparison,
        "time-resolution": this->props.time_resolution,
        "ignore_blank_lines": this->props.ignore_blank_lines,
    };
    this->file_compare = functools.partial(
        _files_same, comparison_args=comparison_args);
#endif
    this->refresh();
}

/*! Update the visibility and order of columns */
void DirDiff::update_treeview_columns(int settings, Glib::ustring key) {
    std::vector<std::pair<Glib::ustring, bool>> columns;
#if 0
    columns = settings.get_value(key);
#endif
    for (size_t i = 0; i < this->treeview.size(); i++) {
        Gtk::TreeView* treeview = this->treeview[i];
        bool extra_cols = false;
        Gtk::TreeViewColumn* last_column = treeview->get_column(0);
        for (std::pair<Glib::ustring, bool> tmp : columns) {
            Glib::ustring column_name = tmp.first;
            bool visible = tmp.second;
            extra_cols = extra_cols or visible;
            Gtk::TreeViewColumn* current_column = this->columns_dict[i][column_name];
            current_column->set_visible(visible);
            treeview->move_column_after(*current_column, *last_column);
            last_column = current_column;
        }
        treeview->set_headers_visible(extra_cols);
    }
}

void DirDiff::on_custom_filter_menu_toggled(Gtk::ToggleAction* item) {
    if (item->get_active()) {
#if 0
        this->custom_popup->signal_deactivate().connect(
                                  lambda popup: item.set_active(False));
        this->custom_popup->popup(None, None,
                                misc.position_menu_under_widget,
                                this->filter_menu_button, 1,
                                Gtk.get_current_event_time());
#endif
    }
}

void DirDiff::_cleanup_filter_menu_button(Glib::RefPtr<Gtk::UIManager> ui) {
    if (this->popup_deactivate_id) {
        this->popup_deactivate_id.disconnect();
    }
    if (this->custom_merge_id) {
        ui->remove_ui(this->custom_merge_id);
    }
    if (std::find(ui->get_action_groups().begin(), ui->get_action_groups().end(), this->filter_actiongroup) != ui->get_action_groups().end()) {
        ui->remove_action_group(this->filter_actiongroup);
    }
}

void DirDiff::_create_filter_menu_button(Glib::RefPtr<Gtk::UIManager> ui) {
    ui->insert_action_group(this->filter_actiongroup, -1);
    this->custom_merge_id = ui->new_merge_id();
    for (std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Gtk::UIManagerItemType, bool> x : this->filter_ui) {
        ui->add_ui(this->custom_merge_id, std::get<0>(x), std::get<1>(x), std::get<2>(x), std::get<3>(x), std::get<4>(x));
    }
    this->popup_deactivate_id = this->popup_menu->signal_deactivate().connect(sigc::mem_fun(this, &DirDiff::on_popup_deactivate_event));
    this->custom_popup = static_cast<Gtk::MenuShell*>(ui->get_widget("/CustomPopup"));
    this->filter_menu_button = dynamic_cast<Gtk::ToggleAction*>(ui->get_widget("/Toolbar/FilterActions/CustomFilterMenu"));
    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    this->filter_menu_button->get_property_value("label", value);
    std::unique_ptr<Gtk::HBox> label = make_tool_button_widget(value.get());
#if 0
    this->filter_menu_button->set_label_widget(label);
#endif
}

void DirDiff::on_container_switch_in_event(Glib::RefPtr<Gtk::UIManager> ui) {
    MeldDoc::on_container_switch_in_event(ui);
#if 0
    this->_create_filter_menu_button(ui);
    this->ui_manager = ui;
#endif
}

void DirDiff::on_container_switch_out_event(Glib::RefPtr<Gtk::UIManager> ui) {
    this->_cleanup_filter_menu_button(ui);
    MeldDoc::on_container_switch_out_event(ui);
}

void DirDiff::on_file_filters_changed() {
    this->_cleanup_filter_menu_button(this->ui_manager);
    bool relevant_change = this->create_name_filters();
    this->_create_filter_menu_button(this->ui_manager);
    if (relevant_change) {
        this->refresh();
    }
}

bool DirDiff::create_name_filters() {
    // Ordering of name filters is irrelevant
    std::set<std::string> old_active;
    for (FilterEntry* f : this->name_filters) {
        if (f->active) {
            old_active.insert(f->filter_string);
        }
    }
    std::set<std::string> new_active;
    for (FilterEntry* f : meldsettings->file_filters) {
        if (f->active) {
            new_active.insert(f->filter_string);
        }
    }
    bool active_filters_changed = old_active != new_active;

    this->name_filters.clear();
    for (FilterEntry* f : meldsettings->file_filters) {
        this->name_filters.push_back(new FilterEntry(*f));
    }
    std::vector<Glib::RefPtr<Gtk::ToggleAction>> actions;
    std::vector<Glib::ustring> disabled_actions;
    this->filter_ui.clear();
    for (size_t i = 0; i < this->name_filters.size(); i++) {
        FilterEntry* f = this->name_filters[i];
        Glib::ustring name = "Hide" + i;
#if 0
        auto callback = [this,i] (int b) { this->_update_name_filter(b, i); };
#endif
        char buff[1000];
        sprintf(_("Hide %s"), f->label.c_str());
        Glib::ustring tooltip = buff;
        Glib::RefPtr<Gtk::ToggleAction> toggle_action = Gtk::ToggleAction::create(name, f->label, tooltip, f->active);
#if 0
        toggle_action->signal_toggled().connect(sigc::ptr_fun(callback));
#endif
        actions.push_back(toggle_action);
        this->filter_ui.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Gtk::UIManagerItemType, bool>("/CustomPopup", name, name, Gtk::UI_MANAGER_MENUITEM, false));
        this->filter_ui.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Gtk::UIManagerItemType, bool>("/Menubar/ViewMenu/FileFilters", name, name, Gtk::UI_MANAGER_MENUITEM, false));
        if (!f->filter) {
            disabled_actions.push_back(name);
        }
    }

    this->filter_actiongroup = Gtk::ActionGroup::create("DirdiffFilterActions");
    for (Glib::RefPtr<Gtk::ToggleAction> action : actions) {
        this->filter_actiongroup->add(action);
    }
    for (Glib::ustring name : disabled_actions) {
        this->filter_actiongroup->get_action(name)->set_sensitive(false);
    }

    return active_filters_changed;
}

void DirDiff::on_text_filters_changed() {
    bool relevant_change = this->create_text_filters();
    if (relevant_change) {
        this->refresh();
    }
}

bool DirDiff::create_text_filters() {
    // In contrast to file filters, ordering of text filters can matter
    std::vector<std::string> old_active;
    for (FilterEntry* f : this->text_filters) {
        if (f->active) {
            old_active.push_back(f->filter_string);
        }
    }
    std::vector<std::string> new_active;
    for (FilterEntry* f : meldsettings->text_filters) {
        if (f->active) {
            new_active.push_back(f->filter_string);
        }
    }

    bool active_filters_changed;
    if (old_active.size() < new_active.size()) {
        active_filters_changed = !std::equal(old_active.begin(), old_active.end(), new_active.begin());
    } else {
        active_filters_changed = !std::equal(new_active.begin(), new_active.end(), old_active.begin());
    }

    this->text_filters.clear();
    for (FilterEntry *f : meldsettings->text_filters) {
        this->text_filters.push_back(new FilterEntry(*f));
    }

    return active_filters_changed;
}

#if 0
template <class T>
void DirDiff::_do_to_others(T master, std::vector<T> objects, int methodname, int args) {
    if (this->do_to_others_lock) {
        return;
    }

    this->do_to_others_lock = true;
    try {
        std::vector<T> tmp(objects.begin(), objects.begin() + this->num_panes);
        for (T o : tmp) {
            if (o != master) {
                others.push_back(o);
            }
        }
        for (T o : others) {
#if 0
            method = getattr(o, methodname);
            method(*args);
#endif
        }
    }
    this->do_to_others_lock = false;
}
#endif

void DirDiff::_sync_vscroll(/*Glib::RefPtr<Gtk::Adjustment> adjustment*/) {
    for (Gtk::ScrolledWindow* sw : this->scrolledwindow) {
        Glib::RefPtr<Gtk::Adjustment> adj = sw->get_vadjustment();
#if 0
        if (adj != adjustment) {
            adj->set_value(adjustment->get_value());
        }
#endif
    }
}

void DirDiff::_sync_hscroll(/*Glib::RefPtr<Gtk::Adjustment> adjustment*/) {
    for (Gtk::ScrolledWindow* sw : this->scrolledwindow) {
        Glib::RefPtr<Gtk::Adjustment> adj = sw->get_hadjustment();
#if 0
        if (adj != adjustment) {
            adj->set_value(adjustment->get_value());
        }
#endif
    }
}

int DirDiff::_get_focused_pane() {
    for (size_t i = 0; i < this->treeview.size(); i++) {
        if (this->treeview[i]->is_focus()) {
            return i;
        }
    }
    return -1;
}

void DirDiff::file_deleted(const char* path, int pane) {
    // is file still extant in other pane?
    Gtk::TreeModel::iterator it = this->model->get_iter(path);
    std::vector<Glib::ustring> files = this->model->value_paths(it);
    bool found = false;
    for (Glib::ustring f : files) {
        if (boost::filesystem::exists(f.c_str())) {
            this->_update_item_state(it);
            found = true;
            break;
        }
    }
    if (!found) {
        // nope its gone
#if 0
        this->model->remove(it);
#endif
    }
    this->_update_diffmaps();
}

void DirDiff::file_created(const char* path, int pane) {
    Gtk::TreeModel::iterator it = this->model->get_iter(path);
    Gtk::TreePath root;
    while (it and this->model->get_path(it) != root) {
        this->_update_item_state(it);
#if 0
        it = this->model->iter_parent(it);
#endif
    }
    this->_update_diffmaps();
}

void DirDiff::on_fileentry_file_set(Gtk::FileChooserButton* entry) {
    std::vector<Glib::RefPtr<const Gio::File>> files;
    for (int i = 0; i < this->num_panes; i++) {
        Gtk::FileChooserButton* e = this->fileentry[i];
        files.push_back(e->get_file());
    }
    std::vector<std::string> paths;
    for (Glib::RefPtr<const Gio::File> f : files) {
        paths.push_back(f->get_path());
    }
    this->set_locations(paths);
}

void DirDiff::set_locations(std::vector<std::string> locations) {
    this->set_num_panes(locations.size());
    // This is difficult to trigger, and to test. Most of the time here we
    // will actually have had UTF-8 from GTK, which has been unicode-ed by
    // the time we get this far. This is a fallback, and may be wrong!
#if 0
    locations = list(locations);
    for (int i = 0; i < locations.size(); i++) {
        std::string l = locations[i];
        if (not isinstance(l, unicode)) {
            locations[i] = l.decode(sys.getfilesystemencoding());
        }
    }
#endif
    // TODO: Support for blank folder comparisons should probably look here
    for (size_t i = 0; i < locations.size(); i++) {
        std::string l = locations[i];
        if (l.empty()) {
            l = ".";
        }
        locations[i] = boost::filesystem::path(l).string();
    }
    this->current_path.clear();
    this->model->clear();
    for (size_t pane = 0; pane < locations.size(); pane++) {
        std::string loc = locations[pane];
        this->fileentry[pane]->set_filename(loc);
    }
    Gtk::TreeStore::iterator child = this->model->add_entries(nullptr, locations);
    this->treeview0->grab_focus();
    this->_update_item_state(child);
    this->recompute_label();
    this->scheduler.remove_all_tasks();
    Gtk::TreePath empty;
    this->recursively_update(empty);
    this->_update_diffmaps();
}

std::pair<std::string, std::vector<std::string>> DirDiff::get_comparison() {
    Gtk::TreeModel::iterator root = this->model->get_iter("");
    std::vector<std::string> folders;
    if (root) {
        for (Glib::ustring folder : this->model->value_paths(root)) {
            folders.push_back(folder);
        }
    }
    return std::pair<std::string, std::vector<std::string>>(TYPE_FOLDER, folders);
}

/*! Recursively update from tree path 'path'. */
void DirDiff::recursively_update(Gtk::TreePath path) {
    Gtk::TreeModel::iterator it = this->model->get_iter(path);
    Gtk::TreeModel::iterator child = it;
    while (child) {
        this->model->erase(child);
        child++;
    }
    this->_update_item_state(it);
    this->scheduler.add_task([this, path] () { this->_search_recursively_iter(path); });
}

void DirDiff::_search_recursively_iter(Gtk::TreePath rootpath) {
    for (Gtk::TreeView* t : this->treeview) {
        Glib::RefPtr<Gtk::TreeSelection> sel = t->get_selection();
        sel->unselect_all();
    }

#if 0
    yield _("[%s] Scanning %s") % (this->label_text, "");
#endif
    int prefixlen = 1 + this->model->value_path(this->model->get_iter(rootpath), 0).size();
    std::set<int> symlinks_followed;
    // TODO: This is horrible.
#if 0
    if (isinstance(rootpath, tuple)) {
        rootpath = Gtk.TreePath(rootpath);
    }
#endif
    std::deque<Gtk::TreePath> todo = {rootpath};
    std::set<int> expanded;

    std::vector<int> shadowed_entries;
    std::vector<int> invalid_filenames;
    while (todo.size()) {
        std::sort(todo.begin(), todo.end()); // depth first
        Gtk::TreePath path = todo.front();
        todo.pop_front();
        Gtk::TreeModel::iterator it = this->model->get_iter(path);
        std::vector<Glib::ustring> roots = this->model->value_paths(it);

        // Buggy ordering when deleting rows means that we sometimes try to
        // recursively update files; this fix seems the least invasive.
        bool non_directory_found = false;
        for (Glib::ustring root : roots) {
            if (!boost::filesystem::is_directory(root.c_str())) {
                non_directory_found = true;
                break;
            }
        }
        if (non_directory_found) {
            continue;
        }

#if 0
        yield _("[%s] Scanning %s") % (this->label_text, roots[0][prefixlen:]);
#endif
        bool differences = false;
        std::vector<int> encoding_errors;

        std::function<std::string(std::string)> canonicalize = [] (std::string in) { return in; };
#if 0
        if (static_cast<Glib::RefPtr<Gtk::ToggleAction>>(this->actiongroup->get_action("IgnoreCase"))->get_active()) {
            canonicalize = [] (std::string in) { std::locale loc; std::string tmp; std::tolower(tmp, loc); return tmp; };
        }
#endif
        CanonicalListing dirs(this->num_panes, canonicalize);
        CanonicalListing files(this->num_panes, canonicalize);

        for (size_t pane = 0; pane < roots.size(); pane++) {
            Glib::ustring root = roots[pane];
            if (not boost::filesystem::is_directory(root.c_str())) {
                continue;
            }

            std::vector<std::string> entries;
            try {
                boost::filesystem::directory_iterator end;
                for (boost::filesystem::directory_iterator dir_iter(root.c_str()); dir_iter != end; dir_iter++) {
                    if (boost::filesystem::is_regular(dir_iter->status())) {
                        std::string filename = (*dir_iter).path().string();
                        entries.push_back(filename);
                    }
                }
            } catch (boost::filesystem::filesystem_error &err) {
                Glib::ustring tmp = err.what();
#if 0
                this->model->add_error(it, tmp, pane);
#endif
                differences = true;
                continue;
            }

            for (FilterEntry* f : this->name_filters) {
                if (not f->active or !f->filter) {
                    continue;
                }
                std::vector<std::string> tmp;
                for (std::string e : entries) {
#if 0
                    if (!f->filter->match(e)) {
                        tmp.push_back(e);
                    }
#endif
                }
                entries = tmp;
            }

            for (std::string e : entries) {
#if 0
                try {
                    if (not isinstance(e, unicode)) {
                        e = e.decode("utf8");
                    }
                } catch (UnicodeDecodeError &e) {
                    approximate_name = e.decode("utf8", "replace");
                    encoding_errors.append((pane, approximate_name));
                    continue;
                }

                try {
                    s = os.lstat(os.path.join(root, e));
                // Covers certain unreadable symlink cases; see bgo#585895
                } catch (OSError &err) {
                    error_string = e + err.strerror;
                    this->model.add_error(it, error_string, pane);
                    continue;
                }

                if (stat.S_ISLNK(s.st_mode)) {
                    if (this->props.ignore_symlinks) {
                        continue;
                    }
                    key = (s.st_dev, s.st_ino);
                    if (std::find(symlinks_followed.begin(), symlinks_followed.end(), key) != symlinks_followed.end()) {
                        continue;
                    }
                    symlinks_followed.add(key);
                    try {
                        s = os.stat(os.path.join(root, e));
                        if (stat.S_ISREG(s.st_mode)) {
                            files.add(pane, e);
                        } else if (stat.S_ISDIR(s.st_mode)) {
                            dirs.add(pane, e);
                        }
                    } catch (OSError &err) {
                        if (err.errno == errno.ENOENT) {
                            error_string = e + ": Dangling symlink";
                        } else {
                            error_string = e + err.strerror;
                        }
                        this->model.add_error(it, error_string, pane);
                        differences = True;
                    }
                } else if (stat.S_ISREG(s.st_mode)) {
                    files.add(pane, e);
                } else if (stat.S_ISDIR(s.st_mode)) {
                    dirs.add(pane, e);
                } else {
                    // FIXME: Unhandled stat type
                }
#endif
            }
        }

#if 0
        for (pane, f in encoding_errors) {
            invalid_filenames.append((pane, roots[pane], f));
        }

        for (pane, f1, f2 in dirs.errors + files.errors) {
            shadowed_entries.append((pane, roots[pane], f1, f2));
        }

        alldirs = this->_filter_on_state(roots, dirs.get());
        allfiles = this->_filter_on_state(roots, files.get());

        if (alldirs or allfiles) {
            for (Glib::ustring names : alldirs) {
                entries = [os.path.join(r, n) for r, n in zip(roots, names)];
                child = this->model.add_entries(it, entries);
                differences |= this->_update_item_state(child);
                todo.append(this->model.get_path(child));
            }
            for (Glib::ustring names : allfiles) {
                entries = [os.path.join(r, n) for r, n in zip(roots, names)];
                child = this->model.add_entries(it, entries);
                differences |= this->_update_item_state(child);
            }
        } else {
            // Our subtree is empty, or has been filtered to be empty
            if (tree.STATE_NORMAL in this->state_filters or
                    not all(os.path.isdir(f) for f in roots)) {
                this->model.add_empty(it);
                if (this->model.iter_parent(it) is None) {
                    expanded.add(rootpath);
                }
            } else {
                // At this point, we have an empty folder tree node; we can
                // prune this and any ancestors that then end up empty.
                while (not this->model.iter_has_child(it)) {
                    parent = this->model.iter_parent(it);

                    // In our tree, there is always a top-level parent with
                    // no siblings. If we're here, we have an empty tree.
                    if (parent is None) {
                        this->model.add_empty(it);
                        expanded.add(rootpath);
                        break;
                    }

                    // Remove the current row, and then revalidate all
                    // sibling paths on the stack by removing and
                    // readding them.
                    had_siblings = this->model.remove(it);
                    if (had_siblings) {
                        parent_path = this->model.get_path(parent);
                        for (int path : todo) {
                            if (parent_path.is_ancestor(path)) {
                                path.prev();
                            }
                        }
                    }

                    it = parent;
                }
            }
        }

        if (differences) {
            expanded.add(path);
        }
#endif
    }

#if 0
    this->_show_tree_wide_errors(invalid_filenames, shadowed_entries);

    for (int path : sorted(expanded)) {
        this->treeview[0].expand_to_path(path);
    }
    yield _("[%s] Done") % this->label_text;
#endif

    this->scheduler.add_task([this] () { this->on_treeview_cursor_changed(); });
    this->treeview[0]->get_selection()->select(Gtk::TreePath());
    this->_update_diffmaps();
}

void DirDiff::_show_tree_wide_errors(std::vector<std::tuple<int, std::string, std::string>> invalid_filenames, std::vector<std::tuple<int, std::string, std::string, std::string>> shadowed_entries) {
    static Glib::ustring header = _("Multiple errors occurred while scanning this folder");
    static Glib::ustring invalid_header = _("Files with invalid encodings found");
    // TRANSLATORS: This is followed by a list of files
    static Glib::ustring invalid_secondary = _("Some files were in an incorrect encoding. "
                          "The names are something like:");
    static Glib::ustring shadowed_header = _("Files hidden by case insensitive comparison");
    // TRANSLATORS: This is followed by a list of files
    static Glib::ustring shadowed_secondary = _("You are running a case insensitive comparison "
                           "on a case sensitive filesystem. The following "
                           "files in this folder are hidden:");

    std::vector<std::vector<std::string>> invalid_entries;
    for (int i = 0; i < this->num_panes; i++) {
        std::vector<std::string> tmp;
        invalid_entries.push_back(tmp);
    }
    for (std::tuple<int, std::string, std::string> tmp : invalid_filenames) {
        int pane = std::get<0>(tmp);
        std::string root = std::get<1>(tmp);
        std::string f = std::get<2>(tmp);
        invalid_entries[pane].push_back(root + "/" + f);
    }

    std::vector<std::vector<std::string>> formatted_entries;
    for (int i = 0; i < this->num_panes; i++) {
        std::vector<std::string> tmp;
        formatted_entries.push_back(tmp);
    }
    for (std::tuple<int, std::string, std::string, std::string> tmp : shadowed_entries) {
        int pane = std::get<0>(tmp);
        std::string root = std::get<1>(tmp);
        std::string f1 = std::get<2>(tmp);
        std::string f2 = std::get<3>(tmp);
        std::vector<std::string> paths = { root + "/" + f1, root + "/" + f2};
        char buff[1000];
        sprintf(buff, _("'%s' hidden by '%s'"), paths[0].c_str(), paths[1].c_str());
        std::string entry_str = buff;
        formatted_entries[pane].push_back(entry_str);
    }

    if (!invalid_filenames.empty() or !shadowed_entries.empty()) {
        for (int pane = 0; pane < this->num_panes; pane++) {
            std::string invalid = boost::join(invalid_entries[pane], "\n");
            std::string shadowed = boost::join(formatted_entries[pane], "\n");
            std::string header = "";
            std::vector<std::string> messages;
            if (!invalid.empty() and !shadowed.empty()) {
                messages = {invalid_secondary, invalid, "",
                            shadowed_secondary, shadowed};
            } else if (!invalid.empty()) {
                header = invalid_header;
                messages = {invalid_secondary, invalid};
            } else if (!shadowed.empty()) {
                header = shadowed_header;
                messages = {shadowed_secondary, shadowed};
            } else {
                continue;
            }
            std::string secondary = boost::join(messages, "\n");
            this->add_dismissable_msg(pane, Gtk::Stock::DIALOG_ERROR, header,
                                     secondary);
        }
    }
}

Gtk::InfoBar* DirDiff::add_dismissable_msg(int pane, const Gtk::BuiltinStockID icon, std::string primary, std::string secondary) {
    Gtk::InfoBar* msgarea = this->msgarea_mgr[pane]->new_from_text_and_icon(
                    icon, primary, secondary);
    msgarea->add_button(_("Hi_de"), Gtk::RESPONSE_CLOSE);
#if 0
    msgarea->signal_response().connect(lambda *args: this->msgarea_mgr[pane].clear());
#endif
    msgarea->show_all();
    return msgarea;
}

void DirDiff::copy_selected(int direction) {
#if 0
    assert(direction == -1 || direction == 1);
    int src_pane = this->_get_focused_pane();
    if (src_pane >= 0) {
        dst_pane = src_pane + direction;
        assert(dst_pane >= 0 and dst_pane < this->num_panes);
        std::vector<Gtk::TreeModel::Path> paths = this->_get_selected_paths(src_pane);
        paths.reverse();
        model = this->model;
        for (int path : paths) { //filter(lambda x: x.name is not None, sel):
            it = model.get_iter(path);
            name = model.value_path(it, src_pane);
            if (name is None) {
                continue;
            }
            src = model.value_path(it, src_pane);
            dst = model.value_path(it, dst_pane);
            try {
                if (os.path.isfile(src)) {
                    dstdir = os.path.dirname(dst);
                    if (not os.path.exists(dstdir)) {
                        os.makedirs(dstdir);
                    }
                    misc.copy2(src, dstdir);
                    this->file_created( path, dst_pane);
                } else if (os.path.isdir(src)) {
                    if (os.path.exists(dst)) {
                        if (run_dialog( _("'%s' exists.\nOverwrite?") % os.path.basename(dst),
                                parent = this,
                                buttonstype = Gtk.ButtonsType.OK_CANCEL) != Gtk.ResponseType.OK) {
                            continue;
                        }
                    }
                    misc.copytree(src, dst);
                    this->recursively_update(path);
                }
#if 0
            } catch (OSError, IOError, shutil.Error as err) {
#else
            } catch (std::exception &err) {
#endif
                misc.error_dialog(
                    _("Error copying file"),
                    _("Couldn't copy %s\nto %s.\n\n%s") % (
                        GLib.markup_escape_text(src),
                        GLib.markup_escape_text(dst),
                        GLib.markup_escape_text(str(err)),
                    )
                );
            }
        }
    }
#endif
}

/*! Delete all selected files/folders recursively. */
void DirDiff::delete_selected() {
#if 0
    // reverse so paths dont get changed
    int pane = this->_get_focused_pane();
    if (pane >= 0) {
        std::vector<Gtk::TreeModel::Path> paths = this->_get_selected_paths(pane);
        paths.reverse();
        for (int path : paths) {
            it = this->model.get_iter(path);
            name = this->model.value_path(it, pane);
            try {
                gfile = Gio.File.new_for_path(name);
                gfile.trash(None);
                this->file_deleted(path, pane);
            } catch (Glib::GError &e) {
                misc.error_dialog(_("Error deleting %s") % name, str(e));
            }
        }
    }
#endif
}

// TODO: Move this and path tools to new tree helper module
void DirDiff::refocus_deleted_path(int model, int path) {
    // Since the passed path has been deleted, either the path is now a
    // valid successor, or there are no successors. If valid, return it.
    // If not, and the path has a predecessor sibling (immediate or
    // otherwise), then return that. If there are no siblings, traverse
    // parents until we get a valid path, and return that.
#if 0
    def tree_path_prev(path):
        if not path or path[-1] == 0:
            return None
        return path[:-1] + (path[-1] - 1,)

    def tree_path_up(path):
        if not path:
            return None
        return path[:-1]

    def valid_path(model, path):
        try:
            model.get_iter(path)
            return True
        except ValueError:
            return False

    if valid_path(model, path):
        return path

    new_path = tree_path_prev(path)
    while new_path:
        if valid_path(model, new_path):
            return new_path
        new_path = tree_path_prev(new_path)

    new_path = tree_path_up(path)
    while new_path:
        if valid_path(model, new_path):
            return new_path
        new_path = tree_path_up(new_path)

    return None;
#endif
}

void DirDiff::on_treemodel_row_deleted(int model, int path) {
#if 0
    if (this->current_path == path) {
        this->current_path = refocus_deleted_path(model, path);
        if (this->current_path and this->focus_pane) {
            this->focus_pane.set_cursor(this->current_path);
        }
    }
#endif
}

void DirDiff::on_treeview_selection_changed(Glib::RefPtr<Gtk::TreeView::Selection> selection, int pane) {
    if (not this->treeview[pane]->is_focus()) {
        return;
    }
    bool have_selection = selection->count_selected_rows() > 0;

    if (have_selection) {
        bool is_valid = true;
        for (Gtk::TreeModel::Path path : selection->get_selected_rows()) {
            FileState state = static_cast<FileState>(this->model->get_state(this->model->get_iter(path), pane));
            if (state == STATE_ERROR || state == STATE_NONEXIST) {
                is_valid = false;
                break;
            }
        }

        this->actiongroup->get_action("DirCompare")->set_sensitive(true);
        this->actiongroup->get_action("Hide")->set_sensitive(true);
        this->actiongroup->get_action("DirDelete")->set_sensitive(is_valid);
        this->actiongroup->get_action("DirCopyLeft")->set_sensitive(is_valid and pane > 0);
        this->actiongroup->get_action("DirCopyRight")->set_sensitive(
            is_valid and pane + 1 < this->num_panes);
        if (this->main_actiongroup) {
            this->main_actiongroup->get_action("OpenExternal")->set_sensitive(is_valid);
        }
    } else {
        this->actiongroup->get_action("DirCompare")->set_sensitive(false);
        this->actiongroup->get_action("Hide")->set_sensitive(false);
        this->actiongroup->get_action("DirDelete")->set_sensitive(false);
        this->actiongroup->get_action("DirCopyLeft")->set_sensitive(false);
        this->actiongroup->get_action("DirCopyRight")->set_sensitive(false);
        if (this->main_actiongroup) {
            this->main_actiongroup->get_action("OpenExternal")->set_sensitive(false);
        }
    }
}

void DirDiff::on_treeview_cursor_changed() {
#if 0
    int pane = this->_get_focused_pane();
    if (pane < or len(this->model) == 0) {
        return;
    }
    Gtk::TreeModel::Path cursor_path;
    Gtk::TreeViewColumn* cursor_col;
    this->treeview[pane]->get_cursor(cursor_path, cursor_col);
    if (not cursor_path) {
        this->m_signal_next_diff_changed.emit(false, false);
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
#endif
}

bool DirDiff::on_treeview_key_press_event(Gtk::TreeView* view, GdkEventKey* event) {
    int pane = -1;
    for (size_t i = 0; i < this->treeview.size(); i++) {
        if (this->treeview[i] == view) {
            pane = i;
            break;
        }
    }
    Gtk::TreeView* tree = nullptr;
    if (GDK_KEY_Right == event->keyval) {
        if (pane + 1 < this->num_panes) {
            tree = this->treeview[pane + 1];
        }
    } else if (GDK_KEY_Left == event->keyval) {
        if (pane-1 >= 0) {
            tree = this->treeview[pane-1];
        }
    }
    if (tree) {
        std::vector<Gtk::TreeModel::Path> paths = this->_get_selected_paths(pane);
        view->get_selection()->unselect_all();
        tree->grab_focus();
        tree->get_selection()->unselect_all();
        if (!paths.empty()) {
            tree->set_cursor(paths[0]);
            for (Gtk::TreePath p : paths) {
                tree->get_selection()->select(p);
            }
        }
        tree->signal_cursor_changed();
    }
    return event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_Right; //handled
}

void DirDiff::on_treeview_row_activated(Gtk::TreeView* view, Gtk::TreeModel::Path path, int column) {
    int pane = -1;
    for (size_t i = 0; i < this->treeview.size(); i++) {
        if (this->treeview[i] == view) {
            pane = i;
            break;
        }
    }
    std::vector<Glib::ustring> rows = this->model->value_paths(this->model->get_iter(path));
    // Click a file: compare; click a directory: expand; click a missing
    // entry: check the next neighbouring entry
    static int pane_ordering[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 1, 0}};
    for (int p : pane_ordering[pane]) {
        if (p < this->num_panes and !rows[p].empty() and boost::filesystem::exists(rows[p].c_str())) {
            pane = p;
            break;
        }
    }
    if (rows[pane].empty()) {
        return;
    }
    if (boost::filesystem::is_regular(rows[pane].c_str())) {
        std::vector<std::string> tmp;
        for (Glib::ustring r : rows) {
            if (boost::filesystem::is_regular(r.c_str())) {
                tmp.push_back(r);
            }
        }
        this->m_signal_create_diff.emit(tmp, false, false, "", {});
    } else if (boost::filesystem::is_directory(rows[pane].c_str())) {
        if (view->row_expanded(path)) {
            view->collapse_row(path);
        } else {
            view->expand_row(path, false);
        }
    }
}

void DirDiff::on_treeview_row_expanded(Gtk::TreeView* view, int it, Gtk::TreeModel::Path path) {
    for (Gtk::TreeView* treeview : this->treeview) {
        if (treeview != view) {
            treeview->expand_row(path, 0);
        }
    }
    this->_update_diffmaps();
}

void DirDiff::on_treeview_row_collapsed(Gtk::TreeView* view, int me, Gtk::TreeModel::Path path) {
    for (Gtk::TreeView* treeview : this->treeview) {
        if (treeview != view) {
            treeview->collapse_row(path);
        }
    }
    this->_update_diffmaps();
}

void DirDiff::on_popup_deactivate_event() {
    assert(this->treeview.size() == this->focus_in_events.size());
    assert(this->treeview.size() == this->focus_out_events.size());
    for (size_t i = 0; i < this->treeview.size(); i++) {
        Gtk::TreeView* treeview = this->treeview[i];
        int inid = this->focus_in_events[i];
        int outid = this->focus_out_events[i];
#if 0
        treeview->handler_unblock(inid);
        treeview->handler_unblock(outid);
#endif
    }
}

bool DirDiff::on_treeview_focus_in_event(GdkEventFocus* event, Gtk::TreeView* tree) {
    this->focus_pane = tree;
    int pane = -1;
    for (int i = 0; i < this->treeview.size(); i++) {
        if (this->treeview[i] == tree) {
            pane = i;
            break;
        }
    }
    this->on_treeview_selection_changed(tree->get_selection(), pane);
#if 0
    tree->signal_cursor_changed().emit();
#endif
}

bool DirDiff::on_treeview_focus_out_event(GdkEventFocus* event) {
    this->actiongroup->get_action("DirCompare")->set_sensitive(false);
    this->actiongroup->get_action("Hide")->set_sensitive(false);
    this->actiongroup->get_action("DirDelete")->set_sensitive(false);
    this->actiongroup->get_action("DirCopyLeft")->set_sensitive(false);
    this->actiongroup->get_action("DirCopyRight")->set_sensitive(false);
    if (this->main_actiongroup) {
        this->main_actiongroup->get_action("OpenExternal")->set_sensitive(false);
    }
}

void DirDiff::on_button_diff_clicked() {
    int pane = this->_get_focused_pane();
    if (pane < 0) {
        return;
    }

    std::vector<Gtk::TreeModel::Path> selected = this->_get_selected_paths(pane);
    for (Gtk::TreeModel::Path row : selected) {
        std::vector<Glib::ustring> row_paths = this->model->value_paths(this->model->get_iter(row));
        std::vector<std::string> paths;
        for (Glib::ustring p : row_paths) {
            if (boost::filesystem::exists(p.c_str())) {
                paths.push_back(p);
            }
        }
        this->m_signal_create_diff.emit(paths, false, false, "", {});
    }
}

void DirDiff::on_button_copy_left_clicked(int button) {
    this->copy_selected(-1);
}

void DirDiff::on_button_copy_right_clicked(int button) {
    this->copy_selected(1);
}

void DirDiff::on_button_delete_clicked(int button) {
    this->delete_selected();
}

void DirDiff::open_external() {
    int pane = this->_get_focused_pane();
    if (pane < 0) {
        return;
    }
    std::list<std::string> files;
    for (Gtk::TreeModel::Path p : this->_get_selected_paths(pane)) {
        Glib::ustring f = this->model->value_path(this->model->get_iter(p), pane);
        if (!f.empty()) {
            files.push_back(f.c_str());
        }
    }
    if (files.empty()) {
        this->_open_files(files);
    }
}

void DirDiff::on_button_ignore_case_toggled(int button) {
    this->refresh();
}

void DirDiff::on_filter_state_toggled(int button) {
    std::vector<std::pair<FileState, std::pair<Glib::ustring, Glib::ustring>>> active_filters;
#if 0
    for (std::pair<FileState, std::pair<Glib::ustring, Glib::ustring>> a : this->state_actions) {
        if (static_cast<Glib::RefPtr<Gtk::ToggleAction>>(this->actiongroup->get_action(a.second.first))->get_active()) {
            active_filters.push_back(a);
        }
    }
#endif

#if 0
    if (set(active_filters) == set(this->state_filters)) {
        return;
    }
#endif

    std::vector<Glib::ustring> state_strs;
    for (std::pair<FileState, std::pair<Glib::ustring, Glib::ustring>> s : active_filters) {
#if 0
        state_strs.push_back(this->state_actions[s.first].first);
#endif
    }
    this->state_filters = active_filters;
    // TODO: Updating the property won't have any effect on its own
#if 0
    this->props.status_filters = state_strs;
#endif
    this->refresh();
}

void DirDiff::_update_name_filter(Gtk::ToggleButton* button, int idx) {
    this->name_filters[idx]->active = button->get_active();
    this->refresh();
}

void DirDiff::on_filter_hide_current_clicked(int button) {
    int pane = this->_get_focused_pane();
    if (pane >= 0) {
        std::vector<Gtk::TreeModel::Path> paths = this->_get_selected_paths(pane);
#if 0
        paths.reverse();
#endif
        for (Gtk::TreeModel::Path p : paths) {
#if 0
            this->model->remove(this->model->get_iter(p));
#endif
        }
    }
}

//
// Selection
//
std::vector<Gtk::TreeModel::Path> DirDiff::_get_selected_paths(int pane) {
    assert(pane >= 0);
#if 0
    return this->treeview[pane]->get_selection()->get_selected_rows()[1];
#else
    std::vector<Gtk::TreeModel::Path> dummy;
    return dummy;
#endif
}

//
// Filtering
//
/*!
 * Get state of 'files' for filtering purposes.
 * Returns STATE_NORMAL, STATE_NEW or STATE_MODIFIED
 *
 * roots - array of root directories
 * fileslist - array of filename tuples of length len(roots)
 */
std::vector<int> DirDiff::_filter_on_state(std::vector<Glib::ustring> roots, const CanonicalListing& fileslist) {
    assert(roots.size() == this->model->ntree);
    std::vector<int> ret;
    std::vector<std::shared_ptr<std::regex>> regexes;
    for (FilterEntry* f : this->text_filters) {
        if (f->active) {
            regexes.push_back(f->filter);
        }
    }
#if 0
    for (int files : fileslist) {
        FileState state;
        curfiles = [ os.path.join( r, f ) for r,f in zip(roots,files) ];
        is_present = [ os.path.exists( f ) for f in curfiles ];
        bool all_present = 0 not in is_present;
        if (all_present) {
            if (this->file_compare(curfiles, regexes) in (
                        Same, SameFiltered, DodgySame)) {
                state = STATE_NORMAL;
            } else {
                state = STATE_MODIFIED;
            }
        } else {
            state = STATE_NEW;
        }
        // Always retain NORMAL folders for comparison; we remove these
        // later if they have no children.
        if (state in this->state_filters or
                all(os.path.isdir(f) for f in curfiles)) {
            ret.append(files);
        }
    }
#endif
    return ret;
}

/*! Update the state of the item at 'it' */
void DirDiff::_update_item_state(Gtk::TreeModel::iterator& it) {
#if 0
    std::vector<Glib::ustring> files = this->model->value_paths(it);
    std::vector<std::regex*> regexes;
    for (int f : this->text_filters) {
        if (f.active) {
            regexes.push_back(f.filter);
        }
    }

    def stat(f):
        try:
            return os.stat(f)
        except OSError:
            return None
    stats = [stat(f) for f in files[:this->num_panes]]
    sizes = [s.st_size if s else 0 for s in stats]
    perms = [s.st_mode if s else 0 for s in stats]

    // find the newest file, checking also that they differ
    mod_times = [s.st_mtime if s else 0 for s in stats]
    newest_index = mod_times.index( max(mod_times) )
    if mod_times.count( max(mod_times) ) == len(mod_times):
        newest_index = -1 // all same
    all_present = 0 not in mod_times
    if all_present:
        all_same = this->file_compare(files, regexes)
        all_present_same = all_same
    else:
        lof = []
        for j in range(len(mod_times)):
            if mod_times[j]:
                lof.append( files[j] )
        all_same = Different
        all_present_same = this->file_compare(lof, regexes)
    different = 1
    one_isdir = [None for i in range(this->model.ntree)]
    for j in range(this->model.ntree):
        if mod_times[j]:
            isdir = os.path.isdir( files[j] )
            // TODO: Differentiate the DodgySame case
            if all_same == Same or all_same == DodgySame:
                this->model.set_path_state(it, j, tree.STATE_NORMAL, isdir)
                different = 0
            elif all_same == SameFiltered:
                this->model.set_path_state(it, j, tree.STATE_NOCHANGE, isdir)
                different = 0
            // TODO: Differentiate the SameFiltered and DodgySame cases
            elif all_present_same in (Same, SameFiltered, DodgySame):
                this->model.set_path_state(it, j, tree.STATE_NEW, isdir)
            elif all_same == FileError or all_present_same == FileError:
                this->model.set_path_state(it, j, tree.STATE_ERROR, isdir)
            // Different and DodgyDifferent
            else:
                this->model.set_path_state(it, j, tree.STATE_MODIFIED, isdir)
            this->model.set_value(it,
                this->model.column_index(COL_EMBLEM, j),
                j == newest_index and "emblem-meld-newer-file" or None)
            one_isdir[j] = isdir

            // A DateCellRenderer would be nicer, but potentially very slow
            TIME = this->model.column_index(COL_TIME, j)
            mod_datetime = datetime.datetime.fromtimestamp(mod_times[j])
            time_str = mod_datetime.strftime("%a %d %b %Y %H:%M:%S")
            this->model.set_value(it, TIME, time_str)

            def natural_size(bytes):
                suffixes = (
                        "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
                        )
                size = float(bytes)
                unit = 0
                while size > 1000 and unit < len(suffixes) - 1:
                    size /= 1000
                    unit += 1
                format_str = "%.1f %s" if unit > 0 else "%d %s"
                return format_str % (size, suffixes[unit])

            // A SizeCellRenderer would be nicer, but potentially very slow
            SIZE = this->model.column_index(COL_SIZE, j)
            size_str = natural_size(sizes[j])
            this->model.set_value(it, SIZE, size_str)

            def format_mode(mode):
                perms = []
                rwx = ((4, 'r'), (2, 'w'), (1, 'x'))
                for group_index in (6, 3, 0):
                    group = mode >> group_index & 7
                    perms.extend([p if group & i else '-' for i, p in rwx])
                return "".join(perms)

            PERMS = this->model.column_index(COL_PERMS, j)
            perm_str = format_mode(perms[j])
            this->model.set_value(it, PERMS, perm_str)

    for (int j = 0; j < this->model->ntree; j++) {
        if (not mod_times[j]) {
            this->model.set_path_state(it, j, tree.STATE_NONEXIST,
                                      True in one_isdir);
        }
    }
    return different;
#endif
}

void DirDiff::popup_in_pane(int pane, int event) {
#if 0
    for ((treeview, inid, outid) in zip(this->treeview, this->focus_in_events, this->focus_out_events)) {
        treeview.handler_block(inid);
        treeview.handler_block(outid);
    }
    this->actiongroup.get_action("DirCopyLeft").set_sensitive(pane > 0);
    this->actiongroup.get_action("DirCopyRight").set_sensitive(pane+1 < this->num_panes);
    if (event) {
        button = event.button;
        time = event.time;
    } else {
        button = 0;
        time = Gtk.get_current_event_time();
    }
    this->popup_menu.popup(None, None, None, None, button, time);
#endif
}

bool DirDiff::on_treeview_popup_menu(int treeview) {
#if 0
    this->popup_in_pane(this->treeview.index(treeview), None);
#endif
    return true;
}

bool DirDiff::on_treeview_button_press_event(Gtk::TreeView* treeview, GdkEventButton* event) {
    // Unselect any selected files in other panes
    for (int i = 0; i < this->num_panes; i++) {
        Gtk::TreeView* v = this->treeview[i];
        if (v != treeview) {
            v->get_selection()->unselect_all();
        }
    }

    if (event->button == 3) {
        treeview->grab_focus();
        Gtk::TreeModel::Path path;
        bool tmp = treeview->get_path_at_pos(int(event->x), int(event->y), path);
        if (!tmp) {
            return false;
        }
        Glib::RefPtr<Gtk::TreeSelection> selection = treeview->get_selection();
#if 0
        std::vector<Gtk::TreeModel::Path> rows = selection->get_selected_rows(model);

        if (std::find(rows.begin(), rows.end(), path[0]) == rows.end()) {
            selection->unselect_all();
            selection->select_path(path[0]);
            treeview->set_cursor(path[0]);
        }

        this->popup_in_pane(this->treeview->index(treeview), event);
#endif
        return true;
    }
    return false;
}

void DirDiff::recurse_tree_states(Gtk::TreeView* treeview, std::vector<FileState>& row_states, Gtk::TreeModel::iterator rowiter, int treeindex) {
#if 0
    row_states.push_back(this->model->get_state(rowiter.iter, treeindex));
    if (treeview->row_expanded(rowiter.path)) {
        for (int row : rowiter.iterchildren()) {
            recurse_tree_states(row);
        }
    }
#endif
}

std::vector<std::tuple<Glib::ustring, int, int>> DirDiff::tree_state_iter(int diffmapindex) {
    int treeindex;
    if (diffmapindex == 0) {
        treeindex = 0;
    } else {
        treeindex = this->num_panes-1;
    }
    Gtk::TreeView* treeview = this->treeview[treeindex];
    std::vector<FileState> row_states;
#if 0
    recurse_tree_states(treeview, row_states, next(iter(this->model)), treeindex);
#endif
    row_states.push_back(STATE_MAX);

    float numlines = float(row_states.size() - 1);
    int chunkstart = 0;
    FileState laststate = row_states[0];
    std::vector<std::tuple<Glib::ustring, int, int>> result;
    for (size_t index = 0; index < row_states.size(); index++) {
        FileState state = row_states[index];
        if (state != laststate) {
#if 0
            Glib::ustring action = this->chunk_type_map[laststate];
#else
            Glib::ustring action = "";
#endif
            if (!action.empty()) {
                result.push_back(std::tuple<Glib::ustring, int, int>(action, chunkstart / numlines, index / numlines));
            }
            chunkstart = index;
            laststate = state;
        }
    }
    return result;
}


std::vector<std::tuple<Glib::ustring, int, int>> DirDiff::get_state_traversal(int diffmapindex) {
    return tree_state_iter(diffmapindex);
}

void DirDiff::set_num_panes(int n) {
    if (n != this->num_panes and (n == 1 || n == 2 || n == 3)) {
        this->model = new DirDiffTreeStore(n);
        for (int i = 0; i < n; i++) {
#if 0
            this->treeview[i]->set_model(this->model);
#endif
        }
#if 0
        this->model->signal_row_deleted().connect(sigc::mem_fun(this, &DirDiff::on_treemodel_row_deleted));
#endif

#if 0
        for ((w, i) in zip(this->diffmap, (0, n - 1))) {
            scroll = this->scrolledwindow[i].get_vscrollbar();
            idx = 1 if i else 0;
            w.setup(scroll, this->get_state_traversal(idx), [this->fill_colors, this->line_colors]);
        }
#endif

        for (ScrollLinkMap* w : this->linkmap) {
            w->associate(this);
        }

#if 0
        for (widget in (
                this->vbox[:n] + this->file_toolbar[:n] + this->diffmap[:n] +
                this->linkmap[:n - 1] + this->dummy_toolbar_linkmap[:n - 1])) {
            widget.show();
        }
#endif

#if 0
        for (widget in (
                this->vbox[n:] + this->file_toolbar[n:] + this->diffmap[n:] +
                this->linkmap[n - 1:] + this->dummy_toolbar_linkmap[n - 1:])) {
            widget.hide();
        }
#endif

        if (this->num_panes != 0) { // not first time through
            this->num_panes = n;
            this->on_fileentry_file_set(nullptr);
        } else {
            this->num_panes = n;
        }
    }
}

void DirDiff::refresh() {
    Gtk::TreeModel::iterator root = this->model->get_iter("");
    if (root) {
        std::vector<std::string> roots;
        for (Glib::ustring tmp : this->model->value_paths(root)) {
            roots.push_back(tmp);
        }
        this->set_locations(roots);
    }
}

void DirDiff::recompute_label() {
    Gtk::TreeModel::iterator root = this->model->get_iter("");
    std::vector<Glib::ustring> filenames = this->model->value_paths(root);
    std::vector<std::string> shortnames;
#if 0
    if (this->custom_labels) {
        label_options = zip(this->custom_labels, filenames);
        shortnames = [l[0] or l[1] for l in label_options];
    } else {
        shortnames = shorten_names(filenames);
    }
#endif
    this->label_text = boost::join(shortnames, " : ");
    this->tooltip_text = this->label_text;
    this->label_changed();
}

void DirDiff::set_labels(std::vector<Glib::ustring> labels) {
#if 0
    labels = labels[:this->num_panes];
    extra = this->num_panes - len(labels);
    if (extra) {
        labels.extend([""] * extra);
    }
    this->custom_labels = labels;
#endif
    this->recompute_label();
}

void DirDiff::_update_diffmaps() {
    for (DiffMap* diffmap : this->diffmap) {
        diffmap->on_diffs_changed(std::tuple<std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::set<std::pair<difflib::chunk_t, difflib::chunk_t>>, std::pair<difflib::chunk_t, difflib::chunk_t>>());
        diffmap->queue_draw();
    }
}

/*!
 * When a file has changed, try to find it in our tree
 * and update its status if necessary
 */
void DirDiff::on_file_changed(Glib::ustring changed_filename) {
    DiffTreeStore* model = this->model;
    std::vector<Gtk::TreeModel::iterator> changed_paths;
    // search each panes tree for changed_filename
    for (int pane = 0; pane < this->num_panes; pane++) {
        Gtk::TreeModel::iterator it = model->get_iter("");
#if 0
        current = model->value_path(it, pane).split(os.sep);
        changed = changed_filename.split(os.sep);
        // early exit. does filename begin with root?
        try {
            if (changed[:len(current)] != current) {
                continue;
            }
        } catch (IndexError &e) {
            continue;
        }
        changed = changed[len(current):];
        // search the tree component at a time
        for (int component : changed) {
            child = model.iter_children(it);
            while (child) {
                child_path = model.value_path(child, pane);
                // Found the changed path
                if (child_path and component == os.path.basename(child_path)) {
                    it = child;
                    break;
                }
                child = this->model.iter_next(child);
            }
            if (not it) {
                break;
            }
        }
        // save if found and unique
        if (it) {
            path = model.get_path(it);
            if (path not in changed_paths) {
                changed_paths.append(path);
            }
        }
#endif
    }
    // do the update
    for (Gtk::TreeModel::iterator path : changed_paths) {
#if 0
        this->_update_item_state(model->get_iter(path));
#endif
    }
}

void DirDiff::next_diff(GdkScrollDirection direction, bool centered) {
    int pane;
    if (this->focus_pane) {
#if 0
        pane = this->treeview.index(this->focus_pane);
#endif
    } else {
        pane = 0;
    }
    Gtk::TreeModel::Path path;
    if (direction == GDK_SCROLL_UP) {
        path = this->prev_path;
    } else {
        path = this->next_path;
    }
    if (path) {
        this->treeview[pane]->expand_to_path(path);
        this->treeview[pane]->set_cursor(path);
    }
}

void DirDiff::on_refresh_activate() {
    this->on_fileentry_file_set(nullptr);
}

Gtk::ResponseType DirDiff::on_delete_event(int appquit) {
    for (sigc::connection h : this->settings_handlers) {
        h.disconnect();
    }
    this->m_signal_close.emit(false);
    return Gtk::RESPONSE_OK;
}

void DirDiff::on_find_activate() {
#if 0
    this->focus_pane->signal_start_interactive_search().emit();
#endif
}
