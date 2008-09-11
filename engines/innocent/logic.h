#ifndef INNOCENT_LOGIC_H
#define INNOCENT_LOGIC_H

#include <memory>
#include <utility>

#include "common/list.h"
#include "common/queue.h"
#include "common/singleton.h"
#include "config.h"

#include "innocent/value.h"

namespace Innocent {
//

class Actor;
class Animation;
class Debugger;
class Engine;
class Resources;
class Interpreter;
class Program;
class Room;

class Logic : public Common::Singleton<Logic> {
public:
	Logic() {}
	~Logic();

	void setEngine(Engine *e);

	void init();
	void initCode();

	// set actor# of the protagonist
	void setProtagonist(uint16);
	Actor *protagonist() const;

	void changeRoom(uint16);

	Engine *engine() { return _engine; }

	void tick();

	void addAnimation(Animation *anim);
	void setRoomLoop(const CodePointer &code);

	const Common::List<Animation *> animations() const { return _animations; }
	Room *room() const { return _room.get(); }
	uint16 roomNumber() const { return _currentRoom; }
	uint16 currentRoom() const { return _currentRoom; }
	Actor *getActor(uint16 id) const;

	Program *blockProgram() const { return _blockProgram.get(); }
	Interpreter *mainInterpreter() const { return _toplevelInterpreter.get(); }
	void runLater(const CodePointer &, uint16 delay = 0);

	Animation *animation(uint16 offset) const;

	friend class Debugger;
private:

	void runQueued();


	Engine *_engine;
	Resources *_resources;
	std::auto_ptr<Interpreter> _toplevelInterpreter, _blockInterpreter;
	Actor *_protagonist;
	uint32 _currentRoom;
	uint16 _currentBlock;
	std::auto_ptr<Program> _blockProgram;
	Common::List<Animation *> _animations;
	std::auto_ptr<CodePointer> _roomLoop;
	std::auto_ptr<Room> _room;

	struct DelayedRun {
		DelayedRun(const CodePointer &c, uint16 d) : code(c), delay(d) {}
		CodePointer code;
		uint16 delay;
	};
	Common::List<DelayedRun> _queued;
};

#define Log Logic::instance()

} // End of namespace Innocent

#endif // INNOCENT_LOGIC_H
