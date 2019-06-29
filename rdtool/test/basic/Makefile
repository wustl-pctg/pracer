include ../test.mk
TARGETS := test1 test2 test3 test4 test5 test6 test7 test8 test9 \
test10 test11 missing test_bags test_stack
# test_loop test_mem test_mem_list test_stack \
# test_stack_mem test_unalign

OBJ_DIR := ./
LIB = $(LIB_DIR)/lib$(TOOL_NAME).a

all: $(TARGETS)
$(TARGETS) : % : %_test


%_test: CFLAGS += $(RDFLAGS)
%_test: CXXFLAGS += $(RDFLAGS)
%_test: LDFLAGS += $(RUNTIME_LIB)
%_test: $(OBJ_DIR)/%.o $(LIB)
	$(CXX) $(OBJ_DIR)/$< $(LIB) $(LDFLAGS) -o $(@:_test=)

.PHONY: all clean %_test $(TARGETS)

clean:
	rm -rf $(TARGETS) *.o
