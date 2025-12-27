import QtQuick 2.15
import QtLocation
import QtPositioning

Item {
    id: root
    width: 800
    height: 600

    property int  currentIndex: -1
    property var  selectedCoord: null

    property double centerLat: 35.5
    property double centerLon: 51.5

    property color neon: "#39ff14"

    property var trailPath: []
    property int maxTrailPoints: 90
    property real minTrailStepMeters: 250

    function resetTrail(c) {
        trailPath = c ? [c] : []
    }

    function appendTrail(c) {
        if (!c) return

        if (trailPath.length > 0) {
            var last = trailPath[trailPath.length - 1]

            if (last && last.distanceTo && last.distanceTo(c) < minTrailStepMeters)
                return
        }

        var t = trailPath.slice(0)
        t.push(c)

        if (t.length > maxTrailPoints)
            t.splice(0, t.length - maxTrailPoints)

        trailPath = t
    }


    function airportById(aid) {
        if (!airportList) return null
        for (var i = 0; i < airportList.length; i++) {
            var a = airportList[i]
            if (a && a.id === aid) return a
        }
        return null
    }


    Plugin {
        id: osm
        name: "osm"

        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }

        PluginParameter { name: "osm.mapping.custom.host"; value: "https://a.basemaps.cartocdn.com/dark_all" }

        PluginParameter {
            name: "osm.mapping.custom.mapcopyright"
            value: "© CARTO • © OpenStreetMap contributors"
        }
    }


    Map {
        id: map
        anchors.fill: parent
        plugin: osm
        zoomLevel: 7
        center: QtPositioning.coordinate(root.centerLat, root.centerLon)

        Component.onCompleted: {
            for (var j = 0; j < map.supportedMapTypes.length; ++j) {
                var t = map.supportedMapTypes[j]
                var n = (t.name || "").toLowerCase()
                var d = (t.description || "").toLowerCase()
                if (n.indexOf("custom") >= 0 || d.indexOf("custom") >= 0) {
                    map.activeMapType = t
                    console.log("[map] activeMapType =", t.name)
                    break
                }
            }
        }

        //Zoom handlers
        property var startCentroid

        PinchHandler {
            id: pinch
            target: null
            grabPermissions: PointerHandler.TakeOverForbidden
            onActiveChanged: if (active) {
                map.startCentroid = map.toCoordinate(pinch.centroid.position, false)
            }
            onScaleChanged: function(delta) {
                map.zoomLevel += Math.log2(delta)
                map.zoomLevel = Math.max(map.minimumZoomLevel,
                                            Math.min(map.maximumZoomLevel, map.zoomLevel))
                map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
            }
        }

        WheelHandler {
            id: wheel
            target: map
            property: "zoomLevel"
            rotationScale: 1/120
            onWheel: {
                map.zoomLevel = Math.max(map.minimumZoomLevel,
                                         Math.min(map.maximumZoomLevel, map.zoomLevel))
            }
        }

        DragHandler {
            id: drag
            target: null
            grabPermissions: PointerHandler.TakeOverForbidden
            property point lastDrag: Qt.point(0, 0)

            onActiveChanged: if (active) lastDrag = Qt.point(0, 0)

            onTranslationChanged: {
                var dx = translation.x - lastDrag.x
                var dy = translation.y - lastDrag.y
                map.pan(-dx, -dy)
                lastDrag = Qt.point(translation.x, translation.y)
            }
        }

        //Trail line for selected flight
        MapItemView {
            id: trailSegments
            z: 2
            visible: root.trailPath.length > 1
            model: Math.max(0, root.trailPath.length - 1)

            delegate: MapPolyline {

                property var p1: root.trailPath[index]
                property var p2: root.trailPath[index + 1]


                property bool ok:
                    p1 !== undefined && p1 !== null &&
                    p2 !== undefined && p2 !== null &&
                    p1.latitude !== undefined && p1.longitude !== undefined &&
                    p2.latitude !== undefined && p2.longitude !== undefined

                visible: ok
                path: ok ? [p1, p2] : []


                line.width: 3


                property real t: (trailSegments.model <= 1) ? 1.0 : (index / (trailSegments.model - 1))
                line.color: Qt.rgba(0.4, 1.0, 0.4, 0.08 + 0.55 * t)
            }

        }

        //Trail head glow
        MapQuickItem {
            id: trailHead
            z: 4
            visible: root.trailPath.length > 0
            coordinate: root.trailPath.length > 0 ? root.trailPath[root.trailPath.length - 1]
                                                  : QtPositioning.coordinate(root.centerLat, root.centerLon)

            anchorPoint.x: 10
            anchorPoint.y: 10

            sourceItem: Item {
                width: 20
                height: 20

                Rectangle {
                    anchors.centerIn: parent
                    width: 8
                    height: 8
                    radius: 4
                    color: root.neon
                    opacity: 0.9
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 8
                    height: 8
                    radius: 4
                    color: "transparent"
                    border.width: 2
                    border.color: root.neon
                    opacity: 0.0
                    scale: 1.0

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.0; to: 0.55; duration: 350 }
                        NumberAnimation { from: 0.55; to: 0.0; duration: 650 }
                    }
                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 2.8; duration: 1000; easing.type: Easing.OutQuad }
                    }
                }
            }
        }

        //Airports
        MapItemView {
            model: airportList
            delegate: MapQuickItem {
                z: 3
                property var airport: modelData
                coordinate: QtPositioning.coordinate(airport.latitude, airport.longitude)
                anchorPoint.x: 8
                anchorPoint.y: 8

                sourceItem: Column {
                    spacing: 2

                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: "transparent"
                        border.width: 2
                        border.color: root.neon
                        opacity: 0.85
                    }

                    Item {
                        implicitWidth: label.implicitWidth + 8
                        implicitHeight: label.implicitHeight + 6

                        Rectangle {
                            anchors.fill: parent
                            radius: 3
                            color: "#000000AA"
                            border.width: 1
                            border.color: root.neon
                        }

                        Text {
                            id: label
                            anchors.centerIn: parent
                            text: airport.name + " · " + airport.country
                            color: root.neon
                            font.pixelSize: 10
                            font.family: "monospace"
                        }
                    }
                }
            }
        }

        //Arrival markers
        MapItemView {
            model: flightModel

            delegate: MapQuickItem {
                z: 4
                visible: arrived

                property var a: root.airportById(dstAirportId)


                coordinate: a ? QtPositioning.coordinate(a.latitude, a.longitude)
                              : QtPositioning.coordinate(latitude, longitude)

                anchorPoint.x: 14
                anchorPoint.y: 14

                sourceItem: Item {
                    width: 28
                    height: 28


                    Rectangle {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 7
                        color: "#FFC107"
                        border.width: 2
                        border.color: "white"
                        opacity: 0.95
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 7
                        color: "transparent"
                        border.width: 2
                        border.color: "#FFC107"
                        opacity: 0.0
                        scale: 1.0

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.0; to: 0.55; duration: 250 }
                            NumberAnimation { from: 0.55; to: 0.0; duration: 850 }
                        }
                        SequentialAnimation on scale {
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 3.5; duration: 1100; easing.type: Easing.OutQuad }
                        }
                    }

                    Item {
                        anchors.top: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: 6
                        implicitWidth: txt.implicitWidth + 10
                        implicitHeight: txt.implicitHeight + 6

                        Rectangle {
                            anchors.fill: parent
                            radius: 3
                            color: "#000000CC"
                            border.width: 1
                            border.color: "#FFC107"
                        }
                        Text {
                            id: txt
                            anchors.centerIn: parent
                            text: a ? ("ARRIVED • " + a.name) : "ARRIVED"
                            color: "#FFC107"
                            font.pixelSize: 11
                            font.family: "monospace"
                        }
                    }
                }
            }
        }

        //Flights
        MapItemView {
            id: flightsView
            model: flightModel

            delegate: MapQuickItem {
                id: flightItem
                z: 6

                property real lat: latitude
                property real lon: longitude
                property real hdg: heading

                coordinate: QtPositioning.coordinate(lat, lon)
                rotation: hdg

                anchorPoint.x: planeImg.width / 2
                anchorPoint.y: planeImg.height / 2

                function updateSelectedCoordIfNeeded(resetTrailAlso) {
                    if (index === root.currentIndex) {
                        var c = QtPositioning.coordinate(lat, lon)
                        root.selectedCoord = c

                        if (resetTrailAlso === true) {
                            root.resetTrail(c)
                        } else {
                            root.appendTrail(c)
                        }
                    }
                }

                Timer {
                    id: trailCoalesce
                    interval: 0
                    repeat: false
                    onTriggered: flightItem.updateSelectedCoordIfNeeded(false)
                }

                onLatChanged: {
                    if (index === root.currentIndex)
                        trailCoalesce.restart()
                }

                onLonChanged: {
                    if (index === root.currentIndex)
                        trailCoalesce.restart()
                }


                Connections {
                    target: root
                    function onCurrentIndexChanged() {
                        flightItem.updateSelectedCoordIfNeeded(true)
                    }
                }

                Component.onCompleted: {
                    updateSelectedCoordIfNeeded(true)
                }


                property bool selected: (index === root.currentIndex)
                property real targetScale: selected ? 1.20 : 1.0

                sourceItem: Item {
                    width: 32; height: 32

                    scale: flightItem.targetScale
                    transformOrigin: Item.Center
                    Behavior on scale {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Image {
                        id: planeImg
                        anchors.centerIn: parent
                        source: "qrc:/plane.png"
                        width: 28; height: 28
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.95
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 36; height: 36
                        radius: 18
                        border.width: 2
                        border.color: (selected) ? root.neon : "transparent"
                        color: "transparent"
                        opacity: 0.9
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.currentIndex = index
                            root.selectedCoord = QtPositioning.coordinate(flightItem.lat, flightItem.lon)
                        }
                    }
                }
            }
        }

        //Radar overlay
        Canvas {
            id: radarCanvas
            anchors.fill: parent
            z: 10
            visible: root.currentIndex >= 0 && root.selectedCoord
            antialiasing: true

            property real radarAngle: 0.0
            property real ringCount: 5

            Timer {
                interval: 16
                running: radarCanvas.visible
                repeat: true
                onTriggered: {
                    radarCanvas.radarAngle = (radarCanvas.radarAngle + 1.2) % 360
                    radarCanvas.requestPaint()
                }
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.clearRect(0, 0, width, height)

                var cx = width / 2
                var cy = height / 2

                if (root.selectedCoord) {
                    var p = map.fromCoordinate(root.selectedCoord, false)
                    cx = p.x
                    cy = p.y
                }

                var maxR = Math.min(width, height) * 0.45

                //rings
                ctx.save()
                ctx.globalAlpha = 0.22
                ctx.strokeStyle = root.neon
                ctx.lineWidth = 1
                for (var i = 1; i <= ringCount; i++) {
                    var r = maxR * (i / ringCount)
                    ctx.beginPath()
                    ctx.arc(cx, cy, r, 0, Math.PI * 2)
                    ctx.stroke()
                }
                ctx.restore()

                //crosshair
                ctx.save()
                ctx.globalAlpha = 0.18
                ctx.strokeStyle = root.neon
                ctx.lineWidth = 1
                ctx.beginPath(); ctx.moveTo(cx - maxR, cy); ctx.lineTo(cx + maxR, cy); ctx.stroke()
                ctx.beginPath(); ctx.moveTo(cx, cy - maxR); ctx.lineTo(cx, cy + maxR); ctx.stroke()
                ctx.restore()

                //wedge
                var ang = radarAngle * Math.PI / 180.0
                var wedge = 18 * Math.PI / 180.0

                ctx.save()
                var grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, maxR)
                grad.addColorStop(0.00, "rgba(57,255,20,0.40)")
                grad.addColorStop(0.35, "rgba(57,255,20,0.18)")
                grad.addColorStop(1.00, "rgba(57,255,20,0.00)")
                ctx.fillStyle = grad

                ctx.beginPath()
                ctx.moveTo(cx, cy)
                ctx.arc(cx, cy, maxR, ang - wedge, ang + wedge, false)
                ctx.closePath()
                ctx.fill()
                ctx.restore()
            }
        }
    }

    //Zoom buttons
    Column {
        id: zoomControls
        spacing: 6
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        z: 50

        Rectangle {
            width: 34; height: 34; radius: 6
            color: "#00000088"
            border.width: 1
            border.color: root.neon
            Text { anchors.centerIn: parent; text: "+"; color: root.neon; font.pixelSize: 18; font.family: "monospace" }
            MouseArea { anchors.fill: parent; onClicked: map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1) }
        }

        Rectangle {
            width: 34; height: 34; radius: 6
            color: "#00000088"
            border.width: 1
            border.color: root.neon
            Text { anchors.centerIn: parent; text: "-"; color: root.neon; font.pixelSize: 18; font.family: "monospace" }
            MouseArea { anchors.fill: parent; onClicked: map.zoomLevel = Math.max(map.minimumZoomLevel, map.zoomLevel - 1) }
        }
    }
}
