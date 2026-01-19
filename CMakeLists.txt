cmake_minimum_required(VERSION 3.16)
project(DatabaseKnowledgeGraph)

# 1. 设置 C++ 17 标准，这是毕设开发的主流标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 开启 Qt 自动处理工具，这是使用 Qt 必备的
set(CMAKE_AUTOMOC ON)  # 自动生成信号槽代码
set(CMAKE_AUTORCC ON)  # 自动处理资源文件
set(CMAKE_AUTOUIC ON)  # 自动处理 .ui 界面文件

# 3. 寻找依赖库：必须包含 Sql 模块来操作 MySQL
find_package(Qt5 COMPONENTS Core Gui Widgets Sql REQUIRED)

# 4. 全局包含路径，方便你以后写 #include "model/GraphNode.h" 而不是相对路径
include_directories(src)

# 5. 指定构建子目录
add_subdirectory(src)