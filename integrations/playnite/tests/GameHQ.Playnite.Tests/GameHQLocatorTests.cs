using System;
using System.IO;
using Xunit;

namespace GameHQ.Playnite.Tests
{
    public class GameHQLocatorTests : IDisposable
    {
        private readonly string _root;

        public GameHQLocatorTests()
        {
            _root = Path.Combine(Path.GetTempPath(), "GameHQ.Playnite.Tests." + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_root);
        }

        public void Dispose()
        {
            try { Directory.Delete(_root, true); } catch (Exception) { /* best effort cleanup */ }
        }

        [Fact]
        public void RejectsANullOrEmptyPath()
        {
            Assert.False(GameHQLocator.IsValidInstall(null));
            Assert.False(GameHQLocator.IsValidInstall(string.Empty));
        }

        [Fact]
        public void RejectsAPathThatDoesNotExist()
        {
            var missing = Path.Combine(_root, "GameHQ.exe");

            Assert.False(GameHQLocator.IsValidInstall(missing));
        }

        [Fact]
        public void RejectsARootExeWithoutAPackagedAppFolder()
        {
            var rootExe = Path.Combine(_root, "GameHQ.exe");
            File.WriteAllBytes(rootExe, new byte[] { 0 });

            Assert.False(GameHQLocator.IsValidInstall(rootExe));
        }

        [Fact]
        public void AcceptsARootExeWithAPackagedAppFolder()
        {
            var rootExe = CreateInstall("valid");

            Assert.True(GameHQLocator.IsValidInstall(rootExe));
        }

        [Fact]
        public void RejectsAnUnexpectedRootExecutableName()
        {
            var install = Path.Combine(_root, "unexpected");
            Directory.CreateDirectory(Path.Combine(install, "app"));
            var rootExe = Path.Combine(install, "Other.exe");
            File.WriteAllBytes(rootExe, new byte[] { 0 });
            File.WriteAllBytes(Path.Combine(install, "app", "GameHQ.exe"), new byte[] { 0 });

            Assert.False(GameHQLocator.IsValidInstall(rootExe));
        }

        [Fact]
        public void CandidateOrderMatchesTheInstallerDiscoveryContract()
        {
            var configured = CreateInstall("configured");
            var installLocation = CreateInstall("install-location");
            var appPath = CreateInstall("app-path");
            var run = CreateInstall("run");
            var fallback = CreateInstall("fallback");

            Assert.Equal(Path.GetFullPath(configured), GameHQLocator.LocateFromCandidates(configured, installLocation, appPath, run, fallback));
            Assert.Equal(Path.GetFullPath(installLocation), GameHQLocator.LocateFromCandidates(null, installLocation, appPath, run, fallback));
            Assert.Equal(Path.GetFullPath(appPath), GameHQLocator.LocateFromCandidates(null, null, appPath, run, fallback));
            Assert.Equal(Path.GetFullPath(run), GameHQLocator.LocateFromCandidates(null, null, null, run, fallback));
            Assert.Equal(Path.GetFullPath(fallback), GameHQLocator.LocateFromCandidates(null, null, null, null, fallback));
        }

        [Fact]
        public void InvalidEarlierCandidatesFallThroughToTheNextValidInstall()
        {
            var missing = Path.Combine(_root, "missing", "GameHQ.exe");
            var valid = CreateInstall("valid-fallback");

            Assert.Equal(Path.GetFullPath(valid), GameHQLocator.LocateFromCandidates(missing, null, valid, null, null));
        }

        private string CreateInstall(string name)
        {
            var install = Path.Combine(_root, name);
            Directory.CreateDirectory(Path.Combine(install, "app"));
            var rootExe = Path.Combine(install, "GameHQ.exe");
            File.WriteAllBytes(rootExe, new byte[] { 0 });
            File.WriteAllBytes(Path.Combine(install, "app", "GameHQ.exe"), new byte[] { 0 });
            return rootExe;
        }
    }
}
