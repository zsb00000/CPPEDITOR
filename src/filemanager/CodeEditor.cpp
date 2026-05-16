#include "CodeEditor.h"
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>

// ==================== CodeEditor 实现 ====================

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    // 行号区域
    m_lineNumberArea = new QWidget(this);
    m_lineNumberArea->setObjectName("LineNumberArea");

    connect(this, &CodeEditor::blockCountChanged, this,
            &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this,
            &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this,
            &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // 设置等宽字体
    QFont font("Consolas", 13);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    // Tab 大小
    QFontMetrics fm(font);
    setTabStopDistance(fm.horizontalAdvance(' ') * m_tabSize);

    // 语法高亮
    new CppHighlighter(document());

    // 连接修改状态
    connect(this, &CodeEditor::modificationChanged, this,
            [this](bool changed)
            {
                if (changed)
                    setModified(true);
            });
}

// ---- 行号区域 ----
int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }
    // 空间 = 边距 + 数字宽度 + 断点栏 + 右边距
    int space = 16 +
                fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits +
                16 + 8;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(),
                                 rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(180);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#f0f0f0"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top =
        qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int lineNumAreaWidth = m_lineNumberArea->width();
    int breakpointColumnX = 8;   // 断点栏起始
    int breakpointColWidth = 12; // 断点栏宽度
    int lineNumStartX = breakpointColumnX + breakpointColWidth + 4;

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            int lineNumber = blockNumber + 1;

            // 画断点标记
            if (m_breakpointLines.contains(lineNumber))
            {
                painter.setBrush(QColor("#e51400"));
                painter.setPen(Qt::NoPen);
                int bpCenterY = top + (bottom - top) / 2;
                painter.drawEllipse(
                    QPoint(breakpointColumnX + breakpointColWidth / 2,
                           bpCenterY),
                    5, 5);
            }

            // 画行号
            QString number = QString::number(lineNumber);
            painter.setPen(QColor("#888888"));
            painter.drawText(lineNumStartX, top,
                             lineNumAreaWidth - lineNumStartX - 4,
                             fontMetrics().height(), Qt::AlignLeft, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---- 断点 ----
QList<int> CodeEditor::breakpointLines() const { return m_breakpointLines; }

void CodeEditor::toggleBreakpoint(int line)
{
    if (m_breakpointLines.contains(line))
        m_breakpointLines.removeAll(line);
    else
        m_breakpointLines.append(line);
    m_lineNumberArea->update();
}

void CodeEditor::clearBreakpoints()
{
    m_breakpointLines.clear();
    m_lineNumberArea->update();
}

// ---- 文件路径 ----
QString CodeEditor::filePath() const { return m_filePath; }
void CodeEditor::setFilePath(const QString &path) { m_filePath = path; }
bool CodeEditor::isModified() const { return m_isModified; }
void CodeEditor::setModified(bool modified)
{
    m_isModified = modified;
    emit modificationChanged(modified);
}

// ==================== CppHighlighter 实现 ====================

CppHighlighter::CppHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // 关键字
    m_keywordFormat.setForeground(QColor("#0000ff"));
    m_keywordFormat.setFontWeight(QFont::Bold);

    const QStringList keywords = {"alignas",
                                  "alignof",
                                  "and",
                                  "and_eq",
                                  "asm",
                                  "auto",
                                  "bitand",
                                  "bitor",
                                  "bool",
                                  "break",
                                  "case",
                                  "catch",
                                  "char",
                                  "char16_t",
                                  "char32_t",
                                  "char8_t",
                                  "class",
                                  "co_await",
                                  "co_return",
                                  "co_yield",
                                  "compl",
                                  "concept",
                                  "const",
                                  "const_cast",
                                  "consteval",
                                  "constexpr",
                                  "constinit",
                                  "continue",
                                  "decltype",
                                  "default",
                                  "delete",
                                  "do",
                                  "double",
                                  "dynamic_cast",
                                  "else",
                                  "enum",
                                  "explicit",
                                  "export",
                                  "extern",
                                  "false",
                                  "final",
                                  "float",
                                  "for",
                                  "friend",
                                  "goto",
                                  "if",
                                  "import",
                                  "inline",
                                  "int",
                                  "long",
                                  "module",
                                  "mutable",
                                  "namespace",
                                  "new",
                                  "noexcept",
                                  "not",
                                  "not_eq",
                                  "nullptr",
                                  "operator",
                                  "or",
                                  "or_eq",
                                  "override",
                                  "private",
                                  "protected",
                                  "public",
                                  "register",
                                  "reinterpret_cast",
                                  "requires",
                                  "return",
                                  "short",
                                  "signed",
                                  "sizeof",
                                  "static",
                                  "static_assert",
                                  "static_cast",
                                  "struct",
                                  "switch",
                                  "template",
                                  "this",
                                  "thread_local",
                                  "throw",
                                  "true",
                                  "try",
                                  "typedef",
                                  "typeid",
                                  "typename",
                                  "union",
                                  "unsigned",
                                  "using",
                                  "virtual",
                                  "void",
                                  "volatile",
                                  "wchar_t",
                                  "while",
                                  "xor",
                                  "xor_eq"};

    for (const QString &kw : keywords)
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(kw));
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // 类名（首字母大写的单词后跟 :: 或 <）
    m_classFormat.setForeground(QColor("#267f99"));
    m_classFormat.setFontWeight(QFont::Bold);
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b[A-Z][a-zA-Z0-9_]*\\b");
        rule.format = m_classFormat;
        m_rules.append(rule);
    }

    // 单行注释
    m_singleLineCommentFormat.setForeground(QColor("#008000"));
    m_singleLineCommentFormat.setFontItalic(true);
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = m_singleLineCommentFormat;
        m_rules.append(rule);
    }

    // 多行注释（在 highlightBlock 中特殊处理）
    m_commentFormat.setForeground(QColor("#008000"));
    m_commentFormat.setFontItalic(true);

    // 字符串（双引号）
    m_quotationFormat.setForeground(QColor("#a31515"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = m_quotationFormat;
        m_rules.append(rule);
    }

    // 字符串（单引号字符）
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("'.'");
        rule.format = m_quotationFormat;
        m_rules.append(rule);
    }

    // 函数调用
    m_functionFormat.setForeground(QColor("#795e26"));
    {
        HighlightRule rule;
        rule.pattern =
            QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
        rule.format = m_functionFormat;
        m_rules.append(rule);
    }

    // 预处理器指令
    m_preprocessorFormat.setForeground(QColor("#9b9b9b"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("^#\\s*\\w+.*");
        rule.format = m_preprocessorFormat;
        m_rules.append(rule);
    }

    // 数字
    m_numberFormat.setForeground(QColor("#098658"));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression(
            "\\b(0x[0-9a-fA-F]+|0b[01]+|\\d+\\.?\\d*[fFlL]?)\\b");
        rule.format = m_numberFormat;
        m_rules.append(rule);
    }
}

void CppHighlighter::highlightBlock(const QString &text)
{
    // 先应用规则
    for (const HighlightRule &rule : m_rules)
    {
        QRegularExpressionMatchIterator matchIterator =
            rule.pattern.globalMatch(text);
        while (matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(),
                      rule.format);
        }
    }

    // 处理多行注释 /* ... */
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf("/*");

    while (startIndex >= 0)
    {
        QRegularExpressionMatch match =
            QRegularExpression("\\*/").match(text, startIndex);
        int endIndex = match.hasMatch() ? match.capturedStart() : -1;
        int commentLength;
        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, m_commentFormat);
        startIndex = text.indexOf("/*", startIndex + commentLength);
    }
}