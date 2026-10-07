// SPDX-License-Identifier: GPL-3.0-or-later
#include "candidate-page.h"
#include <cassert>
#include <iostream>
using namespace fcitx;
class Word : public CandidateWord {
public:
    explicit Word(int i):CandidateWord(Text(std::to_string(i))) {}
    void select(InputContext*) const override {}
};
int main() {
    CommonCandidateList list; list.setPageSize(9);
    for(int i=0;i<34;++i) list.append<Word>(i);
    for(int selected=0;selected<34;++selected) {
        followCandidateCursor(list,selected);
        assert(list.currentPage()==selected/9);
        assert(list.cursorIndex()==selected%9);
        assert(list.candidate(list.cursorIndex()).text().toString()==std::to_string(selected));
    }
    followCandidateCursor(list,0); assert(!list.hasPrev() && list.hasNext());
    list.next(); assert(list.currentPage()==1);
    list.prev(); assert(list.currentPage()==0);
    followCandidateCursor(list,33); assert(!list.hasNext() && list.size()==7);
    std::cout<<"Candidate cursor follows all 34 candidates across four pages; mouse paging passes\n";
}
