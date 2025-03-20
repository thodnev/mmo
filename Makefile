# Disable built-in rules, automatically parallelize
MAKEFLAGS += --no-builtin-variables --no-builtin-rules
MAKEFLAGS += --jobs=$(shell nproc)

.PHONY:	all clean run test_%

modules := rnd common # types
objs := main png_wrap utils
tests := rnd

CXX = clang++
CXXFLAGS = -std=c++2c -I$(pdir) -DDEBUG  # -Wall -Wextra -O3 -flto
LDFLAGS = -std=c++2c # -flto

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

# Tests
test_%: $(bdir)/test_%
	./$<

$(bdir)/test_rnd: $(call tst, rnd) $(call pcm,rnd) | $(bdir)
	$(CXX) $(LDFLAGS) -fprebuilt-module-path=$(bdir)/ -o $@ $^
	
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
	test -n "$(bdir)" && rm -rf "$(bdir)"

main: $(bdir)/main

run: $(bdir)/main
	@./$<

$(bdir)/%.pcm: $(pdir)/%.cppm | $(bdir)
	$(CXX) $(CXXFLAGS) --precompile -o $@ -c $^

$(bdir)/%.o: $(pdir)/%.cpp | $(bdir)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Tests
# $(bdir)/test_%: $(tdir)/test_%.cpp | $(bdir)
# 	$(CXX) $(CXXFLAGS) -o $@ $<
