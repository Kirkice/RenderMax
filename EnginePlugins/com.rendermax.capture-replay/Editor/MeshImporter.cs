using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using UnityEngine;
using UnityEngine.Rendering;

namespace RenderMax.CaptureReplay
{
    public static class MeshImporter
    {
        // CSV parser supports quoted commas and escaped quotes; exporter has one row per line.
        public static string[] ParseRow(string text)
        {
            var fields = new List<string>(); var value = new StringBuilder(); bool quoted = false;
            for (int i = 0; i < text.Length; ++i)
            {
                char c = text[i];
                if (c == '"')
                {
                    if (quoted && i + 1 < text.Length && text[i + 1] == '"') { value.Append('"'); ++i; }
                    else quoted = !quoted;
                }
                else if (c == ',' && !quoted) { fields.Add(value.ToString()); value.Length = 0; }
                else value.Append(c);
            }
            if (quoted) throw new InvalidDataException("Unclosed CSV quote.");
            fields.Add(value.ToString()); return fields.ToArray();
        }
        static float Number(string s)
        {
            float v = float.Parse(s, CultureInfo.InvariantCulture);
            if (float.IsNaN(v) || float.IsInfinity(v)) throw new InvalidDataException("Non-finite vertex component.");
            return v;
        }
        static int Semantic(string name, string semantic)
        {
            name = name.ToUpperInvariant(); int at = name.LastIndexOf(semantic, StringComparison.Ordinal);
            if (at < 0) return -1;
            string suffix = name.Substring(at + semantic.Length);
            int n; return suffix.Length == 0 ? 0 : int.TryParse(suffix, out n) ? n : -1;
        }
        public static Mesh Build(PackageReader reader, MeshDescription desc, List<string> warnings)
        {
            if (desc.stage != "vsin" || desc.coordinateConversion != "none")
                throw new NotSupportedException("Only unconverted VS input meshes are supported.");
            MeshTopology topology;
            switch (desc.topology)
            {
                case "Triangle List": topology = MeshTopology.Triangles; break;
                case "Line List": topology = MeshTopology.Lines; break;
                case "Point List": topology = MeshTopology.Points; break;
                default: throw new NotSupportedException("Topology: " + desc.topology);
            }
            var lines = File.ReadAllLines(reader.FilePath(desc.vertices));
            if (lines.Length != desc.vertexCount + 1 || desc.vertexCount <= 0)
                throw new InvalidDataException("Vertex count mismatch or empty mesh.");
            var header = ParseRow(lines[0]);
            var rows = new string[desc.vertexCount][];
            for (int i = 0; i < rows.Length; ++i)
            {
                rows[i] = ParseRow(lines[i + 1]);
                if (rows[i].Length != header.Length || int.Parse(rows[i][0], CultureInfo.InvariantCulture) != i)
                    throw new InvalidDataException("Invalid vertex row.");
            }
            var mesh = new Mesh { name = "Captured VS Input", indexFormat = IndexFormat.UInt32 };
            try
            {
                bool position = false; int column = 2;
                var used = new HashSet<string>();
                foreach (var attr in desc.attributes)
                {
                    if (attr.perInstance) { warnings.Add("Instance attribute retained in source only: " + attr.name); continue; }
                    int n = attr.components;
                    if (n < 1 || n > 4 || column + n > header.Length) throw new InvalidDataException("Invalid attribute width.");
                    for (int c = 0; c < n; ++c)
                        if (header[column + c] != attr.name + "." + c) throw new InvalidDataException("Attribute/header mismatch.");
                    var values = new List<Vector4>(rows.Length);
                    for (int i = 0; i < rows.Length; ++i)
                    {
                        var v = Vector4.zero;
                        for (int c = 0; c < n; ++c) v[c] = Number(rows[i][column + c]);
                        values.Add(v);
                    }
                    column += n;
                    int uv = Semantic(attr.name, "TEXCOORD");
                    string key = null;
                    if (Semantic(attr.name, "POSITION") == 0)
                    {
                        if (n != 3) throw new NotSupportedException("Mesh.asset needs 3-component position; use raw buffer replay for other inputs.");
                        key = "position"; mesh.SetVertices(values.ConvertAll(v => new Vector3(v.x, v.y, v.z))); position = true;
                    }
                    else if (Semantic(attr.name, "NORMAL") == 0 && n == 3)
                    { key = "normal"; mesh.SetNormals(values.ConvertAll(v => new Vector3(v.x, v.y, v.z))); }
                    else if (Semantic(attr.name, "TANGENT") == 0 && n == 4)
                    { key = "tangent"; mesh.SetTangents(values); }
                    else if (Semantic(attr.name, "COLOR") == 0 && n == 4)
                    { key = "color"; mesh.SetColors(values.ConvertAll(v => new Color(v.x, v.y, v.z, v.w))); }
                    else if (uv >= 0 && uv < 8)
                    {
                        key = "uv" + uv;
                        if (n == 2) mesh.SetUVs(uv, values.ConvertAll(v => new Vector2(v.x, v.y)));
                        else if (n == 3) mesh.SetUVs(uv, values.ConvertAll(v => new Vector3(v.x, v.y, v.z)));
                        else mesh.SetUVs(uv, values);
                    }
                    else warnings.Add("Unmapped attribute retained in CSV/raw buffer: " + attr.name);
                    if (key != null && !used.Add(key)) throw new InvalidDataException("Ambiguous semantic: " + attr.name);
                }
                if (!position) throw new NotSupportedException("No recognized POSITION semantic; explicit mapping required.");
                if (column != header.Length) throw new InvalidDataException("Unconsumed CSV columns.");
                var indexLines = File.ReadAllLines(reader.FilePath(desc.indices));
                if (indexLines.Length != desc.indexCount + 1 || indexLines[0].TrimEnd('\r') != "element,index,rawIndex")
                    throw new InvalidDataException("Index CSV mismatch.");
                var indices = new int[desc.indexCount];
                for (int i = 0; i < indices.Length; ++i)
                {
                    var row = ParseRow(indexLines[i + 1]);
                    if (row.Length != 3 || int.Parse(row[0], CultureInfo.InvariantCulture) != i) throw new InvalidDataException("Invalid index row.");
                    indices[i] = int.Parse(row[1], CultureInfo.InvariantCulture);
                    if (indices[i] < 0 || indices[i] >= desc.vertexCount) throw new InvalidDataException("Index out of range.");
                }
                if ((topology == MeshTopology.Triangles && indices.Length % 3 != 0) ||
                    (topology == MeshTopology.Lines && indices.Length % 2 != 0)) throw new InvalidDataException("Incomplete primitive.");
                mesh.SetIndices(indices, topology, 0, false); mesh.RecalculateBounds();
                if (desc.instanceCount > 1) warnings.Add("Mesh.asset represents geometry only; instancing is not baked.");
                return mesh;
            }
            catch { UnityEngine.Object.DestroyImmediate(mesh); throw; }
        }
    }
}
