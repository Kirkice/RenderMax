using System;
using System.Collections.Generic;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class ImportedDraw
    {
        public uint eventId;
        public Mesh mesh;
        public List<Texture> texturePreviews = new List<Texture>();
        public string sourceDraw;
    }

    public sealed class CaptureAsset : ScriptableObject
    {
        public string sourceManifest;
        public string status = "Evidence imported; GPU replay not implemented";
        public List<ImportedDraw> draws = new List<ImportedDraw>();
    }
}
