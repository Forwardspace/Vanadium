#include "Vanadium.h"

#define VANADIUM_STANDALONE

#ifdef VANADIUM_STANDALONE
int main() {
	const char* instrs = {
		"movri ra #1\ncalli #0"
	};
	
	const char* strings[] = {
		"debugPrint",
		"Hello World!"
	};

	Instruction* parsed = parseInstructionsFromStr(instrs, 0, strlen(instrs));

	Environment env = { 0 };
	executeInstructions(parsed, 0, 0, &env, strings, _countof(strings));

	free(parsed);

	return 0;
}
#endif