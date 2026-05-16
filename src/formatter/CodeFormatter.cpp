#include "CodeFormatter.h"

#include <QFile>
#include <QJsonObject>
#include <QRegularExpression>

CodeFormatter::CodeFormatter(QObject *parent)
    : QObject(parent), m_style(FormatStyle::Allman)
{
    // 尝试自动查找 clang-format
    m_clangFormatPath = findClangFormat();
}

CodeFormatter::~CodeFormatter() = default;

void CodeFormatter::setClangFormatPath(const QString &path)
{
    m_clangFormatPath = path;
}

FormatStyle CodeFormatter::formatStyle() const { return m_style; }

QStringList CodeFormatter::availableStyles()
{
    return {"Allman",   "Java",    "Stroustrup", "Whitesmith", "Google",
            "Chromium", "Mozilla", "WebKit",     "Microsoft"};
}

QString CodeFormatter::currentStyleName() const
{
    return styleToString(m_style);
}

void CodeFormatter::setFormatStyle(FormatStyle style) { m_style = style; }

void CodeFormatter::setFormatStyleByName(const QString &styleName)
{
    static const QHash<QString, FormatStyle> map = {
        {"Allman", FormatStyle::Allman},
        {"Java", FormatStyle::Java},
        {"Stroustrup", FormatStyle::Stroustrup},
        {"Whitesmith", FormatStyle::Whitesmith},
        {"Google", FormatStyle::Google},
        {"Chromium", FormatStyle::Chromium},
        {"Mozilla", FormatStyle::Mozilla},
        {"WebKit", FormatStyle::WebKit},
        {"Microsoft", FormatStyle::Microsoft}};

    if (map.contains(styleName))
        m_style = map[styleName];
}

QJsonObject CodeFormatter::formatCode(const QString &code)
{
    QJsonObject result;

    if (!isClangFormatAvailable())
    {
        result["success"] = false;
        result["error"] = "clang-format 不可用，请安装 LLVM/Clang 工具链";
        return result;
    }

    // 使用 QProcess 同步调用 clang-format
    QProcess process;
    process.setProgram(m_clangFormatPath.isEmpty() ? "clang-format"
                                                   : m_clangFormatPath);
    process.setArguments({"--style=" + styleToString(m_style).toLower()});

    process.start();
    if (!process.waitForStarted(3000))
    {
        result["success"] = false;
        result["error"] = "无法启动 clang-format 进程";
        return result;
    }

    // 写入原始代码
    process.write(code.toUtf8());
    process.closeWriteChannel();

    if (!process.waitForFinished(10000))
    {
        process.kill();
        result["success"] = false;
        result["error"] = "clang-format 超时";
        return result;
    }

    if (process.exitCode() != 0)
    {
        QString err = QString::fromUtf8(process.readAllStandardError());
        result["success"] = false;
        result["error"] = "clang-format 错误: " + err;
        return result;
    }

    QString formatted = QString::fromUtf8(process.readAllStandardOutput());

    result["success"] = true;
    result["formatted"] = formatted;
    emit formattingCompleted(QString(), formatted);
    return result;
}

QJsonObject CodeFormatter::formatFile(const QString &filePath)
{
    QJsonObject result;

    if (!isClangFormatAvailable())
    {
        result["success"] = false;
        result["error"] = "clang-format 不可用";
        return result;
    }

    // 直接调用 clang-format -i 原地格式化
    QProcess process;
    process.setProgram(m_clangFormatPath.isEmpty() ? "clang-format"
                                                   : m_clangFormatPath);
    process.setArguments(
        {"-i", "--style=" + styleToString(m_style).toLower(), filePath});

    process.start();
    if (!process.waitForStarted(3000))
    {
        result["success"] = false;
        result["error"] = "无法启动 clang-format 进行文件格式化";
        return result;
    }

    if (!process.waitForFinished(10000))
    {
        process.kill();
        result["success"] = false;
        result["error"] = "clang-format 文件格式化超时";
        return result;
    }

    if (process.exitCode() != 0)
    {
        QString err = QString::fromUtf8(process.readAllStandardError());
        result["success"] = false;
        result["error"] = "clang-format 错误: " + err;
        return result;
    }

    // 重新读取格式化后的内容
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString formatted = QString::fromUtf8(file.readAll());
        file.close();
        result["success"] = true;
        result["formatted"] = formatted;
        emit formattingCompleted(filePath, formatted);
    }
    else
    {
        result["success"] = true;
        result["formatted"] = QString();
        emit formattingCompleted(filePath, QString());
    }

    return result;
}

QString CodeFormatter::styleToString(FormatStyle style) const
{
    switch (style)
    {
    case FormatStyle::Allman:
        return "Allman";
    case FormatStyle::Java:
        return "Java";
    case FormatStyle::Stroustrup:
        return "Stroustrup";
    case FormatStyle::Whitesmith:
        return "Whitesmith";
    case FormatStyle::Google:
        return "Google";
    case FormatStyle::Chromium:
        return "Chromium";
    case FormatStyle::Mozilla:
        return "Mozilla";
    case FormatStyle::WebKit:
        return "WebKit";
    case FormatStyle::Microsoft:
        return "Microsoft";
    default:
        return "Allman";
    }
}

bool CodeFormatter::isClangFormatAvailable() const
{
    QString path =
        m_clangFormatPath.isEmpty() ? "clang-format" : m_clangFormatPath;

    QProcess proc;
    proc.setProgram(path);
    proc.setArguments({"--version"});
    proc.start();

    if (!proc.waitForStarted(3000))
        return false;

    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

QString CodeFormatter::findClangFormat() const
{
    // 尝试常见路径
    QStringList candidates = {
        "clang-format",
#ifdef Q_OS_WIN
        "C:/Program Files/LLVM/bin/clang-format.exe",
        "C:/Program Files (x86)/LLVM/bin/clang-format.exe",
#endif
    };

    for (const auto &c : candidates)
    {
        QProcess proc;
        proc.setProgram(c);
        proc.setArguments({"--version"});
        proc.start();
        if (proc.waitForStarted(2000))
        {
            proc.waitForFinished(2000);
            if (proc.exitCode() == 0)
                return c;
        }
    }

    return QString();
}