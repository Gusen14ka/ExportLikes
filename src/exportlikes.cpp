#include "exportlikes.hpp"
#include "./ui_exportlikes.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

ExportLikes::ExportLikes(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::ExportLikes),
    spotifyClient_(new QtSpotifyClient(this))
{
    ui->setupUi(this);

    // Connect signals
    connect(ui->addButton, &QPushButton::clicked, this, &ExportLikes::onAddTracksClicked);
    connect(spotifyClient_, &QtSpotifyClient::logMessage, this, &ExportLikes::onLogMessage);
    connect(spotifyClient_, &QtSpotifyClient::progress, this, &ExportLikes::onProgress);
    connect(spotifyClient_, &QtSpotifyClient::finishedAdding, this, &ExportLikes::onFinishedAdding);

    connect(ui->removeButton, &QPushButton::clicked, this, &ExportLikes::onRemoveTracksClicked);
    connect(spotifyClient_, &QtSpotifyClient::finishedRemoving, this, &ExportLikes::onFinishedRemoving);
}

void ExportLikes::onAddTracksClicked(){
    // Entering a clientId
    bool ok = false;
    QString clientId = QInputDialog::getText(
        this,
        "Client Id",
        "Enter Spotify Client ID:",
        QLineEdit::Normal,
        QString(),
        &ok
    );
    if(!ok || clientId.isEmpty()){
        return;
    }

    // Entering redirectUri
    QString redirectUri = QInputDialog::getText(
        this,
        "Redirect URI",
        "Enter redirect URI:",
        QLineEdit::Normal,
        "http://localhost:8888/callback",
        &ok
        );
    if(!ok || redirectUri.isEmpty()){
        return;
    }

    // Enter path to JSON-file
    QString path = QFileDialog::getOpenFileName(
        this,
        "Chose JSON file with tracks",
        "",
        "JSON Files (*.json)"
    );
    if(path.isEmpty()){
        return;
    }

    // Configure the client
    spotifyClient_->setClientId(clientId);
    spotifyClient_->setRedirectUri(redirectUri);
    spotifyClient_->loadLocalJson(path);

    // Launch pipeline
    spotifyClient_->addTracks();
    ui->addButton->setEnabled(false);
}

void ExportLikes::onLogMessage(const QString& msg){
    ui->logView->append("🛈 " + msg);
}

void ExportLikes::onProgress(int current, int total){
    ui->progressLabel->setText(QString("Progress: %1/%2").arg(current).arg(total));
}

void ExportLikes::onFinishedAdding(bool success){
    ui->addButton->setEnabled(true);
    if(success){
        QMessageBox::information(this, "Done",
            "Tracks has been successfuly added to Spotify \"Liked library\"");
    }
    else{
        QMessageBox::information(this, "Error",
            "Something gone wrong...");
    }
}

void ExportLikes::onRemoveTracksClicked(){

    // Entering a clientId
    bool ok = false;
    QString clientId = QInputDialog::getText(
        this,
        "Client Id",
        "Enter Spotify Client ID:",
        QLineEdit::Normal,
        QString(),
        &ok
        );
    if(!ok || clientId.isEmpty()){
        return;
    }

    // Entering redirectUri
    QString redirectUri = QInputDialog::getText(
        this,
        "Redirect URI",
        "Enter redirect URI:",
        QLineEdit::Normal,
        "http://localhost:8888/callback",
        &ok
        );
    if(!ok || redirectUri.isEmpty()){
        return;
    }

    int n = QInputDialog::getInt(
        this,
        "Remove tracks from \"Like's library\"",
        "Enter a number of tracks to remove",
        1,
        1,
        INT_MAX,
        1,
        &ok
    );

    // Configure the client
    spotifyClient_->setClientId(clientId);
    spotifyClient_->setRedirectUri(redirectUri);

    spotifyClient_->removeLastNTracks(n);
    ui->removeButton->setEnabled(false);
}

void ExportLikes::onFinishedRemoving(bool success){
    ui->removeButton->setEnabled(true);
    if(success){
        QMessageBox::information(this, "Done",
                                 "Tracks has been successfuly removed from Spotify \"Liked library\"");
    }
    else{
        QMessageBox::information(this, "Error",
                                 "Something gone wrong...");
    }
}
