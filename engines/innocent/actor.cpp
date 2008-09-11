#include "innocent/actor.h"

#include "common/rect.h"

#include "innocent/innocent.h"
#include "innocent/logic.h"

namespace Innocent {
//

Actor::Actor(const CodePointer &code) : Animation(code, Common::Point()) {
	byte *header = code.code();
	_base = header - code.offset();
	snprintf(_debugInfo, 50, "actor at %s", +code);
	readHeader(header);
	_dir63 = 0;
	_room = 0xffff;
	_debug = false;

	Engine::instance().logic()->addAnimation(this);

	init_opcodes<37>();
}

void Actor::setAnimation(const CodePointer &anim) {
	debugC(3, kDebugLevelScript, "setting animation code of %s to %s", _debugInfo, +anim);
	_base = anim.code();
	_baseOffset = anim.offset();
	_offset = 0;
	_debugInvalid = false;
}

void Actor::setRoom(uint16 r) {
	_room = r;
}

bool Actor::isVisible() const {
	return _room == Log.currentRoom();
}

Animation::Status Actor::tick() {
	if (isVisible()) {
		Animation::Status s;
		if (_debug) gDebugLevel += 3;
			s = Animation::tick();
		if (_debug) gDebugLevel -= 3;
		return s;
	} else
		return kOk;
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

void Actor::toggleDebug() {
	_debug = !_debug;
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

	debugC(3, kDebugLevelAnimation, "actor opcode 0x14: jump to 0x%04x if I'm speaking STUB", off);

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

	return kOk;
}

OPCODE(0x17) {
	byte val = embeddedByte();
	uint16 off = shift();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x17: if facing (currently %d) is %d then change code to 0x%04x", _dir63, val, off);

	if (val == _dir63) {
		_base = _base - _baseOffset + off;
		_baseOffset = off;
		_offset = 0;
	}

	return kOk;
}

OPCODE(0x18) {
	uint16 val = shift();

	debugC(1, kDebugLevelAnimation, "actor opcode 0x18: set next animator to %d STUB", val);

	return kOk;
}

OPCODE(0x23) {
	byte dir = embeddedByte();

	debugC(3, kDebugLevelAnimation, "actor opcode 0x23: face %d", dir);

	_dir63 = dir;

	return kOk;
}

OPCODE(0x24) {
	byte dir = embeddedByte();

	debugC(1, kDebugLevelAnimation, "actor opcode 0x23: set dir65 to %d STUB", dir);

	return kOk;
}

} // end of namespace
