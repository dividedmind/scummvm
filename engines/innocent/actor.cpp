#include "innocent/actor.h"

#include "common/rect.h"

#include "innocent/innocent.h"
#include "innocent/logic.h"
#include "innocent/room.h"
#include "innocent/util.h"

namespace Innocent {
//

Actor::Actor(const CodePointer &code) : Animation(code, Common::Point()) {
	byte *header = code.code();
	_base = header - code.offset();
	snprintf(_debugInfo, 50, "actor at %s", +code);
	readHeader(header);
	_direction = kDirNone;
	_frame = 0;
	_room = 0xffff;
	_debug = false;
	_attentionNeeded = false;

	Engine::instance().logic()->addAnimation(this);

	init_opcodes<37>();
}

bool Actor::isFine() const {
	return 	_room == Log.currentRoom() &&
			_base && !_attentionNeeded;
}

void Actor::setAnimation(uint16 offset) {
	setAnimation(CodePointer(offset, Log.mainInterpreter()));
}

void Actor::setAnimation(const CodePointer &anim) {
	debugC(3, kDebugLevelScript, "setting animation code of %s to %s", _debugInfo, +anim);
	_base = anim.code();
	_baseOffset = anim.offset();
	_offset = 0;
	_debugInvalid = false;
	_attentionNeeded = false;
	clearMainSprite();
	_interval = 1;
	_counter = _ticksLeft = 0;
}

void Actor::hide() {
	_base = 0;
	_baseOffset = _offset = 0;
}

void Actor::callMe(const CodePointer &code) {
	debugC(3, kDebugLevelScript, "actor will call %s when needed", +code);
	_callBacks.push(code);
}

void Actor::tellMe(const CodePointer &code, uint16 timeout) {
	_roomCallbacks.push_back(RoomCallback(timeout, code));
}

bool Actor::isSpeaking() const {
	return _speech.active();
}

void Actor::callMeWhenSilent(const CodePointer &cp) {
	_speech.callWhenDone(cp);
}

void Actor::say(const Common::String &text) {
	_speech = Speech(text);
}

Actor::Speech::~Speech() { while (!_cb.empty()) Log.runLater(_cb.pop()); }

bool Actor::isMoving() const {
	//TODO stub
	return false;
}

void Actor::callMeWhenStill(const CodePointer &cp) {
	assert(false);
}

void Actor::setFrame(uint16 frame) {
	_frame = frame;
	Frame f(Log.room()->getFrame(frame));
	_position = f.position();
}

void Actor::setRoom(uint16 r, uint16 frame, uint16 next_frame) {
	_room = r;
	unless (next_frame)
		next_frame = frame;
	_nextFrame = next_frame;
	setFrame(frame);

	setAnimation(CodePointer(_puppeteer.mainCodeOffset(), Log.mainInterpreter()));
}

bool Actor::nextFrame() {
	if (_framequeue.empty())
		return false;

	Frame next = _framequeue.pop();
	Frame current = Log.room()->getFrame(_frame);

	Direction direction = next - current;

	if (turnTo(direction))
		return true;

	setAnimation(_puppeteer.moveAnimator(direction));
	setFrame(next.index());
	return true;
}

bool Actor::turnTo(Direction dir) {
	if (dir == _direction)
		return false;

	Direction d = _direction>>dir;
	setAnimation(_puppeteer.turnAnimator(d));
	_direction = d;
	return true;
}

void Actor::animate() {
	unless (_puppeteer.valid())
		return;

	unless (_attentionNeeded/* || _timedOut*/)
		return;

	if (_nextAnimator) {
		setAnimation(_nextAnimator);
		_nextAnimator = 0;
		return;
	}

	if (nextFrame()) {
		return;
	}

	if (_nextDirection) {
		if (turnTo(_nextDirection))
			return;
		_direction = _nextDirection;
		_nextDirection = kDirNone;
/*		ax = _nextPuppeteer;
		_nextPuppeteer = 0;
		if (ax)
			goto set_anim;*/
		setAnimation(_puppeteer.offset());
/*	} else if (_nextPuppeteer) {
		ax = _nextPuppeteer;
		_nextPuppeteer = 0;
		if (ax)
			goto set_anim;*/
	} else {
		if (turnTo(kDirUp))
			return;
		_direction = kDirUp;
		setAnimation(_puppeteer.offset());
	}
}

Animation::Status Actor::tick() {
	animate();

	callBacks();

	if (isFine()) {
		Animation::Status s;
		if (_debug) gDebugLevel += 3;
			s = Animation::tick();
		if (_debug) gDebugLevel -= 3;
		return s;
	} else
		return kOk;
}

void Actor::toggleDebug() {
	_debug = !_debug;
}

void Actor::readHeader(const byte *code) {
	_interval = code[kOffsetInterval];
	_ticksLeft = READ_LE_UINT16(code + kOffsetTicksLeft);
	_zIndex = 0;
	_position = Common::Point(READ_LE_UINT16(code + kOffsetLeft), READ_LE_UINT16(code + kOffsetTop));
	uint16 baseOff = READ_LE_UINT16(code + kOffsetCode);
	_base += baseOff;
	_offset = READ_LE_UINT16(code + kOffsetOffset);
	uint16 sprite = READ_LE_UINT16(code + kOffsetMainSprite);
	_room = READ_LE_UINT16(code + kOffsetRoom);

	debugC(3, kDebugLevelFiles, "loading %s: interv %d ticks %d z%d pos%d:%d code %d offset %d sprite %d room %d", _debugInfo, _interval, _ticksLeft, _zIndex, _position.x, _position.y, baseOff, _offset, sprite, _room);

	if (sprite != 0xffff)
		setMainSprite(sprite);
}

void Actor::callBacks() {
	unless (isFine())
		while (!_callBacks.empty())
			Log.runLater(_callBacks.pop());

	foreach (RoomCallback, _roomCallbacks) {
		if (_room == Log.currentRoom() || !it->timeout) {
			Log.runLater(it->callback);
			_roomCallbacks.erase(it);
		} else
			it->timeout--;
	}
}

void Puppeteer::parse(const byte *data) {
	_actorId = READ_LE_UINT16(data + kActorId);
	_offset = READ_LE_UINT16(data + kMainCode);

	assert (kTurnAnimators == kMoveAnimators + 16);
	const byte *d = data + kMoveAnimators;
	for (int i = 0; i < 16; i++) {
		_animators[i] = READ_LE_UINT16(d);
		d += 2;
	}
}

CodePointer Puppeteer::moveAnimator(Direction d) {
	uint16 off = mainCodeOffset();
	for (int i = 0; i < 8; i++) {
		if (i + 1 == d)
			off = _animators[i];
	}

	return CodePointer(off, Log.mainInterpreter());
}

CodePointer Puppeteer::turnAnimator(Direction d) {
	uint16 off = mainCodeOffset();
	for (int i = 0; i < 8; i++) {
		if (i + 1 == d)
			off = _animators[i + 8];
	}

	return CodePointer(off, Log.mainInterpreter());
}

Direction Actor::Frame::operator-(const Actor::Frame &other) const {
	if (other._nexts[0] == _index)
		return kDirUp;
	if (other._nexts[1] == _index)
		return kDirUpRight;
	if (other._nexts[2] == _index)
		return kDirRight;
	if (other._nexts[3] == _index)
		return kDirDownRight;
	if (other._nexts[4] == _index)
		return kDirDown;
	if (other._nexts[5] == _index)
		return kDirDownLeft;
	if (other._nexts[6] == _index)
		return kDirLeft;
	return kDirUpLeft;
}

Direction operator>>(Direction _a, Direction _b) {
	int8 a(_a), b(_b);

	b -= a;

	if (b < -3)
		b += 8;
	if (b > 4)
		b -= 8;

	if (b > 0)
		a++;
	if (b < 0)
		a--;

	if (a < 1)
		a += 8;
	return *reinterpret_cast<Direction *>(&a);
}

template <int opcode>
Animation::Status Actor::opcodeHandler(){
	return Animation::opcodeHandler<opcode>();
}

template<int N>
void Actor::init_opcodes() {
	_handlers[N] = &Innocent::Actor::opcodeHandler<N>;
	init_opcodes<N-1>();
}

template<>
void Actor::init_opcodes<-1>() {}

Animation::Status Actor::op(byte opcode) {
	return (this->*_handlers[opcode])();
}

#define OPCODE(n) template<> Animation::Status Actor::opcodeHandler<n>()

OPCODE(0x14) {
	uint16 off = shift();

	debugC(1, kDebugLevelAnimation, "actor opcode 0x14: jump to 0x%04x if I'm speaking STUB", off);

	// also, some check for non-protagonists

	return kOk;
}

OPCODE(0x15) {
	debugC(1, kDebugLevelAnimation, "actor opcode 0x15: look at cursor direction if its mode is 'See' STUB");

	return kOk;
}

OPCODE(0x16) {
	byte val = embeddedByte();
	uint16 off = shift();

	debugC(1, kDebugLevelAnimation, "actor opcode 0x16: if look direction is %d then jump to 0x%04x STUB", val, off);
	error("stub");

	return kOk;
}

OPCODE(0x17) {
	byte val = embeddedByte();
	uint16 off = shift();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x17: if facing (currently %d) is %d then change code to 0x%04x", _direction, val, off);

	if (val == _direction) {
		_base = _base - _baseOffset + off;
		_baseOffset = off;
		_offset = 0;
	}

	return kOk;
}

OPCODE(0x18) {
	uint16 val = shift();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x18: set next animator to %x", val);
	_nextAnimator = val;

	return kOk;
}

OPCODE(0x23) {
	byte dir = embeddedByte();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x23: face %d", dir);

	switch (dir) {
	case 1:
		_direction = kDirUp;
		break;
	case 2:
		_direction = kDirUpRight;
		break;
	case 3:
		_direction = kDirRight;
		break;
	case 4:
		_direction = kDirDownRight;
		break;
	case 5:
		_direction = kDirDown;
		break;
	case 6:
		_direction = kDirDownLeft;
		break;
	case 7:
		_direction = kDirLeft;
		break;
	case 8:
		_direction = kDirUpLeft;
		break;
	default:
		assert(false);
	}

	return kOk;
}

OPCODE(0x24) {
	byte dir = embeddedByte();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x24: set attention needed flag to %d", dir);
	_attentionNeeded = true;

	return kOk;
}

} // end of namespace
