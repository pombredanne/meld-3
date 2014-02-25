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

#include "tree.h"
#include <cassert>
#include "vc/_vc.h"
#include <boost/filesystem.hpp>

std::vector<const std::type_info*> COL_TYPES = {&typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(std::string), &typeid(Pango::Style),
             &typeid(Pango::Weight), &typeid(bool)};

TextAttribute::TextAttribute(Glib::ustring fg, Pango::Style style, Pango::Weight weight, bool strike) : fg(fg), style(style), weight(weight), strike(strike) {
}

DiffTreeStore::DiffTreeStore(int ntree, std::vector<const std::type_info*> types) {
#if 0
    std::list<std::type_info> full_types;
    for (std::type_info col_type : COL_TYPES) {
        full_types.extend([col_type] * ntree);
    }
    for (std::type_info col_type : types) {
        full_types.extend([col_type] * ntree);
    }
    Gtk.TreeStore.__init__(self, *full_types);
#endif
    this->ntree = ntree;
#if 0
    this->_setup_default_styles();
#endif
}

void DiffTreeStore::on_style_updated(Gtk::Widget* widget) {
    Glib::RefPtr<Gtk::StyleContext> style = widget->get_style_context();
    this->_setup_default_styles(style);
}

Glib::ustring DiffTreeStore::lookup(Glib::RefPtr<Gtk::StyleContext> style, std::string name, const Glib::ustring& _default) {
    Glib::ustring colour;
    try {
        Gdk::RGBA color;
        bool found = style->lookup_color(name, color);
        if (found) {
            colour = color.to_string();
        } else {
            colour = _default;
        }
    } catch (std::exception) {
        colour = _default;
    }
    return colour;
}

void DiffTreeStore::_setup_default_styles(Glib::RefPtr<Gtk::StyleContext> style) {
    Pango::Style roman = Pango::STYLE_NORMAL;
    Pango::Style italic = Pango::STYLE_ITALIC;
    Pango::Weight normal = Pango::WEIGHT_NORMAL;
    Pango::Weight bold = Pango::WEIGHT_BOLD;

    Glib::ustring unk_fg = lookup(style, "unknown-text", "#888888");
    Glib::ustring new_fg = lookup(style, "insert-text", "#008800");
    Glib::ustring mod_fg = lookup(style, "replace-text", "#0044dd");
    Glib::ustring del_fg = lookup(style, "delete-text", "#880000");
    Glib::ustring err_fg = lookup(style, "error-text", "#ffff00");
    Glib::ustring con_fg = lookup(style, "conflict-text", "#ff0000");


    // foreground, style, weight, strikethrough
    this->text_attributes.push_back(new TextAttribute(unk_fg, roman,  normal, false));  // STATE_IGNORED
    this->text_attributes.push_back(new TextAttribute(unk_fg, roman,  normal, false));  // STATE_NONE
    this->text_attributes.push_back(new TextAttribute(0,      roman,  normal, false));  // STATE_NORMAL
    this->text_attributes.push_back(new TextAttribute(0,      italic, normal, false));  // STATE_NOCHANGE
    this->text_attributes.push_back(new TextAttribute(err_fg, roman,  bold,   false));  // STATE_ERROR
    this->text_attributes.push_back(new TextAttribute(unk_fg, italic, normal, false));  // STATE_EMPTY
    this->text_attributes.push_back(new TextAttribute(new_fg, roman,  bold,   false));  // STATE_NEW
    this->text_attributes.push_back(new TextAttribute(mod_fg, roman,  bold,   false));  // STATE_MODIFIED
    this->text_attributes.push_back(new TextAttribute(mod_fg, roman,  normal, false));  // STATE_RENAMED
    this->text_attributes.push_back(new TextAttribute(con_fg, roman,  bold,   false));  // STATE_CONFLICT
    this->text_attributes.push_back(new TextAttribute(del_fg, roman,  bold,   true));  // STATE_REMOVED
    this->text_attributes.push_back(new TextAttribute(del_fg, roman,  bold,   true));  // STATE_MISSING
    this->text_attributes.push_back(new TextAttribute(unk_fg, roman,  normal, true));  // STATE_NONEXIST


    // file-icon, folder-icon, file-tint, folder-tint
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", 0,      0));    // IGNORED
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", 0,      0));    // NONE
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", 0,      0));    // NORMAL
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", 0,      0));    // NOCHANGE
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("dialog-warning", 0       , 0,      0));    // ERROR
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>(0,                0       , 0,      0));    // EMPTY
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", new_fg, 0));    // NEW
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", mod_fg, 0));    // MODIFIED
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", mod_fg, 0));    // RENAMED
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", con_fg, 0));    // CONFLICT
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", del_fg, 0));    // REMOVED
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", unk_fg, unk_fg));  // MISSING
    this->icon_details.push_back(std::tuple<Glib::ustring, Glib::ustring, Glib::ustring, Glib::ustring>("text-x-generic", "folder", unk_fg, unk_fg));  // NONEXIST

    assert(this->icon_details.size() == STATE_MAX && this->text_attributes.size() == STATE_MAX);
}

std::vector<Glib::ustring> DiffTreeStore::value_paths(Gtk::TreeModel::iterator it) {
    std::vector<Glib::ustring> result;
    for (int i = 0; i < this->ntree; i++) {
        result.push_back(this->value_path(it, i));
    }
    return result;
}

Glib::ustring DiffTreeStore::value_path(Gtk::TreeModel::iterator it, int pane) {
    Glib::Value<Glib::ustring> path;
    path.init(Glib::Value<Glib::ustring>::value_type());
    this->get_value_impl(it, this->column_index(COL_PATH, pane), path);
    return path.get();
}

bool DiffTreeStore::is_folder(const Gtk::TreeModel::iterator& it, int pane, std::string path) {
    // A folder may no longer exist, and is only tracked by VC.
    // Therefore, check the icon instead, as the pane already knows.
    Glib::Value<Glib::ustring> icon;
    icon.init(Glib::Value<Glib::ustring>::value_type());
    this->get_value_impl(it, this->column_index(COL_ICON, pane), icon);
    return icon.get() == "folder" or boost::filesystem::is_directory(path);
}

int DiffTreeStore::column_index(Col col, int pane) {
    return this->ntree * col + pane;
}

Gtk::TreeStore::iterator DiffTreeStore::add_entries(const Gtk::TreeNodeChildren* parent, std::vector<std::string> names) {
    Gtk::TreeStore::iterator child;
    if (parent) {
        child = this->append(*parent);
    } else {
        child = this->append();
    }
    for (size_t pane = 0; pane < names.size(); pane++) {
        std::string path = names[pane];
        Glib::Value<Glib::ustring> value;
        value.init(Glib::Value<Glib::ustring>::value_type());
        value.set(path);
        this->set_value_impl(child, this->column_index(COL_PATH, pane), value);
    }
    return child;
}

void DiffTreeStore::add_empty(const Gtk::TreeNodeChildren& parent, std::string text) {
    Gtk::TreeStore::iterator it = this->append(parent);
    for (int pane = 0; pane < this->ntree; pane++) {
        Glib::Value<int> value;
        this->set_value_impl(it, this->column_index(COL_PATH, pane), value);
        this->set_state(it, pane, STATE_EMPTY, text);
    }
}

void DiffTreeStore::add_error(const Gtk::TreeNodeChildren& parent, Glib::ustring msg, int pane) {
    Gtk::TreeStore::iterator it = this->append(parent);
    for (int i = 0; i < this->ntree; i++) {
        Glib::Value<int> value;
        value.init(Glib::Value<int>::value_type());
        value.set(STATE_ERROR);
        this->set_value_impl(it, this->column_index(COL_STATE, i), value);
    }
    this->set_state(it, pane, STATE_ERROR, msg);
}

void DiffTreeStore::set_path_state(const Gtk::TreeStore::iterator& it, int pane, FileState state, int isdir) {
    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    this->get_value_impl(it, this->column_index(COL_PATH, pane), value);
    Glib::ustring fullname = value.get();
    Glib::ustring name = Glib::Markup::escape_text(boost::filesystem::path(fullname).parent_path().string());
    this->set_state(it, pane, state, name, isdir);
}

void DiffTreeStore::set_state(const Gtk::TreeStore::iterator& it, int pane, FileState state, Glib::ustring label, int isdir) {
    Glib::ustring icon = isdir ? std::get<1>(this->icon_details[state]) : std::get<0>(this->icon_details[state]);
    Glib::ustring tint = isdir ? std::get<3>(this->icon_details[state]) : std::get<2>(this->icon_details[state]);
    Glib::Value<int> value1;
    value1.init(Glib::Value<int>::value_type());
    value1.set(state);
    this->set_value_impl(it, this->column_index(COL_STATE, pane), value1);
    Glib::Value<Glib::ustring> value2;
    value2.init(Glib::Value<Glib::ustring>::value_type());
    value2.set(label);
    this->set_value_impl(it, this->column_index(COL_TEXT,  pane), value2);
    Glib::Value<Glib::ustring> value3;
    value3.init(Glib::Value<Glib::ustring>::value_type());
    value3.set(icon);
    this->set_value_impl(it, this->column_index(COL_ICON,  pane), value3);
    // FIXME: This is horrible, but EmblemCellRenderer crashes
    // if you try to give it a Gdk.Color property
    Glib::Value<Glib::ustring> value4;
    value4.init(Glib::Value<Glib::ustring>::value_type());
    value4.set(tint);
    this->set_value_impl(it, this->column_index(COL_TINT,  pane), value4);

    TextAttribute* tmp = this->text_attributes[state];
    Glib::ustring fg = tmp->fg;
    Pango::Style style = tmp->style;
    Pango::Weight weight = tmp->weight;
    bool strike = tmp->strike;
    Glib::Value<Glib::ustring> value5;
    value5.init(Glib::Value<Glib::ustring>::value_type());
    value5.set(fg);
    this->set_value_impl(it, this->column_index(COL_FG, pane), value5);
    Glib::Value<Pango::Style> value6;
    value6.init(Glib::Value<Pango::Style>::value_type());
    value6.set(style);
    this->set_value_impl(it, this->column_index(COL_STYLE, pane), value6);
    Glib::Value<Pango::Weight> value7;
    value7.init(Glib::Value<Pango::Weight>::value_type());
    value7.set(weight);
    this->set_value_impl(it, this->column_index(COL_WEIGHT, pane), value7);
    Glib::Value<bool> value8;
    value8.init(Glib::Value<bool>::value_type());
    value8.set(strike);
    this->set_value_impl(it, this->column_index(COL_STRIKE, pane), value8);
}

FileState DiffTreeStore::get_state(const Gtk::TreeModel::iterator& it, int pane) {
    int STATE = this->column_index(COL_STATE, pane);
    Glib::Value<int> value;
    value.init(Glib::Value<int>::value_type());
    this->get_value_impl(it, STATE, value);
    return static_cast<FileState>(value.get());
}

std::vector<Gtk::TreeModel::iterator> DiffTreeStore::inorder_search_down(Gtk::TreeModel::iterator& it) {
    std::vector<Gtk::TreeModel::iterator> result;
    while (it) {
        Gtk::TreeModel::iterator child;
        if (this->iter_children_vfunc(it, child)) {
            it = child;
        } else {
            Gtk::TreeModel::iterator next;
            if (this->iter_next_vfunc(it, next)) {
                it = next;
            } else {
                while (1) {
                    Gtk::TreeModel::iterator parent;
                    bool tmp = this->iter_parent_vfunc(it, parent);
                    it = parent;
                    if (tmp) {
                        if (this->iter_next_vfunc(it, next)) {
                            it = next;
                            break;
                        }
                    } else {
                        return result;
                    }
                }
            }
        }
        result.push_back(it);
    }
    return result;
}

std::vector<Gtk::TreeModel::iterator> DiffTreeStore::inorder_search_up(Gtk::TreeModel::iterator& it) {
    std::vector<Gtk::TreeModel::iterator> result;
    while (it) {
        Gtk::TreeModel::Path path = this->get_path(it);
        if (path[-1]) {
#if 0
            path = path[:-1] + [path[-1] - 1];
#endif
            it = this->get_iter(path);
            while (1) {
                int nc = this->iter_n_children_vfunc(it);
                if (nc) {
                    Gtk::TreeModel::iterator iter;
                    this->iter_nth_child_vfunc(it, nc-1, iter);
                    it = iter;
                } else {
                    break;
                }
            }
        } else {
            Gtk::TreeModel::iterator up;
            if (this->iter_parent_vfunc(it, up)) {
                it = up;
            } else {
                return result;
            }
        }
        result.push_back(it);
    }
    return result;
}

std::pair<Gtk::TreeModel::Path, Gtk::TreeModel::Path> DiffTreeStore::_find_next_prev_diff(Gtk::TreeModel::Path start_path) {
    Gtk::TreeModel::Path prev_path;
    Gtk::TreeModel::Path next_path;
    Gtk::TreeModel::iterator start_iter = this->get_iter(start_path);

    for (Gtk::TreeModel::iterator it : this->inorder_search_up(start_iter)) {
        FileState state = this->get_state(it, 0);
        if (state != STATE_NORMAL && state != STATE_EMPTY) {
            prev_path = this->get_path(it);
            break;
        }
    }

    for (Gtk::TreeModel::iterator it : this->inorder_search_down(start_iter)) {
        FileState state = this->get_state(it, 0);
        if (state != STATE_NORMAL && state != STATE_EMPTY) {
            next_path = this->get_path(it);
            break;
        }
    }


    return std::pair<Gtk::TreeModel::Path, Gtk::TreeModel::Path>(prev_path, next_path);
}

bool DiffTreeStore::treeview_search_cb(const Glib::RefPtr<TreeModel>& model, int column, const Glib::ustring& key, const Gtk::TreeModel::iterator& iter) {
    // If the key contains a path separator, search the whole path,
    // otherwise just use the filename. If the key is all lower-case, do a
    // case-insensitive match.
#if 0
    abs_search = key.find('/') >= 0;
    lower_key = key.islower();

    for (path in model.value_paths(it)) {
        if (not path) {
            continue;
        }
        lineText = path if abs_search else os.path.basename(path);
        lineText = lineText.lower() if lower_key else lineText;
        if (lineText.find(key) != -1) {
            return false;
        }
    }
#endif
    return true;
}
