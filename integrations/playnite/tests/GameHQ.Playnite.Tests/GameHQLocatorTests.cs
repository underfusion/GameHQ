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
            var rootExe = Path.Combine(_root, "GameHQ.exe");
            File.WriteAllBytes(rootExe, new byte[] { 0 });

            var appDir = Path.Combine(_root, "app");
            Directory.CreateDirectory(appDir);
            File.WriteAllBytes(Path.Combine(appDir, "GameHQ.exe"), new byte[] { 0 });

            Assert.True(GameHQLocator.IsValidInstall(rootExe));
        }
    }
}
