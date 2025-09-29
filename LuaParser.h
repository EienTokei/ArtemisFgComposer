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
    * @brief ����Lua�ļ���״̬��
    * @param path Lua�ļ�·��
    * @return bool:�Ƿ���سɹ�
    */
    bool loadLuaFile(const std::string& path);

    /**
     * @brief ����Lua�ļ���fgpos��
     * @return bool:�Ƿ�����ɹ�
     */
    bool parseFgPos();

    /**
     * @brief ����Lua�ļ����ض���
     * @param group ����
     * @return bool:�Ƿ�����ɹ�
     */
    bool parseGroup(const std::string& group);

    /**
     * @brief �������ƥ������һ��ӳ���
     * @param field ƥ���ֶ�
     * @return bool:�Ƿ�����ɹ�
     */
    bool parseGroups(const std::string& field);

    /**
     * @brief ��ȡ�����������
     * @param group ����
     * @return const PosMap*:����ӳ���
     */
    const PosMap* getGroupPos(const std::string& group) const;

    /**
     * @brief ��ȡ�ض�����ض�����
     * @param group ����
     * @param file �ļ���
     * @return pair<int, int>:����
     */
    const std::pair<int, int> getFilePos(const std::string& group, const std::string& file) const;

    /**
     * @brief ������Ƿ����
     * @param group ����
     * @return bool:�Ƿ����
     */
    bool hasGroup(const std::string& group) const;

    /**
     * @brief ����ض��ļ��Ƿ����
     * @param group ����
     * @param file �ļ���
     * @return bool:�Ƿ����
     */
    bool hasFile(const std::string& group, const std::string& file) const;

    /**
     * @brief ��ȡ�����������б�
     * @return std::vector<std::string>:�����б�
     */
    std::vector<std::string> getGroupNames() const;

    /**
     * @brief ��ȡ�ض����������ļ������б�
     * @param group ����
     * @return std::vector<std::string>:�ļ����б�
     */
    std::vector<std::string> getFileNames(const std::string& group) const;

    /**
     * @brief �������꣨���ڸ��£�
     * @param group ����
     * @param file �ļ���
     * @param pos ����
     * @note ����ļ������ڣ����Զ�����
     */
    void setFilePos(const std::string& group, const std::string& file, const Pos& pos);

    /**
     * @brief �����޸Ļ�Lua�ļ�
     * @param path Lua�ļ�·��
     * @return bool:�Ƿ񱣴�ɹ�
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



