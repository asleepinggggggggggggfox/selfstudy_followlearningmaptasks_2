# 编译器和标志
CXX := g++
# 使用C++11标准，启用常见警告，链接 pthread 库
CXXFLAGS := -std=c++11 -Wall -Wextra -pthread
# 可执行文件名称
TARGET := test_threadpool

# 源文件：只需要您的测试文件
SRCS := test_threadpool.cpp
# 生成目标文件列表 (.o) 和依赖文件列表 (.d)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(CXXFLAGS)

# 编译源文件，并生成依赖信息。关键：-pthread 标志在编译阶段也需要
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# 包含生成的依赖文件，确保头文件更新时能重新编译
-include $(DEPS)

# 清理编译生成的文件
clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

# 声明伪目标
.PHONY: all clean

# 调试模式 (添加调试符号，关闭优化)
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)

# 发布模式 (优化级别提高)
release: CXXFLAGS += -O2 -DNDEBUG
release: $(TARGET)