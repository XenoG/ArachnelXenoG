#include "steam_source_plugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>

namespace arachnel::core {

namespace {

#if defined(Q_OS_WIN)
constexpr const char* kSteamCmdBinary = "steamcmd.exe";
#else
constexpr const char* kSteamCmdBinary = "steamcmd.sh";
#endif

QString steamCmdDir()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + QStringLiteral("/steamcmd");
}

} // namespace

SteamSourcePlugin::SteamSourcePlugin() = default;

QString SteamSourcePlugin::id() const
{
    return QStringLiteral("steam");
}

QString SteamSourcePlugin::name() const
{
    return QStringLiteral("Steam");
}

QString SteamSourcePlugin::description() const
{
    return QStringLiteral("Download games from Steam using SteamCMD");
}

QString SteamSourcePlugin::version() const
{
    return QStringLiteral("1.0.0");
}

QStringList SteamSourcePlugin::capabilities() const
{
    return {QStringLiteral("owns_download")};
}

QVector<CatalogEntry> SteamSourcePlugin::catalog() const
{
    if (m_catalogLoaded)
        return m_catalogCache;

    // The catalog is populated externally via the catalog feed system.
    // This plugin does not self-host a catalog — it relies on the configured
    // catalog URL in sources/settings to provide Steam game entries.
    m_catalogLoaded = true;
    return m_catalogCache;
}

QVector<CatalogEntry> SteamSourcePlugin::search(const QString& query) const
{
    QVector<CatalogEntry> results;
    const QString needle = query.trimmed().toLower();
    if (needle.isEmpty())
        return results;

    for (const CatalogEntry& entry : catalog()) {
        if (entry.title.toLower().contains(needle))
            results.append(entry);
    }
    return results;
}

std::optional<CatalogEntry> SteamSourcePlugin::entryById(const QString& entryId) const
{
    for (const CatalogEntry& entry : catalog()) {
        if (entry.id == entryId)
            return entry;
    }
    return std::nullopt;
}

InstallResult SteamSourcePlugin::installFromDownload(const InstallContext& ctx) const
{
    // owns_download plugins handle install via startOwnedDownload.
    // This is only called for non-owned flows (fallback).
    InstallResult result;
    result.success = true;
    result.installPath = ctx.downloadPath.isEmpty() ? ctx.targetPath : ctx.downloadPath;
    return result;
}

InstallAnalysis SteamSourcePlugin::analyzeDownload(const InstallContext& ctx) const
{
    Q_UNUSED(ctx)
    InstallAnalysis analysis;
    analysis.kind = InstallKind::PortableArchive;
    analysis.confidence = 100;
    analysis.detail = QStringLiteral("Steam depot");
    analysis.canInstall = true;
    return analysis;
}

InstallAnalysis SteamSourcePlugin::analyzeFileNames(const QStringList& fileNames) const
{
    Q_UNUSED(fileNames)
    InstallAnalysis analysis;
    analysis.kind = InstallKind::PortableArchive;
    analysis.confidence = 50;
    return analysis;
}

std::optional<QString> SteamSourcePlugin::detectUpdate(const LibraryGame& local,
                                                       const CatalogEntry& remote) const
{
    if (remote.version.isEmpty())
        return std::nullopt;
    if (local.version.isEmpty() || local.version < remote.version)
        return remote.version;
    return std::nullopt;
}

LaunchInfo SteamSourcePlugin::launchInfo(const LibraryGame& local) const
{
    LaunchInfo info;
    info.executable = local.executablePath;
    info.workingDirectory = local.installPath;
    return info;
}

void SteamSourcePlugin::resetCatalogCache()
{
    m_catalogCache.clear();
    m_catalogLoaded = false;
}

// --- Download via SteamCMD ---

QString SteamSourcePlugin::findSteamCmd() const
{
    // Check our managed copy first
    const QString managed = steamCmdDir() + QLatin1Char('/') + QLatin1String(kSteamCmdBinary);
    if (QFileInfo::exists(managed))
        return managed;

    // Check PATH
    const QString fromPath = QStandardPaths::findExecutable(
        QLatin1String(kSteamCmdBinary));
    if (!fromPath.isEmpty())
        return fromPath;

#if defined(Q_OS_WIN)
    // Common install locations on Windows
    const QStringList candidates = {
        QStringLiteral("C:/SteamCMD/steamcmd.exe"),
        QDir::homePath() + QStringLiteral("/steamcmd/steamcmd.exe"),
    };
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path))
            return path;
    }
#else
    // Common Linux locations
    const QStringList candidates = {
        QStringLiteral("/usr/games/steamcmd"),
        QStringLiteral("/usr/bin/steamcmd"),
        QDir::homePath() + QStringLiteral("/steamcmd/steamcmd.sh"),
    };
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path))
            return path;
    }
#endif

    return {};
}

QString SteamSourcePlugin::ensureSteamCmd() const
{
    QString path = findSteamCmd();
    if (!path.isEmpty())
        return path;

    // Auto-download SteamCMD to our data directory
    const QString dir = steamCmdDir();
    QDir().mkpath(dir);

#if defined(Q_OS_WIN)
    // Download steamcmd.zip and extract
    QProcess download;
    download.setWorkingDirectory(dir);
    download.start(QStringLiteral("powershell"), {
        QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
        QStringLiteral(
            "Invoke-WebRequest -Uri 'https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip' "
            "-OutFile 'steamcmd.zip'; "
            "Expand-Archive -Path 'steamcmd.zip' -DestinationPath '.' -Force; "
            "Remove-Item 'steamcmd.zip'")
    });
    download.waitForFinished(120000);
#else
    // Download steamcmd_linux.tar.gz and extract
    QProcess download;
    download.setWorkingDirectory(dir);
    download.start(QStringLiteral("bash"), {
        QStringLiteral("-c"),
        QStringLiteral(
            "curl -sqL 'https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz' "
            "| tar xzf - ")
    });
    download.waitForFinished(120000);
#endif

    path = dir + QLatin1Char('/') + QLatin1String(kSteamCmdBinary);
    if (QFileInfo::exists(path)) {
#if !defined(Q_OS_WIN)
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
        return path;
    }

    return {};
}

bool SteamSourcePlugin::runSteamCmd(
    const QStringList& args, const QString& installDir,
    const std::function<void(const OwnedDownloadProgress&)>& onProgress,
    const QString& jobId) const
{
    const QString steamcmd = ensureSteamCmd();
    if (steamcmd.isEmpty())
        return false;

    QDir().mkpath(installDir);

    QProcess process;
    process.setWorkingDirectory(QFileInfo(steamcmd).absolutePath());
    process.setProcessChannelMode(QProcess::MergedChannels);

    process.start(steamcmd, args);
    if (!process.waitForStarted(30000))
        return false;

    // Parse output for progress updates
    static const QRegularExpression progressRx(
        QStringLiteral(R"(Update state \(0x\d+\) (\w+), progress: (\d+\.\d+) \((\d+) / (\d+)\))"));

    while (process.state() != QProcess::NotRunning) {
        // Check cancellation
        {
            QMutexLocker locker(&m_cancelMutex);
            if (m_cancelledJobs.contains(jobId)) {
                process.kill();
                process.waitForFinished(5000);
                return false;
            }
            // Check pause - just sleep loop
            while (m_pausedJobs.contains(jobId) && !m_cancelledJobs.contains(jobId)) {
                locker.unlock();
                QThread::msleep(500);
                locker.relock();
            }
            if (m_cancelledJobs.contains(jobId)) {
                process.kill();
                process.waitForFinished(5000);
                return false;
            }
        }

        process.waitForReadyRead(1000);
        const QByteArray output = process.readAll();
        if (output.isEmpty())
            continue;

        const QString text = QString::fromUtf8(output);
        const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        for (const QString& line : lines) {
            const QRegularExpressionMatch match = progressRx.match(line);
            if (!match.hasMatch())
                continue;

            const QString state = match.captured(1);
            const double pct = match.captured(2).toDouble();
            const qint64 downloaded = match.captured(3).toLongLong();
            const qint64 total = match.captured(4).toLongLong();

            OwnedDownloadProgress progress;
            progress.percent = static_cast<int>(pct);
            progress.bytesDownloaded = downloaded;
            progress.totalBytes = total;
            progress.status = QStringLiteral("downloading");
            progress.detail = state;

            if (onProgress)
                onProgress(progress);
        }
    }

    return process.exitCode() == 0;
}

InstallResult SteamSourcePlugin::startOwnedDownload(
    const InstallContext& ctx,
    const std::function<void(const OwnedDownloadProgress&)>& onProgress) const
{
    InstallResult result;

    const QString appId = ctx.steamAppId;
    if (appId.isEmpty()) {
        result.success = false;
        result.error = QStringLiteral("No Steam App ID specified");
        return result;
    }

    // Clear cancellation state for this job
    {
        QMutexLocker locker(&m_cancelMutex);
        m_cancelledJobs.remove(ctx.jobId);
        m_pausedJobs.remove(ctx.jobId);
    }

    const QString installDir = ctx.targetPath.isEmpty()
                                   ? ctx.downloadsPath + QLatin1Char('/') + appId
                                   : ctx.targetPath;

    if (onProgress) {
        OwnedDownloadProgress starting;
        starting.percent = 0;
        starting.status = QStringLiteral("downloading");
        starting.detail = QStringLiteral("Starting SteamCMD…");
        onProgress(starting);
    }

    // Build SteamCMD arguments
    // Anonymous login for free-to-play / demo apps
    QStringList args = {
        QStringLiteral("+@ShutdownOnFailedCommand"), QStringLiteral("1"),
        QStringLiteral("+@NoPromptForPassword"), QStringLiteral("1"),
        QStringLiteral("+login"), QStringLiteral("anonymous"),
        QStringLiteral("+force_install_dir"), QDir::toNativeSeparators(installDir),
        QStringLiteral("+app_update"), appId,
        QStringLiteral("+quit")
    };

    const bool ok = runSteamCmd(args, installDir, onProgress, ctx.jobId);

    if (!ok) {
        QMutexLocker locker(&m_cancelMutex);
        if (m_cancelledJobs.contains(ctx.jobId)) {
            m_cancelledJobs.remove(ctx.jobId);
            result.success = false;
            result.error = QStringLiteral("Download cancelled");
            return result;
        }
    }

    if (!ok) {
        result.success = false;
        result.error = QStringLiteral("SteamCMD download failed for app %1").arg(appId);
        return result;
    }

    result.success = true;
    result.installPath = installDir;
    return result;
}

void SteamSourcePlugin::cancelOwnedDownload(const QString& jobId) const
{
    QMutexLocker locker(&m_cancelMutex);
    m_cancelledJobs.insert(jobId);
    m_pausedJobs.remove(jobId);
}

void SteamSourcePlugin::setOwnedDownloadPaused(const QString& jobId, bool paused) const
{
    QMutexLocker locker(&m_cancelMutex);
    if (paused)
        m_pausedJobs.insert(jobId);
    else
        m_pausedJobs.remove(jobId);
}

} // namespace arachnel::core
