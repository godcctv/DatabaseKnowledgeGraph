#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <QString>

class Attribute {
public:
    int id;                 // 对应 attr_id
    QString entityType;     // 对应 entity_type (NODE 或 RELATION)
    int entityId;           // 对应 entity_id
    QString attrName;       // 对应 attr_name
    QString attrValue;      // 对应 attr_value
    QString attrType;       // 对应 attr_type

    Attribute() : id(-1), entityId(-1) {}
};

#endif