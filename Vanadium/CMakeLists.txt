# CMakeList.txt : CMake project for Vanadium, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (Vanadium 
	"Vanadium.c" "Vanadium.h"
	"environment.h"
	"parser/parser.c" "parser/parser.h"
	"interpreter/interpreter.c" "interpreter/interpreter.h"
	"binding/binding.c" "binding/binding.h"
)

# TODO: Add tests and install targets if needed.
