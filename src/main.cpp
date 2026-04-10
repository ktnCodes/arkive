#include <QGuiApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTextStream>

#include <cstdio>

#include "core/VaultManager.h"
#include "core/GeocodeCache.h"
#include "core/MessageIngestor.h"
#include "core/PhotoIngestor.h"
#include "core/SnapchatIngestor.h"
#include "models/FileTreeModel.h"
#include "models/ArticleModel.h"

namespace {
QMutex g_logMutex;
QString g_logFilePath;

QString logLevelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("INFO");
}

void arkiveMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString line = QStringLiteral("%1 [%2] %3")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), logLevelName(type), message);

    if (context.category && *context.category) {
        line += QStringLiteral(" {category=%1}").arg(QString::fromUtf8(context.category));
    }
    if (context.file && *context.file) {
        line += QStringLiteral(" {source=%1:%2}")
            .arg(QFileInfo(QString::fromUtf8(context.file)).fileName())
            .arg(context.line);
    }

    {
        QMutexLocker locker(&g_logMutex);
        if (!g_logFilePath.isEmpty()) {
            QFile file(g_logFilePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&file);
                out << line << '\n';
                out.flush();
            }
        }
    }

    std::fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
    std::fflush(stderr);

    if (type == QtFatalMsg) {
        std::abort();
    }
}

QString initializeLogging()
{
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appDataPath.isEmpty()) {
        return {};
    }

    QDir logDir(appDataPath);
    if (!logDir.mkpath(QStringLiteral("logs"))) {
        return {};
    }

    g_logFilePath = logDir.filePath(QStringLiteral("logs/arkive.log"));
    qInstallMessageHandler(arkiveMessageHandler);
    return g_logFilePath;
}
}

int main(int argc, char *argv[])
{
    // Use Basic style for full customization support
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName("Arkive");
    app.setOrganizationName("Arkive");
    app.setApplicationVersion("0.1.0");

    const QString logFilePath = initializeLogging();
    qInfo() << "Arkive starting. Version:" << app.applicationVersion();
    if (!logFilePath.isEmpty()) {
        qInfo() << "Log file:" << logFilePath;
    }

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
    SnapchatIngestor snapchatIngestor(&vaultManager);

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Expose C++ objects to QML
    engine.rootContext()->setContextProperty("vaultManager", &vaultManager);
    engine.rootContext()->setContextProperty("fileTreeModel", &fileTreeModel);
    engine.rootContext()->setContextProperty("articleModel", &articleModel);
    engine.rootContext()->setContextProperty("photoIngestor", &photoIngestor);
    engine.rootContext()->setContextProperty("messageIngestor", &messageIngestor);
    engine.rootContext()->setContextProperty("snapchatIngestor", &snapchatIngestor);

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
