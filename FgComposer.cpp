#include "FgComposer.h"

namespace fs = std::filesystem;

FgComposer::FgComposer(const Config& config) : config(config) {
    Logger::Debug("FgComposer��ʼ����ʼ");

    // �����Lua·��������Lua������
    if (!config.luaPath.empty()) {
        if (luaParser.loadLuaFile(config.luaPath)) {

            // ����
            if (luaParser.parseGroups(config.globalName)) {
                Logger::Info("Lua�ļ������ɹ�");
            }
            else {
                Logger::Warning("Lua�ļ�����ʧ��");
            }
        }
        else {
            Logger::Warning("Lua�ļ�����ʧ��: " + config.luaPath);
        }
    }
    else {
        Logger::Info("δ����Lua·������ʹ������������Ϣ");
    }

    Logger::Debug("FgComposer��ʼ�����");
}

FgComposer::~FgComposer() {
    Logger::Debug("��ʼ����FgComposer��Դ");

    // ������Դ
    int freedCount = 0;
    for (auto& [filename, image] : images) {
        ImageProcessor::FreeImage(image);
        freedCount++;
    }

    Logger::Debug("���ͷ� " + std::to_string(freedCount) + " ��ͼ����Դ");
}

bool FgComposer::process() {
    Logger::Info("��ʼ��������");

    // 1. ���ط���ͼ��
    Logger::Info("��ʼ���غͷ���Ŀ¼�е�ͼ��");
    if (!loadAndClassifyImages()) {
        Logger::Error("ͼ����غͷ���ʧ��");
        return false;
    }
    //Logger::Info("ͼ����غͷ�����ɣ������� " + std::to_string(images.size()) + " ��ͼ��");

    // 2. �������
    Logger::Info("��ʼ����ͼ�����");
    if (!generateCombinations()) {
        Logger::Error("ͼ���������ʧ��");
        return false;
    }
    //Logger::Info("ͼ�����������ɣ������� " + std::to_string(combinations.size()) + " �����");

    // 3. �������
    Logger::Info("��ʼ����ͼ�����");
    if (!composeImages()) {
        Logger::Error("ͼ��ϳɺͱ���ʧ��");
        return false;
    }

    Logger::Info("�����������");
    return true;
}

bool FgComposer::loadAndClassifyImages() {
    Logger::Debug("��ʼ���غͷ���Ŀ¼�е�ͼ��: " + config.inputDir);

    try {
        int loadedCount = 0;
        int skippedCount = 0;

        for (const auto& entry : fs::directory_iterator(config.inputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string filepath = entry.path().string();
            std::string extension = entry.path().extension().string();

            // ֻ����PNGͼƬ
            if (extension != ".png" && extension != ".PNG") {
                Logger::Debug("������PNG�ļ�: " + filepath);
                skippedCount++;
                continue;
            }

            std::string filename = entry.path().stem().string();
            Logger::Info("����ͼ���ļ�: " + filename);

            // ����ͼ��
            ImageData image;
            bool loadSuccess = false;
            if (luaParser.Loaded()) {
                Logger::Debug("����ͼ��: " + filename);
                loadSuccess = ImageProcessor::LoadPng(filepath, image);
            }
            else {
                Logger::Debug("ʹ�������������ͼ��: " + filename);
                loadSuccess = ImageProcessor::LoadPngWithPos(filepath, image);
            }

            if (!loadSuccess) {
                Logger::Warning("ͼ�����ʧ��: " + filepath);
                skippedCount++;
                continue;
            }

            // ��������
            if (luaParser.Loaded()) {
                const auto& [x, y] = luaParser.getFilePos(config.globalName, filename);
                image.posX = x;
                image.posY = y;
            }
            // �洢ͼ������
            images[filename] = std::move(image);
            loadedCount++;

            // ����
            std::string groupName = getGroupName(filename);
            if (groupName.empty()) {
                Logger::Warning("�޷�ʶ��ͼ������: " + filename);
                continue;
            }

            std::string partName = getPartName(filename);
            if (partName.empty()) {
                Logger::Warning("�޷�ʶ��ͼ�񲿼���: " + filename);
                continue;
            }

            // ��ӵ���Ӧ����Ͳ���
            groups[groupName].parts[partName].files.push_back(filename);
            Logger::Info("ͼ�����: " + filename + " -> ��[" + groupName + "], ����[" + partName + "]");
        }

        Logger::Info("ͼ��������: �ɹ� " + std::to_string(loadedCount) +
            ", ���� " + std::to_string(skippedCount) +
            ", ������ " + std::to_string(groups.size()));
        return true;
    }
    catch (const fs::filesystem_error& ex) {
        Logger::Error("�ļ�ϵͳ����: " + std::string(ex.what()));
        return false;
    }
    catch (const std::exception& ex) {
        Logger::Error("����ͼ��ʱ�����쳣: " + std::string(ex.what()));
        return false;
    }
}

bool FgComposer::generateCombinations() {
    Logger::Debug("��ʼ����ͼ�����");

    int totalCombinations = 0;

    for (const auto& [groupName, group] : groups) {

        Logger::Info("������: " + groupName);

        // �����л�����
        auto baseIt = group.parts.find("base");
        if (baseIt == group.parts.end() || baseIt->second.files.empty()) {
            Logger::Warning("�� " + groupName + " ȱ�ٻ�����(base)������");
            continue;
        }

        // �ռ�������������
        std::vector<std::vector<std::string>> partFiles;

        for (const auto& [partName, part] : group.parts) {
            if (partName != "base" && !part.files.empty()) {
                partFiles.push_back(part.files);
                Logger::Debug("�� " + groupName + " ���� " + partName + " �� " +
                    std::to_string(part.files.size()) + " ���ļ�");
            }
        }

        // Ϊÿ������ͼ���������
        Logger::Debug("�� " + groupName + " �� " + std::to_string(baseIt->second.files.size()) + " ������ͼ��");

        for (const std::string& baseFile : baseIt->second.files) {
            // ���û������������ֻ�л���ͼ��
            if (partFiles.empty()) {
                Combination combo;
                combo.components = { baseFile };
                combo.outputFilename = makeOutputFilename({ baseFile });
                combinations.push_back(combo);
                totalCombinations++;
                Logger::Debug("���ɻ������: " + combo.outputFilename);
                continue;
            }

            // ʹ�õѿ����������������������
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

            Logger::Info("����ͼ�� " + baseFile + " ������ " + std::to_string(groupCombinations) + " �����");
        }
    }

    Logger::Info("���������ɣ��ܹ� " + std::to_string(totalCombinations) + " �����");
    return true;
}

bool FgComposer::composeImages() {
    Logger::Debug("��ʼ�ϳ�ͼ��");

    // ȷ�����Ŀ¼����
    if (!fs::exists(config.outputDir)) {
        Logger::Info("�������Ŀ¼: " + config.outputDir);
        try {
            fs::create_directories(config.outputDir);
            Logger::Debug("���Ŀ¼�����ɹ�");
        }
        catch (const fs::filesystem_error& ex) {
            Logger::Error("�������Ŀ¼ʧ��: " + std::string(ex.what()));
            return false;
        }
    }
    else {
        Logger::Debug("���Ŀ¼�Ѵ���: " + config.outputDir);
    }

    int successCount = 0;
    int failCount = 0;

    for (size_t i = 0; i < combinations.size(); ++i) {
        const auto& combination = combinations[i];
        if (combination.components.empty()) {
            Logger::Warning("��������� #" + std::to_string(i));
            continue;
        }

        Logger::Info("������� " + std::to_string(i + 1) + "/" + std::to_string(combinations.size()) +
            ": " + combination.outputFilename);

        // �ӻ���ͼ��ʼ
        const std::string& baseFile = combination.components[0];
        auto baseIt = images.find(baseFile);
        if (baseIt == images.end()) {
            Logger::Error("����ͼ��δ�ҵ�: " + baseFile);
            failCount++;
            continue;
        }

        ImageData result = baseIt->second;

        // ��˳�������������
        for (size_t j = 1; j < combination.components.size(); ++j) {
            const std::string& componentFile = combination.components[j];
            auto componentIt = images.find(componentFile);
            if (componentIt == images.end()) {
                Logger::Warning("����ͼ��δ�ҵ�: " + componentFile + "�������ò���");
                continue;
            }
            const ImageData& componentData = componentIt->second;
            // �ϳ�
            result = ImageProcessor::Blend(result, componentData);
        }

        // ��������ͼ��
        std::string outputPath = config.outputDir + "\\" + combination.outputFilename;
        Logger::Debug("����ͼ��: " + outputPath);

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
            Logger::Info("ͼ�񱣴�ɹ�");
        }
        else {
            failCount++;
            Logger::Error("ͼ�񱣴�ʧ��");
        }
    }

    Logger::Info("ͼ��ϳ����: �ɹ� " + std::to_string(successCount) +
        ", ʧ�� " + std::to_string(failCount));

    return failCount == 0; // ������ж��ɹ��ŷ���true
}

std::string FgComposer::getGroupName(const std::string& filename) const {
    std::smatch match;
    std::regex groupRegex(config.groupRule);
    if (regex_search(filename, match, groupRegex)) {
        char first_char = match[1].str()[0];
        std::string groupName = std::string(1, first_char);
        Logger::Debug("��ȡ����: " + filename + " -> " + groupName);
        return groupName;
    }
    Logger::Debug("�޷���ȡ����: " + filename);
    return "";
}

std::string FgComposer::getPartName(const std::string& filename) const {
    for (const auto& rule : config.partRules) {
        if (std::regex_search(filename, std::regex(rule.pattern))) {
            Logger::Debug("ʶ�𲿼�: " + filename + " -> " + rule.partName);
            return rule.partName;
        }
    }
    Logger::Debug("�޷�ʶ�𲿼�: " + filename);
    return "";
}

std::string FgComposer::makeOutputFilename(const std::vector<std::string>& components) const {
    std::string result;
    for (const auto& component : components) {
        result += component;
        result += "_";
    }
    result.pop_back(); // �Ƴ����һ��"_"

    result += ".png";

    Logger::Debug("��������ļ���: " + result);
    return result;
}


//std::pair<int, int> FgComposer::getPos(const std::string& filename, const std::string& group) const {
//    Logger::Debug("��ȡͼ������: " + filename + " (��: " + group + ")");
//
//    // ���ȴ�Lua��������ȡ����
//    if (luaParser.Loaded()) {
//        Pos pos = luaParser.getFilePos(group, filename);
//        Logger::Debug("��Lua��ȡ����: (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")");
//        return { pos.x, pos.y };
//    }
//
//    // ��ͼ�񻺴��л�ȡ���꣨���LoadPngWithPos�Ѿ����������꣩
//    auto it = images.find(filename);
//    if (it != images.end()) {
//        Logger::Debug("��ͼ�񻺴��ȡ����: (" + std::to_string(it->second.posX) + ", " + std::to_string(it->second.posY) + ")");
//        return { it->second.posX, it->second.posY };
//    }
//
//    // Ĭ������
//    Logger::Debug("ʹ��Ĭ������: (0, 0)");
//    return { 0, 0 };
//}