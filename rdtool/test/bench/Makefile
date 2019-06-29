TARGETS = matmul heat fft cholesky cilksort fib fibx strassen
TARGETS += knapsack lu nqueens qsort rectmul #matmul-int
OBJ_DIR_PREFIX := obj
BIN_DIR := bin
MODES = brd base icc insert cilksan cstats prof istats

ifeq ($(mode), brd)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_brd)
	CFLAGS += $(RDFLAGS)
	CXXFLAGS += $(RDFLAGS)
	LIB = $(LIB_DIR)/lib$(TOOL_NAME).a
	LDFLAGS += $(RUNTIME_LIB)
else ifeq ($(mode), base)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_base)
	LDFLAGS += $(RUNTIME_LIB)
else ifeq ($(mode), icc)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_icc)
#	CFLAGS += -fp-model precise -no-vec
#	CXXFLAGS += -fp-model precise -no-vec
	LDFLAGS += $(MALLOC)
	LDFLAGS += -lcilkrts -Wl,-rpath,$(INTEL_LIB)
else ifeq ($(mode), insert)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_insert)
	CFLAGS += $(INSERTFLAGS) 
	CXXFLAGS += $(INSERTFLAGS)
	LDFLAGS += $(RUNTIME_LIB)
	LIB = $(LIB_DIR)/lib$(TOOL_NAME).a
else ifeq ($(mode), cilksan)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_cilksan)
else ifeq ($(mode), cstats)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_cstats)
	CFLAGS += -fcilktool -fsanitize=thread
	CXXFLAGS += -fcilktool -fsanitize=thread
	LDFLAGS += $(RUNTIME_LIB)
	LIB = $(LIB_DIR)/libcilkstats.a
else ifeq ($(mode), istats)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_istats)
	CFLAGS += $(INSERTFLAGS) -DSTATS=1
	CXXFLAGS += $(INSERTFLAGS) -DSTATS=1
	LDFLAGS += $(RUNTIME_LIB)
	LIB = $(LIB_DIR)/lib$(TOOL_NAME).a
else ifeq ($(mode), prof)
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_prof)
	CFLAGS += $(RDFLAGS) -DSTATS=2
	CXXFLAGS += $(RDFLAGS) -DSTATS=2
	LDFLAGS += $(RUNTIME_LIB)
	LIB = $(LIB_DIR)/librd.prof.a
else ifeq ($(mode), asm) # Show the asm generated for brd
	OBJ_DIR := $(OBJ_DIR_PREFIX:=_asm)
ifeq ($(sanitize), yes)	
	CFLAGS += $(RDFLAGS)
	CXXFLAGS += $(RDFLAGS)
else
	CFLAGS += $(INSERTFLAGS) 
	CXXFLAGS += $(INSERTFLAGS)
endif
	CFLAGS += -S -emit-llvm
	CXXFLAGS += -S -emit-llvm
endif
include ../test.mk
# common.mk (included by test.mk) resets CC,CXX
ifeq ($(mode), icc)
	CC = $(ICC)
	CXX = $(ICC)
# I am not sure what vectorization icc does and what clang doesn't
# do, but there is some different in performance
#	-no-vec
	CFLAGS = -O3 -DNDEBUG -march=native -m64 -g -std=c99 -no-vec
	CXXFLAGS = -O3 -DNDEBUG -march=native -m64 -g -std=c++11 -no-vec
endif

most: brd base insert icc cilksan cstats
all: most istats prof
asm:
	$(MAKE) $(TARGETS) mode=asm
brd:
	$(MAKE) $(TARGETS) mode=brd
base:
	$(MAKE) $(TARGETS) mode=base
istats:
	$(MAKE) $(TARGETS) mode=istats
prof:
	$(MAKE) $(TARGETS) mode=prof
insert:
	$(MAKE) $(TARGETS) mode=insert
icc:
	$(MAKE) $(TARGETS) mode=icc
cilksan:
	$(MAKE) $(TARGETS:=_cilksan) mode=cilksan
cstats:
	$(MAKE) $(TARGETS) mode=cstats

$(BIN_DIR):
	mkdir $(BIN_DIR)

%_cilksan: $(CILKSAN_HOME) $(BIN_DIR)
	$(MAKE) -C $(CILKSAN_HOME)/src
	$(MAKE) -C $(CILKSAN_HOME)/test/bench $(@:_cilksan=)
	cp $(CILKSAN_HOME)/test/bench/$(@:_cilksan=) $(BIN_DIR)/$@


ifdef mode
fib: $(OBJ_DIR)/fib.o $(OBJ_DIR)/getoptions.o
fibx: $(OBJ_DIR)/fibx.o $(OBJ_DIR)/getoptions.o
qsort: $(OBJ_DIR)/qsort.o $(OBJ_DIR)/getoptions.o
nqueens: $(OBJ_DIR)/nqueens.o $(OBJ_DIR)/getoptions.o
matmul: $(OBJ_DIR)/matmul.o $(OBJ_DIR)/getoptions.o
heat: $(OBJ_DIR)/heat.o $(OBJ_DIR)/getoptions.o
fft: $(OBJ_DIR)/fft.o $(OBJ_DIR)/getoptions.o
cholesky: $(OBJ_DIR)/cholesky.o $(OBJ_DIR)/getoptions.o
cilksort: $(OBJ_DIR)/cilksort.o $(OBJ_DIR)/getoptions.o
strassen: $(OBJ_DIR)/strassen.o $(OBJ_DIR)/getoptions.o
knapsack: $(OBJ_DIR)/knapsack.o $(OBJ_DIR)/getoptions.o
lu: $(OBJ_DIR)/lu.o $(OBJ_DIR)/getoptions.o
rectmul: $(OBJ_DIR)/rectmul.o $(OBJ_DIR)/getoptions.o
test: $(OBJ_DIR)/test.o

# matmul-int: CXXFLAGS += -DREAL=int
# matmul-int: $(OBJ_DIR)/matmul-int.o

ifeq ($(mode), asm)
ifeq ($(sanitize), yes)
$(TARGETS):
	mv $< $(<:.o=.sanitized.asm)
else
$(TARGETS):
	mv $< $(<:.o=.normal.asm)
endif
else
$(TARGETS): export USE_GOLD = "yes"
$(TARGETS): | $(BIN_DIR)
	$(CXX) $< $(OBJ_DIR)/getoptions.o $(LIB) $(LDFLAGS) -o $(BIN_DIR)/$@_$(mode)
endif
else
$(TARGETS):
	$(MAKE) $@ mode=brd
endif

%_base:
	$(MAKE) $(@:_base=) mode=base
%_brd:
	$(MAKE) $(@:_brd=) mode=brd
%_prof:
	$(MAKE) $(@:_prof=) mode=prof
%_insert:
	$(MAKE) $(@:_insert=) mode=insert
%_cstats:
	$(MAKE) $(@:_cstats=) mode=cstats
%_istats:
	$(MAKE) $(@:_istats=) mode=istats
%_icc:
	$(MAKE) $(@:_icc=) mode=icc

%_all:
	$(MAKE) $(@:_all=_base)
	$(MAKE) $(@:_all=_insert)
	$(MAKE) $(@:_all=_brd)
	$(MAKE) $(@:_all=_icc)
	$(MAKE) $(@:_all=_cilksan)

.PHONY: clean modeclean %_clean $(TARGETS) %_brd %_base %_insert %_icc %_prof %_cstats %_istats %_cilksan

%_clean:
	rm -rf $(BIN_DIR)/$(@:_clean=)_*
	for m in $(MODES); do \
		rm -f $(OBJ_DIR_PREFIX)_$$m/$(@:_clean=.o); \
	done

clean:
	$(MAKE) modeclean mode=base
	$(MAKE) modeclean mode=brd
	$(MAKE) modeclean mode=insert

modeclean:
	rm -rf *.o $(OBJ_DIR) $(BIN_DIR)/*_$(mode)
