#pragma once

#define ENVIRONMENT_FLAG_ZERO 0b1
#define ENVIRONMENT_FLAG_GREATER 0b10
#define ENVIRONMENT_FLAG_LESSER 0b100
#define ENVIRONMENT_FLAG_INSTR_FAILED 0b1000

typedef struct MemoryCell_ {
	uint8_t* data;
	bool used;
} MemoryCell;

struct Environment_;

typedef void(*AbstractExternalFunctionPtr)(struct Environment_*);
typedef struct AbstractExternalFunction_ {
	const char* name;
	AbstractExternalFunctionPtr ptr;
} AbstractExternalFunction;

typedef struct AbstractInternalFunction_ {
	uint64_t name;		//Index of string table
	uint64_t location;
} AbstractInternalFunction;

typedef struct Environment_ {
	uint8_t* instructionMemory;
	uint64_t instructionMemoryAllocated;

	MemoryCell* dataMemory;
	uint64_t dataMemoryAllocated;

	uint64_t registers[26];	//(a-z)
	
	uint64_t ip;	//Instruction pointer
	uint8_t di;		//Data index register
	uint16_t flags;

	uint64_t stackcell;
	uint64_t sp;

	AbstractExternalFunction* externalFunctions;
	uint64_t externalFunctionsAllocated;
	uint64_t numExternalFunctions;

	AbstractInternalFunction* internalFunctions;
	uint64_t internalFunctionsAllocated;
	uint64_t numInternalFunctions;

} Environment;