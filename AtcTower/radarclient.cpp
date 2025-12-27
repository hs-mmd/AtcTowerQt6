#include "radarclient.h"
#include <QtEndian>


static const QByteArray kHeader("\xA5\xA5\xA5\xA5", 4);
static const QByteArray kFooter("\x55\x55\x55\x55", 4);
static const int kFrameSize = 39;
static constexpr int kMaxBufferSize = 1024 * 1024; 


RadarClient::RadarClient(QObject* parent) : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::readyRead, this, &RadarClient::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected, this, [this]{
        emit connectedChanged(true);
        emit stateChanged(m_socket.state());
        qDebug() << "[radar] connected to" << m_socket.peerAddress().toString() << m_socket.peerPort();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]{
        emit connectedChanged(false);
        emit stateChanged(m_socket.state());
        qDebug() << "[radar] disconnected";
    });
    connect(&m_socket, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState){
        emit stateChanged(m_socket.state());
        qDebug() << "[radar] state:" << m_socket.state();
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError){
        emit errorText(m_socket.errorString());
        qWarning() << "[radar] error:" << m_socket.errorString();
    });
}

void RadarClient::connectToHost(const QString& host, quint16 port) {
    auto s = m_socket.state();
    if (s == QAbstractSocket::ConnectedState &&
        m_socket.peerAddress().toString() == host && m_socket.peerPort() == port) {
        emit errorText("Already connected");
        return;
    }
    if (s != QAbstractSocket::UnconnectedState) m_socket.abort();
    qDebug() << "[radar] connecting to" << host << port;
    m_socket.connectToHost(host, port);
}

void RadarClient::disconnectFromHost() {
    m_socket.disconnectFromHost();
}

void RadarClient::onReadyRead()
{

    m_buffer.append(m_socket.readAll());

    if (m_buffer.size() > kMaxBufferSize) {
        qWarning() << "[radar] buffer overflow, disconnecting";
        m_socket.disconnectFromHost();
        m_buffer.clear();
        return;
    }

    while (true) {
        FlightRecord rec;
        if (!tryExtractFrame(m_buffer, rec))
            break;
        if (!rec.isValid()) {
            qWarning() << "[radar] dropped invalid record" << rec.flightId
                       << "type" << rec.typeId
                       << "src" << rec.srcAirportId
                       << "dst" << rec.dstAirportId
                       << "lat" << rec.latitude()
                       << "lon" << rec.longitude()
                       << "alt" << rec.altitude;

            continue;
        }
        emit flightReceived(rec);
    }
}

bool RadarClient::tryExtractFrame(QByteArray& buffer, FlightRecord& out)
{
    int pos = buffer.indexOf(kHeader);
    if (pos < 0) { buffer.clear(); return false; }
    if (buffer.size() < pos + kFrameSize) return false;

    QByteArray frame = buffer.mid(pos, kFrameSize);

    if (memcmp(frame.constData() + 35, kFooter.constData(), 4) != 0) {

        buffer.remove(0, pos + 1);
        return false;
    }


    const uchar* d = reinterpret_cast<const uchar*>(frame.constData());

    out.typeId        = d[4];
    out.srcAirportId  = d[5];
    out.dstAirportId  = d[6];

    out.latitudeRaw   = qFromLittleEndian<quint32>(d + 7);
    out.latitudeFactor= qFromLittleEndian<quint16>(d + 11);
    out.longitudeRaw  = qFromLittleEndian<quint32>(d + 13);
    out.longitudeFactor=qFromLittleEndian<quint16>(d + 17);
    out.altitude      = qFromLittleEndian<quint32>(d + 19);
    out.flightId      = qFromLittleEndian<quint32>(d + 23);


    buffer.remove(0, pos + kFrameSize);
    return true;

}




