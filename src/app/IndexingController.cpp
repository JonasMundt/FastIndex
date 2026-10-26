
#include "IndexingController.hpp"

#include "fastindex/indexing/ParallelIndexingService.hpp"
#include "fastindex/storage/SqliteIndex.hpp"

#include <QByteArray>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <string_view>
#include <utility>

namespace {

std::filesystem::path toPath(const QString& value) {
#ifdef _WIN32
    return std::filesystem::path(value.toStdWString());
#else
    const auto utf8 = value.toUtf8();
    return std::filesystem::u8path(utf8.constData());
#endif
}

QString databaseLocation() {
    const QString directory =
        QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation
        );

    if (directory.isEmpty()) {
        return {};
    }

    return QDir(directory).filePath("fastindex.db");
}

} // namespace

// --------------------------------------------------
// Lifecycle and Settings
// --------------------------------------------------

IndexingController::IndexingController(QObject* parent)
    : QObject(parent) {

    QSettings settings;

    workerCount_ = std::clamp(
        settings.value("indexing/workerCount", 4).toInt(),
        1,
        16
    );

    const QString storedFolder =
        settings.value("indexing/lastFolder").toString();

    if (!storedFolder.isEmpty()) {
        lastFolder_ = QUrl::fromLocalFile(storedFolder);
    }
}

IndexingController::~IndexingController() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }

    if (searchWorker_.joinable()) {
        searchWorker_.request_stop();
        searchWorker_.join();
    }
}

bool IndexingController::busy() const noexcept {
    return busy_;
}

bool IndexingController::searching() const noexcept {
    return searching_;
}

int IndexingController::workerCount() const noexcept {
    return workerCount_;
}

QUrl IndexingController::lastFolder() const {
    return lastFolder_;
}

void IndexingController::setWorkerCount(int count) {
    if (busy_ || searching_) {
        return;
    }

    const int validatedCount =
        std::clamp(count, 1, 16);

    if (workerCount_ == validatedCount) {
        return;
    }

    workerCount_ = validatedCount;

    QSettings settings;
    settings.setValue(
        "indexing/workerCount",
        workerCount_
    );

    emit workerCountChanged();
}

void IndexingController::setLastFolder(
    const QUrl& folder
) {
    if (busy_ || searching_) {
        return;
    }

    if (!folder.isLocalFile()) {
        emit failed("Please select a local folder.");
        return;
    }

    const QString folderPath =
        QDir::cleanPath(folder.toLocalFile());

    if (!QDir(folderPath).exists()) {
        emit failed("Selected folder does not exist.");
        return;
    }

    const QUrl normalizedFolder =
        QUrl::fromLocalFile(folderPath);

    if (lastFolder_ == normalizedFolder) {
        return;
    }

    lastFolder_ = normalizedFolder;

    QSettings settings;
    settings.setValue(
        "indexing/lastFolder",
        folderPath
    );

    emit lastFolderChanged();
}

// --------------------------------------------------
// Indexing
// --------------------------------------------------

void IndexingController::startIndexing(
    const QUrl& folder
) {
    if (busy_ || searching_) {
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

    const QString databasePath = databaseLocation();

    if (databasePath.isEmpty() ||
        !QDir().mkpath(
            QFileInfo(databasePath).absolutePath()
        )) {
        emit failed(
            "Cannot create application data directory."
        );
        return;
    }

    // Capture configuration before starting the thread.
    const int selectedWorkers = workerCount_;

    busy_ = true;
    emit busyChanged();
    emit progressUpdated(0, 0);

    try {
        worker_ = std::jthread(
            [this, folderPath, databasePath, selectedWorkers](
                std::stop_token stop
            ) {
                try {
                    fastindex::storage::SqliteIndex database(
                        toPath(databasePath)
                    );

                    fastindex::indexing::ParallelIndexingService
                        service(
                            database,
                            static_cast<std::size_t>(
                                selectedWorkers
                            )
                        );

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
                                        processed,
                                        total
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
                        [this, processed, total,
                         indexed, skipped, cancelled] {
                            emit progressUpdated(
                                processed,
                                total
                            );

                            busy_ = false;
                            emit busyChanged();

                            emit indexingFinished(
                                cancelled,
                                indexed,
                                skipped
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

// --------------------------------------------------
// Full-Text Search
// --------------------------------------------------

void IndexingController::searchFiles(
    const QString& query
) {
    if (busy_ || searching_) {
        return;
    }

    const QString term = query.trimmed();

    if (term.isEmpty()) {
        emit searchFinished({}, 0);
        return;
    }

    const QString databasePath = databaseLocation();

    if (!QFileInfo::exists(databasePath)) {
        emit searchFailed(
            "No index found. Please index a folder first."
        );
        return;
    }

    searching_ = true;
    emit searchingChanged();

    try {
        searchWorker_ = std::jthread(
            [this, databasePath, term](
                std::stop_token stop
            ) {
                try {
                    if (stop.stop_requested()) {
                        return;
                    }

                    fastindex::storage::SqliteIndex database(
                        toPath(databasePath)
                    );

                    // Treat input as a literal FTS5 phrase.
                    QString phrase = term;
                    phrase.replace("\"", "\"\"");
                    phrase = "\"" + phrase + "\"";

                    const QByteArray utf8 = phrase.toUtf8();

                    const auto matches = database.search(
                        std::string_view(
                            utf8.constData(),
                            static_cast<std::size_t>(
                                utf8.size()
                            )
                        )
                    );

                    constexpr int maxVisibleResults = 100;

                    QVariantList results;
                    results.reserve(
                        std::min(
                            static_cast<int>(matches.size()),
                            maxVisibleResults
                        )
                    );

                    for (const auto& match : matches) {
                        if (results.size() >= maxVisibleResults ||
                            stop.stop_requested()) {
                            break;
                        }

                        const QString path =
                            QString::fromUtf8(
                                match.path.c_str()
                            );

                        QVariantMap item;

                        item.insert(
                            "name",
                            QFileInfo(path).fileName()
                        );

                        item.insert("path", path);

                        item.insert(
                            "snippet",
                            QString::fromUtf8(
                                match.snippet.c_str()
                            )
                        );

                        results.append(item);
                    }

                    if (stop.stop_requested()) {
                        return;
                    }

                    const int total =
                        static_cast<int>(matches.size());

                    QMetaObject::invokeMethod(
                        this,
                        [this,
                         results = std::move(results),
                         total] {
                            searching_ = false;
                            emit searchingChanged();

                            emit searchFinished(
                                results,
                                total
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
                            searching_ = false;
                            emit searchingChanged();
                            emit searchFailed(message);
                        },
                        Qt::QueuedConnection
                    );
                }
            }
        );
    }
    catch (const std::exception& error) {
        searching_ = false;
        emit searchingChanged();
        emit searchFailed(
            QString::fromUtf8(error.what())
        );
    }
}

// --------------------------------------------------
// Open Search Result
// --------------------------------------------------

void IndexingController::openFile(
    const QString& path
) {
    if (!QFileInfo(path).isFile()) {
        emit failed(
            "File no longer exists: " + path
        );
        return;
    }

    if (!QDesktopServices::openUrl(
            QUrl::fromLocalFile(path)
        )) {
        emit failed(
            "Cannot open file: " + path
        );
    }
}
