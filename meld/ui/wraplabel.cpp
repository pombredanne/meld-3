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

#include "wraplabel.h"

WrapLabel::WrapLabel(const Glib::ustring& text) : Gtk::Label(text) {

    this->__wrap_width = 0;
    this->layout = this->get_layout();
    this->layout->set_wrap(Pango::WRAP_WORD_CHAR);

    if (!text.empty()) {
        this->set_text(text);
    }

    this->set_alignment(0.0, 0.0);
}

void WrapLabel::do_size_request(Gtk::Requisition &requisition) {
    Glib::RefPtr<const Pango::Layout> layout = this->get_layout();
    int width, height;
    layout->get_pixel_size(width, height);
    requisition.width = 0;
    requisition.height = height;
}

void WrapLabel::size_allocate(Gtk::Allocation &allocation) {
    Gtk::Label::size_allocate(allocation);
    this->__set_wrap_width(allocation.get_width());
}

void WrapLabel::set_text(const Glib::ustring& text) {
    Gtk::Label::set_text(text);
    this->__set_wrap_width(this->__wrap_width);
}

void WrapLabel::set_markup(const Glib::ustring& text) {
    Gtk::Label::set_markup(text);
    this->__set_wrap_width(this->__wrap_width);
}

void WrapLabel::__set_wrap_width(int width) {
    if (width == 0) {
        return;
    }
    this->layout->set_width(width * Pango::SCALE);
    if (this->__wrap_width != width) {
        this->__wrap_width = width;
        this->queue_resize();
    }
}
