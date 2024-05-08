#include "tga_img_file.h"

#include <corecrt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iostream>

#include "fmt/core.h"

TgaImage::TGA_image::TGA_image(int w, int h, int b)
    : width(w), height(h), bytes_per_pixel(b), data(static_cast<int64_t>(width * height * bytes_per_pixel), 0) {}

bool TgaImage::TGA_image::read_tga_file(const std::string& filepath) {
  std::ifstream in;

  in.open(filepath, std::ios::in | std::ios::binary);
  if (!in.is_open()) {
    std::cerr << fmt::format("[err] can't open file: {}\n", filepath);
    return false;
  }
  TGAHeader header;

  in.read(reinterpret_cast<char*>(&header), sizeof(header));

  if (!in.good()) {
    std::cerr << fmt::format("[err] an error occured while reading the header\n");
    return false;
  }

  auto tga_width{header.width};
  auto tga_height{header.height};
  auto tga_bpp{header.bits_per_pixel >> 3};

  fmt::print("width: {}\n height: {}\n", tga_width, tga_height);

  if (tga_width <= 0 || tga_height <= 0
      || (bytes_per_pixel != TGA_image::GRAY && bytes_per_pixel != TGA_image::RGB && bytes_per_pixel != TGA_image::RGBA)) {
    std::cerr << fmt::format("[err] bad bpp (or width/height) value\n");
    return false;
  }

  auto bytes_num{tga_width * tga_height * tga_bpp};
  data = std::vector<char>(bytes_num, 0);

  if (3 == header.data_type_code || 2 == header.data_type_code) {
    in.read(data.data(), bytes_num);
    if (!in.good()) {
      std::cerr << fmt::format("[err] an error occured while reading the data\n");
      return false;
    }
  } else if (10 == header.data_type_code || 11 == header.data_type_code) {
    if (!load_rle_data(in)) {
      std::cerr << "an error occured while reading the data\n";
      return false;
    }
  } else {
    std::cerr << fmt::format("[err] unknown file format, tga's header.data_type_code: {}\n", header.data_type_code);
  }

  if ((header.image_descriptor & 0x20) == 0) {
  }

  return true;
}

bool TgaImage::TGA_image::load_rle_data(std::ifstream& in) {
  auto pixices = width * height;
  auto curr_bytes = 0;
  auto count = 0;

  while (pixices > count) {
    TgaImage::TGA_color color_buffer{};
    std::uint8_t chunk{0};
    chunk = in.get();

    if (!in.good()) {
      std::cerr << fmt::format("an error occured while reading the data\n");
      return false;
    }

    if (chunk < 128) {
      // highest bit is 0, it's raw packet
      chunk++;
      for (int i = 0; i < chunk; i++) {
        in.read(reinterpret_cast<char*>(color_buffer.bgra.data()), bytes_per_pixel);
        if (!in.good()) {
          std::cerr << fmt::format("an error occured while reading the data\n");
          return false;
        }
        for (int j = 0; j < bytes_per_pixel; j++) {
          data.at(curr_bytes++) = static_cast<char>(color_buffer.bgra.at(j));
        }
        count++;
        if (pixices < count) {
          std::cerr << fmt::format("Too many pixels read\n");
          return false;
        }
      }

    } else {
      // highest bit is 1, it's run-length packets
      chunk -= 127;
      in.read(reinterpret_cast<char*>(color_buffer.bgra.data()), bytes_per_pixel);

      if (!in.good()) {
        std::cerr << fmt::format("an error occured while reading the data\n");
        return false;
      }

      for (int i = 0; i < chunk; i++) {
        for (int t = 0; t < bytes_per_pixel; t++) {
          data.at(curr_bytes++) = static_cast<char>(color_buffer.bgra.at(t));
        }
        count++;
        if (pixices < count) {
          std::cerr << fmt::format("Too many pixels read\n count: {}\n", count);
          return false;
        }
      }
    }
  }

  return true;
}

bool TgaImage::TGA_image::write_tga_file(const std::string& filename, bool vflip, bool rle) {
  constexpr std::array<uint8_t, 4> developer_area_ref{0, 0, 0, 0};
  constexpr std::array<uint8_t, 4> extension_area_ref{0, 0, 0, 0};
  constexpr std::array<uint8_t, 18> footer{'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O',
                                           'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0'};

  std::ofstream out;
  out.open(filename, std::ios::binary);

  if (!out.is_open()) {
    std::cerr << fmt::format("can't open file {}\n", filename);
    return false;
  }

  TGAHeader header{};
  header.bits_per_pixel = bytes_per_pixel << 3;
  header.width = width;
  header.height = height;
  header.data_type_code = (bytes_per_pixel == GRAY ? (rle ? 11 : 3) : (rle ? 10 : 2));
  header.image_descriptor = vflip ? 0x00 : 0x20;

  out.write(reinterpret_cast<const char*>(&header), sizeof(header));

  if (!out.good()) {
    std::cerr << "can't dump the tga file\n";
    return false;
  }
  if (!rle) {
    out.write(reinterpret_cast<const char*>(data.data()), width * height * bytes_per_pixel);
    if (!out.good()) {
      std::cerr << "can't unload raw data\n";
      return false;
    }
  } else if (!unload_rle_data(out)) {
    std::cerr << "can't unload rle data\n";
    return false;
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

bool TgaImage::TGA_image::unload_rle_data(std::ofstream& out) {
  const std::uint8_t MAX_CHUNK_LENGTH = 128;
  auto npixels = width * height;
  size_t cur_pixel = 0;

  while (cur_pixel < npixels) {
    size_t chunk_start{cur_pixel * bytes_per_pixel};
    size_t cur_byte{chunk_start};

    std::uint8_t run_length = 1;
    bool RAW = true;

    while (cur_pixel + run_length < npixels && run_length < MAX_CHUNK_LENGTH) {
      bool same_char{true};
      for (int t = 0; t < bytes_per_pixel && same_char; t++) {
        same_char = {data.at(cur_byte + t) == data.at(cur_byte + t + bytes_per_pixel)};
      }
      run_length++;

      if (1 == run_length) {
        RAW = !same_char;
      }
      if (RAW && same_char) {
        run_length--;
        break;
      }
      if (!RAW && !same_char) {
        break;
      }
    }

    cur_pixel += run_length;

    out.put(RAW ? run_length - 1 : run_length + 127);
    if (!out.good()) {
      std::cerr << "can't dump the tga file\n";
      return false;
    }

    out.write(reinterpret_cast<const char*>(data.data() + chunk_start), (RAW ? run_length * bytes_per_pixel : bytes_per_pixel));

    if (!out.good()) {
      std::cerr << "can't dump the tga file\n";
      return false;
    }
  }
  return true;
}

TgaImage::TGA_color TgaImage::TGA_image::get_pxel_color(const Point& pos) const {
  if (pos.x > width || pos.y > height || pos.x < 0 || pos.y < 0) {
    std::cerr << fmt::format("wrong input pixel position: ({}, {})", pos.x, pos.y);
    return {};
  }
  TGA_color res{};

  for (int i = 0; i < bytes_per_pixel; i++) {
    res.bgra.at(i) = data.at((pos.x + pos.y * width) * bytes_per_pixel + i);
  }

  return res;
}

bool TgaImage::TGA_image::set_pixel_color(Point pos, const TGA_color& color) {
  if (pos.x > width || pos.y > height || pos.x < 0 || pos.y < 0) {
    std::cerr << fmt::format("wrong input pixel position: ({}, {})", pos.x, pos.y);
    return false;
  }
  std::copy(color.bgra.begin(), color.bgra.end(), data.begin() + static_cast<int>(bytes_per_pixel * (pos.x + pos.y * width)));
  return true;
}

void TgaImage::TGA_image::flip_vertically() {
  int half = height >> 1;
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < half; j++) {
      for (int b = 0; b < bytes_per_pixel; b++) {
        std::swap(data[(i + j * width) * bytes_per_pixel + b], data[(i + (height - 1 - j) * width) * bytes_per_pixel + b]);
      }
    }
  }
}

void TgaImage::TGA_image::flip_horizontally() {
  int half = width >> 1;
  for (int i = 0; i < half; i++) {
    for (int j = 0; j < height; j++) {
      for (int b = 0; b < bytes_per_pixel; b++) {
        std::swap(data[(i + j * width) * bytes_per_pixel + b], data[((width - 1 - i) + j * width) * bytes_per_pixel + b]);
      }
    }
  }
}

int TgaImage::TGA_image::read_width() const {
  return width;
}

int TgaImage::TGA_image::read_height() const {
  return width;
}
