#pragma once

#include <QTabWidget>

class CodeEditor;

/**
 * @brief 标签页管理器 —— 管理多个打开文件的 CodeEditor
 *
 * Phase 2 将完整实现。
 */
class EditorTabWidget : public QTabWidget
{
    Q_OBJECT

  public:
    explicit EditorTabWidget(QWidget *parent = nullptr);

    CodeEditor *openFile(const QString &filePath);
    CodeEditor *newFile();
    bool closeFile(int index);
    bool closeAllFiles();

    CodeEditor *currentEditor() const;
    int tabCount() const;
    QString tabFilePath(int index) const;

  signals:
    void currentFileChanged(const QString &filePath);
    void fileClosed(const QString &filePath);

  private:
    void connectEditorSignals(CodeEditor *editor);
};