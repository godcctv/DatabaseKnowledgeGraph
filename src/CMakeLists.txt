# 1. 自动搜索所有文件
file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE FORMS "*.ui")

# 2. 定义可执行程序 (确保 main.cpp 在第一个)
add_executable(DatabaseKnowledgeGraph main.cpp ${SOURCES} ${HEADERS} ${FORMS})

# 3. 必须链接 Qt 库，否则找不到 QMainWindow
target_link_libraries(DatabaseKnowledgeGraph PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Sql
)
