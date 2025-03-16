# Disable built-in rules
MAKEFLAGS += -R -r

#JOBS ?= $(shell nproc)

.PHONY:	all clean run

# build directory
bdir := .build
# project directory
pdir := mmo

CXX = clang++
CXXFLAGS = -std=c++2c -I$(pdir) -DDEBUG # -Wall -Wextra
LDFLAGS = -std=c++2c

all: | $(bdir)/main main

# General dependencies
$(bdir)/main.o: $(bdir)/types.pcm $(bdir)/common.pcm
$(bdir)/main: CXXFLAGS += -fprebuilt-module-path=$(bdir)/
$(bdir)/main: LDFLAGS += $(shell pkg-config libpng --libs)

$(bdir)/main: $(bdir)/main.o $(bdir)/png_wrap.o $(bdir)/utils.o $(bdir)/common.pcm | $(bdir)
	$(CXX) $(LDFLAGS) -lstdc++fs -o $@ $^

$(bdir)/png_wrap.o: CXXFLAGS += $(shell pkg-config libpng --cflags)
#$(bdir)/png_wrap: LDFLAGS += $(shell pkg-config libpng --libs)
#$(bdir)/png_wrap: $(bdir)/png_wrap.o $(bdir)/utils.o
#	$(CXX) $(LDFLAGS) -o $@ $^

$(bdir):
	mkdir $@

clean:
	rm -rf $(bdir)

main: $(bdir)/main

run: $(bdir)/main
	@./$<

$(bdir)/%.pcm: $(pdir)/%.cppm | $(bdir)
	$(CXX) $(CXXFLAGS) --precompile -o $@ -c $^

$(bdir)/%.o: $(pdir)/%.cpp | $(bdir)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

