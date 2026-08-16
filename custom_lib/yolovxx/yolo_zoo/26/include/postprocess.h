/**
 * YOLO26 Post-processing header for Fish Counter
 *
 * Supports YOLO26 2-output format: boxes [1, 4, N] + scores [1, nc, N]
 *
 * Based on the working rknn_minimal_test example.
 */

#ifndef _RKNN_YOLO26_DEMO_POSTPROCESS_H_
#define _RKNN_YOLO26_DEMO_POSTPROCESS_H_

#include "yolo26.h"

#include <stdint.h>
#include <vector>
#include "rknn_api.h"
#include "common.h"
#include "image_utils.h"
// #include "rknn_detector.h"

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_CLASS_NUM 80
#define NMS_THRESH 0.45
#define BOX_THRESH 0.25

// class rknn_app_context_t;

typedef struct {
    image_rect_t box;
    float prop;
    int cls_id;
} object_detect_result;

typedef struct {
    int id;
    int count;
    object_detect_result results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

// Detection result (same structure as postprocess.h for compatibility)
typedef struct
{
    float x1, y1, x2, y2; // Bounding box (in model coordinates)
    float confidence;
    int class_id;
} yolo26_detection_t;

// YOLO26 context for format detection and caching
typedef struct{
    int num_predictions;
    int num_classes;
    bool boxes_transposed; // true = [1, 4, N], false = [1, N, 4]
} yolo_context_t;

/**
 * Initialize YOLO26 post-processing
 * Detects the output format from model attributes
 *
 * @param app_ctx The RKNN app context with model info
 * @param yolo26_ctx Output: YOLO26 context for format info
 * @return 0 on success, -1 on error
 */
int init_post_process(rknn_app_context_t *app_ctx, yolo_context_t *yolo26_ctx);

/**
 * Deinitialize YOLO26 post-processing
 */
void deinit_post_process();
char *coco_cls_to_name(int cls_id);

/**
 * YOLO26 post_process: processes model outputs and returns detections
 * Uses 2-output format: boxes [1, 4, N] + scores [1, nc, N]
 *
 * @param app_ctx The RKNN app context
 * @param yolo26_ctx YOLO26 context from init
 * @param outputs Pointer to output tensor memory array
 * @param letter_box Letterbox transform info for coordinate mapping
 * @param conf_threshold Confidence threshold
 * @param nms_threshold NMS threshold
 * @param od_results Output: detection results (compatible with existing code)
 * @return 0 on success, -1 on error
 */
int post_process(rknn_app_context_t *app_ctx, yolo_context_t *yolo26_ctx, void *outputs, letterbox_t *letter_box, float conf_threshold, float nms_threshold, object_detect_result_list *od_results);

#endif //_RKNN_YOLO26_DEMO_POSTPROCESS_H_