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

#ifndef __MELD__UTIL__COMPAT_H__
#define __MELD__UTIL__COMPAT_H__

#include <string>
#include <vector>
#include <list>
#include <set>

/* Python 2/3 compatibility, named as per six */
typedef std::string text_type;
typedef std::string string_types;

extern std::vector<std::string> splitlines(const std::string &s, bool keepends = false);

extern std::vector<std::string> split(const std::string &s, char delim = ' ');
extern std::vector<std::string> os_listdir(std::string direcotry);
extern std::string regex_escape(const std::string& string_to_escape);
template <typename T>
extern bool contains(std::list<T> list, T element);
template <typename T>
extern bool contains(std::vector<T> vector, T element);
template <typename T>
extern bool contains(std::set<T> set, T element);

class AttributeError : public std::exception {
private:
    std::string message;
public:
    AttributeError(std::string message);
    virtual const char* what() const throw();
};

class IOError : public std::exception {
private:
    std::string message;
public:
    IOError(std::string message);
    virtual const char* what() const throw();
};

class NotImplementedError : public std::exception {
private:
    std::string message;
public:
    NotImplementedError(std::string message);
    NotImplementedError();
    virtual const char* what() const throw();
};

class ValueError : public std::exception {
private:
    std::string message;
public:
    ValueError(std::string message) : message(message) {}
    virtual const char* what() const throw() {
        return message.c_str();
    }
};

class StopIteration : public std::exception {
};

#endif
