#ifndef TYPES_H
#define TYPES_H

#pragma once
#include <QString>
#include <QHash>
#include <QtEndian>
#include <QDebug>


struct AircraftInfo {
    QString name;
    QString type;
    int capacity = 0;
    int rangeNm = 0;
};

inline const QHash<quint8, AircraftInfo>& aircraftCatalog()
{
    static const QHash<quint8, AircraftInfo> k = {
        {0xa1, {"Boing 747-8", "Commercial Airplane", 660, 8000}},
        {0xa2, {"Airbus A380", "Commercial Airplane", 853, 8000}},
        {0xb1, {"Bell 206", "Helicopter", 6, 374}},
        {0xb2, {"Lockheed Martin F-35 Lightning II", "Military Jet", 1, 1188}},
        {0xb3, {"Airbus A320neo", "Commercial Airplane", 194, 4000}},
        {0xb4, {"Boing737 MAX", "Commercial Airplane", 204, 3850}},
        {0xb5, {"Airbus A220", "Commercial Airplane", 150, 3400}},
        {0xb6, {"Airbus A321XLR", "Commercial Airplane", 220, 4700}},
        {0xb7, {"Boeing 787 Dreamliner", "Commercial Airplane", 330, 7530}},
        {0xc1, {"Airbus A350", "Commercial Airplane", 410, 8700}},
        {0xd1, {"Boeing 777", "Commercial Airplane", 396, 7250}},
        {0xd2, {"Boeing 777X", "Commercial Airplane", 426, 7285}},
        {0xd5, {"Airbus A380", "Commercial Airplane", 615, 8000}},
        {0xe1, {"Robinson R44", "Helicopter", 4, 300}},
        {0xe8, {"Airbus H125", "Helicopter", 6, 320}},
        {0xe2, {"Bell 407", "Helicopter", 7, 324}},
        {0xf5, {"Leonardo AW139", "Helicopter", 17, 540}},
        {0xf0, {"Lockheed Martin F-22 Raptor", "Military Jet", 1, 1600}},
        {0xf1, {"Chengdu J-20 Mighty Dragon", "Military Jet", 1, 1100}},
        {0xf2, {"Sukhoi Su-57 Felon", "Military Jet", 1, 2000}},
    };
    return k;
}

struct AirportInfo {
    QString name;
    QString country;
    double  latitude;
    double  longitude;
};

inline const QHash<quint8, AirportInfo>& airportCatalog()
{
    static const QHash<quint8, AirportInfo> k = {
        { 0x1a, { QStringLiteral("Imam Khomeini"),   QStringLiteral("Iran"),         35.416167, 51.152211 } },
        { 0x1b, { QStringLiteral("Shahid Beheshti"), QStringLiteral("Iran"),         32.750831, 51.881112 } },
        { 0x1c, { QStringLiteral("Muscat"),          QStringLiteral("Oman"),         23.600678, 58.282744 } },
        { 0x1d, { QStringLiteral("Istanbul"),        QStringLiteral("Turkey"),       41.276878, 28.730144 } },
        { 0x1e, { QStringLiteral("Shiraz"),          QStringLiteral("Iran"),         29.539147, 52.589939 } },
        { 0x1f, { QStringLiteral("King Fahd"),       QStringLiteral("Saudi Arabia"), 26.470060, 49.798450 } },
        { 0x3a, { QStringLiteral("Dubai"),           QStringLiteral("UAE"),          25.251578, 55.368344 } },
        { 0xa2, { QStringLiteral("Hamad"),           QStringLiteral("Qatar"),        25.269956, 51.602756 } },
        { 0x10, { QStringLiteral("Kuwait"),          QStringLiteral("Kuwait"),       29.240433, 47.971028 } },
        { 0x2b, { QStringLiteral("Erbil"),           QStringLiteral("Iraq"),         36.233569, 43.955458 } },
        { 0x3c, { QStringLiteral("Luxor"),           QStringLiteral("Egypt"),        25.669628, 32.706644 } },
    };
    return k;
}



struct FlightRecord {
    quint8  typeId = 0;
    quint8  srcAirportId = 0;
    quint8  dstAirportId = 0;
    quint32 latitudeRaw = 0;
    quint16 latitudeFactor = 1;
    quint32 longitudeRaw = 0;
    quint16 longitudeFactor = 1;
    quint32 altitude = 0;
    quint32 flightId = 0;

    double latitude() const  { return latitudeFactor ? double(latitudeRaw)  / double(latitudeFactor)  : 0.0; }
    double longitude() const { return longitudeFactor ? double(longitudeRaw) / double(longitudeFactor) : 0.0; }

    bool isValid() const
    {
        const QString typeHex = QStringLiteral("0x%1")
                                    .arg(typeId, 2, 16, QLatin1Char('0'));

        if (!latitudeFactor || !longitudeFactor) {
            qWarning() << "[radar] invalid" << flightId
                       << "- zero lat/lon factor"
                       << "latFactor" << latitudeFactor
                       << "lonFactor" << longitudeFactor;
            return false;
        }

        const double lat = latitude();
        const double lon = longitude();

        if (lat < -90.0 || lat > 90.0) {
            qWarning() << "[radar] invalid" << flightId
                       << "- latitude out of range"
                       << "lat" << lat;
            return false;
        }

        if (lon < -180.0 || lon > 180.0) {
            qWarning() << "[radar] invalid" << flightId
                       << "- longitude out of range"
                       << "lon" << lon;
            return false;
        }

        if (altitude > 60000u) {
            qWarning() << "[radar] invalid" << flightId
                       << "- altitude out of range"
                       << "alt" << altitude;
            return false;
        }
        if (altitude <= 0u){
            qWarning() << "[radar] invalid" << flightId
                       << "- altitude = 0"
                       << "alt" << altitude;
            return false;
        }

        if (flightId == 0u) {
            qWarning() << "[radar] invalid record with flightId = 0";
            return false;
        }

        if (!aircraftCatalog().contains(typeId)) {
            qWarning().noquote() << "[radar] invalid" << flightId
                                 << "- unknown typeId" << typeId
                                 << "(" << typeHex << ")";
            return false;
        }

        if (!airportCatalog().contains(srcAirportId)) {
            qWarning() << "[radar] invalid" << flightId
                       << "- unknown srcAirportId" << srcAirportId;
            return false;
        }

        if (!airportCatalog().contains(dstAirportId)) {
            qWarning() << "[radar] invalid" << flightId
                       << "- unknown dstAirportId" << dstAirportId;
            return false;
        }

        if (srcAirportId == dstAirportId) {
            qWarning() << "[radar] invalid" << flightId
                       << "- srcAirportId == dstAirportId"
                       << srcAirportId;
            return false;
        }

        return true;
    }

};


#endif // TYPES_H
