
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <thread>

class IndexingController : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        bool busy
        READ busy
        NOTIFY busyChanged
    )

    Q_PROPERTY(
        bool searching
        READ searching
        NOTIFY searchingChanged
    )

public:
    explicit IndexingController(QObject* parent = nullptr);
    ~IndexingController() override;

    [[nodiscard]]
    bool busy() const noexcept;

    [[nodiscard]]
    bool searching() const noexcept;

    Q_INVOKABLE void startIndexing(const QUrl& folder);
    Q_INVOKABLE void cancelIndexing();

    Q_INVOKABLE void searchFiles(const QString& query);
    Q_INVOKABLE void openFile(const QString& path);

signals:
    void busyChanged();
    void searchingChanged();

    void progressUpdated(int processed, int total);

    void indexingFinished(
        bool cancelled,
        int indexed,
        int skipped
    );

    void searchFinished(
        QVariantList results,
        int total
    );

    void searchFailed(const QString& message);
    void failed(const QString& message);

private:
    bool busy_ = false;
    bool searching_ = false;

    std::jthread worker_;
    std::jthread searchWorker_;
};
