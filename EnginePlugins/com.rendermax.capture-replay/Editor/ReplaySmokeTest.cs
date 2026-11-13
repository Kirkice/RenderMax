using System;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class ReplaySmokeTest
    {
        // Batch entry for isolated test projects; never imports into an existing project implicitly.
        public static void BatchImportAndRun()
        {
            string[] args = Environment.GetCommandLineArgs();
            int index = Array.IndexOf(args, "-renderMaxManifest");
            if (index < 0 || index + 1 >= args.Length) throw new ArgumentException("-renderMaxManifest is required.");
            var report = AssetPackageImporter.Import(args[index + 1], "Assets/RenderMaxSmoke");
            if (!report.integrityPassed || report.errors.Count != 0)
                throw new InvalidDataException("Batch import failed: " + string.Join("; ", report.errors.ToArray()));
            Selection.activeObject = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxSmoke/Capture.asset");
            Run();
        }

        public static void BatchCaptureSceneTest()
        {
            var capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxSmoke/Capture.asset");
            string original = capture.sourceManifest;
            try
            {
                // Full inventory test; this isolated project only imported event 63's mesh.
                capture.sourceManifest = "C:/Users/admin/Desktop/RenderMax_Unity_Test/Ying_Ultra.rmasset/manifest.json";
                string scene = CaptureSceneBuilder.Create(capture, false);
                UnityEditor.SceneManagement.EditorSceneManager.OpenScene(scene);
                var nodes = UnityEngine.Object.FindObjectsOfType<CaptureDrawState>();
                if (nodes.Length != 104) throw new InvalidOperationException("Expected all 104 event nodes, got " + nodes.Length);
                if (UnityEngine.Object.FindObjectsOfType<Light>().Length != 0) throw new InvalidOperationException("Unexpected light.");
                if (UnityEngine.Object.FindObjectOfType<CaptureReplayController>() == null) throw new InvalidOperationException("Missing controller.");
                int meshes = 0;
                foreach (var node in nodes)
                {
                    var renderer = node.GetComponent<MeshRenderer>(); if (renderer == null) continue;
                    ++meshes;
                    if (!renderer.enabled || renderer.sharedMaterial == null || renderer.sharedMaterial.shader.name != "Standard (Specular setup)")
                        throw new InvalidOperationException("Invalid Standard scene renderer: " + node.eventId);
                    var data = node.GetComponent<CaptureMaterialState>();
                    if (data == null || data.constantEvidence == null) throw new InvalidOperationException("Missing typed constant evidence: " + node.eventId);
                    data.Apply();
                }
                if (meshes != 104) throw new InvalidOperationException("Expected 104 mesh instances, got " + meshes);
                if (UnityEngine.Object.FindObjectOfType<CaptureReplayController>().orderedCommandBufferReplay)
                    throw new InvalidOperationException("Phase 1 must not use CommandBuffer.");
                Debug.Log("RenderMax PHASE 1 structural test PASSED: " + nodes.Length + " nodes; " + meshes + " enabled Standard Specular meshes with constant evidence; " + scene + ". Visual equivalence not tested.");
            }
            finally { capture.sourceManifest = original; }
        }

        public static void BatchSceneTest()
        {
            var capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxSmoke/Capture.asset");
            if (capture == null) throw new InvalidOperationException("Run BatchImportAndRun first.");
            var draw = capture.draws.Find(d => d.eventId == 63);
            string path = SceneRebuilder.Create(capture, draw, false);
            UnityEditor.SceneManagement.EditorSceneManager.OpenScene(path);
            var globals = UnityEngine.Object.FindObjectOfType<GlobalConstantBufferDriver>();
            if (globals == null || UnityEngine.Object.FindObjectsOfType<Light>().Length != 0)
                throw new InvalidOperationException("Scene global-driver/no-light assertion failed.");
            globals.Apply();
            var camera = Camera.main;
            var rt = new RenderTexture(1024, 1024, 24);
            var previous = RenderTexture.active;
            var image = new Texture2D(1024, 1024, TextureFormat.RGBA32, false);
            try
            {
                camera.targetTexture = rt; camera.Render(); RenderTexture.active = rt;
                image.ReadPixels(new Rect(0, 0, 1024, 1024), 0, 0); image.Apply();
                File.WriteAllBytes(Path.GetDirectoryName(path) + "/SceneRender.png", image.EncodeToPNG());
                var comparison = SceneComparison.Compare(capture, draw, image);
                File.WriteAllText(Path.GetDirectoryName(path) + "/Comparison.json", JsonUtility.ToJson(comparison, true));
                if (comparison.differentPixels != 0)
                    throw new InvalidOperationException("Scene readback differs from captured color target. See Comparison.json.");
            }
            finally
            {
                camera.targetTexture = null; RenderTexture.active = previous;
                rt.Release(); UnityEngine.Object.DestroyImmediate(rt); UnityEngine.Object.DestroyImmediate(image);
            }
            Debug.Log("RenderMax scene save/reopen/render test PASSED: " + path);
        }

        [MenuItem("Tools/RenderMax/Advanced/Tests/Test Event 63 Reconstruction")]
        public static void Run()
        {
            var capture = Selection.activeObject as CaptureAsset;
            if (capture == null)
            {
                foreach (string guid in AssetDatabase.FindAssets("t:CaptureAsset"))
                {
                    capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(AssetDatabase.GUIDToAssetPath(guid));
                    if (capture != null) break;
                }
            }
            if (capture == null) throw new InvalidOperationException("Import a Capture.asset first.");
            var draw = capture.draws.Find(d => d.eventId == 63);
            if (draw == null) throw new InvalidOperationException("Event 63 not present.");
            string folder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)) + "/Reconstruction";
            Directory.CreateDirectory(folder);
            using (var replay = new DrawReplay())
            {
                replay.Render(capture, draw, Color.clear, false, FilterMode.Bilinear, TextureWrapMode.Repeat);
                replay.Save(folder + "/Event63_Unity.png");
                File.WriteAllText(folder + "/Event63_Report.txt", replay.Diagnostics + "\nBackend: " + SystemInfo.graphicsDeviceType +
                    "\nGPU: " + SystemInfo.graphicsDeviceName + "\nUnity: " + Application.unityVersion);
            }
            AssetDatabase.Refresh();
            Debug.Log("RenderMax event 63 reconstruction executed: " + folder + ". Not a pixel-equivalence test.");
        }
    }
}
