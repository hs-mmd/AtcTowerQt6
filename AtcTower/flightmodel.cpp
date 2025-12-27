#include "flightmodel.h"


FlightModel::FlightModel(QObject* parent) : QAbstractListModel(parent) {}

int FlightModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant FlightModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_items.size()) return {};

    const Item& it = m_items[idx.row()];

    switch (role) {
    case Qt::DisplayRole: return QStringLiteral("%1   %2")
            .arg(it.rec.flightId)
            .arg(aircraftCatalog().value(it.rec.typeId).name);
    case LatitudeRole:
        return it.rec.latitude();
    case LongitudeRole:
        return it.rec.longitude();
    case HeadingRole:
        return it.headingDeg;
    case DstAirportIdRole:
        return it.rec.dstAirportId;
    case ArrivedRole:
        return it.arrived;

    default:
        return {};
    }
}

QHash<int, QByteArray> FlightModel::roleNames() const {
    return {
            {LatitudeRole, "latitude"},
            {LongitudeRole, "longitude"},
            {HeadingRole, "heading"},
            {DstAirportIdRole, "dstAirportId" },
            {ArrivedRole, "arrived" },
        };
}

void FlightModel::upsertFlight(const FlightRecord& rec)
{
    const int* p = m_indexById.contains(rec.flightId) ? &m_indexById[rec.flightId] : nullptr;

    if (p) {
        int   row = *p;
        Item &it  = m_items[row];

        const double oldLat = it.rec.latitude();
        const double oldLon = it.rec.longitude();
        const double newLat = rec.latitude();
        const double newLon = rec.longitude();


        const QDateTime now = QDateTime::currentDateTimeUtc();
        const double dt = it.lastSeen.isValid() ? it.lastSeen.msecsTo(now) / 1000.0 : 0.0;

        if (dt > 0.0) {
            QGeoCoordinate c1(oldLat, oldLon);
            QGeoCoordinate c2(newLat, newLon);
            const double dist = c1.distanceTo(c2);

            constexpr double kTeleportTimeSec = 2.0;
            constexpr double kTeleportDistM   = 80000.0;

            if (dt <= kTeleportTimeSec && dist >= kTeleportDistM) {
                qWarning().noquote()
                << "[sanity] dropped teleport id" << rec.flightId
                << "dt" << dt << "dist" << dist
                << "latF" << rec.latitudeFactor << "lonF" << rec.longitudeFactor;

                it.lastSeen = QDateTime::currentDateTimeUtc();
                return;
            }
        }




        if (!qFuzzyCompare(oldLat, newLat) || !qFuzzyCompare(oldLon, newLon)) {
            QGeoCoordinate c1(oldLat, oldLon);
            QGeoCoordinate c2(newLat, newLon);

            double az = c1.azimuthTo(c2);

            if (!qIsNaN(az)) {
                if (az < 0.0)
                    az += 360.0;
                it.headingDeg = az;
            }
        }

        it.rec      = rec;
        it.lastSeen = QDateTime::currentDateTimeUtc();

        if (!it.arrived) {
            const auto a = airportCatalog().value(rec.dstAirportId, AirportInfo{});

            if (airportCatalog().contains(rec.dstAirportId)) {
                QGeoCoordinate here(rec.latitude(), rec.longitude());
                QGeoCoordinate dst(a.latitude, a.longitude);
                constexpr double kArrivalMeters = 5000.0;
                if (here.distanceTo(dst) <= kArrivalMeters) {
                    it.arrived = true;
                }
            }
        }


        const QVector<int> roles = {
            LatitudeRole,
            LongitudeRole,
            HeadingRole,
            DstAirportIdRole,
            ArrivedRole,
        };
        emit dataChanged(index(row,0), index(row,0), roles);

    }
    else {
        Item item;
        item.rec        = rec;

        item.arrived = false;
        if (airportCatalog().contains(rec.dstAirportId)) {
            const auto a = airportCatalog().value(rec.dstAirportId);
            QGeoCoordinate here(rec.latitude(), rec.longitude());
            QGeoCoordinate dst(a.latitude, a.longitude);
            constexpr double kArrivalMeters = 5000.0;
            if (here.distanceTo(dst) <= kArrivalMeters) {
                item.arrived = true;
            }
        }

        item.lastSeen   = QDateTime::currentDateTimeUtc();
        item.headingDeg = 0.0;

        beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
        m_items.push_back(item);
        m_indexById.insert(rec.flightId, m_items.size()-1);
        endInsertRows();
    }
}


QVariantMap FlightModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_items.size())
        return m;

    const Item& it = m_items[row];
    const auto& ac = aircraftCatalog().value(it.rec.typeId);

    const auto& srcInfo = airportCatalog().value(it.rec.srcAirportId);
    const auto& dstInfo = airportCatalog().value(it.rec.dstAirportId);


    m.insert("flightId",   QVariant::fromValue<quint32>(it.rec.flightId));
    m.insert("typeId",     it.rec.typeId);
    m.insert("typeName",   ac.name);
    m.insert("typeClass",  ac.type);
    m.insert("capacity",   ac.capacity);
    m.insert("rangeNm",    ac.rangeNm);
    m.insert("srcAirport", srcInfo.name);
    m.insert("dstAirport", dstInfo.name);
    m.insert("srcCountry", srcInfo.country);
    m.insert("dstCountry", dstInfo.country);
    m.insert("latitude",   it.rec.latitude());
    m.insert("longitude",  it.rec.longitude());
    m.insert("altitude",   QVariant::fromValue<quint32>(it.rec.altitude));
    m.insert("lastSeen",   it.lastSeen.toString(Qt::ISODate));
    return m;
}
