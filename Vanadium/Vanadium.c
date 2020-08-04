#include "Vanadium.h"
#include "binding/binding.h"

#define VANADIUM_STANDALONE

#ifdef VANADIUM_STANDALONE
void debugPrint(Environment* env) {
	uint64_t stringPtr = popNextArgument(env);

	printf(env->dataMemory[stringPtr].data);
}

int main() {
	const char* instrs = {
		"pushi #1\ncallexti #0"
	};
	
	const char* strings[] = {
		"debugPrint",
		"Hello World!"
	};

	Instruction* parsed = parseInstructionsFromStr(instrs, 0, strlen(instrs));
	Environment env = { 0 };

	registerExternalFunction(&env, debugPrint, "debugPrint");

	executeInstructions(parsed, 0, 0, &env, strings, _countof(strings));

	free(parsed);

	return 0;
}
#endif