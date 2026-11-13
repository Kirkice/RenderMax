using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    public static class UpgradeStandardScene
    {
        [MenuItem("Tools/RenderMax/Advanced/Legacy/Upgrade to Standard Specular")]
        public static void Run()
        {
            var scene = SceneManager.GetActiveScene();
            if (string.IsNullOrEmpty(scene.path)) throw new InvalidOperationException("Save the current scene first.");
            CaptureAsset capture = null;
            for (string directory = Path.GetDirectoryName(scene.path); !string.IsNullOrEmpty(directory); directory = Path.GetDirectoryName(directory))
            {
                capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>((directory + "/Capture.asset").Replace('\\','/'));
                if (capture != null) break;
            }
            if (capture == null) throw new InvalidOperationException("Cannot locate the Capture.asset above this scene. Open a scene under the imported Capture directory.");
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            string source = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\','/');
            string folder = AssetDatabase.GenerateUniqueAssetPath(Path.GetDirectoryName(scene.path).Replace('\\','/') + "/StandardUpgrade");
            Directory.CreateDirectory(folder);
            int count = 0;
            var issues = new List<string>();
            var states = new List<CaptureDrawState>();
            foreach (var root in scene.GetRootGameObjects()) states.AddRange(root.GetComponentsInChildren<CaptureDrawState>(true));
            var drivers = new List<GlobalConstantBufferDriver>();
            foreach (var root in scene.GetRootGameObjects()) drivers.AddRange(root.GetComponentsInChildren<GlobalConstantBufferDriver>(true));
            foreach (var driver in drivers) { Undo.RecordObject(driver, "Disable legacy global writer"); driver.enabled = false; }
            foreach (var root in scene.GetRootGameObjects())
                foreach (var controller in root.GetComponentsInChildren<CaptureReplayController>(true))
                {
                    Undo.RecordObject(controller, "Disable legacy replay"); controller.orderedCommandBufferReplay = false; controller.Rebuild();
                    Undo.RecordObject(controller.GetComponent<Camera>(), "Enable renderers"); controller.GetComponent<Camera>().cullingMask = ~0;
                }
            foreach (var state in states)
            {
                var renderer = state.GetComponent<MeshRenderer>(); if (renderer == null) continue;
                try
                {
                    var draw = reader.Read<ReplayDraw>(state.sourceDraw);
                    var constants = reader.Read<ConstantDocument>(draw.constants);
                    // Reuse the component rather than adding multiple property-block writers.
                    var material = StandardMaterialBuilder.Create(reader, draw, constants, source, folder, state.gameObject, issues);
                    Undo.RecordObject(renderer, "Use Standard Specular");
                    renderer.sharedMaterial = material; renderer.enabled = true;
                    Undo.RecordObject(state, "Disable legacy captured shader state"); state.translated = false; state.enabled = false;
                    state.GetComponent<CaptureMaterialState>().Apply(); ++count;
                }
                catch (Exception e) { issues.Add("E" + state.eventId + ": " + e); }
            }
            File.WriteAllText(folder + "/UpgradeReport.txt", "Upgraded renderers: " + count + " / " + states.Count + "\n" + string.Join("\n", issues.ToArray()));
            EditorSceneManager.MarkSceneDirty(scene); AssetDatabase.SaveAssets(); AssetDatabase.Refresh();
            Debug.Log("RenderMax Standard upgrade: " + count + " renderers enabled and assigned Standard (Specular setup). Save scene with Ctrl+S. Report: " + folder);
        }
    }
}
