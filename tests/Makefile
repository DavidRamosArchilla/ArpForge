CXX ?= g++
CXXFLAGS ?= -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined

SOURCES = EngineTests.cpp ../src/engine/ArpEngine.cpp ../src/engine/Patterns.cpp

engine_tests: $(SOURCES) $(wildcard ../src/engine/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

test: engine_tests
	./engine_tests

clean:
	rm -f engine_tests

.PHONY: test clean
