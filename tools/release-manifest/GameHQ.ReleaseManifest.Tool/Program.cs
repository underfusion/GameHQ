using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Org.BouncyCastle.Math.EC.Rfc8032;

internal static class Program
{
    private const string ProductId = "underfusion.gamehq";
    private const string TestKeyId = "gamehq-test-2026-01";
    // RFC 8032 vector seed. TEST ONLY. Release validation rejects this key in
    // signed/Stable mode; production key material must never enter this tool.
    private static readonly byte[] TestSeed = Convert.FromHexString(
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
    private static readonly Regex SignatureText = new("^[A-Za-z0-9+/]{86}==$",
        RegexOptions.CultureInvariant);

    private static int Main(string[] args)
    {
        try
        {
            if (args.Length == 0) return Usage();
            var options = ParseOptions(args.Skip(1).ToArray());
            return args[0] switch
            {
                "generate-test" => Generate(options),
                "verify-test" => Verify(options),
                "print-test-public-key" => PrintPublicKey(),
                _ => Usage()
            };
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine("ERROR: " + exception.Message);
            return 2;
        }
    }

    private static int Generate(IReadOnlyDictionary<string, string> options)
    {
        var releaseDir = RequiredPath(options, "release-dir");
        var version = Required(options, "version");
        if (!Regex.IsMatch(version, "^[0-9]+\\.[0-9]+\\.[0-9]+$"))
            throw new InvalidDataException("version must be X.Y.Z");
        var sequence = ulong.Parse(Required(options, "sequence"), CultureInfo.InvariantCulture);
        if (sequence == 0) throw new InvalidDataException("release sequence must be positive");
        var published = DateTimeOffset.ParseExact(Required(options, "published-at-utc"),
            "yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture,
            DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal);

        var artifacts = new[]
        {
            Artifact(releaseDir, $"GameHQ-{version}-win64-setup.exe", "setup"),
            Artifact(releaseDir, $"GameHQ-{version}-win64-portable.zip", "portable"),
            Artifact(releaseDir, $"GameHQ-{version}-win64-update.zip", "update")
        };
        var manifestPath = Path.Combine(releaseDir, "gamehq-release.json");
        var signaturePath = Path.Combine(releaseDir, "gamehq-release.sig");
        byte[] manifest;
        using (var buffer = new MemoryStream())
        {
            using (var writer = new Utf8JsonWriter(buffer, new JsonWriterOptions { Indented = false }))
            {
                writer.WriteStartObject();
                writer.WriteNumber("schemaVersion", 1);
                writer.WriteString("productId", ProductId);
                writer.WriteString("version", version);
                writer.WriteNumber("releaseSequence", sequence);
                writer.WriteString("publishedAtUtc", published.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture));
                writer.WriteString("minimumUpdaterVersion", version);
                writer.WriteStartArray("artifacts");
                foreach (var artifact in artifacts)
                {
                    writer.WriteStartObject();
                    writer.WriteString("kind", artifact.Kind);
                    writer.WriteString("fileName", artifact.FileName);
                    writer.WriteNumber("size", artifact.Size);
                    writer.WriteString("sha256", artifact.Sha256);
                    writer.WriteEndObject();
                }
                writer.WriteEndArray();
                writer.WriteString("keyId", TestKeyId);
                writer.WriteEndObject();
            }
            buffer.WriteByte((byte)'\n');
            manifest = buffer.ToArray();
        }
        var signature = new byte[Ed25519.SignatureSize];
        Ed25519.Sign(TestSeed, 0, manifest, 0, manifest.Length, signature, 0);
        File.WriteAllBytes(manifestPath, manifest);
        File.WriteAllText(signaturePath, Convert.ToBase64String(signature), new UTF8Encoding(false));
        Console.WriteLine($"generated {Path.GetFileName(manifestPath)} sequence={sequence} keyId={TestKeyId}");
        return 0;
    }

    private static int Verify(IReadOnlyDictionary<string, string> options)
    {
        var releaseDir = RequiredPath(options, "release-dir");
        var manifestPath = Path.Combine(releaseDir, "gamehq-release.json");
        var signaturePath = Path.Combine(releaseDir, "gamehq-release.sig");
        var manifest = File.ReadAllBytes(manifestPath);
        if (manifest.Length == 0 || manifest.Length > 1024 * 1024)
            throw new InvalidDataException("manifest size is invalid");
        var signatureText = File.ReadAllText(signaturePath, Encoding.ASCII);
        if (!SignatureText.IsMatch(signatureText))
            throw new InvalidDataException("signature is not strict padded Base64");
        var signature = Convert.FromBase64String(signatureText);
        if (signature.Length != Ed25519.SignatureSize || Convert.ToBase64String(signature) != signatureText)
            throw new InvalidDataException("signature Base64 is non-canonical");
        var publicKey = new byte[Ed25519.PublicKeySize];
        Ed25519.GeneratePublicKey(TestSeed, 0, publicKey, 0);
        if (!Ed25519.Verify(signature, 0, publicKey, 0, manifest, 0, manifest.Length))
            throw new CryptographicException("manifest signature is invalid");

        // Parsing and field trust begin only after raw-byte verification.
        using var document = JsonDocument.Parse(manifest, new JsonDocumentOptions
            { AllowTrailingCommas = false, CommentHandling = JsonCommentHandling.Disallow });
        var root = document.RootElement;
        RequireExactProperties(root, "schemaVersion", "productId", "version", "releaseSequence",
            "publishedAtUtc", "minimumUpdaterVersion", "artifacts", "keyId");
        if (root.GetProperty("schemaVersion").GetInt32() != 1 ||
            root.GetProperty("productId").GetString() != ProductId ||
            root.GetProperty("keyId").GetString() != TestKeyId ||
            root.GetProperty("releaseSequence").GetUInt64() == 0)
            throw new InvalidDataException("manifest identity or sequence is invalid");
        var version = root.GetProperty("version").GetString() ?? "";
        if (!Regex.IsMatch(version, "^[0-9]+\\.[0-9]+\\.[0-9]+$") ||
            root.GetProperty("minimumUpdaterVersion").GetString() != version)
            throw new InvalidDataException("manifest version fields are invalid");
        _ = DateTimeOffset.ParseExact(root.GetProperty("publishedAtUtc").GetString()!,
            "yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture,
            DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal);
        var seenKinds = new HashSet<string>(StringComparer.Ordinal);
        foreach (var entry in root.GetProperty("artifacts").EnumerateArray())
        {
            RequireExactProperties(entry, "kind", "fileName", "size", "sha256");
            var kind = entry.GetProperty("kind").GetString() ?? "";
            var expectedName = kind switch
            {
                "setup" => $"GameHQ-{version}-win64-setup.exe",
                "portable" => $"GameHQ-{version}-win64-portable.zip",
                "update" => $"GameHQ-{version}-win64-update.zip",
                _ => throw new InvalidDataException("unknown artifact kind")
            };
            if (!seenKinds.Add(kind) || entry.GetProperty("fileName").GetString() != expectedName)
                throw new InvalidDataException("duplicate artifact kind or invalid filename");
            var artifact = new FileInfo(Path.Combine(releaseDir, expectedName));
            if (!artifact.Exists || artifact.Length != entry.GetProperty("size").GetInt64())
                throw new InvalidDataException($"artifact size mismatch: {expectedName}");
            using var stream = artifact.OpenRead();
            var actualHash = Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
            if (entry.GetProperty("sha256").GetString() != actualHash)
                throw new InvalidDataException($"artifact hash mismatch: {expectedName}");
        }
        if (!seenKinds.SetEquals(new[] { "setup", "portable", "update" }))
            throw new InvalidDataException("manifest does not contain the exact artifact set");
        Console.WriteLine("verified test-key release manifest");
        return 0;
    }

    private static int PrintPublicKey()
    {
        var key = new byte[Ed25519.PublicKeySize];
        Ed25519.GeneratePublicKey(TestSeed, 0, key, 0);
        Console.WriteLine(Convert.ToBase64String(key));
        return 0;
    }

    private static void RequireExactProperties(JsonElement element, params string[] names)
    {
        var actual = element.EnumerateObject().Select(property => property.Name).ToArray();
        if (!actual.SequenceEqual(names, StringComparer.Ordinal))
            throw new InvalidDataException("manifest fields or canonical order are invalid");
    }

    private static ArtifactInfo Artifact(string releaseDir, string fileName, string kind)
    {
        var info = new FileInfo(Path.Combine(releaseDir, fileName));
        if (!info.Exists) throw new FileNotFoundException("release artifact is missing", info.FullName);
        using var stream = info.OpenRead();
        return new ArtifactInfo(kind, fileName, info.Length,
            Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant());
    }

    private static Dictionary<string, string> ParseOptions(string[] args)
    {
        var result = new Dictionary<string, string>(StringComparer.Ordinal);
        for (var i = 0; i < args.Length; i += 2)
        {
            if (i + 1 >= args.Length || !args[i].StartsWith("--", StringComparison.Ordinal))
                throw new ArgumentException("options must be --name value pairs");
            result.Add(args[i][2..], args[i + 1]);
        }
        return result;
    }

    private static string Required(IReadOnlyDictionary<string, string> options, string name) =>
        options.TryGetValue(name, out var value) && !string.IsNullOrWhiteSpace(value)
            ? value : throw new ArgumentException("missing --" + name);

    private static string RequiredPath(IReadOnlyDictionary<string, string> options, string name)
    {
        var path = Path.GetFullPath(Required(options, name));
        if (!Directory.Exists(path)) throw new DirectoryNotFoundException(path);
        return path;
    }

    private static int Usage()
    {
        Console.Error.WriteLine("Usage: release-manifest <generate-test|verify-test|print-test-public-key> [options]");
        return 1;
    }

    private sealed record ArtifactInfo(string Kind, string FileName, long Size, string Sha256);
}
