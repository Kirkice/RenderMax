using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [ExecuteAlways]
    public sealed class CaptureDrawState : MonoBehaviour
    {
        public uint eventId;
        public string status, sourceDraw;
        public bool translated;
        public GlobalConstantBufferDriver.VectorConstant[] constants;
        public Vector4[] objectColumns, vpColumns;
        public Vector4 textureST;
        void OnEnable() { Apply(); }
        void OnValidate() { Apply(); }
        public void Apply()
        {
            var renderer = GetComponent<MeshRenderer>();
            if (renderer == null || !translated) return;
            var properties = new MaterialPropertyBlock();
            foreach (var entry in constants ?? new GlobalConstantBufferDriver.VectorConstant[0])
            {
                if (entry.values == null || entry.values.Length == 0) continue;
                if (entry.array) properties.SetVectorArray(entry.name, entry.values);
                else properties.SetVector(entry.name, entry.values[0]);
            }
            properties.SetVectorArray("_RMObject", objectColumns);
            properties.SetVectorArray("_RMVP", vpColumns);
            properties.SetVector("_RMST", textureST);
            properties.SetFloat("_RMFlipY", 1);
            renderer.SetPropertyBlock(properties);
        }
    }
}
