using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [InitializeOnLoad]
    internal static class CaptureSceneViewOrderBridge
    {
        sealed class Active
        {
            public CaptureReplayController controller;
            public CaptureReplayController.OrderedCameraSession session;
        }
        static readonly Dictionary<Camera, Active> active = new Dictionary<Camera, Active>();
        static string lastError;
        static CaptureSceneViewOrderBridge()
        {
            Camera.onPreCull += Begin;
            Camera.onPostRender += End;
            CaptureReplayController.RenderingInvalidated += Invalidate;
            AssemblyReloadEvents.beforeAssemblyReload += Cleanup;
            EditorApplication.quitting += Cleanup;
            EditorApplication.playModeStateChanged += state => Cleanup();
            EditorApplication.update += Watchdog;
        }
        static void Begin(Camera camera)
        {
            if (camera == null || camera.cameraType != CameraType.SceneView) return;
            End(camera);
            CaptureReplayController chosen = null;
            foreach (var controller in UnityEngine.Object.FindObjectsOfType<CaptureReplayController>())
            {
                if (!controller.isActiveAndEnabled || !controller.orderedCommandBufferReplay || !controller.orderedSceneView) continue;
                // Refuse ambiguous ordering across multiple independent captures.
                if (chosen != null) { Error("SceneView ordered replay requires exactly one enabled capture controller."); return; }
                chosen = controller;
            }
            if (chosen == null) return;
            var view = SceneView.currentDrawingSceneView;
            // The command path intentionally targets Shaded only: replacement
            // shaders, wireframe, prefab isolation and picking have other semantics.
            if (view == null || view.camera != camera || view.cameraMode.drawMode != DrawCameraMode.Textured) return;
            var stage = UnityEditor.SceneManagement.StageUtility.GetCurrentStageHandle();
            if (!stage.Contains(chosen.gameObject)) return;
            try
            {
                var session = chosen.BeginOrderedCamera(camera, false, renderer =>
                    stage.Contains(renderer.gameObject) && !SceneVisibilityManager.instance.IsHidden(renderer.gameObject));
                active.Add(camera, new Active { controller = chosen, session = session });
                lastError = null;
            }
            catch (Exception e) { Error("SceneView EID replay aborted: " + e.Message); }
        }
        static void Error(string message)
        {
            if (lastError == message) return;
            lastError = message; Debug.LogError(message);
        }
        static void End(Camera camera)
        {
            Active entry;
            if (!active.TryGetValue(camera, out entry)) return;
            entry.session.Dispose(); active.Remove(camera);
        }
        static void Invalidate(CaptureReplayController controller)
        {
            var cameras = new List<Camera>();
            foreach (var pair in active) if (pair.Value.controller == controller) cameras.Add(pair.Key);
            foreach (var camera in cameras) End(camera);
            SceneView.RepaintAll();
        }
        // Camera rendering is synchronous; an active session on the next editor
        // update indicates an interrupted render/closed window. Restore safely.
        static void Watchdog() { if (active.Count > 0) Cleanup(); }
        static void Cleanup()
        {
            foreach (var pair in active) pair.Value.session.Dispose();
            active.Clear();
        }
    }
}
