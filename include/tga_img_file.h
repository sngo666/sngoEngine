#ifndef TGA_IMA_FILE_H
#define TGA_IMA_FILE_H
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "fmt/core.h"

namespace TgaImage {

#pragma pack(push, 1)
struct TGAHeader {
  std::uint8_t id_length = 0;
  std::uint8_t color_map_type = 0;
  std::uint8_t data_type_code = 0;
  std::uint16_t color_map_origin = 0;
  std::uint16_t color_map_length = 0;
  std::uint8_t color_map_depth = 0;
  std::uint16_t x_origin = 0;
  std::uint16_t y_origin = 0;
  std::uint16_t width = 0;
  std::uint16_t height = 0;
  std::uint8_t bits_per_pixel = 0;
  std::uint8_t image_descriptor = 0;
};
#pragma pack(pop)

struct Point {
  int x;
  int y;
};

struct TGA_color {
  std::uint8_t& operator[](const int i) {
    return bgra.at(i);
  }
  TGA_color() = default;
  explicit TGA_color(std::array<uint8_t, 4> _bgra, uint8_t _bpp = 4) : bgra(_bgra), bpp(_bpp){};
  std::array<uint8_t, 4> bgra;
  uint8_t bpp;

 private:
};

struct TGA_image {
  enum FORMAT { GRAY = 1, RGB = 3, RGBA = 4 };
  explicit TGA_image(int w, int h, int bpp);
  TGA_image() = delete;

  [[nodiscard]] bool read_tga_file(const std::string& filepath);
  bool load_rle_data(std::ifstream& in);
  void flip_vertically();
  void flip_horizontally();
  [[nodiscard]] int read_width() const;
  [[nodiscard]] int read_height() const;
  [[nodiscard]] TGA_color get_pxel_color(const Point& pos) const;
  bool set_pixel_color(Point pos, const TGA_color& color);
  bool write_tga_file(const std::string& filename, bool vflip, bool rle);
  bool unload_rle_data(std::ofstream& out);

 private:
  int width{};
  int height{};
  std::uint8_t bytes_per_pixel{};
  std::vector<char> data;
};

bool static write_pure_tga_pic(const std::string& filepath, TGA_color color, bool rle, int bpp, int width, int height) {
  constexpr std::array<uint8_t, 4> developer_area_ref{0, 0, 0, 0};
  constexpr std::array<uint8_t, 4> extension_area_ref{0, 0, 0, 0};
  constexpr std::array<uint8_t, 18> footer{'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O',
                                           'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0'};

  std::ofstream out;
  out.open(filepath, std::ios::binary);

  if (!out.is_open()) {
    std::cerr << fmt::format("can't open file {}\n", filepath);
    return false;
  }

  TGAHeader header{};
  header.bits_per_pixel = bpp << 3;
  header.width = width;
  header.height = height;
  header.data_type_code = rle ? 10 : 2;
  header.image_descriptor = 0x20;

  out.write(reinterpret_cast<const char*>(&header), sizeof(header));
  if (!out.good()) {
    std::cerr << "can't dump the tga file\n";
    return false;
  }

  if (!rle) {
    for (int i = 0; i < width * height; i++) {
      out.write(reinterpret_cast<const char*>(color.bgra.data()), bpp);
      if (!out.good()) {
        std::cerr << "can't unload raw data\n";
        return false;
      }
    }
  } else {
    int count = (width * height) / 128;
    for (int i = 0; i < count; i++) {
      out.put(static_cast<char>(255));
      out.write(reinterpret_cast<const char*>(color.bgra.data()), bpp);
      if (!out.good()) {
        std::cerr << "can't unload raw data\n";
        return false;
      }
    }

    uint8_t rest = (width * height) % 128;
    if (rest > 0) {
      out.put(static_cast<char>(rest + 127));
      out.write(reinterpret_cast<const char*>(color.bgra.data()), bpp);
      if (!out.good()) {
        std::cerr << "can't unload raw data\n";
        return false;
      }
    }
  }
  out.write(reinterpret_cast<const char*>(developer_area_ref.data()), sizeof(developer_area_ref));
  if (!out.good()) {
    std::cerr << "can't dump the tga file\n";
    return false;
  }
  out.write(reinterpret_cast<const char*>(extension_area_ref.data()), sizeof(extension_area_ref));
  if (!out.good()) {
    std::cerr << "can't dump the tga file\n";
    return false;
  }
  out.write(reinterpret_cast<const char*>(footer.data()), sizeof(footer));
  if (!out.good()) {
    std::cerr << "can't dump the tga file\n";
    return false;
  }
  return true;
}

template <size_t N>
TGA_color get_TGAColor_byRGBinput(std::array<uint8_t, N> rgb_colors) {
  TGA_color res{{0x00, 0x00, 0x00, 0x00}, 4};

  if (N < 3) {
    std::cerr << fmt::format("[err] in func get_TGAColor_byRGBinput: too few colors in array, N: {}", N);
    return res;
  }

  if (N > 4) {
    std::cerr << fmt::format("[warn] in func get_TGAColor_byRGBinput: too many colors in array, N: {}", N);
  }

  if (N == 3) {
    res.bgra[0] = rgb_colors[2];
    res.bgra[1] = rgb_colors[1];
    res.bgra[2] = rgb_colors[0];
    res.bpp = 3;
  } else {
    res.bgra[0] = rgb_colors[2];
    res.bgra[1] = rgb_colors[1];
    res.bgra[2] = rgb_colors[0];
    res.bgra[3] = rgb_colors[3];
  }

  return res;
}
}  // namespace TgaImage

#endif