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

#include "../../../src/com/ubuntu/content/debug.h"
#include "../../../src/com/ubuntu/content/utils.cpp"
#include "contenthub.h"
#include "contentpeer.h"
#include "contentstore.h"
#include "contenttransfer.h"
#include "contenttype.h"
#include "qmlimportexporthandler.h"

#include <com/ubuntu/content/hub.h>
#include <com/ubuntu/content/peer.h>
#include <com/ubuntu/content/type.h>

#include <QStringList>

/*!
 * \qmltype ContentHub
 * \instantiates ContentHub
 * \inqmlmodule Ubuntu.Content
 * \brief The central manager for all content operations
 *
 * Example usage for importing content:
 * \qml
 * import QtQuick 2.0
 * import Ubuntu.Components 0.1
 * import Ubuntu.Content 0.1
 *
 * MainView {
 *     id: root
 *     width: units.gu(60)
 *     height: units.gu(90)
 *     property list<ContentItem> importItems
 *     property var activeTransfer
 * 
 *     ContentPeer {
 *         id: picSourceSingle
 *         contentType: ContentType.Pictures
 *         handler: ContentHandler.Source
 *         selectionType: ContentTransfer.Single
 *     }
 *
 *     ContentPeer {
 *         id: picSourceMulti
 *         contentType: ContentType.Pictures
 *         handler: ContentHandler.Source
 *         selectionType: ContentTransfer.Multiple
 *     }
 *
 *     Row {
 *         Button {
 *             text: "Import single item"
 *             onClicked: {
 *                 activeTransfer = picSourceSingle.request()
 *             }
 *         }
 *
 *         Button {
 *             text: "Import multiple items"
 *             onClicked: {
 *                 activeTransfer = picSourceMulti.request()
 *             }
 *         }
 *     }
 *
 *     ContentTransferHint {
 *         id: importHint
 *         anchors.fill: parent
 *         activeTransfer: root.activeTransfer
 *     }
 *
 *     Connections {
 *         target: root.activeTransfer
 *         onStateChanged: {
 *             if (root.activeTransfer.state === ContentTransfer.Charged)
 *                 importItems = root.activeTransfer.items;
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
    TRACE() << Q_FUNC_INFO;
    m_hub = cuc::Hub::Client::instance();
    m_handler = new QmlImportExportHandler(this);
    m_hub->register_import_export_handler(m_handler);
    QString id = app_id();
    if (!id.isEmpty())
        m_hasPending = m_hub->has_pending(id);

    connect(m_handler, SIGNAL(importRequested(com::ubuntu::content::Transfer*)),
            this, SLOT(handleImport(com::ubuntu::content::Transfer*)));
    connect(m_handler, SIGNAL(exportRequested(com::ubuntu::content::Transfer*)),
            this, SLOT(handleExport(com::ubuntu::content::Transfer*)));
    connect(m_handler, SIGNAL(shareRequested(com::ubuntu::content::Transfer*)),
            this, SLOT(handleShare(com::ubuntu::content::Transfer*)));
}

ContentHub *ContentHub::instance()
{
    TRACE() << Q_FUNC_INFO;
    static ContentHub *contentHub = new ContentHub(nullptr);
    return contentHub;
}

/*!
 * \brief ContentHub::importContent creates a ContentTransfer object
 * \a peer
 * \a type
 * \internal
 */
ContentTransfer* ContentHub::importContent(cuc::Peer peer, ContentType::Type contentType)
{
    TRACE() << Q_FUNC_INFO;

    const cuc::Type &hubType = ContentType::contentType2HubType(contentType);
    cuc::Transfer *hubTransfer = m_hub->create_import_from_peer_for_type(peer, hubType);
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(hubTransfer);
    m_activeImports.insert(hubTransfer, qmlTransfer);
    return qmlTransfer;
}

/*!
 * \brief ContentHub::exportContent creates a ContentTransfer object
 * \a peer
 * \a type
 * \internal
 */
ContentTransfer* ContentHub::exportContent(cuc::Peer peer, ContentType::Type contentType)
{
    TRACE() << Q_FUNC_INFO;

    const cuc::Type &hubType = ContentType::contentType2HubType(contentType);
    cuc::Transfer *hubTransfer = m_hub->create_export_to_peer_for_type(peer, hubType);
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(hubTransfer);
    m_activeImports.insert(hubTransfer, qmlTransfer);
    return qmlTransfer;
}

/*!
 * \brief ContentHub::shareContent creates a ContentTransfer object
 * \a peer
 * \a type
 * \internal
 */
ContentTransfer* ContentHub::shareContent(cuc::Peer peer, ContentType::Type contentType)
{
    TRACE() << Q_FUNC_INFO;

    const cuc::Type &hubType = ContentType::contentType2HubType(contentType);
    cuc::Transfer *hubTransfer = m_hub->create_share_to_peer_for_type(peer, hubType);
    ContentTransfer *qmlTransfer = new ContentTransfer(this);
    qmlTransfer->setTransfer(hubTransfer);
    m_activeImports.insert(hubTransfer, qmlTransfer);
    return qmlTransfer;
}

/*!
 * \qmlmethod ContentHub::restoreImports()
 * \internal
 */
void ContentHub::restoreImports()
{
    TRACE() << Q_FUNC_INFO;
}

/*!
 * \qmlproperty list<ContentTransfer> ContentHub::finishedImports
 * \internal
 */
QQmlListProperty<ContentTransfer> ContentHub::finishedImports()
{
    TRACE() << Q_FUNC_INFO;
    return QQmlListProperty<ContentTransfer>(this, m_finishedImports);
}

/*!
 * \brief ContentHub::handleImport handles an incoming request for importing content
 * \internal
 */
void ContentHub::handleImport(com::ubuntu::content::Transfer *transfer)
{
    TRACE() << Q_FUNC_INFO;
    ContentTransfer *qmlTransfer = nullptr;
    if (m_activeImports.contains(transfer)) {
        qmlTransfer = m_activeImports.take(transfer);
        qmlTransfer->collectItems();
    } else {
        // If we don't have a reference to the transfer, it was created
        // by another handler so this would be an Import
        qmlTransfer = new ContentTransfer(this);
        qmlTransfer->setTransfer(transfer);
        connect(qmlTransfer, SIGNAL(stateChanged()),
                this, SLOT(updateState()));
        qmlTransfer->collectItems();
        Q_EMIT importRequested(qmlTransfer);
    }



    // FIXME: maybe we need to emit something else here
//    if (qmlTransfer->state() == ContentTransfer::Charged)
//        Q_EMIT importRequested(qmlTransfer);

    m_finishedImports.append(qmlTransfer);
    Q_EMIT finishedImportsChanged();
}

/*!
 * \brief ContentHub::handleExport handles an incoming request for exporting content
 * \internal
 */
void ContentHub::handleExport(com::ubuntu::content::Transfer *transfer)
{
    TRACE() << Q_FUNC_INFO;
    ContentTransfer *qmlTransfer = nullptr;
    if (m_activeImports.contains(transfer))
        qmlTransfer = m_activeImports.take(transfer);
    else {
        // If we don't have a reference to the transfer, it was created
        // by another handler so this would be an Import
        qmlTransfer = new ContentTransfer(this);
        qmlTransfer->setTransfer(transfer);
        m_activeImports.insert(transfer, qmlTransfer);
        connect(qmlTransfer, SIGNAL(stateChanged()),
                this, SLOT(updateState()));
        Q_EMIT exportRequested(qmlTransfer);
    }

    m_finishedImports.append(qmlTransfer);
    Q_EMIT finishedImportsChanged();
}

/*!
 * \brief ContentHub::handleExport handles an incoming request for sharing content
 * \internal
 */
void ContentHub::handleShare(com::ubuntu::content::Transfer *transfer)
{
    TRACE() << Q_FUNC_INFO;
    ContentTransfer *qmlTransfer = nullptr;
    if (m_activeImports.contains(transfer))
    {
        qmlTransfer = m_activeImports.take(transfer);
        qmlTransfer->collectItems();
    } else {
        // If we don't have a reference to the transfer, it was created
        // by another handler so this would be an Import
        qmlTransfer = new ContentTransfer(this);
        qmlTransfer->setTransfer(transfer);
        connect(qmlTransfer, SIGNAL(stateChanged()),
                this, SLOT(updateState()));
        qmlTransfer->collectItems();
        Q_EMIT shareRequested(qmlTransfer);
    }

    m_finishedImports.append(qmlTransfer);
    Q_EMIT finishedImportsChanged();
}

void ContentHub::updateState()
{
    TRACE() << Q_FUNC_INFO;
}

/*!
 * \qmlproperty bool ContentHub::hasPending
 * True if there is a pending transfer for the handler
 * \internal
 */
bool ContentHub::hasPending()
{
    TRACE() << Q_FUNC_INFO;
    return m_hasPending;
}

/*!
 * \qmlsignal ContentHub::importRequested(ContentTransfer transfer)
 *
 * The signal is triggered when an import is requested.
 */

/*!
 * \qmlsignal ContentHub::exportRequested(ContentTransfer transfer)
 *
 * The signal is triggered when an export is requested.
 */

/*!
 * \qmlsignal ContentHub::shareRequested(ContentTransfer transfer)
 *
 * The signal is triggered when a share is requested.
 */

