@{
    SchemaVersion = 1

    ProductId = 'underfusion.gamehq'
    ProductName = 'GameHQ'
    Publisher = 'underfusion'
    ExecutableName = 'GameHQ.exe'

    # This value is permanent. Changing it would create a second Windows
    # installation instead of upgrading the existing per-user installation.
    InnoAppId = '{1CB27009-5809-408F-8510-1C4F19605565}'
    DefaultInstallSubdirectory = 'Programs\GameHQ'

    ProductRegistryKey = 'Software\underfusion\GameHQ'
    InstallLocationValue = 'InstallLocation'
    InstalledVersionValue = 'Version'
    AppPathRegistryKey = 'Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe'
    RunRegistryKey = 'Software\Microsoft\Windows\CurrentVersion\Run'
    RunRegistryValue = 'GameHQ'

    ArtifactPatterns = @{
        Setup = 'GameHQ-{0}-win64-setup.exe'
        Portable = 'GameHQ-{0}-win64-portable.zip'
        Update = 'GameHQ-{0}-win64-update.zip'
        UpdateChecksum = 'GameHQ-{0}-win64-update.zip.sha256'
        ReleaseManifest = 'gamehq-release.json'
        ReleaseSignature = 'gamehq-release.sig'
    }
}
