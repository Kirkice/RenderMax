/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "TerminalViewer.h"
#include <QClipboard>
#include <QComboBox>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScrollBar>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QStyle>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextStream>
#include <QTime>
#include <QToolButton>
#include <QVBoxLayout>
#include "Code/QRDUtils.h"
#include "Code/Resources.h"

// The marker the shell echoes after each command. It is filtered out of the output and used to
// learn the working directory and to know the command finished.
static const QString MARKER_PREFIX = lit("RMPROMPT:");

static const int SCROLLBACK_LINES = 5000;
static const int MIN_FONT_SIZE = 6;
static const int MAX_FONT_SIZE = 32;

TerminalViewer::TerminalViewer(QWidget *parent) : QFrame(parent)
{
  setObjectName(lit("terminalViewer"));

  setupColours();
  createUi();
  populateShells();

  startShell();
}

TerminalViewer::~TerminalViewer()
{
  stopShell();
}

void TerminalViewer::setupColours()
{
  const bool dark = IsDarkTheme();

  m_TextColour = dark ? QColor(0xD4, 0xD4, 0xD4) : QColor(0x1F, 0x1F, 0x1F);
  m_NoticeColour = dark ? QColor(0x80, 0x80, 0x80) : QColor(0x70, 0x70, 0x70);
  m_CommandColour = dark ? QColor(0x6F, 0xCF, 0x97) : QColor(0x1B, 0x5E, 0x20);

  m_AnsiColour = m_TextColour;

  // The 16 ANSI colours, adjusted so none of them vanish into the page. Programs pick these by
  // index, so the mapping has to keep the usual meaning: red stays red, bright stays brighter.
  static const QColor darkTable[16] = {
      QColor(0x60, 0x60, 0x60), QColor(0xE0, 0x6C, 0x75), QColor(0x98, 0xC3, 0x79),
      QColor(0xE5, 0xC0, 0x7B), QColor(0x61, 0xAF, 0xEF), QColor(0xC6, 0x78, 0xDD),
      QColor(0x56, 0xB6, 0xC2), QColor(0xDC, 0xDF, 0xE4), QColor(0x8A, 0x8A, 0x8A),
      QColor(0xFF, 0x7B, 0x86), QColor(0xB5, 0xE8, 0x90), QColor(0xFF, 0xD6, 0x8A),
      QColor(0x7F, 0xC4, 0xFF), QColor(0xE2, 0x9B, 0xF5), QColor(0x6F, 0xD6, 0xE2),
      QColor(0xFF, 0xFF, 0xFF),
  };

  static const QColor lightTable[16] = {
      QColor(0x00, 0x00, 0x00), QColor(0xC9, 0x1B, 0x00), QColor(0x1B, 0x7F, 0x1B),
      QColor(0x9A, 0x6A, 0x00), QColor(0x1E, 0x5F, 0xBF), QColor(0xA6, 0x26, 0xA4),
      QColor(0x0E, 0x7C, 0x86), QColor(0x38, 0x3A, 0x42), QColor(0x5C, 0x5C, 0x5C),
      QColor(0xE0, 0x3E, 0x3E), QColor(0x1F, 0x9C, 0x3E), QColor(0xB5, 0x7F, 0x00),
      QColor(0x3B, 0x78, 0xD8), QColor(0xC1, 0x3B, 0xC1), QColor(0x0E, 0x9A, 0xA7),
      QColor(0x00, 0x00, 0x00),
  };

  for(int i = 0; i < 16; i++)
    m_AnsiTable[i] = dark ? darkTable[i] : lightTable[i];

  m_FontSize = Formatter::FixedFont().pointSize();

  if(m_FontSize <= 0)
    m_FontSize = 10;
}

void TerminalViewer::createUi()
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // toolbar -------------------------------------------------------------
  QFrame *toolbar = new QFrame(this);
  toolbar->setObjectName(lit("terminalToolbar"));
  toolbar->setFrameShape(QFrame::NoFrame);

  QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
  toolbarLayout->setContentsMargins(8, 2, 8, 2);
  toolbarLayout->setSpacing(6);

  QLabel *shellLabel = new QLabel(tr("Shell:"), toolbar);
  shellLabel->setObjectName(lit("terminalShellLabel"));

  m_ShellCombo = new QComboBox(toolbar);
  m_ShellCombo->setObjectName(lit("terminalShellCombo"));
  m_ShellCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  connect(m_ShellCombo, QOverload<int>::of(&QComboBox::activated), this, &TerminalViewer::changeShell);

  m_StatusLabel = new QLabel(toolbar);
  m_StatusLabel->setObjectName(lit("terminalStatus"));

  m_RestartButton = new QToolButton(toolbar);
  m_RestartButton->setObjectName(lit("terminalRestart"));
  m_RestartButton->setIcon(Icons::arrow_refresh());
  m_RestartButton->setToolTip(tr("Restart the shell process, dropping its session state"));
  m_RestartButton->setAutoRaise(true);
  m_RestartButton->setIconSize(QSize(16, 16));
  connect(m_RestartButton, &QToolButton::clicked, this, &TerminalViewer::restartShell);

  m_ClearButton = new QToolButton(toolbar);
  m_ClearButton->setObjectName(lit("terminalClear"));
  m_ClearButton->setIcon(Icons::del());
  m_ClearButton->setToolTip(tr("Clear the output (Ctrl+L)"));
  m_ClearButton->setAutoRaise(true);
  m_ClearButton->setIconSize(QSize(16, 16));
  connect(m_ClearButton, &QToolButton::clicked, this, &TerminalViewer::clearOutput);

  m_CopyButton = new QToolButton(toolbar);
  m_CopyButton->setObjectName(lit("terminalCopy"));
  m_CopyButton->setIcon(Icons::copy());
  m_CopyButton->setToolTip(tr("Copy the selection, or the whole output when nothing is selected"));
  m_CopyButton->setAutoRaise(true);
  m_CopyButton->setIconSize(QSize(16, 16));
  connect(m_CopyButton, &QToolButton::clicked, this, &TerminalViewer::copyOutput);

  m_SaveButton = new QToolButton(toolbar);
  m_SaveButton->setObjectName(lit("terminalSave"));
  m_SaveButton->setIcon(Icons::save());
  m_SaveButton->setToolTip(tr("Save the output to a text file"));
  m_SaveButton->setAutoRaise(true);
  m_SaveButton->setIconSize(QSize(16, 16));
  connect(m_SaveButton, &QToolButton::clicked, this, &TerminalViewer::saveOutput);

  toolbarLayout->addWidget(shellLabel);
  toolbarLayout->addWidget(m_ShellCombo);
  toolbarLayout->addWidget(m_StatusLabel);
  toolbarLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
  toolbarLayout->addWidget(m_CopyButton);
  toolbarLayout->addWidget(m_SaveButton);
  toolbarLayout->addWidget(m_ClearButton);
  toolbarLayout->addWidget(m_RestartButton);

  // output --------------------------------------------------------------
  m_Output = new QPlainTextEdit(this);
  m_Output->setObjectName(lit("terminalOutput"));
  m_Output->setReadOnly(true);
  m_Output->setUndoRedoEnabled(false);
  m_Output->setFrameShape(QFrame::NoFrame);
  m_Output->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_Output->setMaximumBlockCount(SCROLLBACK_LINES);
  m_Output->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  m_Output->installEventFilter(this);

  QFont fixed = Formatter::FixedFont();
  fixed.setPointSize(m_FontSize);
  m_Output->setFont(fixed);
  m_Output->document()->setDefaultFont(fixed);

  // input ---------------------------------------------------------------
  QFrame *inputFrame = new QFrame(this);
  inputFrame->setObjectName(lit("terminalInputFrame"));
  inputFrame->setFrameShape(QFrame::NoFrame);

  QHBoxLayout *inputLayout = new QHBoxLayout(inputFrame);
  inputLayout->setContentsMargins(8, 2, 8, 2);
  inputLayout->setSpacing(6);

  m_PromptLabel = new QLabel(inputFrame);
  m_PromptLabel->setObjectName(lit("terminalPrompt"));
  m_PromptLabel->setFont(fixed);
  m_PromptLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  m_Input = new QLineEdit(inputFrame);
  m_Input->setObjectName(lit("terminalInput"));
  m_Input->setFont(fixed);
  m_Input->setFrame(false);
  m_Input->setPlaceholderText(tr("Type a command and press Enter"));
  m_Input->installEventFilter(this);
  connect(m_Input, &QLineEdit::returnPressed, this, &TerminalViewer::submitInput);

  inputLayout->addWidget(m_PromptLabel);
  inputLayout->addWidget(m_Input, 1);

  layout->addWidget(toolbar);
  layout->addWidget(m_Output, 1);
  layout->addWidget(inputFrame);

  setFocusProxy(m_Input);
  setMinimumSize(360, 180);

  setWindowTitle(tr("Terminal Viewer"));

  setPromptPath(QDir::currentPath());
}

void TerminalViewer::populateShells()
{
  m_Shells.clear();

#ifdef WIN32
  const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
  const QString systemRoot = environment.value(lit("SystemRoot"), lit("C:/Windows"));

  // Windows PowerShell. Its stdin and stdout follow the console codepage, hence the preamble that
  // switches stdout to UTF-8 and the local 8-bit encoding for the commands themselves.
  const QString powerShell =
      QDir(systemRoot).absoluteFilePath(lit("System32/WindowsPowerShell/v1.0/powershell.exe"));

  if(QFileInfo::exists(powerShell))
  {
    TerminalShell shell;
    shell.name = tr("Windows PowerShell");
    shell.program = powerShell;
    shell.arguments = QStringList() << lit("-NoLogo") << lit("-NoProfile") << lit("-Command")
                                    << lit("-");
    shell.preamble = lit("[Console]::OutputEncoding=[Text.Encoding]::UTF8");
    shell.marker = lit("Write-Output (\"RMPROMPT:\" + (Get-Location).Path)");
    shell.inputUtf8 = false;
    shell.powershell = true;
    m_Shells.push_back(shell);
  }

  // PowerShell 7, if it is installed. Same protocol, but it understands ANSI output rendering so
  // the colours it normally strips when redirected can come through.
  QString pwsh = QStandardPaths::findExecutable(lit("pwsh"));

  if(pwsh.isEmpty())
  {
    const QString candidate = lit("C:/Program Files/PowerShell/7/pwsh.exe");

    if(QFileInfo::exists(candidate))
      pwsh = candidate;
  }

  if(!pwsh.isEmpty())
  {
    TerminalShell shell;
    shell.name = tr("PowerShell 7");
    shell.program = pwsh;
    shell.arguments = QStringList() << lit("-NoLogo") << lit("-NoProfile") << lit("-Command")
                                    << lit("-");
    shell.preamble = lit("$PSStyle.OutputRendering='Ansi'; "
                         "[Console]::OutputEncoding=[Text.Encoding]::UTF8");
    shell.marker = lit("Write-Output (\"RMPROMPT:\" + (Get-Location).Path)");
    shell.inputUtf8 = false;
    shell.powershell = true;
    m_Shells.push_back(shell);
  }

  const QString comSpec = environment.value(lit("ComSpec"), lit("cmd.exe"));

  if(QFileInfo::exists(comSpec))
  {
    TerminalShell shell;
    shell.name = tr("Command Prompt");
    shell.program = comSpec;
    // /D skips the user's AutoRun commands, and /K with echo off turns off the prompt echo of the
    // commands piped in on stdin and the startup banner.
    shell.arguments = QStringList() << lit("/Q") << lit("/D") << lit("/K") << lit("@echo off");
    shell.marker = lit("echo RMPROMPT:%CD%");
    // cmd writes its output in the console codepage even after chcp 65001, so rather than fight it
    // both directions stay in the local 8-bit encoding.
    shell.inputUtf8 = false;
    shell.outputUtf8 = false;
    shell.powershell = false;
    m_Shells.push_back(shell);
  }
#else
  TerminalShell shell;
  shell.name = tr("Shell");
  shell.program = QProcessEnvironment::systemEnvironment().value(lit("SHELL"), lit("/bin/sh"));
  shell.arguments = QStringList() << lit("-i");
  shell.marker = lit("echo RMPROMPT:$PWD");
  shell.inputUtf8 = true;
  shell.powershell = false;
  m_Shells.push_back(shell);
#endif

  m_ShellCombo->blockSignals(true);
  m_ShellCombo->clear();

  for(const TerminalShell &shell : m_Shells)
    m_ShellCombo->addItem(shell.name);

  m_ShellCombo->blockSignals(false);
}

void TerminalViewer::changeShell(int index)
{
  if(index < 0 || index >= m_Shells.count())
    return;

  restartShell();
}

void TerminalViewer::restartShell()
{
  startShell();
}

void TerminalViewer::startShell()
{
  stopShell();

  const int index = m_ShellCombo->currentIndex();

  if(index < 0 || index >= m_Shells.count())
    return;

  const TerminalShell &shell = m_Shells[index];
  const QString startDir = m_PromptPath.isEmpty() ? QDir::currentPath() : m_PromptPath;

  m_Process = new QProcess(this);
  m_Process->setProcessChannelMode(QProcess::MergedChannels);
  m_Process->setWorkingDirectory(QDir(startDir).exists() ? startDir : QDir::currentPath());

  connect(m_Process, &QProcess::readyReadStandardOutput, this, &TerminalViewer::readOutput);

  connect(m_Process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this](int exitCode, QProcess::ExitStatus status) {
            if(m_Process == NULL || sender() != m_Process)
              return;

            m_Started = false;
            setBusy(false);

            if(status == QProcess::CrashExit)
              appendNotice(tr("[the shell process crashed]"));
            else
              appendNotice(tr("[the shell exited with code %1]").arg(exitCode));

            setPromptPath(QString());
          });

  connect(m_Process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if(m_Process == NULL || sender() != m_Process)
      return;

    if(error == QProcess::FailedToStart)
    {
      m_Started = false;
      setBusy(false);
      appendNotice(tr("[failed to start the shell process]"));
    }
  });

  m_Process->start(shell.program, shell.arguments);

  if(!m_Process->waitForStarted(5000))
  {
    appendNotice(tr("[failed to start %1]").arg(shell.program));
    return;
  }

  m_Started = true;
  m_OutputUtf8 = shell.outputUtf8;

  appendNotice(tr("%1 in %2").arg(shell.name, QDir::toNativeSeparators(m_Process->workingDirectory())));

  // Silent: it only sets up the encodings the reader below relies on.
  writeLine(shell.preamble);
  sendMarker();

  m_Input->setFocus();
}

void TerminalViewer::stopShell()
{
  m_Started = false;

  if(m_Process == NULL)
    return;

  QProcess *process = m_Process;
  m_Process = NULL;

  process->disconnect(this);

  if(process->state() != QProcess::NotRunning)
  {
    process->kill();
    process->waitForFinished(3000);
  }

  delete process;

  m_OutputBuffer.clear();
  setBusy(false);
}

void TerminalViewer::writeLine(const QString &line)
{
  if(m_Process == NULL || m_Process->state() != QProcess::Running)
    return;

  const int index = qBound(0, m_ShellCombo->currentIndex(), m_Shells.count() - 1);

  if(index < 0 || m_Shells.isEmpty())
    return;

  const TerminalShell &shell = m_Shells[index];

  QByteArray bytes = shell.inputUtf8 ? line.toUtf8() : line.toLocal8Bit();
  bytes += "\r\n";

  m_Process->write(bytes);
}

void TerminalViewer::sendMarker()
{
  const int index = qBound(0, m_ShellCombo->currentIndex(), m_Shells.count() - 1);

  if(index < 0 || m_Shells.isEmpty())
    return;

  writeLine(m_Shells[index].marker);
}

void TerminalViewer::readOutput()
{
  if(m_Process == NULL)
    return;

  m_OutputBuffer += m_Process->readAllStandardOutput();

  // Only decode whole lines: a UTF-8 sequence split across two reads would otherwise decode to
  // replacement characters in the first half and garbage in the second.
  const int lastNewline = m_OutputBuffer.lastIndexOf('\n');

  if(lastNewline < 0)
    return;

  QByteArray complete = m_OutputBuffer.left(lastNewline + 1);
  m_OutputBuffer.remove(0, lastNewline + 1);

  QString text = m_OutputUtf8 ? QString::fromUtf8(complete)
                              : QTextCodec::codecForLocale()->toUnicode(complete);
  text.replace(lit("\r\n"), lit("\n"));

  const QStringList lines = text.split(QLatin1Char('\n'));

  // The trailing element is empty because the text ends with a newline.
  QString visible;

  for(int i = 0; i + 1 < lines.count(); i++)
  {
    const QString &line = lines[i];

    if(line.startsWith(MARKER_PREFIX))
    {
      if(!visible.isEmpty())
      {
        appendOutput(visible);
        visible.clear();
      }

      setPromptPath(line.mid(MARKER_PREFIX.length()).trimmed());
      setBusy(false);
      continue;
    }

    visible += line;
    visible += QLatin1Char('\n');
  }

  if(!visible.isEmpty())
    appendOutput(visible);
}

void TerminalViewer::appendOutput(const QString &text)
{
  // SGR is the only escape class that changes how the text looks; anything else (cursor movement,
  // window titles, erase-line) is meaningless in a widget that is a scrollback rather than a
  // screen, so it is dropped.
  int start = 0;
  int pos = 0;

  while(pos < text.length())
  {
    if(text[pos] != QChar(27))
    {
      pos++;
      continue;
    }

    if(pos > start)
      appendChunk({text.mid(start, pos - start), m_AnsiColour, m_AnsiBold});

    const int next = applyEscape(text, pos);

    // A dangling escape at the very end of a chunk: skip the byte rather than leave it in the text.
    pos = next > pos ? next : pos + 1;
    start = pos;
  }

  if(start < text.length())
    appendChunk({text.mid(start), m_AnsiColour, m_AnsiBold});
}

int TerminalViewer::applyEscape(const QString &text, int pos)
{
  // text[pos] is the escape character.
  if(pos + 1 >= text.length())
    return pos + 1;

  const QChar kind = text[pos + 1];

  if(kind == QLatin1Char('['))
  {
    int i = pos + 2;
    QString params;

    while(i < text.length() && (text[i] < QChar(0x40) || text[i] > QChar(0x7E)))
    {
      params += text[i];
      i++;
    }

    if(i >= text.length())
      return i;

    if(text[i] == QLatin1Char('m'))
      applySgr(params);

    return i + 1;
  }

  if(kind == QLatin1Char(']'))
  {
    // OSC runs to BEL or to the string terminator.
    int i = pos + 2;

    while(i < text.length() && text[i] != QChar(7))
      i++;

    return i + 1;
  }

  return pos + 2;
}

void TerminalViewer::applySgr(const QString &params)
{
  const QStringList parts = params.split(QLatin1Char(';'), QString::SkipEmptyParts);

  if(parts.isEmpty())
  {
    // A bare ESC[m is a reset.
    m_AnsiColour = m_TextColour;
    m_AnsiBold = false;
    return;
  }

  for(int i = 0; i < parts.count(); i++)
  {
    bool ok = false;
    const int code = parts[i].toInt(&ok);

    if(!ok)
      continue;

    if(code == 0)
    {
      m_AnsiColour = m_TextColour;
      m_AnsiBold = false;
    }
    else if(code == 1)
    {
      m_AnsiBold = true;
    }
    else if(code == 22)
    {
      m_AnsiBold = false;
    }
    else if(code >= 30 && code <= 37)
    {
      m_AnsiColour = m_AnsiTable[code - 30];
    }
    else if(code == 39)
    {
      m_AnsiColour = m_TextColour;
    }
    else if(code >= 90 && code <= 97)
    {
      m_AnsiColour = m_AnsiTable[code - 90 + 8];
    }
    else if(code == 38)
    {
      // Extended foreground colour. Only the 256-colour and true-colour forms are worth following;
      // both consume the following parameters.
      if(i + 2 < parts.count() && parts[i + 1].toInt() == 5)
      {
        const int index = parts[i + 2].toInt();

        if(index >= 0 && index < 16)
        {
          m_AnsiColour = m_AnsiTable[index];
        }
        else if(index >= 16 && index < 232)
        {
          // 6x6x6 colour cube.
          const int cube = index - 16;
          const int steps[6] = {0, 95, 135, 175, 215, 255};
          m_AnsiColour = QColor(steps[cube / 36], steps[(cube / 6) % 6], steps[cube % 6]);
        }
        else if(index >= 232)
        {
          const int grey = 8 + (index - 232) * 10;
          m_AnsiColour = QColor(grey, grey, grey);
        }

        i += 2;
      }
      else if(i + 4 < parts.count() && parts[i + 1].toInt() == 2)
      {
        m_AnsiColour = QColor(qBound(0, parts[i + 2].toInt(), 255),
                              qBound(0, parts[i + 3].toInt(), 255),
                              qBound(0, parts[i + 4].toInt(), 255));
        i += 4;
      }
    }
    // 40-49 and 100-107 set the background. Deliberately ignored: the page is drawn by the theme and
    // a program's block background would fight the selection colour.
  }
}

void TerminalViewer::appendChunk(const TerminalChunk &chunk)
{
  if(chunk.text.isEmpty())
    return;

  QScrollBar *scrollBar = m_Output->verticalScrollBar();
  const bool atEnd = scrollBar->value() + 4 >= scrollBar->maximum();

  QTextCharFormat format;
  format.setForeground(chunk.colour.isValid() ? chunk.colour : m_TextColour);

  if(chunk.bold)
    format.setFontWeight(QFont::Bold);

  QTextCursor cursor(m_Output->document());
  cursor.movePosition(QTextCursor::End);
  cursor.insertText(chunk.text, format);

  if(atEnd)
    scrollBar->setValue(scrollBar->maximum());
}

void TerminalViewer::appendNotice(const QString &text)
{
  appendChunk({lit("[%1] %2\n").arg(QTime::currentTime().toString(lit("hh:mm:ss"))).arg(text),
               m_NoticeColour, false});
}

void TerminalViewer::setPromptPath(const QString &path)
{
  m_PromptPath = path;

  const int index = qBound(0, m_ShellCombo->currentIndex(), m_Shells.count() - 1);
  const bool powershell = (index >= 0 && index < m_Shells.count()) ? m_Shells[index].powershell : true;

  if(path.isEmpty())
    m_PromptLabel->setText(powershell ? lit("PS>") : lit(">"));
  else if(powershell)
    m_PromptLabel->setText(lit("PS %1>").arg(path));
  else
    m_PromptLabel->setText(lit("%1>").arg(path));

  m_PromptLabel->setToolTip(path);
}

void TerminalViewer::setBusy(bool busy)
{
  m_Busy = busy;

  if(!m_Started)
  {
    m_StatusLabel->setText(tr("Not running"));
    m_StatusLabel->setProperty("state", lit("error"));
  }
  else if(busy)
  {
    m_StatusLabel->setText(tr("Running"));
    m_StatusLabel->setProperty("state", lit("busy"));
  }
  else
  {
    m_StatusLabel->setText(tr("Ready"));
    m_StatusLabel->setProperty("state", lit("ok"));
  }

  m_StatusLabel->style()->unpolish(m_StatusLabel);
  m_StatusLabel->style()->polish(m_StatusLabel);
}

void TerminalViewer::runCommand(const QString &command)
{
  if(!m_Started)
  {
    appendNotice(tr("the shell is not running, use the restart button"));
    return;
  }

  if(!command.isEmpty())
  {
    appendChunk({lit("%1 %2\n").arg(m_PromptLabel->text(), command), m_CommandColour, false});
    writeLine(command);
    setBusy(true);
  }

  // Always finish with the marker, even for an empty command: it refreshes the prompt so a bare
  // Enter still shows where the shell is.
  sendMarker();
}

void TerminalViewer::submitInput()
{
  const QString command = m_Input->text();
  m_Input->clear();

  if(!command.trimmed().isEmpty())
  {
    if(m_History.isEmpty() || m_History.last() != command)
      m_History.append(command);

    m_HistoryIndex = m_History.count();
    m_HistoryDraft.clear();
  }

  runCommand(command);

  m_Input->setFocus();
}

void TerminalViewer::clearOutput()
{
  m_Output->clear();
  m_AnsiColour = m_TextColour;
  m_AnsiBold = false;
}

void TerminalViewer::copyOutput()
{
  QTextCursor cursor = m_Output->textCursor();

  if(cursor.hasSelection())
    m_Output->copy();
  else
    QGuiApplication::clipboard()->setText(m_Output->toPlainText());
}

void TerminalViewer::saveOutput()
{
  const QString path = RDDialog::getSaveFileName(this, tr("Save Terminal Output"),
                                                 QDir::homePath(),
                                                 tr("Text files (*.txt);;All files (*)"));

  if(path.isEmpty())
    return;

  QFile file(path);

  if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    RDDialog::critical(this, tr("Error saving output"),
                       tr("Could not open '%1' for writing:\n\n%2").arg(path, file.errorString()));
    return;
  }

  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  stream << m_Output->toPlainText();
}

void TerminalViewer::zoomFont(int delta)
{
  const int size = qBound(MIN_FONT_SIZE, m_FontSize + delta, MAX_FONT_SIZE);

  if(size == m_FontSize)
    return;

  m_FontSize = size;

  QFont font = Formatter::FixedFont();
  font.setPointSize(m_FontSize);

  m_Output->setFont(font);
  m_Output->document()->setDefaultFont(font);
  m_Input->setFont(font);
  m_PromptLabel->setFont(font);
}

bool TerminalViewer::eventFilter(QObject *watched, QEvent *event)
{
  if(event->type() == QEvent::KeyPress)
  {
    QKeyEvent *key = (QKeyEvent *)event;
    const Qt::KeyboardModifiers modifiers = key->modifiers();

    if(watched == m_Input)
    {
      // History walks with the arrow keys, like the shell's own console would.
      if(modifiers == Qt::NoModifier && key->key() == Qt::Key_Up)
      {
        if(m_History.isEmpty())
          return true;

        if(m_HistoryIndex >= m_History.count())
          m_HistoryDraft = m_Input->text();

        m_HistoryIndex = qMax(0, m_HistoryIndex - 1);
        m_Input->setText(m_History[m_HistoryIndex]);
        m_Input->setCursorPosition(m_Input->text().length());
        return true;
      }
      else if(modifiers == Qt::NoModifier && key->key() == Qt::Key_Down)
      {
        if(m_History.isEmpty())
          return true;

        if(m_HistoryIndex < m_History.count() - 1)
        {
          m_HistoryIndex++;
          m_Input->setText(m_History[m_HistoryIndex]);
        }
        else
        {
          m_HistoryIndex = m_History.count();
          m_Input->setText(m_HistoryDraft);
        }

        m_Input->setCursorPosition(m_Input->text().length());
        return true;
      }
    }

    // Ctrl+C is left alone in the input line, where it means copy. Ctrl+Shift+C is the shortcut for
    // taking the output instead.
    if(modifiers == (Qt::ControlModifier | Qt::ShiftModifier) && key->key() == Qt::Key_C)
    {
      copyOutput();
      return true;
    }

    if(modifiers == Qt::ControlModifier)
    {
      if(key->key() == Qt::Key_L)
      {
        clearOutput();
        return true;
      }

      if(watched == m_Input)
      {
        if(key->key() == Qt::Key_U)
        {
          m_Input->clear();
          return true;
        }
        else if(key->key() == Qt::Key_Equal || key->key() == Qt::Key_Plus)
        {
          zoomFont(1);
          return true;
        }
        else if(key->key() == Qt::Key_Minus)
        {
          zoomFont(-1);
          return true;
        }
      }
    }
  }

  return QFrame::eventFilter(watched, event);
}

void TerminalViewer::showEvent(QShowEvent *event)
{
  QFrame::showEvent(event);

  if(m_Input)
    m_Input->setFocus();
}
