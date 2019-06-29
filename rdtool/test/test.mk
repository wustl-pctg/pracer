# Get the absolute path of where this test.mk resides;
# this is necessary because test.mk can be included by multiple 
# different files, and the current working directory is set to 
# where it is included, but not where this test.mk resides
TEST_MK_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
include $(TEST_MK_DIR)/../common.mk

RDFLAGS = -DRACEDETECT -fcilktool -fsanitize=thread
INSERTFLAGS = -DINSERTSONLY -fcilktool
LDFLAGS += -ldl -lpthread $(MALLOC) -lrt
CFLAGS += 
CXXFLAGS += 

#include $(patsubst %,%.d, $(TARGETS))
