#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <png.h>
#include "Config.h"

// 图像数据结构
struct ImageData {
    int posX, posY;             // 位置
    int width;                  // 图像宽度
    int height;                 // 图像高度
    int channels;               // 通道数 (3-RGB, 4-RGBA)
    std::vector<uint8_t> data;  // 图像像素数据

    ImageData() : width(0), height(0), channels(0), posX(0), posY(0) {}
    ImageData(int w, int h, int c, int x = 0, int y = 0) : width(w), height(h), channels(c), posX(x), posY(y) {
        data.resize(w * h * c);
    }
};

class ImageProcessor {
public:
    /**
     * @brief 加载PNG图像文件
     * @param filePath PNG文件路径
     * @param imageData 输出的图像数据
     * @return 成功加载返回true，否则返回false
     */
    static bool LoadPng(const std::string& filePath, ImageData& imageData);

    /**
     * @brief 加载PNG图像文件和坐标信息
     * @param filePath PNG文件路径
     * @param imageData 输出带坐标信息的图像数据
     * @return 成功加载返回true，否则返回false
     */
    static bool LoadPngWithPos(const std::string& filePath, ImageData& imageData);

    /**
     * @brief 从内存数据加载PNG图像
     * @param pngData PNG数据指针
     * @param dataSize 数据大小
     * @param imageData 输出的图像数据
     * @return 成功加载返回true，否则返回false
     */
    static bool LoadPngFromMemory(const uint8_t* pngData, size_t dataSize, ImageData& imageData);

    /**
     * @brief 保存图像为PNG文件
     * @param filePath 输出文件路径
     * @param imageData 图像数据
     * @return 成功保存返回true，否则返回false
     */
    static bool SavePng(const std::string& filePath, const ImageData& imageData);

    /**
     * @brief 保存图像为PNG文件，并写入坐标信息
     * @param filePath 输出文件路径
     * @param imageData 图像数据
     * @return 成功保存返回true，否则返回false
     */
    static bool SavePngWithPos(const std::string& filePath, const ImageData& imageData);

    /**
     * @brief 将图像编码为PNG格式到内存
     * @param imageData 图像数据
     * @param pngData 输出的PNG数据
     * @return 成功编码返回true，否则返回false
     */
    static bool EncodePng(const ImageData& imageData, std::vector<uint8_t>& pngData);

    /**
     * @brief 创建指定大小的空白图像
     * @param width 图像宽度
     * @param height 图像高度
     * @param channels 通道数 (3或4)
     * @param fillColor 填充颜色 (RGBA格式，默认透明)
     * @return 创建的图像数据
     */
    static ImageData CreateImage(int width, int height, int channels = 4, uint32_t fillColor = 0x00000000);

    /**
     * @brief 复制图像区域
     * @param source 源图像
     * @param dest 目标图像
     * @param srcX 源图像中的X坐标
     * @param srcY 源图像中的Y坐标
     * @param destX 目标图像中的X坐标
     * @param destY 目标图像中的Y坐标
     * @param width 要复制的宽度
     * @param height 要复制的高度
     * @return 成功复制返回true，否则返回false
     */
    static bool CopyImageRegion(const ImageData& source, ImageData& dest,
        int srcX, int srcY, int destX, int destY,
        int width, int height);

    /**
     * @brief 图层叠加混合, 并更新坐标
     * @param bg 背景图像
     * @param fg 前景图像
     * @param x X坐标
     * @param y Y坐标
     * @return 合并后的图像数据
     */
    static ImageData Blend(const ImageData& bg, const ImageData& fg, int x = 0, int y = 0);

    /**
     * @brief 检查坐标是否在图像范围内
     * @param image 图像数据
     * @param x X坐标
     * @param y Y坐标
     * @return 在范围内返回true，否则返回false
     */
    static bool IsPosValid(const ImageData& image, int x, int y);

    /**
     * @brief 获取像素值的指针
     * @param image 图像数据
     * @param x X坐标
     * @param y Y坐标
     * @return 像素数据指针，无效坐标返回nullptr
     */
    static uint8_t* GetPixelPtr(ImageData& image, int x, int y);

    /**
     * @brief 获取像素值的常量指针
     * @param image 图像数据
     * @param x X坐标
     * @param y Y坐标
     * @return 像素数据指针，无效坐标返回nullptr
     */
    static const uint8_t* GetPixelPtr(const ImageData& image, int x, int y);

    /**
     * @brief 设置像素值
     * @param image 图像数据
     * @param x X坐标
     * @param y Y坐标
     * @param r 红色分量
     * @param g 绿色分量
     * @param b 蓝色分量
     * @param a 透明度分量
     * @return 成功设置返回true，否则返回false
     */
    static bool SetPixel(ImageData& image, int x, int y,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /**
     * @brief 获取像素值
     * @param image 图像数据
     * @param x X坐标
     * @param y Y坐标
     * @param r 输出的红色分量
     * @param g 输出的绿色分量
     * @param b 输出的蓝色分量
     * @param a 输出的透明度分量
     * @return 成功获取返回true，否则返回false
     */
    static bool GetPixel(const ImageData& image, int x, int y,
        uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

    /**
     * @brief 将图像转换为RGBA格式
     * @param image 输入图像
     * @return 转换后的RGBA图像
     */
    static ImageData ConvertToRGBA(const ImageData& image);

    /**
     * @brief 检查图像数据是否有效
     * @param image 图像数据
     * @return 有效返回true，否则返回false
     */
    static bool IsValid(const ImageData& image);

    /**
     * @brief 释放图像资源
     * @param image 图像数据
     */
    static void FreeImage(ImageData& image);

private:
    // libpng错误和警告处理函数
    static void PngErrorHandler(png_structp png_ptr, png_const_charp error_msg);
    static void PngWarningHandler(png_structp png_ptr, png_const_charp warning_msg);

    /**
     * @brief 初始化libpng读取结构
     * @param pngPtr 输出的png_struct指针
     * @param infoPtr 输出的png_info指针
     * @return 成功初始化返回true，否则返回false
     */
    static bool InitPngRead(png_structp& pngPtr, png_infop& infoPtr);

    /**
     * @brief 初始化libpng写入结构
     * @param pngPtr 输出的png_struct指针
     * @param infoPtr 输出的png_info指针
     * @return 成功初始化返回true，否则返回false
     */
    static bool InitPngWrite(png_structp& pngPtr, png_infop& infoPtr);

    /**
     * @brief 清理libpng读取结构
     * @param pngPtr png_struct指针
     * @param infoPtr png_info指针
     * @param rowPointers 行指针数组
     */
    static void CleanupPngRead(png_structp pngPtr, png_infop infoPtr, png_bytep* rowPointers = nullptr);

    /**
     * @brief 清理libpng写入结构
     * @param pngPtr png_struct指针
     * @param infoPtr png_info指针
     */
    static void CleanupPngWrite(png_structp pngPtr, png_infop infoPtr);
};
