#ifndef YOLOVXX_AI_H
#define YOLOVXX_AI_H

#include "yolovxx.hpp"

// Initializes the NPU and loads the target YOLO model.
yolovxx_ai_context yolovxx_ai_init(const char* model_path);

// Runs inference on a BGR frame. Handles letterboxing and coordinate mapping automatically.
std::vector<yolovxx_detection> yolovxx_ai_infer(yolovxx_ai_context ctx, const cv::Mat& frame);

// Releases NPU hardware resources.
void yolovxx_ai_release(yolovxx_ai_context ctx);

#endif // YOLOVXX_AI_H