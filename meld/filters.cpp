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

#include <boost/algorithm/string/join.hpp>

#include "misc.h"
#include "util/compat.h"
#include "difflib/src/difflib.h"

#include "filters.h"

FilterEntry::FilterEntry(std::string label, bool active, std::shared_ptr<std::regex> filter, std::string filter_string) {
    this->label = label;
    this->active = active;
    this->filter = filter;
    this->filter_string = filter_string;
}

std::unique_ptr<std::regex> FilterEntry::_compile_regex(std::string regex) {
    std::unique_ptr<std::regex> compiled(new std::regex());
    try {
        compiled->assign(regex + "(?m)");
        return compiled;
    } catch (std::regex_error& e) {
        compiled.reset();
    }
    return compiled;
}

std::unique_ptr<std::regex> FilterEntry::_compile_shell_pattern(std::string pattern) {
    std::unique_ptr<std::regex> compiled(new std::regex());
    std::vector<std::string> bits = split(pattern);
    std::string regex;
    if (bits.size() > 1) {
        std::list<std::string> regexes;
        for (std::string b : bits) {
            std::string tmp = shell_to_regex(b);
            regexes.push_back(tmp.substr(0, tmp.length() - 1));
        }
        regex = "(" + boost::algorithm::join(regexes, "|") + ")$";
    } else if (!bits.empty()) {
        regex = shell_to_regex(bits[0]);
    } else {
        // An empty pattern would match everything, so skip it
        compiled.reset();
        return compiled;
    }

    try {
        compiled->assign(regex);
    } catch (std::regex_error& e) {
        compiled.reset();
        return compiled;
    }

    return compiled;
}

FilterEntry* FilterEntry::parse(std::string string, FilterEntry::Type filter_type) {
    std::vector<std::string> elements = split(string, '\t');
    if (elements.size() < 3) {
        return 0;
    }
    std::string name = elements[0];
    bool active = bool(std::stoi(elements[1]));
    std::vector<std::string> subelements(elements.begin() + 2, elements.end());
    std::string filter_string = boost::algorithm::join(subelements, " ");
    std::unique_ptr<std::regex> compiled = FilterEntry::compile_filter(filter_string, filter_type);
    if (!compiled) {
        active = false;
    }
    std::shared_ptr<std::regex> tmp{std::move(compiled)};
    return new FilterEntry(name, active, tmp, filter_string);
}

FilterEntry* FilterEntry::new_from_gsetting(std::tuple<std::string, bool, std::string> elements, FilterEntry::Type filter_type) {
    std::string name = std::get<0>(elements);
    bool active = std::get<1>(elements);
    std::string filter_string = std::get<2>(elements);
    std::unique_ptr<std::regex> compiled = FilterEntry::compile_filter(filter_string, filter_type);
    if (!compiled) {
        active = false;
    }
    std::shared_ptr<std::regex> tmp{std::move(compiled)};
    return new FilterEntry(name, active, tmp, filter_string);
}

std::unique_ptr<std::regex> FilterEntry::compile_filter(std::string filter_string, FilterEntry::Type filter_type) {
    std::unique_ptr<std::regex> compiled;
    if (filter_type == FilterEntry::REGEX) {
        compiled = FilterEntry::_compile_regex(filter_string);
    } else if (filter_type == FilterEntry::SHELL) {
        compiled = FilterEntry::_compile_shell_pattern(filter_string);
    } else {
        throw ValueError("Unknown filter type");
    }
    return compiled;
}

FilterEntry::FilterEntry(const FilterEntry& other) {
    this->label = other.label;
    this->active = other.active;
    this->filter = other.filter;
    this->filter_string = other.filter_string;
}
