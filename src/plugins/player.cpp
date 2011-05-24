/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QAbstractNetworkCache>
#include <QDeclarativeItem>
#include <QDir>
#include <QDomDocument>
#include <QDropEvent>
#include <QFileInfo>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QPixmapCache>
#include <QSettings>
#include <QTime>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

#include "application.h"
#include "chat.h"
#include "player.h"

enum PlayerRole {
    AlbumRole = ChatModel::UserRole,
    ArtistRole,
    DownloadingRole,
    DurationRole,
    ImageUrlRole,
    PlayingRole,
    TitleRole,
};

static bool isLocal(const QUrl &url)
{
    return url.scheme() == "file" || url.scheme() == "qrc";
}

class Item : public ChatModelItem
{
public:
    Item();
    bool updateMetaData(QSoundFile *file);

    QString album;
    QString artist;
    qint64 duration;
    QUrl imageUrl;
    QString title;
    QUrl url;
};

Item::Item()
    : duration(0)
{
}

// Update the metadata for the current item.
bool Item::updateMetaData(QSoundFile *file)
{
    if (!file || !file->open(QIODevice::ReadOnly))
        return false;

    duration = file->duration();

    QStringList values;
    values = file->metaData(QSoundFile::AlbumMetaData);
    if (!values.isEmpty())
        album = values.first();

    values = file->metaData(QSoundFile::ArtistMetaData);
    if (!values.isEmpty())
        artist = values.first();

    values = file->metaData(QSoundFile::TitleMetaData);
    if (!values.isEmpty())
        title = values.first();

    return true;
}

class PlayerModelPrivate
{
public:
    PlayerModelPrivate(PlayerModel *qq);
    QList<Item*> find(ChatModelItem *parent, int role, const QUrl &url);
    void processXml(Item *item, QIODevice *reply);
    void processQueue();
    void save();
    QIODevice *dataFile(Item *item);
    QSoundFile::FileType dataType(Item *item);
    QSoundFile *soundFile(Item *item);

    QMap<QUrl, QUrl> dataCache;
    QNetworkReply *dataReply;
    Item *cursorItem;
    QNetworkAccessManager *network;
    QNetworkDiskCache *networkCache;
    QSoundPlayer *player;
    int playId;
    bool playStop;
    PlayerModel *q;
};

PlayerModelPrivate::PlayerModelPrivate(PlayerModel *qq)
    : dataReply(0),
    cursorItem(0),
    playId(-1),
    playStop(false),
    q(qq)
{
}

QList<Item*> PlayerModelPrivate::find(ChatModelItem *parent, int role, const QUrl &url)
{
    QList<Item*> items;
    foreach (ChatModelItem *it, parent->children) {
        Item *item = static_cast<Item*>(it);
        if (url.isEmpty() ||
            (role == ChatModel::UrlRole && item->url == url) ||
            (role == ImageUrlRole && item->imageUrl == url))
            items << item;
        if (!item->children.isEmpty())
            items << find(item, role, url);
    }
    return items;
}

void PlayerModelPrivate::processQueue()
{
    if (dataReply)
        return;

    QList<Item*> items = find(q->rootItem, ChatModel::UrlRole, QUrl());
    foreach (Item *item, items) {
        // check data
        if (item->url.isValid() &&
            !isLocal(item->url) &&
            !dataCache.contains(item->url))
        {
            //qDebug("Requesting data %s", qPrintable(item->url.toString()));
            dataReply = network->get(QNetworkRequest(item->url));
            dataReply->setProperty("_request_url", item->url);
            q->connect(dataReply, SIGNAL(finished()), q, SLOT(dataReceived()));
            emit q->dataChanged(q->createIndex(item), q->createIndex(item));
            return;
        }
    }
}

void PlayerModelPrivate::processXml(Item *channel, QIODevice *reply)
{
    // parse document
    QDomDocument doc;
    if (!doc.setContent(reply)) {
        qWarning("Received invalid XML");
        return;
    }

    QDomElement channelElement = doc.documentElement().firstChildElement("channel");

    // parse channel
    channel->artist = channelElement.firstChildElement("itunes:author").text();
    channel->title = channelElement.firstChildElement("title").text();
    QDomElement imageUrlElement = channelElement.firstChildElement("image").firstChildElement("url");
    if (!imageUrlElement.isNull())
        channel->imageUrl = QUrl(imageUrlElement.text());
    emit q->dataChanged(q->createIndex(channel), q->createIndex(channel));

    // parse items
    QDomElement itemElement = channelElement.firstChildElement("item");
    while (!itemElement.isNull()) {
        const QString title = itemElement.firstChildElement("title").text();
        QUrl audioUrl;
        QDomElement enclosureElement = itemElement.firstChildElement("enclosure");
        while (!enclosureElement.isNull() && !audioUrl.isValid()) {
            if (QSoundFile::typeFromMimeType(enclosureElement.attribute("type").toAscii()) != QSoundFile::UnknownFile)
                audioUrl = QUrl(enclosureElement.attribute("url"));
            enclosureElement = enclosureElement.nextSiblingElement("enclosure");
        }

        Item *item = new Item;
        item->url = audioUrl;
        item->artist = channel->artist;
        item->imageUrl = channel->imageUrl;
        item->title = title;
        const QStringList durationBits = itemElement.firstChildElement("itunes:duration").text().split(":");
        if (durationBits.size() == 3)
            item->duration = ((((durationBits[0].toInt() * 60) + durationBits[1].toInt()) * 60) + durationBits[2].toInt()) * 1000;

        q->addItem(item, channel);
        itemElement = itemElement.nextSiblingElement("item");
    }
}

QIODevice *PlayerModelPrivate::dataFile(Item *item)
{
    if (isLocal(item->url))
        return new QFile(item->url.toLocalFile());
    else if (dataCache.contains(item->url)) {
        const QUrl dataUrl = dataCache.value(item->url);
        return networkCache->data(dataUrl);
    }
    return 0;
}

QSoundFile::FileType PlayerModelPrivate::dataType(Item *item)
{
    if (isLocal(item->url)) {
        return QSoundFile::typeFromFileName(item->url.toLocalFile());
    } else if (dataCache.contains(item->url)) {
        const QUrl audioUrl = dataCache.value(item->url);
        QNetworkCacheMetaData::RawHeaderList headers = networkCache->metaData(audioUrl).rawHeaders();
        foreach (const QNetworkCacheMetaData::RawHeader &header, headers)
        {
            if (header.first.toLower() == "content-type")
                return QSoundFile::typeFromMimeType(header.second);
        }
    }
    return QSoundFile::UnknownFile;
}

QSoundFile *PlayerModelPrivate::soundFile(Item *item)
{
    QSoundFile::FileType type = dataType(item);
    if (type != QSoundFile::UnknownFile) {
        QIODevice *device = dataFile(item);
        if (device) {
            QSoundFile *file = new QSoundFile(device, type);
            device->setParent(file);
            return file;
        }
    }
    return 0;
}

void PlayerModelPrivate::save()
{
    QSettings settings;
    QStringList values;
    foreach (ChatModelItem *item, q->rootItem->children)
        values << static_cast<Item*>(item)->url.toString();
    settings.setValue("PlayerUrls", values);
}

PlayerModel::PlayerModel(QObject *parent)
    : ChatModel(parent)
{
    bool check;
    d = new PlayerModelPrivate(this);

    d->networkCache = new QNetworkDiskCache(this);
    d->networkCache->setCacheDirectory(wApp->cacheDirectory());
    d->network = new QNetworkAccessManager(this);
    d->network->setCache(d->networkCache);

    // init player
    d->player = wApp->soundPlayer();
    check = connect(d->player, SIGNAL(finished(int)),
                    this, SLOT(finished(int)));
    Q_ASSERT(check);

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(AlbumRole, "album");
    roleNames.insert(ArtistRole, "artist");
    roleNames.insert(DownloadingRole, "downloading");
    roleNames.insert(DurationRole, "duration");
    roleNames.insert(ImageUrlRole, "imageUrl");
    roleNames.insert(PlayingRole, "playing");
    roleNames.insert(TitleRole, "title");
    roleNames.insert(UrlRole, "url");
    setRoleNames(roleNames);

    // load saved playlist
    QSettings settings;
    QStringList values = settings.value("PlayerUrls").toStringList();
    foreach (const QString &value, values)
        addUrl(QUrl(value));
}

PlayerModel::~PlayerModel()
{
    delete d;
}

bool PlayerModel::addUrl(const QUrl &url)
{
    if (!url.isValid())
        return false;

    Item *item = new Item;
    item->title = QFileInfo(url.path()).baseName();
    item->imageUrl = QUrl("qrc:/file.png");
    item->url = url;

    // fetch meta data
    QSoundFile *file = d->soundFile(item);
    if (file) {
        item->updateMetaData(file);
        delete file;
    }

    addItem(item, rootItem);

    d->processQueue();

    // save playlist
    d->save();
    return true;
}

void PlayerModel::dataReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    const QUrl dataUrl = reply->property("_request_url").toUrl();

    // follow redirect
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        redirectUrl = reply->url().resolved(redirectUrl);

        //qDebug("Following redirect to %s", qPrintable(redirectUrl.toString()));
        d->dataReply = d->network->get(QNetworkRequest(redirectUrl));
        d->dataReply->setProperty("_request_url", dataUrl);
        connect(d->dataReply, SIGNAL(finished()), this, SLOT(dataReceived()));
        return;
    }

    //qDebug("Received data for %s", qPrintable(dataUrl.toString()));
    d->dataCache[dataUrl] = reply->url();
    d->dataReply = 0;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("%s", qPrintable(reply->errorString()));

        QList<Item*> items = d->find(rootItem, UrlRole, dataUrl);
        foreach (Item *item, items)
            emit dataChanged(createIndex(item), createIndex(item));

        d->processQueue();
        return;
    }

    // process data
    const QString mimeType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    QList<Item*> items = d->find(rootItem, UrlRole, dataUrl);
    foreach (Item *item, items) {
        if (mimeType == "application/xml") {
            QIODevice *device = d->dataFile(item);
            d->processXml(item, device);
            delete device;
        } else {
            QSoundFile *file = d->soundFile(item);
            if (file) {
                item->updateMetaData(file);
                delete file;
            }
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
    d->processQueue();
}

int PlayerModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QModelIndex PlayerModel::cursor() const
{
    return createIndex(d->cursorItem);
}

void PlayerModel::setCursor(const QModelIndex &index)
{
    Item *item = static_cast<Item*>(index.internalPointer());

    if (item != d->cursorItem) {
        Item *oldItem = d->cursorItem;
        d->cursorItem = item;
        if (oldItem)
            emit dataChanged(createIndex(oldItem), createIndex(oldItem));
        if (item)
            emit dataChanged(createIndex(item), createIndex(item));
        emit cursorChanged(index);
        emit playingChanged(d->cursorItem != 0);
    }
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AlbumRole)
        return item->album;
    else if (role == ArtistRole)
        return item->artist;
    else if (role == DownloadingRole)
        return d->dataReply && d->dataReply->property("_request_url").toUrl() == item->url;
    else if (role == DurationRole)
        return item->duration;
    else if (role == ImageUrlRole)
        return item->imageUrl;
    else if (role == PlayingRole)
        return (item == d->cursorItem);
    else if (role == TitleRole)
        return item->title;
    else if (role == UrlRole)
        return item->url;

    return QVariant();
}

void PlayerModel::finished(int id)
{
    if (id != d->playId)
        return;
    d->playId = -1;
    if (d->playStop) {
        setCursor(QModelIndex());
        d->playStop = false;
    } else {
        const QModelIndex index = cursor();
        QModelIndex nextIndex = index.sibling(index.row() + 1, index.column());
        play(nextIndex);
    }
}

void PlayerModel::play(const QModelIndex &index)
{
    // stop previous audio
    if (d->playId >= 0) {
        d->player->stop(d->playId);
        d->playId = -1;
    }

    // start new audio output
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item) {
        setCursor(QModelIndex());
        return;
    }

    QSoundFile *file = d->soundFile(item);
    if (file) {
        d->playId = d->player->play(file);
        setCursor(index);
    } else {
        QModelIndex nextIndex = index.sibling(index.row() + 1, index.column());
        play(nextIndex);
    }
}

bool PlayerModel::playing() const
{
    return d->cursorItem != 0;
}

bool PlayerModel::removeRow(int row, const QModelIndex &parent)
{
    return QAbstractItemModel::removeRow(row, parent);
}

bool PlayerModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (ChatModel::removeRows(row, count, parent)) {
        // save playlist
        d->save();
        return true;
    }
    return false;
}

void PlayerModel::stop()
{
    if (d->playId >= 0) {
        d->player->stop(d->playId);
        d->playStop = true;
    }
}

#if 0
// FIXME: restore drag and drop
// view->viewport()->setAcceptDrops(true);
// view->viewport()->installEventFilter(this);
static QList<QUrl> getUrls(const QUrl &url) {
    if (!isLocal(url))
        return QList<QUrl>() << url;

    QList<QUrl> urls;
    const QString path = url.toLocalFile();
    if (QFileInfo(path).isDir()) {
        QDir dir(path);
        QStringList children = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        foreach (const QString &child, children)
            urls << getUrls(QUrl::fromLocalFile(dir.filePath(child)));
    } else if (QSoundFile::typeFromFileName(path) != QSoundFile::UnknownFile) {
        urls << url;
    }
    return urls;
}

bool PlayerPanel::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);

    if (e->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *event = static_cast<QDragEnterEvent*>(e);
        event->acceptProposedAction();
        return true;
    }
    else if (e->type() == QEvent::DragLeave)
    {
        return true;
    }
    else if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop)
    {
        QDropEvent *event = static_cast<QDropEvent*>(e);

        int found = 0;
        foreach (const QUrl &url, event->mimeData()->urls())
        {
            // check protocol is supported
            if (!isLocal(url) &&
                url.scheme() != "http" &&
                url.scheme() != "https")
                continue;

            if (event->type() == QEvent::Drop) {
                foreach (const QUrl &child, getUrls(url))
                    m_model->addUrl(child);
            }
            found++;
        }
        if (found)
            event->acceptProposedAction();
        else
            event->ignore();

        return true;
    }
    return false;
}
#endif

