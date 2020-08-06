#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../environment.h"
#include "../parser/parser.h"

uint64_t prepareEnvironment(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount);

//Execute one instruction
int step(Environment* env);

//Scan for functions, etc. but don't execute anything
void prepareInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount);

//Note: if end is 0, the instructions will be executed until the first HLT instruction
int executeInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings, uint64_t constStringsCount);
int executeInstruction(Instruction inst, Environment* env);

uint64_t pop(Environment* env);
void push(Environment* env, uint64_t data);

//Note: if numSteps is 0, the instructions will be executed until the first HLT instruction
//int executeEnvironment(Environment* env, uint64_t numSteps);