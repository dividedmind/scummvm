#include "innocent/inter.h"

#include "innocent/logic.h"

namespace Innocent {

enum {
	kOpcodeMax = 0xfd
};

Interpreter::Interpreter(Logic *l) :
		_logic(l),
		_opcodes(this) {
}

void Interpreter::run(const byte *code, uint16 mode) {
	if (_logic->_status != Logic::kStatusOk)
		return;

	byte opcode = *code;
	if (opcode > kOpcodeMax) {
		_logic->_status = Logic::kInvalidOpcode;
		return;
	}

	const Opcode *operation = _opcodes[opcode];
	(void) operation;
}

OpcodeFactory::OpcodeFactory(Interpreter *i) : _interpreter(i) {}

const Opcode *OpcodeFactory::operator[](byte code) const {
	if (!_opcodes[code].get()) {
		Opcode * op = new Opcode;
		_opcodes[code].reset(op);
	}

	return _opcodes[code].get();
}

void Opcode::operator()(const Argument args[]) const {
	_handler(args);
}

} // End of namespace Innocent
