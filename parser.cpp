#include "parser.h"

#include <QByteArray>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
using PsdPoint = float;

/**
 * @brief Data type identifiers
 */
enum class DataType
{
    Psd,
    Statistic,
    Raw,

    Count
};

#pragma pack(push, 1)
/**
 * @brief Single measurements packet header structure
 */
struct PacketHeader
{
    uint32_t startEpochTime;
    uint32_t durationMs;
    uint16_t sampleTimeMs;
    uint8_t dataType;
    uint8_t sensorType;

    QJsonObject toJson()
    {
        QJsonObject json;

        json["start time"] = QDateTime::fromSecsSinceEpoch(startEpochTime).toString("yyyy-MM-dd hh:mm:ss");
        json["duration ms"] = static_cast<int>(durationMs);
        json["sample time ms"] = static_cast<int>(sampleTimeMs);

        return json;
    }
};

/**
 * @brief PSD measurements header structure
 */
struct PsdHeader
{
    float coreFrequency;
    float coreAmplitude;
    float deltaFrequency;
    uint32_t points;

    QJsonObject toJson()
    {
        QJsonObject json;

        json["core freq"] = coreFrequency;
        json["core ampl"] = coreAmplitude;
        json["delta freq"] = deltaFrequency;
        json["points"] = static_cast<int>(points);

        return json;
    }
};

/**
 * @brief Statistic measurements data structure
 */
struct StatisticData
{
    float max;
    float min;
    float mean;
    float deviation;

    QJsonObject toJson()
    {
        QJsonObject json;

        json["max"] = max;
        json["min"] = min;
        json["mean"] = mean;
        json["deviation"] = deviation;

        return json;
    }
};
#pragma pack(pop)

bool psdToJson(const QByteArray &rawData, QJsonObject &json)
{
    if (rawData.size() < static_cast<qsizetype>(sizeof(PsdHeader)))
    {
        qCritical() << "Psd data size" << rawData.size() << "is too small";
        return false;
    }

    PsdHeader psdHeader;
    memcpy(&psdHeader, rawData.constData(), sizeof(PsdHeader));
    const QByteArray psdPointsData = rawData.sliced(sizeof(PsdHeader));
    if (psdPointsData.size() != static_cast<qsizetype>(psdHeader.points * sizeof(PsdPoint)))
    {
        qCritical() << "Psd points size" << psdPointsData.size() << "!= points count" << psdHeader.points;
        return false;
    }

    float frequency = 0;
    QJsonArray psdPointsJson;
    const PsdPoint *psdPoints = reinterpret_cast<const PsdPoint*>(psdPointsData.constData());
    for (size_t idx = 0; idx < psdHeader.points; idx++)
    {
        QJsonObject pointJson;

        pointJson["ampl"] = psdPoints[idx];
        pointJson["freq"] = frequency;

        psdPointsJson.append(pointJson);
        frequency += psdHeader.deltaFrequency;
    }

    json["psd header"] = psdHeader.toJson();
    json["psd points"] = psdPointsJson;
    return true;
}

bool statisticToJson(const QByteArray &rawData, QJsonObject &json)
{
    if (rawData.size() < static_cast<qsizetype>(sizeof(StatisticData)))
    {
        qCritical() << "Statistic data size" << rawData.size() << "is too small";
        return false;
    }

    StatisticData statisticData;
    memcpy(&statisticData, rawData.constData(), sizeof(StatisticData));

    json["statistic"] = statisticData.toJson();
    return true;
}
}

bool Parser::toJson(const QByteArray &rawData, QByteArray &jsonData)
{
    if (rawData.size() < static_cast<qsizetype>(sizeof(PacketHeader)))
    {
        qCritical() << "Data packet size" << rawData.size() << "is too small";
        return false;
    }

    PacketHeader packetHeader;
    memcpy(&packetHeader, rawData.constData(), sizeof(packetHeader));

    bool result = false;
    const QByteArray packetPayload = rawData.sliced(sizeof(PacketHeader));

    QJsonObject json;
    json["packet header"] = packetHeader.toJson();

    switch (packetHeader.dataType)
    {
    case static_cast<uint8_t>(DataType::Psd):
        result = psdToJson(packetPayload, json);
        break;

    case static_cast<uint8_t>(DataType::Statistic):
        result = statisticToJson(packetPayload, json);
        break;

    default:
        break;
    }

    if (result == true)
    {
        QJsonDocument jsonDoc(json);
        jsonData = jsonDoc.toJson(QJsonDocument::Indented);
    }

    return result;
}
