using Xunit;

namespace GameHQ.Playnite.Tests
{
    // Kept independent of Playnite.SDK (net462-only, GAC-loaded types) so
    // this test project can target a modern TFM and run with plain `dotnet
    // test`. Protocol/locator/launcher tests land alongside p5-2/p5-3.
    public class PlaceholderTests
    {
        [Fact]
        public void TestProjectIsWired()
        {
            Assert.True(true);
        }
    }
}
