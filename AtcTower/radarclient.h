#ifndef RADARCLIENT_H
#define RADARCLIENT_H

#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include "Types.h"

class RadarClient : public QObject {
    Q_OBJECT
public:
    explicit RadarClient(QObject* parent=nullptr);

    int socketState() const { return m_socket.state(); }
    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    bool connected() const { return m_socket.state() == QAbstractSocket::ConnectedState; }

signals:
    void stateChanged(int);
    void flightReceived(const FlightRecord& rec);
    void errorText(const QString& msg);
    void connectedChanged(bool);

private slots:
    void onReadyRead();

private:
    bool tryExtractFrame(QByteArray& buffer, FlightRecord& out);

    QTcpSocket m_socket;
    QByteArray m_buffer;
};

#endif // RADARCLIENT_H
