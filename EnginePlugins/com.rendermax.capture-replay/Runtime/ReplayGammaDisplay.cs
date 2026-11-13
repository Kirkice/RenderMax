using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // RenderMax texture-viewer display override, NOT a project color-space switch.
    // SceneView is handled ONLY by ReplayGammaSceneBridge. Do not add
    // ImageEffectAllowedInSceneView: Unity would also copy the Game camera effect.
    [ExecuteAlways, RequireComponent(typeof(Camera))]
    public sealed class ReplayGammaDisplay : MonoBehaviour
    {
        public bool linearDisplayAsGamma;
        public Shader displayShader;
        [System.NonSerialized] public bool sceneViewBridgeOwned;
        Material displayMaterial;
        void OnRenderImage(RenderTexture source, RenderTexture destination)
        {
            var camera = GetComponent<Camera>();
            if (camera.cameraType == CameraType.SceneView)
            {
                // Ignore automatic/stale Unity copies. Only the explicit bridge
                // instance is allowed to process SceneView color.
                ReplayGammaDisplay owner = null;
                foreach (var effect in GetComponents<ReplayGammaDisplay>())
                    if (effect.enabled && effect.sceneViewBridgeOwned) { owner = effect; break; }
                if (!sceneViewBridgeOwned || owner != this)
                { Graphics.Blit(source, destination); return; }
            }
            if (displayShader == null) displayShader = Shader.Find("Hidden/RenderMax/ReplayGammaDisplay");
            if (displayShader == null || !displayShader.isSupported) { Graphics.Blit(source,destination); return; }
            if (displayMaterial == null) displayMaterial = new Material(displayShader) { hideFlags = HideFlags.HideAndDontSave };
            displayMaterial.SetFloat("_DisplayAsGamma",linearDisplayAsGamma ? 1 : 0);
            displayMaterial.SetFloat("_LinearProject",QualitySettings.activeColorSpace == ColorSpace.Linear ? 1 : 0);
            bool old = GL.sRGBWrite;
            try
            {
                GL.sRGBWrite = QualitySettings.activeColorSpace == ColorSpace.Linear;
                Graphics.Blit(source,destination,displayMaterial);
            }
            finally { GL.sRGBWrite = old; }
        }
        void OnDisable()
        {
            if (displayMaterial == null) return;
            if (Application.isPlaying) Destroy(displayMaterial); else DestroyImmediate(displayMaterial);
            displayMaterial = null;
        }
    }
}
