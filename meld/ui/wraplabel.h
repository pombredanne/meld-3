/* Copyright (c) 2005 VMware, Inc.
 * Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Python translation from wrapLabel.{cc|h} by Gian Mario Tagliaretti
 */

#ifndef __MELD__UI__WRAPLABEL_H__
#define __MELD__UI__WRAPLABEL_H__

#include <gtkmm.h>

class WrapLabel : public Gtk::Label {
private:
    std::string __gtype_name__ = "WrapLabel";

    int __wrap_width;
    Glib::RefPtr<Pango::Layout> layout;

public:
    WrapLabel(const Glib::ustring& text = "");

    void do_size_request(Gtk::Requisition &requisition);

    virtual void size_allocate(Gtk::Allocation &allocation);

    virtual void set_text(const Glib::ustring& text);

    virtual void set_markup(const Glib::ustring& text);

    void __set_wrap_width(int width);

};

#endif
