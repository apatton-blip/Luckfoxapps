#ifndef YOLOVXX_UTIL_H
#define YOLOVXX_UTIL_H

#include "yolovxx.hpp"

// Standardized visual overlay for bounding boxes
void yolovxx_draw_detection(cv::Mat& frame, const yolovxx_detection& det);

#endif //YOLOVXX_UTIL_H