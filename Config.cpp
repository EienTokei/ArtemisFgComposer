#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include "Config.h"

// Config 的方法实现
Config::Config(const std::string& inDir, const std::string& outDir, const std::string& luaFilePath)
    : inputDir(inDir), outputDir(outDir), luaPath(luaFilePath) {
    InitializeDefaultValues();
    InitializeDefaultRules();
}
Config::Config(int argc, char* argv[]) {
    *this = Parse(argc, argv);
}

void Config::InitializeDefaultValues() {
    if (inputDir.empty()) return;

    if (outputDir.empty()) {
        outputDir = inputDir + "_output";
    }

    std::filesystem::path inPath(inputDir);
    std::string typeName = inPath.stem().string();
    std::string charName = inPath.parent_path().stem().string();
    globalName = charName + "_" + typeName;
}

void Config::InitializeDefaultRules() {
    groupRule = R"(([a-z]\d{4}))";

    partRules = {
        PartRule("base", R"(^[a-z]{3}_[a-z0-9]{2}[a-z]\d{4})"),
        PartRule("face", R"(^[a-z]\d{2}[0-8]\d)"),
        PartRule("other", R"(^[a-z]\d{2}9\d)"),
    };
}

Config Config::Parse(int argc, char* argv[]) {
    Config config;

    if (argc < 2) {
        config.helpRequested = true;
        return config;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            config.helpRequested = true;
            return config;
        }
        else if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        }
        else if (arg == "--write-pos-back" || arg == "-w") {
            config.writePosBack = true;
        }
        else if (arg == "--lua-path" || arg == "-l") {
            if (i + 1 >= argc) {
                Logger::Error("--lua-path 选项需要指定参数值");
                config.helpRequested = true;
                return config;
            }
            config.luaPath = argv[++i];
        }
        else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) {
                Logger::Error("--output 选项需要指定参数值");
                config.helpRequested = true;
                return config;
            }
            config.outputDir = argv[++i];
        }
        else if (arg[0] == '-') {
            Logger::Error("未知选项: " + arg);
            config.helpRequested = true;
            return config;
        }
        else {
            if (!config.inputDir.empty()) {
                Logger::Error("只能指定一个输入目录");
                config.helpRequested = true;
                return config;
            }
            config.inputDir = arg;
        }
    }

    // 初始化默认值和规则
    if (!config.helpRequested) {
        config.InitializeDefaultValues();
        config.InitializeDefaultRules();
    }

    return config;
}

bool Config::Validate() const {
    if (helpRequested) {
        return true;
    }
    if (inputDir.empty()) {
        Logger::Error("必须指定输入目录");
        return false;
    }
    if (!inputDir.empty() && !std::filesystem::exists(inputDir)) {
        Logger::Error("输入目录不存在: " + inputDir);
        return false;
    }
    if (!luaPath.empty() && !std::filesystem::exists(luaPath)) {
        Logger::Error("Lua脚本路径不存在: " + luaPath);
        return false;
    }

    // 验证规则
    if (groupRule.empty()) {
        Logger::Error("分组规则不能为空");
        return false;
    }
    if (partRules.empty()) {
        Logger::Error("部件规则不能为空");
        return false;
    }

    return true;
}

// Logger 的方法实现
Logger::Level Logger::currentLevel = Logger::Level::INFO;

void Logger::SetLevel(Level level) {
    currentLevel = level;
}

void Logger::Debug(const std::string& message) {
    Log(Level::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(Level::INFO, message);
}

void Logger::Warning(const std::string& message) {
    Log(Level::WARNING, message);
}

void Logger::Error(const std::string& message) {
    Log(Level::ERROR, message);
}

void Logger::Log(Level level, const std::string& message) {
    if (level < currentLevel) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    std::tm tm;
    localtime_s(&tm, &time);
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    auto& output = (level >= Level::WARNING) ? std::cerr : std::cout;

    output << "[" << ss.str() << "] "
        << "[" << LevelToString(level) << "] "
        << message << std::endl;
}

const char* Logger::LevelToString(Level level) {
    switch (level) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO: return "INFO";
    case Level::WARNING: return "WARNING";
    case Level::ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}