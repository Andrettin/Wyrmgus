//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name iocompat.h - IO platform compatibility header file. */
//
//      (c) Copyright 2002-2005 by Andreas Arens
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#ifndef __IOCOMPAT_H__
#define __IOCOMPAT_H__

/*----------------------------------------------------------------------------
--  Platform dependent IO-related Includes and Definitions
----------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#ifdef _MSC_VER

#include <io.h>
#include <fcntl.h>
#include <direct.h>

#define R_OK 4
#define F_OK 0
#define PATH_MAX _MAX_PATH
#define S_ISDIR(x) ((x) & _S_IFDIR)
#define S_ISREG(x) ((x) & _S_IFREG)

#define mkdir _mkdir
#define access _access

#else

#include <unistd.h>
#include <dirent.h>

#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _WIN32
#define makedir(dir, permissions) mkdir(dir)
#else
#define makedir(dir, permissions) mkdir(dir, permissions)
#endif

#endif
