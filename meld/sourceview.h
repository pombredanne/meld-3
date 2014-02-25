/* Copyright (C) 2009 Vincent Legoll <vincent.legoll@gmail.com>
 * Copyright (C) 2010-2011, 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__SOURCEVIEW_H__
#define __MELD__SOURCEVIEW_H__

#include <gtksourceviewmm.h>

class LanguageManager {
public:
    static Glib::RefPtr<Gsv::Language> get_language_from_file(std::string filename);

    static Glib::RefPtr<Gsv::Language> get_language_from_mime_type(std::string mime_type);

};


class MeldSourceView : public Gsv::View {
private:

    std::map<Glib::ustring, Glib::ustring> __gsettings_bindings__ = {
        {"indent-width", "indent-width"},
        {"insert-spaces-instead-of-tabs", "insert-spaces-instead-of-tabs"},
        {"show-line-numbers", "show-line-numbers"},
        {"draw-spaces", "draw-spaces"},
        {"wrap-mode", "wrap-mode"}
    };

    Glib::RefPtr<Gtk::Builder> m_refGlade;

    static const std::map<guint, GdkModifierType> replaced_entries;

public:
    typedef sigc::signal<void> type_signal_toggle_overwrite;
    type_signal_toggle_overwrite signal_toggle_overwrite() {
        return m_signal_toggle_overwrite;
    }
    type_signal_toggle_overwrite m_signal_toggle_overwrite;

    MeldSourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    virtual ~MeldSourceView();

    int get_y_for_line_num(int line);
    int get_line_num_for_y(int y);
};

#endif
