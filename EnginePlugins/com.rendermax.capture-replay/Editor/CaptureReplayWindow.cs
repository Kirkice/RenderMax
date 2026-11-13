using System;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public sealed class CaptureReplayWindow : EditorWindow
    {
        string manifestPath = "", destination = "Assets/RenderMaxCaptures/Capture01", message = "";
        PackageReader reader;
        Vector2 scroll;
        int selected;
        CaptureAsset capture;
        int tab;
        bool advanced;
        void OnSelectionChange()
        {
            var selectedCapture = Selection.activeObject as CaptureAsset;
            if (selectedCapture != null) capture = selectedCapture;
            Repaint();
        }

        [MenuItem("Tools/RenderMax/Replay Tools")]
        public static void Open() { GetWindow<CaptureReplayWindow>("RenderMax Capture"); }

        void OnGUI()
        {
            EditorGUILayout.LabelField("RenderMax Replay Tools", EditorStyles.boldLabel);
            tab = GUILayout.Toolbar(tab, new[] { "Capture / Import", "Scene / Display" });
            capture = (CaptureAsset)EditorGUILayout.ObjectField("Capture", capture, typeof(CaptureAsset), false);
            using (new EditorGUI.DisabledScope(EditorApplication.isCompiling || EditorApplication.isPlayingOrWillChangePlaymode))
            {
                if (tab == 0) DrawImport(); else DrawScene();
            }
            EditorGUILayout.Space();
            EditorGUILayout.HelpBox(message, MessageType.None);
        }
        void DrawImport()
        {
            EditorGUILayout.HelpBox("Import into a NEW folder, then create a scene and replace materials with generated shaders. Integrity/binding success is not pixel-equivalent replay.", MessageType.Info);
            EditorGUILayout.BeginHorizontal();
            manifestPath = EditorGUILayout.TextField("Manifest", manifestPath);
            if (GUILayout.Button("Browse", GUILayout.Width(65)))
            {
                string path = EditorUtility.OpenFilePanel("Select RenderMax manifest.json", "", "json");
                if (!string.IsNullOrEmpty(path)) { manifestPath = path; reader = null; }
            }
            EditorGUILayout.EndHorizontal();
            if (GUILayout.Button("Read Package")) Run(() => { reader = new PackageReader(manifestPath); selected = 0; message = "Package metadata loaded (hash verification not yet run)."; });
            if (reader != null)
            {
                EditorGUILayout.LabelField("Schema / Scope / Status", reader.Manifest.schemaVersion + " / " + reader.Manifest.scope + " / " + reader.Manifest.status);
                EditorGUILayout.LabelField("Inventory", reader.Manifest.draws.Length + " draws; " + reader.Manifest.files.Length + " files");
                if (GUILayout.Button("Verify All Files (SHA-256)")) Run(() => {
                    try
                    {
                        reader.Validate((i, n) => {
                            if (EditorUtility.DisplayCancelableProgressBar("Verify package", i + " / " + n, (float)i / n)) throw new OperationCanceledException();
                        }); message = "File integrity and draw references verified.";
                    }
                    finally { EditorUtility.ClearProgressBar(); }
                });
                if (reader.Manifest.draws.Length > 0)
                {
                    selected = EditorGUILayout.IntSlider("Draw index", selected, 0, reader.Manifest.draws.Length - 1);
                    var entry = reader.Manifest.draws[selected];
                    EditorGUILayout.LabelField("Event", entry.eventId.ToString());
                    if (GUILayout.Button("Reveal Draw JSON")) Run(() => EditorUtility.RevealInFinder(reader.FilePath(entry.path)));
                    if (GUILayout.Button("Replay Capture — Create All-Draw Scene")) Run(() =>
                    {
                        var importedCapture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(destination + "/Capture.asset");
                        if (importedCapture == null) throw new InvalidOperationException("Import the package into the destination first.");
                        if (!UnityEditor.SceneManagement.EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
                        capture = importedCapture;
                        string scene = CaptureSceneBuilder.Create(importedCapture);
                        message = "Created and opened scene: " + scene;
                    });
                }
            }
            destination = EditorGUILayout.TextField("New Assets Folder", destination);
            using (new EditorGUI.DisabledScope(reader == null))
                if (GUILayout.Button("Validate & Import Evidence Assets")) Run(() => {
                    var report = AssetPackageImporter.Import(manifestPath, destination);
                    capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>(destination + "/Capture.asset");
                    message = "Imported: " + report.imported.Count + "; errors: " + report.errors.Count + "; unsupported: " + report.unsupported.Count + ". See ImportReport.json.";
                });
        }
        void DrawScene()
        {
            scroll = EditorGUILayout.BeginScrollView(scroll);
            using (new EditorGUI.DisabledScope(capture == null))
            {
                if (GUILayout.Button("1. Create All-Draw Scene")) Run(() => {
                    if (!UnityEditor.SceneManagement.EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
                    message = "Created: " + CaptureSceneBuilder.Create(capture);
                });
                if (GUILayout.Button("2. Replace / Rebind Generated Materials")) Run(() => {
                    if (!EditorUtility.DisplayDialog("Replace capture materials", "Preserves material GUIDs and backs up assets to Artifacts. Relevant scenes must be saved first. Generated shaders are not equivalence-validated.", "Replace", "Cancel")) return;
                    message = GeneratedMaterialUpgrade.Replace(capture);
                });
            }
            var controllers = UnityEngine.Object.FindObjectsOfType<CaptureReplayController>();
            if (controllers.Length != 1)
                EditorGUILayout.HelpBox("Open a scene with exactly one active CaptureReplayController to edit replay/display settings.", MessageType.Info);
            else
            {
                var controller = controllers[0];
                EditorGUILayout.ObjectField("Scene Controller", controller, typeof(CaptureReplayController), true);
                using (new EditorGUI.DisabledScope(capture == null))
                    if (GUILayout.Button("Recover Main Camera From Selected Capture")) Run(() => {
                        var camera = controller.GetComponent<Camera>();
                        Undo.RecordObjects(new UnityEngine.Object[] { camera, camera.transform }, "Recover main camera");
                        string result;
                        if (!CaptureMainCameraRecovery.TryApply(capture, camera, out result)) throw new InvalidOperationException(result);
                        Dirty(controller); message = result;
                    });
                EditorGUI.BeginChangeCheck();
                bool ordered = EditorGUILayout.Toggle("EID Ordered Geometry", controller.orderedCommandBufferReplay);
                bool sceneView = EditorGUILayout.Toggle("Also In SceneView", controller.orderedSceneView);
                if (EditorGUI.EndChangeCheck())
                {
                    Undo.RecordObject(controller, "Replay order");
                    controller.orderedCommandBufferReplay = ordered; controller.orderedSceneView = sceneView;
                    controller.Rebuild(); Dirty(controller);
                }
                EditorGUILayout.HelpBox(controller.orderedStatus, MessageType.None);
                var gamma = controller.GetComponent<ReplayGammaDisplay>();
                if (gamma == null)
                {
                    if (GUILayout.Button("Add Gamma Display Control")) Run(() => {
                        gamma = Undo.AddComponent<ReplayGammaDisplay>(controller.gameObject);
                        gamma.displayShader = Shader.Find("Hidden/RenderMax/ReplayGammaDisplay"); Dirty(gamma);
                    });
                }
                else
                {
                    EditorGUI.BeginChangeCheck();
                    bool enabled = EditorGUILayout.Toggle("Gamma Effect Enabled", gamma.enabled);
                    bool asGamma = EditorGUILayout.Toggle("Linear Data As Gamma", gamma.linearDisplayAsGamma);
                    if (EditorGUI.EndChangeCheck())
                    {
                        Undo.RecordObject(gamma, "Gamma display"); gamma.enabled = enabled;
                        gamma.linearDisplayAsGamma = asGamma; Dirty(gamma);
                    }
                }
                EditorGUILayout.HelpBox("SceneView: Shaded mode; Gamma requires Effects enabled. Gamma affects the whole view. EID ordering does not restore original render targets/passes. Changes mark the scene dirty but do not auto-save.", MessageType.Info);
            }
            advanced = EditorGUILayout.Foldout(advanced, "Advanced / Legacy", true);
            if (advanced)
            {
                EditorGUILayout.HelpBox("Single-draw reconstruction, E63 trials, legacy Standard repairs and self-tests are under Tools > RenderMax > Advanced. Legacy tools may overwrite generated material state; do not use them as part of the normal workflow.", MessageType.Warning);
                if (GUILayout.Button("Open Draw Reconstruction")) EditorApplication.ExecuteMenuItem("Tools/RenderMax/Advanced/Experiments/Draw Reconstruction");
            }
            EditorGUILayout.EndScrollView();
        }
        static void Dirty(Component component)
        {
            EditorUtility.SetDirty(component);
            UnityEditor.SceneManagement.EditorSceneManager.MarkSceneDirty(component.gameObject.scene);
            SceneView.RepaintAll();
        }
        void Run(Action action)
        {
            try { action(); }
            catch (Exception e) { message = e.Message; Debug.LogException(e); }
        }
    }
}
