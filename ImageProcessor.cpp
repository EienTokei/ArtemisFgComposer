#include "ImageProcessor.h"
#include <png.h>
#include <fstream>
#include <csetjmp>
#include <cstring>
#include <algorithm>

// PNG�ļ�ǩ��
constexpr size_t PNG_SIGNATURE_SIZE = 8;

bool ImageProcessor::LoadPng(const std::string& filePath, ImageData& imageData) {
    // ���ļ�
    FILE* file;
    errno_t err= fopen_s(&file, filePath.c_str(), "rb");
    if (!file || err != 0) {
        Logger::Error("�޷���PNG�ļ�: " + filePath);
        return false;
    }

    // ���PNGǩ��
    png_byte signature[PNG_SIGNATURE_SIZE];
    if (fread(signature, 1, PNG_SIGNATURE_SIZE, file) != PNG_SIGNATURE_SIZE ||
        png_sig_cmp(signature, 0, PNG_SIGNATURE_SIZE) != 0) {
        fclose(file);
        Logger::Error("�ļ�������Ч��PNG��ʽ: " + filePath);
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngRead(pngPtr, infoPtr)) {
        fclose(file);
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngRead(pngPtr, infoPtr);
        fclose(file);
        return false;
    }

    // �����ļ�����
    png_init_io(pngPtr, file);
    png_set_sig_bytes(pngPtr, PNG_SIGNATURE_SIZE);

    // ��ȡPNG��Ϣ
    png_read_info(pngPtr, infoPtr);

    // ��ȡͼ����Ϣ
    png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
    png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
    png_byte colorType = png_get_color_type(pngPtr, infoPtr);
    png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);

    // ת��Ϊ8λ���
    if (bitDepth == 16) {
        png_set_strip_16(pngPtr);
    }

    // �����ɫ��ͼ��
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(pngPtr);
    }

    // ����Ҷ�ͼ��
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    }

    // ���͸����ͨ��
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(pngPtr);
    }

    // ���������ʽΪRGBA
    if (colorType == PNG_COLOR_TYPE_RGB ||
        colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(pngPtr);
    }

    // ������Ϣ
    png_read_update_info(pngPtr, infoPtr);

    // ��ȡ���º����Ϣ
    width = png_get_image_width(pngPtr, infoPtr);
    height = png_get_image_height(pngPtr, infoPtr);
    colorType = png_get_color_type(pngPtr, infoPtr);

    // ȷ��ͨ����
    int channels = 4; // Ĭ��RGBA

    // ����ͼ������
    imageData.width = static_cast<int>(width);
    imageData.height = static_cast<int>(height);
    imageData.channels = channels;
    imageData.data.resize(width * height * channels);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[height];
    for (png_uint_32 y = 0; y < height; y++) {
        rowPointers[y] = &imageData.data[y * width * channels];
    }

    // ��ȡͼ������
    png_read_image(pngPtr, rowPointers);

    // ��ȡ����
    png_read_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngRead(pngPtr, infoPtr);
    fclose(file);

    Logger::Debug("�ɹ�����PNGͼ��: " + filePath +
        " (" + std::to_string(width) + "x" +
        std::to_string(height) + ")");

    return true;
}

bool ImageProcessor::LoadPngWithPos(const std::string& filePath, ImageData& imageData) {
    // ���ļ�
    FILE* file;
    errno_t err = fopen_s(&file, filePath.c_str(), "rb");
    if (!file || err != 0) {
        Logger::Error("�޷���PNG�ļ�: " + filePath);
        return false;
    }

    // ���PNGǩ��
    png_byte signature[PNG_SIGNATURE_SIZE];
    if (fread(signature, 1, PNG_SIGNATURE_SIZE, file) != PNG_SIGNATURE_SIZE ||
        png_sig_cmp(signature, 0, PNG_SIGNATURE_SIZE) != 0) {
        fclose(file);
        Logger::Error("�ļ�������Ч��PNG��ʽ: " + filePath);
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngRead(pngPtr, infoPtr)) {
        fclose(file);
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngRead(pngPtr, infoPtr);
        fclose(file);
        return false;
    }

    // �����ļ�����
    png_init_io(pngPtr, file);
    png_set_sig_bytes(pngPtr, PNG_SIGNATURE_SIZE);

    // ��ȡPNG��Ϣ
    png_read_info(pngPtr, infoPtr);

    // ��ȡtEXt��:tEXtcomment?pos,209,511,232,192
    png_textp text_ptr = nullptr;
    int num_text = 0;
    int x = 0, y = 0;

    png_get_text(pngPtr, infoPtr, &text_ptr, &num_text);
    if (num_text > 0) {
        for (int i = 0; i < num_text; i++) {
            if (strcmp(text_ptr[i].key, "comment") == 0) {
                if (sscanf_s(text_ptr[i].text, "pos,%d,%d", &x, &y) == 2) {
                    break;
                }
            }
        }
    }
    Logger::Info("tEXt���е�λ����Ϣ: " + std::to_string(x) + "," + std::to_string(y));


    // ��ȡͼ����Ϣ
    png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
    png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
    png_byte colorType = png_get_color_type(pngPtr, infoPtr);
    png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);

    // ת��Ϊ8λ���
    if (bitDepth == 16) {
        png_set_strip_16(pngPtr);
    }

    // �����ɫ��ͼ��
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(pngPtr);
    }

    // ����Ҷ�ͼ��
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    }

    // ���͸����ͨ��
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(pngPtr);
    }

    // ���������ʽΪRGBA
    if (colorType == PNG_COLOR_TYPE_RGB ||
        colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(pngPtr);
    }

    // ������Ϣ
    png_read_update_info(pngPtr, infoPtr);

    // ��ȡ���º����Ϣ
    width = png_get_image_width(pngPtr, infoPtr);
    height = png_get_image_height(pngPtr, infoPtr);
    colorType = png_get_color_type(pngPtr, infoPtr);

    // ȷ��ͨ����
    int channels = 4; // Ĭ��RGBA

    // ����ͼ������
    imageData.posX = x;
    imageData.posY = y;
    imageData.width = static_cast<int>(width);
    imageData.height = static_cast<int>(height);
    imageData.channels = channels;
    imageData.data.resize(width * height * channels);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[height];
    for (png_uint_32 y = 0; y < height; y++) {
        rowPointers[y] = &imageData.data[y * width * channels];
    }

    // ��ȡͼ������
    png_read_image(pngPtr, rowPointers);

    // ��ȡ����
    png_read_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngRead(pngPtr, infoPtr);
    fclose(file);

    Logger::Debug("�ɹ�����PNGͼ��: " + filePath +
        " (" + std::to_string(width) + "x" +
        std::to_string(height) + ")");

    return true;
}

bool ImageProcessor::LoadPngFromMemory(const uint8_t* pngData, size_t dataSize, ImageData& imageData) {
    if (!pngData || dataSize < PNG_SIGNATURE_SIZE) {
        Logger::Error("��Ч��PNG����");
        return false;
    }

    // ���PNGǩ��
    if (png_sig_cmp(pngData, 0, PNG_SIGNATURE_SIZE) != 0) {
        Logger::Error("�ڴ����ݲ�����Ч��PNG��ʽ");
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngRead(pngPtr, infoPtr)) {
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngRead(pngPtr, infoPtr);
        return false;
    }

    // �����ڴ��ȡ�ṹ
    struct PngMemoryReader {
        const uint8_t* data;
        size_t size;
        size_t offset;
    };

    PngMemoryReader reader;
    reader.data = pngData;
    reader.size = dataSize;
    reader.offset = PNG_SIGNATURE_SIZE; // ����ǩ��

    // ���ö�ȡ����
    png_set_read_fn(pngPtr, &reader, [](png_structp pngPtr, png_bytep data, png_size_t length) {
        PngMemoryReader* reader = static_cast<PngMemoryReader*>(png_get_io_ptr(pngPtr));

        if (reader->offset + length > reader->size) {
            png_error(pngPtr, "��ȡ�����ڴ淶Χ");
            return;
        }

        memcpy(data, reader->data + reader->offset, length);
        reader->offset += length;
        });

    // ����ǩ���ֽ�
    png_set_sig_bytes(pngPtr, PNG_SIGNATURE_SIZE);

    // ��ȡPNG��Ϣ
    png_read_info(pngPtr, infoPtr);

    // ��ȡͼ����Ϣ
    png_uint_32 width = png_get_image_width(pngPtr, infoPtr);
    png_uint_32 height = png_get_image_height(pngPtr, infoPtr);
    png_byte colorType = png_get_color_type(pngPtr, infoPtr);
    png_byte bitDepth = png_get_bit_depth(pngPtr, infoPtr);

    // ת��Ϊ8λ���
    if (bitDepth == 16) {
        png_set_strip_16(pngPtr);
    }

    // �����ɫ��ͼ��
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(pngPtr);
    }

    // ����Ҷ�ͼ��
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    }

    // ���͸����ͨ��
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(pngPtr);
    }

    // ���������ʽΪRGBA
    if (colorType == PNG_COLOR_TYPE_RGB ||
        colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_add_alpha(pngPtr, 0xFF, PNG_FILLER_AFTER);
    }

    if (colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(pngPtr);
    }

    // ������Ϣ
    png_read_update_info(pngPtr, infoPtr);

    // ��ȡ���º����Ϣ
    width = png_get_image_width(pngPtr, infoPtr);
    height = png_get_image_height(pngPtr, infoPtr);

    // ����ͼ������
    imageData.width = static_cast<int>(width);
    imageData.height = static_cast<int>(height);
    imageData.channels = 4; // RGBA
    imageData.data.resize(width * height * 4);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[height];
    for (png_uint_32 y = 0; y < height; y++) {
        rowPointers[y] = &imageData.data[y * width * 4];
    }

    // ��ȡͼ������
    png_read_image(pngPtr, rowPointers);

    // ��ȡ����
    png_read_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngRead(pngPtr, infoPtr);

    Logger::Debug("�ɹ����ڴ����PNGͼ�� (" +
        std::to_string(width) + "x" +
        std::to_string(height) + ")");

    return true;
}

bool ImageProcessor::SavePng(const std::string& filePath, const ImageData& imageData) {
    if (!IsValid(imageData)) {
        Logger::Error("��Ч��ͼ������");
        return false;
    }

    // ���ļ�
    FILE* file;
    errno_t err = fopen_s(&file, filePath.c_str(), "wb");
    if (!file) {
        Logger::Error("�޷�����PNG�ļ�: " + filePath);
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngWrite(pngPtr, infoPtr)) {
        fclose(file);
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngWrite(pngPtr, infoPtr);
        fclose(file);
        return false;
    }

    // �����ļ����
    png_init_io(pngPtr, file);

    // ����PNG��Ϣ
    int colorType = PNG_COLOR_TYPE_RGBA;
    if (imageData.channels == 3) {
        colorType = PNG_COLOR_TYPE_RGB;
    }

    png_set_IHDR(pngPtr, infoPtr, imageData.width, imageData.height,
        8, colorType, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // д����Ϣ
    png_write_info(pngPtr, infoPtr);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[imageData.height];
    for (int y = 0; y < imageData.height; y++) {
        rowPointers[y] = const_cast<png_bytep>(&imageData.data[y * imageData.width * imageData.channels]);
    }

    // д��ͼ������
    png_write_image(pngPtr, rowPointers);

    // д�����
    png_write_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngWrite(pngPtr, infoPtr);
    fclose(file);

    Logger::Debug("�ɹ�����PNGͼ��: " + filePath);
    return true;
}

bool ImageProcessor::SavePngWithPos(const std::string& filePath, const ImageData& imageData) {
    if (!IsValid(imageData)) {
        Logger::Error("��Ч��ͼ������");
        return false;
    }

    // ���ļ�
    FILE* file;
    errno_t err = fopen_s(&file, filePath.c_str(), "wb");
    if (!file) {
        Logger::Error("�޷�����PNG�ļ�: " + filePath);
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngWrite(pngPtr, infoPtr)) {
        fclose(file);
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngWrite(pngPtr, infoPtr);
        fclose(file);
        return false;
    }

    // �����ļ����
    png_init_io(pngPtr, file);

    // ����PNG��Ϣ
    int colorType = PNG_COLOR_TYPE_RGBA;
    if (imageData.channels == 3) {
        colorType = PNG_COLOR_TYPE_RGB;
    }

    png_set_IHDR(pngPtr, infoPtr, imageData.width, imageData.height,
        8, colorType, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // д��������Ϣ:tEXtcomment?pos,209,511,232,192
    char key[] = "comment";
    std::string posStr = "pos," + std::to_string(imageData.posX) + "," + std::to_string(imageData.posY) + "," +
        std::to_string(imageData.posX + imageData.width) + "," + std::to_string(imageData.posY + imageData.height);
    png_text text;
    text.compression = PNG_TEXT_COMPRESSION_NONE;
    text.key = key;
    text.text = const_cast<char*>(posStr.c_str());
    text.text_length = posStr.length();
    png_set_text(pngPtr, infoPtr, &text, 1);

    // д����Ϣ
    png_write_info(pngPtr, infoPtr);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[imageData.height];
    for (int y = 0; y < imageData.height; y++) {
        rowPointers[y] = const_cast<png_bytep>(&imageData.data[y * imageData.width * imageData.channels]);
    }

    // д��ͼ������
    png_write_image(pngPtr, rowPointers);

    // д�����
    png_write_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngWrite(pngPtr, infoPtr);
    fclose(file);

    Logger::Debug("�ɹ�����PNGͼ������: " + filePath);
    return true;
}

bool ImageProcessor::EncodePng(const ImageData& imageData, std::vector<uint8_t>& pngData) {
    if (!IsValid(imageData)) {
        Logger::Error("��Ч��ͼ������");
        return false;
    }

    // ��ʼ��libpng�ṹ
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    if (!InitPngWrite(pngPtr, infoPtr)) {
        return false;
    }

    // ���ô�����
    if (setjmp(png_jmpbuf(pngPtr))) {
        CleanupPngWrite(pngPtr, infoPtr);
        return false;
    }

    // �����ڴ�д��ṹ
    struct PngMemoryWriter {
        std::vector<uint8_t>* data;
    };

    PngMemoryWriter writer;
    writer.data = &pngData;

    // ����д�뺯��
    png_set_write_fn(pngPtr, &writer, [](png_structp pngPtr, png_bytep data, png_size_t length) {
        PngMemoryWriter* writer = static_cast<PngMemoryWriter*>(png_get_io_ptr(pngPtr));
        size_t oldSize = writer->data->size();
        writer->data->resize(oldSize + length);
        memcpy(writer->data->data() + oldSize, data, length);
        }, nullptr);

    // ����PNG��Ϣ
    int colorType = PNG_COLOR_TYPE_RGBA;
    if (imageData.channels == 3) {
        colorType = PNG_COLOR_TYPE_RGB;
    }

    png_set_IHDR(pngPtr, infoPtr, imageData.width, imageData.height,
        8, colorType, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // д����Ϣ
    png_write_info(pngPtr, infoPtr);

    // ������ָ��
    png_bytep* rowPointers = new png_bytep[imageData.height];
    for (int y = 0; y < imageData.height; y++) {
        rowPointers[y] = const_cast<png_bytep>(&imageData.data[y * imageData.width * imageData.channels]);
    }

    // д��ͼ������
    png_write_image(pngPtr, rowPointers);

    // д�����
    png_write_end(pngPtr, nullptr);

    // ����
    delete[] rowPointers;
    CleanupPngWrite(pngPtr, infoPtr);

    Logger::Debug("�ɹ�����PNGͼ���ڴ� (" +
        std::to_string(pngData.size()) + " �ֽ�)");

    return true;
}

ImageData ImageProcessor::CreateImage(int width, int height, int channels, uint32_t fillColor) {
    if (width <= 0 || height <= 0 || (channels != 3 && channels != 4)) {
        Logger::Error("��Ч��ͼ�����");
        return ImageData();
    }

    ImageData image(width, height, channels);

    // ��ȡ��ɫ����
    uint8_t r = (fillColor >> 24) & 0xFF;
    uint8_t g = (fillColor >> 16) & 0xFF;
    uint8_t b = (fillColor >> 8) & 0xFF;
    uint8_t a = fillColor & 0xFF;

    // ���ͼ��
    for (int i = 0; i < width * height * channels; i += channels) {
        image.data[i] = r;
        image.data[i + 1] = g;
        image.data[i + 2] = b;

        if (channels == 4) {
            image.data[i + 3] = a;
        }
    }

    return image;
}

bool ImageProcessor::CopyImageRegion(const ImageData& source, ImageData& dest,
    int srcX, int srcY, int destX, int destY,
    int width, int height) {
    if (!IsValid(source) || !IsValid(dest)) {
        Logger::Error("��Ч��ͼ������");
        return false;
    }

    // ���Դ�����Ƿ���Ч
    if (srcX < 0 || srcY < 0 ||
        srcX + width > source.width ||
        srcY + height > source.height) {
        Logger::Error("Դͼ�����򳬳���Χ");
        return false;
    }

    // ���Ŀ�������Ƿ���Ч
    if (destX < 0 || destY < 0 ||
        destX + width > dest.width ||
        destY + height > dest.height) {
        Logger::Error("Ŀ��ͼ�����򳬳���Χ");
        return false;
    }

    // ���ͨ�����Ƿ�ƥ��
    if (source.channels != dest.channels) {
        Logger::Error("Դ��Ŀ��ͼ��ͨ������ƥ��");
        return false;
    }

    // ���и���
    for (int y = 0; y < height; y++) {
        int srcOffset = ((srcY + y) * source.width + srcX) * source.channels;
        int destOffset = ((destY + y) * dest.width + destX) * dest.channels;

        memcpy(&dest.data[destOffset], &source.data[srcOffset], width * source.channels);
    }

    return true;
}

ImageData ImageProcessor::Blend(const ImageData& bg, const ImageData& fg, int x, int y) {
    if (!IsValid(bg)) {
        Logger::Error("��Ч�ı���ͼ������");
        return ImageData();
    }
    if (!IsValid(fg)) {
        Logger::Error("��Ч��ǰ��ͼ������");
        return ImageData();
    }
    if (bg.channels != 4 || fg.channels != 4) {
        Logger::Error("ͼ��ͨ������ƥ��");
        return ImageData();
    }
    // ����
    ImageData result = bg;

    if (x == 0 && y == 0) {
        x = fg.posX - bg.posX;
        y = fg.posY - bg.posY;
    }

    // ����������չ����
    if (x < 0) {
        result.data.insert(result.data.begin(), -x * result.channels, 0);
        result.width += -x;
        x = 0;
    }
    if (y < 0) {
        result.data.insert(result.data.begin(), -y * result.width * result.channels, 0);
        result.height += -y;
        y = 0;
    }
    if (x + fg.width > result.width) {
        result.data.insert(result.data.end(), (x + fg.width - result.width) * result.channels, 0);
        result.width = x + fg.width;
    }
    if (y + fg.height > result.height) {
        result.data.insert(result.data.end(), (y + fg.height - result.height) * result.width * result.channels, 0);
        result.height = y + fg.height;
    }

    // ���»��ͼƬ������
    result.posX = std::min(bg.posX, fg.posX);
    result.posY = std::min(bg.posY, fg.posY);
    // ���»��ͼƬ�ĳߴ�
    result.width = std::max(bg.posX + bg.width, fg.posX + fg.width) - result.posX;
    result.height = std::max(bg.posY + bg.height, fg.posY + fg.height) - result.posY;

    // �����ػ��
    for (int i = 0; i < fg.height; i++) {
        for (int j = 0; j < fg.width; j++) {
            int destX = x + j;
            int destY = y + i;
            uint8_t r, g, b, a;
            if (GetPixel(fg, j, i, r, g, b, a)) {
                // ���
                uint8_t& dr = result.data[(destY * result.width + destX) * result.channels];
                uint8_t& dg = result.data[(destY * result.width + destX) * result.channels + 1];
                uint8_t& db = result.data[(destY * result.width + destX) * result.channels + 2];
                uint8_t& da = result.data[(destY * result.width + destX) * result.channels + 3];
                dr = (r * a + dr * (255 - a)) / 255;
                dg = (g * a + dg * (255 - a)) / 255;
                db = (b * a + db * (255 - a)) / 255;
                da = a + da * (255 - a) / 255;
            }
        }
    }
    return result;
}

bool ImageProcessor::IsCoordinateValid(const ImageData& image, int x, int y) {
    return x >= 0 && x < image.width && y >= 0 && y < image.height;
}

uint8_t* ImageProcessor::GetPixelPtr(ImageData& image, int x, int y) {
    if (!IsCoordinateValid(image, x, y)) {
        return nullptr;
    }

    return &image.data[(y * image.width + x) * image.channels];
}

const uint8_t* ImageProcessor::GetPixelPtr(const ImageData& image, int x, int y) {
    if (!IsCoordinateValid(image, x, y)) {
        return nullptr;
    }

    return &image.data[(y * image.width + x) * image.channels];
}

bool ImageProcessor::SetPixel(ImageData& image, int x, int y,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t* pixel = GetPixelPtr(image, x, y);
    if (!pixel) {
        return false;
    }

    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;

    if (image.channels == 4) {
        pixel[3] = a;
    }

    return true;
}

bool ImageProcessor::GetPixel(const ImageData& image, int x, int y,
    uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    const uint8_t* pixel = GetPixelPtr(image, x, y);
    if (!pixel) {
        return false;
    }

    r = pixel[0];
    g = pixel[1];
    b = pixel[2];

    if (image.channels == 4) {
        a = pixel[3];
    }
    else {
        a = 255;
    }

    return true;
}

ImageData ImageProcessor::ConvertToRGBA(const ImageData& image) {
    if (!IsValid(image)) {
        Logger::Error("��Ч��ͼ������");
        return ImageData();
    }

    if (image.channels == 4) {
        // �Ѿ���RGBA��ʽ��ֱ�ӷ��ظ���
        ImageData result = image;
        return result;
    }

    if (image.channels != 3) {
        Logger::Error("��֧�ֵ�ͼ���ʽ");
        return ImageData();
    }

    // ת��ΪRGBA
    ImageData result(image.width, image.height, 4);

    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            const uint8_t* srcPixel = GetPixelPtr(image, x, y);
            uint8_t* destPixel = GetPixelPtr(result, x, y);

            if (srcPixel && destPixel) {
                destPixel[0] = srcPixel[0]; // R
                destPixel[1] = srcPixel[1]; // G
                destPixel[2] = srcPixel[2]; // B
                destPixel[3] = 255;         // A
            }
        }
    }

    return result;
}

bool ImageProcessor::IsValid(const ImageData& image) {
    return image.width > 0 &&
        image.height > 0 &&
        (image.channels == 3 || image.channels == 4) &&
        image.data.size() == static_cast<size_t>(image.width * image.height * image.channels);
}

void ImageProcessor::FreeImage(ImageData& image) {
    image.width = 0;
    image.height = 0;
    image.channels = 0;
    image.data.clear();
    image.data.shrink_to_fit();
}

void ImageProcessor::PngErrorHandler(png_structp png_ptr, png_const_charp error_msg) {
    Logger::Error("libpng����: " + std::string(error_msg));
    longjmp(png_jmpbuf(png_ptr), 1);
}

void ImageProcessor::PngWarningHandler(png_structp png_ptr, png_const_charp warning_msg) {
    Logger::Warning("libpng����: " + std::string(warning_msg));
}

bool ImageProcessor::InitPngRead(png_structp& pngPtr, png_infop& infoPtr) {
    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, PngErrorHandler, PngWarningHandler);
    if (!pngPtr) {
        Logger::Error("�޷�����PNG��ȡ�ṹ");
        return false;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_read_struct(&pngPtr, nullptr, nullptr);
        Logger::Error("�޷�����PNG��Ϣ�ṹ");
        return false;
    }

    return true;
}

bool ImageProcessor::InitPngWrite(png_structp& pngPtr, png_infop& infoPtr) {
    pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, PngErrorHandler, PngWarningHandler);
    if (!pngPtr) {
        Logger::Error("�޷�����PNGд��ṹ");
        return false;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_write_struct(&pngPtr, nullptr);
        Logger::Error("�޷�����PNG��Ϣ�ṹ");
        return false;
    }

    return true;
}

void ImageProcessor::CleanupPngRead(png_structp pngPtr, png_infop infoPtr, png_bytep* rowPointers) {
    if (rowPointers) {
        delete[] rowPointers;
    }

    if (pngPtr && infoPtr) {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
    }
    else if (pngPtr) {
        png_destroy_read_struct(&pngPtr, nullptr, nullptr);
    }
}

void ImageProcessor::CleanupPngWrite(png_structp pngPtr, png_infop infoPtr) {
    if (pngPtr && infoPtr) {
        png_destroy_write_struct(&pngPtr, &infoPtr);
    }
    else if (pngPtr) {
        png_destroy_write_struct(&pngPtr, nullptr);
    }
}