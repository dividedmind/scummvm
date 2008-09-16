#ifndef INNOCENT_PROGRAM_H
#define INNOCENT_PROGRAM_H

#include "common/list.h"
#include "common/stream.h"

#include "innocent/debug.h"
#include "innocent/resources.h"

namespace Innocent {
//

class Exit;
class Actor;

class Program : public StaticInspectable {
	DEBUG_INFO
public:
	Program(Common::ReadStream &file, uint16 id);
	~Program();

	uint16 begin();
	byte *localVariable(uint16 offset);
	uint16 roomHandler(uint16 room);
	byte *base() const { return _code; }

	SpriteInfo getSpriteInfo(uint16 index) const;

	void loadActors(Interpreter *i);
	void loadExits(Interpreter *i);

	Exit *getExit(uint16 index) const;
	Common::List<Exit *> exitsForRoom(uint16 room) const;

	Actor *actor(uint16 index) const;

private:
	Program() { /* can only be created from a file */ }

	void clearExits();

	uint16 entryPointOffset();

	byte *_code;
	byte _footer[0x10];
	Common::List<Actor *> _actors;
	Exit **_exits;
	uint16 _exitsCount;
};

} // End of namespace Innocent

#endif // INNOCENT_PROGRAM_H
