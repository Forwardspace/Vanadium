#include "parser.h"

const char* instructions[] = {
	"movri",
	"movrr",
	"movmm",
	"movrm",
	"movmr",
	"calli",
	"callr",
	"cmpri",
	"cmprr",
	"cmpmm",
	"cmpmr",
	"cmpmi",
	"brz",
	"brnz",
	"brl",
	"brg",
	"stm",
	"lom",
	"hlt",
	"new",
	"deli",
	"delr",
	"bdi",
	"pushi",
	"pushr",
	"popr",
};

enum InstType getInstructionType(const char* str, uint64_t length) {
	for (int i = 0; i < _countof(instructions); i++) {
		if (strncmp(instructions[i], str, length) == 0) {
			return (enum InstType)i;
		}
	}

	throwParserError("unknown instruction type");
}

enum ArgType getArgType(const char* str, uint64_t length) {
	//Check if int
	if (str[0] == '#') {
		return imm64;
	}
	else if (length == 1 && (str[0] == 't' || str[0] == 'f')) {
		return immb;
	}
	else if (str[0] == '$') {
		return addr;
	}
	else if (str[0] == 'r') {
		return reg;
	}
	/*else if (str[0] == '\'') {
		return strptr;
	}*/
		
	//Todo: implement rest
}

union Argument storeArgument(const char* str, uint64_t length) {
	if (length > 63) {
		throwParserError("argument longer than 63 chars");
	}
	char temp[64] = { 0 };
	memcpy(temp, str, length);

	union Argument arg = { 0 };
	enum ArgType type = getArgType(str, length);

	switch (type) {
	case imm64:
		arg.imm64 = strtol(temp + 1, NULL, 10);
		break;
	case immb:
		arg.immb = temp[0] == 't' ? true : false;
		break;
	case addr:
		arg.addr = strtol(temp + 1, NULL, 10);
		break;
	case reg:
		arg.reg = temp[1];
	}

	//Todo: implement rest
	return arg;
}

Instruction* parseInstructionsFromStr(const char* str, uint64_t start, uint64_t end) {
	//Start with a capacity of 8 instructions
	Instruction* instrs = malloc(sizeof(Instruction) * 8);
	uint64_t instCount = 0;
	uint64_t capacity = 8;

	uint64_t i = start;
	uint64_t len = 0;
	while (true) {
		if (i >= end || str[i] == '\n') {
			if (len != 0) {
				Instruction inst = parseInstructionFromStr(str, (i - len), i);
				if (instCount + 1 > capacity) {
					Instruction* newInstrs = realloc(instrs, sizeof(Instruction) * (capacity * 2));
					if (!newInstrs) {
						throwParserError("unable to reallocate memory");
					}
					capacity *= 2;
				}
				instrs[instCount] = inst;
				instCount++;
			}

			len = 0;
		}
		else {
			len++;
		}

		i++;

		if (i > end) {
			break;
		}
	}

	//Insert a hlt instruction
	Instruction endInstr;
	endInstr.type = HLT;

	if (instCount + 1 > capacity) {
		Instruction* newInstrs = realloc(instrs, sizeof(Instruction) * (capacity + 1));
		if (!newInstrs) {
			throwParserError("unable to reallocate memory");
		}
	}

	instrs[instCount] = endInstr;

	return instrs;
}

Instruction parseInstructionFromStr(const char* str, uint64_t start, uint64_t max) {
	Instruction inst;

	//Find the instrution type (opcode)
	uint64_t end = start;
	while (str[end] != ' ' && end < max) { end++; }

	inst.type = getInstructionType(str + start, end - start);

	//Now find the arguments
	//Argument #1
	end++;
	start = end;
	while (str[end] != ' ' && end < max) { end++; }

	if (end > start) {
		//There is an argument, store it
		inst.leftArg = storeArgument(str + start, end - start);
	}

	//Argument #2
	end++;
	start = end;
	while (str[end] != ' ' && end < max) { end++; }

	if (end > start) {
		inst.rightArg = storeArgument(str + start, end - start);
	}

	return inst;
}

void __declspec(noreturn) throwParserError(const char* str) {
	printf("Vanadium: instruction parser error: %s", str);
	exit(-1);
}