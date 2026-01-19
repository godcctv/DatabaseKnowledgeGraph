#ifndef ONTOLOGY_H
#define ONTOLOGY_H

#include <QString>
#include <QDateTime>

class Ontology {
public:
    int id;
    QString name;
    QString description;
    QString version;
    QDateTime createdAt;
    QDateTime updatedAt;

    Ontology() : id(-1), version("1.0") {}
};

#endif