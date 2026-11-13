using System;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    // A local, explicit request file allows the workflow to run inside the active
    // Editor without starting a second Unity process or discarding open edits.
    [InitializeOnLoad]
    public static class CapturePipelineRunner
    {
        [Serializable] public class Request { public string manifest, destination; public bool upgradeOnly; }
        [Serializable] public class Result
        {
            public string status, scene, materialUpgrade, error;
            public int imported, errors, unsupported;
        }
        const string RequestPath = "Artifacts/RenderMaxPipelineRequest.json";
        const string ResultPath = "Artifacts/RenderMaxPipelineResult.json";
        static double nextCheck;
        static CapturePipelineRunner() { EditorApplication.update += Poll; }
        static void Poll()
        {
            if (EditorApplication.timeSinceStartup < nextCheck) return;
            nextCheck = EditorApplication.timeSinceStartup + 3;
            if (EditorApplication.isCompiling || EditorApplication.isUpdating || EditorApplication.isPlayingOrWillChangePlaymode || !File.Exists(RequestPath)) return;
            // Leave the request pending until the user saves their open edits.
            for (int i = 0; i < SceneManager.sceneCount; ++i)
                if (SceneManager.GetSceneAt(i).isDirty) return;
            var request = JsonUtility.FromJson<Request>(File.ReadAllText(RequestPath));
            File.Move(RequestPath, RequestPath + ".running");
            var result = new Result { status = "running" };
            try
            {
                File.WriteAllText(ResultPath, JsonUtility.ToJson(result, true));
                if (!request.upgradeOnly)
                {
                    var report = AssetPackageImporter.Import(request.manifest, request.destination);
                    result.imported = report.imported.Count;
                    result.errors = report.errors.Count;
                    result.unsupported = report.unsupported.Count;
                    if (result.errors != 0) throw new InvalidOperationException("Import has errors; inspect ImportReport.json.");
                    var capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(request.destination + "/Capture.asset");
                    result.scene = CaptureSceneBuilder.Create(capture, false);
                }
                else result.scene = request.destination + "/CaptureReplay/CaptureReplay.unity";
                // NewScene/Refresh may unload the unreferenced ScriptableObject.
                var reloadedCapture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(request.destination + "/Capture.asset");
                result.materialUpgrade = GeneratedMaterialUpgrade.Replace(reloadedCapture);
                result.status = "completedWithLimitations";
                Debug.Log("RenderMax pipeline completed: " + result.scene);
            }
            catch (Exception e) { result.status = "failed"; result.error = e.ToString(); Debug.LogException(e); }
            finally
            {
                EditorUtility.ClearProgressBar();
                File.WriteAllText(ResultPath, JsonUtility.ToJson(result, true));
                File.Move(RequestPath + ".running", RequestPath + "." + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".done");
            }
        }
    }
}
