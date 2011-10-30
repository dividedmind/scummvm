/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 * Graphics Memory Manager data structures
 * TODO: This should really be named dos_hand.h, or the dos_hand.cpp should be renamed
 */

#ifndef TINSEL_HANDLE_H			// prevent multiple includes
#define TINSEL_HANDLE_H

#include "tinsel/dw.h"			// new data types

namespace Tinsel {

/*----------------------------------------------------------------------*\
|*                              Function Prototypes                     *|
\*----------------------------------------------------------------------*/

void SetupHandleTable(void);			// Loads the graphics handle table index file and preloads all the permanent graphics etc.
void FreeHandleTable(void);

uint8 *LockMem(			// returns the addr of a image, given its memory handle
	SCNHANDLE offset);			// handle and offset to data

void LockScene(					// Called to make the current scene non-discardable
	SCNHANDLE offset);			// handle and offset to data

void UnlockScene(				// Called to make the current scene discardable again
	SCNHANDLE offset);			// handle and offset to data

bool IsCdPlayHandle(SCNHANDLE offset);

void TouchMem(SCNHANDLE offset);

void SetCdPlaySceneDetails(			// Called at scene startup
	int	sceneNum,
	const char *fileName);

void SetCdPlayHandle(				// Called at game startup
	int	fileNum);

void LoadExtraGraphData(
	SCNHANDLE start,		// Handle of start of range
	SCNHANDLE next);		// Handle of end of range + 1

int CdNumber(SCNHANDLE offset);

} // end of namespace Tinsel

#endif	// TINSEL_HANDLE_H
