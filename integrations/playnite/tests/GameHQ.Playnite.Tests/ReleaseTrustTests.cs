using System;
using System.IO;
using System.Text.Json;
using GameHQ.Playnite.Security;
using Xunit;

namespace GameHQ.Playnite.Tests
{
    public sealed class ReleaseTrustTests
    {
        private static ReleaseTrustedKey Key(string id, string publicHex,
            ReleaseKeyState state = ReleaseKeyState.Current) => new ReleaseTrustedKey
        {
            KeyId = id,
            PublicKey = Convert.FromHexString(publicHex),
            State = state
        };

        [Theory]
        [InlineData("rfc8032-1", "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b")]
        [InlineData("rfc8032-2", "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
            "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00")]
        [InlineData("rfc8032-3", "fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025", "af82",
            "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a")]
        public void VerifiesRfc8032Vectors(string id, string publicHex, string messageHex, string signatureHex)
        {
            var result = ReleaseTrust.Verify(Convert.FromHexString(messageHex),
                Convert.ToBase64String(Convert.FromHexString(signatureHex)), id, 1,
                new[] { Key(id, publicHex) });
            Assert.True(result.Accepted, result.Error);
        }

        [Fact]
        public void VerifiesSharedGameHqVector()
        {
            using var document = JsonDocument.Parse(File.ReadAllBytes(
                Path.Combine(AppContext.BaseDirectory, "Fixtures", "gamehq-test-vector.json")));
            var root = document.RootElement;
            var key = new ReleaseTrustedKey
            {
                KeyId = root.GetProperty("keyId").GetString(),
                PublicKey = ReleaseTrust.DecodeStrictPublicKey(root.GetProperty("publicKeyBase64").GetString()),
                State = ReleaseKeyState.Current
            };
            var result = ReleaseTrust.Verify(
                Convert.FromBase64String(root.GetProperty("manifestBase64").GetString()),
                root.GetProperty("signatureBase64").GetString(), key.KeyId,
                root.GetProperty("releaseSequence").GetUInt64(), new[] { key });
            Assert.True(result.Accepted, result.Error);
        }

        [Fact]
        public void RejectsMalformedTamperedAndWrongTrust()
        {
            var key = Key("gamehq-test", "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
            var message = Convert.FromHexString("72");
            var signature = Convert.ToBase64String(Convert.FromHexString(
                "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"));
            Assert.Equal(ReleaseVerifyCode.InvalidSignatureEncoding,
                ReleaseTrust.Verify(message, signature + "\n", key.KeyId, 1, new[] { key }).Code);
            message[0] ^= 1;
            Assert.Equal(ReleaseVerifyCode.InvalidSignature,
                ReleaseTrust.Verify(message, signature, key.KeyId, 1, new[] { key }).Code);
            message[0] ^= 1;
            key.State = ReleaseKeyState.Revoked;
            Assert.Equal(ReleaseVerifyCode.RevokedKey,
                ReleaseTrust.Verify(message, signature, key.KeyId, 1, new[] { key }).Code);
        }

        [Fact]
        public void EnforcesRollbackAndPersistsState()
        {
            var key = Key("gamehq-test", "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
            var message = Convert.FromHexString("72");
            var signature = Convert.ToBase64String(Convert.FromHexString(
                "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"));
            var accepted = ReleaseTrust.Verify(message, signature, key.KeyId, 10, new[] { key });
            Assert.True(accepted.Accepted);
            var state = new ReleaseSequenceState
                { HighestReleaseSequence = 10, ManifestSha256 = accepted.ManifestSha256 };
            Assert.Equal(ReleaseVerifyCode.Rollback,
                ReleaseTrust.Verify(message, signature, key.KeyId, 9, new[] { key }, state).Code);

            var root = Path.Combine(Path.GetTempPath(), "gamehq-release-trust-" + Guid.NewGuid().ToString("N"));
            try
            {
                var path = Path.Combine(root, "release-trust.json");
                ReleaseTrust.StoreSequenceStateAtomically(path, state);
                var loaded = ReleaseTrust.LoadSequenceState(path);
                Assert.Equal(state.HighestReleaseSequence, loaded.HighestReleaseSequence);
                Assert.Equal(state.ManifestSha256, loaded.ManifestSha256);
                Assert.False(File.Exists(path + ".new"));
            }
            finally { if (Directory.Exists(root)) Directory.Delete(root, true); }
        }
    }
}
