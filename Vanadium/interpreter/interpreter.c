#include "interpreter.h"

#define VANADIUM_STACK_SIZE 1024 * 1024 * 16	//16MB

void loadCodeToMemory(Instruction* inst, uint64_t start, uint64_t end, Environment* env) {
	if (env->instructionMemoryAllocated == 0) {
		env->instructionMemory = malloc((end - start) * sizeof(Instruction));
		env->instructionMemoryAllocated = (end - start) * sizeof(Instruction);
	}
	else if (env->instructionMemoryAllocated < (end - start) * sizeof(Instruction)) {
		env->instructionMemory = realloc(env->instructionMemory, (end - start) * sizeof(Instruction));
		env->instructionMemoryAllocated = (end - start) * sizeof(Instruction);
	}

	memcpy(env->instructionMemory, inst + (sizeof(Instruction) * start), (end - start) * sizeof(Instruction));
}

inline uint64_t allocateMemoryCell(Environment* env, uint64_t size) {
	if (env->dataMemoryAllocated == 0) {
		env->dataMemory = calloc(32, sizeof(MemoryCell));
		env->dataMemoryAllocated = 32;

		env->dataMemory[0].used = true;
		env->dataMemory[0].data = calloc(size, 1);
		return 0;
	}
	else {
		for (uint64_t i = 0; i < env->dataMemoryAllocated; i++) {
			if (!(env->dataMemory[i].used)) {
				env->dataMemory[i].used = true;
				env->dataMemory[i].data = calloc(size, 1);
				return i;
			}
		}

		//No free memory cells, allocate more
		env->dataMemory = realloc(env->dataMemoryAllocated + 8, sizeof(MemoryCell));

		uint64_t retval = env->dataMemoryAllocated - 1;
		env->dataMemoryAllocated += 8;

		env->dataMemory[retval].used = true;
		env->dataMemory[retval].data = calloc(size, 1);
		return retval;
	}
}

inline void allocateStack(Environment* env, uint64_t size) {
	env->stackcell = allocateMemoryCell(env, size);
	env->stackcell = 0;
}

inline void push(Environment* env, uint64_t data) {
	*((uint64_t*)(env->dataMemory[env->stackcell].data + env->sp)) = data;

	env->sp += sizeof(uint64_t);
}

inline uint64_t pop(Environment* env) {
	env->sp -= sizeof(uint64_t);

	return *((uint64_t*)(env->dataMemory[env->stackcell].data + env->sp));
}

inline void deleteMemoryCell(uint64_t cell, Environment* env) {
	env->dataMemory[cell].used = false;

	if (env->dataMemory[cell].data) {
		free(env->dataMemory[cell].data);
	}
}

uint64_t instrlen(Instruction* inst) {
	uint64_t len = 0;
	while (inst[len].type != HLT) {
		len++;
	}
	return ++len;
}

void loadConstStringsToMemory(Environment* env, const char** constStrings, uint64_t constStringsCount) {
	for (uint64_t i = 0; i < constStringsCount; i++) {
		//Allocate a new cell for this string and copy it to the cell's data
		strcpy(env->dataMemory[allocateMemoryCell(env, strlen(constStrings[i]) + 1)].data, constStrings[i]);
	}
}

int executeInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount) {
	if (end == 0) {
		end = instrlen(inst);
	}

	loadCodeToMemory(inst, start, end, env);
	loadConstStringsToMemory(env, constStrings, constStringsCount);

	allocateStack(env, VANADIUM_STACK_SIZE);

	int result;
	for (uint64_t i = start; i <= end; i++) {
		if ((result = step(env)) != 0) {
			if (result == 1) {
				//Hlt - still ok
				return 0;
			}
			return result;
		}
	}

	return 0;
}

inline int8_t getRegIndex(uint8_t regname) {
#ifndef VANADIUM_ERROR_CHECKING
	if (regname < 'a' || regname > 'z') {
		return -1;							//Register name out of range
	}
#endif
	return regname - 'a';
}

int step(Environment* env) {
	Instruction inst = *((Instruction*)(env->instructionMemory + (env->ip * sizeof(Instruction))));

	env->ip += 1;

	return executeInstruction(inst, env);
}

inline void updateArithmeticFlags(uint64_t res, Environment* env) {
	if (res == 0) {
		env->flags |= ENVIRONMENT_FLAG_ZERO;
		env->flags &= ~(ENVIRONMENT_FLAG_GREATER);
		env->flags &= ~(ENVIRONMENT_FLAG_LESSER);
	}
	else if (res < 0) {
		env->flags |= ENVIRONMENT_FLAG_LESSER;
		env->flags &= ~(ENVIRONMENT_FLAG_ZERO);
		env->flags &= ~(ENVIRONMENT_FLAG_GREATER);
	}
	else {
		env->flags |= ENVIRONMENT_FLAG_GREATER;
		env->flags &= ~(ENVIRONMENT_FLAG_ZERO);
		env->flags &= ~(ENVIRONMENT_FLAG_LESSER);
	}
}

int executeInstruction(Instruction inst, Environment* env) {
	switch (inst.type) {
	//Stop execution
	case HLT:
		return 1;
	case MOVRI: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		env->registers[reg] = inst.rightArg.imm64;
		break;
	}
	//Copy the value of one register to another
	case MOVRR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		env->registers[regdest] = env->registers[regsrc];
		break;
	}
	//Copy the value of an immediate to a register
	case CMPRI: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		updateArithmeticFlags((env->registers[reg]) - inst.rightArg.imm64, env);
		break;
	}
	//Compare the value of one register with the value of another
	case CMPRR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		updateArithmeticFlags((env->registers[regdest]) - (env->registers[regsrc]), env);
		break;
	}
	//Jump to specified address if the result of the previous comparison is zero (equal)
	case BRZ: {
		if (env->flags & ENVIRONMENT_FLAG_ZERO) {
			env->ip = inst.leftArg.addr;
		}
		break;
	}

	//Jump to specified address if the result of the previous comparison is not zero (not equal)
	case BRNZ: {
		if (!(env->flags & ENVIRONMENT_FLAG_ZERO)) {
			env->ip = inst.leftArg.addr;
		}
		break;
	}
	//Jump to specified address if the result of the previous comparison is less than zero
	case BRL: {
		if (env->flags & ENVIRONMENT_FLAG_LESSER) {
			env->ip = inst.leftArg.addr;
		}
		break;
	}
	//Jump to specified address if the result of the previous comparison is more than zero
	case BRG: {
		if (env->flags & ENVIRONMENT_FLAG_GREATER) {
			env->ip = inst.leftArg.addr;
		}
		break;
	}
	//Allocate a new memory cell and store its address in the specified register
	case NEW: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		env->registers[reg] = allocateMemoryCell(env, inst.rightArg.imm64);
		break;
	}
	//Delete a memory cell pointed to by an immediate
	case DELI: {
		deleteMemoryCell(inst.leftArg.imm64, env);
		break;
	}
	//Delete a memory cell pointed to by an address in a register
	case DELR: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		deleteMemoryCell(env->registers[reg], env);
		break;
	}
	//Call a function specified by a string in the immediate index
	case CALLI: {
		char* functName = env->dataMemory[inst.leftArg.imm64].data;

		if (strcmp(functName, "debugPrint") == 0) {
			printf(env->dataMemory[env->registers[0]].data);
		}
		else {
			printf("Unknown function name specified in CALLI");
			return -1;
		}

		break;
	}
	//Specify a register to be used as an index for later data access
	case BDI: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		env->di = reg;
		break;
	}
	case PUSHI: {
		push(env, inst.leftArg.imm64);
		break;
	}
	case PUSHR: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		push(env, env->registers[reg]);
		break;
	}
	case POPR: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif

		env->registers[reg] = pop(env);
		break;
	}
	}

	return 0;
}
