using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using Org.BouncyCastle.Math.EC.Rfc8032;

namespace GameHQ.Playnite.Security
{
    internal enum ReleaseKeyState { Current, Next, Revoked }
    internal enum ReleaseVerifyCode
    {
        Accepted, InvalidKeyId, UnknownKey, InactiveKey, RevokedKey,
        InvalidSignatureEncoding, InvalidSignature, Rollback, Equivocation, StateError
    }

    internal sealed class ReleaseTrustedKey
    {
        public string KeyId { get; set; }
        public byte[] PublicKey { get; set; }
        public ReleaseKeyState State { get; set; }
        public ulong MinimumReleaseSequence { get; set; }
    }

    internal sealed class ReleaseSequenceState
    {
        public ulong HighestReleaseSequence { get; set; }
        public string ManifestSha256 { get; set; }
    }

    internal sealed class ReleaseVerifyResult
    {
        public ReleaseVerifyCode Code { get; set; }
        public string Error { get; set; }
        public string ManifestSha256 { get; set; }
        public bool Accepted => Code == ReleaseVerifyCode.Accepted;
    }

    internal static class ReleaseTrust
    {
        private static readonly Regex KeyIdPattern = new Regex("^[a-z0-9][a-z0-9._-]{0,63}$",
            RegexOptions.CultureInvariant);
        private static readonly Regex SignaturePattern = new Regex("^[A-Za-z0-9+/]{86}==$",
            RegexOptions.CultureInvariant);
        private static readonly Regex PublicKeyPattern = new Regex("^[A-Za-z0-9+/]{43}=$",
            RegexOptions.CultureInvariant);
        private static readonly Regex StatePattern = new Regex(
            "^\\{\"schemaVersion\":1,\"highestReleaseSequence\":([0-9]+),\"manifestSha256\":\"([0-9a-f]{64})\"\\}\\n?$",
            RegexOptions.CultureInvariant);

        internal static byte[] DecodeStrictSignature(string encoded) =>
            DecodeCanonical(encoded, SignaturePattern, Ed25519.SignatureSize);

        internal static byte[] DecodeStrictPublicKey(string encoded) =>
            DecodeCanonical(encoded, PublicKeyPattern, Ed25519.PublicKeySize);

        private static byte[] DecodeCanonical(string encoded, Regex pattern, int expectedBytes)
        {
            if (encoded == null || !pattern.IsMatch(encoded)) return null;
            try
            {
                var decoded = Convert.FromBase64String(encoded);
                if (decoded.Length != expectedBytes || Convert.ToBase64String(decoded) != encoded) return null;
                return decoded;
            }
            catch (FormatException) { return null; }
        }

        internal static ReleaseVerifyResult Verify(byte[] manifestBytes, string signatureBase64,
            string keyId, ulong releaseSequence, IReadOnlyCollection<ReleaseTrustedKey> trustedKeys,
            ReleaseSequenceState previousState = null)
        {
            if (manifestBytes == null) throw new ArgumentNullException(nameof(manifestBytes));
            if (keyId == null || !KeyIdPattern.IsMatch(keyId))
                return Reject(ReleaseVerifyCode.InvalidKeyId, "invalid keyId");
            var key = trustedKeys?.SingleOrDefault(candidate => candidate.KeyId == keyId);
            if (key == null) return Reject(ReleaseVerifyCode.UnknownKey, "unknown signing key");
            if (key.State == ReleaseKeyState.Revoked)
                return Reject(ReleaseVerifyCode.RevokedKey, "revoked signing key");
            if (key.State != ReleaseKeyState.Current || releaseSequence < key.MinimumReleaseSequence)
                return Reject(ReleaseVerifyCode.InactiveKey, "signing key is not active for this sequence");
            if (key.PublicKey == null || key.PublicKey.Length != Ed25519.PublicKeySize)
                return Reject(ReleaseVerifyCode.StateError, "trusted key has invalid length");
            var signature = DecodeStrictSignature(signatureBase64);
            if (signature == null)
                return Reject(ReleaseVerifyCode.InvalidSignatureEncoding,
                    "signature is not canonical Base64 for 64 bytes");
            if (!Ed25519.Verify(signature, 0, key.PublicKey, 0, manifestBytes, 0, manifestBytes.Length))
                return Reject(ReleaseVerifyCode.InvalidSignature, "Ed25519 signature verification failed");

            string manifestHash;
            using (var sha = SHA256.Create())
                manifestHash = ToHex(sha.ComputeHash(manifestBytes));
            if (previousState != null)
            {
                if (releaseSequence < previousState.HighestReleaseSequence)
                    return Reject(ReleaseVerifyCode.Rollback, "release sequence rollback rejected");
                if (releaseSequence == previousState.HighestReleaseSequence &&
                    !StringComparer.Ordinal.Equals(manifestHash, previousState.ManifestSha256))
                    return Reject(ReleaseVerifyCode.Equivocation,
                        "release sequence was reused for different manifest bytes");
            }
            return new ReleaseVerifyResult
                { Code = ReleaseVerifyCode.Accepted, ManifestSha256 = manifestHash };
        }

        internal static ReleaseSequenceState LoadSequenceState(string path)
        {
            if (!File.Exists(path)) return new ReleaseSequenceState();
            var match = StatePattern.Match(File.ReadAllText(path, Encoding.UTF8));
            if (!match.Success) throw new InvalidDataException("release trust state is corrupt");
            return new ReleaseSequenceState
            {
                HighestReleaseSequence = ulong.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture),
                ManifestSha256 = match.Groups[2].Value
            };
        }

        internal static void StoreSequenceStateAtomically(string path, ReleaseSequenceState state)
        {
            if (state == null || state.ManifestSha256 == null ||
                !Regex.IsMatch(state.ManifestSha256, "^[0-9a-f]{64}$", RegexOptions.CultureInvariant))
                throw new InvalidDataException("release trust state has an invalid manifest hash");
            var directory = Path.GetDirectoryName(Path.GetFullPath(path));
            Directory.CreateDirectory(directory);
            var pending = path + ".new";
            var text = "{\"schemaVersion\":1,\"highestReleaseSequence\":" +
                state.HighestReleaseSequence.ToString(CultureInfo.InvariantCulture) +
                ",\"manifestSha256\":\"" + state.ManifestSha256 + "\"}\n";
            using (var stream = new FileStream(pending, FileMode.Create, FileAccess.Write, FileShare.None,
                       4096, FileOptions.WriteThrough))
            using (var writer = new StreamWriter(stream, new UTF8Encoding(false)))
            {
                writer.Write(text);
                writer.Flush();
                stream.Flush(true);
            }
            if (File.Exists(path)) File.Replace(pending, path, null);
            else File.Move(pending, path);
        }

        private static ReleaseVerifyResult Reject(ReleaseVerifyCode code, string error) =>
            new ReleaseVerifyResult { Code = code, Error = error };

        private static string ToHex(byte[] bytes)
        {
            var builder = new StringBuilder(bytes.Length * 2);
            foreach (var value in bytes) builder.Append(value.ToString("x2", CultureInfo.InvariantCulture));
            return builder.ToString();
        }
    }
}
