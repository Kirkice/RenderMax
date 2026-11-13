using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class SceneBuildReport
    {
        public string status = "building", scene;
        public int events, meshes, translated, cameras;
        public List<string> issues = new List<string>();
        public string limitation = "Phase 1 editable Standard Specular scene. Material semantics inferred; no capture-shader or pixel equivalence claimed. GPU instances, skinning, packed maps and non-float constant application may require reconstruction. Full typed constants retained as JSON. Missing camera poses are reported, not invented.";
    }
    public static class CaptureSceneBuilder
    {
        static string Hash(string source)
        {
            using (var hash = SHA256.Create()) return BitConverter.ToString(hash.ComputeHash(Encoding.UTF8.GetBytes(source.Replace("\r\n", "\n")))) .Replace("-", "").ToLowerInvariant();
        }
        public static string Create(CaptureAsset capture, bool prompt = true)
        {
            if (capture == null) throw new ArgumentNullException("capture");
            if (prompt && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) throw new OperationCanceledException();
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            string captureFolder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\', '/');
            string folder = AssetDatabase.GenerateUniqueAssetPath(captureFolder + "/CaptureReplay");
            Directory.CreateDirectory(folder);
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            var root = new GameObject("RenderMax Capture");
            var drawsRoot = new GameObject("Draws"); drawsRoot.transform.SetParent(root.transform);
            var camerasRoot = new GameObject("Captured Views"); camerasRoot.transform.SetParent(root.transform);
            var report = new SceneBuildReport();
            var states = new List<CaptureDrawState>();
            var imported = new Dictionary<uint, ImportedDraw>();
            foreach (var d in capture.draws) imported[d.eventId] = d;
            var views = new HashSet<string>();
            CapturedCamera recoveredCamera = null;
            // Phase one intentionally uses only the built-in Standard Specular shader.
            try
            {
                foreach (var entry in reader.Manifest.draws)
                {
                    if (EditorUtility.DisplayCancelableProgressBar("Replay: create capture scene", "Event " + entry.eventId, (float)report.events / Math.Max(1, reader.Manifest.draws.Length)))
                        throw new OperationCanceledException("Partial scene retained.");
                    var go = new GameObject("E" + entry.eventId.ToString("D8")); go.transform.SetParent(drawsRoot.transform, false);
                    var state = go.AddComponent<CaptureDrawState>(); state.eventId = entry.eventId; state.sourceDraw = entry.path;
                    states.Add(state); ++report.events;
                    try
                    {
                        var draw = reader.Read<ReplayDraw>(entry.path);
                        var constants = reader.Read<ConstantDocument>(draw.constants);
                        var values = new List<GlobalConstantBufferDriver.VectorConstant>();
                        foreach (var block in constants.blocks ?? new ConstantBlock[0])
                            foreach (var variable in block.variables ?? new ConstantVariable[0]) SceneRebuilder.CollectFloats(variable, values);
                        state.constants = values.ToArray();
                        ImportedDraw mesh;
                        Mesh geometry = imported.TryGetValue(entry.eventId, out mesh) ? mesh.mesh : null;
                        if (geometry == null && !string.IsNullOrEmpty(draw.mesh))
                        {
                            try
                            {
                                var warnings = new List<string>();
                                geometry = MeshImporter.Build(reader, reader.Read<MeshDescription>(draw.mesh), warnings);
                                AssetDatabase.CreateAsset(geometry, folder + "/E" + entry.eventId + "_Mesh.asset");
                                foreach (string warning in warnings) report.issues.Add("E" + entry.eventId + ": " + warning);
                            }
                            catch (Exception e) { report.issues.Add("E" + entry.eventId + " mesh: " + e.Message); }
                        }
                        if (geometry != null)
                        {
                            go.AddComponent<MeshFilter>().sharedMesh = geometry;
                            var renderer = go.AddComponent<MeshRenderer>();
                            renderer.sharedMaterial = StandardMaterialBuilder.Create(reader, draw, constants, captureFolder, folder, go, report.issues);
                            go.GetComponent<CaptureMaterialState>().Apply();
                            renderer.enabled = true; ++report.meshes;
                        }
                        var model = SceneRebuilder.Optional(constants, "hlslcc_mtx4x4unity_ObjectToWorld");
                        var vp = SceneRebuilder.Optional(constants, "hlslcc_mtx4x4unity_MatrixVP");
                        if (model != null)
                        {
                            state.objectColumns = SceneRebuilder.Columns(model);
                            try { SceneRebuilder.ApplyTransform(go.transform, SceneRebuilder.Matrix(state.objectColumns)); }
                            catch (Exception e) { report.issues.Add("E" + entry.eventId + " transform: " + e.Message); }
                        }
                        else report.issues.Add("E" + entry.eventId + ": no ObjectToWorld; node transform left identity (not recovered).");
                        if (vp != null)
                        {
                            state.vpColumns = SceneRebuilder.Columns(vp);
                            string key = JsonUtility.ToJson(vp);
                            if (views.Add(key))
                            {
                                var cameraGo = new GameObject("View E" + entry.eventId); cameraGo.transform.SetParent(camerasRoot.transform);
                                var camera = cameraGo.AddComponent<Camera>(); camera.enabled = false;
                                var viewState = cameraGo.AddComponent<CapturedCamera>();
                                var view = SceneRebuilder.Optional(constants, "hlslcc_mtx4x4unity_MatrixV");
                                var inverse = SceneRebuilder.Optional(constants, "hlslcc_mtx4x4unity_MatrixInvV");
                                if (view != null || inverse != null)
                                {
                                    viewState.view = view != null ? SceneRebuilder.Matrix(SceneRebuilder.Columns(view)) : SceneRebuilder.Matrix(SceneRebuilder.Columns(inverse)).inverse;
                                    var world = viewState.view.inverse;
                                    cameraGo.transform.position = world.GetColumn(3);
                                    cameraGo.transform.rotation = Quaternion.LookRotation(-world.GetColumn(2), world.GetColumn(1));
                                    viewState.lockCapturedView = false;
                                    recoveredCamera = viewState;
                                }
                                else report.issues.Add("View E" + entry.eventId + ": VP-only; original camera pose unrecoverable, identity View used with combined VP projection.");
                                viewState.projection = SceneRebuilder.Matrix(state.vpColumns) * viewState.view.inverse;
                                viewState.Apply(); ++report.cameras;
                            }
                        }
                        state.translated = false;
                        state.status = geometry != null ? "Phase 1: Standard Specular scene instance; captured constants preserved, material mapping inferred." : "No recoverable mesh; event evidence retained.";
                        if (!string.IsNullOrEmpty(draw.mesh))
                        {
                            var desc = reader.Read<MeshDescription>(draw.mesh);
                            if (desc.instanceCount > 1) report.issues.Add("E" + entry.eventId + ": " + desc.instanceCount + " GPU instances; only base geometry represented, instance transforms not reconstructed.");
                        }
                    }
                    catch (Exception e) { state.status = e.Message; report.issues.Add("E" + entry.eventId + ": " + e.Message); }
                }
                report.status = "completedWithLimitations";
            }
            catch (Exception e) { report.status = "partial"; report.issues.Add(e.Message); }
            finally
            {
                var cameraGo = new GameObject("Replay Camera — Geometry Diagnostic"); cameraGo.transform.SetParent(root.transform);
                var camera = cameraGo.AddComponent<Camera>(); camera.clearFlags = CameraClearFlags.SolidColor; camera.backgroundColor = Color.clear;
                camera.cullingMask = ~0; camera.clearFlags = CameraClearFlags.SolidColor;
                camera.nearClipPlane = 0.01f; camera.farClipPlane = 10000;
                Bounds bounds = new Bounds(); bool hasBounds = false;
                foreach (var state in states)
                {
                    var renderer = state.GetComponent<MeshRenderer>();
                    if (renderer == null) continue;
                    if (!hasBounds) { bounds = renderer.bounds; hasBounds = true; }
                    else bounds.Encapsulate(renderer.bounds);
                }
                if (hasBounds)
                {
                    float distance = Mathf.Max(1, bounds.extents.magnitude / Mathf.Tan(camera.fieldOfView * Mathf.Deg2Rad * 0.5f));
                    cameraGo.transform.position = bounds.center - Vector3.forward * distance * 1.2f;
                    cameraGo.transform.LookAt(bounds.center);
                }
                if (recoveredCamera != null)
                {
                    cameraGo.transform.position = recoveredCamera.transform.position;
                    cameraGo.transform.rotation = recoveredCamera.transform.rotation;
                    var liveProjection = cameraGo.AddComponent<CapturedCamera>();
                    liveProjection.lockCapturedView = false;
                    liveProjection.projection = recoveredCamera.projection;
                    liveProjection.width = recoveredCamera.width; liveProjection.height = recoveredCamera.height;
                    liveProjection.Apply();
                    report.issues.Add("Main camera uses last captured view with explicit View/InverseView (main-view selection inferred). Transform is movable; captured projection preserved.");
                }
                else report.issues.Add("No explicit captured View recovered. Main camera framed to bounds (inferred), not original capture pose.");
                string recoveryMessage;
                CaptureMainCameraRecovery.TryApply(capture, camera, out recoveryMessage);
                report.issues.Add(recoveryMessage);
                var gammaDisplay = cameraGo.AddComponent<ReplayGammaDisplay>();
                gammaDisplay.displayShader = Shader.Find("Hidden/RenderMax/ReplayGammaDisplay");
                // Leave OFF initially; this is a display override for non-sRGB data,
                // not an inferred property of the captured render target.
                gammaDisplay.linearDisplayAsGamma = false;
                camera.depthTextureMode = DepthTextureMode.None; cameraGo.tag = "MainCamera";
                var controller = cameraGo.AddComponent<CaptureReplayController>(); controller.events = states.ToArray(); controller.clearBeforeReplay = true; controller.clearColor = Color.clear;
                // Default is an inspectable normal Unity scene; do not also submit the CommandBuffer draws.
                controller.orderedCommandBufferReplay = false;
                controller.limitations = "Phase 1: native MeshRenderers with Standard Specular and live Unity camera. No CommandBuffer replay. Shader reconstruction belongs to phase 2.";
                controller.Rebuild();
                report.scene = folder + "/CaptureReplay.unity";
                EditorSceneManager.SaveScene(scene, report.scene);
                File.WriteAllText(folder + "/ReplayReport.json", JsonUtility.ToJson(report, true));
                AssetDatabase.SaveAssets(); AssetDatabase.Refresh(); EditorUtility.ClearProgressBar(); Selection.activeGameObject = root;
            }
            return report.scene;
        }
    }
}
