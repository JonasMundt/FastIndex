
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <thread>

class IndexingController : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        bool busy
        READ busy
        NOTIFY busyChanged
    )

public:
    explicit IndexingController(QObject* parent = nullptr);
    ~IndexingController() override;

    [[nodiscard]]
    bool busy() const noexcept;

    Q_INVOKABLE void startIndexing(const QUrl& folder);
    Q_INVOKABLE void cancelIndexing();

signals:
    void busyChanged();

    void progressUpdated(int processed, int total);

    void indexingFinished(
        bool cancelled,
        int indexed,
        int skipped
    );

    void failed(const QString& message);

private:
    bool busy_ = false;
    std::jthread worker_;
};
