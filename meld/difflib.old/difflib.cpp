/*!
Module difflib -- helpers for computing deltas between objects.

Function get_close_matches(word, possibilities, n=3, cutoff=0.6):
    Use SequenceMatcher to return list of the best "good enough" matches.

Function context_diff(a, b):
    For two lists of strings, return a delta in context diff format.

Function ndiff(a, b):
    Return a delta: the difference between `a` and `b` (lists of strings).

Function restore(delta, which):
    Return one of the two sequences that generated an ndiff delta.

Function unified_diff(a, b):
    For two lists of strings, return a delta in unified diff format.

Class SequenceMatcher:
    A flexible class for comparing pairs of sequences of any type.

Class Differ:
    For producing human-readable deltas from sequences of lines of text.

Class HtmlDiff:
    For producing HTML side by side comparison with change highlights.
*/

#include <algorithm>
#include <queue>
#include <sstream>
#include <cassert>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "difflib.h"

Match::Match(int a, int b, int size) : a(a), b(b), size(size) {}

static double _calculate_ratio(int matches, int length) {
    if (length) {
        return 2.0 * matches / length;
    }
    return 1.0;
}

DiffChunk::DiffChunk(std::string tag, int start_a, int end_a, int start_b, int end_b) : tag(tag), start_a(start_a), end_a(end_a), start_b(start_b), end_b(end_b) {}

SequenceMatcher::SequenceMatcher(std::function<bool(std::string)> isjunk, std::vector<std::string> a, std::vector<std::string> b, bool autojunk) {

    this->isjunk = isjunk;
    this->a = {""};
    this->b = {""};
    this->autojunk = autojunk;
    this->set_seqs(a, b);

}

SequenceMatcher::~SequenceMatcher() {
    if (this->b2j) {
        for (std::pair<std::string, std::vector<int>*> p : *this->b2j) {
            delete p.second;
            p.second = 0;
        }
    }
    delete this->b2j;
    delete this->fullbcount;
    delete this->matching_blocks;
    delete this->opcodes;
    delete this->bjunk;
    delete this->bpopular;
}

void SequenceMatcher::set_seqs(std::vector<std::string> a, std::vector<std::string> b) {

    this->set_seq1(a);
    this->set_seq2(b);

}

void SequenceMatcher::set_seq1(std::vector<std::string> a) {

    if (a == this->a) {
        return;
    }
    this->a = a;
    this->matching_blocks = 0;
    this->opcodes = 0;
}

void SequenceMatcher::set_seq2(std::vector<std::string> b) {

    if (b == this->b) {
        return;
    }
    this->b = b;
    this->matching_blocks = 0;
    this->opcodes = 0;
    this->fullbcount = 0;
    this->__chain_b();
}

// For each element x in b, set b2j[x] to a list of the indices in
// b where x appears; the indices are in increasing order; note that
// the number of times x appears in b is len(b2j[x]) ...
// when this->isjunk is defined, junk elements don't show up in this
// map at all, which stops the central find_longest_match method
// from starting any matching block at a junk element ...
// b2j also does not contain entries for "popular" elements, meaning
// elements that account for more than 1 + 1% of the total elements, and
// when the sequence is reasonably large (>= 200 elements); this can
// be viewed as an adaptive notion of semi-junk, and yields an enormous
// speedup when, e.g., comparing program files with hundreds of
// instances of "return NULL;" ...
// note that this is only called when b changes; so for cross-product
// kinds of matches, it's best to call set_seq2 once, then set_seq1
// repeatedly

void SequenceMatcher::__chain_b() {
    // Because isjunk is a user-defined (not C) function, and we test
    // for junk a LOT, it's important to minimize the number of calls.
    // Before the tricks described here, __chain_b was by far the most
    // time-consuming routine in the whole module!  If anyone sees
    // Jim Roskind, thank him again for profile.py -- I never would
    // have guessed that.
    // The first trick is to build b2j ignoring the possibility
    // of junk.  I.e., we don't call isjunk at all yet.  Throwing
    // out the junk later is much cheaper than building b2j "right"
    // from the start.
    std::vector<std::string> b = this->b;
    std::map<std::string, std::vector<int>*> *b2j = new std::map<std::string, std::vector<int>*>;
    this->b2j = b2j;

    for (size_t i = 0; i < b.size(); i++) {
        std::string elt = b[i];
        if (b2j->count(elt) == 0) {
            (*b2j)[elt] = new std::vector<int>;
        }
        std::vector<int>* indices = (*b2j)[elt];
        indices->push_back(i);
    }

    // Purge junk elements
    std::set<std::string>* junk = new std::set<std::string>;
    this->bjunk = junk;
    std::function<bool(std::string)> isjunk = this->isjunk;
    if (isjunk) {
        for (std::pair<std::string, std::vector<int>*> p : *b2j) {
            std::string elt = p.first;
            if (isjunk(elt)) {
                junk->insert(elt);
            }
        }
        for (std::string elt : *junk) { // separate loop avoids separate list of keys
            b2j->erase(elt);
        }
    }


    // Purge popular elements that are not junk
    std::set<std::string>* popular = new std::set<std::string>;
    this->bpopular = popular;
    unsigned int n = b.size();
    if (this->autojunk and n >= 200) {
        unsigned int ntest = n; // 100 + 1
        for (std::pair<std::string, std::vector<int>*> p : *b2j) {
            std::string elt = p.first;
            std::vector<int>* idxs = p.second;
            if (idxs->size() > ntest) {
                popular->insert(elt);
            }
        }
        for (std::string elt : *popular) { // ditto; as fast for 1% deletion
            b2j->erase(elt);
        }
    }

}

/*! Deprecated; use 'item in SequenceMatcher().bjunk'. */
bool SequenceMatcher::isbjunk(std::string item) {
#if 0
    warnings.warn("'SequenceMatcher().isbjunk(item)' is deprecated;\n"
                  "use 'item in SMinstance.bjunk' instead.",
                  DeprecationWarning, 2)
#endif
    return std::find(this->bjunk->begin(), this->bjunk->end(), item) != this->bjunk->end();
}

/*! Deprecated; use 'item in SequenceMatcher().bpopular'. */
bool SequenceMatcher::isbpopular(std::string item) {
#if 0
    warnings.warn("'SequenceMatcher().isbpopular(item)' is deprecated;\n"
                  "use 'item in SMinstance.bpopular' instead.",
                  DeprecationWarning, 2)
#endif
    return std::find(this->bpopular->begin(), this->bpopular->end(), item) != this->bpopular->end();
}

Match SequenceMatcher::find_longest_match(int alo, int ahi, int blo, int bhi) {

    // CAUTION:  stripping common prefix or suffix would be incorrect.
    // E.g.,
    //    ab
    //    acab
    // Longest matching block is "ab", but if common prefix is
    // stripped, it's "a" (tied with "b").  UNIX(tm) diff does so
    // strip, so ends up claiming that ab is changed to acab by
    // inserting "ca" in the middle.  That's minimal but unintuitive:
    // "it's obvious" that someone inserted "ac" at the front.
    // Windiff ends up at the same place as diff, but by pairing up
    // the unique 'b's and then matching the first two 'a's.

    std::vector<std::string> a = this->a;
    std::vector<std::string> b = this->b;
    std::map<std::string, std::vector<int>*>* b2j = this->b2j;
    int besti = alo;
    int bestj = blo;
    int bestsize = 0;
    // find longest junk-free match
    // during an iteration of the loop, j2len[j] = length of longest
    // junk-free match ending with a[i-1] and b[j]
    std::map<int, int> j2len;
    std::vector<int> nothing;
    for (int i = alo; i < ahi; i++) {
        // look at all instances of a[i] in b; note that because
        // b2j has no junk keys, the loop is skipped if a[i] is junk
        std::map<int, int> newj2len;
        std::vector<int> v;
        if (b2j->count(a[i])) {
            v = *(*b2j)[a[i]];
        } else {
            v = nothing;
        }
        for (int j : v) {
            // a[i] matches b[j]
            if (j < blo) {
                continue;
            }
            if (j >= bhi) {
                break;
            }
            int c;
            if (j2len.count(j-1)) {
                c = 1;
            } else {
                c = j2len[j-1] + 1;
            }
            int k = newj2len[j] = c;
            if (k > bestsize) {
                besti = i-k+1;
                bestj = j-k+1;
                bestsize = k;
            }
        }
        j2len = newj2len;
    }

    // Extend the best by non-junk elements on each end.  In particular,
    // "popular" non-junk elements aren't in b2j, which greatly speeds
    // the inner loop above, but also means "the best" match so far
    // doesn't contain any junk *or* popular non-junk elements.
    while (besti > alo and bestj > blo and
           std::find(this->bjunk->begin(), this->bjunk->end(), b[bestj-1]) == this->bjunk->end() and
           a[besti-1] == b[bestj-1]) {
        besti = besti-1;
        bestj = bestj-1;
        bestsize = bestsize+1;
    }
    while (besti+bestsize < ahi and bestj+bestsize < bhi and
           std::find(this->bjunk->begin(), this->bjunk->end(), b[bestj+bestsize]) == this->bjunk->end() and
           a[besti+bestsize] == b[bestj+bestsize]) {
        bestsize += 1;
    }

    // Now that we have a wholly interesting match (albeit possibly
    // empty!), we may as well suck up the matching junk on each
    // side of it too.  Can't think of a good reason not to, and it
    // saves post-processing the (possibly considerable) expense of
    // figuring out what to do with it.  In the case of an empty
    // interesting match, this is clearly the right thing to do,
    // because no other kind of match is possible in the regions.
    while (besti > alo and bestj > blo and
           std::find(this->bjunk->begin(), this->bjunk->end(), b[bestj-1]) != this->bjunk->end() and
           a[besti-1] == b[bestj-1]) {
        besti = besti-1;
        bestj = bestj-1;
        bestsize = bestsize+1;
    }
    while (besti+bestsize < ahi and bestj+bestsize < bhi and
           std::find(this->bjunk->begin(), this->bjunk->end(), b[bestj+bestsize]) != this->bjunk->end() and
           a[besti+bestsize] == b[bestj+bestsize]) {
        bestsize = bestsize + 1;
    }

    return Match(besti, bestj, bestsize);
}

std::deque<Match>* SequenceMatcher::get_matching_blocks() {

    if (this->matching_blocks) {
        return this->matching_blocks;
    }
    int la = this->a.size();
    int lb = this->b.size();

    // This is most naturally expressed as a recursive algorithm, but
    // at least one user bumped into extreme use cases that exceeded
    // the recursion limit on their box.  So, now we maintain a list
    // ('queue`) of blocks we still need to look at, and append partial
    // results to `matching_blocks` in a loop; the matches are sorted
    // at the end.
    std::queue<std::tuple<int, int, int, int>> queue;
    queue.push(std::tuple<int, int, int, int>(0, la, 0, lb));
    std::vector<Match> matching_blocks;
    while (!queue.empty()) {
        std::tuple<int, int, int, int> tmp = queue.front();
        queue.pop();
        int alo = std::get<0>(tmp);
        int ahi = std::get<1>(tmp);
        int blo = std::get<2>(tmp);
        int bhi = std::get<3>(tmp);
        Match x = this->find_longest_match(alo, ahi, blo, bhi);
        int i = x.a;
        int j = x.b;
        int k = x.size;
        // a[alo:i] vs b[blo:j] unknown
        // a[i:i+k] same as b[j:j+k]
        // a[i+k:ahi] vs b[j+k:bhi] unknown
        if (k) {   // if k is 0, there was no matching block
            matching_blocks.push_back(x);
            if (alo < i and blo < j) {
                queue.push(std::tuple<int, int, int, int>(alo, i, blo, j));
            }
            if (i+k < ahi and j+k < bhi) {
                queue.push(std::tuple<int, int, int, int>(i+k, ahi, j+k, bhi));
            }
        }
    }
    std::sort(matching_blocks.begin(), matching_blocks.end(), [] (Match _a, Match _b) { return _a.a < _b.a; });

    // It's possible that we have adjacent equal blocks in the
    // matching_blocks list now.  Starting with 2.5, this code was added
    // to collapse them.
    int i1 = 0;
    int j1 = 0;
    int k1 = 0;
    std::deque<Match>* non_adjacent = new std::deque<Match>;
    for (Match _m : matching_blocks) {
        int i2 = _m.a;
        int j2 = _m.b;
        int k2 = _m.size;
        // Is this block adjacent to i1, j1, k1?
        if (i1 + k1 == i2 and j1 + k1 == j2) {
            // Yes, so collapse them -- this just increases the length of
            // the first block by the length of the second, and the first
            // block so lengthened remains the block to compare against.
            k1 += k2;
        } else {
            // Not adjacent.  Remember the first block (k1==0 means it's
            // the dummy we started with), and make the second block the
            // new block to compare against.
            if (k1) {
                non_adjacent->push_back(Match(i1, j1, k1));
            }
            i1 = i2;
            j1 = j2;
            k1 = k2;
        }
    }
    if (k1) {
        non_adjacent->push_back(Match(i1, j1, k1));
    }

    non_adjacent->push_back(Match(la, lb, 0));
    this->matching_blocks = non_adjacent;
    return this->matching_blocks;
}

std::vector<DiffChunk>* SequenceMatcher::get_opcodes() {

    if (this->opcodes) {
        return this->opcodes;
    }
    int i = 0;
    int j = 0;
    std::vector<DiffChunk>* answer = new std::vector<DiffChunk>;
    this->opcodes = answer;
    for (Match _m : *this->get_matching_blocks()) {
        int ai = _m.a;
        int bj = _m.b;
        int size = _m.size;
        // invariant:  we've pumped out correct diffs to change
        // a[:i] into b[:j], and the next matching block is
        // a[ai:ai+size] == b[bj:bj+size].  So we need to pump
        // out a diff to change a[i:ai] into b[j:bj], pump out
        // the matching block, and move (i,j) beyond the match
        std::string tag = "";
        if (i < ai and j < bj) {
            tag = "replace";
        } else if (i < ai) {
            tag = "delete";
        } else if (j < bj) {
            tag = "insert";
        }
        if (!tag.empty()) {
            answer->push_back(DiffChunk(tag, i, ai, j, bj));
        }
        i = ai+size;
        j = bj+size;
        // the list of matching blocks is terminated by a
        // sentinel with size 0
        if (size) {
            answer->push_back(DiffChunk("equal", ai, i, bj, j));
        }
    }

    return answer;
}

std::vector<std::vector<DiffChunk>> SequenceMatcher::get_grouped_opcodes(int n) {

    std::vector<DiffChunk> codes = *this->get_opcodes();
    if (codes.empty()) {
        codes = {DiffChunk("equal", 0, 1, 0, 1)};
    }
    // Fixup leading and trailing groups if they show no changes.
    if (codes[0].tag == "equal") {
        std::string tag = codes[0].tag;
        int i1 = codes[0].start_a;
        int i2 = codes[0].end_a;
        int j1 = codes[0].start_b;
        int j2 = codes[0].end_b;
        codes[0] = DiffChunk(tag, std::max(i1, i2-n), i2, std::max(j1, j2-n), j2);
    }
    if (codes.back().tag == "equal") {
        std::string tag = codes.back().tag;
        int i1 = codes.back().start_a;
        int i2 = codes.back().end_a;
        int j1 = codes.back().start_b;
        int j2 = codes.back().end_b;
        codes[codes.size() - 1] = DiffChunk(tag, i1, std::min(i2, i1+n), j1, std::min(j2, j1+n));
    }

    int nn = n + n;
    std::vector<std::vector<DiffChunk>> result;
    std::vector<DiffChunk> group;
    for (DiffChunk _c : codes) {
        std::string tag = _c.tag;
        int i1 = _c.start_a;
        int i2 = _c.end_a;
        int j1 = _c.start_b;
        int j2 = _c.end_b;
        // End the current group and start a new one whenever
        // there is a large range with no changes.
        if (tag == "equal" and i2-i1 > nn) {
            group.push_back(DiffChunk(tag, i1, std::min(i2, i1+n), j1, std::min(j2, j1+n)));
            result.push_back(group);
            group = std::vector<DiffChunk>();
            i1 = std::max(i1, i2-n);
            j1 = std::max(j1, j2-n);
        }
        group.push_back(DiffChunk(tag, i1, i2, j1 ,j2));
    }
    if (!group.empty() and not (group.size()==1 and group[0].tag == "equal")) {
        result.push_back(group);
    }

    return result;
}

double SequenceMatcher::ratio() {
    std::deque<Match> m = *this->get_matching_blocks();
    int matches = 0;
    for (Match triple : m) {
        matches += triple.size;
    }
    return _calculate_ratio(matches, this->a.size() + this->b.size());
}

double SequenceMatcher::quick_ratio() {

    // viewing a and b as multisets, set matches to the cardinality
    // of their intersection; this counts the number of matches
    // without regard to order, so is clearly an upper bound
    std::map<std::string, int>* fullbcount;
    if (!this->fullbcount) {
        fullbcount = new std::map<std::string, int>;
        this->fullbcount = fullbcount;
        for (std::string elt : this->b) {
            if (fullbcount->count(elt) > 0) {
                (*fullbcount)[elt] = (*fullbcount)[elt] + 1;
            } else {
                (*fullbcount)[elt] = 0;
            }
        }
    }
    fullbcount = this->fullbcount;
    // avail[x] is the number of times x appears in 'b' less the
    // number of times we've seen it in 'a' so far ... kinda
    std::map<std::string, int> avail;
    int matches = 0;
    for (std::string elt : this->a) {
        int numb;
        if (avail.count(elt)) {
            numb = avail[elt];
        } else {
            if (fullbcount->count(elt) > 0) {
                numb = (*fullbcount)[elt];
            } else {
                numb = 0;
            }
        }
        avail[elt] = numb - 1;
        if (numb > 0) {
            matches = matches + 1;
        }
    }
    return _calculate_ratio(matches, this->a.size() + this->b.size());
}

double SequenceMatcher::real_quick_ratio() {

    int la = this->a.size();
    int lb = this->b.size();
    // can't have more matches than the number of elements in the
    // shorter sequence
    return _calculate_ratio(std::min(la, lb), la + lb);
}

std::vector<std::string> get_close_matches(std::vector<std::string> word, std::vector<std::string> possibilities, int n, double cutoff) {

    if (not n >  0) {
        std::stringstream ss;
        ss << "n must be > 0: " << n;
        throw ValueError(ss.str());
    }
    if (not (0.0 <= cutoff && cutoff <= 1.0)) {
        std::stringstream ss;
        ss << "cutoff must be in [0.0, 1.0]: " << cutoff;
        throw ValueError(ss.str());
    }
    // Priority queue to move the best scorers to head of list
    std::priority_queue<std::pair<double, std::string>> result;
    SequenceMatcher s;
    s.set_seq2(word);
    for (std::string x : possibilities) {
        s.set_seq1({x});
        if (s.real_quick_ratio() >= cutoff and
           s.quick_ratio() >= cutoff and
           s.ratio() >= cutoff) {
            result.push(std::pair<double, std::string>(s.ratio(), x));
        }
    }

    // Strip scores for the best n matches
    std::vector<std::string> retval;
    while (!result.empty()) {
        std::pair<double, std::string> pair = result.top();
        retval.push_back(pair.second);
        result.pop();
    }

    return retval;
}

/*!
    Return number of `ch` characters at the start of `line`.

    Example:

    >>> _count_leading("   abc", " ")
    3
 */
static int _count_leading(std::string line, char ch) {

    int i = 0;
    int n = line.length();
    while (i < n and line[i] == ch) {
        i += 1;
    }
    return i;
}

Differ::Differ(std::function<bool(std::string)> linejunk, std::function<bool(std::string)> charjunk) {

    this->linejunk = linejunk;
    this->charjunk = charjunk;

}

std::vector<std::string> Differ::compare(std::vector<std::string> a, std::vector<std::string> b) {

    std::vector<std::string> result;

    SequenceMatcher cruncher(this->linejunk, a, b);
    std::vector<DiffChunk> m = *cruncher.get_opcodes();
    for (DiffChunk _d : m) {
        std::string tag = _d.tag;
        int alo = _d.start_a;
        int ahi = _d.end_a;
        int blo = _d.start_b;
        int bhi = _d.end_b;
        std::vector<std::string> g;
        if (tag == "replace") {
            g = this->_fancy_replace(a, alo, ahi, b, blo, bhi);
        } else if (tag == "delete") {
            g = this->_dump('-', a, alo, ahi);
        } else if (tag == "insert") {
            g = this->_dump('+', b, blo, bhi);
        } else if (tag == "equal") {
            g = this->_dump(' ', a, alo, ahi);
        } else {
            std::stringstream ss;
            ss << "unknown tag " << tag;
            throw ValueError(ss.str());
        }

        for (std::string line : g) {
            result.push_back(line);
        }
    }

    return result;
}

std::vector<std::string> Differ::_dump(char tag, std::vector<std::string> x, int lo, int hi) {
    std::vector<std::string> result;
    for (int i = lo; i < hi; i++) {
        result.push_back(tag + " " + x[i]);
    }
    return result;
}

std::vector<std::string> Differ::_plain_replace(std::vector<std::string> a, int alo, int ahi, std::vector<std::string> b, int blo, int bhi) {
    assert(alo < ahi and blo < bhi);

    std::vector<std::string> result;

    // dump the shorter block first -- reduces the burden on short-term
    // memory if the blocks are of very different sizes
    std::vector<std::string> first;
    std::vector<std::string> second;
    if (bhi - blo < ahi - alo) {
        first  = this->_dump('+', b, blo, bhi);
        second = this->_dump('-', a, alo, ahi);
    } else {
        first  = this->_dump('-', a, alo, ahi);
        second = this->_dump('+', b, blo, bhi);
    }

    for (std::string line : first) {
        result.push_back(line);
    }
    for (std::string line : second) {
        result.push_back(line);
    }

    return result;
}

std::vector<std::string> Differ::_fancy_replace(std::vector<std::string> a, int alo, int ahi, std::vector<std::string> b, int blo, int bhi) {

    std::vector<std::string> result;

    // don't synch up unless the lines have a similarity score of at
    // least cutoff; best_ratio tracks the best score seen so far
    double best_ratio = 0.74;
    double cutoff = 0.75;
    SequenceMatcher cruncher(this->charjunk);
    int eqi = -1;
    int eqj = -1;   // 1st indices of equal lines (if any)

    int best_i;
    int best_j;

    // search for the pair that matches best without being identical
    // (identical lines must be junk lines, & we don't want to synch up
    // on junk -- unless we have to)
    for (int j = blo; j < bhi; j++) {
        std::string bj = b[j];
        cruncher.set_seq2({bj});
        for (int i = alo; i < ahi; i++) {
            std::string ai = "" + a[i];
            if (ai == bj) {
                if (eqi < 0) {
                    eqi = i;
                    eqj = j;
                }
                continue;
            }
            cruncher.set_seq1({ai});
            // computing similarity is expensive, so use the quick
            // upper bounds first -- have seen this speed up messy
            // compares by a factor of 3.
            // note that ratio() is only expensive to compute the first
            // time it's called on a sequence pair; the expensive part
            // of the computation is cached by cruncher
            if (cruncher.real_quick_ratio() > best_ratio and
                  cruncher.quick_ratio() > best_ratio and
                  cruncher.ratio() > best_ratio) {
                best_ratio = cruncher.ratio();
                best_i = i;
                best_j = j;
            }
        }
    }
    if (best_ratio < cutoff) {
        // no non-identical "pretty close" pair
        if (eqi < 0) {
            // no identical pair either -- treat it as a straight replace
            for (std::string line : this->_plain_replace(a, alo, ahi, b, blo, bhi)) {
                result.push_back(line);
            }
            return result;
        }
        // no close pair, but an identical pair -- synch up on that
        best_i = eqi;
        best_j = eqj;
        best_ratio = 1.0;
    } else {
        // there's a close pair, so forget the identical pair (if any)
        eqi = -1;
    }

    // a[best_i] very similar to b[best_j]; eqi is None iff they're not
    // identical

    // pump out diffs from before the synch point
    for (std::string line : this->_fancy_helper(a, alo, best_i, b, blo, best_j)) {
        result.push_back(line);
    }

    // do intraline marking on the synch pair
    std::string aelt = a[best_i];
    std::string belt = b[best_j];
    if (eqi < 0) {
        // pump out a '-', '?', '+', '?' quad for the synched lines
        std::string atags = "";
        std::string btags = "";
        cruncher.set_seqs({aelt}, {belt});
        std::vector<DiffChunk> __d = *cruncher.get_opcodes();
        for (DiffChunk _d : __d) {
            std::string tag = _d.tag;
            int ai1 = _d.start_a;
            int ai2 = _d.end_a;
            int bj1 = _d.start_b;
            int bj2 = _d.end_b;
            int la = ai2 - ai1;
            int lb = bj2 - bj1;
            if (tag == "replace") {
                for (int i = 0; i < la; i++) {
                    atags += "^";
                }
                for (int i = 0; i < lb; i++) {
                    btags += "^";
                }
            } else if (tag == "delete") {
                for (int i = 0; i < la; i++) {
                    atags += "-";
                }
            } else if (tag == "insert") {
                for (int i = 0; i < lb; i++) {
                    btags += "+";
                }
            } else if (tag == "equal") {
                for (int i = 0; i < la; i++) {
                    atags += " ";
                }
                for (int i = 0; i < lb; i++) {
                    btags += " ";
                }
            } else {
                std::stringstream ss;
                ss << "unknown tag " << tag;
                throw ValueError(ss.str());
            }
        }
        for (std::string line : this->_qformat(aelt, belt, atags, btags)) {
            result.push_back(line);
        }
    } else {
        // the synch pair is identical
        result.push_back("  " + aelt);
    }

    // pump out diffs from after the synch point
    for (std::string line : this->_fancy_helper(a, best_i+1, ahi, b, best_j+1, bhi)) {
        result.push_back(line);
    }

    return result;
}

std::vector<std::string> Differ::_fancy_helper(std::vector<std::string> a, int alo, int ahi, std::vector<std::string> b, int blo, int bhi) {
    std::vector<std::string> g;
    if (alo < ahi) {
        if (blo < bhi) {
            g = this->_fancy_replace(a, alo, ahi, b, blo, bhi);
        } else {
            g = this->_dump('-', a, alo, ahi);
        }
    } else if (blo < bhi) {
        g = this->_dump('+', b, blo, bhi);
    }

    std::vector<std::string> result;
    for (std::string line : g) {
        result.push_back(line);
    }
    return result;
}

std::vector<std::string> Differ::_qformat(std::string aline, std::string bline, std::string atags, std::string btags) {

    std::vector<std::string> result;

    // Can hurt, but will probably help most of the time.
    int common = std::min(_count_leading(aline, '\t'),
                          _count_leading(bline, '\t'));
    common = std::min(common, _count_leading(atags.substr(0, common), ' '));
    common = std::min(common, _count_leading(btags.substr(0, common), ' '));
    atags = atags.substr(common);
    boost::algorithm::trim_right(atags);
    btags = btags.substr(common);
    boost::algorithm::trim_right(btags);

    result.push_back("- " + aline);
    if (!atags.empty()) {
        std::stringstream ss;
        ss << "? ";
        for (int i = 0; i < common; i++) {
            ss << '\t';
        }
        ss << atags << "\n";
        result.push_back(ss.str());
    }

    result.push_back("+ " + bline);
    if (!btags.empty()) {
        std::stringstream ss;
        ss << "? ";
        for (int i = 0; i < common; i++) {
            ss << '\t';
        }
        ss << btags << "\n";
        result.push_back(ss.str());
    }

    return result;
}

// With respect to junk, an earlier version of ndiff simply refused to
// *start* a match with a junk element.  The result was cases like this:
//     before: private Thread currentThread;
//     after:  private volatile Thread currentThread;
// If you consider whitespace to be junk, the longest contiguous match
// not starting with junk is "e Thread currentThread".  So ndiff reported
// that "e volatil" was inserted between the 't' and the 'e' in "private".
// While an accurate view, to people that's absurd.  The current version
// looks for matching blocks that are entirely junk-free, then extends the
// longest one of those as far as possible but only with matching junk.
// So now "currentThread" is matched, then extended to suck up the
// preceding blank; then "private" is matched, and extended to suck up the
// following blank; then "Thread" is matched; and finally ndiff reports
// that "volatile " was inserted before "Thread".  The only quibble
// remaining is that perhaps it was really the case that " volatile"
// was inserted after "private".  I can live with that <wink>.

boost::regex IS_LINE_JUNK_default("\\s*#?\\s*$");

bool IS_LINE_JUNK(std::string line, boost::regex& pat) {

    boost::match_results<std::string::const_iterator> results;
    return boost::regex_match(line, results, pat);

}

bool IS_CHARACTER_JUNK(std::string ch) {

    return boost::algorithm::contains(" \t", ch);

}

////////////////////////////////////////////////////////////////////////
///  Unified Diff
////////////////////////////////////////////////////////////////////////

/*! Convert range to the "ed" format */
std::string _format_range_unified(int start, int stop) {
    // Per the diff spec at http://www.unix.org/single_unix_specification/
    int beginning = start + 1;     // lines start numbering with one
    int length = stop - start;
    if (length == 1) {
        return std::to_string(beginning);
    }
    if (not length) {
        beginning -= 1;        // empty ranges begin at line just before the range
    }
    return std::to_string(beginning) + "," +  std::to_string(length);
}

std::vector<std::string> unified_diff(std::string a, std::string b, std::string fromfile, std::string tofile, std::string fromfiledate,
                                      std::string tofiledate, int n, char lineterm) {

    std::vector<std::string> result;

    bool started = false;
    for (std::vector<DiffChunk> group : SequenceMatcher(0, {a}, {b}).get_grouped_opcodes(n)) {
        if (not started) {
            started = true;
            std::string fromdate = "";
            if (!fromfiledate.empty()) {
                fromdate = "\t" + fromfiledate;
            }
            std::string todate = "";
            if (!tofiledate.empty()) {
                todate = "\t" + tofiledate;
            }
            result.push_back("--- " + fromfile + fromdate + lineterm);
            result.push_back("+++ " + tofile + todate + lineterm);
        }

        DiffChunk first = group[0];
        DiffChunk last = group.back();
        std::string file1_range = _format_range_unified(first.start_a, last.end_a);
        std::string file2_range = _format_range_unified(first.start_b, last.end_b);
        result.push_back("@@ -" + file1_range + " +" + file2_range + " @@" + lineterm);

        for (DiffChunk _d : group) {
            std::string tag = _d.tag;
            int i1 = _d.start_a;
            int i2 = _d.end_a;
            int j1 = _d.start_b;
            int j2 = _d.end_b;
            if (tag == "equal") {
                std::string line = a.substr(i1,i2);
                result.push_back(" " + line);
                continue;
            }
            if (tag == "replace" || tag == "delete") {
                std::string line = a.substr(i1, i2);
                result.push_back("-" + line);
            }
            if (tag == "replace" || tag == "insert") {
                std::string line = b.substr(j1, j2);
                result.push_back("+" + line);
            }
        }
    }
    return result;
}


////////////////////////////////////////////////////////////////////////
///  Context Diff
////////////////////////////////////////////////////////////////////////

/*! Convert range to the "ed" format */
static std::string _format_range_context(int start, int stop) {
    // Per the diff spec at http://www.unix.org/single_unix_specification/
    int beginning = start + 1;     // lines start numbering with one
    int length = stop - start;
    if (not length) {
        beginning -= 1;        // empty ranges begin at line just before the range
    }
    if (length <= 1) {
        return std::to_string(beginning);
    }
    return std::to_string(beginning) + "," + std::to_string(beginning + length - 1);
}

// See http://www.unix.org/single_unix_specification/
std::vector<std::string> context_diff(std::string a, std::string b, std::string fromfile, std::string tofile,
                         std::string fromfiledate, std::string tofiledate, int n, char lineterm) {

    std::vector<std::string> result;

    std::map<std::string, std::string> prefix = {{"insert", "+ "}, {"delete", "- "}, {"replace", "! "}, {"equal", "  "}};
    bool started = false;
    for (std::vector<DiffChunk> group : SequenceMatcher(0, {a}, {b}).get_grouped_opcodes(n)) {
        if (not started) {
            started = true;
            std::string fromdate = "";
            if (!fromfiledate.empty()) {
                fromdate = "\t" + fromfiledate;
            }
            std::string todate = "";
            if (!tofiledate.empty()) {
                todate = "\t" + tofiledate;
            }
            result.push_back("*** " + fromfile + fromdate + lineterm);
            result.push_back("--- " + tofile + todate + lineterm);
        }

        DiffChunk first = group[0];
        DiffChunk last = group.back();
        result.push_back("***************" + lineterm);

        std::string file1_range = _format_range_context(first.start_a, last.end_a);
        result.push_back("*** " + file1_range + " ****" + lineterm);

        bool contains_replace_or_delete = false;
        for (DiffChunk d : group) {
            if (d.tag == "replace" || d.tag == "delete") {
                contains_replace_or_delete = true;
                break;
            }
        }

        if (contains_replace_or_delete) {
            for (DiffChunk d : group) {
                std::string tag = d.tag;
                int i1 = d.start_a;
                int i2 = d.end_b;
                if (tag != "insert") {
                    std::string line = a.substr(i1, i2);
                    result.push_back(prefix[tag] + line);
                }
            }
        }

        std::string file2_range = _format_range_context(first.start_b, last.end_b);
        result.push_back("--- " + file2_range + " ----" + lineterm);

        bool contains_replace_or_insert = false;
        for (DiffChunk d : group) {
            if (d.tag == "replace" || d.tag == "insert") {
                contains_replace_or_insert = true;
                break;
            }
        }

        if (contains_replace_or_insert) {
            for (DiffChunk d : group) {
                std::string tag = d.tag;
                int j1 = d.start_b;
                int j2 = d.end_b;
                if (tag != "delete") {
                    std::string line = b.substr(j1, j2);
                    result.push_back(prefix[tag] + line);
                }
            }
        }
    }
}

std::vector<std::string> ndiff(std::vector<std::string> a, std::vector<std::string> b, std::function<bool(std::string)> linejunk, std::function<bool(std::string)> charjunk) {
    return Differ(linejunk, charjunk).compare(a, b);
}

#if 0
int record_sub_info(match_object, std::vector<int>& sub_info) {
    sub_info.append([match_object.group(1)[0],match_object.span()])
    return match_object.group(1)
}
#endif

/*!
    Returns line of text with user's change markup and line formatting.

    lines -- list of lines from the ndiff generator to produce a line of
             text from.  When producing the line of text to return, the
             lines used are removed from this list.
    format_key -- '+' return first line in list with "add" markup around
                      the entire line.
                  '-' return first line in list with "delete" markup around
                      the entire line.
                  '?' return first line in list with add/delete/change
                      intraline markup (indices obtained from second line)
                  None return first line in list with no markup
    side -- indice into the num_lines list (0=from,1=to)
    num_lines -- from/to current line number.  This is NOT intended to be a
                 passed parameter.  It is present as a keyword argument to
                 maintain memory of the current line numbers between calls
                 of this function.

    Note, this function is purposefully not defined at the module scope so
    that data it needs from its parent function (within whose context it
    is defined) does not need to be of module scope.
 */
static std::pair<int, std::string>* _make_line(std::vector<std::string> lines, char format_key, int side, std::vector<int> num_lines = {0, 0}) {
    num_lines[side] += 1;
    // Handle case where no user markup is to be added, just return line of
    // text with user's line format to allow for usage of the line number.
    if (!format_key) {
        std::string f = lines.front();
        lines.erase(lines.begin());
        return new std::pair<int, std::string>(num_lines[side], f.substr(2));
    }

    // regular expression for finding intraline change indices
    static const boost::regex change_re("(\\++|\\-+|\\^+)");

    std::string text;
    // Handle case of intraline changes
    if (format_key == '?') {
        text = lines.front();
        lines.erase(lines.begin());
        std::string markers = lines.front();
        lines.erase(lines.begin());
        // find intraline changes (store change type and indices in tuples)
#if 0
        sub_info = [];
        change_re.sub(record_sub_info,markers);
        // process each tuple inserting our special marks that won't be
        // noticed by an xml/html escaper.
        for (key,(begin,end) in sub_info[::-1]) {
            text = text[0:begin]+'\0'+key+text[begin:end]+'\1'+text[end:];
        }
#endif
        text = text.substr(2);
    // Handle case of add/delete entire line
    } else {
        text = lines.front().substr(2);
        lines.erase(lines.begin());
        // if line of text is just a newline, insert a space so there is
        // something for the user to highlight and see.
        if (text.empty()) {
            text = " ";
        }
        // insert marks that won't be noticed by an xml/html escaper.
        std::stringstream ss;
        ss << '\0' << format_key << text << '\1';
        text = ss.str();
    }
    // Return line of text, first allow user's line formatter to do its
    // thing (such as adding the line number) then replace the special
    // marks with what the user's change markup.
    return new std::pair<int, std::string>(num_lines[side],text);
}

/*!
    Yields from/to lines of text with a change indication.

    This function is an iterator.  It itself pulls lines from a
    differencing iterator, processes them and yields them.  When it can
    it yields both a "from" and a "to" line, otherwise it will yield one
    or the other.  In addition to yielding the lines of from/to text, a
    boolean flag is yielded to indicate if the text line(s) have
    differences in them.

    Note, this function is purposefully not defined at the module scope so
    that data it needs from its parent function (within whose context it
    is defined) does not need to be of module scope.
 */
static std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> _line_iterator(std::vector<std::string>& diff_lines) {
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> result;

    std::vector<std::string>::iterator diff_lines_iterator = diff_lines.begin();

    std::vector<std::string> lines;
    int num_blanks_pending = 0;
    int num_blanks_to_yield = 0;
    std::pair<int, std::string>* from_line;
    std::pair<int, std::string>* to_line;
    while (true) {
        // Load up next 4 lines so we can look ahead, create strings which
        // are a concatenation of the first character of each of the 4 lines
        // so we can do some very readable comparisons.
        while (lines.size() < 4) {
            if (diff_lines_iterator != diff_lines.end()) {
                lines.push_back(*diff_lines_iterator);
                diff_lines_iterator++;
            } else {
                lines.push_back("X");
            }
        }
        std::stringstream ss;
        for (std::string line : lines) {
            ss << line[0];
        }
        std::string s = ss.str();
        if (boost::starts_with(s, "X")) {
            // When no more lines, pump out any remaining blank lines so the
            // corresponding add/delete lines get a matching blank line so
            // all line pairs get yielded at the next level.
            num_blanks_to_yield = num_blanks_pending;
        } else if (boost::starts_with(s, "-?+?")) {
            // simple intraline change
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,'?',0), _make_line(lines,'?',1), true));
            continue;
        } else if (boost::starts_with(s, "--++")) {
            // in delete block, add block coming: we do NOT want to get
            // caught up on blank lines yet, just process the delete line
            num_blanks_pending -= 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,'-',0), 0, true));
            continue;
        } else if (boost::starts_with(s, ("--?+", "--+", "- "))) {
            // in delete block and see a intraline change or unchanged line
            // coming: yield the delete line and then blanks
            from_line = _make_line(lines,'-',0);
            to_line = 0;
            num_blanks_to_yield = num_blanks_pending - 1;
            num_blanks_pending = 0;
        } else if (boost::starts_with(s, "-+?")) {
            // intraline change
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,0,0), _make_line(lines,'?',1), true));
            continue;
        } else if (boost::starts_with(s, "-?+")) {
            // intraline change
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,'?',0), _make_line(lines,0,1), true));
            continue;
        } else if (boost::starts_with(s, "-")) {
            // delete FROM line
            num_blanks_pending -= 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,'-',0), 0, true));
            continue;
        } else if (boost::starts_with(s, "+--")) {
            // in add block, delete block coming: we do NOT want to get
            // caught up on blank lines yet, just process the add line
            num_blanks_pending += 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(0, _make_line(lines,'+',1), true));
            continue;
        } else if (boost::starts_with(s, ("+ ", "+-"))) {
            // will be leaving an add block: yield blanks then add line
            from_line = 0;
            to_line = _make_line(lines,'+',1);
            num_blanks_to_yield = num_blanks_pending + 1;
            num_blanks_pending = 0;
        } else if (boost::starts_with(s, "+")) {
            // inside an add block, yield the add line
            num_blanks_pending += 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(0, _make_line(lines,'+',1), true));
            continue;
        } else if (boost::starts_with(s, " ")) {
            // unchanged text, yield it to both sides
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(_make_line(lines,0,0),_make_line(lines,0,1),false));
            continue;
        }
        // Catch up on the blank lines so when we yield the next from/to
        // pair, they are lined up.
        while (num_blanks_to_yield < 0) {
            num_blanks_to_yield += 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(0,new std::pair<int, std::string>(0,"\n"),true));
        }
        while (num_blanks_to_yield > 0) {
            num_blanks_to_yield -= 1;
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(new std::pair<int, std::string>(0,"\n"),0,true));
        }
        if (boost::starts_with(s, "X")) {
            return result;
        } else {
            result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(from_line,to_line,true));
        }
    }
}

/*!
    Yields from/to lines of text with a change indication.

    This function is an iterator.  It itself pulls lines from the line
    iterator.  Its difference from that iterator is that this function
    always yields a pair of from/to text lines (with the change
    indication).  If necessary it will collect single from/to lines
    until it has a matching pair from/to pair to yield.

    Note, this function is purposefully not defined at the module scope so
    that data it needs from its parent function (within whose context it
    is defined) does not need to be of module scope.
 */
static std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> _line_pair_iterator(std::vector<std::string>& diff_lines) {
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> line = _line_iterator(diff_lines);
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>>::iterator line_iterator = line.begin();
    std::vector<std::pair<std::pair<int, std::string>*, bool>> fromlines;
    std::vector<std::pair<std::pair<int, std::string>*, bool>> tolines;
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> result;
    while (line_iterator != line.end()) {
        // Collecting lines of text until we have a from/to pair
        while (fromlines.empty() or tolines.empty()) {
            std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool> _t = *line_iterator;
            std::pair<int, std::string>* from_line = std::get<0>(_t);
            std::pair<int, std::string>* to_line = std::get<1>(_t);
            bool found_diff = std::get<2>(_t);
            if (from_line) {
                fromlines.push_back(std::pair<std::pair<int, std::string>*, bool>(from_line, found_diff));
            }
            if (to_line) {
                tolines.push_back(std::pair<std::pair<int, std::string>*, bool>(to_line, found_diff));
            }
            line_iterator++;
        }
        // Once we have a pair, remove them from the collection and yield it
        std::pair<int, std::string>* from_line = std::get<0>(fromlines.front());
        bool fromDiff = std::get<1>(fromlines.front());
        std::pair<int, std::string>* to_line = std::get<0>(tolines.front());
        bool to_diff = std::get<1>(tolines.front());
        result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(from_line, to_line, fromDiff or to_diff));
    }
    return result;
}

/*!
    Returns generator yielding marked up from/to side by side differences.

    Arguments:
    fromlines -- list of text lines to compared to tolines
    tolines -- list of text lines to be compared to fromlines
    context -- number of context lines to display on each side of difference,
               if None, all from/to text lines will be generated.
    linejunk -- passed on to ndiff (see ndiff documentation)
    charjunk -- passed on to ndiff (see ndiff documentation)

    This function returns an iterator which returns a tuple:
    (from line tuple, to line tuple, boolean flag)

    from/to line tuple -- (line num, line text)
        line num -- integer or None (to indicate a context separation)
        line text -- original line text with following markers inserted:
            '\0+' -- marks start of added text
            '\0-' -- marks start of deleted text
            '\0^' -- marks start of changed text
            '\1' -- marks end of added/deleted/changed text

    boolean flag -- None indicates context separation, True indicates
        either "from" or "to" line contains a change, otherwise False.

    This function/iterator was originally developed to generate side by side
    file difference for making HTML pages (see HtmlDiff class for example
    usage).

    Note, this function utilizes the ndiff function to generate the side by
    side difference markup.  Optional ndiff arguments may be passed to this
    function and they in turn will be passed to ndiff.
 */
static std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> _mdiff(std::vector<std::string> fromlines, std::vector<std::string> tolines, int *context = 0, std::function<bool(std::string)> linejunk = 0,
           std::function<bool(std::string)> charjunk = IS_CHARACTER_JUNK) {

    // create the difference iterator to generate the differences
    std::vector<std::string> diff_lines = ndiff(fromlines, tolines, linejunk, charjunk);

    // Handle case where user does not want context differencing, just yield
    // them up without doing anything else with them.
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> line_pair = _line_pair_iterator(diff_lines);
    std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>>::iterator line_pair_iterator = line_pair.begin();
    if (!context) {
        return line_pair;
    // Handle case where user wants context differencing.  We must do some
    // storage of lines until we know for sure that they are to be yielded.
    } else {
        std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> result;

        *context += 1;
        int lines_to_write = 0;
        while (true) {
            // Store lines up until we find a difference, note use of a
            // circular queue because we only need to keep around what
            // we need for context.
            int index = 0;
            std::vector<std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>> contextLines;
            for (int i = 0; i < *context; i++) {
                contextLines.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(0,0,false));
            }
            bool found_diff = false;
            while (!found_diff) {
                auto t = *line_pair_iterator;
                auto from_line = std::get<0>(t);
                auto to_line = std::get<1>(t);
                found_diff = std::get<2>(t);
                line_pair_iterator++;
                int i = index % *context;
                contextLines[i] = std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(from_line, to_line, found_diff);
                index += 1;
            }
            // Yield lines that we have collected so far, but first yield
            // the user's separator.
            if (index > *context) {
                result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(0, 0, false));
                lines_to_write = *context;
            } else {
                lines_to_write = index;
                index = 0;
            }
            while (lines_to_write) {
                int i = index % *context;
                index += 1;
                result.push_back(contextLines[i]);
                lines_to_write -= 1;
            }
            // Now yield the context lines after the change
            lines_to_write = *context-1;
            while (lines_to_write) {
                auto t = *line_pair_iterator;
                auto from_line = std::get<0>(t);
                auto to_line = std::get<1>(t);
                found_diff = std::get<2>(t);
                line_pair_iterator++;
                // If another change within the context, extend the context
                if (found_diff) {
                    lines_to_write = *context-1;
                } else {
                    lines_to_write -= 1;
                }
                result.push_back(std::tuple<std::pair<int, std::string>*, std::pair<int, std::string>*, bool>(from_line, to_line, found_diff));
            }
        }
    }
}

#if 0
const std::string _file_template = ""
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\""
"          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
""
"<html>"
""
"<head>"
"    <meta http-equiv=\"Content-Type\""
"          content=\"text/html; charset=ISO-8859-1\" />"
"    <title></title>"
"    <style type=\"text/css\">%(styles)s"
"    </style>"
"</head>"
""
"<body>"
"    %(table)s%(legend)s"
"</body>"
""
"</html>";

const std::string _styles = ""
"        table.diff {font-family:Courier; border:medium;}"
"        .diff_header {background-color:#e0e0e0}"
"        td.diff_header {text-align:right}"
"        .diff_next {background-color:#c0c0c0}"
"        .diff_add {background-color:#aaffaa}"
"        .diff_chg {background-color:#ffff77}"
"        .diff_sub {background-color:#ffaaaa}";

const std::string _table_template = ""
"    <table class=\"diff\" id=\"difflib_chg_%(prefix)s_top\""
"           cellspacing=\"0\" cellpadding=\"0\" rules=\"groups\" >"
"        <colgroup></colgroup> <colgroup></colgroup> <colgroup></colgroup>"
"        <colgroup></colgroup> <colgroup></colgroup> <colgroup></colgroup>"
"        %(header_row)s"
"        <tbody>"
"%(data_rows)s        </tbody>"
"    </table>";

const std::string _legend = ""
"    <table class=\"diff\" summary=\"Legends\">"
"        <tr> <th colspan=\"2\"> Legends </th> </tr>"
"        <tr> <td> <table border="" summary=\"Colors\">"
"                      <tr><th> Colors </th> </tr>"
"                      <tr><td class=\"diff_add\">&nbsp;Added&nbsp;</td></tr>"
"                      <tr><td class=\"diff_chg\">Changed</td> </tr>"
"                      <tr><td class=\"diff_sub\">Deleted</td> </tr>"
"                  </table></td>"
"             <td> <table border=\"\" summary=\"Links\">"
"                      <tr><th colspan=\"2\"> Links </th> </tr>"
"                      <tr><td>(f)irst change</td> </tr>"
"                      <tr><td>(n)ext change</td> </tr>"
"                      <tr><td>(t)op</td> </tr>"
"                  </table></td> </tr>"
"    </table>"

/*!
    For producing HTML side by side comparison with change highlights.

    This class can be used to create an HTML table (or a complete HTML file
    containing the table) showing a side by side, line by line comparison
    of text with inter-line and intra-line change highlights.  The table can
    be generated in either full or contextual difference mode.

    The following methods are provided for HTML generation:

    make_table -- generates HTML for a single side by side table
    make_file -- generates complete HTML file with a single side by side table

    See tools/scripts/diff.py for an example usage of this class.
 */
class HtmlDiff {

    _file_template = _file_template
    _styles = _styles
    _table_template = _table_template
    _legend = _legend
    _default_prefix = 0

    /*!
        HtmlDiff instance initializer

        Arguments:
        tabsize -- tab stop spacing, defaults to 8.
        wrapcolumn -- column number where lines are broken and wrapped,
            defaults to None where lines are not wrapped.
        linejunk,charjunk -- keyword arguments passed into ndiff() (used to by
            HtmlDiff() to generate the side by side HTML differences).  See
            ndiff() documentation for argument default values and descriptions.
     */
    def __init__(self,tabsize=8,wrapcolumn=None,linejunk=None,
                 charjunk=IS_CHARACTER_JUNK):
        this->_tabsize = tabsize
        this->_wrapcolumn = wrapcolumn
        this->_linejunk = linejunk
        this->_charjunk = charjunk

    /*!
        Returns HTML file of side by side comparison with change highlights

        Arguments:
        fromlines -- list of "from" lines
        tolines -- list of "to" lines
        fromdesc -- "from" file column header string
        todesc -- "to" file column header string
        context -- set to True for contextual differences (defaults to False
            which shows full differences).
        numlines -- number of context lines.  When context is set True,
            controls number of lines displayed before and after the change.
            When context is False, controls the number of lines to place
            the "next" link anchors before the next change (so click of
            "next" link jumps to just before the change).
     */
    def make_file(self,fromlines,tolines,fromdesc='',todesc='',context=False,
                  numlines=5):

        return this->_file_template % dict(
            styles = this->_styles,
            legend = this->_legend,
            table = this->make_table(fromlines,tolines,fromdesc,todesc,
                                    context=context,numlines=numlines))
    /*!
        Returns from/to line lists with tabs expanded and newlines removed.

        Instead of tab characters being replaced by the number of spaces
        needed to fill in to the next tab stop, this function will fill
        the space with tab characters.  This is done so that the difference
        algorithms can identify changes in a file when tabs are replaced by
        spaces and vice versa.  At the end of the HTML generation, the tab
        characters will be replaced with a nonbreakable space.
     */
    def _tab_newline_replace(self,fromlines,tolines):
        def expand_tabs(line):
            // hide real spaces
            line = line.replace(' ','\0')
            // expand tabs into spaces
            line = line.expandtabs(this->_tabsize)
            // replace spaces from expanded tabs back into tab characters
            // (we'll replace them with markup after we do differencing)
            line = line.replace(' ','\t')
            return line.replace('\0',' ').rstrip('\n')
        fromlines = [expand_tabs(line) for line in fromlines]
        tolines = [expand_tabs(line) for line in tolines]
        return fromlines,tolines

    /*!
        Builds list of text lines by splitting text lines at wrap point

        This function will determine if the input text line needs to be
        wrapped (split) into separate lines.  If so, the first wrap point
        will be determined and the first line appended to the output
        text line list.  This function is used recursively to handle
        the second part of the split line to further split it.
     */
    def _split_line(self,data_list,line_num,text):
        // if blank line or context separator, just add it to the output list
        if not line_num:
            data_list.append((line_num,text))
            return

        // if line text doesn't need wrapping, just add it to the output list
        size = len(text)
        max = this->_wrapcolumn
        if (size <= max) or ((size -(text.count('\0')*3)) <= max):
            data_list.append((line_num,text))
            return

        // scan text looking for the wrap point, keeping track if the wrap
        // point is inside markers
        i = 0
        n = 0
        mark = ''
        while n < max and i < size:
            if text[i] == '\0':
                i += 1
                mark = text[i]
                i += 1
            } else if (text[i] == '\1':
                i += 1
                mark = ''
            else:
                i += 1
                n += 1

        // wrap point is inside text, break it up into separate lines
        line1 = text[:i]
        line2 = text[i:]

        // if wrap point is inside markers, place end marker at end of first
        // line and start marker at beginning of second line because each
        // line will have its own table tag markup around it.
        if mark:
            line1 = line1 + '\1'
            line2 = '\0' + mark + line2

        // tack on first line onto the output list
        data_list.append((line_num,line1))

        // use this routine again to wrap the remaining text
        this->_split_line(data_list,'>',line2)

    def _line_wrapper(self,diffs):
        """Returns iterator that splits (wraps) mdiff text lines"""

        // pull from/to data and flags from mdiff iterator
        for fromdata,todata,flag in diffs:
            // check for context separators and pass them through
            if flag is None:
                yield fromdata,todata,flag
                continue
            (fromline,fromtext),(toline,totext) = fromdata,todata
            // for each from/to line split it at the wrap column to form
            // list of text lines.
            fromlist,tolist = [],[]
            this->_split_line(fromlist,fromline,fromtext)
            this->_split_line(tolist,toline,totext)
            // yield from/to line in pairs inserting blank lines as
            // necessary when one side has more wrapped lines
            while fromlist or tolist:
                if fromlist:
                    fromdata = fromlist.pop(0)
                else:
                    fromdata = ('',' ')
                if tolist:
                    todata = tolist.pop(0)
                else:
                    todata = ('',' ')
                yield fromdata,todata,flag

    /*!
        Collects mdiff output into separate lists

        Before storing the mdiff from/to data into a list, it is converted
        into a single line of text with HTML markup.
     */
    def _collect_lines(self,diffs):

        fromlist,tolist,flaglist = [],[],[]
        // pull from/to data and flags from mdiff style iterator
        for fromdata,todata,flag in diffs:
            try:
                // store HTML markup of the lines into the lists
                fromlist.append(this->_format_line(0,flag,*fromdata))
                tolist.append(this->_format_line(1,flag,*todata))
            except TypeError:
                // exceptions occur for lines where context separators go
                fromlist.append(None)
                tolist.append(None)
            flaglist.append(flag)
        return fromlist,tolist,flaglist

    /*!
        Returns HTML markup of "from" / "to" text lines

        side -- 0 or 1 indicating "from" or "to" text
        flag -- indicates if difference on line
        linenum -- line number (used for line number column)
        text -- line text to be marked up
    */
    def _format_line(self,side,flag,linenum,text):
        try:
            linenum = '%d' % linenum
            id = ' id="%s%s"' % (this->_prefix[side],linenum)
        except TypeError:
            // handle blank lines where linenum is '>' or ''
            id = ''
        // replace those things that would get confused with HTML symbols
        text=text.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;")

        // make space non-breakable so they don't get compressed or line wrapped
        text = text.replace(' ','&nbsp;').rstrip()

        return '<td class="diff_header"%s>%s</td><td nowrap="nowrap">%s</td>' \
               % (id,linenum,text)

    /*! Create unique anchor prefixes */
    def _make_prefix() {

        // Generate a unique anchor prefix so multiple tables
        // can exist on the same HTML page without conflicts.
        fromprefix = "from%d_" % HtmlDiff._default_prefix
        toprefix = "to%d_" % HtmlDiff._default_prefix
        HtmlDiff._default_prefix += 1
        // store prefixes so line format method has access
        this->_prefix = [fromprefix,toprefix]

    /*! Makes list of "next" links */
    def _convert_flags(self,fromlist,tolist,flaglist,context,numlines):

        // all anchor names will be generated using the unique "to" prefix
        toprefix = this->_prefix[1]

        // process change flags, generating middle column of next anchors/links
        next_id = ['']*len(flaglist)
        next_href = ['']*len(flaglist)
        num_chg, in_change = 0, False
        last = 0
        for i,flag in enumerate(flaglist):
            if flag:
                if not in_change:
                    in_change = True
                    last = i
                    // at the beginning of a change, drop an anchor a few lines
                    // (the context lines) before the change for the previous
                    // link
                    i = max([0,i-numlines])
                    next_id[i] = ' id="difflib_chg_%s_%d"' % (toprefix,num_chg)
                    // at the beginning of a change, drop a link to the next
                    // change
                    num_chg += 1
                    next_href[last] = '<a href="#difflib_chg_%s_%d">n</a>' % (
                         toprefix,num_chg)
            else:
                in_change = False
        // check for cases where there is no content to avoid exceptions
        if not flaglist:
            flaglist = [False]
            next_id = ['']
            next_href = ['']
            last = 0
            if context:
                fromlist = ['<td></td><td>&nbsp;No Differences Found&nbsp;</td>']
                tolist = fromlist
            else:
                fromlist = tolist = ['<td></td><td>&nbsp;Empty File&nbsp;</td>']
        // if not a change on first line, drop a link
        if not flaglist[0]:
            next_href[0] = '<a href="#difflib_chg_%s_0">f</a>' % toprefix
        // redo the last link to link to the top
        next_href[last] = '<a href="#difflib_chg_%s_top">t</a>' % (toprefix)

        return fromlist,tolist,flaglist,next_href,next_id

    /*!
        Returns HTML table of side by side comparison with change highlights

        Arguments:
        fromlines -- list of "from" lines
        tolines -- list of "to" lines
        fromdesc -- "from" file column header string
        todesc -- "to" file column header string
        context -- set to True for contextual differences (defaults to False
            which shows full differences).
        numlines -- number of context lines.  When context is set True,
            controls number of lines displayed before and after the change.
            When context is False, controls the number of lines to place
            the "next" link anchors before the next change (so click of
            "next" link jumps to just before the change).
     */
    def make_table(self,fromlines,tolines,fromdesc='',todesc='',context=False,
                   numlines=5):

        // make unique anchor prefixes so that multiple tables may exist
        // on the same page without conflict.
        this->_make_prefix()

        // change tabs to spaces before it gets more difficult after we insert
        // markup
        fromlines,tolines = this->_tab_newline_replace(fromlines,tolines)

        // create diffs iterator which generates side by side from/to data
        if context:
            context_lines = numlines
        else:
            context_lines = None
        diffs = _mdiff(fromlines,tolines,context_lines,linejunk=self._linejunk,
                      charjunk=self._charjunk)

        // set up iterator to wrap lines that exceed desired width
        if this->_wrapcolumn:
            diffs = this->_line_wrapper(diffs)

        // collect up from/to lines and flags into lists (also format the lines)
        fromlist,tolist,flaglist = this->_collect_lines(diffs)

        // process change flags, generating middle column of next anchors/links
        fromlist,tolist,flaglist,next_href,next_id = this->_convert_flags(
            fromlist,tolist,flaglist,context,numlines)

        s = []
        fmt = '            <tr><td class="diff_next"%s>%s</td>%s' + \
              '<td class="diff_next">%s</td>%s</tr>\n'
        for i in range(len(flaglist)):
            if flaglist[i] is None:
                // mdiff yields None on separator lines skip the bogus ones
                // generated for the first line
                if i > 0:
                    s.append('        </tbody>        \n        <tbody>\n')
            else:
                s.append( fmt % (next_id[i],next_href[i],fromlist[i],
                                           next_href[i],tolist[i]))
        if fromdesc or todesc:
            header_row = '<thead><tr>%s%s%s%s</tr></thead>' % (
                '<th class="diff_next"><br /></th>',
                '<th colspan="2" class="diff_header">%s</th>' % fromdesc,
                '<th class="diff_next"><br /></th>',
                '<th colspan="2" class="diff_header">%s</th>' % todesc)
        else:
            header_row = ''

        table = this->_table_template % dict(
            data_rows=''.join(s),
            header_row=header_row,
            prefix=self._prefix[1])

        return table.replace('\0+','<span class="diff_add">'). \
                     replace('\0-','<span class="diff_sub">'). \
                     replace('\0^','<span class="diff_chg">'). \
                     replace('\1','</span>'). \
                     replace('\t','&nbsp;')

#endif

/*!
    Generate one of the two sequences that generated a delta.

    Given a `delta` produced by `Differ.compare()` or `ndiff()`, extract
    lines originating from file 1 or 2 (parameter `which`), stripping off line
    prefixes.

    Examples:

    >>> diff = ndiff('one\ntwo\nthree\n'.splitlines(keepends=True),
    ...              'ore\ntree\nemu\n'.splitlines(keepends=True))
    >>> diff = list(diff)
    >>> print(''.join(restore(diff, 1)), end="")
    one
    two
    three
    >>> print(''.join(restore(diff, 2)), end="")
    ore
    tree
    emu
 */
std::vector<std::string> restore(std::vector<std::string> delta, int which) {
    std::string tag;
    if (which == 1) {
        tag = "- ";
    } else if (which == 2) {
        tag = "+ ";
    } else {
        std::stringstream ss;
        ss << "unknown delta choice (must be 1 or 2): " << which;
        throw ValueError(ss.str());
    }
    std::vector<std::string> result;
    for (std::string line : delta) {
        if (boost::starts_with(line, "  ") || boost::starts_with(line, tag)) {
            result.push_back(line.substr(2));
        }
    }
    return result;
}

#if 0
def _test():
    import doctest, difflib
    return doctest.testmod(difflib)

if __name__ == "__main__":
    _test()
#endif
