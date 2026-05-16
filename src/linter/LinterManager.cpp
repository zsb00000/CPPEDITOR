#include "LinterManager.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryFile>

LinterManager::LinterManager(QObject *parent) : QObject(parent) {}

LinterManager::~LinterManager() = default;

void LinterManager::setCompilerPath(const QString &compilerPath)
{
    m_compilerPath = compilerPath;
}

void LinterManager::setEnabled(bool enabled) { m_enabled = enabled; }

bool LinterManager::isEnabled() const { return m_enabled; }

QJsonArray LinterManager::lintFile(const QString &filePath, const QString &code)
{
    Q_UNUSED(code)
    if (!m_enabled)
        return {};

    QJsonArray results;

    // 优先使用 clang-tidy
    if (isClangTidyAvailable())
    {
        results = lintWithClangTidy(filePath);
        if (!results.isEmpty())
        {
            emit diagnosticsReady(filePath, results);
            return results;
        }
    }

    // Fallback 1: 使用 clang++ -fsyntax-only（如路径中包含 clang）
    if (!m_compilerPath.isEmpty() && m_compilerPath.contains("clang"))
    {
        results = lintWithClangSyntax(filePath, m_compilerPath);
    }
    else
    {
        // Fallback 2: 使用 g++ -fsyntax-only
        results = lintWithCompiler(filePath);
    }

    emit diagnosticsReady(filePath, results);
    return results;
}

QJsonArray LinterManager::lintWithClangTidy(const QString &filePath)
{
    QJsonArray arr;
    QProcess proc;
    proc.setProgram("clang-tidy");
    proc.setArguments({filePath, "--", "-std=c++20"});

    proc.start();
    if (!proc.waitForStarted(5000))
        return arr;

    if (!proc.waitForFinished(15000))
    {
        proc.kill();
        return arr;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // clang-tidy 输出格式:
    // /path/file.cpp:line:col: severity: message [check-name]
    QRegularExpression re(
        R"(^(.+?):(\d+):(\d+):\s*(error|warning|note):\s*(.+?)\s*\[(.+?)\]$)");
    for (const auto &line : lines)
    {
        auto match = re.match(line.trimmed());
        if (match.hasMatch())
        {
            QJsonObject diag;
            diag["filePath"] = match.captured(1);
            diag["line"] = match.captured(2).toInt();
            diag["column"] = match.captured(3).toInt();
            diag["severity"] = match.captured(4);
            diag["message"] = match.captured(5);
            diag["code"] = match.captured(6);
            arr.append(diag);
        }
    }

    return arr;
}

QJsonArray LinterManager::lintWithCompiler(const QString &filePath)
{
    // 使用 g++ -fsyntax-only
    QString compiler = m_compilerPath.isEmpty() ? "g++" : m_compilerPath;
    return lintWithClangSyntax(filePath, compiler);
}

QJsonArray LinterManager::lintWithClangSyntax(const QString &filePath,
                                              const QString &compiler)
{
    QJsonArray arr;
    QProcess proc;
    proc.setProgram(compiler);
    proc.setArguments(
        {"-fsyntax-only", "-std=c++20", "-Wall", "-Wextra", filePath});

    proc.start();
    if (!proc.waitForStarted(5000))
        return arr;

    if (!proc.waitForFinished(15000))
    {
        proc.kill();
        return arr;
    }

    // g++/clang++ 错误输出在 stderr
    QString output = QString::fromUtf8(proc.readAllStandardError());
    if (output.isEmpty())
        output = QString::fromUtf8(proc.readAllStandardOutput());

    QList<LintDiagnostic> diags = parseCompilerOutput(output);
    for (const auto &d : diags)
    {
        QJsonObject obj;
        obj["filePath"] = d.filePath;
        obj["line"] = d.line;
        obj["column"] = d.column;
        obj["severity"] = d.severity;
        obj["code"] = d.code;
        obj["message"] = d.message;
        arr.append(obj);
    }

    return arr;
}

QList<LintDiagnostic>
LinterManager::parseCompilerOutput(const QString &output) const
{
    QList<LintDiagnostic> results;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // g++/clang++ 格式:
    // file:line:col: error: message
    // file:line:col: warning: message [-Wflag]
    QRegularExpression re(
        R"(^(.+?):(\d+):(\d+):\s*(error|warning|note|fatal error):\s*(.+))");

    for (const auto &line : lines)
    {
        auto match = re.match(line.trimmed());
        if (match.hasMatch())
        {
            LintDiagnostic d;
            d.filePath = match.captured(1);
            d.line = match.captured(2).toInt();
            d.column = match.captured(3).toInt();
            d.severity = match.captured(4);
            d.message = match.captured(5).trimmed();

            // 提取 [check-name]
            QRegularExpression codeRe(R"(\[([^\]]+)\])");
            auto codeMatch = codeRe.match(d.message);
            if (codeMatch.hasMatch())
            {
                d.code = codeMatch.captured(1);
                d.message = d.message.left(codeMatch.capturedStart()).trimmed();
            }

            results.append(d);
        }
    }

    return results;
}

bool LinterManager::isClangTidyAvailable() const
{
    QProcess proc;
    proc.setProgram("clang-tidy");
    proc.setArguments({"--version"});
    proc.start();
    if (!proc.waitForStarted(3000))
        return false;
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}