/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef BidiRun_h
#define BidiRun_h

#include "core/layout/api/LineLayoutItem.h"
#include "platform/text/BidiResolver.h"

namespace blink {

class InlineBox;

struct BidiRun : BidiCharacterRun {
  BidiRun(bool override,
          unsigned char level,
          int start,
          int stop,
          LineLayoutItem lineLayoutItem,
          WTF::Unicode::CharDirection dir,
          WTF::Unicode::CharDirection overrideDir)
      : BidiCharacterRun(override, level, start, stop, dir, overrideDir),
        m_lineLayoutItem(lineLayoutItem),
        m_box(nullptr) {
    // Stored in base class to save space.
    m_hasHyphen = false;
  }

  BidiRun* next() { return static_cast<BidiRun*>(m_next); }

 public:
  LineLayoutItem m_lineLayoutItem;
  InlineBox* m_box;
};

}  // namespace blink

#endif  // BidiRun_h
