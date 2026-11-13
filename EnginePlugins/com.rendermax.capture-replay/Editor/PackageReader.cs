using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class FileEntry { public string path, sha256; public long bytes; }
    [Serializable] public class DrawEntry { public uint eventId; public string path; }
    [Serializable] public class PackageManifest
    {
        public string format, schemaVersion, status, scope, capture, events, report;
        public FileEntry[] files;
        public DrawEntry[] draws;
    }
    [Serializable] public class ResourceReference { public string resourceId, path; }
    [Serializable] public class DrawDescription
    {
        public uint eventId;
        public string name, kind, pipeline, constants, bindings, mesh, meshStatus;
        public ResourceReference[] textures, buffers;
    }
    [Serializable] public class TextureImage { public string path; public int mip, layer; }
    [Serializable] public class RawTextureSubresource { public string path; public int mip, layer, sample, bytes; }
    [Serializable] public class TextureDescription
    {
        public string resourceId, format;
        public uint eventId;
        public int width, height, depth, arraySize, mips, samples;
        public bool srgb, cubemap;
        public TextureImage[] images;
        public RawTextureSubresource[] rawSubresources;
    }
    [Serializable] public class VertexAttribute
    {
        public string name, format;
        public int components, slot, byteOffset, instanceRate;
        public bool perInstance, generic, used;
    }
    [Serializable] public class VertexStream
    {
        public int slot, byteStride;
        public string resourceId, path, byteOffset;
    }
    [Serializable] public class MeshDescription
    {
        public string stage, topology, vertices, indices, instances, indexBuffer, coordinateConversion;
        public int vertexCount, indexCount, instanceCount, baseVertex, indexOffset, vertexOffset, instanceOffset;
        public VertexAttribute[] attributes;
        public VertexStream[] streams;
    }
    [Serializable] public class ImportReport
    {
        public string source, destination;
        public bool integrityPassed;
        public List<string> imported = new List<string>();
        public List<string> unsupported = new List<string>();
        public List<string> errors = new List<string>();
    }

    public sealed class PackageReader
    {
        public readonly string Root;
        public readonly PackageManifest Manifest;
        readonly HashSet<string> listed = new HashSet<string>(StringComparer.Ordinal);

        public PackageReader(string manifestPath)
        {
            Root = Path.GetFullPath(Path.GetDirectoryName(manifestPath));
            Manifest = JsonUtility.FromJson<PackageManifest>(File.ReadAllText(manifestPath));
            if (Manifest == null || Manifest.format != "rendermax.asset-package")
                throw new InvalidDataException("Not a RenderMax asset package.");
            if (Manifest.schemaVersion == null || !Manifest.schemaVersion.StartsWith("1.", StringComparison.Ordinal))
                throw new InvalidDataException("Unsupported schema version: " + Manifest.schemaVersion);
            if (Manifest.status != "completed" && Manifest.status != "completedWithWarnings")
                throw new InvalidDataException("Package is incomplete: " + Manifest.status);
            if (Manifest.files == null || Manifest.draws == null)
                throw new InvalidDataException("Missing file or draw inventory.");
            foreach (var entry in Manifest.files)
            {
                Resolve(entry.path);
                if (!listed.Add(entry.path)) throw new InvalidDataException("Duplicate path: " + entry.path);
            }
        }

        public string Resolve(string relative)
        {
            if (string.IsNullOrEmpty(relative) || Path.IsPathRooted(relative) || relative.Contains("\\") || relative.Contains(":"))
                throw new InvalidDataException("Invalid relative package path: " + relative);
            var full = Path.GetFullPath(Path.Combine(Root, relative));
            if (!full.StartsWith(Root + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("Path escapes package root: " + relative);
            // Do not permit symlinks/junctions to bypass the root check.
            for (var p = full; p != Root && p != null; p = Path.GetDirectoryName(p))
                if ((File.Exists(p) || Directory.Exists(p)) && (File.GetAttributes(p) & FileAttributes.ReparsePoint) != 0)
                    throw new InvalidDataException("Linked paths are not accepted: " + relative);
            return full;
        }

        public string FilePath(string relative)
        {
            if (!listed.Contains(relative)) throw new InvalidDataException("Unlisted reference: " + relative);
            return Resolve(relative);
        }
        public T Read<T>(string relative) { return JsonUtility.FromJson<T>(File.ReadAllText(FilePath(relative))); }

        public void Validate(Action<int, int> progress = null)
        {
            for (int i = 0; i < Manifest.files.Length; ++i)
            {
                var entry = Manifest.files[i];
                var path = FilePath(entry.path);
                if (!File.Exists(path) || new FileInfo(path).Length != entry.bytes)
                    throw new InvalidDataException("Missing file or size mismatch: " + entry.path);
                using (var hash = SHA256.Create())
                using (var stream = File.OpenRead(path))
                {
                    var actual = BitConverter.ToString(hash.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
                    if (!string.Equals(actual, entry.sha256, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("SHA-256 mismatch: " + entry.path);
                }
                if (progress != null) progress(i + 1, Manifest.files.Length);
            }
            foreach (var entry in Manifest.draws)
            {
                var draw = Read<DrawDescription>(entry.path);
                if (draw.eventId != entry.eventId) throw new InvalidDataException("Draw event mismatch.");
                FilePath(draw.pipeline); FilePath(draw.constants); FilePath(draw.bindings);
                if (!string.IsNullOrEmpty(draw.mesh)) FilePath(draw.mesh);
                foreach (var t in draw.textures ?? new ResourceReference[0]) FilePath(t.path);
                foreach (var b in draw.buffers ?? new ResourceReference[0]) FilePath(b.path);
            }
        }
    }
}
