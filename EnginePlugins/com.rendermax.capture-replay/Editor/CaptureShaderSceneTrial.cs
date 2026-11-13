using System;
using System.IO;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    public static class CaptureShaderSceneTrial
    {
        internal const string ShaderFolder = "Packages/com.rendermax.capture-replay/Editor/Shaders/Phase2/CaptureCandidates/";
        [Serializable] class Entry { public string shader; public uint[] eventIds; }
        [Serializable] class Map { public Entry[] entries; }
        [MenuItem("Tools/RenderMax/Advanced/Experiments/Create Full Capture Shader Trial Scene")]
        public static void Create()
        {
            var capture = Selection.activeObject as CaptureAsset;
            if (capture == null) capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxCaptures/Capture01/Capture.asset");
            if (capture == null) throw new InvalidOperationException("Select Capture.asset first.");
            var current = SceneManager.GetActiveScene();
            if (current.name.Contains("Phase2")) throw new InvalidOperationException("Open the phase-1 CaptureReplay scene first, not an existing Phase2 trial.");
            if (current.GetRootGameObjects().Length == 0 || UnityEngine.Object.FindObjectsOfType<CaptureDrawState>().Length == 0)
                throw new InvalidOperationException("Open the phase-1 CaptureReplay scene first.");
            if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
            string captureFolder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\','/');
            string output = AssetDatabase.GenerateUniqueAssetPath(captureFolder + "/Phase2Trial");
            Directory.CreateDirectory(output); AssetDatabase.Refresh();
            string scenePath = output + "/CapturePhase2Trial.unity";
            // Save-as-copy before making any edits: original scene and assets stay intact.
            if (!EditorSceneManager.SaveScene(current, scenePath, true)) throw new IOException("Cannot copy current scene.");
            var scene = EditorSceneManager.OpenScene(scenePath);
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            var map = JsonUtility.FromJson<Map>(File.ReadAllText(ShaderFolder + "SceneShaderMap.json"));
            var shaders = new Dictionary<uint,string>();
            foreach (var entry in map.entries) foreach (uint eid in entry.eventIds) shaders.Add(eid, entry.shader);
            var notes = new List<string> { "EDITABLE CANDIDATE, not faithful frame replay. Live Unity clip position; other transforms/constants captured. Samplers use imported texture settings. Frame dependencies not replayed." };
            int applied = 0, failed = 0;
            bool async = ShaderUtil.allowAsyncCompilation; ShaderUtil.allowAsyncCompilation = false;
            try
            {
                foreach (var root in scene.GetRootGameObjects())
                foreach (var state in root.GetComponentsInChildren<CaptureDrawState>(true))
                {
                    Material mat = null;
                    try
                    {
                        string label;
                        if (!shaders.TryGetValue(state.eventId, out label)) throw new InvalidDataException("No shader mapping");
                        var renderer = state.GetComponent<MeshRenderer>();
                        if (renderer == null) throw new InvalidDataException("No MeshRenderer");
                        var shader = AssetDatabase.LoadAssetAtPath<Shader>(ShaderFolder + label + ".shader");
                        if (shader == null) throw new InvalidDataException("Missing shader " + label);
                        mat = new Material(shader) { name = "E" + state.eventId + "_Phase2" };
                        ShaderUtil.CompilePass(mat, 0, true);
                        if (!shader.isSupported || ShaderUtil.ShaderHasError(shader)) throw new InvalidOperationException("Shader unsupported/compile error: " + label);
                        var draw = reader.Read<ReplayDraw>(state.sourceDraw);
                        foreach (var block in reader.Read<ConstantDocument>(draw.constants).blocks)
                        foreach (var variable in block.variables) Bind(mat, variable, notes);
                        BindTextures(reader, draw, captureFolder, mat, notes);
                        AssetDatabase.CreateAsset(mat, output + "/E" + state.eventId + ".mat");
                        GeneratedMaterialUpgrade.PersistArrays(mat, state.eventId, reader.Read<ConstantDocument>(draw.constants));
                        var standard = state.GetComponent<CaptureMaterialState>(); if (standard != null) standard.enabled = false;
                        state.translated = false; state.enabled = false;
                        renderer.SetPropertyBlock(null); renderer.sharedMaterial = mat; renderer.enabled = true;
                        GeneratedMaterialUpgrade.AttachBindings(renderer);
                        state.status = "Phase2 editable shader candidate; NOT GPU-equivalence validated.";
                        mat = null; ++applied;
                    }
                    catch (Exception e)
                    {
                        ++failed; notes.Add("FAILED E" + state.eventId + ": " + e.Message);
                        // Do not leave a Standard draw looking like a successful Phase2 result.
                        var renderer = state.GetComponent<Renderer>(); if (renderer != null) renderer.enabled = false;
                    }
                    finally { if (mat != null && !AssetDatabase.Contains(mat)) UnityEngine.Object.DestroyImmediate(mat); }
                }
            }
            finally
            {
                ShaderUtil.allowAsyncCompilation = async;
                notes.Insert(0, "Applied=" + applied + ", failed/disabled=" + failed);
                File.WriteAllLines(output + "/BindingReport.txt", notes.ToArray());
                EditorSceneManager.SaveScene(scene); AssetDatabase.SaveAssets(); AssetDatabase.Refresh();
            }
            Debug.Log("Phase2 trial scene: " + scenePath + "; applied=" + applied + "; failed/disabled=" + failed + ". See BindingReport.txt.");
        }
        [MenuItem("Tools/RenderMax/Advanced/Experiments/Repair Captured Constant Names")]
        public static void RepairNames()
        {
            int count = 0;
            foreach (string guid in AssetDatabase.FindAssets("t:Material", new[] { "Assets/RenderMaxCaptures" }))
            {
                var material = AssetDatabase.LoadAssetAtPath<Material>(AssetDatabase.GUIDToAssetPath(guid));
                if (material == null || material.shader == null || !material.shader.name.StartsWith("RenderMax/Phase2/Editable/")) continue;
                var serialized = new SerializedObject(material);
                bool changed = false;
                foreach (string collection in new[] { "m_Floats", "m_Colors", "m_TexEnvs" })
                {
                    var entries = serialized.FindProperty("m_SavedProperties." + collection);
                    if (entries == null || !entries.isArray) continue;
                    for (int i = 0; i < entries.arraySize; ++i)
                    {
                        var key = entries.GetArrayElementAtIndex(i).FindPropertyRelative("first");
                        if (key != null && key.stringValue.StartsWith("_RMCaptured__", StringComparison.Ordinal))
                        {
                            key.stringValue = "_RMCaptured_" + key.stringValue.Substring("_RMCaptured__".Length);
                            changed = true;
                        }
                    }
                }
                if (changed) { serialized.ApplyModifiedPropertiesWithoutUndo(); EditorUtility.SetDirty(material); ++count; }
            }
            AssetDatabase.SaveAssets();
            Debug.Log("Repaired captured constant names on " + count + " Phase2 materials. Reopen scene to refresh materials if needed.");
        }

        internal static string Name(string name)
        {
            return name.StartsWith("hlslcc_") || name.StartsWith("unity_") || name == "_Time" || name == "_WorldSpaceCameraPos" || name == "_ProjectionParams" || name == "_ScreenParams" || name == "_ZBufferParams" ? "_RMCaptured_" + name.TrimStart('_') : name;
        }
        internal static Vector4 Vector(ConstantVariable v)
        {
            var a = v.value ?? new float[0]; return new Vector4(a.Length>0?a[0]:0,a.Length>1?a[1]:0,a.Length>2?a[2]:0,a.Length>3?a[3]:0);
        }
        internal static void Bind(Material m, ConstantVariable v, List<string> notes)
        {
            string n = Name(v.name);
            if (v.members != null && v.members.Length > 0)
            {
                bool vectors = Array.TrueForAll(v.members, x => x.value != null && x.value.Length > 0 && x.value.Length <= 4);
                if (vectors)
                {
                    var values = Array.ConvertAll(v.members, Vector); m.SetVectorArray(n, values);
                    notes.Add("Inferred vector-array layout: " + v.name + " [" + values.Length + "]");
                }
                else notes.Add("UNAPPLIED complex constant: " + v.name);
            }
            else if (v.type == "int" && v.value != null && v.value.Length == 1) m.SetInt(n, (int)v.value[0]);
            else if (v.value != null && v.value.Length == 1) m.SetFloat(n, v.value[0]);
            else if (v.value != null && v.value.Length <= 4) m.SetVector(n, Vector(v));
            else notes.Add("UNAPPLIED constant: " + v.name + " type=" + v.type);
        }
        internal static void BindTextures(PackageReader reader, ReplayDraw draw, string folder, Material mat, List<string> notes)
        {
            var bindings = reader.Read<BindingDocument>(draw.bindings).descriptors;
            foreach (string stage in new[] { "Vertex", "Pixel" })
            {
                string path = stage == "Vertex" ? draw.shaders.Vertex : draw.shaders.Pixel;
                var reflection = reader.Read<MaterialShaderReflection>(path);
                var resources = reflection.readOnlyResources ?? new ReflectedTexture[0];
                for (int i = 0; i < resources.Length; ++i)
                {
                    string id = null;
                    foreach (var b in bindings ?? new Binding[0]) if (b.stage == stage && b.reflectionIndex == i && b.arrayElement == 0 && b.resourceId != "ResourceId::0") id = b.resourceId;
                    if (id == null) { notes.Add("UNBOUND " + stage + " " + resources[i].name); continue; }
                    Texture texture = null;
                    foreach (var r in draw.textures ?? new ResourceReference[0])
                    {
                        if (r.resourceId != id) continue;
                        var meta = reader.Read<TextureDescription>(r.path);
                        texture = CapturedTextureBuilder.Load(reader, meta, r.path, folder);
                    }
                    string property = resources[i].name;
                    // Texture identifiers are NOT captured built-in constants.
                    // hlslcc_zcmp is a compiler alias for the same shadow resource.
                    if (property == "hlslcc_zcmphlslcc_zcmpg_CSMShadowTexture") property = "hlslcc_zcmpg_CSMShadowTexture";
                    if (texture != null && mat.HasProperty(property))
                    {
                        mat.SetTexture(property, texture);
                        if (property == "hlslcc_zcmpg_CSMShadowTexture" && mat.HasProperty("g_CSMShadowTexture"))
                            mat.SetTexture("g_CSMShadowTexture", texture);
                        notes.Add("BOUND " + resources[i].name + " -> " + property + " " + id + " " + AssetDatabase.GetAssetPath(texture));
                    }
                    else throw new InvalidDataException("Cannot bind texture: " + resources[i].name + " " + id + " property=" + property);
                }
            }
        }
    }
}
