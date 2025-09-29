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
        Logger::Error("Lua״̬����ʼ��ʧ��");
    }
}

LuaParser::~LuaParser() {
    if (L) {
        lua_close(L);
        L = nullptr;
        Logger::Debug("Lua״̬���ر�");
    }
}

bool LuaParser::loadLuaFile(const std::string& path) {
    Logger::Debug("���Լ���Lua�ļ�: " + path);

    if (!L) {
        Logger::Error("Lua״̬��δ��ʼ��");
        return false;
    }

    int result = luaL_dofile(L, path.c_str());
    if (result != LUA_OK) {
        Logger::Error("Lua�ļ�����ʧ��: " + path);
        Logger::Error("������Ϣ: " + std::string(lua_tostring(L, -1)));
        lua_pop(L, 1);
        return false;
    }

    currentFilePath = path;
    isLoaded = true;
    Logger::Info("Lua�ļ����سɹ�: " + path);
    return true;
}

bool LuaParser::parseFgPos() {
    Logger::Debug("���Խ���fgpos��");

    if (!isLoaded) {
        Logger::Error("Lua�ļ�δ����");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos���Ǳ��δ�ҵ�");
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
        Logger::Debug("������: " + groupName);

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
                            Logger::Debug("�����ļ�: " + fileName + 
                                "������: (" + std::to_string(x) + 
                                ", " + std::to_string(y) + ")");
                        }
                        else {
                            lua_pop(L, 1);
                            Logger::Warning("�ļ�" + fileName + "ȱ��y����");
                        }
                    }
                    else {
                        lua_pop(L, 1);
                        Logger::Warning("�ļ�" + fileName + "ȱ��x����");
                    }
                }

                lua_pop(L, 1);
            }

            fgPos[groupName] = std::move(posMap); // ʹ���ƶ�����
            groupCount++;
            fileCount += currontFileCount;
            Logger::Debug("��" + groupName + "������ɣ�����" + std::to_string(currontFileCount) + "���ļ�");
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    Logger::Info("fgpos������ɣ�����" + std::to_string(groupCount) + "���飬" + std::to_string(fileCount) + "���ļ�");
    return true;
}

bool LuaParser::parseGroup(const std::string& group) {
    Logger::Debug("���Խ�����: " + group);

    if (!isLoaded) {
        Logger::Error("Lua�ļ�δ����");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos���Ǳ��δ�ҵ�");
        lua_pop(L, 1);
        return false;
    }

    lua_getfield(L, -1, group.c_str());
    if (!lua_istable(L, -1)) {
        Logger::Error("��" + group + "δ�ҵ����Ǳ�");
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

                    // ����ͻ
                    if (posMap.find(fileName) != posMap.end()) {
                        Logger::Warning("ͬ���ļ���ͻ: " + fileName);
                    }

                    posMap[fileName] = Pos(x, y);
                    fileCount++;
                    Logger::Debug("�����ļ�: " + fileName + "������: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                }
                else {
                    lua_pop(L, 1);
                    Logger::Warning("�ļ�" + fileName + "ȱ��y����");
                }
            }
            else {
                lua_pop(L, 1);
                Logger::Warning("�ļ�" + fileName + "ȱ��x����");
            }
        }

        lua_pop(L, 1);
    }

    fgPos[group] = std::move(posMap); // ʹ���ƶ�����
    lua_pop(L, 2);
    Logger::Info("��" + group + "������ɣ�����" + std::to_string(fileCount) + "���ļ�");
    return true;
}

bool LuaParser::parseGroups(const std::string& field) {
    Logger::Debug("���Խ����ֶ�: " + field + "ƥ���������");

    if (!isLoaded) {
        Logger::Error("Lua�ļ�δ����");
        return false;
    }

    lua_getglobal(L, "fgpos");
    if (!lua_istable(L, -1)) {
        Logger::Error("fgpos���Ǳ��δ�ҵ�");
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
            Logger::Debug("ƥ�䵽��: " + groupName);

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
                                Logger::Debug("�����ļ�: " + fileName + "������: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
                            }
                            else {
                                lua_pop(L, 1);
                                Logger::Warning("�ļ�" + fileName + "ȱ��y����");
                            }
                        }
                        else {
                            lua_pop(L, 1);
                            Logger::Warning("�ļ�" + fileName + "ȱ��x����");
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
        Logger::Warning("δ�ҵ�ƥ����ֶ�: " + field);
    }
    else {
        fgPos[field] = std::move(posMap); // ʹ���ƶ�����
        Logger::Info("�ֶ�" + field + "ƥ����������ɣ�����" + std::to_string(groupCount) + "���飬" + std::to_string(fileCount) + "���ļ�");
    }
    return true;
}

const PosMap* LuaParser::getGroupPos(const std::string& group) const {
    auto it = fgPos.find(group);
    if (it == fgPos.end()) {
        return nullptr;
    }
    return &(it->second); // ����ָ�룬���⿽��
}

const std::pair<int, int> LuaParser::getFilePos(const std::string& group, const std::string& file) const {
    auto groupIt = fgPos.find(group);
    if (groupIt == fgPos.end()) {
        Logger::Error("��" + group + "δ�ҵ�");
        return { 0, 0 };
    }
    auto fileIt = groupIt->second.find(file);
    if (fileIt == groupIt->second.end()) {
        Logger::Error("�ļ�" + file + "δ�ҵ�");
        return { 0, 0 };
    }
    Logger::Debug("��ȡ�ļ�λ��: " + group + "/" + file + " -> (" + std::to_string(fileIt->second.x) + "," + std::to_string(fileIt->second.y) + ")");
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
        Logger::Debug("�����ļ�λ��: " + group + "/" + file + " -> (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")");
    }
    else {
        Logger::Debug("����ļ�λ��: " + group + "/" + file + " -> (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")");
    }
}

bool LuaParser::saveToFile(const std::string& path) {
    std::string savePath = path.empty() ? currentFilePath : path;
    Logger::Debug("���Ա���Lua�ļ���: " + savePath);
    if (savePath.empty()) {
        Logger::Error("δָ���ļ�·��");
        return false;
    }

    // ʹ���ַ�����ֱ������Lua���룬���ⴴ���µ�Lua״̬
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
        Logger::Debug("������" + groupPair.first + ",����" + std::to_string(currentFileCount) + "���ļ�");
        luaCode << "    },\n";
    }

    luaCode << "}\n";

    // ֱ��д���ļ�
    std::ofstream file(savePath);
    if (!file.is_open()) {
        Logger::Error("�޷����ļ�����д��: " + savePath);
        return false;
    }

    file << luaCode.str();
    file.close();

    Logger::Info("Lua�ļ�����ɹ�������" + std::to_string(groupCount) + "���飬" + std::to_string(fileCount) + "���ļ�");
    return true;
}

// ��������
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