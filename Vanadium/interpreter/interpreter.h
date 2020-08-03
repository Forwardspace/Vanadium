#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../environment.h"
#include "../parser/parser.h"

int executeInstructions(Instruction* inst, uint64_t start, uint64_t end, Environment* env, const char** constStrings);
int executeInstruction(Instruction inst, Environment* env);