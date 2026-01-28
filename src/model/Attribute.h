// src/model/Attribute.h
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <QString>

class Attribute {
public:
    int id;
    int nodeId;
    int relationId;
    QString attrName;
    QString attrValue;
    QString attrType;

    Attribute() : id(-1), nodeId(-1), relationId(-1) {}


    bool isNodeAttribute() const { return nodeId > 0; }
    bool isRelationAttribute() const { return relationId > 0; }
};

#endif