/* Copyright (C) 2009-2010 Piotr Piastucki <the_leech@users.berlios.de>
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

#include "matchers.h"

#include "merge.h"

#include <boost/algorithm/string.hpp>

AutoMergeDiffer::AutoMergeDiffer() : _Differ() {
    this->auto_merge = false;
}

AutoMergeDiffer::~AutoMergeDiffer() {
}

std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> AutoMergeDiffer::_auto_merge(std::array<difflib::chunk_list_t, 2> _using, std::vector<std::string> texts) {
    std::vector<std::pair<difflib::chunk_t, difflib::chunk_t>> result;
    for (std::pair<difflib::chunk_t, difflib::chunk_t> p : _Differ::_auto_merge(_using, texts)) {
        difflib::chunk_t out0 = p.first;
        difflib::chunk_t out1 = p.second;
        if (this->auto_merge and std::get<0>(out0) == "conflict") {
            // we will try to resolve more complex conflicts automatically here... if possible
            int l0 = std::get<3>(out0);
            int h0 = std::get<4>(out0);
            int l1 = std::get<1>(out0);
            int h1 = std::get<2>(out0);
            int l2 = std::get<3>(out1);
            int h2 = std::get<4>(out1);
            int len0 = h0 - l0;
            int len1 = h1 - l1;
            int len2 = h2 - l2;
            if ((len0 > 0 and len2 > 0) and (len0 == len1 or len2 == len1 or len1 == 0)) {
                MyersSequenceMatcher matcher(texts[0].substr(l0, h0), texts[2].substr(l2, h2), nullptr);
#if 0
                for (difflib::chunk_t chunk : matcher.get_opcodes()) {
                    int s1 = l1;
                    int e1 = l1;
                    if (len0 == len1) {
                        s1 += std::get<1>(chunk);
                        e1 += std::get<2>(chunk);
                    } else if (len2 == len1) {
                        s1 += std::get<3>(chunk);
                        e1 += std::get<4>(chunk);
                    }
                    if (std::get<0>(chunk) == "equal") {
                        out0 = difflib::chunk_t("replace", s1, e1, l0 + std::get<1>(chunk), l0 + std::get<2>(chunk));
                        out1 = difflib::chunk_t("replace", s1, e1, l2 + std::get<3>(chunk), l2 + std::get<4>(chunk));
                        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                    } else {
                        out0 = difflib::chunk_t("conflict", s1, e1, l0 + std::get<1>(chunk), l0 + std::get<2>(chunk));
                        out1 = difflib::chunk_t("conflict", s1, e1, l2 + std::get<3>(chunk), l2 + std::get<4>(chunk));
                        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                    }
                }
#endif
                return result;
//                elif len0 > 0 and len2 > 0:
                // this logic will resolve more conflicts automatically, but unresolved conflicts may sometimes look confusing
                // as the line numbers in ancestor file will be interpolated and may not reflect the actual changes
//                    matcher = MyersSequenceMatcher(None, texts[0][l0:h0], texts[2][l2:h2])
//                    if len0 > len2:
//                        maxindex = 1
//                        maxlen = len0
//                    else:
//                        maxindex = 3
//                        maxlen = len2
//                    for chunk in matcher.get_opcodes():
//                        if chunk[0] == "equal":
//                            out0 = ("replace", l1 + len1 * chunk[maxindex] / maxlen, l1 + len1 * chunk[maxindex + 1] / maxlen, l0 + chunk[1], l0 + chunk[2])
//                            out1 = ("replace", l1 + len1 * chunk[maxindex] / maxlen, l1 + len1 * chunk[maxindex + 1] / maxlen, l2 + chunk[3], l2 + chunk[4])
//                            yield out0, out1
//                        else:
//                            out0 = ("conflict", l1 + len1 * chunk[maxindex] / maxlen, l1 + len1 * chunk[maxindex + 1] / maxlen, l0 + chunk[1], l0 + chunk[2])
//                            out1 = ("conflict", l1 + len1 * chunk[maxindex] / maxlen, l1 + len1 * chunk[maxindex + 1] / maxlen, l2 + chunk[3], l2 + chunk[4])
//                            yield out0, out1
//                    return
            } else {
                // some tricks to resolve even more conflicts automatically
                // unfortunately the resulting chunks cannot be used to highlight changes
                // but hey, they are good enough to merge the resulting file :)
                std::string chunktype = std::get<0>(_using[0][0]);
                for (difflib::chunk_list_t chunkarr : _using) {
                    for (difflib::chunk_t chunk : chunkarr) {
                        if (std::get<0>(chunk) != chunktype) {
                            chunktype = "";
                            break;
                        }
                    }
                    if (chunktype.empty()) {
                        break;
                    }
                }
                if (chunktype == "delete") {
                    // delete + delete (any length) -> split into delete/conflict
                    difflib::chunk_t* seq0 = nullptr;
                    difflib::chunk_t* seq1 = nullptr;
                    int i0 = 0;
                    int i1 = 0;
                    int end0 = 0;
                    int end1 = 0;
                    while (true) {
                        if (!seq0) {
                            if (_using[0].empty()) {
                                break;
                            }
#if 0
                            seq0 = _using[0].pop(0);
#endif
                            i0 = std::get<1>(*seq0);
                            end0 = std::get<4>(*seq0);
                        }
                        if (!seq1) {
                            if (_using[1].empty()) {
                                break;
                            }
#if 0
                            seq1 = _using[1].pop(0);
#endif
                            i1 = std::get<1>(*seq1);
                            end1 = std::get<4>(*seq1);
                        }
                        int highstart = std::max(i0, i1);
                        if (i0 != i1) {
                            out0 = difflib::chunk_t("conflict", i0 - highstart + i1, highstart, std::get<3>(*seq0) - highstart + i1, std::get<3>(*seq0));
                            out1 = difflib::chunk_t("conflict", i1 - highstart + i0, highstart, std::get<3>(*seq1) - highstart + i0, std::get<3>(*seq1));
                            result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                        }
                        int lowend = std::min(std::get<2>(*seq0), std::get<2>(*seq1));
                        if (highstart != lowend) {
                            out0 = difflib::chunk_t("delete", highstart, lowend, std::get<3>(*seq0), std::get<4>(*seq0));
                            out1 = difflib::chunk_t("delete", highstart, lowend, std::get<3>(*seq1), std::get<4>(*seq1));
                            result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                        }
                        i0 = i1 = lowend;
                        if (lowend == std::get<2>(*seq0)) {
                            seq0 = nullptr;
                        }
                        if (lowend == std::get<2>(*seq1)) {
                            seq1 = nullptr;
                        }
                    }

                    if (seq0) {
                        out0 = difflib::chunk_t("conflict", i0, std::get<2>(*seq0), std::get<3>(*seq0), std::get<4>(*seq0));
                        out1 = difflib::chunk_t("conflict", i0, std::get<2>(*seq0), end1, end1 + std::get<2>(*seq0) - i0);
                        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                    } else if (seq1) {
                        out0 = difflib::chunk_t("conflict", i1, std::get<2>(*seq1), end0, end0 + std::get<2>(*seq1) - i1);
                        out1 = difflib::chunk_t("conflict", i1, std::get<2>(*seq1), std::get<3>(*seq1), std::get<4>(*seq1));
                        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
                    }
                    return result;
                }
            }
        }
        result.push_back(std::pair<difflib::chunk_t, difflib::chunk_t>(out0, out1));
    }
    return result;
}

void AutoMergeDiffer::change_sequence(int sequence, int startidx, int sizechange, std::vector<std::string> texts) {
    if (sequence == 1) {
        int lo = 0;
        for (int c : this->unresolved) {
            if (startidx <= c) {
                break;
            }
            lo += 1;
        }
        if (lo < this->unresolved.size()) {
            int hi = lo;
            if (sizechange < 0) {
                std::vector<int> tmp(this->unresolved.begin() + lo, this->unresolved.end());
                for (int c : tmp) {
                    if (startidx - sizechange <= c) {
                        break;
                    }
                    hi += 1;
                }
            } else if (sizechange == 0 and startidx == this->unresolved[lo]) {
                hi += 1;
            }

            if (hi < this->unresolved.size()) {
#if 0
                this->unresolved[hi:] = [c + sizechange for c in this->unresolved[hi:]];
#endif
            }
            this->unresolved.erase(this->unresolved.begin() + lo, this->unresolved.begin() + hi);
        }
    }

    _Differ::change_sequence(sequence, startidx, sizechange, texts);
}

int AutoMergeDiffer::get_unresolved_count() {
    return this->unresolved.size();
}

Merger::Merger() : _Differ() {
    if (typeid(this->differ) == typeid(AutoMergeDiffer*)) {
        static_cast<AutoMergeDiffer*>(this->differ)->auto_merge = true;
    }
}

Merger::~Merger() {
    delete this->differ;
    this->differ = nullptr;
}

void Merger::initialize(std::vector<BufferLines*> sequences, std::vector<BufferLines*> texts) {
#if 0
    this->differ->set_sequences_iter(sequences);
#endif
    this->texts = texts;
}

int Merger::_apply_change(BufferLines* text, difflib::chunk_t change, std::vector<Glib::ustring>& mergedtext) {
    const int LO = 1;
    const int HI = 2;
    if (std::get<0>(change) == "insert") {
        for (int i = std::get<LO + 2>(change); i < std::get<HI + 2>(change); i++) {
            mergedtext.push_back((*text)[i]);
        }
        return 0;
    } else if (std::get<0>(change) == "replace") {
        for (int i = std::get<LO + 2>(change); i < std::get<HI + 2>(change); i++) {
            mergedtext.push_back((*text)[i]);
        }
        return std::get<HI>(change) - std::get<LO>(change);
    } else {
        return std::get<HI>(change) - std::get<LO>(change);
    }
}

Glib::ustring Merger::merge_3_files(bool mark_conflicts) {

    const int LO = 1;
    const int HI = 2;
    this->unresolved.clear();
    int lastline = 0;
    int mergedline = 0;
    std::vector<std::string> mergedtext;
    for (std::pair<difflib::chunk_t, difflib::chunk_t> change : this->differ->all_changes()) {
        int low_mark = lastline;
        if (change.first != difflib::EMPTY_CHUNK) {
            low_mark = std::get<LO>(change.first);
        }
        if (change.second != difflib::EMPTY_CHUNK) {
            if (std::get<LO>(change.second) > low_mark) {
                low_mark = std::get<LO>(change.second);
            }
        }
        for (int i = lastline; i < low_mark; i++) {
            mergedtext.push_back(this->texts[1]->__getitem__(i));
        }
        mergedline += low_mark - lastline;
        lastline = low_mark;
        if (change.first != difflib::EMPTY_CHUNK and change.second != difflib::EMPTY_CHUNK and std::get<0>(change.first) == "conflict") {
            int high_mark = std::max(std::get<HI>(change.first), std::get<HI>(change.second));
            if (mark_conflicts) {
                if (low_mark < high_mark) {
                    for (int i = low_mark; i < high_mark; i++) {
                        mergedtext.push_back("(??)" + this->texts[1]->__getitem__(i));
                        this->unresolved.push_back(mergedline);
                        mergedline += 1;
                    }
                } else {
                    //conflictsize = min(1, max(change[0][HI + 2] - change[0][LO + 2], change[1][HI + 2] - change[1][LO + 2]))
                    //for i in range(conflictsize):
                    mergedtext.push_back("(??)");
                    this->unresolved.push_back(mergedline);
                    mergedline += 1;
                }
                lastline = high_mark;
            }
        } else if (change.first != difflib::EMPTY_CHUNK) {
#if 0
            lastline += this->_apply_change(this->texts[0], change.first, mergedtext);
#endif
            mergedline += std::get<HI + 2>(change.first) - std::get<LO + 2>(change.first);
        } else {
#if 0
            lastline += this->_apply_change(this->texts[2], change.second, mergedtext);
#endif
            mergedline += std::get<HI + 2>(change.second) - std::get<LO + 2>(change.second);
        }
    }
#if 0
    int baselen = this->texts[1].size();
    for (int i = lastline; i < baselen; i++) {
        mergedtext.push_back(this->texts[1][i]);
    }
#endif

    // FIXME: We need to obtain the original line endings from the lines
    // that were merged and use those here instead of assuming "\n".
    return boost::join(mergedtext, "\n");
}

Glib::ustring Merger::merge_2_files(int fromindex, int toindex) {
    const int LO = 1;
    const int HI = 2;
    this->unresolved.clear();
    int lastline = 0;
    std::vector<std::string> mergedtext;
    for (difflib::chunk_t change : this->differ->pair_changes(toindex, fromindex)) {
        int low_mark;
        if (std::get<0>(change) == "conflict") {
            low_mark = std::get<HI>(change);
        } else {
            low_mark = std::get<LO>(change);
        }
        for (int i = lastline; i < low_mark; i++) {
#if 0
            mergedtext.push_back(this->texts[toindex][i]);
#endif
        }
        lastline = low_mark;
        if (std::get<0>(change) != "conflict") {
#if 0
            lastline += this->_apply_change(this->texts[fromindex], change, mergedtext);
#endif
        }
    }
#if 0
    int baselen = this->texts[toindex].size();
    for (int i = lastline; i < baselen; i++) {
        mergedtext.push_back(this->texts[toindex][i]);
    }
#endif

    // FIXME: We need to obtain the original line endings from the lines
    // that were merged and use those here instead of assuming '\n'.
    return boost::join(mergedtext, "\n");
}
