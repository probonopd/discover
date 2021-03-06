import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtQuick 2.4
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kcoreaddons 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.0 as Kirigami

DiscoverPage
{
    id: page
    title: i18n("Updates")

    function start() {
        resourcesUpdatesModel.updateAll()
    }

    property string search: ""
    property string footerLabel: ""

    //TODO: use supportsRefreshing to fetch updates
    mainItem: ListView
    {
        id: updatesView
        currentIndex: -1
        ResourcesUpdatesModel {
            id: resourcesUpdatesModel
            onIsProgressingChanged: {
                window.navigationEnabled = !isProgressing

                if (!isProgressing) {
                    resourcesUpdatesModel.prepare()
                }
            }

            Component.onCompleted: {
                if (!isProgressing) {
                    resourcesUpdatesModel.prepare()
                }
            }
        }

        UpdateModel {
            id: updateModel
            backend: resourcesUpdatesModel
        }

        headerPositioning: ListView.OverlayHeader
        header: PageHeader {
            backgroundImage.source: "qrc:/banners/updatescrop.jpg"

            extra: RowLayout {
                id: updateControls

                Layout.fillWidth: true

                visible: (updateModel.totalUpdatesCount > 0 && resourcesUpdatesModel.isProgressing) || updateModel.hasUpdates

                LabelBackground {
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    text: updateModel.toUpdateCount + " (" + updateModel.updateSize+")"
                }
                Label {
                    text: i18n("updates selected")
                }
                LabelBackground {
                    id: unselectedItem
                    readonly property int unselected: (updateModel.totalUpdatesCount - updateModel.toUpdateCount)
                    text: unselected
                    visible: unselected>0
                }
                Label {
                    text: i18n("updates not selected")
                    visible: unselectedItem.visible
                }
                Item { Layout.fillWidth: true}
                Button {
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 6
                    Layout.rightMargin: Kirigami.Units.gridUnit
                    text: unselectedItem.visible ? i18n("Update Selected") : i18n("Update All")
                    enabled: !resourcesUpdatesModel.isProgressing
                    onClicked: page.start()
                }
            }
        }

        footer: ColumnLayout {
            anchors.right: parent.right
            anchors.left: parent.left
            Kirigami.Heading {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                visible: page.footerLabel !== ""
                text: page.footerLabel
            }
            QIconItem {
                Layout.alignment: Qt.AlignHCenter
                visible: page.footerLabel !== ""
                icon: "update-none"
                opacity: 0.3
                width: 200
                height: 200
            }
            Item {
                visible: page.footerLabel === ""
                height: Kirigami.Units.gridUnit
                width: 1
            }
        }

        model: updateModel

        section {
            property: "section"
            delegate: Kirigami.Heading {
                x: Kirigami.Units.gridUnit
                level: 2
                text: section
            }
        }

        spacing: Kirigami.Units.smallSpacing

        delegate: Kirigami.AbstractListItem {
            x: Kirigami.Units.gridUnit
            width: ListView.view.width - Kirigami.Units.gridUnit * 2
            highlighted: ListView.isCurrentItem || (page.search.length>0 && display.indexOf(page.search)>=0)
            onEnabledChanged: if (!enabled) {
                layout.extended = false;
            }

            Keys.onReturnPressed: {
                itemChecked.clicked()
            }
            Keys.onPressed: if (event.key===Qt.Key_Alt) layout.extended = true
            Keys.onReleased: if (event.key===Qt.Key_Alt)  layout.extended = false

            ColumnLayout {
                id: layout
                property bool extended: false
                onExtendedChanged: if (extended) {
                    updateModel.fetchChangelog(index)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CheckBox {
                        id: itemChecked
                        anchors.verticalCenter: parent.verticalCenter
                        checked: model.checked == Qt.Checked
                        onClicked: model.checked = (model.checked==Qt.Checked ? Qt.Unchecked : Qt.Checked)
                    }

                    QIconItem {
                        Layout.fillHeight: true
                        Layout.preferredWidth: height
                        icon: decoration
                    }

                    Label {
                        Layout.fillWidth: true
                        text: i18n("%1 (%2)", display, version)
                        elide: Text.ElideRight
                    }

                    LabelBackground {
                        Layout.minimumWidth: Kirigami.Units.gridUnit * 6
                        text: size

                        progress: resourceProgress/100
                    }
                }

                ScrollView {
                    id: view
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    frameVisible: true
                    visible: layout.extended && changelog.length>0

                    Label {
                        width: view.viewport.width
                        text: changelog
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                    }
                }

                Button {
                    text: i18n("Open")
                    visible: layout.extended
                    enabled: !resourcesUpdatesModel.isProgressing
                    onClicked: Navigation.openApplication(resource)
                }
            }

            onClicked: {
                layout.extended = !layout.extended
            }
        }
    }

    readonly property alias secSinceUpdate: resourcesUpdatesModel.secsToLastUpdate
    state:  ( updateModel.hasUpdates                     ? "has-updates"
            : resourcesUpdatesModel.isProgressing        ? "progressing"
            : secSinceUpdate < 0                         ? "unknown"
            : secSinceUpdate === 0                       ? "now-uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24       ? "uptodate"
            : secSinceUpdate < 1000 * 60 * 60 * 24 * 7   ? "medium"
            :                                              "low"
            )

    states: [
        State {
            name: "progressing"
            PropertyChanges { target: page; title: i18nc("@info", "Updating...") }
            PropertyChanges { target: page; footerLabel: resourcesUpdatesModel.progress<=0 ? i18nc("@info", "Fetching updates") : "" }
        },
        State {
            name: "has-updates"
            PropertyChanges { target: page; title: i18nc("@info", "Updates") }
        },
        State {
            name: "now-uptodate"
            PropertyChanges { target: page; title: i18nc("@info", "The system is up to date") }
            PropertyChanges { target: page; footerLabel: i18nc("@info", "No updates") }
        },
        State {
            name: "uptodate"
            PropertyChanges { target: page; title: i18nc("@info", "The system is up to date") }
            PropertyChanges { target: page; footerLabel: i18nc("@info", "No updates") }
        },
        State {
            name: "medium"
            PropertyChanges { target: page; title: i18nc("@info", "No updates are available") }
        },
        State {
            name: "low"
            PropertyChanges { target: page; title: i18nc("@info", "Should check for updates") }
        },
        State {
            name: "unknown"
            PropertyChanges { target: page; title: i18nc("@info", "It is unknown when the last check for updates was") }
        }
    ]
}
