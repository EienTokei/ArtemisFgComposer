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
    // 组合：一个完整的合成结果
    struct Combination {
        std::vector<std::string> components;  // 组件文件名
        std::string outputFilename;           // 输出文件名

        Combination() = default;
        Combination(const std::vector<std::string>& files, const std::string& output)
            : components(files), outputFilename(output) {
        }
    };

    // 部件分类结构
    struct Part {
        std::vector<std::string> files;           // 属于该分类的文件名

        Part() = default;
    };

    // 组结构：按字母分组 (a~z)
    struct Group {
        std::map<std::string, Part> parts; // 部件分类映射

        Group() = default;
    };

    explicit FgComposer(const Config& config);
    ~FgComposer();

    /**
     * @brief 执行合成流程
     * @return 成功返回true，失败返回false
     */
    bool process();

    /**
     * @brief 获取处理统计信息
     * @return 合成的组合数量
     */
    int getCombinationCount() const { return combinations.size(); }

private:
    const Config& config;
    std::unordered_map<std::string, Group> groups;           // 组名->组映射
    std::unordered_map<std::string, ImageData> images;       // 文件名->图像数据

    std::vector<Combination> combinations;                   // 所有组合
    LuaParser luaParser;                                     // Lua坐标解析器

    // 笛卡尔积生成器
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

            // 进位算法
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
     * @brief 扫描目录, 加载并分类所有图像
     * @return 成功返回true
     */
    bool loadAndClassifyImages();

    /**
     * @brief 生成所有可能的组合
     * @return 成功返回true
     */
    bool generateCombinations();

    /**
     * @brief 执行图像合成
     * @return 成功返回true
     */
    bool composeImages();

    /**
     * @brief 从文件名提取组名（首字母）
     * @param filename 文件名
     * @return 组名字符串
     */
    std::string getGroupName(const std::string& filename) const;

    /**
     * @brief 确定文件所属部件分类
     * @param filename 文件名
     * @return 部件名称，空字符串表示不匹配任何规则
     */
    std::string getPartName(const std::string& filename) const;

    /**
     * @brief 获取图像的坐标
     * @param filename 文件名
     * @param group 组名
     * @return 坐标对 (x, y)
     */
    //std::pair<int, int> getPos(const std::string& filename, const std::string& group) const;

    /**
     * @brief 生成输出文件名
     * @param components 组件文件名列表
     * @return 输出文件名
     */
    std::string makeOutputFilename(const std::vector<std::string>& components) const;

    /**
     * @brief 生成所有组合并合成
     * @return 成功返回true
     */
    //bool generateAndCompose();
};
