###################################################
CONFIG_DBG=n

TOOL_DIR=
TOOL_PREFIX=
ROOT_DIR:=$(shell pwd)
TARGET_PACKAGE_DIR=$(ROOT_DIR)/
TARGET_NAME=iPlain
DBG_NAME=_dbg
###################################################
cmd = @echo '  $(echo_cmd_$(1))' && $(cmd_$(1))
echo_cmd_clean  = CLEAN  $(2)
cmd_clean  = make --quiet -C $(2) clean
echo_cmd_make   = MAKE  $(2)
cmd_make   = make --quiet -C $(2)
echo_cmd_cc_o_c = CC  $@
cmd_cc_o_c = $(COMPILE) $(COMPILE_FLAGS) -c -o $@ $<
echo_cmd_link   = LINK  $@
cmd_link   = $(LINK) -o $@ $^ $(RELLDFLAGS)
echo_cmd_strip  = STRIP  $@
cmd_strip  = $(STRIP) $@ ; $(STRIP) -x -R .note -R .comment $@
###################################################
COM_SOURCES :=
-include $(ROOT_DIR)/app/media/SOURCES
-include $(ROOT_DIR)/app/network/SOURCES
-include $(ROOT_DIR)/app/sys/SOURCES
C_SRCS   = $(filter %.c, $(COM_SOURCES))
CPP_SRCS = $(filter %.cpp,$(COM_SOURCES))
C_OBJS   = $(C_SRCS:%.c=./obj/%.o)
CPP_OBJS = $(CPP_SRCS:%.cpp=./obj/%.o)

C_FLAGS =	-Wall -Wno-unused -Wno-write-strings -Wno-missing-field-initializers \
			-D_REENTRANT -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT \
			-DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
LD_FLAGS =	-lpthread -lstdc++ -pthread -ldl -lm -lilclient_if \
			-lbcm_host -lcontainers -lilclient -lkhrn_client -lkhrn_static -lopenmaxil -lvcfiled_check -lvchiq_arm -lvchostif -lvcilcs -lvcos -lvcsm \
			-lmp4v2

INC_DIR		= 	-I$(ROOT_DIR)/app/include \
				-I$(ROOT_DIR)/app/include/include \
				-I$(ROOT_DIR)/lib/mp4v2/include/mp4v2 \
				-I/opt/vc/include \
				-I/opt/vc/include/interface/mmal \
				-I/opt/vc/include/interface/vchi \
				-I/opt/vc/include/interface/vchiq_arm \
				-I/opt/vc/include/interface/vcos \
				-I/opt/vc/include/interface/vctypes \
				-I/opt/vc/include/interface/vmcs_host \
				-I/opt/vc/include/interface/vmcs_host/linux \
				-I/opt/vc/include/interface/vcos/pthreads \
				-I/opt/vc/include/interface/vcos/generic
				
LIBS_DIR	= -L$(ROOT_DIR)/app/lib -L/usr/local/lib -L/opt/vc/lib

COMPILE = $(TOOL_PREFIX)gcc $(C_FLAGS) $(INC_DIR)
LINK 	= $(TOOL_PREFIX)gcc
STRIP 	= $(TOOL_PREFIX)strip

DBGCFLAGS = -g -rdynamic
RELCFLAGS = -O3
RELLDFLAGS = $(LIBS_DIR) $(LD_FLAGS)

#target
DBGTARGET = ./$(TARGET_NAME)$(DBG_NAME)
RELTARGET = ./$(TARGET_NAME)
ifeq ($(CONFIG_DBG), y)
	COMPILE_FLAGS=$(DBGCFLAGS)
	TARGET = $(DBGTARGET)
else
	COMPILE_FLAGS=$(RELCFLAGS)
	TARGET = $(RELTARGET)
endif
##################################################
.PHONY: all clean

#如果要使用force build, 需要再建立一层依赖关系！！
#all:test
#test:forceBuild H2642PS
#forceBuild:
#	-@rm -f ./obj/src/PS.o
#	-@rm H2642PS

all:$(TARGET)
	
$(TARGET): $(C_OBJS) $(CPP_OBJS) $(TAR_C_OBJS)
	$(call cmd,link)
	$(call cmd,strip)

$(C_OBJS):./obj/%.o:%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

$(CPP_OBJS):./obj/%.o:%.cpp
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

$(TAR_C_OBJS):./obj/%.o:%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

clean:
	-@rm -rf ./obj
	-@echo " CLEAN obj"
	-@rm -rf $(TARGET)
	-@echo " CLEAN target"









