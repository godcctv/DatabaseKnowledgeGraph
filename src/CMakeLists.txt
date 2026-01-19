# 1. 自动递归搜寻 src 目录下所有子文件夹中的 .cpp 和 .h 文件
# 这样当你以后在 ui/ 或 database/ 文件夹增加代码时，不需要手动改 CMake
file(GLOB_RECURSE SOURCES "*.cpp" "ui/*.cpp" "database/*.cpp" "model/*.cpp" "business/*.cpp" "visualization/*.cpp" "utils/*.cpp")
file(GLOB_RECURSE HEADERS "*.h" "ui/*.h" "database/*.h" "model/*.h" "business/*.h" "visualization/*.h" "utils/*.h")

# 2. 定义可执行程序目标，包含入口 main.cpp 和搜寻到的所有源码
add_executable(DatabaseKnowledgeGraph main.cpp ${SOURCES} ${HEADERS})

# 3. 链接 Qt 的核心模块和数据库模块
target_link_libraries(DatabaseKnowledgeGraph PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
)