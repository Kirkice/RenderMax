using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class ReflectedTexture { public string name; public int bindPoint; }
    [Serializable] public class MaterialShaderReflection { public ReflectedTexture[] readOnlyResources; }
    public static class StandardMaterialBuilder
    {
        static ConstantVariable Find(ConstantDocument doc, params string[] names)
        {
            foreach (string n in names) { var v = SceneRebuilder.Optional(doc, n); if (v != null && v.value != null && v.value.Length > 0) return v; }
            return null;
        }
        static Vector4 Value(ConstantVariable v)
        {
            var result = Vector4.zero; for (int i = 0; i < Math.Min(4, v.value.Length); ++i) result[i] = v.value[i]; return result;
        }
        public static Material Create(PackageReader reader, ReplayDraw draw, ConstantDocument constants, string captureFolder,
            string folder, GameObject go, List<string> issues)
        {
            var shader = Shader.Find("Standard (Specular setup)");
            if (shader == null) throw new InvalidOperationException("Built-in Standard (Specular setup) shader is unavailable.");
            var material = new Material(shader) { name = "E" + draw.eventId + " Standard Specular" };
            var state = go.GetComponent<CaptureMaterialState>();
            if (state == null) state = go.AddComponent<CaptureMaterialState>();
            state.eventId = draw.eventId;
            var floats = new List<GlobalConstantBufferDriver.VectorConstant>();
            foreach (var b in constants.blocks ?? new ConstantBlock[0]) foreach (var v in b.variables ?? new ConstantVariable[0]) SceneRebuilder.CollectFloats(v, floats);
            state.capturedFloats = floats.ToArray();
            string evidence = folder + "/E" + draw.eventId + "_Constants.json";
            File.Copy(reader.FilePath(draw.constants), evidence, false);
            AssetDatabase.ImportAsset(evidence, ImportAssetOptions.ForceSynchronousImport);
            state.constantEvidence = AssetDatabase.LoadAssetAtPath<TextAsset>(evidence);
            var mapped = new List<CaptureMaterialState.Property>(); var notes = new List<string>();
            Action<string, bool, string[]> map = (target, scalar, names) => {
                var value = Find(constants, names); if (value == null) return;
                Vector4 x = Value(value);
                if (scalar) material.SetFloat(target, x.x); else material.SetVector(target, x);
                mapped.Add(new CaptureMaterialState.Property { name = target, value = x, scalar = scalar });
                notes.Add("Inferred: " + value.name + " -> " + target);
            };
            map("_Color", false, new[] { "_BaseColor", "_AlbedoColor", "_Color", "_TintColor" });
            map("_SpecColor", false, new[] { "_SpecColor", "_SpecularColor" });
            map("_BumpScale", true, new[] { "_BumpScale" });
            map("_OcclusionStrength", true, new[] { "_OcclusionScale", "_OcclusionStrength" });
            map("_Cutoff", true, new[] { "_CutOff", "_Cutoff" });
            var smooth = Find(constants, "_Smoothness", "_Glossiness");
            if (smooth != null) map("_Glossiness", true, new[] { smooth.name });
            else notes.Add("Unrecoverable: no direct smoothness constant. Roughness remap/channel conversion not assumed.");
            map("_EmissionColor", false, new[] { "_EmissionColor", "_EmissiveColor" });
            var textureSlots = new HashSet<string>();
            if (draw.shaders != null && !string.IsNullOrEmpty(draw.shaders.Pixel))
            {
                var reflection = reader.Read<MaterialShaderReflection>(draw.shaders.Pixel);
                var bindings = reader.Read<BindingDocument>(draw.bindings);
                var resources = reflection.readOnlyResources ?? new ReflectedTexture[0];
                for (int i = 0; i < resources.Length; ++i)
                {
                    string source = resources[i].name, target = null;
                    string lower = source.ToLowerInvariant();
                    if (lower.Contains("albedo") || lower == "_maintex" || lower == "_basecolormap") target = "_MainTex";
                    else if (lower.Contains("normal") || lower == "_bumpmap") target = "_BumpMap";
                    else if (lower.Contains("emiss")) target = "_EmissionMap";
                    else if (lower == "_occlusionmap") target = "_OcclusionMap";
                    else if (lower == "_specglossmap") target = "_SpecGlossMap";
                    if (target == null || textureSlots.Contains(target)) continue;
                    string resourceId = null;
                    foreach (var binding in bindings.descriptors ?? new Binding[0])
                        if (binding.stage == "Pixel" && binding.reflectionIndex == i && binding.arrayElement == 0 && binding.resourceId != "ResourceId::0") resourceId = binding.resourceId;
                    foreach (var resource in draw.textures ?? new ResourceReference[0])
                    {
                        if (resource.resourceId != resourceId) continue;
                        var t = reader.Read<TextureDescription>(resource.path);
                        if (t.cubemap || t.arraySize > 1 || t.depth > 1 || t.samples > 1) { notes.Add("Unsupported texture: " + source); continue; }
                        foreach (var image in t.images ?? new TextureImage[0])
                        {
                            if (image.mip != 0 || image.layer != 0) continue;
                            string path = captureFolder + "/Textures/" + image.path;
                            var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
                            if (texture == null) { notes.Add("Missing imported texture: " + path); continue; }
                            if (target == "_BumpMap") { notes.Add("Normal texture requires verified encoding conversion; retained as evidence, not bound as Unity packed normal: " + source); continue; }
                            material.SetTexture(target, texture); textureSlots.Add(target);
                            notes.Add("Inferred: " + source + " (" + resourceId + ") -> " + target);
                            var st = Find(constants, source + "_ST");
                            if (st != null && st.value.Length == 4) { var x = Value(st); material.SetTextureScale(target, new Vector2(x.x,x.y)); material.SetTextureOffset(target,new Vector2(x.z,x.w)); }
                        }
                    }
                }
            }
            if (material.GetTexture("_SpecGlossMap") != null) material.EnableKeyword("_SPECGLOSSMAP");
            if (material.GetTexture("_EmissionMap") != null || material.GetColor("_EmissionColor").maxColorComponent > 0) material.EnableKeyword("_EMISSION");
            notes.Add("Inferred: opaque Standard Specular, default lighting/specular/smoothness. Capture blend mode, metallic conversion and packed maps are not guessed.");
            state.standardProperties = mapped.ToArray(); state.mappingNotes = string.Join("\n", notes.ToArray());
            foreach (var note in notes) issues.Add("E" + draw.eventId + " " + note);
            AssetDatabase.CreateAsset(material, folder + "/E" + draw.eventId + ".mat");
            return material;
        }
    }
}
