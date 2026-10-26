
#include "IndexingController.hpp"

#include "fastindex/indexing/ParallelIndexingService.hpp"
#include "fastindex/storage/SqliteIndex.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <QStandardPaths>

#include <exception>
#include <filesystem>
#include <string>

namespace {

std::filesystem::path toPath(const QString& value) {
#ifdef _WIN32
    return std::filesystem::path(value.toStdWString());
#else
    const auto utf8 = value.toUtf8();
    return std::filesystem::u8path(utf8.constData());
#endif
}

} // namespace

IndexingController::IndexingController(QObject* parent)
    : QObject(parent) {
}

IndexingController::~IndexingController() {
    if (worker_.joinable()) {
        worker_.request_stop();
    }
}

bool IndexingController::busy() const noexcept {
    return busy_;
}

void IndexingController::startIndexing(
    const QUrl& folder
) {
    if (busy_) {
        return;
    }

    if (!folder.isLocalFile()) {
        emit failed("Please select a local folder.");
        return;
    }

    const QString folderPath = folder.toLocalFile();

    if (!QDir(folderPath).exists()) {
        emit failed("Selected folder does not exist.");
        return;
    }

    const QString dataDirectory =
        QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation
        );

    if (dataDirectory.isEmpty() ||
        !QDir().mkpath(dataDirectory)) {
        emit failed("Cannot create application data directory.");
        return;
    }

    const QString databasePath =
        QDir(dataDirectory).filePath("fastindex.db");

    busy_ = true;
    emit busyChanged();
    emit progressUpdated(0, 0);

    try {
        worker_ = std::jthread(
            [this, folderPath, databasePath](
                std::stop_token stop
            ) {
                try {
                    fastindex::storage::SqliteIndex database(
                        toPath(databasePath)
                    );

                    fastindex::indexing::ParallelIndexingService
                        service(database, 4);

                    const auto stats = service.indexDirectory(
                        toPath(folderPath),
                        stop,
                        [this](const auto& progress) {
                            const int processed =
                                static_cast<int>(
                                    progress.indexed +
                                    progress.skipped
                                );

                            const int total =
                                static_cast<int>(
                                    progress.discovered
                                );

                            QMetaObject::invokeMethod(
                                this,
                                [this, processed, total] {
                                    emit progressUpdated(
                                        processed, total
                                    );
                                },
                                Qt::QueuedConnection
                            );
                        }
                    );

                    const int processed =
                        static_cast<int>(
                            stats.indexed + stats.skipped
                        );

                    const int total =
                        static_cast<int>(stats.discovered);

                    const int indexed =
                        static_cast<int>(stats.indexed);

                    const int skipped =
                        static_cast<int>(stats.skipped);

                    const bool cancelled = stats.cancelled;

                    QMetaObject::invokeMethod(
                        this,
                        [this, processed, total, indexed,
                         skipped, cancelled] {
                            emit progressUpdated(
                                processed, total
                            );

                            busy_ = false;
                            emit busyChanged();

                            emit indexingFinished(
                                cancelled, indexed, skipped
                            );
                        },
                        Qt::QueuedConnection
                    );
                }
                catch (const std::exception& error) {
                    const QString message =
                        QString::fromUtf8(error.what());

                    QMetaObject::invokeMethod(
                        this,
                        [this, message] {
                            busy_ = false;
                            emit busyChanged();
                            emit failed(message);
                        },
                        Qt::QueuedConnection
                    );
                }
            }
        );
    }
    catch (const std::exception& error) {
        busy_ = false;
        emit busyChanged();
        emit failed(QString::fromUtf8(error.what()));
    }
}

void IndexingController::cancelIndexing() {
    if (busy_ && worker_.joinable()) {
        worker_.request_stop();
    }
}
