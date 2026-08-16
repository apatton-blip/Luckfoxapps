#include "yolovxx_ai.hpp"
#include <iostream>
#include <algorithm>

#ifndef YOLOVXX_CPT_H
#define YOLOVXX_CPT_H

#if YOLO_VERSION == 8
    #include "yolov8.h"
    #define INIT_YOLO init_yolov8_model
    #define INFER_YOLO(ctx, yolo_ctx, img, res) inference_yolov8_model(ctx, img, res)
    #define RELEASE_YOLO release_yolov8_model
#elif YOLO_VERSION == 10
    #include "yolov10.h"
    #define INIT_YOLO init_yolov10_model
    #define INFER_YOLO(ctx, yolo_ctx, img, res) inference_yolov10_model(ctx, img, res)
    #define RELEASE_YOLO release_yolov10_model
#elif YOLO_VERSION == 11
    #include "yolo11.h"
    #define INIT_YOLO init_yolo11_model
    #define INFER_YOLO(ctx, yolo_ctx, img, res) inference_yolo11_model(ctx, img, res)
    #define RELEASE_YOLO release_yolo11_model
#elif YOLO_VERSION == 26
    #include "yolo26.h"
    #define INIT_YOLO init_yolo26_model
    #define INFER_YOLO(ctx, yolo_ctx, img, res) inference_yolo26_model(ctx, yolo_ctx, img, res)
    #define RELEASE_YOLO release_yolo26_model
#else
    #error "Unsupported YOLO_VERSION specified in CMake!"
#endif

#endif // YOLOVXX_CPT_H

constexpr int MODEL_WIDTH = 640;
constexpr int MODEL_HEIGHT = 640;

struct ai_internal_ctx {
    rknn_app_context_t rknn_ctx;
    yolo_context_t yolo_ctx;
};

yolovxx_ai_context yolovxx_ai_init(const char* model_path) {
    ai_internal_ctx* ctx = new ai_internal_ctx();
    memset(&ctx->rknn_ctx, 0, sizeof(rknn_app_context_t));
    
    if (INIT_YOLO(model_path, &ctx->rknn_ctx) != 0) {
        std::cerr << "Failed to initialize RKNN model!" << std::endl;
        delete ctx;
        return nullptr;
    }
    
    // init_post_process();
    return static_cast<yolovxx_ai_context>(ctx);
}

std::vector<yolovxx_detection> yolovxx_ai_infer(yolovxx_ai_context ctx, const cv::Mat& frame) {
    std::vector<yolovxx_detection> detections;
    if (!ctx) return detections;
    
    ai_internal_ctx* internal = static_cast<ai_internal_ctx*>(ctx);

    // OpenCV Letterbox
    float scale = std::min((float)MODEL_WIDTH / frame.cols, (float)MODEL_HEIGHT / frame.rows);
    int scaled_w = (int)(frame.cols * scale);
    int scaled_h = (int)(frame.rows * scale);
    int pad_x = (MODEL_WIDTH - scaled_w) / 2;
    int pad_y = (MODEL_HEIGHT - scaled_h) / 2;

    cv::Mat inputScale, letterboxImage(MODEL_HEIGHT, MODEL_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::resize(frame, inputScale, cv::Size(scaled_w, scaled_h), 0, 0, cv::INTER_LINEAR);
    inputScale.copyTo(letterboxImage(cv::Rect(pad_x, pad_y, scaled_w, scaled_h)));

    // Copy to NPU Memory
    memcpy(internal->rknn_ctx.input_mems[0]->virt_addr, letterboxImage.data, MODEL_WIDTH * MODEL_HEIGHT * 3);
    
    // Populate Image Buffer
    image_buffer_t img;
    memset(&img, 0, sizeof(image_buffer_t));
    img.width = MODEL_WIDTH; 
    img.height = MODEL_HEIGHT;
    img.width_stride = MODEL_WIDTH; 
    img.height_stride = MODEL_HEIGHT;
    img.format = IMAGE_FORMAT_RGB888;
    img.virt_addr = letterboxImage.data;

    object_detect_result_list od_results;
    
    // Run Inference
    INFER_YOLO(&internal->rknn_ctx, &internal->yolo_ctx, &img, &od_results);

    // Map Coordinates
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result* raw_det = &(od_results.results[i]);

        int x1 = (int)((raw_det->box.left - pad_x) / scale);
        int y1 = (int)((raw_det->box.top - pad_y) / scale);
        int x2 = (int)((raw_det->box.right - pad_x) / scale);
        int y2 = (int)((raw_det->box.bottom - pad_y) / scale);

        yolovxx_detection det;
        det.x = x1; det.y = y1;
        det.width = x2 - x1; det.height = y2 - y1;
        det.confidence = raw_det->prop;
        det.class_id = raw_det->cls_id;
        det.label = coco_cls_to_name(raw_det->cls_id); 
        detections.push_back(det);
    }
    return detections;
}

void yolovxx_ai_release(yolovxx_ai_context ctx) {
    if (ctx) {
        ai_internal_ctx* internal = static_cast<ai_internal_ctx*>(ctx);
        RELEASE_YOLO(&internal->rknn_ctx);
        deinit_post_process();
        delete internal;
    }
}

