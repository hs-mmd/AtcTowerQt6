#ifndef FLIGHTMODEL_H
#define FLIGHTMODEL_H

#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <QDateTime>
#include <QVariantMap>
#include <QGeoCoordinate>
#include "Types.h"

class FlightModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LatitudeRole = Qt::UserRole + 1,
        LongitudeRole,
        HeadingRole,
        DstAirportIdRole,
        ArrivedRole,
    };

    explicit FlightModel(QObject* parent=nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariantMap get(int row) const;


public slots:
    void upsertFlight(const FlightRecord& rec);

private:
    struct Item {
        FlightRecord rec;
        QDateTime lastSeen;
        double headingDeg = 0.0;
        bool arrived = false;
    };

    QHash<quint32, int> m_indexById;
    QVector<Item> m_items;
};

#endif
