#ifndef YOLOVXX_H
#define YOLOVXX_H

#include <vector>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

struct yolovxx_detection {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    int class_id;
    std::string label;
};

// Opaque handles representing hardware subsystems
typedef void* yolovxx_ai_context;
typedef void* yolovxx_media_context;

#endif // YOLOVXX_H