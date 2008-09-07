#include "innocent/inter.h"

#include "innocent/graphics.h"

#include "common/util.h"

namespace Innocent {
#define OPCODE(num) template<> void Interpreter::opcodeHandler<num>(Argument *args[])

OPCODE(0x12) {
	// if sound is on then
	debug(1, "sound test opcode 0x12 stub");
	failedCondition();
}

OPCODE(0x2d) {
	// else
	endIf();
}

OPCODE(0x72) {
	// assign 1
	*args[0] = byte(1);
	debug(2, "%p = 1", args[0]->_ptr);
}

OPCODE(0x9d) {
	// set protagonist
	_logic->setProtagonist(*args[0]);
}

OPCODE(0xc9) {
	// set backdrop
	_graphics->setBackdrop(*args[0]);
}


} // End of namespace Innocent
