/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2026 Baldur Karlsson
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

#include "RDTextEdit.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStringListModel>
#include <QSvgRenderer>
#include "Code/QRDUtils.h"

RDTextEdit::RDTextEdit(QWidget *parent) : QTextEdit(parent)
{
  m_WordCharacters = lit("_");
}

RDTextEdit::~RDTextEdit()
{
}

void RDTextEdit::setSingleLine()
{
  if(m_singleLine)
    return;

  m_singleLine = true;

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setLineWrapMode(QTextEdit::NoWrap);

  document()->setDocumentMargin(0);

  QFontMetrics fm(font());
  const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);

  int height = qMax(fm.height() + 2, iconSize);

  QStyleOptionFrame opt;
  initStyleOption(&opt);
  QSize sz = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(100, height), this);

  setFixedHeight(sz.height());

  QObject::connect(this, &QTextEdit::textChanged, [this]() {
    if(m_singleLine)
    {
      QString text = toPlainText();

      if(text.contains(QLatin1Char('\r')) || text.contains(QLatin1Char('\n')))
      {
        text.replace(lit("\r\n"), lit(" "));
        text.replace(QLatin1Char('\r'), lit(" "));
        text.replace(QLatin1Char('\n'), lit(" "));
        setPlainText(text);
      }
    }
  });
}

void RDTextEdit::setDropDown()
{
  if(m_Drop)
    return;

  m_Drop = new RDTextEditDropDownButton(this);
  m_Drop->setToolButtonStyle(Qt::ToolButtonIconOnly);
  QObject::connect(m_Drop, &QToolButton::clicked, [this]() { emit(dropDownClicked()); });

  updateDropButtonGeometry();

  // leave room for the arrow so the text never runs underneath it
  setMinimumHeight(m_Drop->height() + 2);
}

void RDTextEdit::setHoverTrack()
{
  setAttribute(Qt::WA_Hover);
}

void RDTextEdit::enableCompletion()
{
  if(m_Completer)
    return;

  m_Completer = new QCompleter(this);
  m_Completer->setWidget(this);
  m_Completer->setCompletionMode(QCompleter::PopupCompletion);
  m_Completer->setCaseSensitivity(Qt::CaseInsensitive);
  m_Completer->setWrapAround(false);

  QObject::connect(
      m_Completer, OverloadedSlot<const QString &>::of(&QCompleter::activated),
      [this](const QString &str) {
        QTextCursor cur = textCursor();

        // if we're in the middle of a word, move to the end of it
        QString text = toPlainText();
        if(cur.position() > 0 &&
           (text[cur.position() - 1].isLetterOrNumber() ||
            m_WordCharacters.contains(text[cur.position() - 1])) &&
           (text[cur.position()].isLetterOrNumber() ||
            m_WordCharacters.contains(text[cur.position()])))
        {
          cur.movePosition(QTextCursor::EndOfWord);
        }

        // insert what's remaining of the word, after the prefix which is what's already there
        cur.insertText(str.right(str.length() - m_Completer->completionPrefix().length()));
        setTextCursor(cur);
      });
  m_Completer->popup()->installEventFilter(this);

  m_Completer->setCompletionRole(Qt::DisplayRole);

  m_CompletionModel = new QStringListModel(this);

  m_Completer->setModel(m_CompletionModel);
  m_Completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
}

bool RDTextEdit::eventFilter(QObject *watched, QEvent *event)
{
  if(m_Completer && watched == m_Completer->popup() && event->type() == QEvent::Hide)
  {
    emit(completionEnd());
  }

  return QTextEdit::eventFilter(watched, event);
}

void RDTextEdit::setCompletionWordCharacters(QString chars)
{
  m_WordCharacters = chars;
}

void RDTextEdit::setCompletionStrings(QStringList list)
{
  if(m_CompletionModel)
  {
    list.sort(Qt::CaseInsensitive);
    m_CompletionModel->setStringList(list);
  }
}

bool RDTextEdit::completionInProgress()
{
  return m_Completer && m_Completer->popup()->isVisible();
}

void RDTextEdit::focusInEvent(QFocusEvent *e)
{
  QTextEdit::focusInEvent(e);
  emit(enter());
}

void RDTextEdit::focusOutEvent(QFocusEvent *e)
{
  QTextEdit::focusOutEvent(e);
  emit(leave());
}

void RDTextEdit::keyPressEvent(QKeyEvent *e)
{
  if(completionInProgress())
  {
    switch(e->key())
    {
      // if a completion is in progress ignore any events the completer will process
      case Qt::Key_Return:
      case Qt::Key_Enter:
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
      case Qt::Key_Escape: e->ignore(); return;

      // also the completer doesn't hide itself when the cursor is moved so make sure we do that
      // ourselves
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Home:
      case Qt::Key_End:
        m_Completer->popup()->hide();
        emit(completionEnd());
        break;
      default: break;
    }
  }

  if(m_singleLine)
  {
    if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
      emit(keyPress(e));
      e->accept();
      return;
    }
  }

  bool completionShortcut = false;

  if(m_Completer && m_CompletionModel)
  {
    // tab triggers completion
    if(e->key() == Qt::Key_Tab)
      completionShortcut = true;

    // as does ctrl-space and ctrl-E
    if(e->modifiers() & Qt::ControlModifier)
    {
      if(e->key() == Qt::Key_E)
        completionShortcut = true;
      if(e->key() == Qt::Key_Space)
        completionShortcut = true;
    }
  }

  // add ctrl-end and ctrl-home shortcuts, which aren't implemented for read-only edits
  if(e->key() == Qt::Key_End && (e->modifiers() & Qt::ControlModifier))
  {
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
  }
  else if(e->key() == Qt::Key_Home && (e->modifiers() & Qt::ControlModifier))
  {
    verticalScrollBar()->setValue(verticalScrollBar()->minimum());
  }
  else if(!completionShortcut)
  {
    QTextEdit::keyPressEvent(e);
  }

  // stop completing if the character just entered is not a word-compatible character
  if(completionInProgress() && e->text().length() > 0)
  {
    QChar c = e->text()[0];
    if(c.isPrint() && !e->text()[0].isLetterOrNumber() && !m_WordCharacters.contains(e->text()[0]))
    {
      if(c.isSpace() && m_Completer->completionPrefix().trimmed() == QString())
      {
        // don't do anything if we have no prefix so far and the user enters whitespace
      }
      else
      {
        m_Completer->popup()->hide();
        emit(completionEnd());
      }
    }
  }

  emit(keyPress(e));

  // update the completion if it's in progress, or we have our shortcut (and there's no selected
  // text)
  if((completionShortcut && !textCursor().hasSelection()) || completionInProgress())
  {
    triggerCompletion();

    if(completionShortcut)
      e->accept();
  }
}

void RDTextEdit::triggerCompletion()
{
  QString text = toPlainText();

  // start at the cursor position
  int end = textCursor().position();
  int start = end;

  // glob forward through word characters
  while(end < text.length())
  {
    QChar c = text.at(end);
    if(c.isLetterOrNumber() || m_WordCharacters.contains(c))
    {
      end++;
      continue;
    }

    break;
  }

  // glob backwards through word characters too
  while(start > 0)
  {
    start--;

    QChar c = text.at(start);
    if(!c.isLetterOrNumber() && !m_WordCharacters.contains(c))
    {
      start++;
      break;
    }
  }

  // grab the current prefix to be working with
  QString prefix = text.mid(start, end - start);

  bool startedCompletion = false;

  if(!completionInProgress())
  {
    emit(completionBegin(prefix));
    startedCompletion = true;
  }

  // stop completing if text is selected or if there are no candidates
  if(m_CompletionModel->stringList().empty() || textCursor().hasSelection())
  {
    if(startedCompletion || completionInProgress())
    {
      m_Completer->popup()->hide();
      emit(completionEnd());
    }
  }
  else
  {
    // update the prefix as needed
    if(prefix != m_Completer->completionPrefix())
      m_Completer->setCompletionPrefix(prefix);

    // select the first item
    m_Completer->popup()->setCurrentIndex(m_Completer->completionModel()->index(0, 0));

    QRect r = cursorRect();
    r.setWidth(m_Completer->popup()->sizeHintForColumn(0) +
               m_Completer->popup()->verticalScrollBar()->sizeHint().width());
    m_Completer->complete(r);

    // we have to start the completion to get the list of suggestions, but if none of them matched
    // and the popup never appeared we need to end it here as we won't get another notification
    if(startedCompletion && !completionInProgress())
    {
      m_Completer->popup()->hide();
      emit(completionEnd());
    }
  }
}

void RDTextEdit::mouseMoveEvent(QMouseEvent *e)
{
  emit(mouseMoved(e));
  QTextEdit::mouseMoveEvent(e);
}

void RDTextEdit::resizeEvent(QResizeEvent *e)
{
  updateDropButtonGeometry();

  QTextEdit::resizeEvent(e);
}

void RDTextEdit::updateDropButtonGeometry()
{
  if(!m_Drop)
    return;

  // Size and place the button exactly like a combo box places its drop-down, using a combo box to
  // ask the style: as large as the arrow cell, flush inside the frame. This has to be recomputed on
  // every resize, because the height of the cell follows the height of the widget.
  QComboBox reference;
  QStyleOptionComboBox opt;
  opt.initFrom(&reference);
  opt.rect = rect();
  opt.editable = true;
  const QRect cell =
      reference.style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow,
                                       &reference);

  m_Drop->setFixedSize(cell.size());
  m_Drop->setGeometry(QRect(rect().right() - cell.width(), (height() - cell.height()) / 2,
                            cell.width(), cell.height()));

  if(viewportMargins().right() != cell.width())
    setViewportMargins(0, 0, cell.width(), 0);
}

bool RDTextEdit::event(QEvent *e)
{
  if(e->type() == QEvent::HoverEnter)
  {
    emit(hoverEnter());
  }
  else if(e->type() == QEvent::HoverLeave)
  {
    emit(hoverLeave());
  }
  return QTextEdit::event(e);
}

RDTextEditDropDownButton::RDTextEditDropDownButton(QWidget *parent) : QToolButton(parent)
{
}

RDTextEditDropDownButton::~RDTextEditDropDownButton()
{
}

void RDTextEditDropDownButton::paintEvent(QPaintEvent *)
{
  // Draw the drop-down on the widget that takes the clicks. The size comes from
  // RDTextEdit::updateDropButtonGeometry(), which asks the style for the same cell a combo box gets,
  // and the arrow is the same chevron-down.svg that the combo boxes draw - so the two look the same
  // side by side in every style and theme.
  // 12x12 logical pixels is the size the stylesheets give ::down-arrow
  const int arrow = qMin(12, qMin(width(), height()));
  // the stylesheets centre a smaller image by rounding up, so round up here too
  const QRect arrowRect(QPoint((width() - arrow + 1) / 2, (height() - arrow + 1) / 2),
                        QSize(arrow, arrow));

  // .. and the icon itself is a white line drawing, which only reads on a dark background
  QColor colour = palette().color(QPalette::WindowText);
  if(palette().color(QPalette::Base).lightness() < 128)
    colour = Qt::white;

  QSvgRenderer svg(QStringLiteral(":/UE/chevron-down.svg"));

  const qreal dpr = devicePixelRatioF();
  QImage image(arrowRect.size() * dpr, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  {
    QPainter icon(&image);
    svg.render(&icon, QRectF(QPointF(0, 0), QSizeF(image.size())));
    icon.setCompositionMode(QPainter::CompositionMode_SourceIn);
    icon.fillRect(image.rect(), colour);
  }
  image.setDevicePixelRatio(dpr);

  QPainter p(this);

  // the separator between the text and the drop-down, as on a combo box
  p.setPen(palette().color(QPalette::Mid));
  p.drawLine(0, 0, 0, height() - 1);

  p.setRenderHint(QPainter::SmoothPixmapTransform);
  p.drawImage(arrowRect.topLeft(), image);
}
