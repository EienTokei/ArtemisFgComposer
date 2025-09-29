#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "Config.h"
#include "FgComposer.h"

namespace fs = std::filesystem;

void PrintUsage(const char* programName);

int main(int argc, char* argv[]) {
    // �Ϸ�
    if (argc == 2 && fs::is_directory(argv[1])) {
        std::string inputDir = argv[1];

        Config config(inputDir);

        bool success = false;
        try {
            FgComposer composer(config);
            success = composer.process();
        }
        catch (const std::exception& e) {
            Logger::Error("�����쳣: " + std::string(e.what()));
            success = false;
        }
        std::cout << "\n�����������...";
        std::cin.get();
        return success ? 0 : 1;
    }

    // ���������в���
    Config config = Config::Parse(argc, argv);

    // ����
    if (config.helpRequested) {
        PrintUsage(argv[0]);
        std::cout << "\n�����������...";
        std::cin.get();
        return 0;
    }

    Logger::SetLevel(config.verbose ? Logger::Level::DEBUG : Logger::Level::INFO);

    // ��֤����
    if (!config.Validate()) {
        return 1;
    }

    try {
        FgComposer composer(config);
        bool success = composer.process();
        return success ? 0 : 1;
    }
    catch (const std::exception& e) {
        Logger::Error("�����쳣: " + std::string(e.what()));
        return 1;
    }
}

// ��ӡ����
void PrintUsage(const char* programName) {
    std::cout << "�÷�: " << programName << " [ѡ��] <����Ŀ¼>\n"
              << "ѡ��:\n"
              << "  --help, -h              ��ʾ������Ϣ\n"
              << "  --verbose, -v           �����ϸ��־\n"
              << "  --write-pos-back, -w    ��λ����Ϣд���ļ��Զ��κϳ�\n"
              << "  --lua-path, -l <·��>   ����������Ϣ��Lua�ű�·��\n"
              << "  --output, -o <·��>     ���Ŀ¼, Ĭ�ϱ���������Ŀ¼ͬ����output�ļ���\n"
              << "  <����Ŀ¼>              ����Ŀ¼\n"
              << std::endl;
    std::cout << "ʾ��: " << programName << " -v -w -l ./list_windows.tbl ./input" << std::endl;
    std::cout << "ʾ��: " << programName << " --output ./output ./input" << std::endl;
    std::cout << "ʾ��: " << programName << " ./input" << std::endl;
}