#include "FgComposer.h"

namespace fs = std::filesystem;

FgComposer::FgComposer(const Config& config) : config(config) {
    Logger::Debug("FgComposer初始化开始");

    // 如果有Lua路径，加载Lua解析器
    if (!config.luaPath.empty()) {
        if (luaParser.loadLuaFile(config.luaPath)) {

            // 解析
            if (luaParser.parseGroups(config.globalName)) {
                Logger::Info("Lua文件解析成功");
            }
            else {
                Logger::Warning("Lua文件解析失败");
            }
        }
        else {
            Logger::Warning("Lua文件加载失败: " + config.luaPath);
        }
    }
    else {
        Logger::Info("未配置Lua路径，将使用内置坐标信息");
    }

    Logger::Debug("FgComposer初始化完成");
}

FgComposer::~FgComposer() {
    Logger::Debug("开始清理FgComposer资源");

    // 清理资源
    int freedCount = 0;
    for (auto& [filename, image] : images) {
        ImageProcessor::FreeImage(image);
        freedCount++;
    }

    Logger::Debug("已释放 " + std::to_string(freedCount) + " 个图像资源");
}

bool FgComposer::process() {
    Logger::Info("开始处理流程");

    // 1. 加载分类图像
    Logger::Info("开始加载和分类目录中的图像");
    if (!loadAndClassifyImages()) {
        Logger::Error("图像加载和分类失败");
        return false;
    }
    //Logger::Info("图像加载和分类完成，共加载 " + std::to_string(images.size()) + " 个图像");

    // 2. 生成组合
    Logger::Info("开始生成图像组合");
    if (!generateCombinations()) {
        Logger::Error("图像组合生成失败");
        return false;
    }
    //Logger::Info("图像组合生成完成，共生成 " + std::to_string(combinations.size()) + " 个组合");

    // 3. 保存组合
    Logger::Info("开始保存图像组合");
    if (!composeImages()) {
        Logger::Error("图像合成和保存失败");
        return false;
    }

    Logger::Info("处理流程完成");
    return true;
}

bool FgComposer::loadAndClassifyImages() {
    Logger::Debug("开始加载和分类目录中的图像: " + config.inputDir);

    try {
        int loadedCount = 0;
        int skippedCount = 0;

        for (const auto& entry : fs::directory_iterator(config.inputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string filepath = entry.path().string();
            std::string extension = entry.path().extension().string();

            // 只处理PNG图片
            if (extension != ".png" && extension != ".PNG") {
                Logger::Debug("跳过非PNG文件: " + filepath);
                skippedCount++;
                continue;
            }

            std::string filename = entry.path().stem().string();
            Logger::Info("处理图像文件: " + filename);

            // 加载图像
            ImageData image;
            bool loadSuccess = false;
            if (luaParser.Loaded()) {
                Logger::Debug("加载图像: " + filename);
                loadSuccess = ImageProcessor::LoadPng(filepath, image);
            }
            else {
                Logger::Debug("使用内置坐标加载图像: " + filename);
                loadSuccess = ImageProcessor::LoadPngWithPos(filepath, image);
            }

            if (!loadSuccess) {
                Logger::Warning("图像加载失败: " + filepath);
                skippedCount++;
                continue;
            }

            // 更新坐标
            if (luaParser.Loaded()) {
                const auto& [x, y] = luaParser.getFilePos(config.globalName, filename);
                image.posX = x;
                image.posY = y;
            }
            // 存储图像数据
            images[filename] = std::move(image);
            loadedCount++;

            // 分类
            std::string groupName = getGroupName(filename);
            if (groupName.empty()) {
                Logger::Warning("无法识别图像组名: " + filename);
                continue;
            }

            std::string partName = getPartName(filename);
            if (partName.empty()) {
                Logger::Warning("无法识别图像部件名: " + filename);
                continue;
            }

            // 添加到对应的组和部件
            groups[groupName].parts[partName].files.push_back(filename);
            Logger::Info("图像分类: " + filename + " -> 组[" + groupName + "], 部件[" + partName + "]");
        }

        Logger::Info("图像加载完成: 成功 " + std::to_string(loadedCount) +
            ", 跳过 " + std::to_string(skippedCount) +
            ", 总组数 " + std::to_string(groups.size()));
        return true;
    }
    catch (const fs::filesystem_error& ex) {
        Logger::Error("文件系统错误: " + std::string(ex.what()));
        return false;
    }
    catch (const std::exception& ex) {
        Logger::Error("加载图像时发生异常: " + std::string(ex.what()));
        return false;
    }
}

bool FgComposer::generateCombinations() {
    Logger::Debug("开始生成图像组合");

    int totalCombinations = 0;

    for (const auto& [groupName, group] : groups) {

        Logger::Info("处理组: " + groupName);

        // 必须有基础层
        auto baseIt = group.parts.find("base");
        if (baseIt == group.parts.end() || baseIt->second.files.empty()) {
            Logger::Warning("组 " + groupName + " 缺少基础层(base)，跳过");
            continue;
        }

        // 收集所有其他部件
        std::vector<std::vector<std::string>> partFiles;

        for (const auto& [partName, part] : group.parts) {
            if (partName != "base" && !part.files.empty()) {
                partFiles.push_back(part.files);
                Logger::Debug("组 " + groupName + " 部件 " + partName + " 有 " +
                    std::to_string(part.files.size()) + " 个文件");
            }
        }

        // 为每个基础图像生成组合
        Logger::Debug("组 " + groupName + " 有 " + std::to_string(baseIt->second.files.size()) + " 个基础图像");

        for (const std::string& baseFile : baseIt->second.files) {
            // 如果没有其他部件，只有基础图像
            if (partFiles.empty()) {
                Combination combo;
                combo.components = { baseFile };
                combo.outputFilename = makeOutputFilename({ baseFile });
                combinations.push_back(combo);
                totalCombinations++;
                Logger::Debug("生成基础组合: " + combo.outputFilename);
                continue;
            }

            // 使用笛卡尔积生成器生成所有组合
            CombinationGenerator generator(partFiles);
            int groupCombinations = 0;
            while (generator.hasMore()) {
                std::vector<std::string> otherParts = generator.getNext();

                Combination combo;
                combo.components = { baseFile };
                combo.components.insert(combo.components.end(), otherParts.begin(), otherParts.end());
                combo.outputFilename = makeOutputFilename(combo.components);

                combinations.push_back(combo);
                groupCombinations++;
                totalCombinations++;
            }

            Logger::Info("基础图像 " + baseFile + " 生成了 " + std::to_string(groupCombinations) + " 个组合");
        }
    }

    Logger::Info("组合生成完成，总共 " + std::to_string(totalCombinations) + " 个组合");
    return true;
}

bool FgComposer::composeImages() {
    Logger::Debug("开始合成图像");

    // 确保输出目录存在
    if (!fs::exists(config.outputDir)) {
        Logger::Info("创建输出目录: " + config.outputDir);
        try {
            fs::create_directories(config.outputDir);
            Logger::Debug("输出目录创建成功");
        }
        catch (const fs::filesystem_error& ex) {
            Logger::Error("创建输出目录失败: " + std::string(ex.what()));
            return false;
        }
    }
    else {
        Logger::Debug("输出目录已存在: " + config.outputDir);
    }

    int successCount = 0;
    int failCount = 0;

    for (size_t i = 0; i < combinations.size(); ++i) {
        const auto& combination = combinations[i];
        if (combination.components.empty()) {
            Logger::Warning("跳过空组合 #" + std::to_string(i));
            continue;
        }

        Logger::Info("处理组合 " + std::to_string(i + 1) + "/" + std::to_string(combinations.size()) +
            ": " + combination.outputFilename);

        // 从基础图像开始
        const std::string& baseFile = combination.components[0];
        auto baseIt = images.find(baseFile);
        if (baseIt == images.end()) {
            Logger::Error("基础图像未找到: " + baseFile);
            failCount++;
            continue;
        }

        ImageData result = baseIt->second;

        // 按顺序叠加其他部件
        for (size_t j = 1; j < combination.components.size(); ++j) {
            const std::string& componentFile = combination.components[j];
            auto componentIt = images.find(componentFile);
            if (componentIt == images.end()) {
                Logger::Warning("部件图像未找到: " + componentFile + "，跳过该部件");
                continue;
            }
            const ImageData& componentData = componentIt->second;
            // 合成
            result = ImageProcessor::Blend(result, componentData);
        }

        // 保存最终图像
        std::string outputPath = config.outputDir + "\\" + combination.outputFilename;
        Logger::Debug("保存图像到: " + outputPath);

        bool success = false;
        if (config.writePosBack) {
            success = ImageProcessor::SavePngWithPos(outputPath, result);
        }
        else {
            success = ImageProcessor::SavePng(outputPath, result);
        }
        ImageProcessor::FreeImage(result);

        if (success) {
            successCount++;
            Logger::Info("图像保存成功");
        }
        else {
            failCount++;
            Logger::Error("图像保存失败");
        }
    }

    Logger::Info("图像合成完成: 成功 " + std::to_string(successCount) +
        ", 失败 " + std::to_string(failCount));

    return failCount == 0; // 如果所有都成功才返回true
}

std::string FgComposer::getGroupName(const std::string& filename) const {
    std::smatch match;
    std::regex groupRegex(config.groupRule);
    if (regex_search(filename, match, groupRegex)) {
        char first_char = match[1].str()[0];
        std::string groupName = std::string(1, first_char);
        Logger::Debug("提取组名: " + filename + " -> " + groupName);
        return groupName;
    }
    Logger::Debug("无法提取组名: " + filename);
    return "";
}

std::string FgComposer::getPartName(const std::string& filename) const {
    for (const auto& rule : config.partRules) {
        if (std::regex_search(filename, std::regex(rule.pattern))) {
            Logger::Debug("识别部件: " + filename + " -> " + rule.partName);
            return rule.partName;
        }
    }
    Logger::Debug("无法识别部件: " + filename);
    return "";
}

std::string FgComposer::makeOutputFilename(const std::vector<std::string>& components) const {
    std::string result;
    for (const auto& component : components) {
        result += component;
        result += "_";
    }
    result.pop_back(); // 移除最后一个"_"

    result += ".png";

    Logger::Debug("生成输出文件名: " + result);
    return result;
}


//std::pair<int, int> FgComposer::getPos(const std::string& filename, const std::string& group) const {
//    Logger::Debug("获取图像坐标: " + filename + " (组: " + group + ")");
//
//    // 优先从Lua解析器获取坐标
//    if (luaParser.Loaded()) {
//        Pos pos = luaParser.getFilePos(group, filename);
//        Logger::Debug("从Lua获取坐标: (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")");
//        return { pos.x, pos.y };
//    }
//
//    // 从图像缓存中获取坐标（如果LoadPngWithPos已经设置了坐标）
//    auto it = images.find(filename);
//    if (it != images.end()) {
//        Logger::Debug("从图像缓存获取坐标: (" + std::to_string(it->second.posX) + ", " + std::to_string(it->second.posY) + ")");
//        return { it->second.posX, it->second.posY };
//    }
//
//    // 默认坐标
//    Logger::Debug("使用默认坐标: (0, 0)");
//    return { 0, 0 };
//}