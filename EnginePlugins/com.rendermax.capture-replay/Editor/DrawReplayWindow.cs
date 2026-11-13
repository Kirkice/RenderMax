using System;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    public sealed class DrawReplayWindow : EditorWindow
    {
        CaptureAsset capture;
        int selected;
        bool flipY;
        FilterMode filter = FilterMode.Bilinear;
        TextureWrapMode wrap = TextureWrapMode.Repeat;
        Color clear = Color.clear;
        DrawReplay replay;
        string message = "Select a Capture.asset, choose an event, then click Reconstruct Draw. Start with event 63.";
        Vector2 scroll;
        [MenuItem("Tools/RenderMax/Advanced/Experiments/Draw Reconstruction")]
        public static void Open()
        {
            var window = GetWindow<DrawReplayWindow>("RenderMax Draw");
            var asset = Selection.activeObject as CaptureAsset;
            if (asset != null) window.capture = asset;
        }
        void OnDisable() { if (replay != null) replay.Dispose(); replay = null; }
        void OnGUI()
        {
            EditorGUILayout.LabelField("Captured Draw Reconstruction", EditorStyles.boldLabel);
            var next = (CaptureAsset)EditorGUILayout.ObjectField("Capture", capture, typeof(CaptureAsset), false);
            if (next != capture) { capture = next; selected = 0; OnDisable(); }
            EditorGUILayout.HelpBox("One-click Capture scene creation. All events and available meshes are represented. Rendering is a limited geometry diagnostic, not faithful full-frame replay.", MessageType.Warning);
            if (capture == null || capture.draws.Count == 0) return;
            if (GUILayout.Button("Replay Capture — Create All-Draw Scene", GUILayout.Height(32)))
            {
                try { message = "Created all-event scene: " + CaptureSceneBuilder.Create(capture); }
                catch (Exception e) { message = e.Message; Debug.LogException(e); }
            }
            EditorGUILayout.HelpBox("Capture Replay creates all event nodes in one scene. Only translated shaders render; see ReplayReport.json. The controls below are single-draw diagnostics, not required for Capture Replay.", MessageType.Info);
            string[] labels = new string[capture.draws.Count];
            for (int i = 0; i < labels.Length; ++i) labels[i] = "Event " + capture.draws[i].eventId;
            selected = EditorGUILayout.Popup("Draw", Mathf.Clamp(selected, 0, labels.Length - 1), labels);
            flipY = EditorGUILayout.Toggle("Flip clip Y", flipY);
            filter = (FilterMode)EditorGUILayout.EnumPopup("Sampler filter override", filter);
            wrap = (TextureWrapMode)EditorGUILayout.EnumPopup("Sampler wrap override", wrap);
            clear = EditorGUILayout.ColorField("Initial target clear", clear);
            if (GUILayout.Button("Replay — Create Scene"))
            {
                try { message = "Created: " + SceneRebuilder.Create(capture, capture.draws[selected], true); }
                catch (Exception e) { message = e.Message; Debug.LogException(e); }
            }
            if (GUILayout.Button("Reconstruct Draw"))
            {
                try
                {
                    if (replay == null) replay = new DrawReplay();
                    replay.Render(capture, capture.draws[selected], clear, flipY, filter, wrap);
                    message = replay.Diagnostics;
                }
                catch (Exception e) { message = e.Message; Debug.LogException(e); }
            }
            if (replay != null && replay.Output != null)
            {
                if (GUILayout.Button("Save Rendered PNG"))
                {
                    string path = EditorUtility.SaveFilePanel("Save reconstruction", "", "Event" + capture.draws[selected].eventId + "_Unity.png", "png");
                    if (!string.IsNullOrEmpty(path))
                        try { replay.Save(path); } catch (Exception e) { message = e.Message; }
                }
                Rect rect = GUILayoutUtility.GetAspectRect((float)replay.Output.width / replay.Output.height);
                // DrawPreviewTexture shows RGB even if the shader's captured alpha is zero.
                EditorGUI.DrawPreviewTexture(rect, replay.Output, null, ScaleMode.ScaleToFit);
            }
            scroll = EditorGUILayout.BeginScrollView(scroll, GUILayout.MinHeight(90));
            EditorGUILayout.SelectableLabel(message, EditorStyles.wordWrappedLabel, GUILayout.MinHeight(130));
            EditorGUILayout.EndScrollView();
        }
    }
    [CustomEditor(typeof(CaptureAsset))]
    public sealed class CaptureAssetInspector : UnityEditor.Editor
    {
        public override void OnInspectorGUI()
        {
            if (GUILayout.Button("Replay Capture — Create Scene"))
            {
                try { CaptureSceneBuilder.Create((CaptureAsset)target); }
                catch (Exception e) { Debug.LogException(e); }
            }
            if (GUILayout.Button("Open Diagnostics")) { Selection.activeObject = target; DrawReplayWindow.Open(); }
            DrawDefaultInspector();
        }
    }
}
