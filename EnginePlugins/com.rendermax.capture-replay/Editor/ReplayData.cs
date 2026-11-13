using System;

namespace RenderMax.CaptureReplay
{
    [Serializable] public class ShaderPaths { public string Vertex, Pixel; }
    [Serializable] public class ReplayDraw
    {
        public uint eventId;
        public string mesh, pipeline, constants, bindings;
        public ShaderPaths shaders;
        public ResourceReference[] textures;
    }
    [Serializable] public class ConstantVariable
    {
        public string name, type;
        public int rows, columns;
        public float[] value;
        public ConstantVariable[] members;
    }
    [Serializable] public class ConstantBlock { public string stage; public ConstantVariable[] variables; }
    [Serializable] public class ConstantDocument { public ConstantBlock[] blocks; }
    [Serializable] public class Binding { public string stage, resourceId; public int reflectionIndex, arrayElement; }
    [Serializable] public class BindingDocument { public Binding[] descriptors; }
    [Serializable] public class ShaderDocument { public string source; }
    [Serializable] public class RasterState { public string cullMode, fillMode; public bool frontCCW; }
    [Serializable] public class DepthState { public bool depthEnable, depthWrites, depthBounds; public string depthFunction; }
    [Serializable] public class BlendState { public bool enabled, logicOperationEnabled; public int writeMask; }
    [Serializable] public class ViewportState { public float x, y, width, height, minDepth, maxDepth; public bool enabled; }
    [Serializable] public class ScissorState { public float x, y, width, height; public bool enabled; }
    [Serializable] public class ReplayPipeline
    {
        public RasterState rasterizer;
        public DepthState depthTest;
        public BlendState[] colorBlends;
        public ViewportState[] viewports;
        public ScissorState[] scissors;
    }
}
