using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class ReplayGammaMenu
    {
        [MenuItem("Tools/RenderMax/Advanced/Display/Linear Data As Gamma - On")]
        static void On() { Set(true); }
        [MenuItem("Tools/RenderMax/Advanced/Display/Linear Data As Gamma - Off")]
        static void Off() { Set(false); }
        static void Set(bool enabled)
        {
            var controllers = Object.FindObjectsOfType<CaptureReplayController>();
            if (controllers.Length != 1) { Debug.LogError("Open exactly one capture controller scene."); return; }
            var effect = controllers[0].GetComponent<ReplayGammaDisplay>();
            if (effect == null) effect = Undo.AddComponent<ReplayGammaDisplay>(controllers[0].gameObject);
            Undo.RecordObject(effect,"RenderMax display gamma");
            effect.displayShader = Shader.Find("Hidden/RenderMax/ReplayGammaDisplay");
            effect.linearDisplayAsGamma = enabled;
            EditorUtility.SetDirty(effect);
            EditorSceneManager.MarkSceneDirty(effect.gameObject.scene);
            SceneView.RepaintAll();
            Debug.Log("RenderMax display override " + (enabled ? "ON" : "OFF") + ". SceneView requires Effects/Post Processing enabled. Assumes non-sRGB source values; not a color-space repair.", effect);
        }
    }

    [InitializeOnLoad]
    internal static class ReplayGammaSceneBridge
    {
        static readonly System.Collections.Generic.Dictionary<Camera, ReplayGammaDisplay> effects = new System.Collections.Generic.Dictionary<Camera, ReplayGammaDisplay>();
        static ReplayGammaSceneBridge()
        {
            Camera.onPreCull += Sync;
            AssemblyReloadEvents.beforeAssemblyReload += Cleanup;
            EditorApplication.quitting += Cleanup;
            EditorApplication.playModeStateChanged += state => Cleanup();
            EditorApplication.update += Prune;
        }
        static void Sync(Camera camera)
        {
            if (camera == null || camera.cameraType != CameraType.SceneView) return;
            ReplayGammaDisplay source = null;
            foreach (var controller in Object.FindObjectsOfType<CaptureReplayController>())
            {
                if (!controller.isActiveAndEnabled) continue;
                var candidate = controller.GetComponent<ReplayGammaDisplay>();
                if (candidate == null || !candidate.isActiveAndEnabled) continue;
                if (source != null) { Remove(camera); return; }
                source = candidate;
            }
            if (source == null) { Remove(camera); return; }
            ReplayGammaDisplay effect;
            if (!effects.TryGetValue(camera,out effect) || effect == null)
            {
                effect = camera.gameObject.AddComponent<ReplayGammaDisplay>();
                effect.hideFlags = HideFlags.HideAndDontSave;
                effects[camera] = effect;
            }
            effect.sceneViewBridgeOwned = true;
            effect.displayShader = source.displayShader;
            effect.linearDisplayAsGamma = source.linearDisplayAsGamma;
        }
        static void Remove(Camera camera)
        {
            ReplayGammaDisplay effect;
            if (!effects.TryGetValue(camera,out effect)) return;
            if (effect != null) Object.DestroyImmediate(effect);
            effects.Remove(camera);
        }
        static void Prune()
        {
            var dead = new System.Collections.Generic.List<Camera>();
            foreach (var pair in effects) if (pair.Key == null || pair.Value == null) dead.Add(pair.Key);
            foreach (var camera in dead) Remove(camera);
        }
        static void Cleanup()
        {
            foreach (var pair in effects) if (pair.Value != null) Object.DestroyImmediate(pair.Value);
            effects.Clear();
        }
    }
}
