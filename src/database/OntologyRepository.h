#ifndef ONTOLOGYREPOSITORY_H
#define ONTOLOGYREPOSITORY_H

#include <QList>
#include "../model/Ontology.h"

class OntologyRepository {
public:
    // 增：创建新本体
    static bool addOntology(Ontology& ontology);

    // 删：根据ID删除（会级联删除下属所有节点和关系）
    static bool deleteOntology(int id);

    // 改：更新本体基本信息
    static bool updateOntology(const Ontology& ontology);

    // 查：获取所有本体列表（用于启动时的项目选择）
    static QList<Ontology> getAllOntologies();
    
    // 查：根据ID获取单个本体详情
    static Ontology getOntologyById(int id);
};

#endif