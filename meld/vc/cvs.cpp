/* Copyright (C) 2002-2005 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include <boost/filesystem.hpp>

#if 0
import logging
import os
import re
import shutil
import tempfile
import time
#endif

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <sstream>

#include "../misc.h"
#include "_vc.h"

#if 0
log = logging.getLogger(__name__)
#endif


class FakeErrorStream {
    void error(int error) {
    }
};

#if 0
class CVS : public Vc {
    // CVSNT is a drop-in replacement for CVS; if found, it is used instead
    std::string ALT_CMD = "cvsnt";
    int PATCH_STRIP_NUM = 0;
    std::string PATCH_INDEX_RE = "^Index:(.*)$";


    CVS(std::string location) : Vc(location) {
        this->CMD = "cvs";
        this->NAME = "CVS";
        this->VC_DIR = "CVS";
        this->VC_ROOT_WALK = false;
        this->VC_COLUMNS = { DATA_NAME, DATA_STATE, DATA_REVISION, DATA_OPTIONS };
        if (not call({"which", this->ALT_CMD})) {
            this->CMD = this->ALT_CMD;
        }
    }

    virtual std::vector<std::string> commit_command(std::string message) {
        return {this->CMD, "commit", "-m", message};
    }

    bool supports_update_actions_for_paths() {
        return true;
    }

    virtual std::vector<std::string> update_command() {
        return {this->CMD, "update"};
    }

    virtual std::vector<std::string> remove_command(int force = 0) {
        return {this->CMD, "rm", "-f"};
    }

    virtual std::vector<std::string> revert_command() {
        return {this->CMD, "update", "-C"};
    }

    // TODO static
    bool valid_repo(std::string path) {
        return boost::filesystem::exists(path + "/" + VC_DIR + "/Entries");
    }

    virtual std::string get_path_for_repo_file(std::string path, std::string commit = "") {
        if (!commit) {
            throw NotImplementedError();
        }

        if (not path.startswith(this->root + os.path.sep)) {
            raise _vc.InvalidVCPath(self, path, "Path not in repository");
        }
        path = path[len(this->root) + 1:];

        diffiter = misc.read_pipe_iter([this->CMD, "diff", "-u", path],
                                       FakeErrorStream(), workdir=this->root);
        patch = None;
        while (!patch) {
            patch = next(diffiter);
        }
        status = next(diffiter);

        tmpdir = tempfile.mkdtemp("-meld");
        destfile = os.path.join(tmpdir, os.path.basename(path));

        try {
            shutil.copyfile(os.path.join(this->root, path), destfile);
        } catch (IOError &e) {
            // For missing files, create a new empty file
            open(destfile, "w").close();
        }

        patchcmd = ["patch", "-R", "-d", tmpdir];
        try {
            with (open(os.devnull, "w") as NULL) {
                result = misc.write_pipe(patchcmd, patch, error=NULL)
                assert result == 0;
            }

            with (open(destfile) as patched_file) {
                with (tempfile.NamedTemporaryFile(prefix='meld-tmp',
                                                 delete=False) as temp_file) {
                    shutil.copyfileobj(patched_file, temp_file);
                }
            }

            return temp_file.name;
        } catch (OSError, AssertionError) {
            return;
        } finally {
            if (boost::filesystem::exists(destfile)) {
                boost::filesystem::remove(destfile);
            }
            if (boost::filesystem::exists(tmpdir)) {
                boost::filesystem::remove(tmpdir);
            }
        }
    }

    void add(int runner, int files) {
        // CVS needs to add folders from their immediate parent
        dirs = [s for s in files if os.path.isdir(s)];
        files = [s for s in files if os.path.isfile(s)];
        std::vector<std::string> command = {this->CMD, "add"};
        for path in dirs:
            runner(command, [path], refresh=True,
                   working_dir=os.path.dirname(path))
        if (files) {
            runner(command, files, refresh=True);
        }
    }

    std::pair<std::vector<Dir*>, std::vector<File*>> _get_dirsandfiles(std::string directory, std::vector<std::pair<std::string, std::string>> dirs, std::vector<std::pair<std::string, std::string>> files) {
        std::string vc_path = directory + "/" + this->VC_DIR;


        std::ifstream entriesFile(vc_path + "/Entries");
        std::string entries;
        if (entriesFile.is_open()) {
            entries = std::string((std::istreambuf_iterator<char>(entriesFile)), std::istreambuf_iterator<char>());
            // poor mans universal newline
            boost::replace_all(entries, "\r", "\n");
            boost::replace_all(entries, "\n\n", "\n");
        // No CVS directory
        } else {
            std::vector<Dir*> d;
            for (std::pair<std::string, std::string> x : dirs) {
                d.push_back(new Dir(x.second, x.first, STATE_NONE));
            }
            std::vector<File*> f;
            for (std::pair<std::string, std::string> x : files) {
                f.push_back(new File(x.second, x.first, STATE_NONE));
            }
            return std::pair<std::vector<Dir*>, std::vector<File*>>(d, f);
        }
        entriesFile.close();

        std::ifstream entriesLogFile(vc_path + "/Entries.Log");
        if (entriesLogFile.is_open()) {
            std::string logentries((std::istreambuf_iterator<char>(entriesLogFile)), std::istreambuf_iterator<char>());

            std::regex re("^([AR])\\s*(.+)$(?m)");
            boost::match_flag_type flags = boost::match_default;
            std::string::const_iterator start = logentries.begin();
            std::string::const_iterator end = logentries.end();
            boost::sregex_token_iterator match(start, end, re, flags);

            std::vector<std::string> toadd;
            boost::sregex_token_iterator end1;
            for( ; match != end1; ++match ) {
                if ((*match)[0] == "A") {
                    toadd.append((*match)[1]);
                } else if ((*match)[0] == "R") {
                    std::remove(toadd.begin(), toadd.end(), (*match)[1]);
                } else {
                    log.warning("Unknown Entries.Log line '%s'", match[0]);
                }
            }
            entries += boost::join(toadd, "\n");
        }
        entriesLogFile.close();

        std::vector<File*> retfiles;
        std::vector<Dir*> retdirs;

        std::regex re("^(D?)/([^/]+)/(.+)$(?m)");
        boost::match_flag_type flags = boost::match_default;
        std::string::const_iterator start = entries.begin();
        std::string::const_iterator end = entries.end();
        boost::sregex_token_iterator match(start, end, re, flags);
#if 0
        matches.sort();
#endif

        boost::sregex_token_iterator end1;
        std::vector<std::string> cvsfiles;
        for( ; match != end1; ++match) {
            std::string isdir = (*match)[0];
            std::string name = (*match)[1];
            std::string path = directory + "/" + name;
            std::vector<std::string> tmp = boost::split((*match)[2], "/");
            std::string rev = tmp[0];
            std::string date = tmp[1];
            std::string options = tmp[2];
            std::string tag = tmp[3];
            if (isdir) {
                FileState state;
                if (boost::filesystem::exists(path)) {
                    state = STATE_NORMAL;
                } else {
                    state = STATE_MISSING;
                }
                retdirs.append(new Dir(path, name, state));
            } else {
                FileState state;
                if (rev.startswith("-")) {
                    state = STATE_REMOVED;
                } else if (date == "dummy timestamp") {
                    if (rev[0] == "0") {
                        state = STATE_NEW;
                    } else {
                        state = STATE_ERROR;
                    }
                } else if (date == "dummy timestamp from new-entry") {
                    state = STATE_MODIFIED;
                } else {
                    date_sub = lambda x: "%3i" % int(x.group());
                    date = re.sub(r"\s*\d+", date_sub, date, 1);
                    plus = date.find("+");
                    if (plus >= 0) {
                        state = STATE_CONFLICT;
                        try {
                            txt = open(path, "U").read();
                            if (txt.find("\n=======\n") == -1) {
                                state = STATE_MODIFIED;
                            }
                        } catch (IOError) {
                            // pass
                        }
                    } else {
                        try {
                            mtime = os.stat(path).st_mtime;
                            if (time.asctime(time.gmtime(mtime)) == date) {
                                state = _vc.STATE_NORMAL;
                            } else {
                                state = _vc.STATE_MODIFIED;
                            }
                        } catch (OSError &e) {
                            state = STATE_MISSING;
                        }
                    }
                }
                retfiles.append(new File(path, name, state, rev, options));
            }
            // known
            cvsfiles.push_back(date);
        }

        // ignored
        std::ifstream globalIgnoredFile(get_end("HOME") + "/.cvsignore");
        std::vector<std::string> ignored;
        if (globalIgnoredFile.is_open()) {
            ignored = boost::split(std::string((std::istreambuf_iterator<char>(globalIgnoredFile)), std::istreambuf_iterator<char>()));
        }
        globalIgnoredFile.close();

        std::ifstream ignoredFile(directory + "/.cvsignore");
        if (ignoredFile.is_open()) {
            ignored += boost::split(std::string((std::istreambuf_iterator<char>(ignoredFile)), std::istreambuf_iterator<char>()));
        }
        ignoredFile.close();

        std::regex ignore_re;
        if (!ignored.empty()) {
            try {
                std::vector<std::string> regexes;
                for (std::string i : ignored) {
                    regexes.push_back(shell_to_regex(i));
                }
                ignore_re.assign("(" + boost::join(regexes, "|") + ")");
            } catch (boost::bad_expression &e) {
#if 0
                log.warning(
                    "Error converting %s to a regular expression: %s'" %
                    (",".join(ignored), err))
#endif
                // Matches nothing
                ignore_re.assign("a^");
            }
        } else {
            // Matches nothing
            ignore_re.assign("a^");
        }

        for (std::pair<std::string, std::string> tmp : files) {
            std::string f = tmp.first;
            std::string path = tmp.second;
            if (std::find(cvsfiles.begin(), cvsfiles.end(), f) == cvsfiles.end()) {
                state = (ignore_re.match(f) is None and _vc.STATE_NONE or
                         _vc.STATE_IGNORED);
                retfiles.append(_vc.File(path, f, state));
            }
        }
        for (std::pair<std::string, std::string> tmp : dirs) {
            std::string d = tmp.first;
            std::string path = tmp.second;
            if (std::find(cvsfiles.begin(), cvsfiles.end(), d) == cvsfiles.end()) {
                state = (ignore_re.match(d) is None and _vc.STATE_NONE or
                         _vc.STATE_IGNORED);
                retdirs.append(_vc.Dir(path, d, state));
            }
        }

        return std::pair<std::vector<Dir*>, std::vector<File*>>(retdirs, retfiles);
    }
}
#endif
