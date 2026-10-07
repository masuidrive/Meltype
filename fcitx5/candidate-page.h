// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <fcitx/candidatelist.h>
#include <algorithm>
inline void followCandidateCursor(fcitx::CommonCandidateList& list, int selected) {
    if (list.totalSize() == 0) return;
    selected = std::clamp(selected, 0, list.totalSize()-1);
    list.setPage(selected / list.pageSize());
    list.setGlobalCursorIndex(selected);
}
