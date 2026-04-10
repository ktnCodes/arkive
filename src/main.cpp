#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "core/VaultManager.h"
#include "models/FileTreeModel.h"
#include "models/ArticleModel.h"

int main(int argc, char *argv[])
{
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

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Expose C++ objects to QML
    engine.rootContext()->setContextProperty("vaultManager", &vaultManager);
    engine.rootContext()->setContextProperty("fileTreeModel", &fileTreeModel);
    engine.rootContext()->setContextProperty("articleModel", &articleModel);

    // Load main QML
    const QUrl url(u"qrc:/Arkive/qml/Main.qml"_qs);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
