#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "core/VaultManager.h"
#include "core/GeocodeCache.h"
#include "core/MessageIngestor.h"
#include "core/PhotoIngestor.h"
#include "models/FileTreeModel.h"
#include "models/ArticleModel.h"

int main(int argc, char *argv[])
{
    // Use Basic style for full customization support
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("Arkive");
    app.setOrganizationName("Arkive");
    app.setApplicationVersion("0.1.0");

    // Initialize vault manager
    VaultManager vaultManager;
    vaultManager.ensureVaultExists();

    // Set up models
    FileTreeModel fileTreeModel;
    fileTreeModel.setRootPath(vaultManager.vaultPath());

    ArticleModel articleModel;

    // Initialize geocode cache and photo ingestor
    QString cachePath = vaultManager.vaultPath() + "/../geocode_cache.db";
    GeocodeCache geocodeCache(cachePath);
    PhotoIngestor photoIngestor(&vaultManager, &geocodeCache);
    MessageIngestor messageIngestor(&vaultManager);

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Expose C++ objects to QML
    engine.rootContext()->setContextProperty("vaultManager", &vaultManager);
    engine.rootContext()->setContextProperty("fileTreeModel", &fileTreeModel);
    engine.rootContext()->setContextProperty("articleModel", &articleModel);
    engine.rootContext()->setContextProperty("photoIngestor", &photoIngestor);
    engine.rootContext()->setContextProperty("messageIngestor", &messageIngestor);

    // Load main QML
    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/Arkive/qml/Main.qml"_s);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
