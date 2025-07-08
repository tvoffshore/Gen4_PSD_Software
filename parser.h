#ifndef PARSER_H
#define PARSER_H

#include <QByteArray>

class Parser
{
public:
    static bool toJson(const QByteArray &rawData, QByteArray &jsonData);
};

#endif // PARSER_H
