#ifndef ATTRIBUTEREPOSITORY_H
#define ATTRIBUTEREPOSITORY_H

#include <QList>
#include <QString>
#include "../model/Attribute.h"

class AttributeRepository {
public:
    // --- 增 ---
    // 向数据库添加一条属性
    static bool addAttribute(Attribute& attr);

    // --- 删 ---
    // 根据主键ID删除属性
    static bool deleteAttribute(int attrId);
    // 删除某个实体的所有属性（如删除节点时联动）
    static bool deleteAttributesByEntity(const QString& entityType, int entityId);

    // --- 改 ---
    static bool updateAttribute(const Attribute& attr);

    // --- 查 ---
    // 获取指定实体（节点或关系）的所有扩展属性
    static QList<Attribute> getAttributesForEntity(const QString& entityType, int entityId);
};

#endif