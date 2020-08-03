#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

//Opcode + arguments
//arguments: I(mmediate), R(egister), M(emory)

enum InstType {
	MOVRI,
	MOVRR,
	MOVMM,
	MOVRM,
	MOVMR,
	CALLI,
	CALLR,
	CMPRI,
	CMPRR,
	CMPMM,
	CMPMR,
	CMPMI,
	BRZ,
	BRNZ,
	BRL,
	BRG,
	STM,
	LOM,
	HLT,
	NEW,
	DELI,
	DELR,
	BDI,
	PUSHI,
	PUSHR,
	POPR,
};

enum ArgType {
	imm64,
	imm32,
	imm16,
	imm8,
	immf,
	immd,
	imms,
	immb,
	addr,
	reg,
	strptr
};

union Argument {
	uint64_t imm64;
	uint32_t imm32;
	uint16_t imm16;
	uint8_t imm8;
	float immf;
	double immd;
	char* imms;
	bool immb;
	uint64_t addr;
	uint8_t reg;
};

typedef struct Instruction_ {
	enum InstType type;
	union Argument leftArg;
	union Argument rightArg;
} Instruction;

Instruction* parseInstructionsFromStr(const char* str, uint64_t start, uint64_t end);
Instruction parseInstructionFromStr(const char* str, uint64_t start, uint64_t max);

void __declspec(noreturn) throwParserError(const char* err);