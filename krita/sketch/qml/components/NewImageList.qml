/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

import QtQuick 1.1
import org.krita.sketch 1.0

ListView {
    id: base;
    clip: true;

    signal clicked(variant options);

    delegate: Item {
        height: header.height + listItem.height;
        width: base.width;
        Rectangle {
            id: header;
            width: base.width;
            height: index > 0 && (base.model.groupNameOf(index - 1) != model.groupName) ? Constants.GridHeight : 0;
            visible: height > 0;gradient: Gradient {
                GradientStop {
                    position: 0
                    color: Settings.theme.color("pages/welcome/create/header/start");
                }


                GradientStop {
                    position: 1
                    color: Settings.theme.color("pages/welcome/create/header/stop");
                }
            }
            Label {
                anchors {
                    left: parent.left;
                    leftMargin: Constants.DefaultMargin;
                    verticalCenter: parent.verticalCenter;
                }
                text: model.groupName;
                font: Settings.theme.font("title");
                color: Settings.theme.color("pages/welcome/create/header/text");
            }
            Image {
                anchors {
                    right: parent.right;
                    rightMargin: Constants.DefaultMargin * 3;
                    verticalCenter: parent.verticalCenter;
                }
                height: parent.height / 2;
                width: height;
                smooth: true;
                source: Settings.theme.icon("expansionmarker");
                rotation: model.groupFolded ? 90 : 0;
                Behavior on rotation { PropertyAnimation { duration: Constants.AnimationDuration; } }
            }
            MouseArea {
                anchors.fill: parent;
                onClicked: base.model.toggleGroup(model.groupName);
            }

            Shadow { width: parent.width; height: Constants.GridHeight / 8; anchors.top: parent.bottom; }
        }
        ListItem {
            id: listItem;
            anchors.top: header.bottom;
            anchors.topMargin: header.visible ? Constants.DefaultMargin : 0;
            width: base.width;
            opacity: model.groupFolded ? 0 : 1;
            Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
            height: model.groupFolded ? 0 : Constants.GridHeight * 1.75
            Behavior on height { PropertyAnimation { duration: Constants.AnimationDuration; } }

            title: model.name;
            image.source: Krita.fileExists(model.icon) ? model.icon : Settings.theme.icon(model.icon);
            image.asynchronous: false;
            imageShadow: false;

            gradient: Gradient {
                GradientStop { position: 0; color: Settings.theme.color("components/newImageList/start") }
                GradientStop { position: 0.4; color: Settings.theme.color("components/newImageList/stop"); }
            }

            onClicked: {
                switch(model.file) {
                    case "a5p": {
                        base.clicked({
                            width: 600,
                            height: 875,
                            resolution: 150,
                        });
                    }
                    case "a5l": {
                        base.clicked({
                            width: 875,
                            height: 600,
                            resolution: 150,
                        });
                    }
                    case "a4p": {
                        base.clicked({
                            width: 1200,
                            height: 1750,
                            resolution: 150,
                        });
                    }
                    case "a4l": {
                        base.clicked({
                            width: 1750,
                            height: 1200,
                            resolution: 150,
                        });
                    }
                    case "screen": {
                        base.clicked({
                            width: Krita.Window.width,
                            height: Krita.Window.height,
                            resolution: 72.0
                        });
                    }
                    case "custom": {
                        base.clicked(null);
                    }
                    case "clip": {
                        base.clicked({ source: "clipboard" });
                    }
                    case "webcam": {
                        base.clicked({ source: "webcam" });
                    }
                    default: {
                        console.debug(model.file);
                        if(model.file.indexOf("template://") === 0) {
                            base.clicked({ template: model.file });
                        }
                    }
                }
            }
        }
    }

    model: TemplatesModel {}
//     model: ListModel {
//         ListElement { bnrole: "custom"; name: "Custom Image"; image: "filenew-black" }
//         ListElement { bnrole: "clip";   name: "From Clipboard"; image: "fileclip-black" }
// 
//         ListElement { bnrole: "a4p";    name: "Blank Image (A4 Portrait)"; image: "A4portrait-black" }
//         ListElement { bnrole: "a4l";    name: "Blank Image (A4 Landscape)"; image: "A4landscape-black" }
//         ListElement { bnrole: "screen"; name: "Blank Image (Screen Size)"; image: "filenew-black" }
// //                 ListElement { bnrole: "a5p";    name: "Blank Image (A5 Portrait)"; image: "../images/svg/icon-A4portrait-black.svg" }
// //                 ListElement { bnrole: "a5l";    name: "Blank Image (A5 Landscape)"; image: "../images/svg/icon-A4landscape-black.svg" }
// //                 ListElement { bnrole: "webcam"; name: "From Camera"; image: "../images/svg/icon-camera-black.svg" }
//     }

    ScrollDecorator { }
}
