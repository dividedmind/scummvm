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
 */

#ifndef INTERSPECTIVE_PROG_DAT_H
#define INTERSPECTIVE_PROG_DAT_H

#include <memory>

#include "common/stream.h"

#include "interspective/datafile.h"

namespace Interspective {

class Resources;
class Program;

class ProgDat : public Datafile {
public:
	ProgDat(Resources *resources);
	~ProgDat();

	void load();

	const char *filename() const { return "iuc_prog.dat"; }
	void readFile(Common::SeekableReadStream &stream);

	Program *getScript(uint16 id);

private:
	byte *_data;
	std::auto_ptr<Common::SeekableReadStream> _file;
};

} // End of namespace Interspective

#endif // INTERSPECTIVE_PROG_DAT
