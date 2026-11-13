using System;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public static class RepairSceneRenderers
    {
        [MenuItem("Tools/RenderMax/Advanced/Legacy/Repair Scene Materials and Renderers")]
        public static void Run()
        {
            var scene = UnityEngine.SceneManagement.SceneManager.GetActiveScene();
            if (string.IsNullOrEmpty(scene.path)) throw new InvalidOperationException("Save the scene before repairing.");
            string folder = Path.GetDirectoryName(scene.path).Replace('\\', '/') + "/DiagnosticMaterials";
            Directory.CreateDirectory(folder); AssetDatabase.Refresh();
            var shader = AssetDatabase.LoadAssetAtPath<Shader>("Packages/com.rendermax.capture-replay/Editor/Shaders/UnsupportedPreview.shader");
            if (shader == null) throw new InvalidOperationException("Diagnostic shader missing.");
            int count = 0;
            foreach (var root in scene.GetRootGameObjects())
            {
                foreach (var controller in root.GetComponentsInChildren<CaptureReplayController>(true))
                {
                    Undo.RecordObject(controller, "Use normal scene renderers");
                    controller.orderedCommandBufferReplay = false; controller.Rebuild();
                    Undo.RecordObject(controller.GetComponent<Camera>(), "Enable scene rendering");
                    controller.GetComponent<Camera>().cullingMask = ~0;
                }
                foreach (var state in root.GetComponentsInChildren<CaptureDrawState>(true))
                {
                    var renderer = state.GetComponent<MeshRenderer>();
                    if (renderer == null) continue;
                    Undo.RecordObject(renderer, "Repair captured mesh renderer");
                    if (renderer.sharedMaterial == null)
                    {
                        var material = new Material(shader) { name = "UNTRANSLATED Event " + state.eventId };
                        string path = AssetDatabase.GenerateUniqueAssetPath(folder + "/E" + state.eventId + ".mat");
                        AssetDatabase.CreateAsset(material, path); renderer.sharedMaterial = material;
                    }
                    if (state.translated && renderer.sharedMaterial != null)
                    {
                        var material = renderer.sharedMaterial;
                        string shaderPath = AssetDatabase.GetAssetPath(material.shader);
                        if (!string.IsNullOrEmpty(shaderPath) && shaderPath.StartsWith("Assets/", StringComparison.Ordinal))
                        {
                            string name = material.shader.name;
                            string template = File.ReadAllText("Packages/com.rendermax.capture-replay/Editor/Shaders/CapturedAlbedo.shader");
                            File.WriteAllText(shaderPath, template.Replace("Hidden/RenderMax/CapturedAlbedo", name));
                            AssetDatabase.ImportAsset(shaderPath, ImportAssetOptions.ForceSynchronousImport);
                        }
                        material.EnableKeyword("RENDERMAX_EDITABLE_SCENE"); EditorUtility.SetDirty(material);
                    }
                    renderer.enabled = true; state.Apply(); ++count;
                }
            }
            EditorSceneManager.MarkSceneDirty(scene); AssetDatabase.SaveAssets();
            Debug.Log("RenderMax repaired " + count + " mesh renderers. Untranslated shaders use explicitly diagnostic magenta materials. Save the scene to retain changes.");
        }
    }
}
