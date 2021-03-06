PROJECT_NAME := ftnode 
PROJECT_SRCS := $(wildcard ftnode.cpp) $(wildcard ./src/*.cpp) $(wildcard ./tinyxml/*.cpp) 
PROJECT_OBJS := ${PROJECT_SRCS:.cpp=.o}
PROJECT_INCLUDE_DIRS := ../SQLAPI/include ./headers ./tinyxml
PROJECT_LIBRARY_DIRS := 
PROJECT_LIBRARIES := ../SQLAPI/lib/libsqlapid.a -lpthread -lboost_system -lboost_thread -DBOOST_THREAD_VERSION=5 -ldl -lpq

CXXFLAGS += $(foreach includedir,$(PROJECT_INCLUDE_DIRS),-I$(includedir))
CXXFLAGS += -Wall -std=c++14
LDFLAGS += $(foreach librarydir,$(PROJECT_LIBRARY_DIRS),-L$(librarydir))
LDLIBS += $(foreach library,$(PROJECT_LIBRARIES), $(library))

all:	${PROJECT_NAME}

$(PROJECT_NAME):	$(PROJECT_OBJS)
	$(LINK.cpp) $(PROJECT_OBJS) -o $(PROJECT_NAME) $(LDLIBS)

.PHONY:	clean
clean:
	@- rm -rf $(PROJECT_OBJS) $(PROJECT_NAME) *.o

