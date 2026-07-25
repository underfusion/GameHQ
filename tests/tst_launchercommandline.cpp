#include "launcher/LauncherCommandLine.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <vector>

#include <windows.h>

// The package launcher hands whatever it was given straight to app\GameHQ.exe.
// Its old fixed buffers could truncate or overflow, and a substring search
// decided whether a launch was a post-update one, so both the safety and the
// meaning of a user's arguments depended on their length and spelling.
//
// The unit tests below pin the parsing rules; the last one proves the whole
// thing end to end by starting a real child process exactly the way the
// launcher does and comparing the arguments that child received.
//
// Nothing here uses raw string literals: moc mis-lexes a backslash before a
// quote inside one and then finds no classes at all in the file.
class TestLauncherCommandLine : public QObject
{
    Q_OBJECT

private:
    static std::wstring wide(const QString& s) { return s.toStdWString(); }

    static QStringList parsed(const QString& commandLine)
    {
        QStringList out;
        for (const std::wstring& argument : launcher::parseArguments(wide(commandLine)))
            out << QString::fromStdWString(argument);
        return out;
    }

    // Non-ASCII game folders are ordinary, and the launcher works in wide
    // characters end to end, so nothing here may depend on the code page.
    static QString unicodePath()
    {
        return QString::fromUtf8(
            "D:\\Gry\\Wied\xC5\xBAmin 3 \xE2\x80\x94 \xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\\save.png");
    }

private slots:
    void skipsTheProgramNameWhicheverWayItIsWritten()
    {
        // Unquoted: the name ends at the first separator.
        QCOMPARE(QString::fromStdWString(launcher::argumentTail(L"GameHQ.exe --flag")),
                 QStringLiteral(" --flag"));
        // Quoted: it ends at the closing quote, spaces and all.
        QCOMPARE(QString::fromStdWString(
                     launcher::argumentTail(L"\"C:\\Program Files\\GameHQ\\GameHQ.exe\" --flag")),
                 QStringLiteral(" --flag"));
        // No arguments at all is not a special case.
        QCOMPARE(QString::fromStdWString(launcher::argumentTail(L"\"C:\\GameHQ\\GameHQ.exe\"")),
                 QString());
    }

    void parsesQuotesSpacesAndTrailingBackslashes()
    {
        // The classic trap: one backslash before the closing quote escapes it,
        // so the argument never ends and " --open" is swallowed into it. That
        // is what Windows really does, and forwarding the tail verbatim is what
        // keeps the launcher and its child agreeing about it.
        QCOMPARE(parsed(QStringLiteral("hq.exe \"C:\\Games\\My Game\\\" --open")),
                 QStringList({ QStringLiteral("C:\\Games\\My Game\" --open") }));
        // 2n backslashes before a quote collapse to n and let the quote close
        // the run, so the trailing separator survives as itself.
        QCOMPARE(parsed(QStringLiteral("hq.exe \"C:\\Games\\My Game\\\\\" --open")),
                 QStringList({ QStringLiteral("C:\\Games\\My Game\\"), QStringLiteral("--open") }));
        // Backslashes away from a quote are literal.
        QCOMPARE(parsed(QStringLiteral("hq.exe C:\\a\\\\b\\c")),
                 QStringList({ QStringLiteral("C:\\a\\\\b\\c") }));
        // "" inside a quoted run is one literal quote and also ends that run,
        // so what follows is unquoted again.
        QCOMPARE(parsed(QStringLiteral("hq.exe \"say \"\"hi\"\" now\" plain")),
                 QStringList({ QStringLiteral("say \"hi"), QStringLiteral("now plain") }));
        // An explicitly empty argument stays an argument.
        QCOMPARE(parsed(QStringLiteral("hq.exe \"\" tail")),
                 QStringList({ QString(), QStringLiteral("tail") }));
        // Tabs separate exactly like spaces, and runs of them collapse.
        QCOMPARE(parsed(QStringLiteral("hq.exe \t a  \t b ")),
                 QStringList({ QStringLiteral("a"), QStringLiteral("b") }));
    }

    void parsesUnicodeArgumentsUnchanged()
    {
        const QString path = unicodePath();
        QCOMPARE(parsed(QStringLiteral("hq.exe \"%1\"").arg(path)), QStringList({ path }));
    }

    void recognisesThePostUpdateSwitchOnlyAsAWholeArgument()
    {
        QVERIFY(launcher::hasSwitch(L"\"C:\\GameHQ\\GameHQ.exe\" --post-update", L"--post-update"));
        QVERIFY(launcher::hasSwitch(L"hq.exe --other --post-update", L"--post-update"));
        // The switch text inside a value is not the switch. This is what the
        // old substring search got wrong: any such argument silently suppressed
        // the "update in progress" guard.
        QVERIFY(!launcher::hasSwitch(L"hq.exe \"C:\\shots\\--post-update.png\"", L"--post-update"));
        QVERIFY(!launcher::hasSwitch(L"hq.exe --post-updater", L"--post-update"));
        QVERIFY(!launcher::hasSwitch(L"hq.exe", L"--post-update"));
        // Not even when it is the program name itself.
        QVERIFY(!launcher::hasSwitch(L"--post-update", L"--post-update"));
    }

    void refusesACommandLineWindowsCannotCarry()
    {
        const std::wstring exe = L"C:\\GameHQ\\app\\GameHQ.exe";
        // Just fits: the exe, its two quotes, the tail and a terminating null.
        const std::size_t fits = launcher::kMaxCommandLineChars - exe.size() - 3;
        QVERIFY(!launcher::buildChildCommandLine(exe, std::wstring(fits, L'a')).empty());
        QVERIFY(launcher::buildChildCommandLine(exe, std::wstring(fits + 1, L'a')).empty());
    }

    void quotesOnlyTheChildExecutable()
    {
        QCOMPARE(QString::fromStdWString(launcher::buildChildCommandLine(
                     L"C:\\Program Files\\GameHQ\\app\\GameHQ.exe", L" --flag \"a b\\\"")),
                 QStringLiteral("\"C:\\Program Files\\GameHQ\\app\\GameHQ.exe\" --flag \"a b\\\""));
    }

    // End to end: build the child command line the way the launcher does, start
    // a real process with it, and check Windows handed that child exactly the
    // arguments the user typed.
    void launchesTheChildWithIdenticalArguments()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString echoPath = QStringLiteral(LAUNCHER_ARGECHO_FIXTURE);
        QVERIFY(QFile::exists(echoPath));

        const QStringList tails = {
            QString(),
            QStringLiteral(" --post-update"),
            QStringLiteral(" \"C:\\Games\\My Game\\\" --open"),
            QStringLiteral(" \"C:\\Games\\My Game\\\\\" --open"),
            QStringLiteral(" \"say \"\"hi\"\" now\" plain"),
            QStringLiteral(" \"%1\"").arg(unicodePath()),
            QStringLiteral(" \"C:\\shots\\--post-update.png\""),
        };

        for (int index = 0; index < tails.size(); ++index) {
            const QString& tail = tails.at(index);
            const QString outPath = tempDir.filePath(QStringLiteral("argv-%1.txt").arg(index));
            QVERIFY(qputenv("GAMEHQ_ARGECHO_OUT", outPath.toUtf8()));

            const std::wstring exe = wide(QDir::toNativeSeparators(echoPath));
            const std::wstring commandLine = launcher::buildChildCommandLine(exe, wide(tail));
            QVERIFY(!commandLine.empty());
            std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
            mutableCommandLine.push_back(L'\0');

            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            QVERIFY2(CreateProcessW(exe.c_str(), mutableCommandLine.data(), nullptr, nullptr,
                                    FALSE, 0, nullptr, nullptr, &si, &pi),
                     qPrintable(QStringLiteral("CreateProcessW failed for tail: ") + tail));
            QCOMPARE(WaitForSingleObject(pi.hProcess, 30000), DWORD(WAIT_OBJECT_0));
            DWORD exitCode = 1;
            QVERIFY(GetExitCodeProcess(pi.hProcess, &exitCode));
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            QCOMPARE(exitCode, DWORD(0));

            QFile received(outPath);
            QVERIFY(received.open(QIODevice::ReadOnly));
            QStringList actual = QString::fromUtf8(received.readAll())
                                     .split(QLatin1Char('\n'));
            received.close();
            // The fixture ends every line, so the split leaves one empty tail
            // entry. argv[0] is the executable the launcher chose, not part of
            // the forwarded arguments.
            QVERIFY(!actual.isEmpty() && actual.constLast().isEmpty());
            actual.removeLast();
            QVERIFY(!actual.isEmpty());
            actual.removeFirst();

            // What the child got must match what the same command line means to
            // the launcher's own parser: the pass-through changed nothing.
            QCOMPARE(actual, parsed(QStringLiteral("hq.exe") + tail));
        }
    }
};

QTEST_GUILESS_MAIN(TestLauncherCommandLine)
#include "tst_launchercommandline.moc"
