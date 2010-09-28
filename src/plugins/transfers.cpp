/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "QXmppClient.h"
#include "QXmppShareExtension.h"
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "systeminfo.h"
#include "transfers.h"
#include "chat_utils.h"

static qint64 fileSizeLimit = 50000000; // 50 MB

enum TransfersColumns {
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

static QIcon jobIcon(QXmppTransferJob *job)
{
    if (job->state() == QXmppTransferJob::FinishedState)
    {
        if (job->error() == QXmppTransferJob::NoError)
            return QIcon(":/contact-available.png");
        else
            return QIcon(":/contact-busy.png");
    }
    return QIcon(":/contact-offline.png");
}

ChatTransferPrompt::ChatTransferPrompt(QXmppTransferJob *job, const QString &contactName, QWidget *parent)
    : QMessageBox(parent), m_job(job)
{
    setIcon(QMessageBox::Question);
    setText(tr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?")
            .arg(contactName, job->fileName(), sizeToString(job->fileSize())));
    setWindowModality(Qt::NonModal);
    setWindowTitle(tr("File from %1").arg(contactName));

    setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    setDefaultButton(QMessageBox::NoButton);

    /* connect signals */
    connect(this, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(slotButtonClicked(QAbstractButton*)));
}

void ChatTransferPrompt::slotButtonClicked(QAbstractButton *button)
{
    if (standardButton(button) == QMessageBox::Yes)
        emit fileAccepted(m_job);
    else
        emit fileDeclined(m_job);
}

ChatTransfersView::ChatTransfersView(QWidget *parent)
    : QTableWidget(parent)
{
    setColumnCount(MaxColumn);
    setHorizontalHeaderItem(NameColumn, new QTableWidgetItem(tr("File name")));
    setHorizontalHeaderItem(SizeColumn, new QTableWidgetItem(tr("Size")));
    setHorizontalHeaderItem(ProgressColumn, new QTableWidgetItem(tr("Progress")));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setShowGrid(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setResizeMode(NameColumn, QHeaderView::Stretch);
    
    connect(this, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(slotDoubleClicked(int,int)));
}

void ChatTransfersView::addJob(QXmppTransferJob *job)
{
    if (jobs.contains(job))
        return;

    const QString fileName = QFileInfo(job->data(QXmppShareExtension::LocalPathRole).toString()).fileName();

    jobs.insert(0, job);
    insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileName);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    nameItem->setIcon(jobIcon(job));
    setItem(0, NameColumn, nameItem);

    QTableWidgetItem *sizeItem = new QTableWidgetItem(sizeToString(job->fileSize()));
    sizeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setItem(0, SizeColumn, sizeItem);

    QProgressBar *progress = new QProgressBar;
    progress->setMaximum(100);
    setCellWidget(0, ProgressColumn, progress);

    connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyed(QObject*)));
    connect(job, SIGNAL(finished()), this, SLOT(slotFinished()));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(slotProgress(qint64, qint64)));
    connect(job, SIGNAL(stateChanged(QXmppTransferJob::State)), this, SLOT(slotStateChanged(QXmppTransferJob::State)));
}

QXmppTransferJob *ChatTransfersView::currentJob()
{
    int jobRow = currentRow();
    if (jobRow < 0 || jobRow >= jobs.size())
        return 0;
    return jobs.at(jobRow);
}

void ChatTransfersView::removeCurrentJob()
{
    QXmppTransferJob *job = currentJob();
    if (!job)
        return;
    if (job->state() == QXmppTransferJob::FinishedState)
        job->deleteLater();
    else
        job->abort();
}

void ChatTransfersView::slotDestroyed(QObject *obj)
{
    int jobRow = jobs.indexOf(static_cast<QXmppTransferJob*>(obj));
    if (jobRow < 0)
        return;

    jobs.removeAt(jobRow);
    removeRow(jobRow);
    emit updateButtons();
}

void ChatTransfersView::slotDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row < 0 || row >= jobs.size())
        return;

    QXmppTransferJob *job = jobs.at(row);
    const QString localFilePath = job->data(QXmppShareExtension::LocalPathRole).toString();
    if (localFilePath.isEmpty())
        return;
    if (job->direction() == QXmppTransferJob::IncomingDirection &&
        (job->state() != QXmppTransferJob::FinishedState || job->error() != QXmppTransferJob::NoError))
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath));
}

void ChatTransfersView::slotFinished()
{
    // find the job that completed
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    // if the job failed, reset the progress bar
    const QString localFilePath = job->data(QXmppShareExtension::LocalPathRole).toString();
    QProgressBar *progress = qobject_cast<QProgressBar*>(cellWidget(jobRow, ProgressColumn));
    if (progress && job->error() != QXmppTransferJob::NoError)
    {
        progress->reset();

        // delete failed downloads
        if (job->direction() == QXmppTransferJob::IncomingDirection &&
            !localFilePath.isEmpty())
            QFile(localFilePath).remove();
    }
}

void ChatTransfersView::slotProgress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    QProgressBar *progress = qobject_cast<QProgressBar*>(cellWidget(jobRow, ProgressColumn));
    if (progress && total > 0)
    {
        progress->setValue((100 * done) / total);
        qint64 speed = job->speed();
        if (job->direction() == QXmppTransferJob::IncomingDirection)
            progress->setToolTip(tr("Downloading at %1").arg(speedToString(speed)));
        else
            progress->setToolTip(tr("Uploading at %1").arg(speedToString(speed)));
    }
}

void ChatTransfersView::slotStateChanged(QXmppTransferJob::State state)
{
    Q_UNUSED(state);

    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    item(jobRow, NameColumn)->setIcon(jobIcon(job));
    emit updateButtons();
}

ChatTransfers::ChatTransfers(QXmppClient *xmppClient, ChatRosterModel *chatRosterModel, QWidget *parent)
    : ChatPanel(parent), client(xmppClient), rosterModel(chatRosterModel)
{
    // disable in-band bytestreams
    client->transferManager().setSupportedMethods(
        QXmppTransferJob::SocksMethod);

    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("File transfers"));

    /* help label */
    QLabel *helpLabel = new QLabel(tr("The file transfer feature is experimental and the transfer speed is limited so as not to interfere with your internet connection."));
    helpLabel->setWordWrap(true);
    layout()->addWidget(helpLabel);

    /* download location label */
    const QString downloadsLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(SystemInfo::storageLocation(SystemInfo::DownloadsLocation)).toString(),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    QLabel *downloadsLabel = new QLabel(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(downloadsLink));
    downloadsLabel->setOpenExternalLinks(true);
    downloadsLabel->setWordWrap(true);
    layout()->addWidget(downloadsLabel);

    /* transfers list */
    tableWidget = new ChatTransfersView;
    connect(tableWidget, SIGNAL(updateButtons()), this, SLOT(updateButtons()));
    connect(tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(updateButtons()));
    layout()->addWidget(tableWidget);

    /* buttons */
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    removeButton = new QPushButton;
    connect(removeButton, SIGNAL(clicked()), tableWidget, SLOT(removeCurrentJob()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout()->addWidget(buttonBox);

    updateButtons();

    /* connect signals */
    connect(&client->transferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
        this, SLOT(fileReceived(QXmppTransferJob*)));
}

ChatTransfers::~ChatTransfers()
{
}

void ChatTransfers::addJob(QXmppTransferJob *job)
{
    tableWidget->addJob(job);
    emit registerPanel();
}

void ChatTransfers::fileAccepted(QXmppTransferJob *job)
{
    // determine file location
    QDir downloadsDir(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));
    const QString filePath = QXmppShareExtension::availableFilePath(downloadsDir.path(), job->fileName());

    // open file
    QFile *file = new QFile(filePath, job);
    if (!file->open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write to" << filePath;
        job->abort();
        return;
    }

    // add transfer to list
    job->setData(QXmppShareExtension::LocalPathRole, filePath);
    addJob(job);

    // start transfer
    job->accept(file);
}

void ChatTransfers::fileDeclined(QXmppTransferJob *job)
{
    job->abort();
}

void ChatTransfers::fileReceived(QXmppTransferJob *job)
{
    // if the job was already accepted or refused (by the shares plugin)
    // stop here
    if (job->state() != QXmppTransferJob::OfferState)
        return;

    const QString bareJid = jidToBareJid(job->jid());
//    const QString contactName = rosterModel->contactName(bareJid);

    // prompt user
    ChatTransferPrompt *dlg = new ChatTransferPrompt(job, bareJid, this);
    connect(dlg, SIGNAL(fileAccepted(QXmppTransferJob*)), this, SLOT(fileAccepted(QXmppTransferJob*)));
    connect(dlg, SIGNAL(fileDeclined(QXmppTransferJob*)), this, SLOT(fileDeclined(QXmppTransferJob*)));
    dlg->show();
}

/** Handle file drag & drop on roster entries.
 */
void ChatTransfers::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    if (!client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    if (type != ChatRosterItem::Contact || !event->mimeData()->hasUrls())
        return;

    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    QStringList fullJids = rosterModel->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
    if (fullJids.isEmpty())
        return;

    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        if (url.scheme() != "file")
            continue;
        if (event->type() == QEvent::Drop)
            sendFile(fullJids.first(), url.toLocalFile());
        found++;
    }
    if (found)
        event->acceptProposedAction();
}

void ChatTransfers::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    if (!client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Contact)
    {
        QStringList fullJids = rosterModel->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
        if (fullJids.isEmpty())
            return;

        QAction *action = menu->addAction(QIcon(":/add.png"), tr("Send a file"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), this, SLOT(sendFilePrompt()));
    }
}

void ChatTransfers::sendFile(const QString &fullJid, const QString &filePath)
{
    // check file size
    if (QFileInfo(filePath).size() > fileSizeLimit)
    {
        QMessageBox::warning(this,
            tr("Send a file"),
            tr("Sorry, but you cannot send files bigger than %1.")
                .arg(sizeToString(fileSizeLimit)));
        return;
    }

    // send file
    QXmppTransferJob *job = client->transferManager().sendFile(fullJid, filePath);
    job->setData(QXmppShareExtension::LocalPathRole, filePath);
    addJob(job);
}

void ChatTransfers::sendFileAccepted(const QString &filePath)
{
    QString fullJid = sender()->objectName();
    sendFile(fullJid, filePath);
}

void ChatTransfers::sendFilePrompt()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QFileDialog *dialog = new QFileDialog(this, tr("Send a file"));
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setObjectName(fullJid);
    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(deleteLater()));
    dialog->open(this, SLOT(sendFileAccepted(QString)));
}

QSize ChatTransfers::sizeHint() const
{
    return QSize(400, 200);
}

void ChatTransfers::updateButtons()
{
    QXmppTransferJob *job = tableWidget->currentJob();
    if (!job)
    {
        removeButton->setEnabled(false);
        removeButton->setIcon(QIcon(":/remove.png"));
    }
    else if (job->state() == QXmppTransferJob::FinishedState)
    {
        removeButton->setIcon(QIcon(":/remove.png"));
        removeButton->setEnabled(true);
    }
    else
    {
        removeButton->setIcon(QIcon(":/close.png"));
        removeButton->setEnabled(true);
    }
}

// PLUGIN

class TransfersPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool TransfersPlugin::initialize(Chat *chat)
{
    /* register panel */
    ChatTransfers *transfers = new ChatTransfers(chat->client(), chat->rosterModel());
    transfers->setObjectName("transfers");
    chat->addPanel(transfers);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
            transfers, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            transfers, SLOT(rosterMenu(QMenu*, QModelIndex)));

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_T), chat);
    connect(shortcut, SIGNAL(activated()), transfers, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(transfers, TransfersPlugin)

