using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // Recover a rigid perspective view from OpenGL VP rows, accepting only
    // conventional finite projections that reconstruct the captured VP.
    public static class CaptureMainCameraRecovery
    {
        [Serializable] class Pipeline { public Viewport[] viewports; }
        [Serializable] class Viewport { public float width, height; }
        sealed class Candidate
        {
            public Vector3 position; public Quaternion rotation;
            public Matrix4x4 projection, vp;
            public float fov, aspect, near, far;
            public int width, height, votes; public uint eventId;
        }
        [MenuItem("Tools/RenderMax/Advanced/Camera/Recover Main Camera")]
        static void RecoverOpenScene()
        {
            var controllers = UnityEngine.Object.FindObjectsOfType<CaptureReplayController>();
            if (controllers.Length != 1) throw new InvalidOperationException("Open exactly one capture replay scene.");
            var controller = controllers[0];
            var capture = AssetDatabase.LoadAssetAtPath<CaptureAsset>("Assets/RenderMaxCaptures/Capture01/Capture.asset");
            if (capture == null) throw new InvalidOperationException("Capture01/Capture.asset not found.");
            var camera = controller.GetComponent<Camera>();
            Undo.RecordObjects(new UnityEngine.Object[] { camera, camera.transform }, "Recover captured main camera");
            string message;
            if (!TryApply(capture, camera, out message)) throw new InvalidOperationException(message);
            EditorSceneManager.MarkSceneDirty(camera.gameObject.scene);
            Debug.Log(message, camera);
        }
        public static bool TryApply(CaptureAsset capture, Camera camera, out string message)
        {
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            // Formula below assumes the OpenGL [-1,1] clip-depth convention.
            if (!File.ReadAllText(reader.FilePath("capture.json")).Contains("OpenGL"))
            { message = "Camera recovery currently supports OpenGL captures only."; return false; }
            var candidates = new List<Candidate>();
            foreach (var entry in reader.Manifest.draws)
            {
                var draw = reader.Read<ReplayDraw>(entry.path);
                var constants = reader.Read<ConstantDocument>(draw.constants);
                var variable = SceneRebuilder.Optional(constants, "hlslcc_mtx4x4unity_MatrixVP");
                if (variable == null) continue;
                var vp = SceneRebuilder.Matrix(SceneRebuilder.Columns(variable));
                Candidate candidate;
                if (!Decompose(vp, out candidate)) continue;
                var existing = candidates.Find(x => Difference(x.vp, vp) < 0.00001f);
                if (existing != null) { ++existing.votes; continue; }
                candidate.eventId = entry.eventId; candidate.votes = 1;
                // Pipeline is not part of ReplayDraw's minimal schema.
                var json = JsonUtility.FromJson<PipelineReference>(File.ReadAllText(reader.FilePath(entry.path)));
                if (!string.IsNullOrEmpty(json.pipeline))
                {
                    var pipeline = reader.Read<Pipeline>(json.pipeline);
                    if (pipeline.viewports != null && pipeline.viewports.Length > 0)
                    {
                        candidate.width = Mathf.RoundToInt(pipeline.viewports[0].width);
                        candidate.height = Mathf.RoundToInt(pipeline.viewports[0].height);
                    }
                }
                candidates.Add(candidate);
            }
            candidates.Sort((a,b) => b.votes.CompareTo(a.votes));
            if (candidates.Count == 0) { message = "No conventional perspective VP passed reconstruction checks; camera left unchanged."; return false; }
            if (candidates.Count > 1 && candidates[0].votes == candidates[1].votes)
            { message = "Ambiguous main view (equal event counts); camera left unchanged."; return false; }
            var best = candidates[0];
            var old = camera.GetComponent<CapturedCamera>();
            if (old != null) { Undo.RecordObject(old, "Disable legacy captured camera matrix override"); old.enabled = false; }
            camera.ResetWorldToCameraMatrix(); camera.ResetProjectionMatrix(); camera.ResetCullingMatrix();
            camera.transform.SetPositionAndRotation(best.position, best.rotation);
            camera.orthographic = false; camera.fieldOfView = best.fov; camera.aspect = best.aspect;
            camera.nearClipPlane = best.near; camera.farClipPlane = best.far;
            // Preserve any off-axis terms, not just the Inspector FOV.
            camera.projectionMatrix = best.projection;
            var persistent = old != null ? old : Undo.AddComponent<CapturedCamera>(camera.gameObject);
            persistent.lockCapturedView = false;
            persistent.conservativeCulling = false;
            persistent.projection = best.projection;
            persistent.width = best.width > 0 ? best.width : 2118;
            persistent.height = best.height > 0 ? best.height : 978;
            persistent.enabled = true;
            persistent.Apply();
            EditorUtility.SetDirty(persistent);
            message = "Recovered main perspective camera from E" + best.eventId + " (" + best.votes +
                " events; main-view selection inferred). Position=" + best.position.ToString("F6") +
                ", rotation=" + best.rotation.eulerAngles.ToString("F6") + ", FOVy=" + best.fov +
                ", aspect=" + best.aspect + ", near=" + best.near + ", far=" + best.far +
                ", source viewport=" + best.width + "x" + best.height +
                ". Other VP groups are not represented by this camera. GameView dimensions must be set separately.";
            return true;
        }
        [Serializable] class PipelineReference { public string pipeline; }
        static bool Decompose(Matrix4x4 original, out Candidate c)
        {
            c = null;
            Vector3 forward = original.GetRow(3); float scale = forward.magnitude;
            if (scale < 0.00001f) return false;
            var m = original;
            for (int i=0;i<16;++i) m[i] /= scale;
            forward = m.GetRow(3);
            Vector3 row0 = m.GetRow(0), row1 = m.GetRow(1), row2 = m.GetRow(2);
            float ox = Vector3.Dot(row0,forward), oy = Vector3.Dot(row1,forward);
            var right = row0 - ox*forward; var up = row1 - oy*forward;
            float sx = right.magnitude, sy = up.magnitude;
            if (sx < 0.00001f || sy < 0.00001f) return false;
            right /= sx; up /= sy;
            if (Mathf.Abs(Vector3.Dot(right,up)) > 0.0001f || Vector3.Dot(Vector3.Cross(right,up),forward) < 0.9999f) return false;
            float a = Vector3.Dot(row2,forward);
            if ((row2-a*forward).magnitude > 0.0001f) return false;
            var basis = Matrix4x4.identity;
            basis.SetRow(0,new Vector4(row0.x,row0.y,row0.z,0));
            basis.SetRow(1,new Vector4(row1.x,row1.y,row1.z,0));
            basis.SetRow(2,new Vector4(forward.x,forward.y,forward.z,0));
            if (Mathf.Abs(basis.determinant) < 0.00001f) return false;
            Vector3 position = basis.inverse.MultiplyVector(new Vector3(-m.m03,-m.m13,-m.m33));
            float b = m.m23 + Vector3.Dot(row2,position);
            float near = -b/(a+1), far = -b/(a-1);
            if (!(near > 0 && far > near && far < 1000000)) return false;
            var rotation = Quaternion.LookRotation(forward,up);
            var view = Matrix4x4.Scale(new Vector3(1,1,-1)) * Matrix4x4.TRS(position,rotation,Vector3.one).inverse;
            var projection = m * view.inverse;
            var expected = Matrix4x4.Frustum(near*(-1-ox)/sx,near*(1-ox)/sx,
                near*(-1-oy)/sy,near*(1-oy)/sy,near,far);
            if (Difference(expected,projection)>0.0001f || Difference(expected*view,m)>0.0001f) return false;
            c = new Candidate { position=position,rotation=rotation,projection=projection,vp=original,
                fov=2*Mathf.Atan(1/sy)*Mathf.Rad2Deg,aspect=sy/sx,near=near,far=far };
            return true;
        }
        static float Difference(Matrix4x4 a, Matrix4x4 b)
        {
            float delta=0, size=1;
            for(int i=0;i<16;++i) { delta=Mathf.Max(delta,Mathf.Abs(a[i]-b[i]));size=Mathf.Max(size,Mathf.Abs(b[i])); }
            return delta/size;
        }
    }
}
