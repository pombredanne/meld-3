/* Copyright (C) 2009 Vincent Legoll <vincent.legoll@gmail.com>
 * Copyright (C) 2010-2011, 2013-2014 Kai Willadsen <kai.willadsen@gmail.com>
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

#include "sourceview.h"
#include "settings.h"

Glib::RefPtr<Gsv::Language> LanguageManager::get_language_from_file(std::string filename) {
    Glib::RefPtr<Gio::File> f = Gio::File::create_for_path(filename);
    Glib::RefPtr<Gio::FileInfo> info;
    try {
        info = f->query_info(G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                            Gio::FILE_QUERY_INFO_NONE);
    } catch (Glib::Error &e) {
        return Glib::RefPtr<Gsv::Language>();
    }
    std::string content_type = info->get_content_type();
    return Gsv::LanguageManager::get_default()->guess_language(filename, content_type);
}

Glib::RefPtr<Gsv::Language> LanguageManager::get_language_from_mime_type(std::string mime_type) {
    Glib::ustring content_type = Gio::content_type_from_mime_type(mime_type);
    return Gsv::LanguageManager::get_default()->guess_language("", content_type);
}

const std::map<guint, GdkModifierType> MeldSourceView::replaced_entries = {
    // We replace the default GtkSourceView undo mechanism
    {GDK_KEY_z, GDK_CONTROL_MASK},
    {GDK_KEY_z, (GdkModifierType)(GDK_CONTROL_MASK | GDK_SHIFT_MASK)},
    // We replace the default line movement behaviour of Alt+Up/Down
    {GDK_KEY_Up, GDK_MOD1_MASK},
    {GDK_KEY_KP_Up, GDK_MOD1_MASK},
    {GDK_KEY_Down, GDK_MOD1_MASK},
    {GDK_KEY_KP_Down, GDK_MOD1_MASK},
    // ...and Alt+Left/Right
    {GDK_KEY_Left, GDK_MOD1_MASK},
    {GDK_KEY_KP_Left, GDK_MOD1_MASK},
    {GDK_KEY_Right, GDK_MOD1_MASK},
    {GDK_KEY_KP_Right, GDK_MOD1_MASK}
};

MeldSourceView::MeldSourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) : Gsv::View(cobject), m_refGlade(refGlade) {
    bind_settings(this, __gsettings_bindings__);
#if binding_set_is_always_null
    GtkBindingSet* binding_set = gtk_binding_set_find("GtkSourceView");
    for (std::pair<guint, GdkModifierType> tmp : MeldSourceView::replaced_entries) {
        guint key = tmp.first;
        GdkModifierType modifiers = tmp.second;
        gtk_binding_entry_remove(binding_set, key, modifiers);
    }
#endif

}

MeldSourceView::~MeldSourceView() {
}

int MeldSourceView::get_y_for_line_num(int line) {
    Glib::RefPtr<Gtk::TextBuffer> buf = this->get_buffer();
    Gtk::TextBuffer::iterator it = buf->get_iter_at_line(line);
    int y, h;
    this->get_line_yrange(it, y, h);
    if (line >= buf->get_line_count()) {
        return y + h - 1;
    }
    return y;
}

int MeldSourceView::get_line_num_for_y(int y) {
    Gtk::TextBuffer::iterator target_iter;
    int line_top;
    this->get_line_at_y(target_iter, y, line_top);
    return target_iter.get_line();
}
