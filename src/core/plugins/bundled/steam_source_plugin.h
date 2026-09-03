#pragma once

#include "plugin_interface.h"

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QString>

namespace arachnel::core {

/**
 * Bundled Steam source plugin.
 *
 * Downloads games via SteamCMD (anonymous or authenticated).
 * Provides catalog from a remote JSON endpoint and owns the full
 * download/install lifecycle (capability: "owns_download").
 *
 * This plugin ships inside the app binary as the default source.
 * An external .arach plugin with the same id ("steam") will override it
 * if a newer version is installed via the plugin system.
 */
class SteamSourcePlugin : public ISourcePlugin
{
public:
    SteamSourcePlugin();
    ~SteamSourcePlugin() override = default;

    // ISourcePlugin interface
    QString id() const override;
    QString name() const override;
    QString description() const override;
    QString version() const override;
    QStringList capabilities() const override;

    QVector<CatalogEntry> catalog() const override;
    QVector<CatalogEntry> search(const QString& query) const override;
    std::optional<CatalogEntry> entryById(const QString& entryId) const override;

    InstallResult installFromDownload(const InstallContext& ctx) const override;
    InstallAnalysis analyzeDownload(const InstallContext& ctx) const override;
    InstallAnalysis analyzeFileNames(const QStringList& fileNames) const override;
    std::optional<QString> detectUpdate(const LibraryGame& local,
                                        const CatalogEntry& remote) const override;
    LaunchInfo launchInfo(const LibraryGame& local) const override;

    InstallResult startOwnedDownload(
        const InstallContext& ctx,
        const std::function<void(const OwnedDownloadProgress&)>& onProgress) const override;
    void cancelOwnedDownload(const QString& jobId) const override;
    void setOwnedDownloadPaused(const QString& jobId, bool paused) const override;

    void resetCatalogCache() override;

private:
    QString findSteamCmd() const;
    QString ensureSteamCmd() const;
    bool runSteamCmd(const QStringList& args, const QString& installDir,
                     const std::function<void(const OwnedDownloadProgress&)>& onProgress,
                     const QString& jobId) const;

    mutable QVector<CatalogEntry> m_catalogCache;
    mutable bool m_catalogLoaded = false;
    mutable QMutex m_cancelMutex;
    mutable QSet<QString> m_cancelledJobs;
    mutable QSet<QString> m_pausedJobs;
};

} // namespace arachnel::core
