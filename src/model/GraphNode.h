#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <QString>
#include <QJsonObject>

class GraphNode {
public:
    int id;                 // 对应 node_id
    int ontologyId;         // 对应 ontology_id
    QString nodeType;       // 对应 node_type
    QString name;           // 对应 name
    QString description;    // 对应 description
    float posX;             // 对应 pos_x
    float posY;             // 对应 pos_y
    QString color;          // 对应 color
    QJsonObject properties; // 对应 properties (JSON)

    GraphNode() : id(-1), ontologyId(-1), posX(0.0f), posY(0.0f), color("#3498db") {}

    QJsonObject toJson() const;
    static GraphNode fromJson(const QJsonObject& json);
};

#endif