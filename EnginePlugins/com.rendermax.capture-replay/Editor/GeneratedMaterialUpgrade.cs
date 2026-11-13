using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    public static class GeneratedMaterialUpgrade
    {
        [Serializable] class Entry { public string shader; public uint[] eventIds; }
        [Serializable] class Map { public Entry[] entries; }
        sealed class Plan
        {
            public string path;
            public Material original, replacement;
            public ReplayDraw draw;
            public ConstantDocument constants;
        }

        [MenuItem("Tools/RenderMax/Advanced/Materials/Replace With Generated Shaders")]
        public static void ReplaceSelected()
        {
            var capture = Selection.activeObject as CaptureAsset;
            if (capture == null) capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxCaptures/Capture01/Capture.asset");
            if (capture == null) throw new InvalidOperationException("Select Capture.asset first.");
            if (!EditorUtility.DisplayDialog("Replace capture materials",
                "Replace mapped Standard/Phase2 E<eventId>.mat assets in this capture, preserving GUIDs. " +
                "Back up materials/scenes to Artifacts/RenderMaxMaterialUpgrade first. " +
                "Unmapped/custom materials are skipped. Generated shaders are editable candidates, not faithful frame replay.",
                "Replace", "Cancel")) return;
            Debug.Log(Replace(capture));
        }

        public static string Replace(CaptureAsset capture)
        {
            if (capture == null) throw new ArgumentNullException("capture");
            if (EditorApplication.isPlayingOrWillChangePlaymode || EditorApplication.isCompiling)
                throw new InvalidOperationException("Stop Play mode and wait for compilation first.");
            string folder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\', '/');
            if (!folder.StartsWith("Assets/", StringComparison.Ordinal)) throw new InvalidOperationException("Capture must be under Assets.");
            // Never silently save or discard edits in an already-open capture scene.
            for (int i = 0; i < SceneManager.sceneCount; ++i)
            {
                var scene = SceneManager.GetSceneAt(i);
                if (scene.path.StartsWith(folder + "/", StringComparison.Ordinal) && scene.isDirty)
                    throw new InvalidOperationException("Save the modified capture scene first: " + scene.path);
            }
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            var map = JsonUtility.FromJson<Map>(File.ReadAllText(CaptureShaderSceneTrial.ShaderFolder + "SceneShaderMap.json"));
            var shaders = new Dictionary<uint, Shader>();
            foreach (var entry in map.entries)
            {
                var shader = AssetDatabase.LoadAssetAtPath<Shader>(CaptureShaderSceneTrial.ShaderFolder + entry.shader + ".shader");
                if (shader == null) throw new InvalidDataException("Missing generated shader: " + entry.shader);
                foreach (uint id in entry.eventIds) shaders.Add(id, shader);
            }
            var draws = reader.Manifest.draws.ToDictionary(x => x.eventId, x => x.path);
            if (draws.Count != shaders.Count || draws.Keys.Any(id => !shaders.ContainsKey(id)))
                throw new InvalidDataException("This capture does not match the generated event map. Generate a map for this capture first.");
            var plans = new List<Plan>();
            var notes = new List<string> { "Editable generated shader candidates; not full-frame/pixel-equivalence validated." };
            string backup = "Artifacts/RenderMaxMaterialUpgrade/" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N").Substring(0, 8);
            var scenePaths = AssetDatabase.FindAssets("t:Scene", new[] { folder }).Select(AssetDatabase.GUIDToAssetPath).ToArray();
            bool async = ShaderUtil.allowAsyncCompilation;
            var originalActiveScene = SceneManager.GetActiveScene();
            int changed = 0, renderers = 0;
            try
            {
                ShaderUtil.allowAsyncCompilation = false;
                var checkedShaders = new HashSet<Shader>();
                foreach (string guid in AssetDatabase.FindAssets("t:Material", new[] { folder }))
                {
                    string path = AssetDatabase.GUIDToAssetPath(guid);
                    var match = Regex.Match(Path.GetFileName(path), @"^E(\d+)\.mat$");
                    uint id;
                    var material = AssetDatabase.LoadAssetAtPath<Material>(path);
                    if (!match.Success || !uint.TryParse(match.Groups[1].Value, out id) || !draws.ContainsKey(id) || !shaders.ContainsKey(id) || material == null ||
                        (material.shader != null && material.shader.name != "Standard" && material.shader.name != "Standard (Specular setup)" &&
                         !material.shader.name.StartsWith("RenderMax/Phase2/Editable/", StringComparison.Ordinal)))
                    { notes.Add("SKIPPED (unmapped/custom): " + path); continue; }
                    if (EditorUtility.IsDirty(material)) throw new InvalidOperationException("Save modified material first: " + path);
                    var plan = new Plan { path = path, original = material, replacement = new Material(shaders[id]), draw = reader.Read<ReplayDraw>(draws[id]) };
                    plans.Add(plan);
                    if (checkedShaders.Add(shaders[id]))
                    {
                        ShaderUtil.CompilePass(plan.replacement, 0, true);
                        if (!shaders[id].isSupported || ShaderUtil.ShaderHasError(shaders[id]))
                            throw new InvalidOperationException("Generated shader failed compilation: " + shaders[id].name);
                    }
                    plan.constants = reader.Read<ConstantDocument>(plan.draw.constants);
                    var bindingNotes = new List<string>();
                    foreach (var block in plan.constants.blocks ?? new ConstantBlock[0])
                        foreach (var variable in block.variables ?? new ConstantVariable[0]) CaptureShaderSceneTrial.Bind(plan.replacement, variable, bindingNotes);
                    CaptureShaderSceneTrial.BindTextures(reader, plan.draw, folder, plan.replacement, bindingNotes);
                    foreach (string note in bindingNotes) notes.Add(path + ": " + note);
                }
                if (plans.Count == 0) throw new InvalidOperationException("No mapped capture materials found. No changes made.");
                // Complete all shader/resource preflight before touching existing assets.
                foreach (var plan in plans) Backup(plan.path, backup);
                foreach (string path in scenePaths) Backup(path, backup);
                File.WriteAllLines(backup + "/BindingReport.txt", notes.ToArray());
                foreach (var plan in plans)
                {
                    string name = plan.original.name;
                    EditorUtility.CopySerialized(plan.replacement, plan.original);
                    plan.original.name = name;
                    PersistArrays(plan.original, plan.draw.eventId, plan.constants);
                    EditorUtility.SetDirty(plan.original);
                    ++changed;
                }
                AssetDatabase.SaveAssets();
                foreach (string path in scenePaths)
                {
                    var scene = SceneManager.GetSceneByPath(path);
                    bool openedHere = !scene.IsValid() || !scene.isLoaded;
                    if (openedHere) scene = EditorSceneManager.OpenScene(path, OpenSceneMode.Additive);
                    bool modified = false;
                    try
                    {
                        foreach (var root in scene.GetRootGameObjects())
                        foreach (var renderer in root.GetComponentsInChildren<Renderer>(true))
                        {
                            if (!AttachBindings(renderer)) continue;
                            modified = true; ++renderers;
                            var standard = renderer.GetComponent<CaptureMaterialState>(); if (standard != null) standard.enabled = false;
                            var draw = renderer.GetComponent<CaptureDrawState>();
                            if (draw != null) { draw.enabled = false; draw.translated = false; draw.status = "Generated editable shader; NOT GPU-equivalence validated."; }
                            renderer.SetPropertyBlock(null);
                            EditorUtility.SetDirty(renderer.gameObject);
                        }
                        if (modified)
                        {
                            EditorSceneManager.MarkSceneDirty(scene);
                            if (!EditorSceneManager.SaveScene(scene)) throw new IOException("Failed to save scene " + path);
                        }
                    }
                    finally { if (openedHere) EditorSceneManager.CloseScene(scene, true); }
                }
                notes.Insert(0, "SUCCESS materials=" + changed + ", scene renderers=" + renderers + ", compiled shaders=" + checkedShaders.Count);
                AssetDatabase.SaveAssets();
            }
            catch (Exception e)
            {
                notes.Insert(0, "FAILED after " + changed + " material updates: " + e.Message + ". Restore backed-up files if necessary.");
                throw;
            }
            finally
            {
                ShaderUtil.allowAsyncCompilation = async;
                if (originalActiveScene.IsValid() && originalActiveScene.isLoaded) SceneManager.SetActiveScene(originalActiveScene);
                foreach (var plan in plans) if (plan.replacement != null) UnityEngine.Object.DestroyImmediate(plan.replacement);
                Directory.CreateDirectory(backup);
                File.WriteAllLines(backup + "/BindingReport.txt", notes.ToArray());
            }
            return "Generated shader upgrade: " + changed + " materials, " + renderers + " renderers. Backup/report: " + backup;
        }

        static void Backup(string path, string folder)
        {
            string target = folder + "/" + path;
            Directory.CreateDirectory(Path.GetDirectoryName(target));
            File.Copy(path, target, false);
            if (File.Exists(path + ".meta")) File.Copy(path + ".meta", target + ".meta", false);
        }

        internal static void PersistArrays(Material material, uint eventId, ConstantDocument constants)
        {
            string path = AssetDatabase.GetAssetPath(material);
            var data = AssetDatabase.LoadAllAssetsAtPath(path).OfType<GeneratedMaterialBindings>().FirstOrDefault();
            if (data == null)
            {
                data = ScriptableObject.CreateInstance<GeneratedMaterialBindings>();
                data.name = "Captured generated shader arrays";
                AssetDatabase.AddObjectToAsset(data, material);
            }
            var arrays = new Dictionary<string, GlobalConstantBufferDriver.VectorConstant>();
            foreach (var block in constants.blocks ?? new ConstantBlock[0])
            foreach (var v in block.variables ?? new ConstantVariable[0])
            {
                if (v.members == null || v.members.Length == 0 || !Array.TrueForAll(v.members, x => x.value != null && x.value.Length > 0 && x.value.Length <= 4)) continue;
                string name = CaptureShaderSceneTrial.Name(v.name);
                arrays[name] = new GlobalConstantBufferDriver.VectorConstant { name = name, array = true, values = Array.ConvertAll(v.members, CaptureShaderSceneTrial.Vector) };
            }
            data.material = material; data.eventId = eventId; data.arrays = arrays.Values.ToArray();
            data.Apply(); EditorUtility.SetDirty(data);
        }

        internal static bool AttachBindings(Renderer renderer)
        {
            var data = new List<GeneratedMaterialBindings>();
            foreach (var material in renderer.sharedMaterials)
            {
                if (material == null || material.shader == null ||
                    !material.shader.name.StartsWith("RenderMax/Phase2/Editable/", StringComparison.Ordinal)) continue;
                string path = AssetDatabase.GetAssetPath(material);
                if (string.IsNullOrEmpty(path)) continue;
                data.AddRange(AssetDatabase.LoadAllAssetsAtPath(path).OfType<GeneratedMaterialBindings>().Where(x => x.material == material));
            }
            if (data.Count == 0) return false;
            var state = renderer.GetComponent<GeneratedMaterialState>();
            if (state == null) state = renderer.gameObject.AddComponent<GeneratedMaterialState>();
            state.bindings = data.Distinct().ToArray(); state.enabled = true; state.Apply();
            EditorUtility.SetDirty(state);
            return true;
        }
    }
}
