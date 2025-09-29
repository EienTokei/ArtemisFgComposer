#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "Config.h"
#include "FgComposer.h"

namespace fs = std::filesystem;

void PrintUsage(const char* programName);

int main(int argc, char* argv[]) {
    // 拖放
    if (argc == 2 && fs::is_directory(argv[1])) {
        std::string inputDir = argv[1];

        Config config(inputDir);

        bool success = false;
        try {
            FgComposer composer(config);
            success = composer.process();
        }
        catch (const std::exception& e) {
            Logger::Error("发生异常: " + std::string(e.what()));
            success = false;
        }
        std::cout << "\n按任意键继续...";
        std::cin.get();
        return success ? 0 : 1;
    }

    // 解析命令行参数
    Config config = Config::Parse(argc, argv);

    // 帮助
    if (config.helpRequested) {
        PrintUsage(argv[0]);
        std::cout << "\n按任意键继续...";
        std::cin.get();
        return 0;
    }

    Logger::SetLevel(config.verbose ? Logger::Level::DEBUG : Logger::Level::INFO);

    // 验证输入
    if (!config.Validate()) {
        return 1;
    }

    try {
        FgComposer composer(config);
        bool success = composer.process();
        return success ? 0 : 1;
    }
    catch (const std::exception& e) {
        Logger::Error("发生异常: " + std::string(e.what()));
        return 1;
    }
}

// 打印帮助
void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项] <输入目录>\n"
              << "选项:\n"
              << "  --help, -h              显示帮助信息\n"
              << "  --verbose, -v           输出详细日志\n"
              << "  --write-pos-back, -w    将位置信息写回文件以二次合成\n"
              << "  --lua-path, -l <路径>   含有坐标信息的Lua脚本路径\n"
              << "  --output, -o <路径>     输出目录, 默认保存在输入目录同级的output文件夹\n"
              << "  <输入目录>              输入目录\n"
              << std::endl;
    std::cout << "示例: " << programName << " -v -w -l ./list_windows.tbl ./input" << std::endl;
    std::cout << "示例: " << programName << " --output ./output ./input" << std::endl;
    std::cout << "示例: " << programName << " ./input" << std::endl;
}