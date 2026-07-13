#pragma once

#include <QString>

class CaptureDatabase;

class CurrentGameService
{
public:
    explicit CurrentGameService(CaptureDatabase* db);

    int currentGameId() const { return m_currentGameId; }
    bool currentGameAvailable() const { return m_currentGameHasCaptures; }
    bool lastUpdateChangedGameMetadata() const { return m_lastUpdateChangedGameMetadata; }

    bool syncToForegroundGame();
    bool update(const QString& gameName, const QString& executablePath);

private:
    static constexpr int kClearAfterMisses = 3;

    int runningCapturedGameFallback() const;

    CaptureDatabase* m_db;
    int m_currentGameId = -1;
    bool m_currentGameHasCaptures = false;
    bool m_lastUpdateChangedGameMetadata = false;
    int m_foregroundMisses = 0;
};
