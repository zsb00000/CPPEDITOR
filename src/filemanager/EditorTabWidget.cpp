#include "EditorTabWidget.h"
#include "CodeEditor.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTabBar>
#include <QTextStream>

EditorTabWidget::EditorTabWidget(QWidget *parent) : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);

    // 关闭标签页按钮
    connect(this, &EditorTabWidget::tabCloseRequested, this,
            &EditorTabWidget::closeFile);

    // 切换标签页信号
    connect(this, &QTabWidget::currentChanged, this,
            [this](int /* index */)
            {
                CodeEditor *editor = currentEditor();
                if (editor)
                    emit currentFileChanged(editor->filePath());
            });
}

CodeEditor *EditorTabWidget::openFile(const QString &filePath)
{
    // 检查是否已打开
    for (int i = 0; i < count(); ++i)
    {
        CodeEditor *ed = qobject_cast<CodeEditor *>(widget(i));
        if (ed && ed->filePath() == filePath)
        {
            setCurrentIndex(i);
            return ed;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "打开失败",
                             QString("无法打开文件:\n%1").arg(filePath));
        return nullptr;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    auto *editor = new CodeEditor(this);
    editor->setPlainText(content);
    editor->setFilePath(filePath);
    editor->setModified(false);
    connectEditorSignals(editor);

    QFileInfo fi(filePath);
    int idx = addTab(editor, fi.fileName());
    setCurrentIndex(idx);
    setTabToolTip(idx, filePath);

    return editor;
}

CodeEditor *EditorTabWidget::newFile()
{
    auto *editor = new CodeEditor(this);
    editor->setFilePath(QString());
    editor->setModified(false);
    connectEditorSignals(editor);

    static int untitledCounter = 0;
    int idx = addTab(editor, QString("未命名%1").arg(++untitledCounter));
    setCurrentIndex(idx);

    return editor;
}

bool EditorTabWidget::closeFile(int index)
{
    if (index < 0 || index >= count())
        return false;

    CodeEditor *editor = qobject_cast<CodeEditor *>(widget(index));
    if (!editor)
        return false;

    // 检查是否需保存
    if (editor->isModified())
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "保存更改",
            QString("文件 \"%1\" 已修改，是否保存？").arg(tabText(index)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save)
        {
            // 由外部 ActionManager 处理保存，这里简单 emit 信号
            // 实际保存逻辑在 ActionManager::onFileSave 中
        }
        else if (reply == QMessageBox::Cancel)
        {
            return false;
        }
    }

    QString closedPath = editor->filePath();
    removeTab(index);
    editor->deleteLater();
    emit fileClosed(closedPath);
    return true;
}

bool EditorTabWidget::closeAllFiles()
{
    while (count() > 0)
    {
        if (!closeFile(0))
            return false;
    }
    return true;
}

CodeEditor *EditorTabWidget::currentEditor() const
{
    return qobject_cast<CodeEditor *>(currentWidget());
}

int EditorTabWidget::tabCount() const { return count(); }

QString EditorTabWidget::tabFilePath(int index) const
{
    CodeEditor *editor = qobject_cast<CodeEditor *>(widget(index));
    return editor ? editor->filePath() : QString();
}

void EditorTabWidget::connectEditorSignals(CodeEditor *editor)
{
    // 修改状态改变时更新标签页标题
    connect(editor, &CodeEditor::modificationChanged, this,
            [this, editor](bool modified)
            {
                int idx = indexOf(editor);
                if (idx < 0)
                    return;
                QString title = tabText(idx);
                if (modified && !title.endsWith(" *"))
                    setTabText(idx, title + " *");
                else if (!modified && title.endsWith(" *"))
                    setTabText(idx, title.left(title.length() - 2));
            });
}