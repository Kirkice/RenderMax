using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [ExecuteAlways]
    public sealed class GlobalConstantBufferDriver : MonoBehaviour
    {
        [System.Serializable]
        public class VectorConstant
        {
            public string name;
            public Vector4[] values;
            public bool array;
        }
        public VectorConstant[] floatConstants = new VectorConstant[0];
        public Vector4[] objectToWorld = new Vector4[4];
        public Vector4[] viewProjection = new Vector4[4];
        public Vector4 textureST = new Vector4(1, 1, 0, 0);

        void OnEnable() { Apply(); }
        void Update() { Apply(); }
        public void Apply()
        {
            foreach (var entry in floatConstants ?? new VectorConstant[0])
            {
                if (entry == null || string.IsNullOrEmpty(entry.name) || entry.values == null || entry.values.Length == 0) continue;
                if (entry.array) Shader.SetGlobalVectorArray(entry.name, entry.values);
                else Shader.SetGlobalVector(entry.name, entry.values[0]);
            }
            if (objectToWorld != null && objectToWorld.Length == 4)
            {
                Shader.SetGlobalVectorArray("_RMObject", objectToWorld);
                Shader.SetGlobalVectorArray("hlslcc_mtx4x4unity_ObjectToWorld", objectToWorld);
            }
            if (viewProjection != null && viewProjection.Length == 4)
            {
                Shader.SetGlobalVectorArray("_RMVP", viewProjection);
                Shader.SetGlobalVectorArray("hlslcc_mtx4x4unity_MatrixVP", viewProjection);
            }
            Shader.SetGlobalVector("_RMST", textureST);
            Shader.SetGlobalVector("_AlbedoMap_ST", textureST);
        }
    }
}
