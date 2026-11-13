using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [ExecuteAlways, RequireComponent(typeof(Camera))]
    public sealed class CapturedCamera : MonoBehaviour
    {
        public bool lockCapturedView = true;
        public bool conservativeCulling = true;
        public Matrix4x4 view = Matrix4x4.identity;
        public Matrix4x4 projection = Matrix4x4.identity;
        public int width = 1024, height = 1024;
        void OnEnable() { Apply(); }
        void OnPreCull() { Apply(); }
        public void Apply()
        {
            var camera = GetComponent<Camera>();
            if (lockCapturedView) camera.worldToCameraMatrix = view;
            else camera.ResetWorldToCameraMatrix();
            camera.projectionMatrix = projection;
            camera.aspect = (float)width / height;
            // Shader-defined clip transforms still require conservative CPU culling.
            if (conservativeCulling) camera.cullingMatrix = Matrix4x4.Ortho(-100000, 100000, -100000, 100000, -100000, 100000);
            else camera.ResetCullingMatrix();
            var globals = FindObjectOfType<GlobalConstantBufferDriver>();
            if (globals != null) globals.Apply();
        }
        void OnDisable()
        {
            var camera = GetComponent<Camera>();
            if (camera != null) { camera.ResetWorldToCameraMatrix(); camera.ResetProjectionMatrix(); camera.ResetCullingMatrix(); }
        }
    }
}
