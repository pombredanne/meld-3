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

#ifndef __MELD__VC__VC_H__
#define __MELD__VC__VC_H__

#include <list>
#include <libintl.h>
#define _(str) gettext(str)
#include <map>

#include "../difflib/src/difflib.h"
#include "../util/compat.h"

// ignored, new, normal, ignored changes,
// error, placeholder, vc added
// vc modified, vc renamed, vc conflict, vc removed
// locally removed, end
enum FileState {STATE_IGNORED, STATE_NONE, STATE_NORMAL, STATE_NOCHANGE,
    STATE_ERROR, STATE_EMPTY, STATE_NEW,
    STATE_MODIFIED, STATE_RENAMED, STATE_CONFLICT, STATE_REMOVED,
    STATE_MISSING, STATE_NONEXIST, STATE_MAX};

// VC conflict types
enum ConflictType {CONFLICT_MERGED, CONFLICT_BASE, CONFLICT_LOCAL,
    CONFLICT_REMOTE, CONFLICT_MAX};
// These names are used by BZR, and are logically identical.
extern ConflictType CONFLICT_OTHER;
extern ConflictType CONFLICT_THIS;

extern std::vector<std::string> conflicts;
#if 0
assert len(conflicts) == CONFLICT_MAX;
#endif

enum Data {DATA_INVALID, DATA_NAME, DATA_STATE, DATA_REVISION, DATA_OPTIONS};


class Entry {
protected:
    // These are labels for possible states of version controlled files;
    // not all states have a label to avoid visual clutter.
    std::map<FileState, std::string> state_names = {
        {STATE_IGNORED, _("Ignored")},
        {STATE_NONE, _("Unversioned")},
        {STATE_NORMAL, ""},
        {STATE_NOCHANGE, ""},
        {STATE_ERROR, _("Error")},
        {STATE_EMPTY, ""},
        {STATE_NEW, _("Newly added")},
        {STATE_MODIFIED, _("Modified")},
        {STATE_RENAMED, _("Renamed")},
        {STATE_CONFLICT, std::string("<b>") + _("Conflict") + "</b>"},
        {STATE_REMOVED, _("Removed")},
        {STATE_MISSING, _("Missing")},
        {STATE_NONEXIST, _("Not present")}
    };
public:
    std::string path;
protected:
    std::string parent;
    std::string rev;
    std::string options;
public:
    FileState state;
    bool isdir;
    std::string name;
protected:

    Entry(std::string path, std::string name, FileState state);

    std::string __str__();

    std::string __repr__();

    std::string get_status();

};


class Dir : public Entry {
public:
    Dir(std::string path, std::string name, FileState state, std::string options = "");
};


class File : public Entry {
public:
    File(std::string path, std::string name, FileState state, std::string rev = "", std::string options = "");
};

class Vc {
public:
    std::string NAME;
    std::string CMD;
protected:

    std::string VC_DIR = "";
    bool VC_ROOT_WALK = true;
    int *VC_METADATA = 0;

public:
    std::vector<Data> VC_COLUMNS = { DATA_NAME, DATA_STATE };
    std::string root;
    std::string location;
protected:

    Vc(std::string path);

    virtual std::vector<std::string> commit_command(std::string message);
public:
    virtual std::vector<std::string> update_command();
protected:
    virtual std::vector<std::string> add_command();
public:
    virtual std::vector<std::string> remove_command(int force = 0);
protected:
    virtual std::vector<std::string> revert_command();

    virtual void resolved_command();

    // Prototyping VC interface version 2

public:
    virtual std::vector<std::string> get_files_to_commit(std::vector<std::string> paths);

    virtual std::string get_commit_message_prefill();

    virtual void update(int runner, int files);

    virtual void push(int runner);

    virtual void revert(int runner, std::vector<std::string> files);

    virtual std::string get_commits_to_push_summary();

    virtual void add(int runner, int files);

    virtual void remove(int runner, int files);

    virtual void resolve(int runner, int files);

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
    virtual std::string get_path_for_repo_file(std::string path, std::string commit = "");

    /*!
     * Returns a file path for the conflicted repository path
     *
     * *conflict* is the side of the conflict to be retrieved, and
     * must be one of the CONFLICT_* constants.
     */
    virtual std::pair<std::string, bool> get_path_for_conflict(std::string path, ConflictType conflict);
    virtual bool supports_get_path_for_conflict() = 0;

    virtual void update_actions_for_paths(std::map<std::string, FileState> path_states, std::map<std::string, bool> actions);
    virtual bool supports_update_actions_for_paths() = 0;

    std::string get_working_directory(std::string workdir);

    virtual void cache_inventory(std::string topdir);

    virtual void uncache_inventory();

    /*!
     * Update the state of a specific file.  For example after a file
     * has been modified and saved, its state may be out of date and require
     * updating.  This can be implemented for Vc plugins that cache file
     * states, eg 'git' an 'bzr' so that the top-level file status is always
     * accurate.
     */
    void update_file_state(int path);

    std::vector<Entry*> listdir(std::string path = ".");

    std::pair<std::vector<Dir*>, std::vector<File*>> lookup_files(std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files, std::string directory = "");

    virtual std::pair<std::vector<Dir*>, std::vector<File*>> _get_dirsandfiles(std::string directory, std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files);

    /*!
     * Return the entry associated with the given path in this VC
     * If the given path does not correspond to any entry in the VC, this
     * method returns return None.
     */
    virtual Entry* get_entry(std::string path);

    static bool is_installed(const Vc& cls);

    std::pair<std::string, std::string> is_in_repo(std::string path);

    static bool check_repo_root(const Vc& cls, std::string location);

    static std::string find_repo_root(const Vc& cls, std::string location);

    /*! Determine if a directory is a valid repository for this class */
    static bool valid_repo(const Vc& cls, std::string path);
};


class CachedVc : public Vc {

protected:
    std::map<std::string, FileState> _tree_cache;

    CachedVc(std::string location);

    virtual void cache_inventory(std::string directory);

    virtual void uncache_inventory();

    virtual std::map<std::string, FileState> _lookup_tree_cache(std::string directory);

    std::map<std::string, FileState> _get_tree_cache(std::string directory);
};

/*! Raised when a VC module is passed an invalid (or not present) path. */
class InvalidVCPath : public ValueError {
private:
    Vc& vc;
    std::string path;
    std::string error;
public:
    InvalidVCPath(Vc& vc, std::string path, std::string error);

    virtual const char* what() const throw();
};


/*! Raised when a VC module is passed a revision spec it can't handle. */
class InvalidVCRevision : public ValueError {

    Vc& vc;
    int revision;
    int error;

    InvalidVCRevision(Vc& vc, int revision, int error);

    virtual const char* what() const throw();
};

/*! Return the stdout output of a given command */
extern bool _popen(std::vector<std::string> cmd, std::string cwd, std::string& result);


/*! Return the return value of a given command */
extern int call(std::vector<std::string> cmds, std::string cwd = "");

#endif
