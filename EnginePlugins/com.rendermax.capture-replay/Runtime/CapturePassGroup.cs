using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // Metadata only. Turning this GameObject off excludes its descendant draw items.
    [DisallowMultipleComponent]
    public sealed class CapturePassGroup : MonoBehaviour
    {
        public string passId, passName;
        public uint markerEventId;
        public string[] markerPath;
        public uint[] markerEventIds;
        [Tooltip("Marker ancestry from the capture, not a guessed Shadow/GBuffer classification.")]
        public string nameSource;
    }
}
