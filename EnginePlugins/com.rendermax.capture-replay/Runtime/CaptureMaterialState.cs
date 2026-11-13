using System;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // Original JSON preserves types/stages/arrays that are not representable by Standard.
    [ExecuteAlways]
    public sealed class CaptureMaterialState : MonoBehaviour
    {
        public TextAsset constantEvidence;
        public uint eventId;
        [TextArea] public string mappingNotes;
        public GlobalConstantBufferDriver.VectorConstant[] capturedFloats;
        [Serializable] public class Property { public string name; public Vector4 value; public bool scalar; }
        public Property[] standardProperties;
        void OnEnable() { Apply(); }
        void OnValidate() { Apply(); }
        public void Apply()
        {
            // A migrated renderer owns typed generated-shader bindings, not Standard overrides.
            if (!enabled || GetComponent<GeneratedMaterialState>() != null) return;
            var renderer = GetComponent<Renderer>(); if (renderer == null) return;
            var block = new MaterialPropertyBlock();
            foreach (var v in capturedFloats ?? new GlobalConstantBufferDriver.VectorConstant[0])
            {
                if (v.values == null || v.values.Length == 0 || string.IsNullOrEmpty(v.name)) continue;
                // Unity owns these live scene transforms/time values. Never replace them with capture values.
                if (v.name.StartsWith("unity_", StringComparison.Ordinal) || v.name.StartsWith("hlslcc_", StringComparison.Ordinal) ||
                    v.name == "_Time" || v.name == "_WorldSpaceCameraPos" || v.name == "_ProjectionParams") continue;
                if (v.array) block.SetVectorArray(v.name, v.values);
                else block.SetVector(v.name, v.values[0]);
            }
            foreach (var p in standardProperties ?? new Property[0])
            {
                if (p.scalar) block.SetFloat(p.name, p.value.x); else block.SetVector(p.name, p.value);
            }
            renderer.SetPropertyBlock(block);
        }
    }
}
