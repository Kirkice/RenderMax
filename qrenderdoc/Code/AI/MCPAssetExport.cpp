/* RenderMax asset package exporter. Original buffers are authoritative; CSV is decoded evidence. */
#include "Code/precompiled.h"
#include "MCPAssetExport.h"
#include "MCPShaderTools.h"
#include "MCPResourceTools.h"
#include "MCPTools.h"
#include "Code/QRDUtils.h"
#include "Code/Interface/QRDInterface.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImage>
#include <QSaveFile>
#include <QSet>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace MCP
{
namespace
{
QString S(const rdcstr &s) { return QString::fromUtf8(s.c_str()); }
QString ID(ResourceId id) { return ResourceIdToString(id); }
QString Hash(const QByteArray &b) { return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex()); }
QByteArray Json(const QJsonObject &o) { return QJsonDocument(o).toJson(QJsonDocument::Indented); }
struct Writer
{
  QString root;
  QJsonArray files, warnings;
  QSet<QString> written;
  void Write(const QString &path, const QByteArray &data)
  {
    if(written.contains(path)) return;
    if(!QDir().mkpath(QFileInfo(root + QStringLiteral("/") + path).absolutePath())) throw std::runtime_error("Cannot create export directory");
    QSaveFile f(root + QStringLiteral("/") + path);
    if(!f.open(QIODevice::WriteOnly) || f.write(data) != data.size() || !f.commit())
      throw std::runtime_error("Cannot commit export file");
    written.insert(path);
    files.push_back(QJsonObject{{QStringLiteral("path"), path}, {QStringLiteral("bytes"), double(data.size())}, {QStringLiteral("sha256"), Hash(data)}});
  }
  void Object(const QString &path, const QJsonObject &o) { Write(path, Json(o)); }
  void Adopt(const QString &path)
  {
    QFile f(root + QStringLiteral("/") + path);
    if(!f.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot read saved texture");
    const QByteArray b = f.readAll();
    written.insert(path);
    files.push_back(QJsonObject{{QStringLiteral("path"), path}, {QStringLiteral("bytes"), double(b.size())}, {QStringLiteral("sha256"), Hash(b)}});
  }
  void Warn(uint32_t event, const QString &text) { warnings.push_back(QJsonObject{{QStringLiteral("eventId"), double(event)}, {QStringLiteral("message"), text}}); }
};
void Check(CallContext &call)
{
  if(call.IsCancelled() || call.IsExpired()) throw std::runtime_error("Export cancelled or deadline exceeded; package is incomplete");
}
void Walk(const rdcarray<ActionDescription> &actions, QJsonArray &events, QList<const ActionDescription *> &draws)
{
  for(const ActionDescription &a : actions)
  {
    events.push_back(QJsonObject{{QStringLiteral("eventId"), double(a.eventId)}, {QStringLiteral("name"), S(a.customName)},
                                {QStringLiteral("flags"), QString::number(uint32_t(a.flags))}, {QStringLiteral("numIndices"), double(a.numIndices)},
                                {QStringLiteral("numInstances"), double(a.numInstances)}});
    if(bool(a.flags & ActionFlags::Drawcall) || bool(a.flags & ActionFlags::Dispatch)) draws.push_back(&a);
    Walk(a.children, events, draws);
  }
}
QString Buffer(Writer &w, IReplayController &r, ResourceId id)
{
  if(id == ResourceId()) return QString();
  bytebuf b = r.GetBufferData(id, 0, 0);
  QByteArray bytes((const char *)b.data(), int(b.size()));
  QString path = QStringLiteral("assets/buffers/") + Hash(bytes) + QStringLiteral(".bin");
  w.Write(path, bytes);
  return path;
}
QJsonObject Mesh(Writer &w, IReplayController &r, const ActionDescription &a, const QString &dir)
{
  const PipeState &p = r.GetPipelineState();
  const auto attrs = p.GetVertexInputs();
  const auto vbs = p.GetVBuffers();
  QMap<int, QByteArray> data;
  QJsonArray streams, descriptions;
  for(int i = 0; i < int(vbs.size()); i++)
  {
    const auto &v = vbs[i];
    if(v.resourceId == ResourceId()) continue;
    const QString path = Buffer(w, r, v.resourceId);
    QFile f(w.root + QStringLiteral("/") + path); f.open(QIODevice::ReadOnly); data[i] = f.readAll();
    streams.push_back(QJsonObject{{QStringLiteral("slot"), i}, {QStringLiteral("resourceId"), ID(v.resourceId)}, {QStringLiteral("path"), path},
       {QStringLiteral("byteOffset"), QString::number(qulonglong(v.byteOffset))}, {QStringLiteral("byteStride"), double(v.byteStride)}});
  }
  auto quote = [](QString s) { s.replace(QChar(34), QString(2, QChar(34))); return QChar(34) + s + QChar(34); };
  QStringList header; header << QStringLiteral("vertexId") << QStringLiteral("sourceVertex");
  QList<VertexInputAttribute> vertexAttrs, instanceAttrs;
  for(const auto &attr : attrs)
  {
    if(!attr.used && attr.name.empty()) continue;
    const int n = attr.format.compCount;
    descriptions.push_back(QJsonObject{{QStringLiteral("name"), S(attr.name)}, {QStringLiteral("components"), n},
       {QStringLiteral("slot"), attr.vertexBuffer}, {QStringLiteral("byteOffset"), double(attr.byteOffset)},
       {QStringLiteral("format"), S(attr.format.Name())}, {QStringLiteral("perInstance"), attr.perInstance},
       {QStringLiteral("instanceRate"), attr.instanceRate}, {QStringLiteral("generic"), attr.genericEnabled}, {QStringLiteral("used"), attr.used}});
    if(attr.perInstance) instanceAttrs.push_back(attr);
    else
    {
      vertexAttrs.push_back(attr);
      for(int c = 0; c < n; c++) header << quote(S(attr.name) + QStringLiteral(".") + QString::number(c));
    }
  }
  auto decode = [&](const VertexInputAttribute &attr, uint64_t row) -> QStringList {
    QStringList out;
    if(attr.genericEnabled)
    {
      for(int c = 0; c < attr.format.compCount; c++)
      {
        double value = attr.format.compType == CompType::UInt ? attr.genericValue.uintValue[c] :
                       attr.format.compType == CompType::SInt ? attr.genericValue.intValue[c] : attr.genericValue.floatValue[c];
        out << QString::number(value, 'g', 17);
      }
      return out;
    }
    if(attr.vertexBuffer < 0 || attr.vertexBuffer >= int(vbs.size())) throw std::runtime_error("Invalid vertex buffer slot");
    const auto &vb = vbs[attr.vertexBuffer];
    const QByteArray &b = data[attr.vertexBuffer];
    const uint64_t offset = vb.byteOffset + attr.byteOffset + row * vb.byteStride;
    if(offset >= uint64_t(b.size())) throw std::runtime_error("Vertex fetch outside captured buffer");
    const byte *ptr = (const byte *)b.constData() + offset;
    ShaderConstant var; var.type.rows = 1; var.type.columns = attr.format.compCount;
    const QVariantList values = GetVariants(attr.format, var, ptr, (const byte *)b.constData() + b.size());
    if(values.size() != attr.format.compCount) throw std::runtime_error("Vertex format could not be decoded completely");
    for(const QVariant &v : values)
    {
      if(!v.isValid()) throw std::runtime_error("Invalid decoded vertex component");
      const double n = v.toDouble();
      out << (std::isfinite(n) ? QString::number(n, 'g', 17) : QStringLiteral("NaN"));
    }
    return out;
  };
  const bool indexed = bool(a.flags & ActionFlags::Indexed);
  const BoundVBuffer ib = p.GetIBuffer();
  bytebuf indices;
  QString indexPath;
  if(indexed)
  {
    if(ib.byteStride != 1 && ib.byteStride != 2 && ib.byteStride != 4) throw std::runtime_error("Unsupported index width");
    indexPath = Buffer(w, r, ib.resourceId);
    indices = r.GetBufferData(ib.resourceId, ib.byteOffset + uint64_t(a.indexOffset) * ib.byteStride,
                              uint64_t(a.numIndices) * ib.byteStride);
    if(indices.size() != uint64_t(a.numIndices) * ib.byteStride) throw std::runtime_error("Incomplete index buffer");
  }
  if(p.GetPrimitiveTopology() != Topology::TriangleList && p.GetPrimitiveTopology() != Topology::LineList &&
     p.GetPrimitiveTopology() != Topology::PointList)
    w.Warn(a.eventId, QStringLiteral("Non-list topology preserved; Unity importer must handle strips/restarts/patches explicitly."));
  QByteArray vertices = (header.join(QLatin1Char(',')) + QStringLiteral("\n")).toUtf8(), idx = QByteArray("element,index,rawIndex\n");
  QMap<qint64, uint32_t> remap;
  for(uint32_t i = 0; i < a.numIndices; i++)
  {
    uint32_t raw = i;
    if(indexed) { raw = 0; memcpy(&raw, indices.data() + size_t(i) * ib.byteStride, ib.byteStride); }
    const qint64 source = indexed ? qint64(raw) + a.baseVertex : qint64(i) + a.vertexOffset;
    if(source < 0) throw std::runtime_error("Negative resolved vertex index");
    if(!remap.contains(source))
    {
      const uint32_t dest = uint32_t(remap.size()); remap[source] = dest;
      QStringList row; row << QString::number(dest) << QString::number(source);
      for(const auto &attr : vertexAttrs) row << decode(attr, uint64_t(source));
      vertices += (row.join(QLatin1Char(',')) + QStringLiteral("\n")).toUtf8();
    }
    idx += QString(QStringLiteral("%1,%2,%3\n")).arg(i).arg(remap[source]).arg(raw).toUtf8();
  }
  w.Write(dir + QStringLiteral("/vertices.csv"), vertices); w.Write(dir + QStringLiteral("/indices.csv"), idx);
  QStringList ih; ih << QStringLiteral("instance");
  for(const auto &attr : instanceAttrs)
    for(int c = 0; c < attr.format.compCount; c++) ih << quote(S(attr.name) + QStringLiteral(".") + QString::number(c));
  QByteArray instances = (ih.join(QLatin1Char(',')) + QStringLiteral("\n")).toUtf8();
  for(uint32_t i = 0; i < a.numInstances; i++)
  {
    QStringList row; row << QString::number(i);
    for(const auto &attr : instanceAttrs)
      row << decode(attr, attr.instanceRate > 0 ? uint64_t(a.instanceOffset) + i / attr.instanceRate : 0);
    instances += (row.join(QLatin1Char(',')) + QStringLiteral("\n")).toUtf8();
  }
  w.Write(dir + QStringLiteral("/instances.csv"), instances);
  QJsonObject mesh{{QStringLiteral("stage"), QStringLiteral("vsin")}, {QStringLiteral("vertexCount"), remap.size()}, {QStringLiteral("indexCount"), double(a.numIndices)},
    {QStringLiteral("topology"), detail::TopologyName(p.GetPrimitiveTopology())}, {QStringLiteral("attributes"), descriptions}, {QStringLiteral("streams"), streams},
    {QStringLiteral("vertices"), dir + QStringLiteral("/vertices.csv")}, {QStringLiteral("indices"), dir + QStringLiteral("/indices.csv")}, {QStringLiteral("instances"), dir + QStringLiteral("/instances.csv")},
    {QStringLiteral("instanceCount"), double(a.numInstances)}, {QStringLiteral("baseVertex"), a.baseVertex}, {QStringLiteral("indexOffset"), double(a.indexOffset)},
    {QStringLiteral("vertexOffset"), double(a.vertexOffset)}, {QStringLiteral("instanceOffset"), double(a.instanceOffset)},
    {QStringLiteral("indexBuffer"), indexPath}, {QStringLiteral("coordinateConversion"), QStringLiteral("none")}};
  w.Object(dir + QStringLiteral("/mesh.json"), mesh); return mesh;
}

void Texture(Writer &w, IReplayController &r, const TextureDescription &t, uint32_t event, QJsonArray &refs)
{
  const QString dir = QString(QStringLiteral("assets/textures/E%1_T%2")).arg(event).arg(ID(t.resourceId).remove(QStringLiteral("ResourceId::")));
  QDir().mkpath(w.root + QStringLiteral("/") + dir);
  QJsonArray images, rawSubresources;
  // GetTextureData is the API-native readback evidence. Keep it separately from
  // decoded image exports, especially for ASTC, stencil and integer formats.
  for(uint32_t mip = 0; mip < t.mips; mip++)
    for(uint32_t layer = 0; layer < qMax(1U, t.arraysize); layer++)
      for(uint32_t sample = 0; sample < qMax(1U, t.msSamp); sample++)
      {
        const bytebuf bytes = r.GetTextureData(t.resourceId, Subresource{mip, layer, sample});
        if(bytes.empty()) { w.Warn(event, QStringLiteral("Empty raw texture readback: ") + ID(t.resourceId)); continue; }
        const QByteArray data((const char *)bytes.data(), int(bytes.size()));
        const QString path = QStringLiteral("assets/texture-data/") + Hash(data) + QStringLiteral(".bin");
        w.Write(path, data);
        rawSubresources.push_back(QJsonObject{{QStringLiteral("mip"), double(mip)},
          {QStringLiteral("layer"), double(layer)}, {QStringLiteral("sample"), double(sample)},
          {QStringLiteral("path"), path}, {QStringLiteral("bytes"), double(bytes.size())}});
      }
  const bool hdr = t.format.compType == CompType::Float || t.format.compType == CompType::Depth;
  const bool special = t.format.compType == CompType::UInt || t.format.compType == CompType::SInt ||
                       t.format.compType == CompType::SNorm || t.format.compType == CompType::SScaled || t.format.compType == CompType::UScaled;
  const bool dds = t.cubemap || special;
  if(special) w.Warn(event, QStringLiteral("Integer/signed texture preserved as DDS instead of lossy PNG/TGA: ") + ID(t.resourceId));
  const uint32_t layers = qMax(1U, t.arraysize);
  for(uint32_t mip = 0; mip < (dds ? 1U : t.mips); mip++)
    for(uint32_t layer = 0; layer < (dds ? 1U : layers); layer++)
    {
      TextureSave save; save.resourceId = t.resourceId;
      save.destType = dds ? FileType::DDS : hdr ? FileType::EXR : FileType::PNG;
      save.alpha = AlphaMapping::Preserve;
      save.mip = dds ? -1 : int(mip); save.slice.sliceIndex = dds ? -1 : int(layer);
      const QString ext = dds ? QStringLiteral("dds") : hdr ? QStringLiteral("exr") : QStringLiteral("png");
      QString path = dir + (dds ? QStringLiteral("/texture.") + ext : QString(QStringLiteral("/layer_%1_mip_%2.")).arg(layer, 3, 10, QLatin1Char('0')).arg(mip, 3, 10, QLatin1Char('0')) + ext);
      if(t.depth > 1 && !dds) { w.Warn(event, QStringLiteral("3D texture exported as DDS to retain all depth slices")); save.destType = FileType::DDS; save.mip = -1; save.slice.sliceIndex = -1; path = dir + QStringLiteral("/texture.dds"); }
      const auto result = r.SaveTexture(save, (w.root + QStringLiteral("/") + path).toUtf8().constData());
      if(!result.OK()) { w.Warn(event, QStringLiteral("Texture save failed: ") + ID(t.resourceId) + QStringLiteral(": ") + S(result.Message())); return; }
      QJsonObject image{{QStringLiteral("mip"), save.mip}, {QStringLiteral("layer"), save.slice.sliceIndex}};
      if(save.destType == FileType::PNG)
      {
        // Inspect replay's decoded image, including compressed formats such as ASTC.
        // Channel count alone does not establish whether alpha contains useful data.
        QImage decoded(w.root + QStringLiteral("/") + path);
        if(decoded.isNull()) throw std::runtime_error("Cannot decode PNG for alpha inspection");
        decoded = decoded.convertToFormat(QImage::Format_ARGB32);
        int alphaMin = 255, alphaMax = 0;
        for(int y = 0; y < decoded.height(); y++)
        {
          const QRgb *pixels = reinterpret_cast<const QRgb *>(decoded.constScanLine(y));
          for(int x = 0; x < decoded.width(); x++)
          {
            const int value = qAlpha(pixels[x]);
            alphaMin = qMin(alphaMin, value); alphaMax = qMax(alphaMax, value);
          }
        }
        const bool meaningfulAlpha = !((alphaMin == 0 && alphaMax == 0) ||
                                      (alphaMin == 255 && alphaMax == 255));
        image[QStringLiteral("alphaMin")] = alphaMin;
        image[QStringLiteral("alphaMax")] = alphaMax;
        image[QStringLiteral("meaningfulAlpha")] = meaningfulAlpha;
        if(meaningfulAlpha)
        {
          const QString pngPath = path;
          path.chop(3); path += QStringLiteral("tga");
          save.destType = FileType::TGA;
          const auto tgaResult = r.SaveTexture(save, (w.root + QStringLiteral("/") + path).toUtf8().constData());
          if(!tgaResult.OK()) throw std::runtime_error("Cannot save alpha-bearing TGA");
          if(!QFile::remove(w.root + QStringLiteral("/") + pngPath))
            throw std::runtime_error("Cannot remove intermediate PNG");
        }
      }
      image[QStringLiteral("path")] = path;
      w.Adopt(path); images.push_back(image);
      if(t.depth > 1) goto saved;
    }
 saved:
  QJsonObject meta{{QStringLiteral("resourceId"), ID(t.resourceId)}, {QStringLiteral("eventId"), double(event)}, {QStringLiteral("width"), double(t.width)},
    {QStringLiteral("height"), double(t.height)}, {QStringLiteral("depth"), double(t.depth)}, {QStringLiteral("arraySize"), double(t.arraysize)},
    {QStringLiteral("mips"), double(t.mips)}, {QStringLiteral("samples"), double(t.msSamp)}, {QStringLiteral("cubemap"), t.cubemap},
    {QStringLiteral("format"), S(t.format.Name())}, {QStringLiteral("formatType"), double(uint32_t(t.format.type))},
    {QStringLiteral("componentType"), double(uint32_t(t.format.compType))}, {QStringLiteral("componentCount"), double(t.format.compCount)},
    {QStringLiteral("componentByteWidth"), double(t.format.compByteWidth)}, {QStringLiteral("bgraOrder"), t.format.BGRAOrder()},
    {QStringLiteral("rawSubresources"), rawSubresources}, {QStringLiteral("rawLayout"), QStringLiteral("IReplayController::GetTextureData native API readback; not an engine source image")},
    {QStringLiteral("provenance"), QJsonObject{{QStringLiteral("Confirmed"), QStringLiteral("GPU description and event-scoped replay readback")},
       {QStringLiteral("Inferred"), QStringLiteral("PNG/TGA/EXR preview interpretation")},
       {QStringLiteral("Unrecoverable"), QStringLiteral("Original source file, importer settings and engine GUID")}}},
    {QStringLiteral("srgb"), t.format.SRGBCorrected()}, {QStringLiteral("images"), images},
    {QStringLiteral("multisamplePolicy"), QStringLiteral("resolved average where supported")}};
  w.Object(dir + QStringLiteral("/texture.json"), meta);
  refs.push_back(QJsonObject{{QStringLiteral("resourceId"), ID(t.resourceId)}, {QStringLiteral("path"), dir + QStringLiteral("/texture.json")}});
}

ToolResult Export(QueryAdapter &adapter, const QJsonObject &args, CallContext &call, bool whole)
{
  const QString root = QDir::cleanPath(args.value(QStringLiteral("outputPath")).toString());
  if(!QFileInfo(root).isAbsolute() || QFileInfo::exists(root))
    return ToolResult::Failure(Error(ErrorCode::InvalidParams, QStringLiteral("outputPath must be a new absolute directory; existing paths are never overwritten")));
  ToolResult permission = adapter.WithCaptureUI(call, [](ICaptureContext &ctx) {
    return ctx.Config().MCP_AllowSensitiveData ? ToolResult::Success(QJsonObject()) :
      ToolResult::Failure(Error(ErrorCode::Unsupported, QStringLiteral("Enable MCP_AllowSensitiveData to export assets")));
  });
  if(!permission.ok) return permission;
  const CaptureSnapshot snapshot = adapter.Snapshot();
  const uint32_t requested = uint32_t(args.value(QStringLiteral("eventId")).toDouble(snapshot.selectedEvent));
  Writer w; w.root = root;
  if(!QDir().mkpath(root)) return ToolResult::Failure(Error(ErrorCode::InternalError, QStringLiteral("Cannot create output directory")));
  w.Object(QStringLiteral("manifest.json"), QJsonObject{{QStringLiteral("format"), QStringLiteral("rendermax.asset-package")}, {QStringLiteral("schemaVersion"), QStringLiteral("1.0.0")}, {QStringLiteral("status"), QStringLiteral("inProgress")}});
  ToolResult result = adapter.DispatchBlock("renderdoc_exportAssets", call, [&](IReplayController &r) {
    QJsonArray events, draws, resources; QList<const ActionDescription *> actions;
    Walk(r.GetRootActions(), events, actions);
    w.Object(QStringLiteral("events/index.json"), QJsonObject{{QStringLiteral("events"), events}});
    w.Object(QStringLiteral("capture.json"), QJsonObject{{QStringLiteral("filename"), snapshot.filename}, {QStringLiteral("api"), snapshot.api},
        {QStringLiteral("frameNumber"), double(snapshot.frameNumber)}, {QStringLiteral("scope"), whole ? QStringLiteral("capture") : QStringLiteral("draw")}});
    bool found = false;
    const auto textures = r.GetTextures();
    for(const auto *a : actions)
    {
      if(!whole && a->eventId != requested) continue;
      found = true; Check(call); r.SetFrameEvent(a->eventId, true);
      const QString dir = QString(QStringLiteral("draws/E%1")).arg(a->eventId, 8, 10, QLatin1Char('0'));
      const PipeState &p = r.GetPipelineState();
      w.Object(dir + QStringLiteral("/pipeline.json"), PipelineStateToJson(r));
      QJsonObject shaderRefs; QJsonArray constants, bindings, textureRefs, bufferRefs;
      QSet<QString> textureIds, bufferIds;
      for(int si = 0; si <= int(ShaderStage::Mesh); si++)
      {
        const ShaderStage stage = ShaderStage(si);
        const ShaderReflection *refl = p.GetShaderReflection(stage);
        if(!refl) continue;
        const ResourceId pipeline = stage == ShaderStage::Compute ? p.GetComputePipelineObject() : p.GetGraphicsPipelineObject();
        QJsonObject shader = ShaderReflectionToJson(*refl, true);
        const QString sd = QStringLiteral("assets/shaders/") + Hash(Json(shader));
        const QString sp = sd + QStringLiteral("/shader.json");
        if(!w.written.contains(sp))
        {
          w.Object(sp, shader);
          w.Write(sd + QStringLiteral("/bytecode.bin"), QByteArray((const char *)refl->rawBytes.data(), int(refl->rawBytes.size())));
          w.Write(sd + QStringLiteral("/disassembly.txt"), QByteArray(r.DisassembleShader(pipeline, refl, rdcstr()).c_str()));
          int fi = 0;
          for(const auto &file : refl->debugInfo.files)
            w.Write(sd + QString(QStringLiteral("/source/%1_")).arg(fi++) + QFileInfo(S(file.filename)).fileName(), QByteArray(file.contents.c_str()));
        }
        shaderRefs[detail::ShaderStageName(stage)] = sp;
        for(const auto &list : {p.GetReadOnlyResources(stage), p.GetReadWriteResources(stage), p.GetSamplers(stage), p.GetConstantBlocks(stage)})
          for(const UsedDescriptor &b : list)
          {
            bindings.push_back(QJsonObject{{QStringLiteral("stage"), detail::ShaderStageName(stage)}, {QStringLiteral("resourceId"), ID(b.descriptor.resource)},
               {QStringLiteral("reflectionIndex"), double(b.access.index)}, {QStringLiteral("arrayElement"), double(b.access.arrayElement)},
               {QStringLiteral("byteOffset"), QString::number(qulonglong(b.descriptor.byteOffset))},
               {QStringLiteral("byteSize"), QString::number(qulonglong(b.descriptor.byteSize))}});
            if(b.descriptor.resource != ResourceId()) { textureIds.insert(ID(b.descriptor.resource)); bufferIds.insert(ID(b.descriptor.resource)); }
          }
        for(uint32_t bi = 0; bi < refl->constantBlocks.size(); bi++)
        {
          const auto &block = refl->constantBlocks[bi];
          rdcarray<UsedDescriptor> blocks;
          for(const auto &b : p.GetConstantBlocks(stage)) if(b.access.index == bi) blocks.push_back(b);
          if(blocks.empty() && !block.bufferBacked) blocks.push_back(p.GetConstantBlock(stage, bi, 0));
          for(const auto &b : blocks)
          {
            QJsonArray values;
            for(const auto &v : r.GetCBufferVariableContents(pipeline, p.GetShader(stage), stage, refl->entryPoint, bi,
                  b.descriptor.resource, b.descriptor.byteOffset, b.descriptor.byteSize)) values.push_back(ShaderVariableToJson(v));
            constants.push_back(QJsonObject{{QStringLiteral("stage"), detail::ShaderStageName(stage)}, {QStringLiteral("block"), S(block.name)},
               {QStringLiteral("bufferBacked"), block.bufferBacked}, {QStringLiteral("resourceId"), ID(b.descriptor.resource)},
               {QStringLiteral("arrayElement"), double(b.access.arrayElement)}, {QStringLiteral("variables"), values}, {QStringLiteral("layoutReference"), sp}});
          }
        }
      }
      for(const auto &out : p.GetOutputTargets()) textureIds.insert(ID(out.resource));
      textureIds.insert(ID(p.GetDepthTarget().resource));
      for(const auto &t : textures) if(textureIds.contains(ID(t.resourceId))) { Check(call); Texture(w, r, t, a->eventId, textureRefs); }
      for(const auto &b : r.GetBuffers()) if(bufferIds.contains(ID(b.resourceId)))
        bufferRefs.push_back(QJsonObject{{QStringLiteral("resourceId"), ID(b.resourceId)}, {QStringLiteral("path"), Buffer(w, r, b.resourceId)}});
      QJsonObject draw{{QStringLiteral("eventId"), double(a->eventId)}, {QStringLiteral("name"), S(a->customName)}, {QStringLiteral("shaders"), shaderRefs},
        {QStringLiteral("pipeline"), dir + QStringLiteral("/pipeline.json")}, {QStringLiteral("constants"), dir + QStringLiteral("/constants.json")},
        {QStringLiteral("bindings"), dir + QStringLiteral("/bindings.json")}, {QStringLiteral("textures"), textureRefs}, {QStringLiteral("buffers"), bufferRefs},
        {QStringLiteral("kind"), bool(a->flags & ActionFlags::Dispatch) ? QStringLiteral("dispatch") : QStringLiteral("draw")}};
      if(bool(a->flags & ActionFlags::Drawcall))
      {
        try { Mesh(w, r, *a, dir + QStringLiteral("/mesh")); draw[QStringLiteral("mesh")] = dir + QStringLiteral("/mesh/mesh.json"); }
        catch(const std::exception &e) { w.Warn(a->eventId, QString(QStringLiteral("Mesh decode failed; captured buffers retained: ")) + QString::fromUtf8(e.what())); draw[QStringLiteral("meshStatus")] = QStringLiteral("failed"); }
      }
      w.Object(dir + QStringLiteral("/constants.json"), QJsonObject{{QStringLiteral("blocks"), constants}});
      w.Object(dir + QStringLiteral("/bindings.json"), QJsonObject{{QStringLiteral("descriptors"), bindings}});
      w.Object(dir + QStringLiteral("/draw.json"), draw);
      draws.push_back(QJsonObject{{QStringLiteral("eventId"), double(a->eventId)}, {QStringLiteral("path"), dir + QStringLiteral("/draw.json")}});
    }
    if(!found) throw std::runtime_error("Requested event is not a draw or dispatch");
    w.Object(QStringLiteral("export-report.json"), QJsonObject{{QStringLiteral("warnings"), w.warnings},
      {QStringLiteral("limitations"), QJsonArray{QStringLiteral("Unity semantic Shader reconstruction is a separate step; raw shader evidence exported."),
       QStringLiteral("Resource content is event-scoped after action replay; output textures represent post-action state."),
       QStringLiteral("Primitive restart/topology conversion and engine scene reconstruction are not performed."),
       QStringLiteral("Mesh data is GPU input, not recovered original authoring assets.")}}});
    QJsonObject manifest{{QStringLiteral("format"), QStringLiteral("rendermax.asset-package")}, {QStringLiteral("schemaVersion"), QStringLiteral("1.0.0")},
       {QStringLiteral("status"), w.warnings.isEmpty() ? QStringLiteral("completed") : QStringLiteral("completedWithWarnings")},
       {QStringLiteral("scope"), whole ? QStringLiteral("capture") : QStringLiteral("draw")}, {QStringLiteral("capture"), QStringLiteral("capture.json")}, {QStringLiteral("events"), QStringLiteral("events/index.json")},
       {QStringLiteral("report"), QStringLiteral("export-report.json")}, {QStringLiteral("draws"), draws}, {QStringLiteral("files"), w.files},
       {QStringLiteral("provenance"), QJsonObject{{QStringLiteral("Confirmed"), QStringLiteral("Captured GPU inputs, replay state, reflection and buffer/texture readbacks")},
         {QStringLiteral("Inferred"), QStringLiteral("Decoded preview format selection; no semantic engine reconstruction")},
         {QStringLiteral("Unrecoverable"), QStringLiteral("Original materials, authoring meshes, scenes, GUIDs and shader source when absent from capture")}}}};
    // Manifest cannot hash itself. Replace the initial in-progress manifest atomically.
    w.written.remove(QStringLiteral("manifest.json"));
    QJsonArray filtered; for(const auto &f : w.files) if(f.toObject().value(QStringLiteral("path")) != QStringLiteral("manifest.json")) filtered.push_back(f);
    manifest[QStringLiteral("files")] = filtered;
    w.Object(QStringLiteral("manifest.json"), manifest);
    return ToolResult::Success(QJsonObject{{QStringLiteral("outputPath"), root}, {QStringLiteral("drawCount"), draws.size()}, {QStringLiteral("warningCount"), w.warnings.size()}, {QStringLiteral("status"), manifest[QStringLiteral("status")]}});
  });
  if(!result.ok)
  {
    w.written.remove(QStringLiteral("manifest.json"));
    w.Object(QStringLiteral("manifest.json"), QJsonObject{{QStringLiteral("format"), QStringLiteral("rendermax.asset-package")}, {QStringLiteral("schemaVersion"), QStringLiteral("1.0.0")}, {QStringLiteral("status"), QStringLiteral("failed")}, {QStringLiteral("files"), w.files}});
  }
  return result;
}
}
void RegisterAssetExportTools(ToolRegistry &registry, QueryAdapter &adapter)
{
  for(bool whole : {false, true})
  {
    const QString name = whole ? QStringLiteral("renderdoc_exportFrameAssets") : QStringLiteral("renderdoc_exportDrawAssets");
    QJsonObject schema{{QStringLiteral("type"), QStringLiteral("object")}, {QStringLiteral("additionalProperties"), false},
      {QStringLiteral("properties"), QJsonObject{{QStringLiteral("outputPath"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}, {QStringLiteral("minLength"), 1}}},
        {QStringLiteral("eventId"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}, {QStringLiteral("minimum"), 1}, {QStringLiteral("maximum"), 4294967295.0}}}}},
      {QStringLiteral("required"), QJsonArray{QStringLiteral("outputPath")}}};
    registry.Add(ToolDefinition{name, name,
      QStringLiteral("Export an event-scoped asset package to a NEW absolute directory: PNG/TGA/EXR/DDS textures, decoded mesh CSV and raw buffers, shader evidence, constants and pipeline. Synchronous; inspect warnings. No original-engine or semantic shader recovery claim."),
      ToolEffects::SideEffect, ToolRequirements::LoadedCapture | ToolRequirements::ReplayController, schema},
      [&adapter, whole](const QJsonObject &args, CallContext &call) { return Export(adapter, args, call, whole); });
  }
}
}
