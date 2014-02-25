/* Copyright (C) 2002-2005 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2005 Aaron Bentley <aaron.bentley@utoronto.ca>
 * Copyright (C) 2007 José Fonseca <j_r_fonseca@yahoo.co.uk>
 * Copyright (C) 2010-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include <regex>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/fstream.hpp>

#if 0
import errno
import os
import re
import shutil
import subprocess
import tempfile
#endif

#include "../conf.h"

#include "_vc.h"

class Git : public CachedVc {

    std::string GIT_DIFF_FILES_RE = ":(\\d+) (\\d+) [a-z0-9]+ [a-z0-9]+ ([XADMTU])\\t(.*)";
    std::regex DIFF_RE;

    std::map<ConflictType, int> conflict_map = {
        // These are the arguments for git-show
        // CONFLICT_MERGED has no git-show argument unfortunately.
        {CONFLICT_BASE, 1},
        {CONFLICT_LOCAL, 2},
        {CONFLICT_REMOTE, 3}
    };

    std::map<std::string, FileState> state_map = {
        {"X", STATE_NONE},      // Unknown
        {"A", STATE_NEW},       // New
        {"D", STATE_REMOVED},   // Deleted
        {"M", STATE_MODIFIED},  // Modified
        {"T", STATE_MODIFIED},  // Type-changed
        {"U", STATE_CONFLICT}   // Unmerged
    };

    std::map<std::string, std::string> _tree_meta_cache;

    Git(std::string location) : CachedVc(location) {
        DIFF_RE.assign(GIT_DIFF_FILES_RE);
        this->CMD = "git";
        this->NAME = "Git";
        this->VC_DIR = ".git";

        this->VC_COLUMNS = {DATA_NAME, DATA_STATE, DATA_OPTIONS};
        this->_tree_cache.clear();
        this->_tree_meta_cache.clear();
    }

    // TODO static
    bool is_installed() {
        std::string result;
        std::vector<std::string> tmp;
        tmp.push_back(this->CMD);
        tmp.push_back("--version");
        if (!_popen(tmp, "", result)) {
            return false;
        }
        assert(boost::starts_with(result, "git version"));
        return true;
    }

    // TODO static
    bool check_repo_root(std::string location) {
        // Check exists instead of isdir, since .git might be a git-file
        return boost::filesystem::exists(location + "/" + this->VC_DIR);
    }

    virtual std::vector<std::string> commit_command(std::string message) {
        return {this->CMD, "commit", "-m", message};
    }

    // Prototyping VC interface version 2

    void update_actions_for_paths(std::map<std::string, FileState> path_states, std::map<std::string, bool> actions) {
        std::vector<std::string> keys;
        std::vector<FileState> states;
        for (std::map<std::string, FileState>::iterator it = path_states.begin(); it != path_states.end(); ++it) {
            keys.push_back(it->first);
            states.push_back(it->second);
        }

        actions["VcCompare"] = !path_states.empty();
        // TODO: We can't disable this for NORMAL, because folders don't
        // inherit any state from their children, but committing a folder with
        // modified children is expected behaviour.
        bool tmp = true;
        for (FileState s : states) {
            if (s != STATE_NONE && s != STATE_IGNORED) {
                tmp = false;
                break;
            }
        }
        actions["VcCommit"] = tmp;

        actions["VcUpdate"] = true;
        // TODO: We can't do this; this shells out for each selection change...
        // actions["VcPush"] = bool(this->get_commits_to_push());
        actions["VcPush"] = true;

        tmp = true;
        for (FileState s : states) {
            if (s != STATE_NORMAL && s != STATE_REMOVED) {
                tmp = false;
                break;
            }
        }
        actions["VcAdd"] = tmp;
        tmp = false;
        for (FileState s : states) {
            if (s == STATE_CONFLICT) {
                tmp = true;
                break;
            }
        }
        actions["VcResolved"] = tmp;
        tmp = true;
        for (FileState s : states) {
            if (s != STATE_NONE && s != STATE_IGNORED && s != STATE_REMOVED) {
                tmp = false;
                break;
            }
        }
        actions["VcRemove"] = tmp and
            std::find(keys.begin(), keys.end(), this->root) == keys.end();
        tmp = true;
        for (FileState s : states) {
            if (s != STATE_NONE && s != STATE_NORMAL && s != STATE_IGNORED) {
                tmp = false;
                break;
            }
        }
        actions["VcRevert"] = tmp;
    }

    bool supports_update_actions_for_paths() {
        return true;
    }

    virtual std::string get_commits_to_push_summary() {
        std::map<std::string, std::vector<std::string>> branch_refs = this->get_commits_to_push();
        int unpushed_branches = 0;
        int unpushed_commits = 0;
        for (std::map<std::string, std::vector<std::string>>::iterator it = branch_refs.begin(); it != branch_refs.end(); ++it) {
            std::vector<std::string> v = it->second;
            if (!v.empty()) {
                unpushed_branches++;
            }
            unpushed_commits += v.size();
        }
        std::string label;
        if (unpushed_commits) {
            if (unpushed_branches > 1) {
                // Translators: First %s is replaced by translated "%d unpushed
                // commits", second %s is replaced by translated "%d branches"

                char buff[1000];
                sprintf(buff, ngettext("%d unpushed commit", "%d unpushed commits", unpushed_commits), unpushed_commits);
                std::string x = buff;
                sprintf(buff, ngettext("%d branch", "%d branches", unpushed_branches), unpushed_branches);
                std::string y = buff;
                sprintf(buff, _("%s in %s"), x.c_str(), y.c_str());
                label =  buff;
            } else {
                // Translators: These messages cover the case where there is
                // only one branch, and are not part of another message.
                char buff[1000];
                sprintf(buff, ngettext("%d unpushed commit", "%d unpushed commits", unpushed_commits), unpushed_commits);
                label = buff;
            }
        } else {
            label = "";
        }
        return label;
    }

    std::map<std::string, std::vector<std::string>> get_commits_to_push() {
        std::string result;
        std::vector<std::string> cmds{this->CMD, "for-each-ref",
                    "--format=%(refname:short) %(upstream:short)",
                    "refs/heads"};
        _popen(cmds, this->location, result);
        std::vector<std::string> branch_remotes;
        boost::split(branch_remotes, result, boost::is_any_of("\n"));

        std::map<std::string, std::vector<std::string>> branch_revisions;
        for (std::string line : branch_remotes) {
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_any_of(" "));
            if (parts.size() != 2) {
                continue;
            }
            std::string branch = parts[0];
            std::string remote = parts[1];

            std::vector<std::string> cmds{this->CMD, "rev-list", branch, std::string("^") + remote};
            _popen(cmds, this->location, result);
            std::vector<std::string> revisions;
            boost::split(revisions, result, boost::is_any_of("\n"));
            branch_revisions[branch] = revisions;
        }
        return branch_revisions;
    }

    virtual std::vector<std::string> get_files_to_commit(std::list<std::string> paths) {
        std::set<std::string> _files;
        for (std::string p : paths) {
            if (boost::filesystem::is_directory(p)) {
                std::vector<std::string> entries = this->_get_modified_files(p);
                for (std::string e : entries) {
                    std::match_results<std::string::const_iterator> columns;
                    std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
                    std::string::const_iterator start = e.begin();
                    std::string::const_iterator end = e.end();
                    std::regex_search(start, end, columns, this->DIFF_RE, flags);
                    std::string name = columns[3];
                    _files.insert(name);
                }
            } else {
#if 0
                _files.insert(boost::filesystem::path(p, this->root).relative_path().c_str());
#endif
            }
        }
        std::vector<std::string> files(_files.begin(), _files.end());
        std::sort(files.begin(), files.end());
        return files;
    }

    /*! This will be inserted into the commit dialog when commit is run */
    virtual std::string get_commit_message_prefill() {
        std::string commit_path = this->root + "/.git/MERGE_MSG";
        if (boost::filesystem::exists(commit_path)) {
            // If I have to deal with non-ascii, non-UTF8 pregenerated commit
            // messages, I'm taking up pig farming.
#if 0
            with open(commit_path) as f:
                message = f.read().decode("utf8")
            return "\n".join(
                (l for l in message.splitlines() if not l.startswith("#")));
#endif
        }
        return "";
    }

    virtual void update(int runner, int files) {
        std::vector<std::string> command {this->CMD, "pull"};
#if 0
        runner(command, [], refresh=True, working_dir=this->root);
#endif
    }

    virtual void push(int runner) {
        std::vector<std::string> command {this->CMD, "push"};
#if 0
        runner(command, [], refresh=True, working_dir=this->root);
#endif
    }

    virtual void add(int runner, int files) {
        std::vector<std::string> command {this->CMD, "add"};
#if 0
        runner(command, files, refresh=True, working_dir=this->root);
#endif
    }

    virtual void remove(int runner, int files) {
        std::vector<std::string> command {this->CMD, "rm", "-r"};
#if 0
        runner(command, files, refresh=True, working_dir=this->root);
#endif
    }

    virtual void revert(int runner, std::vector<std::string> files) {
        std::vector<bool> exists;
        std::vector<bool> missing;
        for (std::string f : files) {
            bool tmp = boost::filesystem::exists(f);
            exists.push_back(tmp);
            missing.push_back(not tmp);
        }
        if (!exists.empty()) {
            std::vector<std::string> command {this->CMD, "checkout"};
#if 0
            runner(command, exists, refresh=True, working_dir=this->root);
#endif
        }
        if (!missing.empty()) {
            std::vector<std::string> command {this->CMD, "checkout", "HEAD"};
#if 0
            runner(command, missing, refresh=True, working_dir=this->root);;
#endif
        }
    }

    virtual void resolve(int runner, int files) {
        std::vector<std::string> command = { this->CMD, "add" };
#if 0
        runner(command, files, refresh=True, working_dir=this->root);
#endif
    }

    virtual std::pair<std::string, bool> get_path_for_conflict(std::string path, ConflictType conflict) {
        if (not boost::starts_with(path, this->root + "/")) {
            throw InvalidVCPath(*this, path, "Path not in repository");
        }

        if (conflict == CONFLICT_MERGED) {
            // Special case: no way to get merged result from git directly
            return std::pair<std::string, bool>(path, false);
        }

#if 0
        path = path[len(this->root) + 1:];
#endif
#ifdef _WIN32
            path = path.replace("\\", "/");
#endif

        boost::format fmt(":%s:%s");
        fmt % (this->conflict_map[conflict], path);
        std::vector<std::string> args = {"git", "show", fmt.str()};
#if 0
        process = subprocess.Popen(args,
                                   cwd=this->location, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE);
        vc_file = process.stdout;
#else
        std::string vc_file = "";
#endif

        // Error handling here involves doing nothing; in most cases, the only
        // sane response is to return an empty temp file.

        std::string prefix = "meld-tmp-" + conflicts[conflict] + "-";

        boost::filesystem::path f = boost::filesystem::unique_path(prefix + "%%%%-%%%%-%%%%-%%%%");
        boost::filesystem::ofstream(f, std::ios_base::app) << vc_file;

        return std::pair<std::string, bool>(f.string(), true);
    }

    virtual std::string get_path_for_repo_file(std::string path, std::string commit = "") {
        if (commit.empty()) {
            commit = "HEAD";
        } else {
            throw NotImplementedError();
        }

        if (not boost::starts_with(path, this->root + "/")) {
            throw InvalidVCPath(*this, path, "Path not in repository");
        }
#if 0
        path = path[len(this->root) + 1:];
#endif
#ifdef _WIN32
        path = path.replace("\\", "/");
#endif

        std::string obj = commit + ":" + path;
#if 0
        process = subprocess.Popen([this->CMD, "cat-file", "blob", obj],
                                   cwd=this->root, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE);
#else
        std::string vc_file = "";
#endif

        // Error handling here involves doing nothing; in most cases, the only
        // sane response is to return an empty temp file.

        boost::filesystem::path f = boost::filesystem::unique_path("meld-tmp%%%%-%%%%-%%%%-%%%%");
        boost::filesystem::ofstream(f, std::ios_base::app) << vc_file;

        return f.string();
    }

    static bool valid_repo(const Vc& cls, std::string path) {
        // TODO: On Windows, this exit code is wrong under the normal shell; it
        // appears to be correct under the default git bash shell however.
        return not call({cls.CMD, "branch"}, path);
    }

    std::string get_working_directory(std::string workdir) {
        if (boost::starts_with(workdir, "/")) {
            return this->root;
        } else {
            return "";
        }
    }

    std::vector<std::string> _get_modified_files(std::string path) {
        // Update the index before getting status, otherwise we could
        // be reading stale status information
        call({this->CMD, "update-index", "--refresh"}, this->location);

        // Get the status of files that are different in the "index" vs
        // the HEAD of the git repository
        std::string result;
        _popen({this->CMD, "diff-index", "--cached", "HEAD", path}, this->location, result);
        std::vector<std::string> entries;
#if 0
        entries = proc.read().split("\n")[:-1];
#endif

        // Get the status of files that are different in the "index" vs
        // the files on disk
        _popen({this->CMD, "diff-files", "-0", path}, this->location, result);
#if 0
        entries += (proc.read().split("\n")[:-1]);
#endif

        // An unmerged file or a file that has been modified, added to
        // git's index, then modified again would result in the file
        // showing up in both the output of "diff-files" and
        // "diff-index".  The following command removes duplicate
        // file entries.
#if 0
        entries = list(set(entries));
#endif

        return entries;
    }

    /*! Update the state of the file(s) at tree_state['path'] */
    void _update_tree_state_cache(std::string path, std::map<std::string, FileState> tree_state) {
        std::vector<std::string> entries;
        std::vector<std::string> ignored_entries;
        std::vector<std::string> unversioned_entries;
        while (1) {
            entries = this->_get_modified_files(path);

            // Identify ignored files and folders
            std::string result;
            _popen({this->CMD, "ls-files", "--others", "--ignored", "--exclude-standard", "--directory", path}, this->location, result);
            boost::split(ignored_entries, result, boost::is_any_of("\n"));

            // Identify unversioned files
            _popen({this->CMD, "ls-files", "--others", "--exclude-standard", path}, this->location, result);
            boost::split(unversioned_entries, result, boost::is_any_of("\n"));

            break;
        }

        if (entries.empty() and boost::filesystem::is_regular(path)) {
            // If we're just updating a single file there's a chance that it
            // was it was previously modified, and now has been edited
            // so that it is un-modified.  This will result in an empty
            // 'entries' list, and tree_state['path'] will still contain stale
            // data.  When this corner case occurs we force tree_state['path']
            // to STATE_NORMAL.
#if 0
            path = os.path.abspath(path);
#endif
            tree_state[path] = STATE_NORMAL;
        } else {
            // There are 1 or more modified files, parse their state
            for (std::string entry : entries) {
                std::match_results<std::string::const_iterator> columns;
                std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
                std::string::const_iterator start = entry.begin();
                std::string::const_iterator end = entry.end();
                std::regex_search(start, end, columns, this->DIFF_RE, flags);
                std::string old_mode = columns[0];
                std::string new_mode = columns[1];
                std::string statekey = columns[2];
                std::string name = columns[3];
#ifdef _WIN32
                // Git returns unix-style paths on Windows
                name = os.path.normpath(name.strip());
#endif
#if 0
            path = os.path.abspath(path);
#endif
                boost::trim(name);
                path = this->root + "/" + name;
                boost::trim(statekey);
                FileState state;
                if (this->state_map.count(statekey) == 0) {
                    state = STATE_NONE;
                } else {
                    state = this->state_map[statekey];
                }
                tree_state[path] = state;
                if (old_mode != new_mode) {
                    boost::format fmt(_("Mode changed from %s to %s"));
                    fmt % old_mode.c_str() % new_mode.c_str();
                    std::string msg = fmt.str();
                    this->_tree_meta_cache[path] = msg;
                }
            }

            for (std::string entry : ignored_entries) {
                boost::trim(entry);
                std::string path = this->location + "/" + entry;
#if 0
                path = os.path.abspath(path);
#endif
                tree_state[path] = STATE_IGNORED;
            }

            for (std::string entry : unversioned_entries) {
                boost::trim(entry);
                std::string path = this->location + "/" + entry;
#if 0
                path = os.path.abspath(path);
#endif
                tree_state[path] = STATE_NONE;
            }
        }
    }

    std::map<std::string, FileState> _lookup_tree_cache(std::string rootdir) {
        // Get a list of all files in rootdir, as well as their status
        std::map<std::string, FileState> tree_state;
        this->_update_tree_state_cache("./", tree_state);
        return tree_state;
    }

    void update_file_state(std::string path) {
        std::map<std::string, FileState> tree_state = this->_get_tree_cache(boost::filesystem::path(path).parent_path().string());
        this->_update_tree_state_cache(path, tree_state);
    }

    virtual std::pair<std::vector<Dir*>, std::vector<File*>> _get_dirsandfiles(std::string directory, std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files) {

        std::map<std::string, FileState> tree = this->_get_tree_cache(directory);

        std::vector<File*> retfiles;
        std::vector<Dir*> retdirs;
        for (auto p : files) {
            std::string name = p.first;
            std::string path = p.second;
            FileState state;
            if (tree.count(path) == 0) {
                state = STATE_NORMAL;
            } else {
                state = tree[path];
            }
            std::string meta;
            if (this->_tree_meta_cache.count(path) == 0) {
                meta = "";
            } else {
                meta = this->_tree_meta_cache[path];
            }
            retfiles.push_back(new File(path, name, state, "", meta));
        }
        for (auto p : dirs) {
            std::string name = p.first;
            std::string path = p.second;
            FileState state;
            if (tree.count(path) == 0) {
                state = STATE_NORMAL;
            } else {
                state = tree[path];
            }
            retdirs.push_back(new Dir(path, name, state));
        }
        for (auto p : tree) {
            std::string path = p.first;
            FileState state = p.second;
            // removed files are not in the filesystem, so must be added here
            if (state == STATE_REMOVED || state == STATE_MISSING) {
                boost::filesystem::path p(path);
                std::string folder = p.parent_path().string();
                std::string name = p.filename().string();
                if (folder == directory) {
                    retfiles.push_back(new File(path, name, state));
                }
            }
        }
        return std::pair<std::vector<Dir*>, std::vector<File*>>(retdirs, retfiles);
    }
};
