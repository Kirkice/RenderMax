using System;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class GeneratedShaderSmokeTest
    {
        [Serializable] public class CompileEntry { public string asset, backend, errors; public bool supported, passBound; }
        [Serializable] public class CompileReport { public System.Collections.Generic.List<CompileEntry> shaders = new System.Collections.Generic.List<CompileEntry>(); public string limitation = "Compile/bind check only. No captured resources or visual equivalence validated."; }
        public static void CheckAllCandidates()
        {
            var report = new CompileReport();
            bool asyncCompilation = ShaderUtil.allowAsyncCompilation;
            ShaderUtil.allowAsyncCompilation = false;
            var previous = RenderTexture.active;
            var target = new RenderTexture(16, 16, 0);
            try
            {
                target.Create(); RenderTexture.active = target;
                foreach (string guid in AssetDatabase.FindAssets("t:Shader", new[] { "Assets/Phase2Batch" }))
                {
                    string path = AssetDatabase.GUIDToAssetPath(guid);
                    var shader = AssetDatabase.LoadAssetAtPath<Shader>(path);
                    var entry = new CompileEntry { asset = path, backend = SystemInfo.graphicsDeviceType.ToString(), supported = shader != null && shader.isSupported };
                    report.shaders.Add(entry);
                    if (shader == null) { entry.errors = "Shader missing"; continue; }
                    var material = new Material(shader);
                    try
                    {
                        // Explicit synchronous compilation; SetPass alone can bind an error program.
                        ShaderUtil.CompilePass(material, 0, true);
                        entry.passBound = material.SetPass(0);
                        Graphics.Blit(Texture2D.whiteTexture, target, material);
                        var readback = new Texture2D(16, 16, TextureFormat.RGBA32, false);
                        try { RenderTexture.active = target; readback.ReadPixels(new Rect(0, 0, 16, 16), 0, 0); readback.Apply(); }
                        finally { UnityEngine.Object.DestroyImmediate(readback); }
                        entry.supported = shader.isSupported;
                        foreach (var message in ShaderUtil.GetShaderMessages(shader))
                            entry.errors += message.severity + ": " + message.message + "\n";
                        if (!entry.supported)
                            entry.errors += "BLOCKED: shader.isSupported=false after synchronous compilation and readback. SetPass success is NOT acceptance; an error/fallback program may have been bound.\n";
                    }
                    catch (Exception e) { entry.errors += e.ToString(); }
                    finally { UnityEngine.Object.DestroyImmediate(material); }
                }
            }
            finally
            {
                ShaderUtil.allowAsyncCompilation = asyncCompilation;
                RenderTexture.active = previous; target.Release(); UnityEngine.Object.DestroyImmediate(target);
                File.WriteAllText("Assets/Phase2Batch/UnityCompileReport.json", JsonUtility.ToJson(report, true));
            }
            Debug.Log("RenderMax batch compilation checked " + report.shaders.Count + " shader candidates on " + SystemInfo.graphicsDeviceType);
        }

        public static void CheckEditableShader()
        {
            var shader = AssetDatabase.LoadAssetAtPath<Shader>("Packages/com.rendermax.capture-replay/Editor/Shaders/Phase2/Event63.shader");
            if (shader == null || ShaderUtil.ShaderHasError(shader) || !shader.isSupported)
                throw new InvalidOperationException("Editable E63 shader unsupported on " + SystemInfo.graphicsDeviceType);
            var material = new Material(shader);
            var target = new RenderTexture(16, 16, 0);
            var previous = RenderTexture.active;
            try
            {
                target.Create();
                material.SetTexture("_AlbedoMap", Texture2D.whiteTexture);
                Graphics.Blit(Texture2D.whiteTexture, target, material);
                if (ShaderUtil.ShaderHasError(shader)) throw new InvalidOperationException("Editable E63 failed draw-time compilation.");
            }
            finally
            {
                RenderTexture.active = previous;
                target.Release(); UnityEngine.Object.DestroyImmediate(target); UnityEngine.Object.DestroyImmediate(material);
            }
            Debug.Log("RenderMax E63 editable shader draw-time check completed: " + SystemInfo.graphicsDeviceType + ". Not a visual equivalence validation.");
        }

        public static void Run()
        {
            var shader = AssetDatabase.LoadAssetAtPath<Shader>("Assets/Phase2Test/Faithful.shader");
            if (shader == null || ShaderUtil.ShaderHasError(shader) || !shader.isSupported)
                throw new InvalidOperationException("Generated shader failed Unity import/support check.");
            var capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxSmoke/Capture.asset");
            if (capture == null) throw new InvalidOperationException("Missing isolated smoke Capture.asset.");
            var draw = capture.draws.Find(d => d.eventId == 63);
            if (draw == null) throw new InvalidOperationException("Missing event 63.");
            using (var replay = new DrawReplay())
            {
                // replay.Render(capture, draw, Color.clear, false, FilterMode.Bilinear, TextureWrapMode.Repeat, shader);
                // replay.Save("Assets/Phase2Test/Event63_Unity.png");
            }
            if (ShaderUtil.ShaderHasError(shader)) throw new InvalidOperationException("Generated shader failed lazy compilation.");
            var image = new Texture2D(2, 2, TextureFormat.RGBA32, false, true);
            try
            {
                image.LoadImage(File.ReadAllBytes("Assets/Phase2Test/Event63_Unity.png"));
                var comparison = SceneComparison.Compare(capture, draw, image);
                File.WriteAllText("Assets/Phase2Test/Comparison.json", JsonUtility.ToJson(comparison, true));
                Debug.Log("RenderMax GENERATED E63: Unity compilation, explicit matrix/ST/texture binding and GPU readback completed. Different pixels=" + comparison.differentPixels + "; max byte error=" + comparison.maxByteError + ". Blank reference is not conclusive shader validation.");
                if (comparison.differentPixels != 0) throw new InvalidOperationException("RGBA8 reference comparison failed. See Assets/Phase2Test/Comparison.json.");
            }
            finally { UnityEngine.Object.DestroyImmediate(image); }
        }
    }
}
