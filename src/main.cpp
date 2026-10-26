
#include "app/IndexingController.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QVariant>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("FastIndex");
    QCoreApplication::setApplicationName("FastIndex");

    IndexingController controller;

    QQmlApplicationEngine engine;

    engine.setInitialProperties({
        {
            "controller",
            QVariant::fromValue(
                static_cast<QObject*>(&controller)
            )
        }
    });

    engine.loadFromModule("FastIndex", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
