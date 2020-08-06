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
		"declfunct $3\n"	//Prints Hello World
		"bdi ra\n"
		
		"pushi #1\n"
		"callexti $0\n"
		"hlt\n"

		"declfunct $2\n"	//Prints HelloAWorld
		"bdi ra\n"

		"movri rb #1\n"
		"movri ra #5\n"
		"movbmi rb #65\n"

		"pushi #1\n"
		"callexti $0\n"
		"hlt\n"
	};
	
	const char* strings[] = {
		"debugPrint",
		"Hello World!",
		"printHelloAWorld",
		"printNormal"
	};

	Instruction* parsed = parseInstructionsFromStr(instrs, 0, strlen(instrs));
	
	Environment env = { 0 };
	registerExternalFunction(&env, debugPrint, "debugPrint");
	
	prepareInstructions(parsed, 0, 0, &env, strings, _countof(strings));

	callInternalFunction(&env, "printNormal");
	callInternalFunction(&env, "printNormal");
	callInternalFunction(&env, "printHelloAWorld");

	free(parsed);

	return 0;
}
#endif