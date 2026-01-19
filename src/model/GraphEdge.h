#ifndef GRAPHEDGE_H
#define GRAPHEDGE_H

#include <QString>
#include <QJsonObject>

class GraphEdge {
public:
    int id;                 // 对应 relation_id
    int ontologyId;         // 对应 ontology_id
    int sourceId;           // 对应 source_id
    int targetId;           // 对应 target_id
    QString relationType;   // 对应 relation_type
    float weight;           // 对应 weight
    QJsonObject properties; // 对应 properties (JSON)

    GraphEdge() : id(-1), ontologyId(-1), sourceId(-1), targetId(-1), weight(1.0f) {}

    QJsonObject toJson() const;
    static GraphEdge fromJson(const QJsonObject& json);
};

#endif