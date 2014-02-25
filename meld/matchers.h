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

#ifndef __MELD__MATCHERS_H__
#define __MELD__MATCHERS_H__

#include <deque>
#include "difflib/src/difflib.h"

extern void init_worker();

extern difflib::chunk_list_t matcher_worker(std::string text1, std::string textn);

class Snake {
public:
    Snake *lastsnake;
    int x;
    int y;
    int snake;
    Snake(Snake *lastsnake, int x, int y, int snake);
};

class MyersSequenceMatcher : public difflib::SequenceMatcher<std::string> {
protected:

    std::vector<int> aindex;
    std::vector<int> bindex;
    int common_prefix;
    int common_suffix;
    bool lines_discarded;

public:

    MyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk = nullptr);

    virtual difflib::match_list_t get_matching_blocks();

    difflib::chunk_list_t get_difference_opcodes();

    std::pair<std::string, std::string> preprocess_remove_prefix_suffix(std::string a, std::string b);

    std::pair<std::string, std::vector<int>> index_matching(std::string a, std::string b);

    virtual std::pair<std::string, std::string> preprocess_discard_nonmatching_lines(std::string a, std::string b);

    /*!
     * Pre-processing optimizations:
     * 1) remove common prefix and common suffix
     * 2) remove lines that do not match
     */
    std::pair<std::string, std::string> preprocess();

    /*!
     * Perform some post-processing cleanup to reduce 'chaff' and make
     * the result more human-readable. Since Myers diff is a greedy
     * algorithm backward scanning of matching chunks might reveal
     * some smaller chunks that can be combined together.
     */
    void postprocess();

    /*!
     * Build list of matching blocks based on snakes
     *
     * The resulting blocks take into consideration multiple preprocessing
     * optimizations:
     *  * add separate blocks for common prefix and suffix
     *  * shift positions and split blocks based on the list of discarded
     *    non-matching lines
     */
    void build_matching_blocks(Snake* lastsnake);

    /*!
     * Optimized implementation of the O(NP) algorithm described by Sun Wu,
     * Udi Manber, Gene Myers, Webb Miller
     * ("An O(NP) Sequence Comparison Algorithm", 1989)
     * http://research.janelia.org/myers/Papers/np_diff.pdf
     */
    virtual void initialise();
};

class InlineMyersSequenceMatcher : public MyersSequenceMatcher {
public:

    InlineMyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk);

    std::pair<std::string, std::vector<int>> index_matching_kmers(std::string a, std::string b);

    virtual std::pair<std::string, std::string> preprocess_discard_nonmatching_lines(std::string a, std::string b);
};

class SyncPointMyersSequenceMatcher : public MyersSequenceMatcher {
private:
    std::vector<std::pair<int, int>>* syncpoints;
    std::vector<difflib::match_list_t> split_matching_blocks;
public:

    SyncPointMyersSequenceMatcher(std::string a, std::string b, junk_function_type isjunk = nullptr, std::vector<std::pair<int, int>> *syncpoints = nullptr);

    virtual void initialise();

    virtual difflib::chunk_list_t get_opcodes();
};


#endif
