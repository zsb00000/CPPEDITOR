#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QTextStream>

#include "core/MainWindow.h"
#include "ui/QssManager.h"

static void ensureDirectories()
{
    // 确保 DATA 目录结构存在
    const QString dataDir = QCoreApplication::applicationDirPath() + "/DATA";
    QDir dir;
    dir.mkpath(dataDir + "/config");
    dir.mkpath(dataDir + "/themes");
}

static void registerFonts()
{
    // 注册等宽字体（可选：Cascadia Code, Fira Code 等）
    QFontDatabase fontDb;
    // 如果系统有等宽字体，优先使用
    if (fontDb.families().contains("Consolas"))
    {
        QFont mono("Consolas", 12);
        QApplication::setFont(mono);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CPPEditor");
    app.setOrganizationName("CPPEditor-Team");
    app.setApplicationVersion("0.1.0");

    ensureDirectories();
    registerFonts();

    // 加载默认 QSS 主题
    QssManager::instance().loadTheme("default");

    MainWindow window;
    window.show();

    return app.exec();
}