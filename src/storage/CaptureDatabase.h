#pragma once
#include <QHash>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
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

// One captures row as seen by a bulk scan: enough to answer "is this file
// already registered, and does it still need a thumbnail?" without a query per file.
struct CaptureIndexEntry
{
    QString thumbnailPath;   // empty when unset or when the row is tombstoned
    bool deleted = false;    // deleted_at IS NOT NULL
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
    QString activation = QStringLiteral("press"); // "press" | "tap" | "hold" (v4 canonical)
    int holdMs = 0;         // 0 = not applicable, or "use the configured default" for a hold
    bool unbound = false;   // explicit "no trigger" override
    // Appended last so every existing brace-initialization keeps its meaning.
    int tapCount = 1;       // taps a "tap" activation needs, 1-3
};

struct ControllerLayoutRow
{
    QString logicalId;
    QString layoutSignature;
    QStringList buttonLabels;
    bool needsReconfirmation = false;
};

class CaptureDatabase : public QObject
{
    Q_OBJECT
public:
    explicit CaptureDatabase(QString filePath, QObject* parent = nullptr);
    ~CaptureDatabase() override;

    // Highest schema this build understands. A database stamped higher was
    // written by a newer GameHQ and must never be modified by this one.
    static constexpr int kCurrentSchemaVersion = 6;

    bool open();      // opens + runs pending migrations
    int schemaVersion() const;

    // Captures. category: all | recent | favorites | screenshots | clips.
    // gameId >= 0 filters to one game. Returns newest first.
    QVector<CaptureRecord> listCaptures(const QString& category, int gameId = -1) const;
    bool hasCapture(const QString& filePath) const;
    // Whole-table snapshot keyed by stored path; see CaptureQueries::captureIndex.
    QHash<QString, CaptureIndexEntry> captureIndex() const;
    // Normalizes filePath the same way the captures table stores it, so callers
    // holding a captureIndex() can look up their on-disk paths.
    static QString storedPathKey(const QString& filePath);
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
    bool upsertBindingOverridesAtomically(const QVector<BindingOverrideRow>& rows);
    bool clearBindingOverride(const QString& deviceGroup, const QString& deviceProfile,
                               const QString& actionId, int slot);
    bool clearBindingOverridesForGroup(const QString& deviceGroup);
    bool clearBindingOverridesForProfile(const QString& deviceGroup, const QString& deviceProfile);
    bool clearAllBindingOverrides();
    ControllerLayoutRow controllerLayout(const QString& logicalId) const;
    bool upsertControllerLayout(const ControllerLayoutRow& row);
    bool confirmControllerLayout(const QString& logicalId);

    // Watched folders.
    QStringList watchedFolders() const;
    bool addWatchedFolder(const QString& path, const QString& source);
    bool removeWatchedFolder(const QString& path);

private:
    bool migrate();
    bool applyV1();
    bool applyV2();
    bool applyV3();
    bool applyV4();
    bool applyV5();
    bool applyV6();
    bool ensureGameMetadataColumns();
    bool repairsV1Done() const;
    bool markRepairsV1Done();
    bool refreshIconsForExtractorFormat();
    int findOrCreateGame(const QString& displayName, const QString& executablePath = QString());
    void updateGameExecutable(int gameId, const QString& executablePath);

    QString m_filePath;
    QSqlDatabase m_db;
};
