/* Copyright (C) 2002-2005 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2010, 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
 * Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <map>
#include <cassert>
#include <sstream>
#include <boost/filesystem.hpp>
#include <functional>
#include <boost/algorithm/string/join.hpp>

#include "../conf.h"

#include "_vc.h"

// Lifted from the itertools recipes section
template <class T>
std::pair<std::vector<T>, std::vector<T>> partition(std::function<bool(T)> pred, std::vector<T> iterable) {
    std::vector<T> first;
    std::vector<T> second;
    for (T t : iterable) {
        if (pred(t)) {
            first.push_back(t);
        } else {
            second.push_back(t);
        }
    }
    return std::pair<std::vector<T>, std::vector<T>>(first, second);
}

ConflictType CONFLICT_OTHER = CONFLICT_REMOTE;
ConflictType CONFLICT_THIS = CONFLICT_LOCAL;

std::vector<std::string> conflicts = {_("Merged"), _("Base"), _("Local"), _("Remote")};

Entry::Entry(std::string path, std::string name, FileState state) {
    this->path = path;
    this->state = state;
    boost::filesystem::path tmp(name);
    this->parent = tmp.parent_path().string();
    this->name = tmp.filename().string();
}

std::string Entry::__str__() {
    std::stringstream ss;
    ss << typeid(this).name() << ":" << this->path << " ";
    std::string tmp = this->get_status();
    if (tmp.empty()) {
        ss << "Normal";
    } else {
        ss << tmp;
    }
    return ss.str();
}

std::string Entry::__repr__() {
    std::stringstream ss;
    ss << typeid(this).name() << "(" + this->name << ", " << this->path << ", " << this->state << ")";
    return ss.str();
}

std::string Entry::get_status() {
    if (this->state_names.count(this->state)) {
        return this->state_names[this->state];
    }
    return "";
}


Dir::Dir(std::string path, std::string name, FileState state, std::string options) : Entry(path, name, state) {
    this->isdir = true;
    this->rev = "";
    this->options = options;
}


File::File(std::string path, std::string name, FileState state, std::string rev, std::string options) : Entry(path, name, state) {
    assert(path[-1] != '/');
    this->isdir = false;
    this->rev = rev;
    this->options = options;
}


Vc::Vc(std::string path) {
    // Save the requested comparison location. The location may be a
    // sub-directory of the repository we are diffing and can be useful in
    // limiting meld's output to the requested location.
    //
    // If the location requested is a file (e.g., a single-file command line
    // comparison) then the location is set to the containing directory.
    std::pair<std::string, std::string> pair = this->is_in_repo(path);
    this->root = pair.first;
    this->location = pair.second;
    if (this->root.empty()) {
        throw ValueError("");
    }
}

std::vector<std::string> Vc::commit_command(std::string message) {
    throw NotImplementedError();
}

void Vc::update_actions_for_paths(std::map<std::string, FileState> path_states, std::map<std::string, bool> actions) {
}

std::vector<std::string> Vc::update_command() {
    throw NotImplementedError();
}

std::vector<std::string> Vc::add_command() {
    throw NotImplementedError();
}

std::vector<std::string> Vc::remove_command(int force) {
    throw NotImplementedError();
}

std::vector<std::string> Vc::revert_command() {
    throw NotImplementedError();
}

void Vc::resolved_command() {
    throw NotImplementedError();
}

// Prototyping VC interface version 2

std::vector<std::string> Vc::get_files_to_commit(std::vector<std::string> paths) {
    throw NotImplementedError();
}

std::string Vc::get_commit_message_prefill() {
    return 0;
}

void Vc::update(int runner, int files) {
    throw NotImplementedError();
}

void Vc::push(int runner) {
    throw NotImplementedError();
}

void Vc::revert(int runner, std::vector<std::string> files) {
    throw NotImplementedError();
}

std::string Vc::get_commits_to_push_summary() {
    throw NotImplementedError();
}

void Vc::add(int runner, int files) {
    throw NotImplementedError();
}

void Vc::remove(int runner, int files) {
    throw NotImplementedError();
}

void Vc::resolve(int runner, int files) {
    throw NotImplementedError();
}

/*!
 * Returns a file path for the repository path at commit
 *
 * If *commit* is given, the path returned will point to a copy of
 * the file at *path* at the given commit, as interpreted by the
 * VCS. If *commit* is **None**, the current revision is used.
 *
 * Even if the VCS maintains an on-disk copy of the given path, a
 * temp file with file-at-commit content must be created and its
 * path returned, to avoid destructive editing. The VCS plugin
 * must **not** delete temp files it creates.
 */
std::string Vc::get_path_for_repo_file(std::string path, std::string commit) {
    throw NotImplementedError();
}

/*!
 * Returns a file path for the conflicted repository path
 *
 * *conflict* is the side of the conflict to be retrieved, and
 * must be one of the CONFLICT_* constants.
 */
std::pair<std::string, bool> Vc::get_path_for_conflict(std::string path, ConflictType conflict) {
    throw NotImplementedError();
}

std::string Vc::get_working_directory(std::string workdir) {
    return workdir;
}

void Vc::cache_inventory(std::string topdir) {
}

void Vc::uncache_inventory() {
}

/*!
 * Update the state of a specific file.  For example after a file
 * has been modified and saved, its state may be out of date and require
 * updating.  This can be implemented for Vc plugins that cache file
 * states, eg 'git' an 'bzr' so that the top-level file status is always
 * accurate.
 */
void Vc::update_file_state(int path) {
}

std::vector<Entry*> Vc::listdir(std::string path) {
    std::vector<std::string> entries;
    try {
        for (std::string e : os_listdir(path)) {
            if (e != this->VC_DIR) {
                entries.push_back(e);
            }
        }
        std::sort(entries.begin(), entries.end());
    } catch (...) {
        entries.clear();
    }
    std::vector<std::pair<std::string, std::string>> full_entries;
    for (auto f : entries) {
        full_entries.push_back(std::pair<std::string, std::string>(f, path + "/" + f));
    }
    std::function<bool(std::pair<std::string, std::string>)> tmp1 = [] (std::pair<std::string, std::string> p) { return boost::filesystem::is_directory(p.second); };
    auto tmp = partition(tmp1, full_entries);
    std::vector<std::pair<std::string, std::string>> cfiles = tmp.first;
    std::vector<std::pair<std::string, std::string>> cdirs = tmp.second;
    std::pair<std::vector<Dir*>, std::vector<File*>> tmp2 = this->lookup_files(cdirs, cfiles, path);
    std::vector<Dir*> dirs = tmp2.first;
    std::vector<File*> files = tmp2.second;
    std::vector<Entry*> result;
    result.insert(result.end(), dirs.begin(), dirs.end());
    result.insert(result.end(), files.begin(), files.end());
    return result;
}

std::pair<std::vector<Dir*>, std::vector<File*>> Vc::lookup_files(std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files, std::string directory) {
    // Assumes that all files are in the same directory. files is an array
    // of (name, path) tuples.
    if (dirs.size()) {
        directory = boost::filesystem::path(dirs[0].second).parent_path().string();
    } else if (files.size()) {
        directory = boost::filesystem::path(files[0].second).parent_path().string();
    }
    return this->_get_dirsandfiles(directory, dirs, files);
}

std::pair<std::vector<Dir*>, std::vector<File*>> Vc::_get_dirsandfiles(std::string directory, std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files) {
    throw NotImplementedError();
}

/*!
 * Return the entry associated with the given path in this VC
 * If the given path does not correspond to any entry in the VC, this
 * method returns return None.
 */
Entry* Vc::get_entry(std::string path) {
    std::vector<Entry*> vc_files;
    std::vector<std::pair<std::string, std::string>> empty;
    std::vector<std::pair<std::string, std::string>> other = {std::pair<std::string, std::string>(boost::filesystem::path(path).parent_path().string(), path)};
    for (Dir* x : this->lookup_files(empty, other).first) {
        if (x->name == path) {
            vc_files.push_back(x);
        }
    }
    if (vc_files.empty()) {
        return nullptr;
    }
    return vc_files[0];
}

bool Vc::is_installed(const Vc& cls) {
    std::vector<std::string> tmp;
    tmp.push_back(cls.CMD);
    return call(tmp) == 0;
}

std::pair<std::string, std::string> Vc::is_in_repo(std::string path) {
    std::string location;
    if (boost::filesystem::is_directory(path)) {
        location = path;
    } else {
        location = boost::filesystem::path(path).parent_path().string();
    }

    std::string root;
    if (this->VC_ROOT_WALK) {
        root = find_repo_root(*this, location);
    } else if (check_repo_root(*this, location)) {
        root = location;
    }
    return std::pair<std::string, std::string>(root, location);
}

bool Vc::check_repo_root(const Vc& cls, std::string location) {
    return boost::filesystem::is_directory(location + "/" + cls.VC_DIR);
}

std::string Vc::find_repo_root(const Vc& cls, std::string location) {
    while (!location.empty()) {
        if (cls.check_repo_root(cls, location)) {
            return location;
        }

        std::string old = location;
        location = boost::filesystem::path(location).parent_path().string();
        if (location == old) {
            break;
        }
    }
    return "";
}

/*! Determine if a directory is a valid repository for this class */
bool Vc::valid_repo(const Vc& cls, std::string path) {
    throw new NotImplementedError();
}



CachedVc::CachedVc(std::string location) : Vc(location) {
}

void CachedVc::cache_inventory(std::string directory) {
    this->_tree_cache = this->_lookup_tree_cache(directory);
}

void CachedVc::uncache_inventory() {
    this->_tree_cache.clear();
}

std::map<std::string, FileState> CachedVc::_lookup_tree_cache(std::string directory) {
    throw NotImplementedError();
}

std::map<std::string, FileState> CachedVc::_get_tree_cache(std::string directory) {
    if (this->_tree_cache.empty()) {
        this->cache_inventory(directory);
    }
    return this->_tree_cache;
}

InvalidVCPath::InvalidVCPath(Vc& vc, std::string path, std::string error) : ValueError(""), vc(vc) {
    this->path = path;
    this->error = error;
}

const char* InvalidVCPath::what() const throw() {
    std::stringstream ss;
    ss << this->vc.NAME << ": Path " << this->path << " is invalid or not present" <<std::endl << "Error: " << this->error << std::endl;
    return ss.str().c_str();
}


InvalidVCRevision::InvalidVCRevision(Vc& vc, int revision, int error) : ValueError(""), vc(vc) {
    this->revision = revision;
    this->error = error;
}

const char* InvalidVCRevision::what() const throw() {
    std::stringstream ss;
    ss << this->vc.NAME << ": Doesn't understand or have revision " << this->revision << std::endl << "Error: " << this->error << std::endl;
    return ss.str().c_str();
}

/*! Return the stdout output of a given command */
bool _popen(std::vector<std::string> cmds, std::string cwd, std::string& result) {
    std::string cmd = boost::algorithm::join(cmds, " ");
    if (!cwd.empty()) {
        cmd = "cd " + cwd + " && " + cmd;
    }
    FILE* proc = popen(cmd.c_str(), "r");
    if (!proc) {
        return false;
    }
    char buffer[128];
    while (!feof(proc)) {
        if (fgets(buffer, 128, proc) != NULL) {
            result += buffer;
        }
    }
    pclose(proc);
    return true;
}


/*! Return the return value of a given command */
int call(std::vector<std::string> cmds, std::string cwd) {
    std::string cmd = boost::algorithm::join(cmds, " ");
    if (!cwd.empty()) {
        cmd = "cd " + cwd + " && " + cmd;
    }
    return std::system(cmd.c_str());
}
