INLUCDE_DIR = 

C_FLAGS =	-Wall -O2 -Wno-unused \
			-D_REENTRANT -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS 

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c, %.o, $(SOURCES))

COMPILE_TOOLS = $(TOOL_PREFIX)gcc
AR = $(TOOL_PREFIX)ar
RANLIB = $(TOOL_PREFIX)ranlib

LIB = libposix_if.a

all : $(LIB) 

$(LIB) : $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
	
%.o:%.c
	$(COMPILE_TOOLS) $(C_FLAGS) $(INLUCDE_DIR) -c $^ -o $@

clean :
	rm -f *.o  $(LIB)
