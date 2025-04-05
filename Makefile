# Disable built-in rules, automatically parallelize
MAKEFLAGS += --no-builtin-variables --no-builtin-rules
MAKEFLAGS += --jobs=$(shell nproc)

.PHONY:	all clean run test_%

modules := types common rnd pthfind
objs := main png_wrap utils
tests := rnd

CXX = clang++
CXXFLAGS += -std=c++2c -I$(pdir) -fprebuilt-module-path=$(bdir)/
CXXFLAGS += -O3 -flto -Wall -Wextra -ggdb3 #-pg
LDFLAGS = -std=c++2c -O3 -flto -I$(pdir) -fprebuilt-module-path=$(bdir)/ -ggdb3 # -pg
LDLIBS = $(shell pkg-config libpng --libs)
# CXXFLAGS += -DDEBUG
# LDFLAGS += -DDEBUG

# build directory
bdir := .build
# project directory
pdir := mmo
# tests directory
tdir := tests

define pcm
	$(addprefix $(bdir)/,$(addsuffix .pcm,$(1)))
endef

define obj
	$(addprefix $(bdir)/,$(addsuffix .o,$(1)))
endef

define tst
	$(addprefix $(tdir)/test_,$(addsuffix .cpp,$(1)))
endef

pcm_modules := $(call pcm,$(modules))
obj_files := $(call obj,$(objs))


all: | $(bdir)/main main

# Test dependencies
$(bdir)/test_pthfind: $(call obj,utils) $(call pcm,common types pthfind)
$(bdir)/test_pathfind_algo: $(call obj,utils) $(call pcm,common types)
$(bdir)/test_path: $(call pcm,types)
$(bdir)/test_rnd: $(call pcm,rnd)
$(bdir)/test_binmask: $(call pcm,common) $(call obj,utils)

# Intermodule dependencies
$(call pcm,common): | $(call pcm,types)
$(call pcm,pthfind): | $(call pcm,common)

# General dependencies
$(bdir)/main.o: $(pcm_modules)
$(bdir)/main: LDFLAGS += 

$(bdir)/main: $(obj_files) $(pcm_modules) | $(bdir)
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

$(bdir)/png_wrap.o: CXXFLAGS += $(shell pkg-config libpng --cflags)
#$(bdir)/png_wrap: LDFLAGS += $(shell pkg-config libpng --libs)
#$(bdir)/png_wrap: $(bdir)/png_wrap.o $(bdir)/utils.o
#	$(CXX) $(LDFLAGS) -o $@ $^

# FIXME: header dependencies
$(call obj,utils): $(pdir)/utils.hpp

$(bdir):
	mkdir $@

clean:
	test -n "$(bdir)" && rm -rf "$(bdir)"

main: $(bdir)/main

run: $(bdir)/main
	@./$<

$(bdir)/%.pcm: $(pdir)/%.cppm | $(bdir)
	$(CXX) $(CXXFLAGS) --precompile -o $@ -c $^ $(EXTRAFLAGS)

$(bdir)/%.o: $(pdir)/%.cpp | $(bdir)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Tests
$(bdir)/test_%: $(call tst, %) | $(bdir)
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

test_%: $(bdir)/test_%
	@echo -n "Running "
	$<	
