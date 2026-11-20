using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RenderMax.CaptureReplay
{
    public static class CapturePassHierarchyChecks
    {
        [Serializable] class Report { public int passed; public string error; public List<string> tests = new List<string>(); }
        [Serializable] class Index { public CapturePassInfo[] events; }
        static CapturePassInfo Info(uint draw, uint marker, string name, uint[] ids, string[] path)
        {
            return new CapturePassInfo { eventId = draw, parentEventId = marker, passEventId = marker,
                passId = "marker:" + marker, passName = name, passNameSource = "marker", markerEventIds = ids, markerPath = path };
        }
        // Batch entry; uses only a temporary additive scene and temporary metadata files.
        public static void Run()
        {
            var report = new Report();
            var original = SceneManager.GetActiveScene();
            var testScene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Additive);
            string temporary = Path.GetFullPath("Temp/RenderMaxPassChecks-" + Guid.NewGuid().ToString("N"));
            try
            {
                SceneManager.SetActiveScene(testScene);
                Directory.CreateDirectory(temporary);
                Action<bool,string> check = (ok,name) => { if (!ok) throw new Exception(name); ++report.passed; report.tests.Add(name); };
                var root = new GameObject("Pass hierarchy test");
                var hierarchy = new CapturePassHierarchy(root.transform);
                var issues = new List<string>();
                var a = Info(104,9001,"ShadowPass",new uint[] {9000,9001},new[] {"Frame","ShadowPass"});
                Transform parent = hierarchy.GetParent(a,issues);
                check(parent.name.Contains("ShadowPass") && parent.parent.name.Contains("Frame"), "Marker path creates nested named pass parents");
                check(hierarchy.GetParent(Info(128,9001,"ShadowPass",a.markerEventIds,a.markerPath),issues) == parent, "Draws in the same pass share one parent");
                var b = Info(146,9003,"ShadowPass",new uint[] {9000,9003},new[] {"Frame","ShadowPass"});
                check(hierarchy.GetParent(b,issues) != parent, "Same-named distinct passes are not merged");
                var nested = Info(162,9002,"Cascade 0",new uint[] {9000,9001,9002},new[] {"Frame","ShadowPass","Cascade 0"});
                check(hierarchy.GetParent(nested,issues).parent == parent, "Nested markers retain their enclosing pass");
                var go = new GameObject("E00000104"); go.transform.SetParent(parent,false); go.transform.localPosition = new Vector3(3,4,5);
                var state = go.AddComponent<CaptureDrawState>(); CapturePassHierarchy.Apply(state,a);
                check(go.transform.position == new Vector3(3,4,5), "Grouping preserves draw world transforms under identity parents");
                check(state.passId == "marker:9001" && state.parentEventId == 9001 && state.markerEventIds.Length == 2, "Draw component retains pass identity and complete ancestry");
                check(go.activeInHierarchy, "Pass groups are enabled by default"); parent.gameObject.SetActive(false);
                check(!go.activeInHierarchy, "Disabling a pass excludes its descendant draw items"); parent.gameObject.SetActive(true);
                check(go.activeInHierarchy, "Re-enabling a pass restores its draw items");
                Transform unknown = hierarchy.GetParent(new CapturePassInfo { eventId = 1, passName = "ShadowPass" },issues);
                check(unknown.GetComponent<CapturePassGroup>().passId == "ungrouped", "Names without marker identity are not guessed into a pass");
                check(hierarchy.GetParent(null,issues) == unknown, "Legacy events share an explicit ungrouped parent");
                check(issues.Count == 1, "Missing legacy metadata produces one explanatory warning");
                var depthOnly = Info(200,9010,"Depth-only Pass #1",new uint[] {9010},new[] {"Depth-only Pass #1"});
                check(hierarchy.GetParent(depthOnly,issues).GetComponent<CapturePassGroup>().passName == "Depth-only Pass #1", "Depth-only markers are not mislabeled as ShadowPass");
                string json = JsonUtility.ToJson(a);
                check(JsonUtility.FromJson<CapturePassInfo>(json).markerEventIds[1] == 9001, "Pass metadata survives Unity JSON round-trip");
                File.WriteAllText(temporary + "/draw.json", "{\"eventId\":104}");
                File.WriteAllText(temporary + "/events.json", JsonUtility.ToJson(new Index { events = new[] { a } }));
                File.WriteAllText(temporary + "/manifest.json", JsonUtility.ToJson(new PackageManifest { format = "rendermax.asset-package", schemaVersion = "1.0", status = "completed", events = "events.json", files = new[] { new FileEntry {path="draw.json"},new FileEntry {path="events.json"} }, draws = new[] { new DrawEntry {eventId=104,path="draw.json"} } }));
                var reader = new PackageReader(temporary + "/manifest.json");
                var indexed = new CapturePassHierarchy(new GameObject("Indexed").transform,reader);
                check(indexed.Read(reader, reader.Manifest.draws[0]).passEventId == 9001, "Explicit event-index metadata is a supported fallback");
                File.WriteAllText(temporary + "/events.json", "{\"events\":[{\"eventId\":9001,\"name\":\"ShadowPass\",\"flags\":\"64\"},{\"eventId\":104,\"name\":\"\",\"flags\":\"2\"}]}");
                var legacy = new CapturePassHierarchy(new GameObject("Legacy").transform,reader);
                var legacyInfo = legacy.Read(reader,reader.Manifest.draws[0]);
                check(legacy.GetParent(legacyInfo,new List<string>()).GetComponent<CapturePassGroup>().nameSource == "missing", "Old flat event order is never treated as reliable pass ancestry");
            }
            catch (Exception e) { report.error = e.ToString(); }
            finally
            {
                if (original.IsValid() && original.isLoaded) SceneManager.SetActiveScene(original);
                EditorSceneManager.CloseScene(testScene,true);
                if (Directory.Exists(temporary)) Directory.Delete(temporary,true);
                Directory.CreateDirectory("Artifacts"); File.WriteAllText("Artifacts/PassHierarchyChecks.json", JsonUtility.ToJson(report,true));
            }
            if (!string.IsNullOrEmpty(report.error)) throw new Exception(report.error);
            Debug.Log("Capture Pass hierarchy checks passed: " + report.passed);
        }
    }
}
