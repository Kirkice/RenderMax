using System;
using System.Collections.Generic;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    // Groups by exact marker identity/path, never by a guessed engine pass type.
    public sealed class CapturePassHierarchy
    {
        [Serializable] class EventIndex { public CapturePassInfo[] events; }
        readonly Transform root;
        readonly Dictionary<uint, CapturePassInfo> events = new Dictionary<uint, CapturePassInfo>();
        readonly Dictionary<string, Transform> groups = new Dictionary<string, Transform>();
        bool reportedMissing;
        public int GroupCount { get { return groups.Count; } }
        public CapturePassHierarchy(Transform root, PackageReader reader = null)
        {
            this.root = root;
            if (reader == null || string.IsNullOrEmpty(reader.Manifest.events)) return;
            var index = reader.Read<EventIndex>(reader.Manifest.events);
            foreach (var item in index.events ?? new CapturePassInfo[0]) events[item.eventId] = item;
        }
        public CapturePassInfo Read(PackageReader reader, DrawEntry entry)
        {
            var info = reader.Read<CapturePassInfo>(entry.path);
            CapturePassInfo indexed;
            if (!HasIdentity(info) && events.TryGetValue(entry.eventId, out indexed) && HasIdentity(indexed)) info = indexed;
            return info;
        }
        static bool HasIdentity(CapturePassInfo info)
        {
            return info != null && info.passEventId != 0 && !string.IsNullOrEmpty(info.passId);
        }
        public Transform GetParent(CapturePassInfo info, List<string> issues)
        {
            if (!HasIdentity(info))
            {
                if (info != null && info.passId == "ungrouped" && info.passNameSource == "ungrouped")
                    return Group("ungrouped", root, "Ungrouped [No enclosing marker]", 0, "ungrouped", "Ungrouped", new string[0], new uint[0], "ungrouped");
                if (!reportedMissing)
                {
                    issues.Add("Pass metadata unavailable for some events. Grouped under Ungrouped; re-export with marker ancestry to restore pass membership. No Shadow/GBuffer classification was guessed.");
                    reportedMissing = true;
                }
                return Group("ungrouped", root, "Ungrouped [Pass metadata unavailable]", 0, "ungrouped", "Ungrouped", new string[0], new uint[0], "missing");
            }
            var names = info.markerPath ?? new string[0];
            var ids = info.markerEventIds ?? new uint[0];
            if (names.Length == 0 || names.Length != ids.Length || ids[ids.Length-1] != info.passEventId || !ValidIds(ids))
            {
                // Retain an explicit pass identity even if an older producer omitted full ancestry.
                issues.Add("E" + info.eventId + ": incomplete marker ancestry; grouped by explicit passId " + info.passId);
                return Group("pass:" + info.passId, root, Display(info.passName, info.passEventId), info.passEventId,
                    info.passId, info.passName, names, ids, info.passNameSource);
            }
            Transform parent = root;
            string key = "marker";
            for (int i=0;i<ids.Length;++i)
            {
                key += "/" + ids[i];
                var path = new string[i+1]; Array.Copy(names, path, i+1);
                var pathIds = new uint[i+1]; Array.Copy(ids, pathIds, i+1);
                parent = Group(key, parent, Display(names[i], ids[i]), ids[i], "marker:" + ids[i], names[i], path, pathIds, "marker");
            }
            return parent;
        }
        static bool ValidIds(uint[] ids)
        {
            var seen = new HashSet<uint>();
            foreach (uint id in ids) if (id == 0 || !seen.Add(id)) return false;
            return true;
        }
        static string Display(string name, uint id)
        {
            if (string.IsNullOrEmpty(name)) name = "Unnamed marker";
            return name.Replace('/', '\u2215').Replace('\n', ' ').Replace('\r', ' ') + " [E" + id + "]";
        }
        Transform Group(string key, Transform parent, string label, uint marker, string id, string name, string[] path, uint[] pathIds, string source)
        {
            Transform existing;
            if (groups.TryGetValue(key, out existing)) return existing;
            var go = new GameObject(label);
            go.transform.SetParent(parent, false);
            // All groups remain enabled; grouping is metadata/UI, not a rendering policy.
            var state = go.AddComponent<CapturePassGroup>();
            state.markerEventId = marker; state.passId = id; state.passName = name;
            state.markerPath = path; state.markerEventIds = pathIds; state.nameSource = source;
            groups.Add(key, go.transform);
            return go.transform;
        }
        public static void Apply(CaptureDrawState state, CapturePassInfo info)
        {
            state.parentEventId = info == null ? 0 : info.parentEventId;
            state.passEventId = info == null ? 0 : info.passEventId;
            state.passId = HasIdentity(info) ? info.passId : "ungrouped";
            state.passName = HasIdentity(info) ? info.passName : "Ungrouped";
            state.passNameSource = info != null && !string.IsNullOrEmpty(info.passNameSource) ? info.passNameSource : "missing";
            state.markerPath = info == null ? new string[0] : info.markerPath ?? new string[0];
            state.markerEventIds = info == null ? new uint[0] : info.markerEventIds ?? new uint[0];
        }
    }
}
