#pragma once

#include "lua.hpp"
#include <unordered_map>
#include <string>

//fgpos = {
//	tak_bca = {
//		a0001 = {x = 457,y = 192},
//		a0099 = {x = 464,y = 164},
//		tak_bca0000 = {x = 341,y = 113},
//		tak_bca0502 = {x = 331,y = 113},
//	},
//	tak_z2b = {
//		b0001 = {x = 329,y = 226},
//		b0099 = {x = 351,y = 158},
//		tak_z2b0000 = {x = 177,y = 14},
//		tak_z2b0502 = {x = 101,y = 14},
//	},
//}

struct Pos {
    int x;
    int y;
    Pos(int x = 0, int y = 0) : x(x), y(y) {}
};

using PosMap = std::unordered_map<std::string, Pos>;
using FgPos = std::unordered_map<std::string, PosMap>;

class LuaParser {
public:
    LuaParser();
    ~LuaParser();

    /**
    * @brief 加载Lua文件到状态机
    * @param path Lua文件路径
    * @return bool:是否加载成功
    */
    bool loadLuaFile(const std::string& path);

    /**
     * @brief 解析Lua文件的fgpos表
     * @return bool:是否解析成功
     */
    bool parseFgPos();

    /**
     * @brief 解析Lua文件的特定组
     * @param group 组名
     * @return bool:是否解析成功
     */
    bool parseGroup(const std::string& group);

    /**
     * @brief 解析多个匹配组至一个映射表
     * @param field 匹配字段
     * @return bool:是否解析成功
     */
    bool parseGroups(const std::string& field);

    /**
     * @brief 获取整个组的坐标
     * @param group 组名
     * @return const PosMap*:坐标映射表
     */
    const PosMap* getGroupPos(const std::string& group) const;

    /**
     * @brief 获取特定组的特定坐标
     * @param group 组名
     * @param file 文件名
     * @return pair<int, int>:坐标
     */
    const std::pair<int, int> getFilePos(const std::string& group, const std::string& file) const;

    /**
     * @brief 检查组是否存在
     * @param group 组名
     * @return bool:是否存在
     */
    bool hasGroup(const std::string& group) const;

    /**
     * @brief 检查特定文件是否存在
     * @param group 组名
     * @param file 文件名
     * @return bool:是否存在
     */
    bool hasFile(const std::string& group, const std::string& file) const;

    /**
     * @brief 获取所有组名的列表
     * @return std::vector<std::string>:组名列表
     */
    std::vector<std::string> getGroupNames() const;

    /**
     * @brief 获取特定组中所有文件名的列表
     * @param group 组名
     * @return std::vector<std::string>:文件名列表
     */
    std::vector<std::string> getFileNames(const std::string& group) const;

    /**
     * @brief 设置坐标（用于更新）
     * @param group 组名
     * @param file 文件名
     * @param pos 坐标
     * @note 如果文件不存在，会自动创建
     */
    void setFilePos(const std::string& group, const std::string& file, const Pos& pos);

    /**
     * @brief 保存修改回Lua文件
     * @param path Lua文件路径
     * @return bool:是否保存成功
     */
    bool saveToFile(const std::string& path = "");

    bool Loaded() const { return isLoaded; }

private:
    lua_State* L;
    FgPos fgPos;
    bool isLoaded;
    std::string currentFilePath;

    bool pushFgPosToLua();
    bool pushPosMapToLua(const PosMap& posMap);
    bool pushPosToLua(const Pos& pos);
};



