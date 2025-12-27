#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once
#include <QMainWindow>


class QListView; class QLabel; class QLineEdit; class QPushButton;
class QQuickWidget; class QQmlContext;
class FlightModel; class RadarClient;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(FlightModel* model, RadarClient* radar, QWidget* parent=nullptr);

private slots:
    void onRowChanged(const QModelIndex& current);
    void onConnectClicked();
    void onDisconnectClicked();

private:
    FlightModel* m_model;
    RadarClient* m_radar;

    QListView*   m_list;
    QLabel*      m_lblId; QLabel* m_lblType; QLabel* m_lblClass;
    QLabel*      m_lblFrom; QLabel* m_lblTo; QLabel* m_lblLatLon;
    QLabel*      m_lblAlt; QLabel* m_lblSeen;

    QLineEdit*   m_host; QLineEdit* m_port;
    QPushButton* m_btnConn; QPushButton* m_btnDis;

    QQuickWidget* m_map;

    void updateDetails(int row);
    void centerMap(double lat, double lon);


};
#endif // MAINWINDOW_H


