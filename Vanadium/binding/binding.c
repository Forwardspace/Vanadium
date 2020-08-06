#include "binding.h"

void registerExternalFunction(Environment* env, AbstractExternalFunctionPtr fun, const char* name) {
	if (env->externalFunctionsAllocated == 0) {
		env->externalFunctions = malloc(sizeof(AbstractExternalFunction) * 8);
	}
	else if (env->externalFunctionsAllocated < env->externalFunctions + 1) {
		env->externalFunctionsAllocated += 8;
		env->externalFunctions = realloc(env->externalFunctions, sizeof(AbstractExternalFunction) * env->externalFunctionsAllocated);
	}

	AbstractExternalFunction extFun = { 0 };
	extFun.name = name;
	extFun.ptr = fun;

	env->externalFunctions[env->numExternalFunctions] = extFun;
	env->numExternalFunctions++;
}

uint64_t popNextArgument(Environment* env) {
	return pop(env);
}

void pushReturnValue(Environment* env, uint64_t value) {
	push(env, value);
}

bool callInternalFunction(Environment* env, const char* name) {
	for (uint64_t i = 0; i < env->numInternalFunctions; i++) {
		if (strcmp(env->internalFunctions[i].name, name) == 0) {
			env->ip = env->internalFunctions[i].location;

			//Run until hlt or otherwise done
			while (step(env) == 0);

			return true;
		}
	}

	//Function not found
	return false;
}