#pragma once

#include <map>
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <filesystem>
#include "LuaParser.h"
#include "ImageProcessor.h"
#include "Config.h"

class FgComposer {
public:
    // ��ϣ�һ�������ĺϳɽ��
    struct Combination {
        std::vector<std::string> components;  // ����ļ���
        std::string outputFilename;           // ����ļ���

        Combination() = default;
        Combination(const std::vector<std::string>& files, const std::string& output)
            : components(files), outputFilename(output) {
        }
    };

    // ��������ṹ
    struct Part {
        std::vector<std::string> files;           // ���ڸ÷�����ļ���

        Part() = default;
    };

    // ��ṹ������ĸ���� (a~z)
    struct Group {
        std::map<std::string, Part> parts; // ��������ӳ��

        Group() = default;
    };

    explicit FgComposer(const Config& config);
    ~FgComposer();

    /**
     * @brief ִ�кϳ�����
     * @return �ɹ�����true��ʧ�ܷ���false
     */
    bool process();

    /**
     * @brief ��ȡ����ͳ����Ϣ
     * @return �ϳɵ��������
     */
    int getCombinationCount() const { return combinations.size(); }

private:
    const Config& config;
    std::unordered_map<std::string, Group> groups;           // ����->��ӳ��
    std::unordered_map<std::string, ImageData> images;       // �ļ���->ͼ������

    std::vector<Combination> combinations;                   // �������
    LuaParser luaParser;                                     // Lua���������

    // �ѿ�����������
    class CombinationGenerator {
    private:
        const std::vector<std::vector<std::string>>& arrays;
        std::vector<size_t> indices;
        bool hasNext;

    public:
        CombinationGenerator(const std::vector<std::vector<std::string>>& arr)
            : arrays(arr), indices(arr.size(), 0), hasNext(!arr.empty()) {
        }

        bool hasMore() const { return hasNext; }

        std::vector<std::string> getNext() {
            std::vector<std::string> result;
            for (size_t i = 0; i < arrays.size(); ++i) {
                result.push_back(arrays[i][indices[i]]);
            }

            // ��λ�㷨
            for (int i = arrays.size() - 1; i >= 0; --i) {
                indices[i]++;
                if (indices[i] < arrays[i].size()) break;
                indices[i] = 0;
                if (i == 0) hasNext = false;
            }

            return result;
        }
    };

    /**
     * @brief ɨ��Ŀ¼, ���ز���������ͼ��
     * @return �ɹ�����true
     */
    bool loadAndClassifyImages();

    /**
     * @brief �������п��ܵ����
     * @return �ɹ�����true
     */
    bool generateCombinations();

    /**
     * @brief ִ��ͼ��ϳ�
     * @return �ɹ�����true
     */
    bool composeImages();

    /**
     * @brief ���ļ�����ȡ����������ĸ��
     * @param filename �ļ���
     * @return �����ַ���
     */
    std::string getGroupName(const std::string& filename) const;

    /**
     * @brief ȷ���ļ�������������
     * @param filename �ļ���
     * @return �������ƣ����ַ�����ʾ��ƥ���κι���
     */
    std::string getPartName(const std::string& filename) const;

    /**
     * @brief ��ȡͼ�������
     * @param filename �ļ���
     * @param group ����
     * @return ����� (x, y)
     */
    //std::pair<int, int> getPos(const std::string& filename, const std::string& group) const;

    /**
     * @brief ��������ļ���
     * @param components ����ļ����б�
     * @return ����ļ���
     */
    std::string makeOutputFilename(const std::vector<std::string>& components) const;

    /**
     * @brief ����������ϲ��ϳ�
     * @return �ɹ�����true
     */
    //bool generateAndCompose();
};
