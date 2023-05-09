#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <cmath>

#include "zstd.h"

using Array = std::tuple<std::unique_ptr<char[]>, size_t>;

size_t Length(const Array& a) {
    return std::get<1>(a);
}

#pragma pack(1)
struct BMPHeader {
    BMPHeader() = default;

    BMPHeader(int x, int y, unsigned int colour_depth_bytes = 4) : x(x), y(y) {
        offset = sizeof(BMPHeader);
        pixels_size = colour_depth_bytes * x * y;
        size = pixels_size + offset;
        depth = colour_depth_bytes * 8;
    }

    // header
    uint16_t id = 0x4D42;
    uint32_t size;
    uint16_t reserved0 = 0;
    uint16_t reserved1 = 0;
    uint32_t offset;
    // bitmapinfoheader
    uint32_t bitmapinfoheader_size = 40;
    int32_t x;
    int32_t y;
    uint16_t planes = 1;
    uint16_t depth;
    uint32_t compression = 0;
    uint32_t pixels_size;
    int32_t horizontal_res = 2835;
    int32_t vertical_res = 2835;
    uint32_t palette = 0;
    uint32_t important = 0;
};

std::ifstream OpenFileRead(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Couldn't open '" + path + "' for reading");

    return file;
}

std::ofstream OpenFileWrite(const std::string& path) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Couldn't open '" + path + "' for writing");

    return file;
}

template<typename T>
void Read(std::istream& in, T* t, size_t n) {
    auto p = reinterpret_cast<char*>(t);
    in.read(p, n * sizeof(T));
}

template<typename T>
T Read(std::istream& in) {
    T result;
    Read(in, &result, 1);

    return result;
}

template<typename T>
void Write(std::ostream& out, const T* t, size_t n) {
    auto p = reinterpret_cast<const char*>(t);
    out.write(p, n * sizeof(T));
}

template<typename T>
void Write(std::ostream& out, const T& t) {
    Write(out, &t, 1);
}

Array ReadArray(std::istream& in, size_t n) {
    auto data = std::make_unique_for_overwrite<char[]>(n);
    Read(in, data.get(), n);

    return Array(std::move(data), n);
}

void WriteArray(std::ostream& out, const Array& array) {
    auto& [p, n] = array;
    Write(out, p.get(), n);
}

Array ReadEntireFile(const std::string& path) {
    auto file = OpenFileRead(path);

    file.seekg(0, std::ios::end);
    uint64_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    return ReadArray(file, size);
}

Array Compress(const Array& array) {
    auto& [p, n] = array;

    auto cap = ZSTD_compressBound(n);
    auto result = std::make_unique_for_overwrite<char[]>(cap);

    auto size = ZSTD_compress(result.get(), cap, p.get(), n, 0);

    if (ZSTD_isError(size)) throw std::runtime_error(ZSTD_getErrorName(size));

    return Array(std::move(result), size);
}

Array Decompress(const Array& array, uint64_t size) {
    auto& [p, n] = array;

    auto result = std::make_unique_for_overwrite<char[]>(size);
    auto sz = ZSTD_decompress(result.get(), size, p.get(), n);

    if (ZSTD_isError(sz)) throw std::runtime_error(ZSTD_getErrorName(sz));
    if (sz != size) throw std::runtime_error("Size mismatch");

    return Array(std::move(result), size);
}

template<typename T>
T DivCeil(T a, T b) {
    return (a + b - 1) / b;
}

std::pair<int32_t, int32_t> CalculateSize(uint64_t size) {
    auto size_4 = (size + 3) / 4;
    uint64_t n = std::sqrt(size_4);

    auto w = DivCeil(size_4, n * 4) * 4;
    auto h = DivCeil(size_4, w);

    return std::pair(w, h);
}

void SaveEncodedFile(const std::string& path, uint64_t size, uint64_t compressed, const Array& data) {
    auto file = OpenFileWrite(path);

    auto n = Length(data) + 2 * sizeof(uint64_t);
    auto [w, h] = CalculateSize(n);

    BMPHeader header(w, h);
    Write(file, header);

    Write(file, size);
    Write(file, compressed);

    WriteArray(file, data);

    auto pad_n = header.pixels_size - n;
    auto padding = Array(std::make_unique<char[]>(pad_n), pad_n);

    WriteArray(file, padding);
}

void EncodeFile(const std::string& input_path, const std::string& output_path, bool compress) {
    auto data = ReadEntireFile(input_path);
    auto n = Length(data);

    if (compress) {
        auto compressed = Compress(data);
        auto c = Length(compressed);

        SaveEncodedFile(output_path, n, c, compressed);
    } else {
        SaveEncodedFile(output_path, n, 0, data);
    }
}

Array LoadEncodedFile(const std::string& path) {
    auto file = OpenFileRead(path);

    Read<BMPHeader>(file);

    auto size = Read<uint64_t>(file);
    auto compressed = Read<uint64_t>(file);

    if (compressed) {
        auto data = ReadArray(file, compressed);
        return Decompress(data, size);
    } else {
        return ReadArray(file, size);
    }
}

void DecodeFile(const std::string& input_path, const std::string& output_path) {
    auto data = LoadEncodedFile(input_path);

    auto file = OpenFileWrite(output_path);
    WriteArray(file, data);
}

void ProcessFile(const std::string& path) {
    if (path.ends_with(".bmp")) {
        DecodeFile(path, path.substr(0, path.size() - 4));
    } else {
        EncodeFile(path, path + ".bmp", true);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [options] <files...>\n";
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        ProcessFile(argv[i]);
    }
}
