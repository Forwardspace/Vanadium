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
		"bdi ra\n"			//Setup the data index for later

		"movri rb #1\n"		//Copy the letter 'A' to the 5th position in "Hello World!"
		"movri ra #5\n"
		"movbmi rb #65\n"

		"pushi #1\n"		//Print the new string
		"callexti $0\n"
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