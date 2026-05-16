#include "ActionManager.h"
#include "GDBmanager/GDBmanager.h"
#include "MainWindow.h"
#include "compilemanager/CompilerManager.h"
#include "filemanager/CodeEditor.h"
#include "filemanager/EditorTabWidget.h"
#include "filemanager/FileTreeWidget.h"
#include "ui/QssManager.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QTextEdit>
#include <QTextStream>

ActionManager::ActionManager(MainWindow *mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow)
{
    // 创建子模块
    m_compilerManager = new CompilerManager(this);
    m_debuggerManager = new DebuggerManager(this);

    m_compilerManager->setup();

    setupActions();
    setupConnections();
}

ActionManager::~ActionManager() = default;

CompilerManager *ActionManager::compilerManager() const
{
    return m_compilerManager;
}
DebuggerManager *ActionManager::debuggerManager() const
{
    return m_debuggerManager;
}

// ==================== 创建 Menus + Actions ====================

QList<QMenu *> ActionManager::createMenus()
{
    return {m_menuFile, m_menuEdit,  m_menuBuild,    m_menuDebug,
            m_menuView, m_menuTools, m_menuSettings, m_menuHelp};
}

QList<QAction *> ActionManager::createToolBarActions()
{
    return {m_actNew,  m_actOpen,       m_actSave,      m_actUndo,
            m_actRedo, m_actCompile,    m_actRun,       m_actCompileRun,
            m_actStop, m_actDebugStart, m_actDebugStop, m_actFormat,
            m_actJudge};
}

void ActionManager::setupActions()
{
    // ---- 文件菜单 ----
    m_menuFile = new QMenu("文件(&F)", m_mainWindow);

    m_actNew = new QAction("新建(&N)", this);
    m_actNew->setShortcut(QKeySequence::New);

    m_actOpen = new QAction("打开(&O)...", this);
    m_actOpen->setShortcut(QKeySequence::Open);

    m_actSave = new QAction("保存(&S)", this);
    m_actSave->setShortcut(QKeySequence::Save);

    m_actSaveAs = new QAction("另存为(&A)...", this);
    m_actSaveAs->setShortcut(QKeySequence("Ctrl+Shift+S"));

    m_actClose = new QAction("关闭(&W)", this);
    m_actClose->setShortcut(QKeySequence::Close);

    m_actOpenFolder = new QAction("打开文件夹(&K)...", this);
    m_actOpenFolder->setShortcut(QKeySequence("Ctrl+K, Ctrl+O"));

    m_actExit = new QAction("退出(&X)", this);
    m_actExit->setShortcut(QKeySequence("Alt+F4"));

    m_menuFile->addAction(m_actNew);
    m_menuFile->addAction(m_actOpen);
    m_menuFile->addAction(m_actSave);
    m_menuFile->addAction(m_actSaveAs);
    m_menuFile->addAction(m_actClose);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actOpenFolder);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actExit);

    // ---- 编辑菜单 ----
    m_menuEdit = new QMenu("编辑(&E)", m_mainWindow);

    m_actUndo = new QAction("撤销(&Z)", this);
    m_actUndo->setShortcut(QKeySequence::Undo);

    m_actRedo = new QAction("重做(&Y)", this);
    m_actRedo->setShortcut(QKeySequence::Redo);

    m_actCut = new QAction("剪切(&X)", this);
    m_actCut->setShortcut(QKeySequence::Cut);

    m_actCopy = new QAction("复制(&C)", this);
    m_actCopy->setShortcut(QKeySequence::Copy);

    m_actPaste = new QAction("粘贴(&V)", this);
    m_actPaste->setShortcut(QKeySequence::Paste);

    m_actDelete = new QAction("删除(&D)", this);
    m_actDelete->setShortcut(QKeySequence::Delete);

    m_actSelectAll = new QAction("全选(&A)", this);
    m_actSelectAll->setShortcut(QKeySequence::SelectAll);

    m_menuEdit->addAction(m_actUndo);
    m_menuEdit->addAction(m_actRedo);
    m_menuEdit->addSeparator();
    m_menuEdit->addAction(m_actCut);
    m_menuEdit->addAction(m_actCopy);
    m_menuEdit->addAction(m_actPaste);
    m_menuEdit->addAction(m_actDelete);
    m_menuEdit->addSeparator();
    m_menuEdit->addAction(m_actSelectAll);

    // ---- 编译菜单 ----
    m_menuBuild = new QMenu("编译(&B)", m_mainWindow);

    m_actCompile = new QAction("编译(&C)", this);
    m_actCompile->setShortcut(QKeySequence("F5"));

    m_actRun = new QAction("运行(&R)", this);
    m_actRun->setShortcut(QKeySequence("Ctrl+F5"));

    m_actCompileRun = new QAction("编译并运行(&B)", this);
    m_actCompileRun->setShortcut(QKeySequence("F6"));

    m_actStop = new QAction("停止(&S)", this);
    m_actStop->setShortcut(QKeySequence("Shift+F5"));

    m_menuBuild->addAction(m_actCompile);
    m_menuBuild->addAction(m_actRun);
    m_menuBuild->addAction(m_actCompileRun);
    m_menuBuild->addSeparator();
    m_menuBuild->addAction(m_actStop);

    // ---- 调试菜单 ----
    m_menuDebug = new QMenu("调试(&D)", m_mainWindow);

    m_actDebugStart = new QAction("开始调试(&S)", this);
    m_actDebugStart->setShortcut(QKeySequence("F9"));

    m_actDebugStop = new QAction("停止调试(&P)", this);
    m_actDebugStop->setShortcut(QKeySequence("Shift+F9"));

    m_actDebugBreakpoint = new QAction("切换断点(&B)", this);
    m_actDebugBreakpoint->setShortcut(QKeySequence("F10"));

    m_actDebugStepOver = new QAction("逐过程(&O)", this);
    m_actDebugStepOver->setShortcut(QKeySequence("F10"));

    m_actDebugStepInto = new QAction("逐语句(&I)", this);
    m_actDebugStepInto->setShortcut(QKeySequence("F11"));

    m_actDebugStepOut = new QAction("跳出(&U)", this);
    m_actDebugStepOut->setShortcut(QKeySequence("Shift+F11"));

    m_actDebugContinue = new QAction("继续(&C)", this);
    m_actDebugContinue->setShortcut(QKeySequence("F5"));

    m_menuDebug->addAction(m_actDebugStart);
    m_menuDebug->addAction(m_actDebugStop);
    m_menuDebug->addSeparator();
    m_menuDebug->addAction(m_actDebugBreakpoint);
    m_menuDebug->addSeparator();
    m_menuDebug->addAction(m_actDebugStepOver);
    m_menuDebug->addAction(m_actDebugStepInto);
    m_menuDebug->addAction(m_actDebugStepOut);
    m_menuDebug->addAction(m_actDebugContinue);

    // ---- 视图菜单 ----
    m_menuView = new QMenu("视图(&V)", m_mainWindow);

    m_actToggleFileTree = new QAction("文件树(&T)", this);
    m_actToggleFileTree->setCheckable(true);
    m_actToggleFileTree->setChecked(true);

    m_actToggleOutputPanel = new QAction("输出面板(&O)", this);
    m_actToggleOutputPanel->setCheckable(true);
    m_actToggleOutputPanel->setChecked(true);

    m_menuView->addAction(m_actToggleFileTree);
    m_menuView->addAction(m_actToggleOutputPanel);

    // ---- 工具菜单 ----
    m_menuTools = new QMenu("工具(&T)", m_mainWindow);

    m_actFormat = new QAction("格式化代码(&F)", this);
    m_actFormat->setShortcut(QKeySequence("Ctrl+Shift+F"));

    m_actJudge = new QAction("评测(&J)", this);
    m_actJudge->setShortcut(QKeySequence("Ctrl+Shift+J"));

    m_menuTools->addAction(m_actFormat);
    m_menuTools->addSeparator();
    m_menuTools->addAction(m_actJudge);

    // ---- 设置菜单 ----
    m_menuSettings = new QMenu("设置(&S)", m_mainWindow);

    m_actSetCompiler = new QAction("编译器设置(&C)...", this);

    m_menuStyle = new QMenu("格式化风格", m_mainWindow);
    QStringList styles = {"Allman",     "Java",       "K&R",
                          "Stroustrup", "Whitesmith", "GNU"};
    for (const QString &s : styles)
    {
        QAction *act = m_menuStyle->addAction(s);
        act->setCheckable(true);
    }

    m_menuTheme = new QMenu("主题", m_mainWindow);
    // 主题列表将在加载完成后填充

    m_menuSettings->addAction(m_actSetCompiler);
    m_menuSettings->addMenu(m_menuStyle);
    m_menuSettings->addMenu(m_menuTheme);

    // ---- 帮助菜单 ----
    m_menuHelp = new QMenu("帮助(&H)", m_mainWindow);
    QAction *actAbout = m_menuHelp->addAction("关于(&A)...");
}

// ==================== 信号-槽 连接 ====================

void ActionManager::setupConnections()
{
    // 文件操作
    connect(m_actNew, &QAction::triggered, this, &ActionManager::onFileNew);
    connect(m_actOpen, &QAction::triggered, this, &ActionManager::onFileOpen);
    connect(m_actSave, &QAction::triggered, this, &ActionManager::onFileSave);
    connect(m_actSaveAs, &QAction::triggered, this,
            &ActionManager::onFileSaveAs);
    connect(m_actClose, &QAction::triggered, this, &ActionManager::onFileClose);
    connect(m_actOpenFolder, &QAction::triggered, this,
            &ActionManager::onFolderOpen);
    connect(m_actExit, &QAction::triggered, qApp, &QApplication::quit);

    // 编辑操作
    connect(m_actUndo, &QAction::triggered, this, &ActionManager::onEditUndo);
    connect(m_actRedo, &QAction::triggered, this, &ActionManager::onEditRedo);
    connect(m_actCut, &QAction::triggered, this, &ActionManager::onEditCut);
    connect(m_actCopy, &QAction::triggered, this, &ActionManager::onEditCopy);
    connect(m_actPaste, &QAction::triggered, this, &ActionManager::onEditPaste);
    connect(m_actDelete, &QAction::triggered, this,
            &ActionManager::onEditDelete);
    connect(m_actSelectAll, &QAction::triggered, this,
            &ActionManager::onEditSelectAll);

    // 编译运行
    connect(m_actCompile, &QAction::triggered, this, &ActionManager::onCompile);
    connect(m_actRun, &QAction::triggered, this, &ActionManager::onRun);
    connect(m_actCompileRun, &QAction::triggered, this,
            &ActionManager::onCompileAndRun);
    connect(m_actStop, &QAction::triggered, this, &ActionManager::onStop);

    // 调试
    connect(m_actDebugStart, &QAction::triggered, this,
            &ActionManager::onDebugStart);
    connect(m_actDebugStop, &QAction::triggered, this,
            &ActionManager::onDebugStop);
    connect(m_actDebugBreakpoint, &QAction::triggered, this,
            &ActionManager::onDebugToggleBreakpoint);
    connect(m_actDebugStepOver, &QAction::triggered, this,
            &ActionManager::onDebugStepOver);
    connect(m_actDebugStepInto, &QAction::triggered, this,
            &ActionManager::onDebugStepInto);
    connect(m_actDebugStepOut, &QAction::triggered, this,
            &ActionManager::onDebugStepOut);
    connect(m_actDebugContinue, &QAction::triggered, this,
            &ActionManager::onDebugContinue);

    // 格式化
    connect(m_actFormat, &QAction::triggered, this,
            &ActionManager::onFormatCode);

    // 视图
    connect(m_actToggleFileTree, &QAction::toggled, this,
            &ActionManager::onToggleFileTree);
    connect(m_actToggleOutputPanel, &QAction::toggled, this,
            &ActionManager::onToggleOutputPanel);

    // 评测
    connect(m_actJudge, &QAction::triggered, this, &ActionManager::onJudge);

    // 设置
    connect(m_actSetCompiler, &QAction::triggered, this,
            &ActionManager::onSetCompiler);

    // 文件树双击 → 打开文件
    FileTreeWidget *ft = m_mainWindow->fileTree();
    if (ft)
    {
        connect(ft, &FileTreeWidget::fileDoubleClicked, this,
                [this](const QString &path)
                {
                    EditorTabWidget *tabs = m_mainWindow->editorTabs();
                    if (tabs)
                        tabs->openFile(path);
                });
    }

    // 填充主题菜单
    QStringList themes = QssManager::instance().availableThemes();
    for (const QString &theme : themes)
    {
        QAction *act = m_menuTheme->addAction(theme);
        act->setCheckable(true);
        connect(act, &QAction::triggered, this,
                [theme]() { QssManager::instance().loadTheme(theme); });
    }
}

// ==================== 辅助 ====================

CodeEditor *ActionManager::currentEditor() const
{
    EditorTabWidget *tabs = m_mainWindow->editorTabs();
    return tabs ? tabs->currentEditor() : nullptr;
}

void ActionManager::updateWindowTitle()
{
    CodeEditor *editor = currentEditor();
    QString title = "CPPEditor";
    if (editor && !editor->filePath().isEmpty())
    {
        title = QString("%1 - %2").arg(QFileInfo(editor->filePath()).fileName(),
                                       title);
    }
    m_mainWindow->setWindowTitle(title);
}

// ==================== 文件操作实现 ====================

void ActionManager::onFileNew()
{
    EditorTabWidget *tabs = m_mainWindow->editorTabs();
    if (tabs)
        tabs->newFile();
}

void ActionManager::onFileOpen()
{
    EditorTabWidget *tabs = m_mainWindow->editorTabs();
    if (!tabs)
        return;

    QString filePath = QFileDialog::getOpenFileName(
        m_mainWindow, "打开文件", QString(),
        "C++ 源文件 (*.cpp *.h *.hpp *.c *.cc *.cxx);;"
        "所有文件 (*.*)");

    if (!filePath.isEmpty())
        tabs->openFile(filePath);
}

void ActionManager::onFileSave()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    if (editor->filePath().isEmpty())
    {
        onFileSaveAs();
        return;
    }

    QFile file(editor->filePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << editor->toPlainText();
        file.close();
        editor->setModified(false);
    }
    else
    {
        QMessageBox::warning(
            m_mainWindow, "保存失败",
            QString("无法写入文件:\n%1").arg(editor->filePath()));
    }
}

void ActionManager::onFileSaveAs()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    QString filePath = QFileDialog::getSaveFileName(
        m_mainWindow, "另存为", editor->filePath(),
        "C++ 源文件 (*.cpp *.h *.hpp *.c);;所有文件 (*.*)");

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << editor->toPlainText();
        file.close();
        editor->setFilePath(filePath);
        editor->setModified(false);
        updateWindowTitle();
    }
}

void ActionManager::onFileClose()
{
    EditorTabWidget *tabs = m_mainWindow->editorTabs();
    if (tabs)
        tabs->closeFile(tabs->currentIndex());
}

void ActionManager::onFolderOpen()
{
    QString folder =
        QFileDialog::getExistingDirectory(m_mainWindow, "打开文件夹");

    if (folder.isEmpty())
        return;

    FileTreeWidget *ft = m_mainWindow->fileTree();
    if (ft)
        ft->setRootPath(folder);
}

// ==================== 编辑操作实现 ====================

void ActionManager::onEditUndo()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->undo();
}

void ActionManager::onEditRedo()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->redo();
}

void ActionManager::onEditCut()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->cut();
}

void ActionManager::onEditCopy()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->copy();
}

void ActionManager::onEditPaste()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->paste();
}

void ActionManager::onEditDelete()
{
    CodeEditor *editor = currentEditor();
    if (editor)
    {
        QTextCursor cursor = editor->textCursor();
        cursor.removeSelectedText();
    }
}

void ActionManager::onEditSelectAll()
{
    CodeEditor *editor = currentEditor();
    if (editor)
        editor->selectAll();
}

// ==================== 编译与运行 ====================

void ActionManager::appendOutput(const QString &text, const QString &tabName)
{
    QTabWidget *panel = m_mainWindow->outputPanel();
    if (!panel)
        return;

    // 查找对应标签页
    for (int i = 0; i < panel->count(); ++i)
    {
        if (panel->tabText(i) == tabName)
        {
            auto *output = qobject_cast<QTextEdit *>(panel->widget(i));
            if (output)
            {
                output->append(text);
                panel->setCurrentIndex(i);
            }
            return;
        }
    }
}

void ActionManager::onCompile()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
    {
        QMessageBox::information(m_mainWindow, "编译", "没有打开的文件。");
        return;
    }

    // 确保已保存
    if (editor->isModified() || editor->filePath().isEmpty())
        onFileSave();

    if (editor->filePath().isEmpty())
        return;

    QJsonObject result = m_compilerManager->compiling(editor->filePath());

    QString type = result["type"].toString();
    QString message = result["message"].toString();

    appendOutput(message, "编译输出");

    if (type == "success")
    {
        // 记录输出路径供后续运行使用
        QFileInfo fi(editor->filePath());
        m_lastOutputPath = fi.absolutePath() + "/" + fi.completeBaseName();
#ifdef Q_OS_WIN
        m_lastOutputPath += ".exe";
#endif
    }
}

void ActionManager::onRun()
{
    if (m_lastOutputPath.isEmpty())
    {
        // 先尝试编译
        onCompile();
        // 如果编译失败则无法运行
        if (m_lastOutputPath.isEmpty())
            return;
    }

    QJsonObject result = m_compilerManager->running(m_lastOutputPath);
    appendOutput(result["message"].toString(), "编译输出");
}

void ActionManager::onCompileAndRun()
{
    m_lastOutputPath.clear();
    onCompile();
    if (!m_lastOutputPath.isEmpty())
    {
        QJsonObject result = m_compilerManager->running(m_lastOutputPath);
        appendOutput(result["message"].toString(), "编译输出");
    }
}

void ActionManager::onStop()
{
    // TODO: 通过进程管理中止正在运行的编译/调试进程
    appendOutput("[停止] 操作已中止。", "编译输出");
}

// ==================== 调试 ====================

void ActionManager::onDebugStart()
{
    if (m_lastOutputPath.isEmpty())
    {
        onCompile();
        if (m_lastOutputPath.isEmpty())
            return;
    }

    QJsonObject result = m_debuggerManager->startDebug(m_lastOutputPath);
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugStop()
{
    QJsonObject result = m_debuggerManager->stopDebug();
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugToggleBreakpoint()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    int line = editor->textCursor().blockNumber() + 1;
    editor->toggleBreakpoint(line);

    QJsonObject result =
        m_debuggerManager->toggleBreakpoint(editor->filePath(), line);
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugStepOver()
{
    QJsonObject result = m_debuggerManager->stepOver();
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugStepInto()
{
    QJsonObject result = m_debuggerManager->stepInto();
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugStepOut()
{
    QJsonObject result = m_debuggerManager->stepOut();
    appendOutput(result["message"].toString(), "调试输出");
}

void ActionManager::onDebugContinue()
{
    QJsonObject result = m_debuggerManager->continueExec();
    appendOutput(result["message"].toString(), "调试输出");
}

// ==================== 格式化 ====================

void ActionManager::onFormatCode()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
        return;

    // TODO: Phase 4 将连接真实的格式化引擎（clang-format）
    // 目前给出提示
    appendOutput("[格式化] 格式化功能将在 Phase 4 实现。"
                 " 已保存当前文件。",
                 "编译输出");
    onFileSave();
}

void ActionManager::onFormatStyleChanged()
{
    // TODO: Phase 4
}

// ==================== 视图 ====================

void ActionManager::onToggleFileTree()
{
    FileTreeWidget *ft = m_mainWindow->fileTree();
    if (ft)
        ft->setVisible(!ft->isVisible());
}

void ActionManager::onToggleOutputPanel()
{
    QTabWidget *panel = m_mainWindow->outputPanel();
    if (panel)
        panel->setVisible(!panel->isVisible());
}

// ==================== 评测 ====================

void ActionManager::onJudge()
{
    CodeEditor *editor = currentEditor();
    if (!editor)
    {
        QMessageBox::information(m_mainWindow, "评测", "没有打开的文件。");
        return;
    }

    // TODO: Phase 6 连接真实评测引擎
    appendOutput("[评测] 评测功能将在 Phase 6 实现。", "评测结果");
}

// ==================== 设置 ====================

void ActionManager::onSetCompiler()
{
    QString compilerPath =
        QFileDialog::getOpenFileName(m_mainWindow, "选择编译器", QString(),
#ifdef Q_OS_WIN
                                     "可执行文件 (*.exe);;所有文件 (*.*)");
#else
                                     "所有文件 (*)");
#endif

    if (compilerPath.isEmpty())
        return;

    bool ok;
    QString name = QInputDialog::getText(
        m_mainWindow, "编译器名称", "请输入编译器标识名称:", QLineEdit::Normal,
        "GCC", &ok);

    if (!ok)
        return;

    m_compilerManager->setcompile(compilerPath, name);
}

void ActionManager::onSetTheme()
{
    // 通过 QssManager 切换主题，由菜单 action 直接调用
}