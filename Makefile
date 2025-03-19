# Disable built-in rules, automatically parallelize
MAKEFLAGS += --no-builtin-variables --no-builtin-rules
MAKEFLAGS += --jobs=$(shell nproc)

.PHONY:	all clean run

modules := rnd common # types
objs := main png_wrap utils

CXX = clang++ -fno-inline
CXXFLAGS = -std=c++2c -I$(pdir) -DDEBUG # -Wall -Wextra
LDFLAGS = -std=c++2c

# build directory
bdir := .build
# project directory
pdir := mmo

define pcm
	$(addprefix $(bdir)/,$(addsuffix .pcm,$(1)))
endef

define obj
	$(addprefix $(bdir)/,$(addsuffix .o,$(1)))
endef

pcm_modules := $(call pcm,$(modules))
obj_files := $(call obj,$(objs))


all: | $(bdir)/main main

# General dependencies
$(bdir)/main.o: $(pcm_modules)

$(bdir)/main: CXXFLAGS += -fprebuilt-module-path=$(bdir)/
$(bdir)/main: LDFLAGS += $(shell pkg-config libpng --libs)

$(bdir)/main: $(obj_files) $(pcm_modules) | $(bdir)
	$(CXX) $(LDFLAGS) -o $@ $^

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

