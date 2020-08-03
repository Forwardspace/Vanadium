#pragma once
#include "interpreter/interpreter.h"

#define ENVIRONMENT_FLAG_ZERO 0b1
#define ENVIRONMENT_FLAG_GREATER 0b10
#define ENVIRONMENT_FLAG_LESSER 0b100

typedef struct MemoryCell_ {
	uint8_t* data;
	bool used;
} MemoryCell;

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
} Environment;