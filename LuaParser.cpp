#include "LuaParser.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

LuaParser::LuaParser() : L(nullptr), isLoaded(false) {
    L = luaL_newstate();
    if (L) {
        luaL_openlibs(L);
    }
    else {
        Logger::Error("Lua状态机初始化失败");
    }
}

LuaParser::~LuaParser() {
    if (L) {
        lua_close(L);
        L = nullptr;
        Logger::Debug("Lua状态机关闭");
    }
}

bool LuaParser::loadLuaFile(const std::string& path) {
    Logger::Debug("尝试加载Lua文件: " + path);

    if (!L) {
        Logger::Error("Lua状态机未初始化");
        return false;
    }

    int result = luaL_dofile(L, path.c_str());
    if (result != LUA_OK) {
        Logger::Error("Lua文件加载失败: " + path);
        Logger::Error("错误信息: " + std::string(lua_tostring(L, -1)));
        lua_pop(L, 1);
        return false;
    }

    currentFilePath = path;
    isLoaded = true;
    Logger::Info("Lua文件加载成功: " + path);
    return true;
}

bool LuaParser::parseFgPos() {
    Logger::Debug("尝试解析fgpos表");

    if (!isLoaded) {
        Logger::Error("Lua文件未加载");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos不是表或未找到");
        lua_pop(L, 1);
        return false;
    }

    int groupCount = 0;
    int fileCount = 0;
    fgPos.clear();

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (!lua_isstring(L, -2)) {
            lua_pop(L, 1);
            continue;
        }

        std::string groupName = lua_tostring(L, -2);
        Logger::Debug("解析组: " + groupName);

        if (lua_istable(L, -1)) {
            PosMap posMap;
            int currontFileCount = 0;

            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (!lua_isstring(L, -2)) {
                    lua_pop(L, 1);
                    continue;
                }

                std::string fileName = lua_tostring(L, -2);

                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "x");
                    if (lua_isnumber(L, -1)) {
                        int x = lua_tointeger(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, -1, "y");
                        if (lua_isnumber(L, -1)) {
                            int y = lua_tointeger(L, -1);
                            lua_pop(L, 1);

                            posMap[fileName] = Pos(x, y);
                            currontFileCount++;
                            Logger::Debug("解析文件: " + fileName + 
                                "，坐标: (" + std::to_string(x) + 
                                ", " + std::to_string(y) + ")");
                        }
                        else {
                            lua_pop(L, 1);
                            Logger::Warning("文件" + fileName + "缺少y坐标");
                        }
                    }
                    else {
                        lua_pop(L, 1);
                        Logger::Warning("文件" + fileName + "缺少x坐标");
                    }
                }

                lua_pop(L, 1);
            }

            fgPos[groupName] = std::move(posMap); // 使用移动语义
            groupCount++;
            fileCount += currontFileCount;
            Logger::Debug("组" + groupName + "解析完成，包含" + std::to_string(currontFileCount) + "个文件");
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    Logger::Info("fgpos解析完成，包含" + std::to_string(groupCount) + "个组，" + std::to_string(fileCount) + "个文件");
    return true;
}

bool LuaParser::parseGroup(const std::string& group) {
    Logger::Debug("尝试解析组: " + group);

    if (!isLoaded) {
        Logger::Error("Lua文件未加载");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos不是表或未找到");
        lua_pop(L, 1);
        return false;
    }

    lua_getfield(L, -1, group.c_str());
    if (!lua_istable(L, -1)) {
        Logger::Error("组" + group + "未找到或不是表");
        lua_pop(L, 2);
        return false;
    }

    int fileCount = 0;
    PosMap posMap;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (!lua_isstring(L, -2)) {
            lua_pop(L, 1);
            continue;
        }

        std::string fileName = lua_tostring(L, -2);

        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "x");
            if (lua_isnumber(L, -1)) {
                int x = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, -1, "y");
                if (lua_isnumber(L, -1)) {
                    int y = lua_tointeger(L, -1);
                    lua_pop(L, 1);

                    // 检测冲突
                    if (posMap.find(fileName) != posMap.end()) {
                        Logger::Warning("同名文件冲突: " + fileName);
                    }

                    posMap[fileName] = Pos(x, y);
                    fileCount++;
                    Logger::Debug("解析文件: " + fileName + "，坐标: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                }
                else {
                    lua_pop(L, 1);
                    Logger::Warning("文件" + fileName + "缺少y坐标");
                }
            }
            else {
                lua_pop(L, 1);
                Logger::Warning("文件" + fileName + "缺少x坐标");
            }
        }

        lua_pop(L, 1);
    }

    fgPos[group] = std::move(posMap); // 使用移动语义
    lua_pop(L, 2);
    Logger::Info("组" + group + "解析完成，包含" + std::to_string(fileCount) + "个文件");
    return true;
}

bool LuaParser::parseGroups(const std::string& field) {
    Logger::Debug("尝试解析字段: " + field + "匹配的所有组");

    if (!isLoaded) {
        Logger::Error("Lua文件未加载");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos不是表或未找到");
        lua_pop(L, 1);
        return false;
    }

    int groupCount = 0;
    int fileCount = 0;
    PosMap posMap;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (!lua_isstring(L, -2)) {
            lua_pop(L, 1);
            continue;
        }

        std::string groupName = lua_tostring(L, -2);
        if (groupName.find(field) != std::string::npos) {
            Logger::Debug("匹配到组: " + groupName);

            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    if (!lua_isstring(L, -2)) {
                        lua_pop(L, 1);
                        continue;
                    }

                    std::string fileName = lua_tostring(L, -2);
                    if (lua_istable(L, -1)) {
                        lua_getfield(L, -1, "x");
                        if (lua_isnumber(L, -1)) {
                            int x = lua_tointeger(L, -1);
                            lua_pop(L, 1);

                            lua_getfield(L, -1, "y");
                            if (lua_isnumber(L, -1)) {
                                int y = lua_tointeger(L, -1);
                                lua_pop(L, 1);

                                posMap[fileName] = Pos(x, y);
                                fileCount++;
                                Logger::Debug("解析文件: " + fileName + "，坐标: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                            }
                            else {
                                lua_pop(L, 1);
                                Logger::Warning("文件" + fileName + "缺少y坐标");
                            }
                        }
                        else {
                            lua_pop(L, 1);
                            Logger::Warning("文件" + fileName + "缺少x坐标");
                        }
                    }
                    lua_pop(L, 1);
                }
            }
            groupCount++;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (groupCount == 0) {
        Logger::Warning("未找到匹配的字段: " + field);
    }
    else {
        fgPos[field] = std::move(posMap); // 使用移动语义
        Logger::Info("字段" + field + "匹配的组解析完成，包含" + std::to_string(groupCount) + "个组，" + std::to_string(fileCount) + "个文件");
    }
    return true;
}

const PosMap* LuaParser::getGroupPos(const std::string& group) const {
    auto it = fgPos.find(group);
    if (it == fgPos.end()) {
        return nullptr;
    }
    return &(it->second); // 返回指针，避免拷贝
}

const std::pair<int, int> LuaParser::getFilePos(const std::string& group, const std::string& file) const {
    auto groupIt = fgPos.find(group);
    if (groupIt == fgPos.end()) {
        Logger::Error("组" + group + "未找到");
        return { 0, 0 };
    }
    auto fileIt = groupIt->second.find(file);
    if (fileIt == groupIt->second.end()) {
        Logger::Error("文件" + file + "未找到");
        return { 0, 0 };
    }
    Logger::Debug("获取文件位置: " + group + "/" + file + " -> (" + std::to_string(fileIt->second.x) + "," + std::to_string(fileIt->second.y) + ")");
    return  { fileIt->second.x, fileIt->second.y };
}

bool LuaParser::hasGroup(const std::string& group) const {
    return fgPos.find(group) != fgPos.end();
}

bool LuaParser::hasFile(const std::string& group, const std::string& file) const {
    auto groupIt = fgPos.find(group);
    if (groupIt == fgPos.end()) {
        return false;
    }
    return groupIt->second.find(file) != groupIt->second.end();
}

std::vector<std::string> LuaParser::getGroupNames() const {
    std::vector<std::string> names;
    names.reserve(fgPos.size());

    for (const auto& pair : fgPos) {
        names.push_back(pair.first);
    }

    return names;
}

std::vector<std::string> LuaParser::getFileNames(const std::string& group) const {
    std::vector<std::string> names;
    auto groupIt = fgPos.find(group);

    if (groupIt != fgPos.end()) {
        names.reserve(groupIt->second.size());
        for (const auto& pair : groupIt->second) {
            names.push_back(pair.first);
        }
    }

    return names;
}

void LuaParser::setFilePos(const std::string& group, const std::string& file, const Pos& pos) {
    bool existed = hasFile(group, file);
    fgPos[group][file] = pos;

    if (existed) {
        Logger::Debug("更新文件位置: " + group + "/" + file + " -> (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")");
    }
    else {
        Logger::Debug("添加文件位置: " + group + "/" + file + " -> (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")");
    }
}

bool LuaParser::saveToFile(const std::string& path) {
    std::string savePath = path.empty() ? currentFilePath : path;
    Logger::Debug("尝试保存Lua文件到: " + savePath);
    if (savePath.empty()) {
        Logger::Error("未指定文件路径");
        return false;
    }

    // 使用字符串流直接生成Lua代码，避免创建新的Lua状态
    std::stringstream luaCode;
    luaCode << "fgpos = {\n";

    int groupCount = 0;
    int fileCount = 0;

    for (const auto& groupPair : fgPos) {
        luaCode << "    " << groupPair.first << " = {\n";
        groupCount++;
        int currentFileCount = 0;

        for (const auto& filePair : groupPair.second) {
            luaCode << "        " << filePair.first << " = {x = "
                << filePair.second.x << ", y = " << filePair.second.y << "},\n";
            currentFileCount++;
        }

        fileCount += currentFileCount;
        Logger::Debug("保存组" + groupPair.first + ",包含" + std::to_string(currentFileCount) + "个文件");
        luaCode << "    },\n";
    }

    luaCode << "}\n";

    // 直接写入文件
    std::ofstream file(savePath);
    if (!file.is_open()) {
        Logger::Error("无法打开文件进行写入: " + savePath);
        return false;
    }

    file << luaCode.str();
    file.close();

    Logger::Info("Lua文件保存成功，包含" + std::to_string(groupCount) + "个组，" + std::to_string(fileCount) + "个文件");
    return true;
}

// 辅助方法
bool LuaParser::pushFgPosToLua() {
    lua_newtable(L);

    for (const auto& groupPair : fgPos) {
        lua_pushstring(L, groupPair.first.c_str());
        pushPosMapToLua(groupPair.second);
        lua_settable(L, -3);
    }

    return true;
}

bool LuaParser::pushPosMapToLua(const PosMap& posMap) {
    lua_newtable(L);

    for (const auto& filePair : posMap) {
        lua_pushstring(L, filePair.first.c_str());
        pushPosToLua(filePair.second);
        lua_settable(L, -3);
    }

    return true;
}

bool LuaParser::pushPosToLua(const Pos& pos) {
    lua_newtable(L);

    lua_pushstring(L, "x");
    lua_pushinteger(L, pos.x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushinteger(L, pos.y);
    lua_settable(L, -3);

    return true;
}