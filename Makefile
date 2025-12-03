# 编译器选择
CXX := g++

# 编译选项
CXXFLAGS := -std=c++11 -Wall -Wextra -O2 -pthread

# 目标可执行文件名称
TARGET := test_threadpool

# 源文件 (不包含头文件)
SOURCES := TheadPool.cpp test_threadpool.cpp

# 生成对应的目标文件列表 (.o 文件)
OBJECTS := $(SOURCES:.cpp=.o)

# 头文件依赖 (根据 .h 文件自动生成，避免修改头文件后不重新编译的问题)
HEADERS := ThreadPool.h

# 默认目标：编译生成可执行文件
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS)

# 编译每个 .cpp 文件为 .o 文件
# 此行保证了每个 .o 文件依赖于对应的 .cpp 文件和 HEADERS 中列出的头文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	rm -f $(TARGET) $(OBJECTS)

# 重新构建：先清理再编译
rebuild: clean all

# 声明伪目标，防止与同名文件冲突
.PHONY: all clean rebuild