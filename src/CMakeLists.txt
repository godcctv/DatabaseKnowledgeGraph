# 1. 自动搜索当前目录及子目录下所有源文件和头文件
file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE HEADERS "*.h")

# 2. 移除 main.cpp，因为我们要单独处理它，防止自动搜寻导致重复
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# 3. 定义可执行程序，手动加上 main.cpp 确保它是入口
add_executable(DatabaseKnowledgeGraph main.cpp ${SOURCES} ${HEADERS}
        database/NodeRepository.h
        database/NodeRepository.cpp
        database/OntologyRepository.h
        database/OntologyRepository.cpp
        database/RelationshipRepository.h
        database/RelationshipRepository.cpp
        database/AttributeRepository.h
        database/AttributeRepository.cpp
        business/EditorCommand.h
        business/GraphEditor.h
        business/GraphEditor.cpp
        business/QueryEngine.h
        business/QueryEngine.cpp)

# 4. 统一链接 Qt5 模块
target_link_libraries(DatabaseKnowledgeGraph PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
)