using System;
using System.IO;
using System.Security.Cryptography;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class ImportSelfTests
    {
        [MenuItem("Tools/RenderMax/Advanced/Tests/Run Import Self Tests")]
        public static void Run()
        {
            string root = Path.Combine(Path.GetTempPath(), "RenderMaxTest_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);
            try
            {
                var row = MeshImporter.ParseRow("0,\"UV,0\",\"a\"\"b\"");
                Require(row.Length == 3 && row[1] == "UV,0" && row[2] == "a\"b", "CSV quotes");
                bool rejected = false;
                try { MeshImporter.ParseRow("\"bad"); } catch (InvalidDataException) { rejected = true; }
                Require(rejected, "CSV malformed quote rejection");
                File.WriteAllText(Path.Combine(root, "data.bin"), "evidence");
                string hash;
                using (var h = SHA256.Create()) hash = BitConverter.ToString(h.ComputeHash(File.ReadAllBytes(Path.Combine(root, "data.bin")))).Replace("-", "").ToLowerInvariant();
                var manifest = new PackageManifest { format = "rendermax.asset-package", schemaVersion = "1.0.0", status = "completed", draws = new DrawEntry[0],
                    files = new[] { new FileEntry { path = "data.bin", bytes = 8, sha256 = hash } } };
                string path = Path.Combine(root, "manifest.json");
                File.WriteAllText(path, JsonUtility.ToJson(manifest));
                var reader = new PackageReader(path); reader.Validate();
                rejected = false;
                try { reader.Resolve("../escape"); } catch (InvalidDataException) { rejected = true; }
                Require(rejected, "Traversal rejection");
                File.WriteAllText(Path.Combine(root, "data.bin"), "tampered");
                rejected = false;
                try { reader.Validate(); } catch (InvalidDataException) { rejected = true; }
                Require(rejected, "Hash tamper rejection");
                manifest.status = "failed"; File.WriteAllText(path, JsonUtility.ToJson(manifest));
                rejected = false;
                try { new PackageReader(path); } catch (InvalidDataException) { rejected = true; }
                Require(rejected, "Incomplete package rejection");
                Debug.Log("RenderMax import self tests PASSED (6 checks).");
            }
            finally { Directory.Delete(root, true); }
        }
        static void Require(bool value, string name) { if (!value) throw new Exception("RenderMax self test failed: " + name); }
    }
}
