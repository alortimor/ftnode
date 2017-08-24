PROJECT_NAME := db_service 
PROJECT_SRCS := $(wildcard ./src/*.cpp) $(wildcard ./tinyxml/*.cpp) 
PROJECT_OBJS := ${PROJECT_SRCS:.cpp=.o}
PROJECT_INCLUDE_DIRS := ../SQLAPI/include ./src/header ./tinyxml
PROJECT_LIBRARY_DIRS := 
PROJECT_LIBRARIES := ../SQLAPI/lib/libsqlapid.a -lpthread -lboost_system -lboost_thread -DBOOST_THREAD_VERSION -ldl -lpq

CXXFLAGS += $(foreach includedir,$(PROJECT_INCLUDE_DIRS),-I$(includedir))
CXXFLAGS += -Wall -g
LDFLAGS += $(foreach librarydir,$(PROJECT_LIBRARY_DIRS),-L$(librarydir))
LDLIBS += $(foreach library,$(PROJECT_LIBRARIES), $(library))

.PHONY: all clean

all:	${PROJECT_NAME}

$(PROJECT_NAME):	$(PROJECT_OBJS)
	$(LINK.cpp) $(PROJECT_OBJS) -o $(PROJECT_NAME) $(LDLIBS)

clean:
	@- rm -rf $(PROJECT_OBJS) $(PROJECT_NAME) *.o

