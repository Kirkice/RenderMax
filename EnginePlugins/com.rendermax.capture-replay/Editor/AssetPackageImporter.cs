using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class AssetPackageImporter
    {
        public static ImportReport Import(string manifestPath, string destination)
        {
            destination = destination.Replace('\\', '/').TrimEnd('/');
            if (!destination.StartsWith("Assets/", StringComparison.Ordinal) || destination.Contains("..") || Directory.Exists(destination) || File.Exists(destination))
                throw new ArgumentException("Destination must be a NEW folder below Assets/.");
            var report = new ImportReport { source = manifestPath, destination = destination };
            var reader = new PackageReader(manifestPath);
            try
            {
                reader.Validate((i, n) => {
                    if (EditorUtility.DisplayCancelableProgressBar("RenderMax: verify SHA-256", i + " / " + n, (float)i / n))
                        throw new OperationCanceledException();
                });
                report.integrityPassed = true;
                Directory.CreateDirectory(destination);
                // Preserve every evidence file. The imported assets never replace authoritative bytes.
                for (int i = 0; i < reader.Manifest.files.Length; ++i)
                {
                    var entry = reader.Manifest.files[i];
                    if (EditorUtility.DisplayCancelableProgressBar("RenderMax: copy evidence", entry.path, (float)i / reader.Manifest.files.Length))
                        throw new OperationCanceledException();
                    var target = destination + "/Evidence~/" + entry.path;
                    Directory.CreateDirectory(Path.GetDirectoryName(target));
                    File.Copy(reader.FilePath(entry.path), target, false);
                }
                File.Copy(manifestPath, destination + "/Evidence~/manifest.json", false);
                AssetDatabase.Refresh();
                var capture = ScriptableObject.CreateInstance<CaptureAsset>();
                AssetDatabase.CreateAsset(capture, destination + "/Capture.asset");
                capture.sourceManifest = destination + "/Evidence~/manifest.json";
                var textureCache = new Dictionary<string, List<Texture>>();
                for (int i = 0; i < reader.Manifest.draws.Length; ++i)
                {
                    var entry = reader.Manifest.draws[i];
                    if (EditorUtility.DisplayCancelableProgressBar("RenderMax: import draws", "Event " + entry.eventId, (float)i / reader.Manifest.draws.Length))
                        throw new OperationCanceledException();
                    var draw = reader.Read<DrawDescription>(entry.path);
                    string folder = destination + "/Draws/E" + entry.eventId.ToString("D8");
                    Directory.CreateDirectory(folder); AssetDatabase.Refresh();
                    var imported = new ImportedDraw { eventId = entry.eventId, sourceDraw = entry.path };
                    if (!string.IsNullOrEmpty(draw.mesh))
                    {
                        try
                        {
                            var warnings = new List<string>();
                            var mesh = MeshImporter.Build(reader, reader.Read<MeshDescription>(draw.mesh), warnings);
                            string path = folder + "/VSInput.asset"; AssetDatabase.CreateAsset(mesh, path);
                            imported.mesh = mesh; report.imported.Add(path);
                            foreach (string w in warnings) report.unsupported.Add("E" + entry.eventId + ": " + w);
                        }
                        catch (Exception e) { report.errors.Add("E" + entry.eventId + " mesh: " + e.Message); }
                    }
                    foreach (var reference in draw.textures ?? new ResourceReference[0])
                    {
                        List<Texture> previews;
                        if (!textureCache.TryGetValue(reference.path, out previews))
                        {
                            previews = ImportTextures(reader, reference.path, destination, report);
                            textureCache.Add(reference.path, previews);
                        }
                        imported.texturePreviews.AddRange(previews);
                    }
                    capture.draws.Add(imported); EditorUtility.SetDirty(capture);
                }
                report.unsupported.Add("Shader translation, constant/sampler binding, GPU draw replay and pixel comparison are not implemented in 0.1.");
                AssetDatabase.SaveAssets(); Selection.activeObject = capture;
            }
            catch (Exception e)
            {
                report.errors.Add(e.ToString());
                // A partial import is deliberately retained with an explicit report, never overwritten.
                Debug.LogError("RenderMax import did not complete: " + e.Message);
            }
            finally
            {
                EditorUtility.ClearProgressBar();
                if (Directory.Exists(destination))
                {
                    File.WriteAllText(destination + "/ImportReport.json", JsonUtility.ToJson(report, true));
                    AssetDatabase.Refresh();
                }
            }
            return report;
        }

        static List<Texture> ImportTextures(PackageReader reader, string metadataPath, string destination, ImportReport report)
        {
            var result = new List<Texture>();
            var t = reader.Read<TextureDescription>(metadataPath);
            if (t.cubemap || t.depth > 1 || t.arraySize > 1 || t.samples > 1)
                report.unsupported.Add(metadataPath + ": cube/volume/array/MSAA assembly not implemented; evidence preserved, images are previews only.");
            foreach (var image in t.images ?? new TextureImage[0])
            {
                string ext = Path.GetExtension(image.path).ToLowerInvariant();
                if (ext != ".png" && ext != ".tga" && ext != ".exr")
                { report.unsupported.Add(image.path + ": DDS importer not implemented; original retained."); continue; }
                try
                {
                    string path = destination + "/Textures/" + image.path;
                    Directory.CreateDirectory(Path.GetDirectoryName(path));
                    File.Copy(reader.FilePath(image.path), path, false);
                    AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceSynchronousImport);
                    var importer = AssetImporter.GetAtPath(path) as TextureImporter;
                    if (importer == null) throw new InvalidDataException("Unity did not recognize texture.");
                    importer.textureType = TextureImporterType.Default;
                    importer.sRGBTexture = t.srgb;
                    importer.alphaSource = TextureImporterAlphaSource.FromInput;
                    importer.alphaIsTransparency = false;
                    importer.mipmapEnabled = false;
                    importer.npotScale = TextureImporterNPOTScale.None;
                    importer.textureCompression = TextureImporterCompression.Uncompressed;
                    importer.crunchedCompression = false;
                    importer.isReadable = true;
                    int width = Math.Max(1, t.width >> Math.Max(0, image.mip));
                    int height = Math.Max(1, t.height >> Math.Max(0, image.mip));
                    if (Math.Max(width, height) > 16384) throw new NotSupportedException("Texture exceeds importer size limit; raw data retained.");
                    importer.maxTextureSize = Mathf.Min(16384, Mathf.NextPowerOfTwo(Math.Max(32, Math.Max(width, height))));
                    importer.SaveAndReimport();
                    var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
                    if (texture == null || texture.width != width || texture.height != height)
                        throw new InvalidDataException("Imported texture dimensions differ from capture.");
                    result.Add(texture); report.imported.Add(path);
                }
                catch (Exception e) { report.errors.Add(image.path + ": " + e.Message); }
            }
            if (t.mips > 1) report.unsupported.Add(metadataPath + ": individual captured mip images imported separately; exact GPU mip-chain assembly pending.");
            return result;
        }
    }
}
