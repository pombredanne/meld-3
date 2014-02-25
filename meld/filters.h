/* Copyright (C) 2011-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#ifndef __MELD__FILTERS_H__
#define __MELD__FILTERS_H__

#include <vector>
#include <string>
#include <regex>

class FilterEntry {

    const std::vector<std::string> __slots__ = {"label", "active", "filter", "filter_string"};

public:
    enum Type {
        REGEX = 0,
        SHELL = 1
    };

    std::string label;
    bool active;
    std::shared_ptr<std::regex> filter;
    std::string filter_string;

    FilterEntry(std::string label, bool active, std::shared_ptr<std::regex> filter, std::string filter_string);

    static std::unique_ptr<std::regex> _compile_regex(std::string regex);

    static std::unique_ptr<std::regex> _compile_shell_pattern(std::string pattern);

    static FilterEntry *parse(std::string string, FilterEntry::Type filter_type);

    static FilterEntry *new_from_gsetting(std::tuple<std::string, bool, std::string> elements, FilterEntry::Type filter_type);

    static std::unique_ptr<std::regex> compile_filter(std::string filter_string, FilterEntry::Type filter_type);

    FilterEntry(const FilterEntry& other);
};

#endif
