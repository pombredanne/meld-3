/* Copyright (C) 2002-2005 Stephen Kennedy <stevek@gnome.org>
 * Copyright (C) 2012 Kai Willadsen <kai.willadsen@gmail.com>
 * Copyright (C) 2014 Christoph Brill <egore911@gmail.com>
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

#ifndef __MELD__VC__INIT_H__
#define __MELD__VC__INIT_H__

#include <vector>
#include <string>

#include "_vc.h"

#if 0
// FIXME: This is a horrible hack to help cx_Freeze pick up these plugins when
// freezing the distributable package.
from . import (
git, mercurial, bzr, fossil, monotone, darcs, svk, svn, svn_17, cvs)
#endif

// Tuple with module name and vc.NAME field, ordered according to best-guess
// as to which VC a user is likely to want by default in a multiple-VC situation
extern const std::vector<std::string> vc_names;

#if 0
_plugins = [importlib.import_module("." + vc, __package__) for vc in vc_names]
#endif

extern void get_plugins_metadata();

extern const std::vector<std::string> vc_sort_order;

/*!
 * \brief Pick only the Vcs with the longest repo root
 *
 * Some VC plugins search their repository root
 * by walking the filesystem upwards its root
 * and now that we display multiple VCs in the
 * same directory, we must filter those other
 * repositories that are located in the search
 * path towards "/" as they are not relevant
 * to the user.
 */
extern std::vector<Vc*> get_vcs(std::string location);

#endif
