/* Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
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

#include "compat.h"

#include <sstream>
#include <boost/filesystem.hpp>
#include <regex>

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    split(s, delim, result);
    return result;
}

std::vector<std::string> os_listdir(std::string direcotry) {
    boost::filesystem::path someDir(direcotry);
    boost::filesystem::directory_iterator end_iter;

    std::vector<std::string> result;

    if (boost::filesystem::exists(someDir) && boost::filesystem::is_directory(someDir)) {
        for (boost::filesystem::directory_iterator dir_iter(someDir); dir_iter != end_iter; ++dir_iter) {
            if (boost::filesystem::is_regular_file(dir_iter->status())) {
                const boost::filesystem::directory_entry& entry = *dir_iter;
                result.push_back(entry.path().string());
            }
        }
    }

    return result;
}

std::string regex_escape(const std::string& string_to_escape) {
    static const std::regex re_boostRegexEscape("[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]");
    const std::string rep("\\\\\\1&");
    std::string result = std::regex_replace(string_to_escape, re_boostRegexEscape, rep, std::regex_constants::match_default | std::regex_constants::format_sed);
    return result;
}

template <typename T>
bool contains(std::list<T> list, T element) {
    return std::find(list.begin(), list.end(), element) != list.end();
}

template <typename T>
bool contains(std::vector<T> vector, T element) {
    return std::find(vector.begin(), vector.end(), element) != vector.end();
}

template <typename T>
bool contains(std::set<T> set, T element) {
    return std::find(set.begin(), set.end(), element) != set.end();
}

AttributeError::AttributeError(std::string message) : message(message) {}

const char* AttributeError::what() const throw() {
    return message.c_str();
}


IOError::IOError(std::string message) : message(message) {}

const char* IOError::what() const throw() {
    return message.c_str();
}

NotImplementedError::NotImplementedError(std::string message) : message(message) {}

NotImplementedError::NotImplementedError() : message("") {}

const char* NotImplementedError::what() const throw() {
    return message.c_str();
}
