#ifndef ONTOLOGYREPOSITORY_H
#define ONTOLOGYREPOSITORY_H

#include <QList>
#include <QString>
#include "../model/Ontology.h" // 复用已有的模型类

class OntologyRepository
{
public:
    // 初始化数据库表结构 (只负责 ontology 表)
    static void initDatabase();

    // --- 项目/知识库管理 ---

    // 获取所有项目列表
    static QList<Ontology> getAllOntologies();

    // 添加新项目
    static bool addOntology(const QString& name, const QString& desc);

    // 删除项目 (级联删除下属节点)
    static bool deleteOntology(int id);

    // 获取单个项目详情
    static Ontology getOntologyById(int id);
};

#endif // ONTOLOGYREPOSITORY_H