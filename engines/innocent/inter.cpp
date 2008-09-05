#include "innocent/inter.h"

#include "common/endian.h"
#include "common/util.h"

#include "innocent/logic.h"

namespace Innocent {

enum Debug {
	kOpcodeDetails = 3
};

enum {
	kOpcodeMax = 0xfd
};

class Uint16Argument : public Argument {
public:
	Uint16Argument(byte *ptr) : _ptr(ptr) {}
	uint16 value() const { return READ_LE_UINT16(_ptr); }

private:
	byte *_ptr;
};

Interpreter::Interpreter(Logic *l) :
		_logic(l) {
}

void Interpreter::run(byte *code, uint16 mode) {
	_code = code;
	_mode = mode;
	run();
}

void Interpreter::run() {
	uint16 abort = 0, abort_exec0 = 0, abort_exec1 = 0;

	while (_logic->_status == Logic::kStatusOk) {
		if (_logic->_status != Logic::kStatusOk)
			return;

		byte opcode = *(_code++);
		if (opcode > kOpcodeMax) {
			_logic->_status = Logic::kInvalidOpcode;
			return;
		}

		uint8 nargs = _argumentsCounts[opcode];

		_currentCode = opcode;
		OpcodeHandler handler = _handlers[opcode];
		if (!handler)
			handler = &Interpreter::defaultHandler;

		Argument *args[6];

		for (uint i = 0; i < nargs; i++)
			args[i] = getArgument();

		if (nargs == 0)
			_code += 2;

		if (opcode == 0x2c || opcode == 0x2d || opcode == 1 || !abort)
			(*handler)(this, args);
		else if (opcode != 0 && opcode < 0x26)
			abort++;

		for (int i = 0; i < nargs; i++)
			delete args[i];

		if (abort || abort_exec0 || abort_exec1)
			break;
	}
}

void Interpreter::defaultHandler(Interpreter *self, Argument /*args*/*[]) {
	warning("unhandled opcode %02x", self->_currentCode);
}

enum ArgumentTypes {
	kArgumentImmediate = 1
};

Argument *Interpreter::getArgument() {
	uint8 argument_type = _code[0];
	_code += 1;
	debug(kOpcodeDetails, "argument type %02x", argument_type);

	byte *ptr;
	switch (argument_type) {
		case kArgumentImmediate:
			debug(kOpcodeDetails, "immediate, value 0x%04x", READ_LE_UINT16(_code));
			ptr = _code;
			_code += 2;
			return new Uint16Argument(ptr);
		default:
			error("don't know how to handle argument type 0x%02x", argument_type);
	}
}

const uint8 Interpreter::_argumentsCounts[] = {
	#include "opcodes_nargs.data"
};

} // End of namespace Innocent
