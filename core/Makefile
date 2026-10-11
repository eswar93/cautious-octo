# Host-side tests for the portable C++ core.  Windows: use WSL2 or MSYS2/MinGW.
CXX ?= g++
# Same restrictions the firmware build uses: no exceptions, no RTTI.
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -fno-exceptions -fno-rtti
test: test/test_core.cpp window.hpp fixed_queue.hpp link_monitor.hpp predictor.hpp pipeline.hpp model_weights.hpp
	$(CXX) $(CXXFLAGS) -o test_core test/test_core.cpp
	./test_core
clean:
	rm -f test_core test_core.exe
