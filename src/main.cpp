#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Keepnote");
    app.setApplicationName("keepnote");
    app.setDesktopFileName("keepnote");
    app.setWindowIcon(QIcon::fromTheme("keepnote"));
    QQuickStyle::setStyle("Material");

    Backend backend;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &backend, &Backend::flush);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
