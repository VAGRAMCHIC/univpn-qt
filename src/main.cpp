#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTranslator>
#include <QLocale>
#include <QStandardPaths>
#include <QDir>
#include <QSharedMemory>
#include <QDebug>

#include "core/app_controller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("clgvpn"));
    app.setOrganizationName(QStringLiteral("clgvpn"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    // Single-instance
    QSharedMemory sharedMemory(QStringLiteral("clgvpn-single-instance"));
    if (!sharedMemory.create(1)) {
        qWarning("clgvpn is already running.");
        return 1;
    }

    // Каталог данных
    const QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir + QStringLiteral("/core"));

    // AppController — фасад для QML
    AppController controller;
    controller.init(dataDir + QStringLiteral("/core"));

    // Переводчик
    QTranslator translator;
    const QString localeCode = controller.localeCode();
    const QLocale locale = localeCode.isEmpty()
                               ? QLocale::system()
                               : QLocale(localeCode);
    if (translator.load(locale, QStringLiteral("app"), QStringLiteral("_"),
                        QStringLiteral(":/i18n"))) {
        QCoreApplication::installTranslator(&translator);
    }

    // QML
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(
        QStringLiteral("App"), &controller);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("Clgvpn", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
