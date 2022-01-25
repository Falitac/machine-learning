#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

class MachinePainter {
private:

};


int main(int argc, char** argv) {
  std::mt19937 mt{std::random_device{}()};

  std::string directoryContentName = "new-images/";
  char* image_name = "obraz.png";

  int width, height, channels;
  void* data = stbi_load(image_name, &width, &height, &channels, 4);

  if(!data) {
    std::fprintf(stderr, "Couldn't load %s\n", image_name);
    std::exit(EXIT_FAILURE);
  }

  for(int i = 0; i <= 1000; i+=10 ) {
    char buffer[128];
    std::string fileName = "new-images/";
    std::sprintf(buffer, "%3d.png", i);
    fileName.append(buffer);

    stbi_write_png(fileName.c_str(), width, height, 4, data, 0);
  }

  std::printf("WIDTH:    %5d\n", width);
  std::printf("HEIGHT:   %5d\n", height);
  std::printf("CHANNELS: %5d\n", channels);

  return EXIT_SUCCESS;
}