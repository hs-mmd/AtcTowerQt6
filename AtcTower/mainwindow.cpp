#include "MainWindow.h"
#include <QListView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QMessageBox>
#include "flightmodel.h"
#include "radarclient.h"

MainWindow::MainWindow(FlightModel* model, RadarClient* radar, QWidget* parent)
    : QMainWindow(parent), m_model(model), m_radar(radar)
{
    auto *central = new QWidget;
    setCentralWidget(central);

    m_list = new QListView;
    m_list->setModel(m_model);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setMinimumWidth(200);


    connect(m_list->selectionModel(), &QItemSelectionModel::currentChanged,this, &MainWindow::onRowChanged);

    connect(m_model, &QAbstractItemModel::dataChanged,
            this,
            [this](
                const QModelIndex& topLeft,
                const QModelIndex& bottomRight,
                const QVector<int>& /*roles*/)
            {
                const int row = m_list->currentIndex().row();

                if (row < 0)
                    return;

                if (topLeft.row() <= row && row <= bottomRight.row())
                    updateDetails(row);

            });





    m_map = new QQuickWidget;
    m_map->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QVariantList airportList;
    const auto &airports = airportCatalog();

    for (auto it = airports.cbegin(); it != airports.cend(); ++it) {
        QVariantMap a;
        a.insert("id",        static_cast<int>(it.key()));
        a.insert("name",      it.value().name);
        a.insert("country",   it.value().country);
        a.insert("latitude",  it.value().latitude);
        a.insert("longitude", it.value().longitude);
        airportList.push_back(a);
    }

    m_map->rootContext()->setContextProperty("flightModel", m_model);
    m_map->rootContext()->setContextProperty("airportList", airportList);

    m_map->setSource(QUrl(QStringLiteral("qrc:/map.qml")));
    m_map->setMinimumWidth(700);




    auto mkRow = [](const QString& k, QLabel*& v){
        auto *w = new QWidget;
        auto *h = new QHBoxLayout(w);
        auto *lblK = new QLabel(QString("<b>%1</b>").arg(k));
        v = new QLabel("-");
        h->addWidget(lblK);
        h->addWidget(v, 1);
        return w;
    };
    QWidget *right = new QWidget;
    right->setMinimumWidth(250);
    auto *rv = new QVBoxLayout(right);
    rv->addWidget(mkRow("Flight ID:", m_lblId));
    rv->addWidget(mkRow("Aircraft:",  m_lblType));
    rv->addWidget(mkRow("Class:",     m_lblClass));
    rv->addWidget(mkRow("From:",      m_lblFrom));
    rv->addWidget(mkRow("To:",        m_lblTo));
    rv->addWidget(mkRow("Lat/Lon:",   m_lblLatLon));
    rv->addWidget(mkRow("Altitude:",  m_lblAlt));
    rv->addWidget(mkRow("Last seen:", m_lblSeen));
    rv->addStretch();


    auto *split = new QSplitter;

    split->addWidget(m_list);
    split->addWidget(m_map);
    split->addWidget(right);
    split->setSizes({250, 800, 300});

    split->setCollapsible(0, false);
    split->setCollapsible(1, false);
    split->setCollapsible(2, false);

    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setStretchFactor(2, 0);



    m_host = new QLineEdit("127.0.0.1");
    m_port = new QLineEdit("9000");
    m_btnConn = new QPushButton("Connect");
    m_btnDis  = new QPushButton("Disconnect");
    auto *lblConn = new QLabel("Disconnected");
    lblConn->setMinimumWidth(140);

    m_btnConn->setStyleSheet(
        "QPushButton { background:#1db954; color:white; font-weight:600; padding:6px 14px; border-radius:6px; }"
        "QPushButton:disabled { background:#2e7d32; color:#c8e6c9; }"
        );

    m_btnDis->setStyleSheet(
        "QPushButton { background:#e53935; color:white; font-weight:600; padding:6px 14px; border-radius:6px; }"
        "QPushButton:disabled { background:#8e2a28; color:#ffcdd2; }"
        );


    connect(m_btnConn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_btnDis,  &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_radar, &RadarClient::connectedChanged, this, [=](bool ok){
        m_btnConn->setEnabled(!ok);
        m_btnDis->setEnabled(ok);
    });

    connect(m_radar, &RadarClient::stateChanged, this, [=](int st){
        QString s;
        switch (static_cast<QAbstractSocket::SocketState>(st)) {
        case QAbstractSocket::UnconnectedState: s = "Unconnected"; break;
        case QAbstractSocket::HostLookupState:  s = "HostLookup";  break;
        case QAbstractSocket::ConnectingState:  s = "Connecting";  break;
        case QAbstractSocket::ConnectedState:   s = "Connected";   break;
        case QAbstractSocket::ClosingState:     s = "Closing";     break;
        default:                                s = "Unknown";     break;
        }
        lblConn->setText(s);
    });


    auto *bottom = new QWidget;
    auto *hb = new QHBoxLayout(bottom);
    hb->addWidget(m_host);
    hb->addWidget(m_port);
    hb->addWidget(m_btnConn);
    hb->addWidget(m_btnDis);
    hb->addWidget(lblConn);
    hb->addStretch();


    auto *v = new QVBoxLayout(central);
    v->addWidget(split, 1);
    v->addWidget(bottom, 0);

    connect(m_radar, &RadarClient::errorText, this, [this](const QString& msg){
        QMessageBox::warning(this, "Radar", msg);
    });

}

void MainWindow::onRowChanged(const QModelIndex& current)
{
    const int row = current.row();
    updateDetails(row);

    QVariantMap rec = m_model->get(row);
    if (!rec.isEmpty()) {
        centerMap(rec.value("latitude").toDouble(),
                  rec.value("longitude").toDouble());
    }
}

void MainWindow::updateDetails(int row)
{
    if (m_map && m_map->rootObject()) {
        m_map->rootObject()->setProperty("currentIndex", row);
    }

    QVariantMap rec = m_model->get(row);
    if (rec.isEmpty()) {
        m_lblId->setText("-"); m_lblType->setText("-"); m_lblClass->setText("-");
        m_lblFrom->setText("-"); m_lblTo->setText("-"); m_lblLatLon->setText("-");
        m_lblAlt->setText("-"); m_lblSeen->setText("-");
        return;
    }
    m_lblId->setText(QString::number(rec.value("flightId").toUInt()));
    m_lblType->setText(rec.value("typeName").toString());
    m_lblClass->setText(rec.value("typeClass").toString());
    m_lblFrom->setText(rec.value("srcAirport").toString() + "  -  " + rec.value("srcCountry").toString()) ;
    m_lblTo->setText(rec.value("dstAirport").toString() +  "  -  " + rec.value("dstCountry").toString());
    const double lat = rec.value("latitude").toDouble();
    const double lon = rec.value("longitude").toDouble();
    m_lblLatLon->setText(QString::number(lat, 'f', 4) + " , " + QString::number(lon, 'f', 4));
    m_lblAlt->setText(QString::number(rec.value("altitude").toUInt()));
    m_lblSeen->setText(rec.value("lastSeen").toString());
}

void MainWindow::centerMap(double lat, double lon)
{
    if (!m_map || !m_map->rootObject())
        return;

    QQuickItem *root = m_map->rootObject();
    root->setProperty("centerLat", lat);
    root->setProperty("centerLon", lon);
}


void MainWindow::onConnectClicked()
{
    m_radar->connectToHost(m_host->text(), m_port->text().toUShort());
}
void MainWindow::onDisconnectClicked()
{
    m_radar->disconnectFromHost();
}
