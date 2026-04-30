#include "CompilerManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

// 一些常量
static const QString PROCESS_NAME = QStringLiteral("CompilerManager");
static const QString ADDR_BROADCAST = QStringLiteral("*"); // 默认广播地址

CompilerManager::CompilerManager(QObject *parent) : QObject(parent) {}

CompilerManager::~CompilerManager() = default;

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