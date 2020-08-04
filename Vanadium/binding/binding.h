#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../environment.h"
#include "../interpreter/interpreter.h"

void registerExternalFunction(Environment* env, AbstractExternalFunctionPtr fun, const char* name);
//void removeExternalFunction(Environment* env, AbstractExternalFunctionPtr fun, const char* name);

uint64_t popNextArgument(Environment* env);
void pushReturnValue(Environment* env, uint64_t value);

//

//void pushNextArgument(uint64_t arg);

bool callInternalFunction(Environment* env, const char* name);