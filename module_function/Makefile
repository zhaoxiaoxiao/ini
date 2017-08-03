
MODULE := frameProc

CXX = g++
GCC = gcc

RM = rm -rf
CP = cp -rf

CONFIGFILE := 
CPPFLAGS := #-Wall -g
CFLAGS :=

INCLUDE := #-I./include -I/usr/local/log4c/include
LDFLAGS := #-L/usr/lib64/mysql -L/usr/local/log4c/lib -L/usr/local/lib
LIBRARY := -pthread -lrt #-lmysqlclient -llog4c

BUILD_DIR = $(MODULE)_build

TARGET = $(BUILD_DIR)/$(MODULE)

LOCAL_SRC_CFILES := \
		main.c			\
		cmd_shell.c		\
		frame_thread_pool.c	\
		frame_time.c		\
		frame_tool.c		\
		log_mini.c		\
		bind_cpu_core.c		\
		daemon_process.c	\
		lottery.c		\
		socket_api.c		\
		es_http_api.c		\
		key_value_part.c	\
		memory_pool.c		\
		http_applayer.c		\
		http_head_info.c	\
		es_obj_info.c		\
		es_search_result.c	\
		es_test_function.c	\

LOCAL_SRC_CCFILES := \

LOCAL_SRC_CPPFILES := \

LOCAL_C_INCLUDES := \

LOCAL_CC_INCLUDES := \

LOCAL_CPP_INCLUDES := \

CFLAGS		:=	$(addprefix -I,$(LOCAL_C_INCLUDES))
CCFLAGS		:=	$(addprefix -I,$(LOCAL_CC_INCLUDES))
CPPFLAGS	:=	$(addprefix -I,$(LOCAL_CPP_INCLUDES))

OBJSC		:= $(patsubst %.c,$(BUILD_DIR)/%.o,$(LOCAL_SRC_CFILES))
OBJSCXX		:= $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LOCAL_SRC_CPPFILES))
OBJSCC		:= $(patsubst %.cc,$(BUILD_DIR)/%.o,$(LOCAL_SRC_CCFILES))
OBJS :=  $(OBJSC) $(OBJSCC) $(OBJSCXX)

OBJ_DIRS	:= $(sort $(dir $(OBJS)))

all:checkdirs $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(LIBRARY) $(LDFLAGS) -o $@

$(OBJSC): $(BUILD_DIR)/%.o: %.c
	$(CXX) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJSCXX): $(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(INCLUDE) -c $< -o $@

$(OBJSCC): $(BUILD_DIR)/%.o: %.cc
	$(CXX) $(CCFLAGS) $(INCLUDE) -c $< -o $@

checkdirs: $(OBJ_DIRS)

$(OBJ_DIRS):
	mkdir -p $@

install:
	$(CP) $(CONFIGFILE) $(BUILD_DIR)/	

.PHONY : clean
clean :
	-$(RM) $(BUILD_DIR)

