using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

namespace RenderMax.CaptureReplay
{
    // First verified source pair. Unknown shaders must never silently use this shader.
    public sealed class DrawReplay : IDisposable
    {
        public RenderTexture Output { get; private set; }
        public string Diagnostics { get; private set; }
        Material material;
        Texture2D sampled;
        Mesh ownedMesh;
        static string Hash(string text)
        {
            using (var h = SHA256.Create())
                return BitConverter.ToString(h.ComputeHash(Encoding.UTF8.GetBytes(text.Replace("\r\n", "\n")))).Replace("-", "").ToLowerInvariant();
        }
        static ConstantVariable Find(ConstantVariable[] variables, string name)
        {
            foreach (var v in variables ?? new ConstantVariable[0])
            {
                if (v.name == name) return v;
                var found = Find(v.members, name); if (found != null) return found;
            }
            return null;
        }
        static ConstantVariable Constant(ConstantDocument doc, string stage, string name)
        {
            foreach (var block in doc.blocks ?? new ConstantBlock[0])
            {
                if (block.stage != stage) continue;
                var found = Find(block.variables, name); if (found != null) return found;
            }
            throw new InvalidDataException("Missing constant: " + stage + " / " + name);
        }
        static Vector4 Vector(ConstantVariable v)
        {
            if (v.value == null || v.value.Length != 4) throw new InvalidDataException("Expected float4: " + v.name);
            return new Vector4(v.value[0], v.value[1], v.value[2], v.value[3]);
        }
        static Vector4[] Columns(ConstantVariable v)
        {
            if (v.members == null || v.members.Length != 4) throw new InvalidDataException("Expected four captured columns: " + v.name);
            return new[] { Vector(v.members[0]), Vector(v.members[1]), Vector(v.members[2]), Vector(v.members[3]) };
        }
        static CompareFunction Compare(string name)
        {
            switch (name)
            {
                case "Never": return CompareFunction.Never;
                case "Less": return CompareFunction.Less;
                case "Equal": return CompareFunction.Equal;
                case "Less Equal": return CompareFunction.LessEqual;
                case "Greater": return CompareFunction.Greater;
                case "Not Equal": return CompareFunction.NotEqual;
                case "Greater Equal": return CompareFunction.GreaterEqual;
                case "Always": return CompareFunction.Always;
                default: throw new NotSupportedException("Depth comparison: " + name);
            }
        }
        public void Render(CaptureAsset capture, ImportedDraw imported, Color clear, bool flipY, FilterMode filter, TextureWrapMode wrap)
        {
            Dispose();
            try
            {
                var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
                var draw = reader.Read<ReplayDraw>(imported.sourceDraw);
                if (draw.shaders == null || draw.shaders.Vertex == null || draw.shaders.Pixel == null)
                    throw new NotSupportedException("This draw has no supported vertex/pixel shader pair.");
                var vs = reader.Read<ShaderDocument>(draw.shaders.Vertex);
                var ps = reader.Read<ShaderDocument>(draw.shaders.Pixel);
                if (vs.source == null || ps.source == null ||
                    Hash(vs.source) != "813734f62a029010e79aa9c9d98846a23a746049dbd9b364c73f712514ee5f9f" ||
                    Hash(ps.source) != "611c1f3e7a8426b48e2f7f8c2b73cd48af194ef685d5df64b15993fc74b7dcb2")
                    throw new NotSupportedException("Shader pair not translated yet. Only the source-verified captured albedo pass is currently supported (e.g. event 63).");
                var pipeline = reader.Read<ReplayPipeline>(draw.pipeline);
                if (pipeline.rasterizer == null || pipeline.depthTest == null || pipeline.viewports == null || pipeline.viewports.Length == 0)
                    throw new InvalidDataException("Missing raster/depth/viewport state.");
                if (pipeline.rasterizer.fillMode != "Solid" || pipeline.depthTest.depthBounds)
                    throw new NotSupportedException("Wireframe or depth bounds not implemented.");
                var blend = pipeline.colorBlends != null && pipeline.colorBlends.Length > 0 ? pipeline.colorBlends[0] : null;
                if (blend == null || blend.enabled || blend.logicOperationEnabled)
                    throw new NotSupportedException("Blended or logic-op draw requires initial destination state; not supported in isolated preview.");
                var viewport = pipeline.viewports[0];
                if (viewport.minDepth != 0 || viewport.maxDepth != 1 || viewport.x != 0 || viewport.y != 0)
                    throw new NotSupportedException("Only zero-origin full depth-range viewport supported in this first pass.");
                int width = checked((int)viewport.width), height = checked((int)viewport.height);
                if (width < 1 || height < 1 || width > 8192 || height > 8192) throw new NotSupportedException("Preview size outside 1..8192.");
                string resource = null;
                foreach (var b in reader.Read<BindingDocument>(draw.bindings).descriptors ?? new Binding[0])
                    if (b.stage == "Pixel" && b.reflectionIndex == 0 && b.arrayElement == 0 && b.resourceId != "ResourceId::0")
                    {
                        if (resource != null && resource != b.resourceId) throw new InvalidDataException("Ambiguous sampled resource.");
                        resource = b.resourceId;
                    }
                TextureDescription texture = null;
                foreach (var reference in draw.textures ?? new ResourceReference[0])
                    if (reference.resourceId == resource) texture = reader.Read<TextureDescription>(reference.path);
                if (texture == null || texture.cubemap || texture.depth > 1 || texture.arraySize > 1 || texture.samples > 1)
                    throw new NotSupportedException("Expected a bound ordinary 2D texture.");
                // Assemble all exported mip images into one explicit uncompressed GPU texture.
                sampled = new Texture2D(texture.width, texture.height, TextureFormat.RGBA32, texture.mips > 1, !texture.srgb);
                sampled.name = "Captured " + resource; sampled.hideFlags = HideFlags.HideAndDontSave;
                if (sampled.mipmapCount != texture.mips) throw new NotSupportedException("Partial mip chain not implemented.");
                for (int mip = 0; mip < texture.mips; ++mip)
                {
                    TextureImage image = null;
                    foreach (var candidate in texture.images ?? new TextureImage[0])
                        if (candidate.layer == 0 && candidate.mip == mip) image = candidate;
                    if (image == null) throw new InvalidDataException("Missing captured mip " + mip);
                    string path = reader.FilePath(image.path);
                    // Unity's asset importer decodes TGA; LoadImage itself only handles PNG/JPG.
                    string assetPath = Path.GetDirectoryName(capture.sourceManifest).Replace('\\', '/');
                    assetPath = assetPath.Substring(0, assetPath.Length - "Evidence~".Length) + "Textures/" + image.path;
                    var decoded = AssetDatabase.LoadAssetAtPath<Texture2D>(assetPath);
                    if (decoded == null || !decoded.isReadable) throw new InvalidDataException("Readable imported texture not found: " + assetPath);
                    if (decoded.width != Math.Max(1, texture.width >> mip) || decoded.height != Math.Max(1, texture.height >> mip))
                        throw new InvalidDataException("Mip dimension mismatch.");
                    sampled.SetPixels32(decoded.GetPixels32(0), mip);
                }
                sampled.Apply(false, false); sampled.filterMode = filter; sampled.wrapMode = wrap;
                var shader = Shader.Find("Hidden/RenderMax/CapturedAlbedo");
                if (shader == null || !shader.isSupported) throw new InvalidOperationException("Replay shader unavailable or unsupported.");
                material = new Material(shader) { hideFlags = HideFlags.HideAndDontSave };
                var constants = reader.Read<ConstantDocument>(draw.constants);
                material.SetVectorArray("_RMObject", Columns(Constant(constants, "Vertex", "hlslcc_mtx4x4unity_ObjectToWorld")));
                material.SetVectorArray("_RMVP", Columns(Constant(constants, "Vertex", "hlslcc_mtx4x4unity_MatrixVP")));
                material.SetVector("_RMST", Vector(Constant(constants, "Pixel", "_AlbedoMap_ST")));
                material.SetTexture("_RMTexture", sampled); material.SetFloat("_RMFlipY", flipY ? -1 : 1);
                // D3D's screen-space winding convention differs from GL's lower-left convention.
                bool ccw = pipeline.rasterizer.frontCCW;
                if (SystemInfo.graphicsUVStartsAtTop) ccw = !ccw;
                if (flipY) ccw = !ccw;
                CullMode cull;
                switch (pipeline.rasterizer.cullMode)
                {
                    case "No Cull": case "None": cull = CullMode.Off; break;
                    case "Back": cull = ccw ? CullMode.Front : CullMode.Back; break;
                    case "Front": cull = ccw ? CullMode.Back : CullMode.Front; break;
                    default: throw new NotSupportedException("Cull mode: " + pipeline.rasterizer.cullMode);
                }
                material.SetInt("_RMCull", (int)cull);
                material.SetInt("_RMZTest", (int)(pipeline.depthTest.depthEnable ? Compare(pipeline.depthTest.depthFunction) : CompareFunction.Always));
                material.SetInt("_RMZWrite", pipeline.depthTest.depthEnable && pipeline.depthTest.depthWrites ? 1 : 0);
                material.SetInt("_RMColorMask", blend.writeMask);
                Mesh mesh = imported.mesh;
                if (mesh == null)
                {
                    ownedMesh = MeshImporter.Build(reader, reader.Read<MeshDescription>(draw.mesh), new List<string>());
                    ownedMesh.hideFlags = HideFlags.HideAndDontSave; mesh = ownedMesh;
                }
                var meshDesc = reader.Read<MeshDescription>(draw.mesh);
                if (meshDesc.instanceCount != 1) throw new NotSupportedException("Instanced replay not implemented.");
                Output = new RenderTexture(width, height, 24, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Linear)
                { name = "RenderMax isolated draw " + draw.eventId, hideFlags = HideFlags.HideAndDontSave, antiAliasing = 1 };
                Output.Create();
                var cmd = new CommandBuffer { name = "RenderMax isolated Draw " + draw.eventId };
                try
                {
                    cmd.SetRenderTarget(Output);
                    cmd.ClearRenderTarget(true, true, clear, 1);
                    cmd.SetViewport(new Rect(0, 0, width, height));
                    if (pipeline.scissors != null && pipeline.scissors.Length > 0 && pipeline.scissors[0].enabled)
                    {
                        var s = pipeline.scissors[0];
                        cmd.EnableScissorRect(new Rect(s.x, SystemInfo.graphicsUVStartsAtTop ? height - s.y - s.height : s.y, s.width, s.height));
                    }
                    cmd.DrawMesh(mesh, Matrix4x4.identity, material, 0, 0);
                    cmd.DisableScissorRect(); Graphics.ExecuteCommandBuffer(cmd);
                }
                finally { cmd.Release(); }
                Diagnostics = "Reconstructed source-verified albedo pass, event " + draw.eventId + "\n" +
                    "Captured ObjectToWorld / VP / ST and sampled texture bound.\n" +
                    "Isolated draw: cleared RGBA8 + depth24 target; no captured pre-draw depth/stencil.\n" +
                    "Sampler is a manual override (" + filter + ", " + wrap + "), not recovered capture state.\n" +
                    "GL-to-backend depth/winding mapping and image orientation require reference comparison.\n" +
                    "No full-frame or pixel-equivalence claim.";
            }
            catch { Dispose(); throw; }
        }
        public void Save(string path)
        {
            if (Output == null) throw new InvalidOperationException("Render a draw first.");
            var previous = RenderTexture.active;
            var image = new Texture2D(Output.width, Output.height, TextureFormat.RGBA32, false, true);
            try
            {
                RenderTexture.active = Output;
                image.ReadPixels(new Rect(0, 0, Output.width, Output.height), 0, 0); image.Apply();
                File.WriteAllBytes(path, image.EncodeToPNG());
            }
            finally { RenderTexture.active = previous; UnityEngine.Object.DestroyImmediate(image); }
        }
        public void Dispose()
        {
            if (Output != null) { Output.Release(); UnityEngine.Object.DestroyImmediate(Output); Output = null; }
            if (material != null) UnityEngine.Object.DestroyImmediate(material);
            if (sampled != null) UnityEngine.Object.DestroyImmediate(sampled);
            if (ownedMesh != null) UnityEngine.Object.DestroyImmediate(ownedMesh);
        }
    }
}
