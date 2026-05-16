#pragma once

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QPaintEvent;
class QResizeEvent;

/**
 * @brief C++ 代码编辑器 —— 带行号、断点栏、语法高亮
 *
 * Phase 2 将完整实现。此处提供最小头文件。
 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

  public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;

    // 断点标记（Phase 5 连接 GDBmanager）
    QList<int> breakpointLines() const;
    void toggleBreakpoint(int line);
    void clearBreakpoints();

    // 文件路径关联
    QString filePath() const;
    void setFilePath(const QString &path);
    bool isModified() const;
    void setModified(bool modified);

  signals:
    void modificationChanged(bool modified);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

  private:
    QWidget *m_lineNumberArea = nullptr;
    QString m_filePath;
    bool m_isModified = false;
    QList<int> m_breakpointLines;
    int m_tabSize = 4;
};

/**
 * @brief C++ 语法高亮器
 */
class CppHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

  public:
    explicit CppHighlighter(QTextDocument *parent = nullptr);

  protected:
    void highlightBlock(const QString &text) override;

  private:
    struct HighlightRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightRule> m_rules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_classFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_quotationFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_singleLineCommentFormat;
};