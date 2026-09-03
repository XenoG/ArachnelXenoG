#include "job_orchestrator.h"

#include "i18n.h"
#include "job_status.h"

#include <QDateTime>

namespace arachnel::core {
namespace {
QString isoNow() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODate); }
} // namespace

void JobOrchestrator::reportPluginProgress(const QString& jobId,
                                           const OwnedDownloadProgress& progress)
{
    const int row = m_jobs->indexOfJob(jobId);
    if (row < 0)
        return;
    JobEntry job = jobFromModelRow(row);
    if (isJobTerminal(job.status))
        return;

    const bool wasPaused = isJobPaused(job.status);
    if (!progress.status.isEmpty()) {
        // In-flight CDN chunks keep emitting "downloading" after user pause - don't unstick.
        if (!(wasPaused && progress.status == QStringLiteral("downloading")))
            job.status = progress.status;
    } else if (job.status == QStringLiteral("starting")) {
        job.status = QStringLiteral("downloading");
    }

    const bool paused = isJobPaused(job.status);
    const int previousProgress = job.progress;
    const qint64 previousDownloaded = job.bytesDownloaded;
    int nextProgress = qBound(0, progress.percent, 100);
    qint64 downloaded = progress.bytesDownloaded;
    qint64 total = progress.totalBytes;
    if (total <= 0) {
        // Plugin sent an explicit "unknown total" with bytes - drop stale cache
        // (old fake totals like "19.5 GB" from percent invention).
        if (downloaded > 0)
            m_pluginEstimatedTotal.remove(jobId);
        else {
            const auto estimated = m_pluginEstimatedTotal.constFind(jobId);
            if (estimated != m_pluginEstimatedTotal.cend())
                total = estimated.value();
        }
    }
    if (downloaded <= 0 && total > 0 && progress.percent > 0)
        downloaded = total * progress.percent / 100;
    // Drop only *estimated* totals that disagree badly with bytes. Never discard
    // an explicit plugin total (Steam ACF BytesToDownload is trusted).
    if (progress.totalBytes <= 0 && total > 0 && downloaded > 0 && total > downloaded * 2)
        total = 0;

    if (total > 0 && downloaded > 0
        && (progress.percent <= 0 || progress.totalBytes <= 0)) {
        // Only invent percent from bytes when the plugin did not provide one.
        // Never clamp to 99 or override a real plugin percent (steamidra).
        const int bytePercent =
            static_cast<int>(qMin<qint64>(99, downloaded * 100 / total));
        if (bytePercent > nextProgress)
            nextProgress = bytePercent;
    }

    // Status-only ticks often send percent=0 - never rewind the bar mid-download.
    if (nextProgress < previousProgress && previousProgress < 100) {
        const bool statusOnly = progress.percent <= 0 && downloaded <= previousDownloaded;
        const bool noRealRegression = downloaded + 64 * 1024 >= previousDownloaded;
        if (statusOnly || noRealRegression)
            nextProgress = previousProgress;
    }
    job.progress = nextProgress;
    job.bytesDownloaded = downloaded;
    job.totalBytes = total;

    if (total > 0)
        m_pluginEstimatedTotal.insert(jobId, total);

    int rate = paused ? 0 : progress.downloadRateBps;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (paused) {
        m_pluginSpeed.remove(jobId);
    } else if (downloaded > 0) {
        const auto it = m_pluginSpeed.constFind(jobId);
        if (it != m_pluginSpeed.cend() && nowMs > it->ms) {
            const qint64 dBytes = downloaded - it->bytes;
            const qint64 dt = nowMs - it->ms;
            // Allow large folder-size jumps (depot downloads often land in chunks).
            if (rate <= 0 && dBytes > 8 * 1024 && dt >= 400)
                rate = static_cast<int>(dBytes * 1000 / dt);
            if (rate > 0 && rate < 8 * 1024)
                rate = 0;
            if (rate <= 0 && dBytes == 0 && it->rate >= 8 * 1024 && dt < 5000)
                rate = it->rate;
        }
        SpeedSample sample;
        sample.bytes = downloaded;
        sample.ms = nowMs;
        sample.rate = rate > 0 ? rate : m_pluginSpeed.value(jobId).rate;
        m_pluginSpeed.insert(jobId, sample);
    }

    if (paused) {
        if (downloaded > 0 || total > 0)
            job.detail = buildTransferDetail(downloaded, total, 0);
        else
            job.detail = QStringLiteral("Paused");
    } else if (downloaded > 0 || total > 0) {
        job.detail = buildTransferDetail(downloaded, total, rate);
    } else if (!progress.detail.isEmpty()) {
        job.detail = progress.detail;
    }

    updateJobInModel(job);
    persistJob(job);
}

QString JobOrchestrator::formatBytes(qint64 bytes) const
{
    static const QStringList units{QStringLiteral("B"), QStringLiteral("KB"), QStringLiteral("MB"),
                                   QStringLiteral("GB"), QStringLiteral("TB")};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1) {
        value /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', unit == 0 ? 0 : 1), units.at(unit));
}

QString JobOrchestrator::formatSpeed(int bytesPerSec) const
{
    // Match plugin: hide crumb rates (EMA decay used to show "1 B/s").
    if (bytesPerSec < 8 * 1024)
        return QStringLiteral("-");
    return QStringLiteral("%1/s").arg(formatBytes(bytesPerSec));
}

QString JobOrchestrator::formatEta(qint64 remainingBytes, int bytesPerSec) const
{
    if (bytesPerSec <= 0 || remainingBytes <= 0)
        return QStringLiteral("-");
    const qint64 seconds = remainingBytes / bytesPerSec;
    if (seconds < 60)
        return QStringLiteral("%1 s").arg(seconds);
    if (seconds < 3600)
        return QStringLiteral("%1 min").arg(seconds / 60);
    return QStringLiteral("%1 h").arg(seconds / 3600);
}

QString JobOrchestrator::buildHttpDetail(qint64 downloaded, qint64 total) const
{
    return buildTransferDetail(downloaded, total, 0);
}

} // namespace arachnel::core
