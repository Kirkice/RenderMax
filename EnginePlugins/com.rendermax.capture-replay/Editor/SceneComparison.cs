using System;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class ColorComparison
    {
        public uint eventId;
        public int pixels, differentPixels, maxByteError;
        public double meanByteError;
        public string reference, limitation = "RGBA8 preview comparison only. A blank reference match does not validate visible geometry, shader translation, camera pose, depth or whole-frame replay.";
    }
    [Serializable] public class TargetResource { public string exactId; }
    [Serializable] public class ColorTarget { public TargetResource resource; }
    [Serializable] public class TargetPipeline { public ColorTarget[] outputTargets; }

    public static class SceneComparison
    {
        public static ColorComparison Compare(CaptureAsset capture, ImportedDraw imported, Texture2D image)
        {
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            var draw = reader.Read<ReplayDraw>(imported.sourceDraw);
            var pipeline = reader.Read<TargetPipeline>(draw.pipeline);
            if (pipeline.outputTargets == null || pipeline.outputTargets.Length == 0)
                throw new NotSupportedException("No color target to compare.");
            string id = "ResourceId::" + pipeline.outputTargets[0].resource.exactId;
            string folder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\', '/');
            foreach (var resource in draw.textures ?? new ResourceReference[0])
            {
                if (resource.resourceId != id) continue;
                var metadata = reader.Read<TextureDescription>(resource.path);
                foreach (var mip in metadata.images ?? new TextureImage[0])
                {
                    if (mip.mip != 0 || mip.layer != 0) continue;
                    var reference = AssetDatabase.LoadAssetAtPath<Texture2D>(folder + "/Textures/" + mip.path);
                    if (reference == null || reference.width != image.width || reference.height != image.height)
                        throw new InvalidDataException("Missing or mismatched reference preview.");
                    var a = image.GetPixels32(); var b = reference.GetPixels32();
                    var report = new ColorComparison { eventId = imported.eventId, pixels = a.Length, reference = mip.path };
                    long sum = 0;
                    for (int i = 0; i < a.Length; ++i)
                    {
                        int r = Math.Abs(a[i].r - b[i].r), g = Math.Abs(a[i].g - b[i].g), bl = Math.Abs(a[i].b - b[i].b), alpha = Math.Abs(a[i].a - b[i].a);
                        int max = Math.Max(Math.Max(r,g), Math.Max(bl,alpha));
                        if (max != 0) ++report.differentPixels;
                        report.maxByteError = Math.Max(report.maxByteError, max); sum += r + g + bl + alpha;
                    }
                    report.meanByteError = (double)sum / (a.Length * 4);
                    return report;
                }
            }
            throw new NotSupportedException("Captured color target has no comparable imported preview.");
        }
    }
}
