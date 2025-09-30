#pragma once

#include <string>
#include <iostream>

struct Config {
    // 分类规则定义
    struct PartRule {
        std::string partName;
        std::string pattern;

        PartRule(const std::string& name, const std::string& pat)
            : partName(name), pattern(pat) {
        }
    };

    // 基础配置项
    bool helpRequested = false;
    bool verbose = false;
    bool writePosBack = false;
    std::string inputDir;
    std::string outputDir;
    std::string luaPath;
    std::string globalName;

    // 分类规则
    std::string groupRule;
    std::vector<PartRule> partRules;

    // 构造函数
    Config() = default;
    Config(const std::string& inDir, const std::string& outDir = "", const std::string& luaFilePath = "");
    explicit Config(int argc, char* argv[]);

    void InitializeDefaultValues();
    void InitializeDefaultRules();
    bool Validate() const;

    static Config Parse(int argc, char* argv[]);
};

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static void SetLevel(Level level);

    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);

private:
    static Level currentLevel;

    static void Log(Level level, const std::string& message);
    static const char* LevelToString(Level level);
};
