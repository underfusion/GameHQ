#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

// SQLite metadata store (gamehq.db). Schema versioned via PRAGMA user_version;
// see docs/database.md. Hard rule: favorites are never auto-deleted.

struct CaptureRecord
{
    int id = -1;
    QString filePath;
    QString type;          // "screenshot" | "video"
    int gameId = -1;
    QString gameName;
    QString createdAt;     // ISO 8601
    bool isFavorite = false;
    QString thumbnailPath;
    QString source;
};

struct GameEntry
{
    int id = -1;
    QString name;
    QString iconPath;
    QString executablePath;
};

struct BindingRow
{
    QString deviceType;    // "keyboard" | "controller"
    QString inputCode;     // "Share", "Ctrl+Shift+S", …
    QString action;        // "screenshot", "save_replay", "overlay_toggle", …
    QString pressType;     // "tap" | "hold" | "combo"
    int holdMs = 0;        // 0 = not applicable
};

// A user override for one action/slot. Built-in defaults live in ActionCatalog
// (and, until the resolver lands, HotkeyManager); this table only ever holds
// explicit changes, so an empty table means "every action uses its default".
struct BindingOverrideRow
{
    QString deviceGroup;    // "keyboard" | "controller"
    QString deviceProfile;  // device family/fingerprint scope; empty = applies to all devices in the group
    QString actionId;       // ActionCatalog id, e.g. "global.screenshot"
    int slot = 1;           // 1 = primary, 2 = secondary
    QString triggerCode;    // canonical control id or key chord; empty when unbound
    QString activation = QStringLiteral("press"); // "press" | "tap" | "hold" | "double_tap"
    int holdMs = 0;         // 0 = not applicable
    bool unbound = false;   // explicit "no trigger" override
};

class CaptureDatabase : public QObject
{
    Q_OBJECT
public:
    explicit CaptureDatabase(QString filePath, QObject* parent = nullptr);
    ~CaptureDatabase() override;

    bool open();      // opens + runs pending migrations
    int schemaVersion() const;

    // Captures. category: all | recent | favorites | screenshots | clips.
    // gameId >= 0 filters to one game. Returns newest first.
    QVector<CaptureRecord> listCaptures(const QString& category, int gameId = -1) const;
    bool hasCapture(const QString& filePath) const;
    bool hasCapturesForGame(int gameId) const;
    // Inserts if new; resolves/creates the game row. Returns new id or -1.
    int insertCapture(const QString& filePath, const QString& type,
                      const QString& gameName, const QString& createdAt,
                      const QString& source, const QString& executablePath = QString());
    bool setFavorite(int captureId, bool favorite);
    bool setThumbnail(int captureId, const QString& thumbnailPath);
    QString thumbnailForCapture(const QString& filePath) const;
    bool setThumbnailForCapture(const QString& filePath, const QString& thumbnailPath);
    bool deleteCapture(int captureId);   // removes the DB row (file handled by caller)

    // Games (only those that have captures), alphabetical.
    QVector<GameEntry> listGames() const;
    bool rememberGameExecutable(const QString& displayName, const QString& executablePath);

    // Input bindings (docs/controller-input.md). Editing UI is 1.0 scope;
    // for now the defaults are seeded once and read back by InputEngine.
    bool seedDefaultBindings();          // inserts defaults only if table empty
    QVector<BindingRow> listBindings() const;

    // Binding overrides (schema v2). Defaults stay in code; these rows are the
    // only user-facing changes, keyed by (device group, device profile, action, slot).
    QVector<BindingOverrideRow> listBindingOverrides() const;
    bool upsertBindingOverride(const BindingOverrideRow& row);
    bool clearBindingOverride(const QString& deviceGroup, const QString& deviceProfile,
                               const QString& actionId, int slot);
    bool clearBindingOverridesForGroup(const QString& deviceGroup);
    bool clearBindingOverridesForProfile(const QString& deviceGroup, const QString& deviceProfile);
    bool clearAllBindingOverrides();

    // Watched folders.
    QStringList watchedFolders() const;
    bool addWatchedFolder(const QString& path, const QString& source);
    bool removeWatchedFolder(const QString& path);

private:
    bool migrate();
    bool applyV1();
    bool applyV2();
    bool applyV3();
    bool ensureGameMetadataColumns();
    int findOrCreateGame(const QString& displayName, const QString& executablePath = QString());
    void updateGameExecutable(int gameId, const QString& executablePath);

    QString m_filePath;
    QSqlDatabase m_db;
};
