/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QtGui>
#include <QPlainTextEdit>

namespace FeatherPad {

/* First, I subclassed QTextEdit just to gain control
   over pressing Enter and have auto-indentation.
   Afterward, I changed it to QPlainTextEdit and
   added codes for showing line numbers, adapted
   from "Qt4 Documentation/widgets-codeeditor.html".
   Then, I also changed the default DnD.
   At last, I replaced the vertical scrollbar. */
class TextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEdit (QWidget *parent = 0);
    ~TextEdit();

    void lineNumberAreaPaintEvent (QPaintEvent *event);
    int lineNumberAreaWidth();
    void showLineNumbers (bool show);

    bool autoIndentation;
    QTextEdit::ExtraSelection currentLine;
    void setScrollJumpWorkaround (bool apply)
    {
        scrollJumpWorkaround = apply;
    }

signals:
    /* inform the main widget */
    void fileDropped (const QString&);

protected:
    virtual void keyPressEvent (QKeyEvent *event)
    {
        if (isReadOnly())
        {
            QPlainTextEdit::keyPressEvent (event);
            return;
        }

        if (autoIndentation && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
        {
            /* first get the current cursor for computing the indentation */
            QTextCursor start = textCursor();
            QString indent = computeIndentation (start);
            /* then press Enter normally... */
            QPlainTextEdit::keyPressEvent (event);
            /* ... and insert indentation */
            start = textCursor();
            start.insertText (indent);
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
        {
            /* when text is selected, use arrow keys
               to go to the start or end of the selection */
            QTextCursor cursor = textCursor();
            if (event->modifiers() == Qt::NoModifier && cursor.hasSelection())
            {
                QString str = cursor.selectedText();
                if (event->key() == Qt::Key_Left)
                {
                    if (str.isRightToLeft())
                        cursor.setPosition (cursor.selectionEnd());
                    else
                        cursor.setPosition (cursor.selectionStart());
                }
                else
                {
                    if (str.isRightToLeft())
                        cursor.setPosition (cursor.selectionStart());
                    else
                        cursor.setPosition (cursor.selectionEnd());
                }
                cursor.clearSelection();
                setTextCursor (cursor);
                event->accept();
                return;
            }
        }
        /* because of a bug in Qt5, the non-breaking space (ZWNJ) isn't inserted with SHIFT+SPACE */
        else if (event->key() == 0x200c)
        {
            insertPlainText (QChar (0x200C));
            event->accept();
            return;
        }

        QPlainTextEdit::keyPressEvent (event);
    }

    // a workaround for Qt5's scroll jump bug
    virtual void wheelEvent (QWheelEvent *event)
    {
        if (scrollJumpWorkaround && event->angleDelta().manhattanLength() > 240)
            event->ignore();
        else
            QPlainTextEdit::wheelEvent (event);
    }

    void resizeEvent (QResizeEvent *event);

    /* we want to pass dropping of files to
       the main widget with a custom signal */
    bool canInsertFromMimeData (const QMimeData* source) const
    {
        return source->hasUrls() || QPlainTextEdit::canInsertFromMimeData (source);
    }
    void insertFromMimeData (const QMimeData* source)
    {
        if (source->hasUrls())
        {
            foreach (QUrl url, source->urls())
                emit fileDropped (url.toLocalFile());
        }
        else
            QPlainTextEdit::insertFromMimeData (source);
    }

private slots:
    void updateLineNumberAreaWidth (int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea (const QRect&, int);

private:
    QString computeIndentation (QTextCursor& cur) const
    {
        if (cur.hasSelection())
        {// this is more intuitive to me
            if (cur.anchor() <= cur.position())
                cur.setPosition (cur.anchor());
            else
                cur.setPosition (cur.position());
        }
        QTextCursor tmp = cur;
        tmp.movePosition (QTextCursor::StartOfBlock);
        QString str;
        if (tmp.atBlockEnd())
            return str;
        int pos = tmp.position();
        tmp.setPosition (++pos, QTextCursor::KeepAnchor);
        QString selected;
        while (!tmp.atBlockEnd()
               && tmp <= cur
               && ((selected = tmp.selectedText()) == " "
                   || (selected = tmp.selectedText()) == "\t"))
        {
            str.append (selected);
            tmp.setPosition (pos);
            tmp.setPosition (++pos, QTextCursor::KeepAnchor);
        }
        if (tmp.atBlockEnd()
            && tmp <= cur
            && ((selected = tmp.selectedText()) == " "
                || (selected = tmp.selectedText()) == "\t"))
        {
            str.append (selected);
        }
        return str;
    }

    QWidget *lineNumberArea;
    bool scrollJumpWorkaround; // for working around Qt5's scroll jump bug
};
/*************************/
class LineNumberArea : public QWidget
{
public:
    LineNumberArea (TextEdit *Editor) : QWidget (Editor)
    {
        editor = Editor;
    }

    QSize sizeHint() const
    {
        return QSize (editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        editor->lineNumberAreaPaintEvent (event);
    }

private:
    TextEdit *editor;
};

}

#endif // TEXTEDIT_H
