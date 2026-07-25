using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using Org.BouncyCastle.Math.EC.Rfc8032;

internal static class ProductionKeyStore
{
    internal const string SeedEnvironmentVariable = "GAMEHQ_RELEASE_ED25519_SEED_BASE64";
    private static readonly Regex KeyIdPattern = new("^[a-z0-9][a-z0-9._-]{0,63}$",
        RegexOptions.CultureInvariant);

    internal static (string PublicKeyBase64, string PublicKeySha256) Provision(
        string keyId, string credentialTarget, string repository, string environment)
    {
        ValidateKeyId(keyId);
        if (!OperatingSystem.IsWindows())
            throw new PlatformNotSupportedException("production key provisioning requires Windows");
        if (!Regex.IsMatch(repository, "^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"))
            throw new ArgumentException("GitHub repository must be owner/name");
        if (string.IsNullOrWhiteSpace(environment))
            throw new ArgumentException("GitHub environment is required");
        if (CredentialExists(credentialTarget)
            || GitHubEnvironmentSecretExists(repository, environment))
            throw new InvalidOperationException(
                "production key storage already exists; provisioning will not overwrite it");

        var seed = RandomNumberGenerator.GetBytes(Ed25519.SecretKeySize);
        try
        {
            var publicKey = new byte[Ed25519.PublicKeySize];
            Ed25519.GeneratePublicKey(seed, 0, publicKey, 0);
            var seedText = Convert.ToBase64String(seed);
            WriteCredential(credentialTarget, keyId, seedText);
            SetGitHubEnvironmentSecret(repository, environment, seedText);
            return (Convert.ToBase64String(publicKey),
                Convert.ToHexString(SHA256.HashData(publicKey)).ToLowerInvariant());
        }
        finally
        {
            CryptographicOperations.ZeroMemory(seed);
        }
    }

    internal static byte[] LoadSeed(IReadOnlyDictionary<string, string> options)
    {
        string encoded;
        if (options.TryGetValue("credential-target", out var target)
            && !string.IsNullOrWhiteSpace(target)) {
            encoded = ReadCredential(target);
        } else {
            encoded = Environment.GetEnvironmentVariable(SeedEnvironmentVariable)
                ?? throw new InvalidOperationException(
                    $"{SeedEnvironmentVariable} is not available in this protected release context");
        }

        byte[] seed;
        try {
            seed = Convert.FromBase64String(encoded);
        } catch (FormatException) {
            throw new InvalidDataException("production seed is not canonical Base64");
        }
        if (seed.Length != Ed25519.SecretKeySize || Convert.ToBase64String(seed) != encoded)
            throw new InvalidDataException("production seed must be canonical Base64 for 32 bytes");
        return seed;
    }

    internal static byte[] DecodePublicKey(string encoded)
    {
        byte[] key;
        try {
            key = Convert.FromBase64String(encoded);
        } catch (FormatException) {
            throw new InvalidDataException("production public key is not canonical Base64");
        }
        if (key.Length != Ed25519.PublicKeySize || Convert.ToBase64String(key) != encoded)
            throw new InvalidDataException("production public key must be canonical Base64 for 32 bytes");
        return key;
    }

    internal static void ValidateKeyId(string keyId)
    {
        if (!KeyIdPattern.IsMatch(keyId))
            throw new InvalidDataException("keyId must match ^[a-z0-9][a-z0-9._-]{0,63}$");
    }

    private static void SetGitHubEnvironmentSecret(
        string repository, string environment, string seedText)
    {
        var start = new ProcessStartInfo("gh") {
            UseShellExecute = false,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };
        start.ArgumentList.Add("secret");
        start.ArgumentList.Add("set");
        start.ArgumentList.Add(SeedEnvironmentVariable);
        start.ArgumentList.Add("--env");
        start.ArgumentList.Add(environment);
        start.ArgumentList.Add("--repo");
        start.ArgumentList.Add(repository);
        using var process = Process.Start(start)
            ?? throw new InvalidOperationException("could not start GitHub CLI");
        process.StandardInput.Write(seedText);
        process.StandardInput.Close();
        var standardError = process.StandardError.ReadToEnd();
        _ = process.StandardOutput.ReadToEnd();
        process.WaitForExit();
        if (process.ExitCode != 0)
            throw new InvalidOperationException(
                "GitHub environment secret storage failed: " + standardError.Trim());
    }

    private static bool GitHubEnvironmentSecretExists(string repository, string environment)
    {
        var start = new ProcessStartInfo("gh") {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };
        start.ArgumentList.Add("secret");
        start.ArgumentList.Add("list");
        start.ArgumentList.Add("--env");
        start.ArgumentList.Add(environment);
        start.ArgumentList.Add("--repo");
        start.ArgumentList.Add(repository);
        using var process = Process.Start(start)
            ?? throw new InvalidOperationException("could not start GitHub CLI");
        var output = process.StandardOutput.ReadToEnd();
        var standardError = process.StandardError.ReadToEnd();
        process.WaitForExit();
        if (process.ExitCode != 0)
            throw new InvalidOperationException(
                "GitHub environment secret lookup failed: " + standardError.Trim());
        return output.Split('\n', StringSplitOptions.RemoveEmptyEntries)
            .Any(line => line.Split('\t', 2)[0] == SeedEnvironmentVariable);
    }

    private static bool CredentialExists(string target)
    {
        if (CredRead(target, CredentialType.Generic, 0, out var pointer)) {
            CredFree(pointer);
            return true;
        }
        const int ElementNotFound = 1168;
        var error = Marshal.GetLastWin32Error();
        if (error == ElementNotFound)
            return false;
        throw new Win32Exception(error);
    }

    private static void WriteCredential(string target, string userName, string secret)
    {
        if (string.IsNullOrWhiteSpace(target))
            throw new ArgumentException("credential target is required");
        var blob = Marshal.StringToCoTaskMemUni(secret);
        try
        {
            var credential = new Credential {
                Type = CredentialType.Generic,
                TargetName = target,
                UserName = userName,
                CredentialBlob = blob,
                CredentialBlobSize = checked((uint)(secret.Length * sizeof(char))),
                Persist = CredentialPersist.LocalMachine
            };
            if (!CredWrite(ref credential, 0))
                throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        finally
        {
            for (var index = 0; index < secret.Length * sizeof(char); ++index)
                Marshal.WriteByte(blob, index, 0);
            Marshal.FreeCoTaskMem(blob);
        }
    }

    private static string ReadCredential(string target)
    {
        if (!OperatingSystem.IsWindows())
            throw new PlatformNotSupportedException("Windows Credential Manager is unavailable");
        if (!CredRead(target, CredentialType.Generic, 0, out var pointer))
            throw new Win32Exception(Marshal.GetLastWin32Error());
        try
        {
            var credential = Marshal.PtrToStructure<Credential>(pointer);
            if (credential.CredentialBlob == IntPtr.Zero
                || credential.CredentialBlobSize == 0
                || credential.CredentialBlobSize % sizeof(char) != 0)
                throw new InvalidDataException("stored production credential is malformed");
            return Marshal.PtrToStringUni(credential.CredentialBlob,
                checked((int)credential.CredentialBlobSize / sizeof(char)))
                ?? throw new InvalidDataException("stored production credential is empty");
        }
        finally
        {
            CredFree(pointer);
        }
    }

    private enum CredentialType : uint { Generic = 1 }
    private enum CredentialPersist : uint { LocalMachine = 2 }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct Credential
    {
        internal uint Flags;
        internal CredentialType Type;
        internal string TargetName;
        internal string? Comment;
        internal System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
        internal uint CredentialBlobSize;
        internal IntPtr CredentialBlob;
        internal CredentialPersist Persist;
        internal uint AttributeCount;
        internal IntPtr Attributes;
        internal string? TargetAlias;
        internal string UserName;
    }

    [DllImport("advapi32.dll", EntryPoint = "CredWriteW",
        CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredWrite(ref Credential credential, uint flags);

    [DllImport("advapi32.dll", EntryPoint = "CredReadW",
        CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CredRead(string target, CredentialType type, uint flags,
        out IntPtr credential);

    [DllImport("advapi32.dll")]
    private static extern void CredFree(IntPtr credential);
}
