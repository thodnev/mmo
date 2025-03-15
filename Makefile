# Disable built-in rules
MAKEFLAGS += -R -r

JOBS ?= $(shell nproc)

.PHONY:	all clean run

# build directory
bdir := .build
# project directory
pdir := mmo

CXX = clang++
CXXFLAGS = -std=c++2c # -Wall -Wextra
LDFLAGS = $(CXXFLAGS)

all: | $(bdir)/main main

# General dependencies
$(bdir)/main.o: $(bdir)/types.pcm $(bdir)/common.pcm
$(bdir)/main.o: CXXFLAGS += -fmodule-file=types=$(bdir)/types.pcm -fmodule-file=common=$(bdir)/common.pcm
$(bdir)/main: $(bdir)/main.o | $(bdir)
	$(CXX) $(LDFLAGS) -o $@ $^

$(bdir)/png_wrap.o: CXXFLAGS += $(shell pkg-config libpng --cflags)
$(bdir)/png_wrap: LDFLAGS += $(shell pkg-config libpng --libs)
$(bdir)/png_wrap: $(bdir)/png_wrap.o $(bdir)/utils.o
	$(CXX) $(LDFLAGS) -o $@ $^

$(bdir):
	mkdir $@

clean:
	rm -rf $(bdir)

main: $(bdir)/main
	-ln -s $< $@

run: main
	@./$<

$(bdir)/%.pcm: $(pdir)/%.cppm | $(bdir)
	$(CXX) $(CXXFLAGS) --precompile -o $@ -c $^

$(bdir)/%.o: $(pdir)/%.cpp | $(bdir)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

