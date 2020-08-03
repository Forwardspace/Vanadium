# CMakeList.txt : CMake project for Vanadium, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (Vanadium 
	"Vanadium.c" "Vanadium.h"
	"parser/parser.c" "parser/parser.h"
	"interpreter/interpreter.c" "interpreter/interpreter.h"
	"environment.h"
)

# TODO: Add tests and install targets if needed.
