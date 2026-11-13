using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    internal static class CapturedTextureBuilder
    {
        internal static Texture Load(PackageReader reader, TextureDescription meta, string reference, string folder)
        {
            if (!meta.cubemap && meta.format != "D16")
            {
                if (meta.arraySize > 1 || meta.depth > 1 || meta.samples > 1) return null;
                var image = (meta.images ?? new TextureImage[0]).FirstOrDefault(x => x.layer == 0 && x.mip == 0);
                return image == null ? null : AssetDatabase.LoadAssetAtPath<Texture>(folder + "/Textures/" + image.path);
            }
            string asset = folder + "/Textures/" + Path.GetDirectoryName(reference).Replace('\\', '/') + "/CapturedTexture.asset";
            var existing = AssetDatabase.LoadAssetAtPath<Texture>(asset);
            if (existing != null) return existing;
            if (meta.depth != 1 || meta.samples != 1) throw new InvalidDataException("Unsupported texture dimensions: " + reference);
            Texture result;
            if (meta.cubemap)
            {
                if (meta.format != "R8G8B8A8_UNORM" || meta.width != meta.height || meta.arraySize != 6)
                    throw new InvalidDataException("Unsupported cube format: " + reference);
                var cube = new Cubemap(meta.width, TextureFormat.RGBA32, meta.mips > 1);
                if (cube.mipmapCount != meta.mips) throw new InvalidDataException("Incomplete captured cube mip chain: " + reference);
                for (int mip = 0; mip < meta.mips; ++mip)
                for (int face = 0; face < 6; ++face)
                {
                    int size = Math.Max(1, meta.width >> mip);
                    var bytes = Raw(reader, meta, mip, face, size * size * 4);
                    var colors = new Color[size * size];
                    // GL and Unity cube faces share +X,-X,+Y,-Y,+Z,-Z ordering.
                    // Retain native face rows; do not apply the 2D PNG export flip.
                    for (int i = 0; i < colors.Length; ++i)
                        colors[i] = new Color32(bytes[4*i], bytes[4*i+1], bytes[4*i+2], bytes[4*i+3]);
                    cube.SetPixels(colors, (CubemapFace)face, mip);
                }
                cube.Apply(false, false); result = cube;
            }
            else
            {
                var bytes = Raw(reader, meta, 0, 0, meta.width * meta.height * 2);
                var values = new byte[meta.width * meta.height * 4];
                for (int i = 0; i < meta.width * meta.height; ++i)
                {
                    float depth = (bytes[2*i] | (bytes[2*i+1] << 8)) / 65535f;
                    Buffer.BlockCopy(BitConverter.GetBytes(depth), 0, values, 4*i, 4);
                }
                var texture = new Texture2D(meta.width, meta.height, TextureFormat.RFloat, false, true);
                texture.LoadRawTextureData(values); texture.Apply(false, false); result = texture;
            }
            result.name = "Captured " + meta.resourceId + " E" + meta.eventId;
            result.wrapMode = TextureWrapMode.Clamp;
            result.filterMode = meta.cubemap ? FilterMode.Trilinear : FilterMode.Point;
            Directory.CreateDirectory(Path.GetDirectoryName(asset));
            AssetDatabase.CreateAsset(result, asset);
            return result;
        }
        static byte[] Raw(PackageReader reader, TextureDescription meta, int mip, int layer, int expected)
        {
            var sub = (meta.rawSubresources ?? new RawTextureSubresource[0]).Single(x => x.mip == mip && x.layer == layer && x.sample == 0);
            var bytes = File.ReadAllBytes(reader.FilePath(sub.path));
            if (bytes.Length != expected || sub.bytes != expected) throw new InvalidDataException("Texture byte count mismatch: " + sub.path);
            return bytes;
        }
    }
}
