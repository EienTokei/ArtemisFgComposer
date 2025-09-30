#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <png.h>
#include "Config.h"

// ͼ�����ݽṹ
struct ImageData {
    int posX, posY;             // λ��
    int width;                  // ͼ����
    int height;                 // ͼ��߶�
    int channels;               // ͨ���� (3-RGB, 4-RGBA)
    std::vector<uint8_t> data;  // ͼ����������

    ImageData() : width(0), height(0), channels(0), posX(0), posY(0) {}
    ImageData(int w, int h, int c, int x = 0, int y = 0) : width(w), height(h), channels(c), posX(x), posY(y) {
        data.resize(w * h * c);
    }
};

class ImageProcessor {
public:
    /**
     * @brief ����PNGͼ���ļ�
     * @param filePath PNG�ļ�·��
     * @param imageData �����ͼ������
     * @return �ɹ����ط���true�����򷵻�false
     */
    static bool LoadPng(const std::string& filePath, ImageData& imageData);

    /**
     * @brief ����PNGͼ���ļ���������Ϣ
     * @param filePath PNG�ļ�·��
     * @param imageData �����������Ϣ��ͼ������
     * @return �ɹ����ط���true�����򷵻�false
     */
    static bool LoadPngWithPos(const std::string& filePath, ImageData& imageData);

    /**
     * @brief ���ڴ����ݼ���PNGͼ��
     * @param pngData PNG����ָ��
     * @param dataSize ���ݴ�С
     * @param imageData �����ͼ������
     * @return �ɹ����ط���true�����򷵻�false
     */
    static bool LoadPngFromMemory(const uint8_t* pngData, size_t dataSize, ImageData& imageData);

    /**
     * @brief ����ͼ��ΪPNG�ļ�
     * @param filePath ����ļ�·��
     * @param imageData ͼ������
     * @return �ɹ����淵��true�����򷵻�false
     */
    static bool SavePng(const std::string& filePath, const ImageData& imageData);

    /**
     * @brief ����ͼ��ΪPNG�ļ�����д��������Ϣ
     * @param filePath ����ļ�·��
     * @param imageData ͼ������
     * @return �ɹ����淵��true�����򷵻�false
     */
    static bool SavePngWithPos(const std::string& filePath, const ImageData& imageData);

    /**
     * @brief ��ͼ�����ΪPNG��ʽ���ڴ�
     * @param imageData ͼ������
     * @param pngData �����PNG����
     * @return �ɹ����뷵��true�����򷵻�false
     */
    static bool EncodePng(const ImageData& imageData, std::vector<uint8_t>& pngData);

    /**
     * @brief ����ָ����С�Ŀհ�ͼ��
     * @param width ͼ����
     * @param height ͼ��߶�
     * @param channels ͨ���� (3��4)
     * @param fillColor �����ɫ (RGBA��ʽ��Ĭ��͸��)
     * @return ������ͼ������
     */
    static ImageData CreateImage(int width, int height, int channels = 4, uint32_t fillColor = 0x00000000);

    /**
     * @brief ����ͼ������
     * @param source Դͼ��
     * @param dest Ŀ��ͼ��
     * @param srcX Դͼ���е�X����
     * @param srcY Դͼ���е�Y����
     * @param destX Ŀ��ͼ���е�X����
     * @param destY Ŀ��ͼ���е�Y����
     * @param width Ҫ���ƵĿ��
     * @param height Ҫ���Ƶĸ߶�
     * @return �ɹ����Ʒ���true�����򷵻�false
     */
    static bool CopyImageRegion(const ImageData& source, ImageData& dest,
        int srcX, int srcY, int destX, int destY,
        int width, int height);

    /**
     * @brief ͼ����ӻ��, ����������
     * @param bg ����ͼ��
     * @param fg ǰ��ͼ��
     * @param x X����
     * @param y Y����
     * @return �ϲ����ͼ������
     */
    static ImageData Blend(const ImageData& bg, const ImageData& fg, int x = 0, int y = 0);

    /**
     * @brief ��������Ƿ���ͼ��Χ��
     * @param image ͼ������
     * @param x X����
     * @param y Y����
     * @return �ڷ�Χ�ڷ���true�����򷵻�false
     */
    static bool IsPosValid(const ImageData& image, int x, int y);

    /**
     * @brief ��ȡ����ֵ��ָ��
     * @param image ͼ������
     * @param x X����
     * @param y Y����
     * @return ��������ָ�룬��Ч���귵��nullptr
     */
    static uint8_t* GetPixelPtr(ImageData& image, int x, int y);

    /**
     * @brief ��ȡ����ֵ�ĳ���ָ��
     * @param image ͼ������
     * @param x X����
     * @param y Y����
     * @return ��������ָ�룬��Ч���귵��nullptr
     */
    static const uint8_t* GetPixelPtr(const ImageData& image, int x, int y);

    /**
     * @brief ��������ֵ
     * @param image ͼ������
     * @param x X����
     * @param y Y����
     * @param r ��ɫ����
     * @param g ��ɫ����
     * @param b ��ɫ����
     * @param a ͸���ȷ���
     * @return �ɹ����÷���true�����򷵻�false
     */
    static bool SetPixel(ImageData& image, int x, int y,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /**
     * @brief ��ȡ����ֵ
     * @param image ͼ������
     * @param x X����
     * @param y Y����
     * @param r ����ĺ�ɫ����
     * @param g �������ɫ����
     * @param b �������ɫ����
     * @param a �����͸���ȷ���
     * @return �ɹ���ȡ����true�����򷵻�false
     */
    static bool GetPixel(const ImageData& image, int x, int y,
        uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

    /**
     * @brief ��ͼ��ת��ΪRGBA��ʽ
     * @param image ����ͼ��
     * @return ת�����RGBAͼ��
     */
    static ImageData ConvertToRGBA(const ImageData& image);

    /**
     * @brief ���ͼ�������Ƿ���Ч
     * @param image ͼ������
     * @return ��Ч����true�����򷵻�false
     */
    static bool IsValid(const ImageData& image);

    /**
     * @brief �ͷ�ͼ����Դ
     * @param image ͼ������
     */
    static void FreeImage(ImageData& image);

private:
    // libpng����;��洦����
    static void PngErrorHandler(png_structp png_ptr, png_const_charp error_msg);
    static void PngWarningHandler(png_structp png_ptr, png_const_charp warning_msg);

    /**
     * @brief ��ʼ��libpng��ȡ�ṹ
     * @param pngPtr �����png_structָ��
     * @param infoPtr �����png_infoָ��
     * @return �ɹ���ʼ������true�����򷵻�false
     */
    static bool InitPngRead(png_structp& pngPtr, png_infop& infoPtr);

    /**
     * @brief ��ʼ��libpngд��ṹ
     * @param pngPtr �����png_structָ��
     * @param infoPtr �����png_infoָ��
     * @return �ɹ���ʼ������true�����򷵻�false
     */
    static bool InitPngWrite(png_structp& pngPtr, png_infop& infoPtr);

    /**
     * @brief ����libpng��ȡ�ṹ
     * @param pngPtr png_structָ��
     * @param infoPtr png_infoָ��
     * @param rowPointers ��ָ������
     */
    static void CleanupPngRead(png_structp pngPtr, png_infop infoPtr, png_bytep* rowPointers = nullptr);

    /**
     * @brief ����libpngд��ṹ
     * @param pngPtr png_structָ��
     * @param infoPtr png_infoָ��
     */
    static void CleanupPngWrite(png_structp pngPtr, png_infop infoPtr);
};
