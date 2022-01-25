#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <vector>
#include <algorithm>
#include <utility>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define DEFAULT_SPECIMEN_COUNT 150
#define DEFAULT_BEST_COUNT 10

std::mt19937 mt{std::random_device{}()};

struct Image {
  void* data;
  int width, height;
  int channels;

  Image() = default;

  Image(const std::string& fileLocation) {
    loadImage(fileLocation);
  }

  int loadImage(std::string fileLocation) {
    data = stbi_load(fileLocation.c_str(), &width, &height, &channels, 3);
    if(!data) {
      std::fprintf(stderr, "Couldn't load %s\n", fileLocation.c_str());
      return -1;
    }
    channels = 3;

    return 0;
  }

  int saveImage(std::string fileLocation) {
    if(!data) {
      std::fprintf(stderr, "Can't write empty data %s\n", fileLocation.c_str());
      return -1;
    }
    stbi_write_png(fileLocation.c_str(), width, height, channels, data, 0);

    return 0;
  }

};

class MachinePainter {
public:
  MachinePainter(const Image& image, size_t specCount = 1, size_t bestCount = 1, size_t dumpEach = 1)
  : referenceImage(static_cast<uint8_t*>(image.data))
  , width(image.width)
  , height(image.height)
  , channels(image.channels)
  , randomUniformColor(0, 255)
  , randomUniformWidth(0, image.width)
  , randomUniformHeight(0, image.height)
  , randomUniformRadius(1, 100)
  , allImages(specCount, std::vector<uint8_t>(image.channels * image.width * image.height))
  , bestImagesIndexes(bestCount)
  , dumpEach(dumpEach)
  { //image.height < image.width ? image.height : image.width
  }

  void run() {
    for(;; iteration++) {
      paint();
      check();
      if(iteration % dumpEach == 0) {
        dumpBest();
      }
      mutate();
    }
  }


private:
  size_t iteration = 0;
  int width, height;
  int channels;
  uint8_t* referenceImage;

  std::vector<std::vector<uint8_t>> allImages;
  std::vector<size_t> bestImagesIndexes;

  std::uniform_int_distribution<int> randomUniformWidth;
  std::uniform_int_distribution<int> randomUniformHeight;
  std::uniform_int_distribution<int> randomUniformRadius;
  std::uniform_int_distribution<int> randomUniformColor;

  int dumpEach;

  inline size_t getPixelAddress(int x, int y, int color) {
    return (y * width + x) * channels + color;
  }

  void paint() {
    auto isInBoundaries = [&] (int x, int y) {
      return 0 <= x && x < width && 0 <= y && y < height;
    };
    for(auto& image : allImages) {
      auto setMeanGrayColor = [&] (int x, int y, int8_t value) {
        for(int i = 0; i < channels; i++) {
          auto& colorFromImage = image[getPixelAddress(x, y, i)];
          colorFromImage = (colorFromImage + value) / 2;
        }
        if(channels == 4) {
          image[getPixelAddress(x, y, 3)] = 0xff; // alpha
        }
      };
      int cx = randomUniformWidth(mt);
      int cy = randomUniformHeight(mt);
      int cr = randomUniformRadius(mt);

      auto randomColor = randomUniformColor(mt);
      for(int y = cy - cr; y <= cy + cr; y++) {
        for(int x = cx - cr; x <= cx + cr; x++) {
          if(!isInBoundaries(x, y)) {
            continue;
          }
          auto diffx = cx - x;
          auto diffy = cy - y;
          if(diffx * diffx + diffy * diffy <= cr * cr) {
            setMeanGrayColor(x, y, randomColor);
          }
        }
      }
    }
  }

  void check() {
    std::vector<std::pair<double, size_t>> ranking(allImages.size());
    for(size_t i = 0; auto& image : allImages) {
      double result = 0.0;
      for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
          auto referenceColor = referenceImage[getPixelAddress(x, y, 0)];
          auto imageColor = image[getPixelAddress(x, y, 0)];
          auto diff = double(referenceColor) - double(imageColor);
          result += diff * diff;
        }
      }
      ranking[i++] = std::make_pair(result, i);
    }
    std::sort(ranking.begin(), ranking.end(), [&](const auto& element1, const auto& element2) {
      return element1.first < element2.first;
    });

    std::printf("%4i %.2lf\n", iteration, ranking.front().first);
    
    for(size_t i = 0; i < bestImagesIndexes.size(); i++) {
      bestImagesIndexes[i] = ranking[i].second;
    }
  }

  void dumpBest() {
    for(size_t i = 0; i < bestImagesIndexes.size(); i++) {
      std::string outputPath = "new-images/";
      char fileName[128];
      std::snprintf(fileName, 128, "%i.png", iteration / dumpEach, i);
      outputPath.append(fileName);

      Image image;
      image.data = static_cast<void*>(allImages[bestImagesIndexes[i]].data());
      image.width = width;
      image.height = height;
      image.channels = channels;
      image.saveImage(outputPath);
    }
  }

  void mutate() {
    std::vector<std::vector<uint8_t>> bestImages(bestImagesIndexes.size());
    for(size_t i = 0; i < bestImagesIndexes.size(); i++) {
      bestImages[i] = allImages[bestImagesIndexes[i]];
    }

    for(size_t i = 0; i < allImages.size(); i++ ) {
      allImages[i] = bestImages[i % bestImages.size()];
    }
  }
};


bool isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

int main(int argc, char** argv) {
  std::string imageName = "obraz.png";
  int specimenCount = DEFAULT_SPECIMEN_COUNT;
  int bestCount = DEFAULT_BEST_COUNT;
  int dumpEach = 1;

  auto getNumberFromArgs = [&argc, &argv] (int argIndex, auto& number) {
    if(isNumber(argv[argIndex])) {
      number = atoi(argv[argIndex]);
    }
  };
  if(argc > 1) {
    imageName = std::string(argv[1]);
  }
  if(argc > 2) {
    getNumberFromArgs(2, specimenCount);
  }
  if(argc > 3) {
    getNumberFromArgs(3, bestCount);
  }
  if(argc > 4) {
    getNumberFromArgs(4, dumpEach);
  }

  std::puts("Start with parameters:");
  std::printf("Image name: %s\n", imageName.c_str());
  std::printf("Specimen count: %i\n", specimenCount);
  std::printf("Best count: %i\n", bestCount);
  std::printf("Dump each iteration: %i\n", dumpEach);
  
  Image image(imageName);
  MachinePainter painter{image, specimenCount, bestCount, dumpEach};

  painter.run();

  return EXIT_SUCCESS;
}