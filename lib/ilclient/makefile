INLUCDE_DIR = -I./ -I/opt/vc/include \
			-I/opt/vc/include/interface/mmal \
			-I/opt/vc/include/interface/vchi \
			-I/opt/vc/include/interface/vchiq_arm \
			-I/opt/vc/include/interface/vcos \
			-I/opt/vc/include/interface/vctypes \
			-I/opt/vc/include/interface/vmcs_host \
			-I/opt/vc/include/interface/vmcs_host/linux \
			-I/opt/vc/include/interface/vcos/pthreads \
			-I/opt/vc/include/interface/vcos/generic

C_FLAGS =	-Wall -O2 -Wno-unused \
			-D_REENTRANT -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DHAVE_LIBOPENMAX=2 \
			-DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM 

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c, %.o, $(SOURCES))

ROOTDIR = ../..
include $(ROOTDIR)/rules

COMPILE_TOOLS = $(TOOL_PREFIX)gcc
AR = $(TOOL_PREFIX)ar
RANLIB = $(TOOL_PREFIX)ranlib

LIB = libilclient_if.a

all : $(LIB) 

$(LIB) : $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
	cp -f $@ $(APP_COMMON_LIB_DIR)
	chmod 777 $(APP_COMMON_LIB_DIR)/$(LIB)
	
%.o:%.c
	$(COMPILE_TOOLS) $(C_FLAGS) $(INLUCDE_DIR) -c $^ -o $@

clean :
	rm -f *.o  $(LIB)
