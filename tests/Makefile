# Host-side tests for the pure logic shared with the firmware. Run with ./gg test.
# Each test_*.cpp is one executable. Shared sources go in SRCS, include dirs in INC.
CXX      ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1 -g
INC      := -I../libraries/GaggiaProtocol/src -I../gaggiano-v2 -I../gaggiano-controller-v1
SRCS     := ../libraries/GaggiaProtocol/src/gaggia_protocol.cpp ../gaggiano-v2/profile_format.cpp ../gaggiano-v2/sequencer.cpp ../gaggiano-v2/history.cpp ../gaggiano-v2/timezones.cpp ../gaggiano-controller-v1/steam_assist.cpp

TESTS := $(wildcard test_*.cpp)
BINS  := $(patsubst %.cpp,build/%,$(TESTS))

.PHONY: all run clean
all: run

build:
	@mkdir -p build

build/%: %.cpp check.h $(SRCS) | build
	$(CXX) $(CXXFLAGS) $(INC) $< $(SRCS) -o $@

run: $(BINS)
	@status=0; for t in $(BINS); do ./$$t || status=1; done; exit $$status

clean:
	rm -rf build
