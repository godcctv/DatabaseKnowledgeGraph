# 1. 手动列出所有源文件
set(SOURCES
        main.cpp

        # 业务逻辑层
        business/GraphEditor.cpp
        business/QueryEngine.cpp
        business/ForceDirectedLayout.cpp

        # 数据库层
        database/DatabaseConnection.cpp
        database/NodeRepository.cpp
        database/RelationshipRepository.cpp
        database/OntologyRepository.cpp
        database/AttributeRepository.cpp

        # UI 界面层
        ui/mainwindow.cpp
        ui/addnodedialog.cpp
        ui/addedgedialog.cpp

        ui/VisualNode.cpp
        ui/VisualEdge.cpp
)

set(HEADERS
        # 业务逻辑头文件
        business/GraphEditor.h
        business/QueryEngine.h
        business/EditorCommand.h
        business/ForceDirectedLayout.h

        # 模型头文件
        model/GraphNode.h
        model/GraphEdge.h
        model/Ontology.h
        model/Attribute.h

        # 数据库头文件
        database/DatabaseConnection.h
        database/NodeRepository.h
        database/RelationshipRepository.h
        database/OntologyRepository.h
        database/AttributeRepository.h

        # UI 头文件
        ui/mainwindow.h
        ui/addnodedialog.h
        ui/addedgedialog.h
        ui/VisualNode.h
        ui/VisualEdge.h
)

set(FORMS
        ui/mainwindow.ui
        ui/addnodedialog.ui
        ui/addedgedialog.ui
        ui/VisualNode.h
        ui/VisualNode.cpp
        ui/VisualEdge.h
        ui/VisualEdge.cpp
        business/ForceDirectedLayout.h

)

# 2. 定义可执行程序 (直接使用上面定义的列表)
add_executable(DatabaseKnowledgeGraph ${SOURCES} ${HEADERS} ${FORMS})

# 3. 链接 Qt 库
target_link_libraries(DatabaseKnowledgeGraph PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
)