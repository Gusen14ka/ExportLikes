#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include "QtSpotifyClient.hpp"
#include "ui_exportlikes.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ExportLikes;
}
QT_END_NAMESPACE

class ExportLikes : public QMainWindow {
    Q_OBJECT

public:
    ExportLikes(QWidget* parent = nullptr);
    ~ExportLikes() override = default;

private slots:
    void onAddTracksClicked();
    void onRemoveTracksClicked();
    void onLogMessage(const QString& msg);
    void onProgress(int current, int total);
    void onFinishedAdding(bool success);
    void onFinishedRemoving(bool success);

private:
    std::unique_ptr<Ui::ExportLikes> ui;

    QtSpotifyClient* spotifyClient_;
};
