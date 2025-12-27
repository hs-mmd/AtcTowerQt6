#include <QApplication>
#include "flightmodel.h"
#include "radarclient.h"
#include "mainwindow.h"
#include <QtCore/QResource>




int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    FlightModel  model;
    RadarClient  radar;

    QObject::connect(&radar, &RadarClient::flightReceived,
                     &model, &FlightModel::upsertFlight);

    MainWindow w(&model, &radar);
    w.resize(1350, 720);
    w.show();

    return app.exec();
}
