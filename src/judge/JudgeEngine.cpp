#include "JudgeEngine.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryFile>

JudgeEngine::JudgeEngine(QObject *parent) : QObject(parent) {}

JudgeEngine::~JudgeEngine() = default;

void JudgeEngine::setCompilerPath(const QString &path)
{
    m_compilerPath = path;
}

void JudgeEngine::setCompileArgs(const QStringList &args)
{
    m_compileArgs = args;
}

void JudgeEngine::setTimeLimit(double seconds) { m_timeLimit = seconds; }

QJsonObject JudgeEngine::judge(const QString &sourcePath,
                               const QJsonArray &testCases)
{
    QJsonObject result;
    QList<TestCase> cases = parseTestCases(testCases);

    if (cases.isEmpty())
    {
        result["success"] = false;
        result["error"] = "没有有效的测试用例";
        return result;
    }

    // 1. 编译
    QJsonObject compileResult = compile(sourcePath);
    bool compileOk = compileResult["success"].toBool();
    emit compilationFinished(compileOk,
                             compileResult["executablePath"].toString());

    if (!compileOk)
    {
        result["success"] = false;
        result["compileError"] = compileResult["error"].toString();

        // 所有用例标记为编译错误
        QJsonArray arr;
        for (const auto &tc : cases)
        {
            JudgeResult jr;
            jr.caseId = tc.id;
            jr.status = "CompilationError";
            jr.error = compileResult["error"].toString();
            QJsonObject jrObj;
            jrObj["caseId"] = jr.caseId;
            jrObj["status"] = jr.status;
            jrObj["error"] = jr.error;
            arr.append(jrObj);
            emit caseFinished(jr);
        }
        result["results"] = arr;
        return result;
    }

    QString exePath = compileResult["executablePath"].toString();

    // 2. 逐一运行测试用例
    QJsonArray resultsArr;
    QList<JudgeResult> allResults;

    for (const auto &tc : cases)
    {
        JudgeResult jr = runTestCase(exePath, tc);
        allResults.append(jr);

        QJsonObject jrObj;
        jrObj["caseId"] = jr.caseId;
        jrObj["status"] = jr.status;
        jrObj["actualOutput"] = jr.actualOutput;
        jrObj["expectedOutput"] = jr.expectedOutput;
        jrObj["error"] = jr.error;
        jrObj["time"] = jr.time;
        jrObj["exitCode"] = jr.exitCode;
        resultsArr.append(jrObj);

        emit caseFinished(jr);
    }

    // 清理临时可执行文件
    QFile::remove(exePath);

    emit judgingFinished(allResults);

    result["success"] = true;
    result["results"] = resultsArr;
    return result;
}

QJsonObject JudgeEngine::compile(const QString &sourcePath)
{
    QJsonObject result;
    QString compiler = m_compilerPath.isEmpty() ? "g++" : m_compilerPath;

    // 生成临时输出文件路径
    QString outputPath = QFileInfo(sourcePath).absolutePath() + "/" +
                         QFileInfo(sourcePath).baseName() + "_judge_" +
                         QString::number(QDateTime::currentMSecsSinceEpoch()) +
#ifdef Q_OS_WIN
                         ".exe";
#else
                         ".out";
#endif

    QProcess proc;
    proc.setProgram(compiler);

    QStringList args;
    args << "-std=c++20" << "-O2" << "-Wall";
    args << m_compileArgs;
    args << "-o" << outputPath << sourcePath;

    proc.setArguments(args);
    proc.start();

    if (!proc.waitForStarted(10000))
    {
        result["success"] = false;
        result["error"] = "无法启动编译器";
        return result;
    }

    if (!proc.waitForFinished(30000))
    {
        proc.kill();
        result["success"] = false;
        result["error"] = "编译超时";
        return result;
    }

    if (proc.exitCode() != 0)
    {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        result["success"] = false;
        result["error"] = "编译失败:\n" + err;
        return result;
    }

    result["success"] = true;
    result["executablePath"] = outputPath;
    return result;
}

JudgeResult JudgeEngine::runTestCase(const QString &executablePath,
                                     const TestCase &testCase)
{
    JudgeResult result;
    result.caseId = testCase.id;
    result.expectedOutput = testCase.expectedOutput;

    QProcess proc;
    proc.setProgram(executablePath);

    proc.start();
    if (!proc.waitForStarted(5000))
    {
        result.status = "RuntimeError";
        result.error = "无法启动可执行文件";
        return result;
    }

    // 写入输入
    if (!testCase.input.isEmpty())
    {
        proc.write(testCase.input.toUtf8());
        proc.closeWriteChannel();
    }

    QElapsedTimer timer;
    timer.start();

    double limit = testCase.timeLimit > 0 ? testCase.timeLimit : m_timeLimit;
    int timeoutMs = static_cast<int>(limit * 1000);

    bool finished = proc.waitForFinished(timeoutMs);

    result.time = timer.elapsed() / 1000.0;

    if (!finished)
    {
        proc.kill();
        proc.waitForFinished(2000);
        result.status = "TimeLimitExceeded";
        result.error = QString("超时 > %1s").arg(limit);
        result.exitCode = -1;
        return result;
    }

    result.exitCode = proc.exitCode();
    result.actualOutput = QString::fromUtf8(proc.readAllStandardOutput());

    QString stderrOutput = QString::fromUtf8(proc.readAllStandardError());
    if (!stderrOutput.isEmpty())
    {
        result.error = stderrOutput;
    }

    // 判定
    if (result.exitCode != 0)
    {
        result.status = "RuntimeError";
    }
    else if (normalizeOutput(result.actualOutput) ==
             normalizeOutput(result.expectedOutput))
    {
        result.status = "Accepted";
    }
    else
    {
        result.status = "Wrong Answer";
    }

    return result;
}

QList<TestCase> JudgeEngine::parseTestCases(const QJsonArray &jsonArray) const
{
    QList<TestCase> cases;
    for (const auto &val : jsonArray)
    {
        QJsonObject obj = val.toObject();
        TestCase tc;
        tc.id = obj["id"].toString();
        tc.input = obj["input"].toString();
        tc.expectedOutput = obj["expectedOutput"].toString();
        tc.timeLimit =
            obj["timeLimit"].toDouble(tc.timeLimit > 0 ? tc.timeLimit : 1.0);
        cases.append(tc);
    }
    return cases;
}

QString JudgeEngine::normalizeOutput(const QString &output) const
{
    QString normalized = output;
    // 移除行尾空白
    QStringList lines = normalized.split('\n');
    for (auto &line : lines)
        line = line.trimmed();
    normalized = lines.join('\n');
    // 移除尾部空行
    while (normalized.endsWith('\n'))
        normalized.chop(1);
    normalized = normalized.trimmed();
    return normalized;
}