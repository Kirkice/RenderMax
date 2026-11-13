using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

namespace RenderMax.CaptureReplay
{
    [ExecuteAlways, RequireComponent(typeof(Camera))]
    public sealed class CaptureReplayController : MonoBehaviour
    {
        public CaptureDrawState[] events;
        [Tooltip("Ascending EID submission of generated editable geometry, not original frame replay.")]
        public bool orderedCommandBufferReplay;
        [Tooltip("Also use EID ordering in editor Scene views. Requires ordered mode above.")]
        public bool orderedSceneView = true;
        public bool clearBeforeReplay = true;
        public Color clearColor = Color.clear;
        [TextArea] public string limitations = "EID-ordered geometry only. Original render targets, clear/copy/resolve, dispatch, sampler semantics and resource lifetimes are not replayed.";
        [TextArea] public string orderedStatus;
        OrderedCameraSession gameSession;
        public static event Action<CaptureReplayController> RenderingInvalidated;
        void OnEnable() { Rebuild(); }
        void OnValidate() { Rebuild(); }
        void OnPreCull()
        {
            EndGame();
            if (!orderedCommandBufferReplay) return;
            try { gameSession = BeginOrderedCamera(GetComponent<Camera>(), clearBeforeReplay, null); }
            catch (Exception e) { orderedStatus = "Ordered rendering aborted: " + e.Message; Debug.LogException(e, this); }
        }
        void OnPostRender() { EndGame(); }
        public void Rebuild()
        {
            EndGame();
            if (RenderingInvalidated != null) RenderingInvalidated(this);
            orderedStatus = orderedCommandBufferReplay ? "Ready: ascending EID, pass 0, live transforms." : "Normal Unity sorting.";
        }
        void EndGame() { if (gameSession != null) gameSession.Dispose(); gameSession = null; }
        void OnDisable() { Rebuild(); }
        void OnDestroy() { EndGame(); if (RenderingInvalidated != null) RenderingInvalidated(this); }

        // One session per camera render. The editor bridge supplies its own visibility
        // predicate; ordering, validation and recording are shared with Game view.
        public OrderedCameraSession BeginOrderedCamera(Camera camera, bool clear, Predicate<MeshRenderer> visible)
        {
            var session = new OrderedCameraSession(camera);
            try
            {
                session.Record(events, clear, clearColor, visible);
                orderedStatus = camera.name + ": ascending EID, " + session.DrawCount + " draws; " + session.Skipped + " excluded. Not full-frame replay.";
                return session;
            }
            catch { session.Dispose(); throw; }
        }

        public sealed class OrderedCameraSession : IDisposable
        {
            readonly Camera camera;
            CommandBuffer commands;
            readonly List<MeshRenderer> suppressed = new List<MeshRenderer>();
            public int DrawCount { get; private set; }
            public int Skipped { get; private set; }
            internal OrderedCameraSession(Camera camera) { this.camera = camera; }
            internal void Record(CaptureDrawState[] events, bool clear, Color color, Predicate<MeshRenderer> visible)
            {
                var ordered = new List<CaptureDrawState>();
                var ids = new HashSet<uint>();
                var eligible = new HashSet<MeshRenderer>();
                foreach (var draw in events ?? new CaptureDrawState[0])
                {
                    if (draw == null) continue;
                    if (!ids.Add(draw.eventId)) throw new InvalidOperationException("Duplicate EID: " + draw.eventId);
                    ordered.Add(draw);
                }
                ordered.Sort((a,b) => a.eventId.CompareTo(b.eventId));
                foreach (var draw in ordered)
                {
                    var renderer = draw.GetComponent<MeshRenderer>();
                    if (renderer == null || !renderer.enabled || !renderer.gameObject.activeInHierarchy ||
                        (camera.cullingMask & (1 << renderer.gameObject.layer)) == 0 ||
                        (visible != null && !visible(renderer))) { ++Skipped; continue; }
                    var filter = draw.GetComponent<MeshFilter>();
                    if (filter == null || filter.sharedMesh == null) throw new InvalidOperationException("Missing mesh at E" + draw.eventId);
                    var materials = renderer.sharedMaterials;
                    if (filter.sharedMesh.subMeshCount != 1 || materials.Length != 1)
                        throw new InvalidOperationException("Expected one submesh/material at E" + draw.eventId);
                    var material = materials[0];
                    if (material == null || material.shader == null || !material.shader.isSupported ||
                        !material.shader.name.StartsWith("RenderMax/Phase2/Editable/", StringComparison.Ordinal))
                        throw new InvalidOperationException("Unsupported generated material at E" + draw.eventId);
                    eligible.Add(renderer);
                }
                commands = new CommandBuffer { name = "RenderMax EID ordered generated geometry" };
                if (clear) commands.ClearRenderTarget(true, true, color, SystemInfo.usesReversedZBuffer ? 0f : 1f);
                foreach (var draw in ordered)
                {
                    var renderer = draw.GetComponent<MeshRenderer>();
                    if (!eligible.Contains(renderer)) continue;
                    var state = renderer.GetComponent<GeneratedMaterialState>();
                    if (state != null) state.Apply();
                    var properties = new MaterialPropertyBlock(); renderer.GetPropertyBlock(properties);
                    string marker = "RenderMax E" + draw.eventId;
                    commands.BeginSample(marker);
                    commands.DrawMesh(draw.GetComponent<MeshFilter>().sharedMesh, renderer.localToWorldMatrix,
                        renderer.sharedMaterial, 0, 0, properties);
                    commands.EndSample(marker); ++DrawCount;
                }
                camera.AddCommandBuffer(CameraEvent.BeforeForwardOpaque, commands);
                // Only originally enabled renderers are touched. Restored at the end
                // of this camera render, never persisted as disabled scene objects.
                foreach (var renderer in eligible) { suppressed.Add(renderer); renderer.enabled = false; }
            }
            public void Dispose()
            {
                foreach (var renderer in suppressed) if (renderer != null) renderer.enabled = true;
                suppressed.Clear();
                if (commands == null) return;
                if (camera != null) camera.RemoveCommandBuffer(CameraEvent.BeforeForwardOpaque, commands);
                commands.Release(); commands = null;
            }
        }
    }
}
