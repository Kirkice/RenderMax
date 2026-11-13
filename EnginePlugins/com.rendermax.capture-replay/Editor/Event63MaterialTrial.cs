using System;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class Event63MaterialTrial
    {
        [MenuItem("Tools/RenderMax/Advanced/Experiments/Apply E63 Shader to Current Scene")]
        public static void Apply()
        {
            var shader = AssetDatabase.LoadAssetAtPath<Shader>("Packages/com.rendermax.capture-replay/Editor/Shaders/Phase2/Event63.shader");
            if (shader == null || ShaderUtil.ShaderHasError(shader) || !shader.isSupported) throw new InvalidOperationException("E63 candidate shader missing, has compilation errors, or is unsupported on " + SystemInfo.graphicsDeviceType + ".");
            int count = 0;
            foreach (var root in UnityEngine.SceneManagement.SceneManager.GetActiveScene().GetRootGameObjects())
            foreach (var state in root.GetComponentsInChildren<CaptureDrawState>(true))
            {
                if (state.eventId != 63) continue;
                var renderer = state.GetComponent<MeshRenderer>();
                var data = state.GetComponent<CaptureMaterialState>();
                if (renderer == null || renderer.sharedMaterial == null || data == null || data.constantEvidence == null)
                    throw new InvalidOperationException("E63 requires an existing material and constant evidence.");
                var original = renderer.sharedMaterial;
                string folder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(original)).Replace('\\', '/');
                // Resolve evidence before allocating a material; repeated runs do not depend on the old shader.
                var texture = CapturedTexture(state, folder);
                var material = new Material(shader) { name = "E63_Phase2_EditableCandidate" };
                var constants = JsonUtility.FromJson<ConstantDocument>(data.constantEvidence.text);
                foreach (var block in constants.blocks)
                foreach (var variable in block.variables)
                {
                    // Editable E63 only consumes ST; live Unity matrices drive vertex position.
                    if (variable.name == "_AlbedoMap_ST") material.SetVector(variable.name, Vector(variable));
                }
                material.SetTexture("_AlbedoMap", texture);
                material.SetInt("_RMCull", 0); material.SetInt("_RMZTest", 4); material.SetInt("_RMZWrite", 1);
                string path = AssetDatabase.GenerateUniqueAssetPath(folder + "/E63_Phase2.mat");
                AssetDatabase.CreateAsset(material, path);
                Undo.RecordObject(renderer, "Trial E63 generated shader");
                Undo.RecordObject(data, "Disable Standard property writer"); data.enabled = false;
                renderer.SetPropertyBlock(null); renderer.sharedMaterial = material; renderer.enabled = true;
                EditorUtility.SetDirty(renderer); EditorUtility.SetDirty(data);
                EditorSceneManager.MarkSceneDirty(state.gameObject.scene);
                Debug.Log("E63 candidate assigned: " + path + ". Original retained: " + AssetDatabase.GetAssetPath(original) + ". Editable vertex position; capture-reflected texture binding (imported derivative); diagnostic pipeline, NOT GPU-validated. Ctrl+Z restores assignment/component state.");
                ++count;
            }
            AssetDatabase.SaveAssets();
            if (count == 0) throw new InvalidOperationException("No E63 node in the active scene. Open the CaptureReplay scene first.");
        }
        static Texture2D CapturedTexture(CaptureDrawState state, string materialFolder)
        {
            CaptureAsset capture = null;
            string captureFolder = null;
            for (string dir = materialFolder; !string.IsNullOrEmpty(dir); dir = Path.GetDirectoryName(dir))
            {
                string assetPath = (dir + "/Capture.asset").Replace('\\', '/');
                capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(assetPath);
                if (capture != null) { captureFolder = dir.Replace('\\', '/'); break; }
            }
            if (capture == null) throw new InvalidDataException("Cannot locate Capture.asset above E63 material folder: " + materialFolder);
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            var draw = reader.Read<ReplayDraw>(state.sourceDraw);
            var reflection = reader.Read<MaterialShaderReflection>(draw.shaders.Pixel);
            int index = Array.FindIndex(reflection.readOnlyResources, r => r.name == "_AlbedoMap");
            if (index < 0) throw new InvalidDataException("E63 reflection has no _AlbedoMap.");
            string resourceId = null;
            foreach (var binding in reader.Read<BindingDocument>(draw.bindings).descriptors)
                if (binding.stage == "Pixel" && binding.reflectionIndex == index && binding.arrayElement == 0 && binding.resourceId != "ResourceId::0")
                {
                    if (resourceId != null && resourceId != binding.resourceId) throw new InvalidDataException("Ambiguous E63 _AlbedoMap binding.");
                    resourceId = binding.resourceId;
                }
            foreach (var resource in draw.textures ?? new ResourceReference[0])
            {
                if (resource.resourceId != resourceId) continue;
                var metadata = reader.Read<TextureDescription>(resource.path);
                if (metadata.cubemap || metadata.arraySize > 1 || metadata.depth > 1 || metadata.samples > 1)
                    throw new NotSupportedException("E63 _AlbedoMap is not an ordinary 2D texture.");
                foreach (var image in metadata.images ?? new TextureImage[0])
                {
                    if (image.mip != 0 || image.layer != 0) continue;
                    string path = captureFolder + "/Textures/" + image.path;
                    var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
                    if (texture == null) throw new InvalidDataException("Missing imported E63 texture: " + path);
                    Debug.Log("E63 _AlbedoMap -> " + resourceId + " -> " + path);
                    return texture;
                }
            }
            throw new InvalidDataException("Cannot resolve captured E63 _AlbedoMap texture: " + resourceId);
        }

        static Vector4 Vector(ConstantVariable v)
        {
            if (v.value == null || v.value.Length != 4) throw new InvalidDataException("Expected float4: " + v.name);
            return new Vector4(v.value[0],v.value[1],v.value[2],v.value[3]);
        }
    }
}
