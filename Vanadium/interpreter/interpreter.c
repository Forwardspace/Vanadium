#include "interpreter.h"

#define VANADIUM_STACK_SIZE 1024 * 1024 * 16	//16MB

void loadCodeToMemory(Instruction* __restrict inst, uint64_t start, uint64_t end, Environment* __restrict env) {
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

inline uint64_t allocateMemoryCell(Environment* __restrict env, uint64_t size) {
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
	env->dataMemory[env->stackcell].size = size;
}

void push(Environment* env, uint64_t data) {
	*((uint64_t*)(env->dataMemory[env->stackcell].data + env->sp)) = data;

	env->sp += sizeof(uint64_t);
}

uint64_t pop(Environment* env) {
	env->sp -= sizeof(uint64_t);

	return *((uint64_t*)(env->dataMemory[env->stackcell].data + env->sp));
}

inline void deleteMemoryCell(uint64_t cell, Environment* env) {
	env->dataMemory[cell].used = false;

	if (env->dataMemory[cell].data) {
		free(env->dataMemory[cell].data);
	}
}

uint64_t instrlen(Instruction* __restrict inst) {
	uint64_t len = 0;
	while (inst[len].type != END) {
		len++;
	}
	return ++len;
}

void loadConstStringsToMemory(Environment* env, const char** constStrings, uint64_t constStringsCount) {
	for (uint64_t i = 0; i < constStringsCount; i++) {
		//Allocate a new cell for this string and copy it to the cell's data
		uint64_t size = strlen(constStrings[i]) + 1;
		uint64_t memoryCell = allocateMemoryCell(env, size);

		strcpy(env->dataMemory[memoryCell].data, constStrings[i]);
		env->dataMemory[memoryCell].size = size;
	}
}

uint64_t prepareEnvironment(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount) {
	if (end == 0) {
		end = instrlen(inst);
	}

	loadCodeToMemory(inst, start, end, env);
	loadConstStringsToMemory(env, constStrings, constStringsCount);
	 
	allocateStack(env, VANADIUM_STACK_SIZE);

	return end;
}

inline void declareInternalFunction(Environment* __restrict env, uint64_t ip, const char* __restrict name, uint64_t nameLength) {
	if (env->internalFunctionsAllocated == 0) {
		env->internalFunctions = malloc(sizeof(AbstractExternalFunction) * 8);
		env->internalFunctionsAllocated = 8;
	}
	else if (env->numInternalFunctions + 1 > env->internalFunctionsAllocated) {
		env->internalFunctionsAllocated += 8;
		env->internalFunctions = realloc(env->internalFunctions, sizeof(AbstractExternalFunction) * env->internalFunctionsAllocated);
	}

	AbstractInternalFunction funct;
	funct.location = ip;
	funct.name = calloc(nameLength + 1, 1);

	memcpy(funct.name, name, nameLength);

	env->internalFunctions[env->numInternalFunctions] = funct;
	env->numInternalFunctions++;
}

void preprocessInstructions(Environment* env, Instruction* inst, uint64_t start, uint64_t end) {
	for (uint64_t i = start; i < end; i++) {
		switch (inst[i].type) {
		case DECLFUNCT:
			declareInternalFunction(env, i + 1, env->dataMemory[inst[i].leftArg.addr].data, env->dataMemory[inst[i].leftArg.addr].size);
			break;
		}
	}
}

void prepareInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount) {
	end = prepareEnvironment(inst, start, end, env, constStrings, constStringsCount);
	preprocessInstructions(env, inst, start, end);
}

int executeInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount) {
	prepareInstructions(inst, start, end, env, constStrings, constStringsCount);

	int result;
	for (uint64_t i = start; i <= end; i++) {
		if ((result = step(env)) != 0) {
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

inline void callExternalFunction(Environment* __restrict env, const char* __restrict name, uint64_t size) {
	for (uint64_t i = 0; i < env->numExternalFunctions; i++) {
		if (strncmp(env->externalFunctions[i].name, name, size) == 0) {
			env->externalFunctions[i].ptr(env);
			
			return;
		}
	}

	//Function not found - set error flag
	env->flags |= ENVIRONMENT_FLAG_INSTR_FAILED;
}

////////////////////////////////////////////////////////////

//Note: the following function is quite unreadable to minimize its performance overhead
//The pseudocode for each instruction is, however, simple:
//
//executeInstruction(instruction, environment):
//	switch (instruction type):
//		...
//		case RELEVANTINSTRUCTION:
//			getRegister(s)FromInstruction()			(optional)
//			doSomethingBasedOnInstructionType()
//			modifyFlags();							(optional)
//
//			break
//		...
int executeInstruction(Instruction inst, Environment* env) {
	switch (inst.type) {
	//Stop execution
	case HLT:
	case END:
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
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		env->registers[regdest] = env->registers[regsrc];
		break;
	}
	//Copy all data from one memory cell pointed to by one register to another (pointed to by another register)
	case MOVMMRR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		//Allocate enough space for the copy
		if (env->dataMemory[env->registers[regdest]].data != NULL) {
			env->dataMemory[env->registers[regdest]].data = realloc(env->dataMemory[env->registers[regdest]].data, env->dataMemory[env->registers[regsrc]].size);
		}
		else {
			env->dataMemory[env->registers[regdest]].data = malloc(env->dataMemory[regdest].size);
		}
		env->dataMemory[env->registers[regdest]].size = env->dataMemory[env->registers[regsrc]].size;

		memcpy(env->dataMemory[env->registers[regdest]].data, env->dataMemory[env->registers[regsrc]].data, env->dataMemory[env->registers[regsrc]].size);

		break;
	}
	case MOVMMII: {
		if (env->dataMemory[inst.leftArg.addr].data != NULL) {
			env->dataMemory[inst.leftArg.addr].data = realloc(env->dataMemory[inst.leftArg.addr].data, env->dataMemory[inst.rightArg.addr].size);
		}
		else {
			env->dataMemory[inst.leftArg.addr].data = malloc(env->dataMemory[inst.leftArg.addr].size);
		}
		env->dataMemory[inst.leftArg.addr].size = env->dataMemory[inst.leftArg.addr].size;

		memcpy(env->dataMemory[inst.leftArg.addr].data, env->dataMemory[inst.rightArg.addr].data, env->dataMemory[inst.rightArg.addr].size);

		break;
	}
	//Copy all data from a register to a memory cell pointed to by a value in a register
	case MOVRMR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		if (env->dataMemory[env->registers[regdest]].data != NULL) {
			env->dataMemory[env->registers[regdest]].data = realloc(env->dataMemory[env->registers[regdest]].data, sizeof(uint64_t));
		}
		else {
			env->dataMemory[env->registers[regdest]].data = malloc(sizeof(uint64_t));
		}
		env->dataMemory[env->registers[regdest]].size = sizeof(uint64_t);

		*((uint64_t*)(env->dataMemory[env->registers[regdest]].data)) = env->registers[regsrc];

		break;
	}
	//Copy an immediate to a memory cell pointed to by a value in a register
	case MOVMIR: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		if (env->dataMemory[env->registers[reg]].data != NULL) {
			env->dataMemory[env->registers[reg]].data = realloc(env->dataMemory[env->registers[reg]].data, sizeof(uint64_t));
		}
		else {
			env->dataMemory[env->registers[reg]].data = malloc(sizeof(uint64_t));
		}
		env->dataMemory[env->registers[reg]].size = sizeof(uint64_t);

		*((uint64_t*)(env->dataMemory[env->registers[reg]].data)) = inst.rightArg.imm64;

		break;
	}
	//Copy a byte to a memory cell (pointed to by a register) to a position indexed by the data index
	case MOVBMIR: {
		int8_t reg = getRegIndex(inst.leftArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (reg == -1) {
			return -1;
		}
#endif
		env->dataMemory[env->registers[reg]].data[env->registers[env->di]] = inst.rightArg.imm8;
		break;
	}
	//Copy a byte stored in a register to a memory cell (pointed to by another register) to a position indexed by the data index
	case MOVBMRR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		env->dataMemory[env->registers[regdest]].data[env->registers[env->di]] = (uint8_t)(env->registers[regsrc]);
		break;
	}
	//Copy a byte stored in a memory cell pointed to by the data index to a register
	case MOVBRMR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
#ifndef VANADIUM_NO_ERROR_CHECKING
		if (regdest == -1 || regsrc == -1) {
			return -1;
		}
#endif
		env->registers[regdest] = env->dataMemory[env->registers[regsrc]].data[env->registers[env->di]];

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
	//Compare the value of one register with the value of another by subtraction
	case CMPRR: {
		int8_t regdest = getRegIndex(inst.leftArg.reg);
		int8_t regsrc = getRegIndex(inst.rightArg.reg);
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
	case CALLEXTI: {
		char* functName = env->dataMemory[inst.leftArg.imm64].data;

		callExternalFunction(env, functName, env->dataMemory[inst.leftArg.imm64].size);

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
