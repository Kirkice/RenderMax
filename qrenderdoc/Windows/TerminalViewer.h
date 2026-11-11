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
#include <QFrame>
#include <QPointer>
#include <QJsonObject>
class QComboBox;
class QLabel;
class QTimer;
class TerminalViewer : public QFrame
{
  Q_OBJECT
public:
  explicit TerminalViewer(QWidget *parent = NULL);
  ~TerminalViewer();
  void restartShell();
private:
  void initialise();
  void stopShell();
  void poll();
  void message(const QString &json);
  void send(const QJsonObject &object);
  void resizeRenderer();
  bool eventFilter(QObject *watched,QEvent *event) override;
  void showEvent(QShowEvent *event) override;
  struct Native;
  Native *n;
  QComboBox *m_Shell = NULL;
  QLabel *m_Status = NULL;
  QWidget *m_Host = NULL;
  QTimer *m_Timer = NULL;
};
