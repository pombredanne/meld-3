/* Copyright (C) 2009-2013 Piotr Piastucki <the_leech@users.berlios.de>
 * Copyright (C) 2012-2013 Kai Willadsen <kai.willadsen@gmail.com>
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

#include <csignal>
#include <map>
#include "difflib/src/difflib.h"
#include "util/compat.h"

#include "matchers.h"

// Support re-execing on Windows
#ifdef _WIN32
    self_path = os.path.realpath(__file__);
    self_dir = os.path.abspath(os.path.dirname(self_path));
    sys.path[0:0] = [self_dir];
#endif

void init_worker() {
    signal(SIGINT, SIG_IGN);
}

difflib::chunk_list_t matcher_worker(std::string text1, std::string textn) {
    InlineMyersSequenceMatcher matcher(text1, textn, nullptr);
    return matcher.get_opcodes();
}


#if _MYERS_USES_VECTOR
static int find_common_prefix(std::vector<std::string> a, std::vector<std::string> b) {
    if (a.empty() or b.empty()) {
        return 0;
    }
    if (a[0] == b[0]) {
        int pointermax = std::min(a.size(), b.size());
        int pointermid = pointermax;
        int pointermin = 0;
        while (pointermin < pointermid) {
            std::vector<std::string>::iterator start = a.begin() + pointermin;
            std::vector<std::string>::iterator end = a.begin() + pointermid;
            std::vector<std::string> first(start, end);
            start = b.begin() + pointermin;
            end = b.begin() + pointermid;
            std::vector<std::string> second(start, end);
            if (first == second) {
                pointermin = pointermid;
            } else {
                pointermax = pointermid;
            }
            pointermid = int((pointermax - pointermin) / 2 + pointermin);
        }
        return pointermid;
    }
    return 0;
}
#endif

static int find_common_prefix(std::string a, std::string b) {
    if (a.empty() or b.empty()) {
        return 0;
    }
    if (a[0] == b[0]) {
        int pointermax = std::min(a.size(), b.size());
        int pointermid = pointermax;
        int pointermin = 0;
        while (pointermin < pointermid) {
            if (a.substr(pointermin, pointermid) == b.substr(pointermin, pointermid)) {
                pointermin = pointermid;
            } else {
                pointermax = pointermid;
            }
            pointermid = int((pointermax - pointermin) / 2 + pointermin);
        }
        return pointermid;
    }
    return 0;
}


static int find_common_suffix(std::string a, std::string b) {
    if (a.empty() or b.empty()) {
        return 0;
    }
    if (a[-1] == b[-1]) {
        int pointermax = std::min(a.length(), b.length());
        int pointermid = pointermax;
        int pointermin = 0;
        while (pointermin < pointermid) {
            std::string a_tail = a.substr(-pointermid,a.length() - pointermin);
            std::string b_tail = b.substr(-pointermid,b.length() - pointermin);
            if (a_tail == b_tail) {
                pointermin = pointermid;
            } else {
                pointermax = pointermid;
            }
            pointermid = int((pointermax - pointermin) / 2 + pointermin);
        }
        return pointermid;
    }
    return 0;
}

Snake::Snake(Snake *lastsnake, int x, int y, int snake) : lastsnake(lastsnake), x(x), y(y), snake(snake) {}

MyersSequenceMatcher::MyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk) : difflib::SequenceMatcher<std::string>(a, b, isjunk) {
    if (isjunk) {
        throw NotImplementedError("isjunk is not supported yet");
    }
    this->aindex.clear();
    this->bindex.clear();
    this->common_prefix = this->common_suffix = -1;
    this->lines_discarded = false;
}

difflib::match_list_t MyersSequenceMatcher::get_matching_blocks() {
    if (!this->matching_blocks_) {
        this->initialise();
    }
    return *this->matching_blocks_;
}

difflib::chunk_list_t MyersSequenceMatcher::get_difference_opcodes() {
    difflib::chunk_list_t result;
    for (difflib::chunk_t chunk : this->get_opcodes()) {
        if (std::get<0>(chunk) != "equal") {
            result.push_back(chunk);
        }
    }
    return result;
}

std::pair<std::string, std::string> MyersSequenceMatcher::preprocess_remove_prefix_suffix(std::string a, std::string b) {
    // remove common prefix and common suffix
    this->common_prefix = this->common_suffix = -1;
    this->common_prefix = find_common_prefix(a, b);
    if (this->common_prefix > 0) {
        a = a.substr(this->common_prefix);
        b = b.substr(this->common_prefix);
    }

    if (a.size() > 0 and b.size() > 0) {
        this->common_suffix = find_common_suffix(a, b);
        if (this->common_suffix > 0) {
            a = a.substr(0, a.length() - this->common_suffix);
            b = b.substr(0, b.length() - this->common_suffix);
        }
    }
    return std::pair<std::string, std::string>(a, b);
}

std::pair<std::string, std::vector<int>> MyersSequenceMatcher::index_matching(std::string a, std::string b) {
    std::set<char> aset;
    for (char s : a) {
        aset.insert(s);
    }
    std::string matches;
    std::vector<int> index;
    for (size_t i = 0; i < b.size(); i++) {
        char line = b[i];
        if (std::find(aset.begin(), aset.end(), line) != aset.end()) {
            matches.push_back(line);
            index.push_back(i);
        }
    }
    return std::pair<std::string, std::vector<int>>(matches, index);
}

std::pair<std::string, std::string> MyersSequenceMatcher::preprocess_discard_nonmatching_lines(std::string a, std::string b) {
    // discard lines that do not match any line from the other file
    if (a.empty() or b.empty()) {
        this->aindex.clear();
        this->bindex.clear();
        return std::pair<std::string, std::string>(a, b);
    }

    std::pair<std::string, std::vector<int>> tmp;
    tmp = index_matching(a, b);
    std::string indexed_b = tmp.first;
    this->bindex = tmp.second;
    tmp = index_matching(b, a);
    std::string indexed_a = tmp.first;
    this->aindex = tmp.second;

    // We only use the optimised result if it's worthwhile. The constant
    // represents a heuristic of how many lines constitute 'worthwhile'.
    this->lines_discarded = (b.size() - indexed_b.size() > 10 or
                            a.size() - indexed_a.size() > 10);
    if (this->lines_discarded) {
        a = indexed_a;
        b = indexed_b;
    }
    return std::pair<std::string, std::string>(a, b);
}

std::pair<std::string, std::string> MyersSequenceMatcher::preprocess() {
    std::pair<std::string, std::string> x = this->preprocess_remove_prefix_suffix(this->a_, this->b_);
    return this->preprocess_discard_nonmatching_lines(x.first, x.second);
}

void MyersSequenceMatcher::postprocess() {
    difflib::match_list_t mb;
    for (difflib::match_t t : *this->matching_blocks_) {
        mb.push_back(t);
    }
    int i = this->matching_blocks_->size() - 2;
    while (i >= 0) {
        difflib::match_t _m = (*this->matching_blocks_)[i];
        int cur_a = std::get<0>(_m);
        int cur_b = std::get<1>(_m);
        int cur_len = std::get<2>(_m);
        i -= 1;
        while (i >= 0) {
            difflib::match_t _m = (*this->matching_blocks_)[i];
            int prev_a = std::get<0>(_m);
            int prev_b = std::get<1>(_m);
            int prev_len = std::get<2>(_m);
            if (prev_b + prev_len == cur_b or prev_a + prev_len == cur_a) {
                std::string::const_iterator start = this->a_.begin() + (cur_a - prev_len);
                std::string::const_iterator end = this->a_.begin() + cur_a;
                std::string prev_slice_a(start, end);
                start = this->b_.begin() + (cur_b - prev_len);
                end = this->b_.begin() + cur_b;
                std::string prev_slice_b(start, end);
                if (prev_slice_a == prev_slice_b) {
                    cur_b -= prev_len;
                    cur_a -= prev_len;
                    cur_len += prev_len;
                    i -= 1;
                    continue;
                }
            }
            break;
        }
        mb.push_back(difflib::match_t(cur_a, cur_b, cur_len));
    }
    std::reverse(mb.begin(), mb.end());
    this->matching_blocks_->clear();
    for (difflib::match_t m : mb) {
        this->matching_blocks_->push_back(m);
    }
}

void MyersSequenceMatcher::build_matching_blocks(Snake* lastsnake) {
    std::deque<difflib::match_t> matching_blocks;

    int common_prefix = this->common_prefix;
    int common_suffix = this->common_suffix;
    std::vector<int> aindex = this->aindex;
    std::vector<int> bindex = this->bindex;
    while (lastsnake) {
        int x = lastsnake->x;
        int y = lastsnake->y;
        int snake = lastsnake->snake;
        lastsnake = lastsnake->lastsnake;
        if (this->lines_discarded) {
            // split snakes if needed because of discarded lines
            x += snake - 1;
            y += snake - 1;
            int xprev = aindex[x] + common_prefix;
            int yprev = bindex[y] + common_prefix;
            if (snake > 1) {
                int newsnake = 1;
                for (int i = 1; i < snake; i++) {
                    x -= 1;
                    y -= 1;
                    int xnext = aindex[x] + common_prefix;
                    int ynext = bindex[y] + common_prefix;
                    if ((xprev - xnext != 1) or (yprev - ynext != 1)) {
                        matching_blocks.push_front(difflib::match_t(xprev, yprev, newsnake));
                        newsnake = 0;
                    }
                    xprev = xnext;
                    yprev = ynext;
                    newsnake += 1;
                }
                matching_blocks.push_front(difflib::match_t(xprev, yprev, newsnake));
            } else {
                matching_blocks.push_front(difflib::match_t(xprev, yprev, snake));
            }
        } else {
            matching_blocks.push_front(difflib::match_t(x + common_prefix,
                                       y + common_prefix, snake));
        }
    }
    if (common_prefix) {
        matching_blocks.push_front(difflib::match_t(0, 0, common_prefix));
    }
    if (common_suffix) {
        matching_blocks.push_back(difflib::match_t(this->a_.size() - common_suffix,
                                this->b_.size() - common_suffix,
                                common_suffix));
    }
    matching_blocks.push_back(difflib::match_t(this->a_.size(), this->b_.size(), 0));
    this->matching_blocks_->clear();
    for (difflib::match_t m : matching_blocks) {
        this->matching_blocks_->push_back(m);
    }
    // clean-up to free memory
    this->aindex.clear();
    this->bindex.clear();
}

void MyersSequenceMatcher::initialise() {

    std::pair<std::string, std::string> tmp = this->preprocess();
    std::string a = tmp.first;
    std::string b = tmp.second;
    int m = a.size();
    int n = b.size();
    int middle = m + 1;
    Snake* lastsnake = 0;
    int delta = n - m + middle;
    int dmin = std::min(middle, delta);
    int dmax = std::max(middle, delta);
    if (n > 0 and m > 0) {
        int size = n + m + 2;
        std::vector<std::pair<int, Snake*>> fp;
        for (int i = 0; i < size; i++) {
            fp[i] = std::pair<int, Snake*>(-1, 0);
        }
        int p = -1;
        while (true) {
            p += 1;
            // move along vertical edge
            int yv = -1;
            Snake *node = 0;
            for (int km = dmin - p; km < delta; km++) {
                std::pair<int, Snake*> t = fp[km + 1];
                if (yv < t.first) {
                    yv = t.first;
                    node = t.second;
                } else {
                    yv += 1;
                }
                int x = yv - km + middle;
                if (x < m and yv < n and a[x] == b[yv]) {
                    int snake = x;
                    x += 1;
                    yv += 1;
                    while (x < m and yv < n and a[x] == b[yv]) {
                        x += 1;
                        yv += 1;
                    }
                    snake = x - snake;
                    node = new Snake(node, x - snake, yv - snake, snake);
                }
                fp[km] = std::pair<int, Snake*>(yv, node);
            }
            // move along horizontal edge
            int yh = -1;
            node = 0;
            for (int km = dmax + p; km > delta; km--) {
                std::pair<int, Snake*> t = fp[km - 1];
                if (yh <= t.first) {
                    yv = t.first;
                    node = t.second;
                    yh += 1;
                }
                int x = yh - km + middle;
                if (x < m and yh < n and a[x] == b[yh]) {
                    int snake = x;
                    x += 1;
                    yh += 1;
                    while (x < m and yh < n and a[x] == b[yh]) {
                        x += 1;
                        yh += 1;
                    }
                    snake = x - snake;
                    node = new Snake(node, x - snake, yh - snake, snake);
                }
                fp[km] = std::pair<int, Snake*>(yh, node);
            }
            // point on the diagonal that leads to the sink
            int y;
            if (yv < yh) {
                y = fp[delta + 1].first;
                node = fp[delta + 1].second;
            } else {
                y = fp[delta - 1].first;
                node = fp[delta - 1].second;
                y += 1;
            }
            int x = y - delta + middle;
            if (x < m and y < n and a[x] == b[y]) {
                int snake = x;
                x += 1;
                y += 1;
                while (x < m and y < n and a[x] == b[y]) {
                    x += 1;
                    y += 1;
                }
                snake = x - snake;
                node = new Snake(node, x - snake, y - snake, snake);
            }
            fp[delta] = std::pair<int, Snake *>(y, node);
            if (y >= n) {
                lastsnake = node;
                break;
            }
        }
    }
    this->build_matching_blocks(lastsnake);
    this->postprocess();
}


InlineMyersSequenceMatcher::InlineMyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk) : MyersSequenceMatcher(a, b, isjunk) {
}


std::pair<std::string, std::vector<int>> InlineMyersSequenceMatcher::index_matching_kmers(std::string a, std::string b) {
    std::set<std::string> aset;
    for (size_t i = 0; i < a.size() - 2; i++) {
        aset.insert(a.substr(i, i + 3));
    }
    std::string matches;
    std::vector<int> index;
    size_t next_poss_match = 0;
    // Start from where we can get a valid triple
    for (size_t i = 2; i < b.size(); i++) {
        if (std::find(aset.begin(), aset.end(), b.substr(i - 2, i + 1)) == aset.end()) {
            continue;
        }
        // Make sure we don't re-record matches from overlapping kmers
        for (size_t j = std::max(next_poss_match, i - 2); j < i + 1; j++) {
            matches.push_back(b[j]);
            index.push_back(j);
        }
        next_poss_match = i + 1;
    }
    return std::pair<std::string, std::vector<int>>(matches, index);
}


std::pair<std::string, std::string> InlineMyersSequenceMatcher::preprocess_discard_nonmatching_lines(std::string a, std::string b) {

    if (a.size() <= 2 and b.size() <= 2) {
        this->aindex.clear();
        this->bindex.clear();
        return std::pair<std::string, std::string>(a, b);
    }

    std::pair<std::string, std::vector<int>>  tmp;
    tmp = index_matching_kmers(a, b);
    std::string indexed_b = tmp.first;
    this->bindex = tmp.second;
    tmp = index_matching_kmers(b, a);
    std::string indexed_a = tmp.first;
    this->aindex = tmp.second;

    // We only use the optimised result if it's worthwhile. The constant
    // represents a heuristic of how many lines constitute 'worthwhile'.
    this->lines_discarded = (b.size() - indexed_b.size() > 10 or
                            a.size() - indexed_a.size() > 10);
    if (this->lines_discarded) {
        a = indexed_a;
        b = indexed_b;
    }
    return std::pair<std::string, std::string>(a, b);
}


SyncPointMyersSequenceMatcher::SyncPointMyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk, std::vector<std::pair<int, int>> *syncpoints) : MyersSequenceMatcher(a, b, isjunk) {
    this->syncpoints = syncpoints;
}

template <class T = std::string>
class Chunk {
public:
    int ai;
    int bi;
    T a;
    T b;
    Chunk(int ai, int bi, T a, T b) : ai(ai), bi(bi), a(a), b(b) {}
};

void SyncPointMyersSequenceMatcher::initialise() {
    if (!this->syncpoints or this->syncpoints->empty()) {
        MyersSequenceMatcher::initialise();
    } else {
        std::vector<Chunk<std::string>> chunks;
        size_t ai = 0;
        size_t bi = 0;
        for (std::pair<int, int> tmp : *this->syncpoints) {
            int aj = tmp.first;
            int bj = tmp.second;
            chunks.push_back(Chunk<std::string>(ai, bi, this->a_.substr(ai, aj-ai), this->b_.substr(bi, bj-bi)));
            ai = aj;
            bi = bj;
        }
        if (ai < this->a_.size() or bi < this->b_.size()) {
            chunks.push_back(Chunk<std::string>(ai, bi, this->a_.substr(ai), this->b_.substr(bi)));
        }

        this->split_matching_blocks.clear();
        this->matching_blocks_->clear();
        for (Chunk<std::string> c : chunks) {
            int ai = c.ai;
            int bi = c.bi;
            std::string a = c.a;
            std::string b = c.b;
            MyersSequenceMatcher matcher(a, b, this->is_junk_);
            matcher.initialise();
            difflib::match_list_t tmp = matcher.get_matching_blocks();
            std::deque<difflib::match_t> blocks = std::deque<difflib::match_t>(tmp.begin(), tmp.end());
            std::vector<difflib::match_t> matching_blocks(blocks.size());
            int l = matching_blocks.size() - 1;
            if (l >= 0 and blocks.size() > 1) {
                int aj = std::get<0>(matching_blocks[l]);
                int bj = std::get<1>(matching_blocks[l]);
                int bl = std::get<2>(matching_blocks[l]);
                if (aj + bl == ai and bj + bl == bi and
                        std::get<0>(blocks[0]) == 0 and std::get<1>(blocks[0]) == 0) {
                    difflib::match_t block = blocks.front();
                    blocks.pop_front();
                    matching_blocks[l] = difflib::match_t(aj, bj, bl + std::get<2>(block));
                }
            }
            for (size_t i = 0; i < blocks.size() - 1; i++) {
                difflib::match_t m = blocks[i];
                int x = std::get<0>(m);
                int y = std::get<1>(m);
                int l = std::get<2>(m);
                matching_blocks.push_back(difflib::match_t(ai + x, bi + y, l));
            }
            this->matching_blocks_->insert(this->matching_blocks_->end(), matching_blocks.begin(), matching_blocks.end());
#if 0
            // Split matching blocks each need to be terminated to get our
            // split chunks correctly created
            this->split_matching_blocks.append(
                matching_blocks + [(ai + a.length(), bi + b.length(), 0)]);
#endif
        }
        this->matching_blocks_->push_back(difflib::match_t(this->a_.size(), this->b_.size(), 0));
    }
}

difflib::chunk_list_t SyncPointMyersSequenceMatcher::get_opcodes() {
    // This is just difflib.SequenceMatcher.get_opcodes in which we instead
    // iterate over our internal set of split matching blocks.
    if (this->opcodes_) {
        return *this->opcodes_;
    }
    int i = 0;
    int j = 0;
    this->opcodes_.reset(new difflib::chunk_list_t);
    this->get_matching_blocks();
    for (difflib::match_list_t matching_blocks : this->split_matching_blocks) {
        for (difflib::match_t m : matching_blocks) {
            int ai = std::get<0>(m);
            int bj = std::get<1>(m);
            int size = std::get<2>(m);
            std::string tag = "";
            if (i < ai and j < bj) {
                tag = "replace";
            } else if (i < ai) {
                tag = "delete";
            } else if (j < bj) {
                tag = "insert";
            }
            if (!tag.empty()) {
                opcodes_->push_back(difflib::chunk_t(tag, i, ai, j, bj));
            }
            i = ai+size;
            j = bj+size;
            // the list of matching blocks is terminated by a
            // sentinel with size 0
            if (size) {
                opcodes_->push_back(difflib::chunk_t("equal", ai, i, bj, j));
            }
        }
    }
    return *opcodes_;
}
