using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // Material.SetVectorArray is not serialized by Unity. Keep the captured arrays as a
    // material sub-asset and reapply them after scene/domain reload (including in players).
    public sealed class GeneratedMaterialBindings : ScriptableObject
    {
        public Material material;
        public uint eventId;
        public GlobalConstantBufferDriver.VectorConstant[] arrays;

        void OnEnable() { Apply(); }

        public void Apply()
        {
            if (material == null || material.shader == null ||
                !material.shader.name.StartsWith("RenderMax/Phase2/Editable/", System.StringComparison.Ordinal)) return;
            foreach (var entry in arrays ?? new GlobalConstantBufferDriver.VectorConstant[0])
                if (entry != null && !string.IsNullOrEmpty(entry.name) && entry.values != null && entry.values.Length > 0)
                    material.SetVectorArray(entry.name, entry.values);
        }
    }
}
