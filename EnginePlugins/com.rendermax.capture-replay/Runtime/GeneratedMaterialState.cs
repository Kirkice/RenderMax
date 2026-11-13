using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [ExecuteAlways, DisallowMultipleComponent]
    public sealed class GeneratedMaterialState : MonoBehaviour
    {
        public GeneratedMaterialBindings[] bindings;
        void OnEnable() { Apply(); }
        void OnValidate() { Apply(); }
        public void Apply()
        {
            foreach (var data in bindings ?? new GeneratedMaterialBindings[0])
                if (data != null) data.Apply();
        }
    }
}
