CC = g++
TARGET = transpotor


# 包含的源文件目录列表
# SRC_DIR = 

SRCS := $(wildcard ./*.cpp ./src/*.cpp)
SRCS += $(wildcard $(foreach n, $(SRC_DIR), $(n)/*.cpp))
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

CXXFLAGS = --std=c++11 -I./ -I./include -L./lib -lpthread -Wl,--rpath=./lib

# 依赖文件搜索路径
# VPATH += 

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CXXFLAGS)

$(SRC):%.o:%.cpp
	$(CC) -c $< $(CXXFLAGS)

.PHONY : clean
clean:
	rm $(TARGET) $(OBJS)