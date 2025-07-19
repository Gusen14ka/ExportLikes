#include "exportlikes.hpp"
#include "./ui_exportlikes.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>

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

    connect(ui->authButton, &QPushButton::clicked, this, &ExportLikes::onAuthButtonClicked);
    connect(spotifyClient_, &QtSpotifyClient::reauthorization, this, &ExportLikes::onReauthorization);
    connect(spotifyClient_, &QtSpotifyClient::finishedAuthorization, this, &ExportLikes::onFinishedAuthorization);

    connect(ui->chbDeveloper, &QCheckBox::checkStateChanged, this, &ExportLikes::onCheckDeleveloper);
    connect(ui->getTracksButton, &QPushButton::clicked, this, &ExportLikes::onGetTracksClicked);

    loadEnvFile();
}

void ExportLikes::loadEnvFile(){
    QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString envPath = configDir + "/.env";

    QFile f(envPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)){
        onLogMessage(".env not found or cannot open.");
        return;
    }
    while (!f.atEnd()){
        QString line = f.readLine().trimmed();
        if(line.startsWith("YANDEX_TOKEN=")){
            QString token = line.section("=", 1);
            ui->leYandexToken->setText(token);
            onLogMessage("Loaded token from .env");
            break;
        }
    }
    f.close();
}

bool ExportLikes::saveEnvFile(const QString& token){
    QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    QDir().mkdir(configDir);
    QString envPath = configDir + "/.env";

    QFile f(envPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        onLogMessage("Error: Cannot write .env");
        return false;
    }

    ui->logView->append(".env path: " + envPath);

    QTextStream out(&f);
    out << "YANDEX_TOKEN=" << token << "\n";
    f.close();
    onLogMessage("Token saved to .env");
    return true;
}

void ExportLikes::onGetTracksClicked(){
    QString token = ui->leYandexToken->text().trimmed();
    if(token.isEmpty()){
        QMessageBox::warning(this, "Error", "Please enter the Yandex Music token. If you don't know what is it and how to get it, read the guide");
        return;
    }

    saveEnvFile(token);

    // Prepare to launch Py scripte
    QString appDir = QCoreApplication::applicationDirPath();
    QString scriptPath = appDir + "/scripts/export_yandex_music_likes.exe";
    if(!QFile::exists(scriptPath)){
        qDebug() << "Script not found";
        return;
    }

    // Launch script
    // Connect output
    auto* proc = new QProcess(this);
    connect(proc, &QProcess::readyReadStandardOutput, [this, proc](){
        ui->logView->append(QString::fromLocal8Bit(proc->readAllStandardOutput()));
    });

    // Connect errors
    connect(proc, &QProcess::readyReadStandardError, [this,proc]() {
        ui->logView->append("<font color=\"red\">" +
                      QString::fromLocal8Bit(proc->readAllStandardError()) +
                      "</font>");
    });

    // Connect executing status
    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            [this,proc](int code, QProcess::ExitStatus status){
                ui->logView->append(
                    code == 0 ? "Script finished successfully"
                              : QString("<font color=\"red\">Script crashed, exit code %1</font>")
                                    .arg(code)
                    );
                proc->deleteLater();
            });

    // Start script
    ui->logView->append("Starting script...");
    proc->start(scriptPath);
}

void ExportLikes::onAddTracksClicked(){
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

void ExportLikes::onFinishedAuthorization(bool success){
    ui->authButton->setEnabled(true);
    if(success){
        QMessageBox::information(this, "Done",
                                 "Authorization is successful");
        ui->addButton->setEnabled(true);
        ui->removeButton->setEnabled(true);
    }
    else{
        QMessageBox::information(this, "Error",
                                 "Something gone wrong...");
    }
}

void ExportLikes::onReauthorization(){
    QMessageBox::information(this, "Reauthorization",
                             "Your token has expired, please authorize again.");
}

void ExportLikes::onAuthButtonClicked(){
    spotifyClient_->setClientId(ui->leClientId->text());
    spotifyClient_->setRedirectUri(ui->leRedirectUri->text());

    spotifyClient_->authorization();
    ui->authButton->setEnabled(false);
}

void ExportLikes::onCheckDeleveloper(bool check){
    if(check){
        ui->leClientId->setEnabled(true);
        ui->leRedirectUri->setEnabled(true);
    }
    else{
        ui->leClientId->setText(spotifyClient_->getClientId());
        ui->leClientId->setEnabled(false);

        ui->leRedirectUri->setEnabled(false);
        ui->leRedirectUri->setText(spotifyClient_->getRedirectUri());
    }
}
