#pragma once

#include <QAction>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

// ==================== 数据结构 ====================
struct Breakpoint
{
    int number = -1;
    QString type; // breakpoint, watchpoint
    QString enabled;
    QString address;
    QString func;
    QString file;
    int line = 0;
};

struct Variable
{
    QString name;
    QString value;
    QString type;
};

struct StackFrame
{
    int level = 0;
    QString address;
    QString func;
    QString file;
    int line = 0;
};

// ==================== 调试图形界面 ====================
class DebuggerConsole : public QWidget
{
    Q_OBJECT
  public:
    explicit DebuggerConsole(QWidget *parent = nullptr);

    void appendConsoleOutput(const QString &text);
    void clearConsole();

    // 更新图形组件
    void updateBreakpoints(const QList<Breakpoint> &bps);
    void updateLocals(const QList<Variable> &vars);
    void updateStack(const QList<StackFrame> &frames);
    void setProgramStatus(bool running); // true: 运行中, false: 已停止
    void setCurrentLocation(const QString &file, int line); // 可选

  signals:
    void sendCommand(const QString &command); // 依旧保留
    // 图形操作衍生的命令
    void continueRequested();
    void pauseRequested();
    void stepOverRequested();
    void stepIntoRequested();
    void stepOutRequested();
    void restartRequested();
    void addBreakpointRequested(
        const QString &location); // e.g. "main" or "file.cpp:10"

  private slots:
    void onContinue();
    void onPause();
    void onStepOver();
    void onStepInto();
    void onStepOut();
    void onRestart();
    void onAddBreakpoint();

  private:
    void setupUi();

    // 图形区域
    QToolBar *m_toolbar = nullptr;
    QTreeWidget *m_breakpointsView = nullptr;
    QTreeWidget *m_localsView = nullptr;
    QTreeWidget *m_stackView = nullptr;
    QLabel *m_statusLabel = nullptr;

    // 控制台区域（保留）
    QPlainTextEdit *m_consoleOutput = nullptr;
    QLineEdit *m_commandInput = nullptr;
};

// ==================== 调试器管理器 ====================
class DebuggerManager : public QObject
{
    Q_OBJECT
  public:
    explicit DebuggerManager(QObject *parent = nullptr);
    ~DebuggerManager();

    void initial(const QString &gdbPath);
    void setup();

    // 高层调试 API（供 ActionManager 调用）
    QJsonObject startDebug(const QString &executablePath);
    QJsonObject stopDebug();
    QJsonObject toggleBreakpoint(const QString &filePath, int line);
    QJsonObject stepOver();
    QJsonObject stepInto();
    QJsonObject stepOut();
    QJsonObject continueExec();

  public slots:
    void trans(const QString &command);

  private slots:
    void recv();
    void onGdbError();
    void onGdbFinished(int exitCode, QProcess::ExitStatus status);

    // 图形按钮对应的动作
    void handleContinue();
    void handlePause();
    void handleStepOver();
    void handleStepInto();
    void handleStepOut();
    void handleRestart();
    void handleAddBreakpoint(const QString &location);

  private:
    void startGdb();
    void startUi();
    void parseGdbOutput(const QString &line);
    void emitStopped(const QString &reason, const QString &details);
    // 发送 GDB 命令并回显到控制台
    void sendToGdb(const QString &cmd);

  signals:
    void requestCompileWithDebug();

    // 图形界面更新信号
    void breakpointsUpdated(const QList<Breakpoint> &bps);
    void localsUpdated(const QList<Variable> &vars);
    void stackUpdated(const QList<StackFrame> &frames);
    void programStopped(const QString &reason);
    void programRunning();
    void consoleOutput(const QString &text);

  private:
    QProcess *m_gdbProcess = nullptr;
    QString m_gdbPath;
    DebuggerConsole *m_ui = nullptr;

    // MI 解析状态缓存
    QList<Breakpoint> m_breakpoints;
    QList<Variable> m_currentLocals;
    QList<StackFrame> m_currentStack;
    QString m_currentExecutable;
    bool m_isRunning = false;
    // 用于记录异步命令的token（简化起见，可不用）
};