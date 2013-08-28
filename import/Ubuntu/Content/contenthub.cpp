/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "contenthub.h"
#include <contentpeer.h>
#include <contenttransfer.h>
#include <contenttype.h>
#include <qmlimportexporthandler.h>

#include <com/ubuntu/content/hub.h>
#include <com/ubuntu/content/peer.h>
#include <com/ubuntu/content/type.h>

#include <QDebug>

/*!
 * \qmltype ContentHub
 * \instantiates ContentHub
 * \inqmlmodule Ubuntu.Content 0.1
 * \brief FIXME documentation
 *
 * FIXME documentation
 *
 * Example usage for importing content:
 * \qml
 * import QtQuick 2.0
 * import Ubuntu.Components 0.1
 * import Ubuntu.Content 0.1
 *
 * Rectangle {
 *     Button {
 *         text: "Import from default"
 *          onClicked: {
 *              var peer = ContentHub.defaultSourceForType(ContentType.Pictures);
 *              var transfer = ContentHub.importContent(ContentType.Pictures, peer);
 *              transfer.start()
 *         }
 *     }
 *     Button {
 *         text: "Import from a selectable list"
 *          onClicked: {
 *              var transfer = ContentHub.importContent(ContentType.Pictures);
 *         }
 *     }
 *     property list<ContentItem> importItems
 *     Connections {
 *         target: ContentHub
 *         onFinishedImportsChanged: {
 *             var idx = ContentHub.finishedImports.length - 1;
 *             importItmes = ContentHub.finishedImports[idx].items;
 *         }
 *     }
 * }
 * \endqml
 *
 * Example usage for providing a content export:
 * \qml
 * import QtQuick 2.0
 * import Ubuntu.Content 0.1
 *
 * Rectangle {
 *     property list<ContentItem> selectedItems
 *     Connections {
 *         target: ContentHub
 *         onExportRequested: {
 *             // show content picker
 *             transfer.items = selectedItems;
 *             transfer.state = ContentTransfer.Charged;
 *         }
 *     }
 * }
 * \endqml
 */

namespace cuc = com::ubuntu::content;

ContentHub::ContentHub(QObject *parent)
    : QObject(parent),
      m_hub(0)
{
    m_hub = cuc::Hub::Client::instance();
    m_handler = new QmlImportExportHandler(this);
    m_hub->register_import_export_handler(m_handler);

    connect(m_handler, SIGNAL(importRequested(com::ubuntu::content::Transfer*)),
            this, SLOT(handleImport(com::ubuntu::content::Transfer*)));
    connect(m_handler, SIGNAL(exportRequested(com::ubuntu::content::Transfer*)),
            this, SLOT(handleExport(com::ubuntu::content::Transfer*)));
}

/*!
 * \qmlmethod ContentHub::defaultSourceForType()
 *
 *  Returns the default peer for the given content \a type
 */
ContentPeer *ContentHub::defaultSourceForType(int type)
{
    qDebug() << Q_FUNC_INFO;

    const cuc::Type &hubType = ContentType::contentType2HubType(type);
    cuc::Peer hubPeer = m_hub->default_peer_for_type(hubType);

    ContentPeer *qmlPeer = new ContentPeer(this);
    qmlPeer->setPeer(hubPeer);

    return qmlPeer;
}

/*!
 * \qmlmethod ContentHub::knownSourcesForType()
 *
 *  Returns all possible peers for the given content \a type
 */
QList<ContentPeer *> ContentHub::knownSourcesForType(int type)
{
    qDebug() << Q_FUNC_INFO;

    const cuc::Type &hubType = ContentType::contentType2HubType(type);
    QVector<cuc::Peer> hubPeers = m_hub->known_peers_for_type(hubType);

    QList<ContentPeer *> qmlPeers;
    foreach (const cuc::Peer &hubPeer, hubPeers) {
        ContentPeer *qmlPeer = new ContentPeer(this);
        qmlPeer->setPeer(hubPeer);
        qmlPeers.append(qmlPeer);
    }

    return qmlPeers;
}

/*!
 * \qmlmethod ContentHub::importContent()
 *
 * Start a request to import data of \a type from a peer, the user needs to select
 */
ContentTransfer *ContentHub::importContent(int type)
{
    qDebug() << Q_FUNC_INFO << static_cast<ContentType::Type>(type);

    const cuc::Type &hubType = ContentType::contentType2HubType(type);
//    FIXME show user a selection of possible peers
    cuc::Peer hubPeer = m_hub->default_peer_for_type(hubType);
    cuc::Transfer *hubTransfer = m_hub->create_import_for_type_from_peer(hubType, hubPeer);
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(hubTransfer);

    connect(hubTransfer,
            SIGNAL(stateChanged()),
            this,
            SLOT(startImport()));

    return qmlTransfer;
}

void ContentHub::startImport()
{
    qDebug() << Q_FUNC_INFO;
    auto t = dynamic_cast<cuc::Transfer*>(sender());
    if (t->state() == cuc::Transfer::charged)
    {
        qDebug() << Q_FUNC_INFO << "Transfer in progress";
        this->handleImport(t);
    }
}

/*!
 * \qmlmethod ContentHub::importContent()
 *
 * Start a request to import data of \a type from the given \peer
 */
ContentTransfer *ContentHub::importContent(int type, ContentPeer *peer)
{
    qDebug() << Q_FUNC_INFO << static_cast<ContentType::Type>(type) << peer;

    const cuc::Type &hubType = ContentType::contentType2HubType(type);
    cuc::Transfer *hubTransfer = m_hub->create_import_for_type_from_peer(hubType, peer->peer());
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(hubTransfer);

    connect(hubTransfer,
            SIGNAL(stateChanged()),
            this,
            SLOT(startImport()));

    return qmlTransfer;
}

/*!
 * \qmlmethod ContentHub::restoreImports()
 *
 *  FIXME add documentation
 */
void ContentHub::restoreImports()
{
    qDebug() << Q_FUNC_INFO;
}

/*!
 * \qmlproperty list<ContentTransfer> ContentHub::finishedImports
 *
 * FIXME add documentation
 */
QQmlListProperty<ContentTransfer> ContentHub::finishedImports()
{
    qDebug() << Q_FUNC_INFO;
    return QQmlListProperty<ContentTransfer>(this, m_finishedImports);
}

/*!
 * \brief ContentHub::handleImport handles an incoming request for importing content
 * \param transfer
 */
void ContentHub::handleImport(com::ubuntu::content::Transfer *transfer)
{
    qDebug() << Q_FUNC_INFO;
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(transfer);
    m_finishedImports.append(qmlTransfer);
    Q_EMIT finishedImportsChanged();
}

/*!
 * \brief ContentHub::handleExport handles an incoming request for exporting content
 * \param transfer
 */
void ContentHub::handleExport(com::ubuntu::content::Transfer *transfer)
{
    qDebug() << Q_FUNC_INFO;
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(transfer);

    Q_EMIT exportRequested(qmlTransfer);
}
