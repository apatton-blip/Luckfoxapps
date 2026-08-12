#include "yolovxx_util.hpp"
#include <iostream>

void yolovxx_draw_detection(cv::Mat& frame, const yolovxx_detection& det) {
    cv::rectangle(frame, cv::Point(det.x, det.y), cv::Point(det.x + det.width, det.y + det.height), cv::Scalar(0, 255, 0), 3);
    char text[64];
    snprintf(text, sizeof(text), "%s %.1f%%", det.label.c_str(), det.confidence * 100.0f);
    cv::putText(frame, text, cv::Point(det.x, det.y - 8), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
}