#include "CompilerManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

// 一些常量
static const QString PROCESS_NAME = QStringLiteral("CompilerManager");
static const QString ADDR_BROADCAST = QStringLiteral("*"); // 默认广播地址

CompilerManager::CompilerManager(QObject *parent) : QObject(parent) {}

CompilerManager::~CompilerManager()
{
    killCompileProcess();
    killRunProcess();
}

// ---------- 工具函数 ----------
QJsonObject CompilerManager::makeResponse(const QString &process,
                                          const QString &type,
                                          const QString &address,
                                          const QString &message)
{
    QJsonObject obj;
    obj["process"] = process;
    obj["type"] = type; // "success", "error", "info" ...
    obj["address"] = address;
    obj["message"] = message;
    return obj;
}

QString CompilerManager::configFilePath() const
{
    // 配置文件放在应用同级目录的 config 文件夹下
    const QString configDir =
        QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(configDir);
    return configDir + "/compiler.json";
}

// ---------- 配置持久化 ----------
void CompilerManager::loadConfig()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "[CompilerManager] config not found, using defaults.";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    m_compilerPath = obj.value("path").toString();
    m_compilerName = obj.value("name").toString();
    const QJsonArray arr = obj.value("defaultFlags").toArray();
    m_defaultFlags.clear();
    for (const auto &v : arr)
        m_defaultFlags.append(v.toString());
}

void CompilerManager::saveConfig() const
{
    QJsonObject obj;
    obj["path"] = m_compilerPath;
    obj["name"] = m_compilerName;
    QJsonArray arr;
    for (const QString &f : m_defaultFlags)
        arr.append(f);
    obj["defaultFlags"] = arr;

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

// ---------- 公开接口 ----------

void CompilerManager::setup()
{
    loadConfig();
    // 若路径为空，可以在此做一些默认搜索，但暂时留空
    if (m_compilerPath.isEmpty())
    {
        qDebug() << "[CompilerManager] No compiler configured. Use "
                    "setcompile() to set one.";
    }
    else
    {
        qDebug() << "[CompilerManager] Loaded compiler:" << m_compilerName
                 << "at" << m_compilerPath;
    }
}

QJsonObject CompilerManager::setcompile(const QString &compilerPath,
                                        const QString &compilerName,
                                        const QStringList &defaultFlags)
{
    m_compilerPath = compilerPath;
    m_compilerName = compilerName.isEmpty() ? "Custom Compiler" : compilerName;
    m_defaultFlags = defaultFlags;

    saveConfig();

    return makeResponse(
        PROCESS_NAME, "success", ADDR_BROADCAST,
        QString("Compiler set to %1 (%2)").arg(m_compilerName, m_compilerPath));
}

QJsonObject CompilerManager::compiling(const QString &sourceFile,
                                       const QString &outputFile,
                                       const QStringList &extraFlags) const
{
    if (m_compilerPath.isEmpty())
    {
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST,
                            "No compiler configured. Use setcompile first.");
    }

    // 确定输出文件名
    QString out = outputFile;
    if (out.isEmpty())
    {
        QFileInfo fi(sourceFile);
        out = fi.absolutePath() + "/" + fi.completeBaseName(); // 不带扩展名
#ifdef Q_OS_WIN
        out += ".exe";
#endif
    }

    // 拼接参数
    QStringList arguments;
    arguments << sourceFile;
    arguments << "-o" << out;
    arguments << m_defaultFlags;
    arguments << extraFlags;

    QProcess process;
    process.start(m_compilerPath, arguments);
    process.waitForFinished(-1);

    QString stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString stdErr = QString::fromLocal8Bit(process.readAllStandardError());
    int exitCode = process.exitCode();

    QString fullMessage;
    if (exitCode == 0)
    {
        fullMessage = QString("Compilation succeeded.\nOutput: %1").arg(out);
        if (!stdOut.isEmpty())
            fullMessage += "\n[stdout]\n" + stdOut;
        return makeResponse(PROCESS_NAME, "success", ADDR_BROADCAST,
                            fullMessage);
    }
    else
    {
        fullMessage =
            QString("Compilation failed with exit code %1.\n").arg(exitCode);
        if (!stdErr.isEmpty())
            fullMessage += "[stderr]\n" + stdErr;
        if (!stdOut.isEmpty())
            fullMessage += "[stdout]\n" + stdOut;
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST, fullMessage);
    }
}

QJsonObject CompilerManager::running(const QString &executablePath,
                                     const QStringList &args, bool pause) const
{
    QFileInfo fi(executablePath);
    if (!fi.exists() || !fi.isExecutable())
    {
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST,
                            "Executable not found or not runnable: " +
                                executablePath);
    }

    QProcess process;
    QString program;
    QStringList fullArgs;

    if (pause)
    {
        // 通过cmd运行并暂停，确保窗口保持
        program = "cmd";
        // 构建命令: /c ""executable" args... & pause"
        QString cmdLine =
            QString("\"\"%1\" %2\" & pause")
                .arg(QDir::toNativeSeparators(executablePath), args.join(' '));
        fullArgs << "/c" << cmdLine;
    }
    else
    {
        program = executablePath;
        fullArgs = args;
    }

    process.start(program, fullArgs);
    process.waitForFinished(-1);

    QString stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString stdErr = QString::fromLocal8Bit(process.readAllStandardError());
    int exitCode = process.exitCode();

    QString message =
        QString("Program finished with exit code %1.\n").arg(exitCode);
    if (!stdOut.isEmpty())
        message += "[stdout]\n" + stdOut;
    if (!stdErr.isEmpty())
        message += "[stderr]\n" + stdErr;

    return makeResponse(PROCESS_NAME, exitCode == 0 ? "success" : "error",
                        ADDR_BROADCAST, message);
}

QJsonObject CompilerManager::manage()
{
    // TODO: 待后续实现（如多编译器切换、高级设置）
    return makeResponse(PROCESS_NAME, "info", ADDR_BROADCAST,
                        "manage() not implemented yet.");
}

// ========== 异步 API 实现 ==========

QJsonObject CompilerManager::compileAsync(const QString &sourceFile,
                                          const QString &outputFile,
                                          const QStringList &extraFlags)
{
    if (m_compilerPath.isEmpty())
    {
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST,
                            "No compiler configured. Use setcompile first.");
    }

    // 若已有编译在跑，先停止
    killCompileProcess();

    // 确定输出文件名
    QString out = outputFile;
    if (out.isEmpty())
    {
        QFileInfo fi(sourceFile);
        out = fi.absolutePath() + "/" + fi.completeBaseName();
#ifdef Q_OS_WIN
        out += ".exe";
#endif
    }
    m_lastOutputPath = out;

    // 拼接参数
    QStringList arguments;
    arguments << sourceFile;
    arguments << "-o" << out;
    arguments << m_defaultFlags;
    arguments << extraFlags;

    m_compileProcess = new QProcess(this);

    connect(m_compileProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &CompilerManager::onCompileFinished);
    connect(m_compileProcess, &QProcess::readyReadStandardOutput, this,
            &CompilerManager::onCompileReadyRead);
    connect(m_compileProcess, &QProcess::readyReadStandardError, this,
            &CompilerManager::onCompileReadyRead);

    m_compileProcess->start(m_compilerPath, arguments);
    emit compilationStarted(sourceFile);

    return makeResponse(PROCESS_NAME, "success", ADDR_BROADCAST,
                        "Compilation started asynchronously.");
}

QJsonObject CompilerManager::runAsync(const QString &executablePath,
                                      const QStringList &args)
{
    QFileInfo fi(executablePath);
    if (!fi.exists() || !fi.isExecutable())
    {
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST,
                            "Executable not found: " + executablePath);
    }

    killRunProcess();

    m_runProcess = new QProcess(this);
    m_runProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_runProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &CompilerManager::onRunFinished);
    connect(m_runProcess, &QProcess::readyRead, this,
            &CompilerManager::onRunReadyRead);

    m_runProcess->setProgram(executablePath);
    m_runProcess->setArguments(args);
    m_runProcess->start();

    emit programStarted(executablePath);

    return makeResponse(PROCESS_NAME, "success", ADDR_BROADCAST,
                        "Program started asynchronously.");
}

QJsonObject CompilerManager::compileAndRun(const QString &sourceFile,
                                           const QStringList &extraFlags)
{
    if (m_compilerPath.isEmpty())
    {
        return makeResponse(PROCESS_NAME, "error", ADDR_BROADCAST,
                            "No compiler configured.");
    }

    // 先异步编译，编译成功后在信号槽中自动运行
    // 保存待运行的 exe 信息（compileAsync 会设置 m_lastOutputPath）
    QJsonObject result = compileAsync(sourceFile, QString(), extraFlags);
    m_pendingRunExe = m_lastOutputPath;
    return result;
}

QJsonObject CompilerManager::stop()
{
    killCompileProcess();
    killRunProcess();
    m_pendingRunExe.clear();
    return makeResponse(PROCESS_NAME, "success", ADDR_BROADCAST,
                        "All processes stopped.");
}

QString CompilerManager::lastOutputPath() const { return m_lastOutputPath; }

// ────────── 私有槽 ──────────

void CompilerManager::onCompileFinished(int exitCode,
                                        QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    QString stdOut, stdErr;
    if (m_compileProcess)
    {
        stdOut =
            QString::fromLocal8Bit(m_compileProcess->readAllStandardOutput());
        stdErr =
            QString::fromLocal8Bit(m_compileProcess->readAllStandardError());
    }

    QString error;
    QString outputPath = m_lastOutputPath;
    bool success = (exitCode == 0);

    if (!success)
    {
        error = stdErr;
        if (error.isEmpty())
            error = stdOut;
        if (error.isEmpty())
            error =
                QString("Compilation failed with exit code %1").arg(exitCode);
    }

    // 若 compileAndRun 触发且编译成功 → 自动运行
    if (success && !m_pendingRunExe.isEmpty())
    {
        QString exe = m_pendingRunExe;
        m_pendingRunExe.clear();
        emit compilationFinished(true, outputPath, QString());
        runAsync(exe);
    }
    else
    {
        m_pendingRunExe.clear();
        emit compilationFinished(success, outputPath, error);
    }

    killCompileProcess();
}

void CompilerManager::onCompileReadyRead()
{
    if (!m_compileProcess)
        return;
    QString out =
        QString::fromLocal8Bit(m_compileProcess->readAllStandardOutput());
    QString err =
        QString::fromLocal8Bit(m_compileProcess->readAllStandardError());

    QStringList lines;
    if (!out.isEmpty())
        lines.append(out.split('\n', Qt::SkipEmptyParts));
    if (!err.isEmpty())
        lines.append(err.split('\n', Qt::SkipEmptyParts));

    for (const QString &line : lines)
        emit compilationOutput(line);
}

void CompilerManager::onRunFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    // 读取剩余输出
    if (m_runProcess)
    {
        QString remaining = QString::fromLocal8Bit(m_runProcess->readAll());
        if (!remaining.isEmpty())
            emit programOutput(remaining);
    }
    emit programFinished(exitCode);
    killRunProcess();
}

void CompilerManager::onRunReadyRead()
{
    if (!m_runProcess)
        return;
    QString data = QString::fromLocal8Bit(m_runProcess->readAll());
    if (!data.isEmpty())
        emit programOutput(data);
}

void CompilerManager::killCompileProcess()
{
    if (m_compileProcess)
    {
        if (m_compileProcess->state() != QProcess::NotRunning)
        {
            m_compileProcess->kill();
            m_compileProcess->waitForFinished(2000);
        }
        m_compileProcess->deleteLater();
        m_compileProcess = nullptr;
    }
}

void CompilerManager::killRunProcess()
{
    if (m_runProcess)
    {
        if (m_runProcess->state() != QProcess::NotRunning)
        {
            m_runProcess->kill();
            m_runProcess->waitForFinished(2000);
        }
        m_runProcess->deleteLater();
        m_runProcess = nullptr;
    }
}
