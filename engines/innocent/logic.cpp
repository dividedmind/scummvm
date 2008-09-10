#include "innocent/logic.h"

#include <algorithm>

#include "common/util.h"

#include "innocent/innocent.h"
#include "innocent/inter.h"
#include "innocent/program.h"
#include "innocent/animation.h"
#include "innocent/resources.h"
#include "innocent/room.h"
#include "innocent/util.h"

namespace Innocent {

Logic::Logic(Engine *e) :
		_engine(e),
		_resources(e->resources()),
		_currentRoom(0xffff),
		_currentBlock(0xffff) {
}

Logic::~Logic() {
	for (Common::List<Animation *>::iterator it = _animations.begin(); it != _animations.end(); ++it)
		delete *it;
}

void Logic::init() {
	_toplevelInterpreter.reset(new Interpreter(this, _resources->mainBase(), "main code"));
}

void Logic::initCode() {
	debugC(2, kDebugLevelScript | kDebugLevelFlow, ">>>running initial code");
	_toplevelInterpreter->run(_resources->mainEntryPoint(), kCodeInitial);
	debugC(2, kDebugLevelScript | kDebugLevelFlow, "<<<finished initial code");
}

void Logic::tick() {
	runQueued();

	if (_roomLoop.get()) {
//		gDebugLevel--; // room loops aren't that interesting
		debugC(3, kDebugLevelScript | kDebugLevelFlow, ">>>running room loop code");
		_roomLoop->run(kCodeRoomLoop);
		debugC(3, kDebugLevelScript | kDebugLevelFlow, "<<<finished room loop code");
//		gDebugLevel++;
	}

	if (!_animations.empty())
		debugC(2, kDebugLevelFlow | kDebugLevelAnimation, "running animations");
	for (Common::List<Animation *>::iterator it = _animations.begin(); it != _animations.end(); ++it) {
		Animation::Status ret = (*it)->tick();
		if (ret == Animation::kRemove) {
			delete (*it);
			_animations.erase(it);
		}
	}
}

void Logic::setProtagonist(uint16 actor) {
	_protagonist = actor;
}

void Logic::changeRoom(uint16 newRoom) {
	if (newRoom == _currentRoom)
		return;
	_currentRoom = newRoom;
	_roomLoop.reset(0);
	uint16 newBlock = _resources->blockOfRoom(_currentRoom);
	if (newBlock != _currentBlock) {
		_currentBlock = newBlock;
		_blockProgram.reset(_resources->loadCodeBlock(newBlock));

		char buf[100];
		snprintf(buf, 100, "block %d code", newBlock);

		_blockInterpreter.reset(new Interpreter(this, _blockProgram->base(), buf));
		_blockProgram->loadActors(_blockInterpreter.get());
		_blockProgram->loadExits(_blockInterpreter.get());

		debugC(2, kDebugLevelScript, ">>>running block entry code for block %d", newBlock);
		_blockInterpreter->run(_blockProgram->begin(), kCodeNewBlock);
		debugC(2, kDebugLevelScript, "<<<finished block entry code for block %d", newBlock);
	}

	_room.reset(new Room(this));
	debugC(2, kDebugLevelScript, ">>>running room entry code for room %d", newRoom);
	_blockInterpreter->run(_blockProgram->roomHandler(newRoom), kCodeNewRoom);
	debugC(2, kDebugLevelScript, "<<<finished room entry code for room %d", newRoom);
}

void Logic::runLater(const CodePointer &p, uint16 delay) {
	_queued.push_back(DelayedRun(p, delay));
}

void Logic::runQueued() {
	if (_queued.empty()) return;

	Common::Queue<Common::List<DelayedRun>::iterator> toRemove;
	debugC(2, kDebugLevelFlow | kDebugLevelScript, ">>>running queued code");
	foreach (DelayedRun, _queued)
		if (it->delay) {
			debugC(3, kDebugLevelScript, "delayed %s, delay now %d", +it->code,
					it->delay);
			it->delay--;
		} else {
			debugC(2, kDebugLevelFlow | kDebugLevelScript, ">>>running %s", +it->code);
			it->code.run();
			debugC(2, kDebugLevelFlow | kDebugLevelScript, "<<<finished %s", +it->code);
			toRemove.push(it);
		}
	debugC(2, kDebugLevelFlow | kDebugLevelScript, "<<<finished queued code");

	while (!toRemove.empty())
		_queued.erase(toRemove.pop());
}

void Logic::addAnimation(Animation *anim) {
	_animations.push_back(anim);
}

void Logic::setRoomLoop(const CodePointer &code) {
	_roomLoop.reset(new CodePointer(code));
}

/* counting starts with 1 */
Actor *Logic::getActor(uint16 id) const {
	id--;
	if (id < _resources->mainDat()->actorsCount())
		return _resources->mainDat()->actor(id);
	else
		error("local actors not handled yet");
//		return _blockProgram->actor(id - _main->actorsCount());
}


Animation *Logic::animation(uint16 offset) const {
	foreach (Animation *, _animations)
		if ((*it)->baseOffset() == offset)
			return (*it);

	return 0;
}


} // End of namespace Innocent
