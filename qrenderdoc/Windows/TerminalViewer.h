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

#pragma once

#include <QColor>
#include <QFrame>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QToolButton;

// One shell the panel can drive. The shell is spawned as a child process with its stdin/stdout
// piped to the panel, which is the closest thing to a terminal Windows offers without a ConPTY
// handle: there is no pseudo-console, so anything that requires one (a full-screen editor, the
// progress-bar rendering PowerShell does when talking to a real console) falls back to the plain
// redirected behaviour of the shell.
struct TerminalShell
{
  QString name;
  QString program;
  QStringList arguments;

  // Sent once, before the first command. This puts the shell's stdout into UTF-8 regardless of the
  // console codepage, so everything read back can be decoded as UTF-8.
  QString preamble;

  // Echoed after every command to learn the current directory and to know that the command has
  // finished. The shell prints this as a whole line and it is filtered out of the output.
  QString marker;

  // Whether commands must be written as UTF-8. The Windows shells read stdin through the console
  // input codepage, so commands match the local 8-bit encoding; a shell switched to codepage 65001
  // takes UTF-8 instead.
  bool inputUtf8 = false;

  // Whether the output comes back as UTF-8. PowerShell is switched over by the preamble; cmd keeps
  // the console codepage, so its output is decoded with the local 8-bit encoding instead.
  bool outputUtf8 = true;

  // PowerShell prompt shape, as opposed to the bare "C:\dir>" of cmd.
  bool powershell = true;
};

// A block of output with one appearance. Splitting at escapes means a QPlainTextEdit document keeps
// the colours a program asked for without the panel having to keep terminal state around.
struct TerminalChunk
{
  QString text;
  QColor colour;
  bool bold = false;
};

class TerminalViewer : public QFrame
{
  Q_OBJECT

public:
  explicit TerminalViewer(QWidget *parent = NULL);
  ~TerminalViewer();

  // Runs a command as if it had been typed, then returns. Used by the panel's own controls and
  // available to anything holding the viewer.
  void runCommand(const QString &command);

  // Kills the shell process and starts a fresh one with the currently selected shell.
  void restartShell();

private:
  void createUi();
  void setupColours();
  void populateShells();

  void startShell();
  void stopShell();
  void writeLine(const QString &line);
  void sendMarker();

  void readOutput();
  void appendOutput(const QString &text);
  void appendChunk(const TerminalChunk &chunk);
  void appendNotice(const QString &text);
  void scrollIfAtEnd();

  int applyEscape(const QString &text, int pos);
  void applySgr(const QString &params);

  void setPromptPath(const QString &path);
  void setBusy(bool busy);

  void submitInput();
  void clearOutput();
  void copyOutput();
  void saveOutput();
  void changeShell(int index);
  void zoomFont(int delta);

  bool eventFilter(QObject *watched, QEvent *event) override;
  void showEvent(QShowEvent *event) override;

  QComboBox *m_ShellCombo = NULL;
  QLabel *m_PromptLabel = NULL;
  QLineEdit *m_Input = NULL;
  QPlainTextEdit *m_Output = NULL;
  QLabel *m_StatusLabel = NULL;
  QToolButton *m_RestartButton = NULL;
  QToolButton *m_ClearButton = NULL;
  QToolButton *m_CopyButton = NULL;
  QToolButton *m_SaveButton = NULL;

  QProcess *m_Process = NULL;

  QVector<TerminalShell> m_Shells;
  // Bytes that have arrived but do not yet form a whole line, so a UTF-8 sequence split across two
  // reads is decoded once rather than turned into replacement characters.
  QByteArray m_OutputBuffer;

  QStringList m_History;
  int m_HistoryIndex = 0;
  QString m_HistoryDraft;

  // Prompt state, refreshed by the marker the shell echoes after each command.
  QString m_PromptPath;
  bool m_Busy = false;
  bool m_Started = false;
  bool m_OutputUtf8 = true;

  // Escape-sequence state: an SGR run can span several lines, so the current colour survives
  // between chunks.
  QColor m_AnsiColour;
  bool m_AnsiBold = false;

  // Palette, resolved once from the theme. The panel is rebuilt when the style changes only in the
  // sense that a fresh one picks up the new colours; the dock keeps its instance otherwise.
  QColor m_TextColour;
  QColor m_NoticeColour;
  QColor m_CommandColour;
  QColor m_AnsiTable[16];

  int m_FontSize = 0;
};
