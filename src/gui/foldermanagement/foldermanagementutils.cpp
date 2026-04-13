#include "foldermanagementutils.h"

#include "common/asserts.h"

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace OCC {


bool FolderManagementUtils::prepareFolder(const QString &folder)
{
    if (!QFileInfo::exists(folder)) {
        if (!OC_ENSURE(QDir().mkpath(folder))) {
            return false;
        }
#ifdef Q_OS_WIN
        // First create a Desktop.ini so that the folder and favorite link show our application's icon.
        const QFileInfo desktopIniPath{QStringLiteral("%1/Desktop.ini").arg(path)};
        {
            const QString updateIconKey = QStringLiteral("%1/UpdateIcon").arg(Theme::instance()->appName());
            QSettings desktopIni(desktopIniPath.absoluteFilePath(), QSettings::IniFormat);
            if (desktopIni.value(updateIconKey, true).toBool()) {
                qCInfo(lcFolder) << "Creating" << desktopIni.fileName() << "to set a folder icon in Explorer.";
                desktopIni.setValue(QStringLiteral(".ShellClassInfo/IconResource"), QDir::toNativeSeparators(qApp->applicationFilePath()));
                desktopIni.setValue(updateIconKey, true);
            } else {
                qCInfo(lcFolder) << "Skip icon update for" << desktopIni.fileName() << "," << updateIconKey << "is disabled";
            }
        }

        const QString longFolderPath = FileSystem::longWinPath(path);
        const QString longDesktopIniPath = FileSystem::longWinPath(desktopIniPath.absoluteFilePath());
        // Set the folder as system and Desktop.ini as hidden+system for explorer to pick it.
        // https://msdn.microsoft.com/en-us/library/windows/desktop/cc144102
        const DWORD folderAttrs = GetFileAttributesW(reinterpret_cast<const wchar_t *>(longFolderPath.utf16()));
        if (!SetFileAttributesW(reinterpret_cast<const wchar_t *>(longFolderPath.utf16()), folderAttrs | FILE_ATTRIBUTE_SYSTEM)) {
            const auto error = GetLastError();
            qCWarning(lcFolder) << "SetFileAttributesW failed on" << longFolderPath << Utility::formatWinError(error);
        }
        if (!SetFileAttributesW(reinterpret_cast<const wchar_t *>(longDesktopIniPath.utf16()), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
            const auto error = GetLastError();
            qCWarning(lcFolder) << "SetFileAttributesW failed on" << longDesktopIniPath << Utility::formatWinError(error);
        }
#else
        QFile::Permissions perm = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
        QFile file(folder);
        file.setPermissions(perm);
#endif
    }
    return true;
}

qsizetype FolderManagementUtils::numberOfSyncJournals(const QString &path)
{
    return QDir(path).entryList({QStringLiteral(".sync_*.db"), QStringLiteral("._sync_*.db")}, QDir::Hidden | QDir::Files).size();
}

QString FolderManagementUtils::checkPathLength(const QString &path)
{
#ifdef Q_OS_WIN
    if (path.size() > MAX_PATH) {
        if (!FileSystem::longPathsEnabledOnWindows()) {
            return tr("The path '%1' is too long. Please enable long paths in the Windows settings or choose a different folder.").arg(path);
        }
    }
#else
    Q_UNUSED(path)
#endif
    return {};
}

} // OCC
