# AtcTower (Qt6)

A Qt6 application (Widgets + QML Map) that connects to a radar simulator over TCP, parses fixed-size binary frames, keeps flights in a list model, and visualizes flights and airports on a QtLocation map.

## Features
- TCP client (QTcpSocket) + binary frame extraction (header/footer, fixed frame size)
- Flight list based on `QAbstractListModel` (live updates)
- QML Map (QtLocation/QtPositioning): aircraft markers, airport pins
- Selection, follow selected aircraft, trail (track history)
- Arrival marker for arrived flights
- Widgets UI: flight list, map view (QQuickWidget), flight details panel, connect/disconnect controls

## Tech Stack
- Qt 6.x
- CMake
- Modules: Core, Widgets, Network, Quick, QuickWidgets, Qml, Positioning, Location

## Build (Windows / MinGW)
1. Open the project in Qt Creator (Kit: Desktop Qt 6.x MinGW 64-bit)
2. Select **Release** build configuration
3. Build and run

### Build via CMake (optional)
```bash
cmake -S . -B build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
