using System;
using System.Collections.Generic;
using UnityEngine;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class ImportedDraw
    {
        public uint eventId;
        public Mesh mesh;
        public List<Texture> texturePreviews = new List<Texture>();
        public string sourceDraw;
        public CapturePassInfo pass;
    }

    // URP remains zero so existing captures keep the current generator's behavior.
    public enum CaptureShaderPipeline { URP = 0, BuiltIn = 1 }

    public sealed class CaptureAsset : ScriptableObject
    {
        public const int ShaderGeneratorVersion = 2;
        public bool MatchesShaderTarget(string targetPipeline, int generatorVersion)
        {
            return generatorVersion == ShaderGeneratorVersion && targetPipeline == ShaderTarget;
        }
        public bool CanReplayInCurrentProject(out string reason)
        {
            var pipeline = UnityEngine.Rendering.GraphicsSettings.currentRenderPipeline;
            string active = "builtin";
            if (pipeline != null)
            {
                active = "unsupported";
                for (Type type = pipeline.GetType(); type != null; type = type.BaseType)
                    if (type.FullName == "UnityEngine.Rendering.Universal.UniversalRenderPipelineAsset") { active = "urp"; break; }
            }
            if (active == ShaderTarget) { reason = ""; return true; }
            reason = "Capture.asset 的 Shader 目标为 " + ShaderTarget + "，当前工程为 " + active +
                "。可生成该目标的资源，但重现前需选择匹配的目标或在匹配管线的工程中打开。";
            return false;
        }
        public string sourceManifest;
        [Tooltip("Shader generation target. Does not change the Unity project's render pipeline.")]
        public CaptureShaderPipeline shaderPipeline = CaptureShaderPipeline.URP;
        public string ShaderTarget
        {
            get
            {
                switch (shaderPipeline)
                {
                    case CaptureShaderPipeline.URP: return "urp";
                    case CaptureShaderPipeline.BuiltIn: return "builtin";
                    default: throw new ArgumentOutOfRangeException("shaderPipeline");
                }
            }
        }
        public string status = "Evidence imported; GPU replay not implemented";
        public List<ImportedDraw> draws = new List<ImportedDraw>();
    }
}
