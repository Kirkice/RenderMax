using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    public static class SceneRebuilder
    {
        const string ShaderName = "RenderMax/Captured/Event63Albedo";

        public static string Create(CaptureAsset capture, ImportedDraw imported, bool saveAndOpen)
        {
            if (capture == null || imported == null || imported.mesh == null)
                throw new InvalidOperationException("The selected draw has no imported Mesh.asset.");
            var reader = new PackageReader(Path.GetFullPath(capture.sourceManifest));
            var draw = reader.Read<ReplayDraw>(imported.sourceDraw);
            if (draw.shaders == null || draw.shaders.Vertex == null || draw.shaders.Pixel == null)
                throw new NotSupportedException("The draw has no vertex/pixel shader pair.");

            if (saveAndOpen && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
                throw new OperationCanceledException("Scene creation cancelled.");
            // Validate the supported shader pair and bindings before modifying assets or scenes.
            using (var check = new DrawReplay())
                check.Render(capture, imported, Color.clear, false, FilterMode.Bilinear, TextureWrapMode.Repeat);

            string captureFolder = Path.GetDirectoryName(AssetDatabase.GetAssetPath(capture)).Replace('\\', '/');
            string folder = AssetDatabase.GenerateUniqueAssetPath(captureFolder + "/Reconstruction/E" + imported.eventId.ToString("D8") + "_Scene");
            Directory.CreateDirectory(folder);
            string shaderPath = folder + "/Captured.shader";
            File.WriteAllText(shaderPath, ShaderSource());
            AssetDatabase.ImportAsset(shaderPath, ImportAssetOptions.ForceSynchronousImport);
            var shader = AssetDatabase.LoadAssetAtPath<Shader>(shaderPath);
            if (shader == null || !shader.isSupported) throw new InvalidOperationException("Generated shader failed to compile: " + shaderPath);

            var constants = reader.Read<ConstantDocument>(draw.constants);
            var objectToWorld = Columns(Constant(constants, "Vertex", "hlslcc_mtx4x4unity_ObjectToWorld"));
            var viewProjection = Columns(Constant(constants, "Vertex", "hlslcc_mtx4x4unity_MatrixVP"));
            var material = new Material(shader) { name = "Captured Event " + imported.eventId + " Material" };
            var textureST = Vector(Constant(constants, "Pixel", "_AlbedoMap_ST"));
            // These uniforms are global, deliberately absent from material properties.
            material.SetInt("_RMCull", 2); material.SetInt("_RMZTest", 4); material.SetInt("_RMZWrite", 1); material.SetInt("_RMColorMask", 15);
            Texture2D texture = FindBoundTexture(reader, draw, captureFolder);
            if (texture != null) material.SetTexture("_RMTexture", texture);
            string materialPath = folder + "/Captured.mat";
            AssetDatabase.CreateAsset(material, materialPath);

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            var root = new GameObject("RenderMax Event " + imported.eventId);
            var meshObject = new GameObject("Captured Mesh E" + imported.eventId);
            meshObject.transform.SetParent(root.transform, false);
            Matrix4x4 model = Matrix(objectToWorld);
            ApplyTransform(meshObject.transform, model);
            meshObject.AddComponent<MeshFilter>().sharedMesh = imported.mesh;
            meshObject.AddComponent<MeshRenderer>().sharedMaterial = material;
            var cameraObject = new GameObject("Captured Camera (derived)");
            var camera = cameraObject.AddComponent<Camera>();
            camera.name = "Captured Camera";
            camera.clearFlags = CameraClearFlags.SolidColor; camera.backgroundColor = Color.clear;
            camera.orthographic = false; camera.fieldOfView = 60; camera.nearClipPlane = 0.01f; camera.farClipPlane = 10000;
            var pipeline = reader.Read<ReplayPipeline>(draw.pipeline);
            string cameraStatus;
            Matrix4x4 view, projection;
            var viewConstant = Optional(constants, "hlslcc_mtx4x4unity_MatrixV");
            var inverseView = Optional(constants, "hlslcc_mtx4x4unity_MatrixInvV");
            if (viewConstant != null || inverseView != null)
            {
                view = viewConstant != null ? Matrix(Columns(viewConstant)) : Matrix(Columns(inverseView)).inverse;
                projection = Matrix(viewProjection) * view.inverse;
                cameraStatus = "Confirmed: view recovered from captured View/InverseView. Projection derived as VP * inverse(View).";
            }
            else
            {
                // A VP factorization is not unique. Preserve exact clip/culling transform without inventing pose.
                view = Matrix4x4.identity;
                projection = Matrix(viewProjection);
                cameraStatus = "Unrecoverable: this draw exports VP only, so camera position/rotation cannot be uniquely recovered. Explicit identity View + combined VP projection preserves clip coordinates, NOT original camera pose.";
            }
            Matrix4x4 cameraWorld = view.inverse;
            cameraObject.transform.position = cameraWorld.GetColumn(3);
            cameraObject.transform.rotation = Quaternion.LookRotation(-cameraWorld.GetColumn(2), cameraWorld.GetColumn(1));
            var cameraState = cameraObject.AddComponent<CapturedCamera>();
            cameraState.view = view; cameraState.projection = projection;
            if (pipeline.viewports != null && pipeline.viewports.Length > 0)
            {
                cameraState.width = (int)pipeline.viewports[0].width;
                cameraState.height = (int)pipeline.viewports[0].height;
            }
            cameraState.Apply();
            camera.targetDisplay = 0; cameraObject.tag = "MainCamera";
            var globals = root.AddComponent<GlobalConstantBufferDriver>();
            var floatConstants = new List<GlobalConstantBufferDriver.VectorConstant>();
            foreach (var block in constants.blocks ?? new ConstantBlock[0])
                foreach (var variable in block.variables ?? new ConstantVariable[0])
                    CollectFloats(variable, floatConstants);
            globals.floatConstants = floatConstants.ToArray();
            globals.objectToWorld = objectToWorld;
            globals.viewProjection = viewProjection;
            globals.textureST = textureST;
            globals.Apply();
            File.WriteAllText(folder + "/SceneReport.txt", "Single-draw scene. Confirmed: Mesh Transform decomposed from captured ObjectToWorld and checked for shear. Shader uses captured local-to-clip arithmetic, not Unity automatic model transform (no double application). GlobalConstantBufferDriver applies float constants, ObjectToWorld, VP and ST. No light created.\n" + cameraStatus + "\nSampler/initial targets remain preview defaults. Raw constant-buffer layout/slot binding and full-frame replay are not implemented.");
            string scenePath = folder + "/Event" + imported.eventId + ".unity";
            EditorSceneManager.SaveScene(scene, scenePath);
            AssetDatabase.SaveAssets(); AssetDatabase.Refresh();
            if (saveAndOpen) EditorSceneManager.OpenScene(scenePath, OpenSceneMode.Single);
            return scenePath;
        }

        internal static Matrix4x4 Matrix(Vector4[] columns)
        {
            var m = new Matrix4x4(); for (int i = 0; i < 4; ++i) m.SetColumn(i, columns[i]); return m;
        }
        internal static void ApplyTransform(Transform transform, Matrix4x4 m)
        {
            Vector3 x = m.GetColumn(0), y = m.GetColumn(1), z = m.GetColumn(2);
            Vector3 scale = new Vector3(x.magnitude, y.magnitude, z.magnitude);
            if (scale.x < 1e-7f || scale.y < 1e-7f || scale.z < 1e-7f) throw new NotSupportedException("Singular object transform.");
            if (Vector3.Dot(Vector3.Cross(x, y), z) < 0) scale.x = -scale.x;
            Quaternion rotation = Quaternion.LookRotation(z / scale.z, y / scale.y);
            Vector3 position = m.GetColumn(3);
            Matrix4x4 rebuilt = Matrix4x4.TRS(position, rotation, scale);
            for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c)
                if (Mathf.Abs(rebuilt[r,c] - m[r,c]) > 0.001f * Mathf.Max(1, Mathf.Abs(m[r,c])))
                    throw new NotSupportedException("Object matrix contains shear/non-TRS terms; cannot represent it as a Unity Transform without baking.");
            transform.localPosition = position; transform.localRotation = rotation; transform.localScale = scale;
        }
        internal static ConstantVariable Optional(ConstantDocument doc, string name)
        {
            foreach (var b in doc.blocks ?? new ConstantBlock[0])
            {
                var found = Find(b.variables, name); if (found != null) return found;
            }
            return null;
        }

        internal static void CollectFloats(ConstantVariable variable, List<GlobalConstantBufferDriver.VectorConstant> result)
        {
            if (variable.type != "float") return;
            if (variable.members != null && variable.members.Length > 0)
            {
                bool vectorArray = true;
                foreach (var member in variable.members)
                    if (member.value == null || member.value.Length == 0 || member.value.Length > 4 || member.type != "float") vectorArray = false;
                if (vectorArray)
                {
                    var values = new Vector4[variable.members.Length];
                    for (int i = 0; i < values.Length; ++i)
                        for (int c = 0; c < variable.members[i].value.Length; ++c) values[i][c] = variable.members[i].value[c];
                    result.Add(new GlobalConstantBufferDriver.VectorConstant { name = variable.name, values = values, array = true });
                }
                else foreach (var member in variable.members) CollectFloats(member, result);
            }
            else if (variable.value != null && variable.value.Length > 0 && variable.value.Length <= 4)
            {
                Vector4 value = Vector4.zero;
                for (int c = 0; c < variable.value.Length; ++c) value[c] = variable.value[c];
                result.Add(new GlobalConstantBufferDriver.VectorConstant { name = variable.name, values = new[] { value } });
            }
        }

        static ConstantVariable Find(ConstantVariable[] values, string name)
        {
            foreach (var v in values ?? new ConstantVariable[0]) { if (v.name == name) return v; var x = Find(v.members, name); if (x != null) return x; } return null;
        }
        static ConstantVariable Constant(ConstantDocument doc, string stage, string name)
        {
            foreach (var b in doc.blocks ?? new ConstantBlock[0]) if (b.stage == stage) { var x = Find(b.variables, name); if (x != null) return x; }
            throw new InvalidDataException("Missing constant: " + stage + "/" + name);
        }
        internal static Vector4 Vector(ConstantVariable v) { if (v == null || v.value == null || v.value.Length != 4) throw new InvalidDataException("Expected float4: " + v.name); return new Vector4(v.value[0], v.value[1], v.value[2], v.value[3]); }
        internal static Vector4[] Columns(ConstantVariable v)
        {
            if (v.members == null || v.members.Length != 4) throw new InvalidDataException("Expected matrix columns: " + v.name);
            var r = new Vector4[4]; for (int i = 0; i < 4; ++i) r[i] = Vector(v.members[i]); return r;
        }
        internal static Texture2D FindBoundTexture(PackageReader reader, ReplayDraw draw, string captureFolder)
        {
            var bindings = reader.Read<BindingDocument>(draw.bindings);
            string id = null; foreach (var b in bindings.descriptors ?? new Binding[0]) if (b.stage == "Pixel" && b.resourceId != "ResourceId::0") id = b.resourceId;
            foreach (var r in draw.textures ?? new ResourceReference[0])
            {
                var t = reader.Read<TextureDescription>(r.path); if (r.resourceId != id || t.images == null || t.images.Length == 0) continue;
                string path = captureFolder + "/Textures/" + t.images[0].path;
                return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            }
            return null;
        }
        static string ShaderSource()
        {
            string template = "Packages/com.rendermax.capture-replay/Editor/Shaders/CapturedAlbedo.shader";
            return File.ReadAllText(template).Replace("Hidden/RenderMax/CapturedAlbedo", ShaderName)
                .Replace("float _RMFlipY;", "static const float _RMFlipY = 1;");
        }
    }
}
