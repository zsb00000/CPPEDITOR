#pragma once

#include <QMenu>
#include <QObject>

class QAction;
class QMenu;
class QWidget;
class MainWindow;
class CompilerManager;
class DebuggerManager;
class FileTreeWidget;
class EditorTabWidget;
class CodeEditor;
class QssManager;

/**
 * @brief 动作管理器 —— 连接菜单/快捷键到各模块
 *
 * 职责：
 * 1. 创建所有 QAction / QMenu
 * 2. 连接信号-槽，协调模块间交互
 * 3. 管理编译→运行→调试→评测的命令流程
 */
class ActionManager : public QObject
{
    Q_OBJECT

  public:
    explicit ActionManager(MainWindow *mainWindow);
    ~ActionManager() override;

    // 获取菜单栏（MainWindow 调用 setupMenuBar 时安装）
    QList<QMenu *> createMenus();

    // 工具栏 action 列表
    QList<QAction *> createToolBarActions();

    // 模块访问器
    CompilerManager *compilerManager() const;
    DebuggerManager *debuggerManager() const;

  private slots:
    // ---- 文件操作 ----
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileClose();
    void onFolderOpen();

    // ---- 编辑操作 ----
    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditPaste();
    void onEditDelete();
    void onEditSelectAll();

    // ---- 编译与运行 ----
    void onCompile();
    void onRun();
    void onCompileAndRun();
    void onStop();

    // ---- 调试 ----
    void onDebugStart();
    void onDebugStop();
    void onDebugToggleBreakpoint();
    void onDebugStepOver();
    void onDebugStepInto();
    void onDebugStepOut();
    void onDebugContinue();

    // ---- 格式化 ----
    void onFormatCode();
    void onFormatStyleChanged();

    // ---- 视图 ----
    void onToggleFileTree();
    void onToggleOutputPanel();

    // ---- 评测 ----
    void onJudge();

    // ---- 设置 ----
    void onSetCompiler();
    void onSetTheme();

  private:
    void setupActions();
    void setupConnections();

    // 辅助：获取当前活动编辑器
    CodeEditor *currentEditor() const;

    // 辅助：将文本追加到输出面板指定标签页
    void appendOutput(const QString &text, const QString &tabName);

    // 更新窗口标题
    void updateWindowTitle();

    // ---------- 成员 ----------
    MainWindow *m_mainWindow = nullptr;

    // 模块实例
    CompilerManager *m_compilerManager = nullptr;
    DebuggerManager *m_debuggerManager = nullptr;

    // 编译输出文件路径（临时记录，供运行/调试使用）
    QString m_lastOutputPath;

    // -------- Actions --------
    // 文件
    QAction *m_actNew = nullptr;
    QAction *m_actOpen = nullptr;
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAs = nullptr;
    QAction *m_actClose = nullptr;
    QAction *m_actOpenFolder = nullptr;
    QAction *m_actExit = nullptr;

    // 编辑
    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;
    QAction *m_actCut = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actPaste = nullptr;
    QAction *m_actDelete = nullptr;
    QAction *m_actSelectAll = nullptr;

    // 编译
    QAction *m_actCompile = nullptr;
    QAction *m_actRun = nullptr;
    QAction *m_actCompileRun = nullptr;
    QAction *m_actStop = nullptr;

    // 调试
    QAction *m_actDebugStart = nullptr;
    QAction *m_actDebugStop = nullptr;
    QAction *m_actDebugBreakpoint = nullptr;
    QAction *m_actDebugStepOver = nullptr;
    QAction *m_actDebugStepInto = nullptr;
    QAction *m_actDebugStepOut = nullptr;
    QAction *m_actDebugContinue = nullptr;

    // 格式化
    QAction *m_actFormat = nullptr;
    QAction *m_actJudge = nullptr;

    // 视图
    QAction *m_actToggleFileTree = nullptr;
    QAction *m_actToggleOutputPanel = nullptr;

    // 设置
    QAction *m_actSetCompiler = nullptr;
    QMenu *m_menuStyle = nullptr;
    QMenu *m_menuTheme = nullptr;

    // 菜单
    QMenu *m_menuFile = nullptr;
    QMenu *m_menuEdit = nullptr;
    QMenu *m_menuBuild = nullptr;
    QMenu *m_menuDebug = nullptr;
    QMenu *m_menuView = nullptr;
    QMenu *m_menuTools = nullptr;
    QMenu *m_menuSettings = nullptr;
    QMenu *m_menuHelp = nullptr;
};