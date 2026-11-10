// Versioned, self-contained RenderMax trace archive. Included by SystemTrace.cpp only.
namespace
{
const QByteArray RdtMagic("RenderMaxRDT\r\n", 14);
const quint64 RdtLimit = 512ULL * 1024 * 1024;
bool ReadRdtPart(QDataStream &stream, QByteArray &data)
{
  quint64 size = 0; stream >> size;
  if(stream.status() != QDataStream::Ok || size > RdtLimit || size > quint64(stream.device()->bytesAvailable())) return false;
  data.resize(int(size));
  return size == 0 || stream.readRawData(data.data(), int(size)) == int(size);
}
bool ReadRdt(const QString &path, QByteArray &timeline, QByteArray &raw, QString &error)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly)) { error=file.errorString(); return false; }
  if(file.read(RdtMagic.size())!=RdtMagic){error=QStringLiteral("Not a RenderMax RDT archive");return false;}
  QDataStream stream(&file);stream.setVersion(QDataStream::Qt_5_9);
  quint32 version=0;stream >> version;
  if(version!=1){error=QStringLiteral("Unsupported RDT version %1").arg(version);return false;}
  QByteArray metadata,digest;
  if(!ReadRdtPart(stream,metadata) || !ReadRdtPart(stream,timeline) || !ReadRdtPart(stream,raw) || !ReadRdtPart(stream,digest) || !file.atEnd())
  {error=QStringLiteral("Truncated, oversized or malformed RDT archive");return false;}
  QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData(metadata);hash.addData(timeline);hash.addData(raw);
  if(digest!=hash.result()){error=QStringLiteral("RDT checksum mismatch");return false;}
  if(timeline.isEmpty() || timeline.startsWith(RdtMagic))
  {error=QStringLiteral("Empty or nested RDT timeline is not supported");return false;}
  return true;
}
}

bool SaveSystemTraceArchive(const QString &sourcePath,const QString &destination,QString &error)
{
  error.clear();
  const QFileInfo source(sourcePath),target(destination);
  if(!source.isAbsolute() || !source.isFile() || !target.isAbsolute())
  {error=QStringLiteral("Source must exist and both paths must be absolute");return false;}
  if(source.canonicalFilePath()==target.canonicalFilePath())
  {error=QStringLiteral("Choose a destination different from the source");return false;}
  QFile input(sourcePath);
  if(!input.open(QIODevice::ReadOnly)){error=input.errorString();return false;}
  QByteArray timeline,raw;
  if(input.peek(RdtMagic.size())==RdtMagic)
  {
    input.close();if(!ReadRdt(sourcePath,timeline,raw,error))return false;
  }
  else
  {
    if(quint64(input.size())>RdtLimit){error=QStringLiteral("Timeline exceeds 512 MiB archive limit");return false;}
    timeline=input.readAll();
    if(input.error()!=QFile::NoError){error=input.errorString();return false;}
    // Capture output uses a stable sibling name. This attachment is optional, never a dependency.
    QFile original(source.dir().absoluteFilePath(QStringLiteral("capture.perfetto-trace")));
    if(original.exists())
    {
      if(!original.open(QIODevice::ReadOnly) || quint64(original.size())>RdtLimit)
      {error=QStringLiteral("Cannot read original Perfetto attachment or it exceeds 512 MiB");return false;}
      raw=original.readAll();if(original.error()!=QFile::NoError){error=original.errorString();return false;}
    }
  }
  if(!ImportSystemTraceFile(sourcePath).valid){error=QStringLiteral("Source is not a valid importable System Trace");return false;}
  const QByteArray metadata=QJsonDocument(QJsonObject{{QStringLiteral("format"),QStringLiteral("RenderMax System Trace")},
    {QStringLiteral("version"),1},{QStringLiteral("sourceName"),source.fileName()},
    {QStringLiteral("createdUtc"),QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    {QStringLiteral("rawPerfettoAttached"),!raw.isEmpty()}}).toJson(QJsonDocument::Compact);
  QCryptographicHash hash(QCryptographicHash::Sha256);hash.addData(metadata);hash.addData(timeline);hash.addData(raw);
  QSaveFile file(destination);
  if(!file.open(QIODevice::WriteOnly)){error=file.errorString();return false;}
  file.write(RdtMagic);QDataStream stream(&file);stream.setVersion(QDataStream::Qt_5_9);stream << quint32(1);
  for(const auto &part:{metadata,timeline,raw,hash.result()})
  {stream << quint64(part.size());if(!part.isEmpty())stream.writeRawData(part.constData(),part.size());}
  if(stream.status()!=QDataStream::Ok || !file.commit()){error=file.errorString();return false;}
  return true;
}
