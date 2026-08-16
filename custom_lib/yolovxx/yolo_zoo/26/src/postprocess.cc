/**
 * YOLO26 Post-processing for Fish Counter on RV1106
 *
 * Supports YOLO26 2-output format: boxes [1, 4, N] + scores [1, nc, N]
 *
 * Based on the working rknn_minimal_test example.
 */

#include "yolo26.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>
#define LABEL_NALE_TXT_PATH "./model/coco_80_labels_list.txt"

static char *labels[OBJ_CLASS_NUM];

// Clamp value to range
inline static int clamp(float val, int min, int max){
    return val > min ? (val < max ? val : max) : min;
}

// Dequantize INT8 to float
static inline float dequantize(int8_t qnt, int32_t zp, float scale){
    return ((float)qnt - (float)zp) * scale;
}

// Sigmoid function
static inline float sigmoid(float x){
    return 1.0f / (1.0f + expf(-x));
}

// Calculate IoU between two boxes
static float calculate_iou(float x1_a, float y1_a, float x2_a, float y2_a,
                           float x1_b, float y1_b, float x2_b, float y2_b){
    float x1 = std::max(x1_a, x1_b);
    float y1 = std::max(y1_a, y1_b);
    float x2 = std::min(x2_a, x2_b);
    float y2 = std::min(y2_a, y2_b);

    float inter_w = std::max(0.0f, x2 - x1);
    float inter_h = std::max(0.0f, y2 - y1);
    float inter_area = inter_w * inter_h;

    float area_a = (x2_a - x1_a) * (y2_a - y1_a);
    float area_b = (x2_b - x1_b) * (y2_b - y1_b);
    float union_area = area_a + area_b - inter_area;

    return union_area > 0 ? inter_area / union_area : 0;
}

// Non-Maximum Suppression for YOLO26 detections
static std::vector<yolo26_detection_t> nms(std::vector<yolo26_detection_t> &detections, float nms_threshold){
    // Sort by confidence (descending)
    std::sort(detections.begin(), detections.end(),
              [](const yolo26_detection_t &a, const yolo26_detection_t &b)
              { return a.confidence > b.confidence; });

    std::vector<yolo26_detection_t> result;
    std::vector<bool> suppressed(detections.size(), false);

    for (size_t i = 0; i < detections.size(); i++)
    {
        if (suppressed[i])
            continue;

        result.push_back(detections[i]);

        for (size_t j = i + 1; j < detections.size(); j++)
        {
            if (suppressed[j])
                continue;

            // Only suppress same class
            if (detections[i].class_id != detections[j].class_id)
                continue;

            float iou = calculate_iou(
                detections[i].x1, detections[i].y1, detections[i].x2, detections[i].y2,
                detections[j].x1, detections[j].y1, detections[j].x2, detections[j].y2);

            if (iou > nms_threshold)
            {
                suppressed[j] = true;
            }
        }
    }

    return result;
}

// Parse YOLO26 2-output format: boxes [1, 4, N] + scores [1, nc, N]
static std::vector<yolo26_detection_t> parse_yolo26_2output(
    rknn_tensor_mem **output_mems,
    rknn_tensor_attr *output_attrs,
    int model_width,
    int model_height,
    float conf_threshold, int num_classes,
    bool boxes_transposed){

    std::vector<yolo26_detection_t> detections;

    int8_t *boxes_data = (int8_t *)output_mems[0]->virt_addr;
    int32_t boxes_zp = output_attrs[0].zp;
    float boxes_scale = output_attrs[0].scale;

    int8_t *scores_data = (int8_t *)output_mems[1]->virt_addr;
    int32_t scores_zp = output_attrs[1].zp;
    float scores_scale = output_attrs[1].scale;

    // Determine number of anchors
    int num_anchors;
    if (boxes_transposed)
    {
        num_anchors = output_attrs[0].dims[2];
    }
    else
    {
        num_anchors = output_attrs[0].dims[1];
    }

    // Check if scores are already sigmoid or logits
    float scores_min = 9999, scores_max = -9999;
    for (int i = 0; i < (int)output_attrs[1].n_elems; i++)
    {
        float val = dequantize(scores_data[i], scores_zp, scores_scale);
        if (val < scores_min)
            scores_min = val;
        if (val > scores_max)
            scores_max = val;
    }
    bool scores_are_sigmoid = (scores_min >= -0.1f && scores_max <= 1.1f);

    for (int i = 0; i < num_anchors; i++)
    {
        // Get box coordinates
        float x1, y1, x2, y2;
        if (boxes_transposed)
        {
            // [1, 4, N] - channel first
            x1 = dequantize(boxes_data[0 * num_anchors + i], boxes_zp, boxes_scale);
            y1 = dequantize(boxes_data[1 * num_anchors + i], boxes_zp, boxes_scale);
            x2 = dequantize(boxes_data[2 * num_anchors + i], boxes_zp, boxes_scale);
            y2 = dequantize(boxes_data[3 * num_anchors + i], boxes_zp, boxes_scale);
        }
        else
        {
            // [1, N, 4] - anchor first
            x1 = dequantize(boxes_data[i * 4 + 0], boxes_zp, boxes_scale);
            y1 = dequantize(boxes_data[i * 4 + 1], boxes_zp, boxes_scale);
            x2 = dequantize(boxes_data[i * 4 + 2], boxes_zp, boxes_scale);
            y2 = dequantize(boxes_data[i * 4 + 3], boxes_zp, boxes_scale);
        }

        // Check if format is xywh instead of xyxy
        if (x2 < x1 || y2 < y1)
        {
            // Assume xywh format: convert to xyxy
            float cx = x1, cy = y1, w = x2, h = y2;
            x1 = cx - w / 2.0f;
            y1 = cy - h / 2.0f;
            x2 = cx + w / 2.0f;
            y2 = cy + h / 2.0f;
        }

        // Find max class score
        float max_score = -9999;
        int max_class = 0;
        for (int c = 0; c < num_classes; c++)
        {
            float score;
            if (boxes_transposed)
            {
                // [1, nc, N]
                score = dequantize(scores_data[c * num_anchors + i], scores_zp, scores_scale);
            }
            else
            {
                // [1, N, nc]
                score = dequantize(scores_data[i * num_classes + c], scores_zp, scores_scale);
            }

            if (!scores_are_sigmoid)
            {
                score = sigmoid(score);
            }

            if (score > max_score)
            {
                max_score = score;
                max_class = c;
            }
        }

        if (max_score < conf_threshold)
            continue;

        // Clamp to model bounds
        x1 = std::max(0.0f, std::min(x1, (float)model_width));
        y1 = std::max(0.0f, std::min(y1, (float)model_height));
        x2 = std::max(0.0f, std::min(x2, (float)model_width));
        y2 = std::max(0.0f, std::min(y2, (float)model_height));

        // Skip invalid boxes
        if (x2 <= x1 || y2 <= y1)
            continue;

        yolo26_detection_t det;
        det.x1 = x1;
        det.y1 = y1;
        det.x2 = x2;
        det.y2 = y2;
        det.confidence = max_score;
        det.class_id = max_class;

        detections.push_back(det);
    }

    return detections;
}

int init_post_process(rknn_app_context_t *app_ctx, yolo_context_t *yolo_ctx){
    if (!app_ctx || !yolo_ctx)
    {
        printf("[YOLO26] ERROR: Invalid parameters\n");
        return -1;
    }

    memset(yolo_ctx, 0, sizeof(yolo_context_t));

    int n_output = app_ctx->io_num.n_output;

    printf("[YOLO26] Initializing 2-output format (%d outputs)...\n", n_output);

    // Print output tensor info
    // for (int i = 0; i < n_output; i++)
    // {
    //     printf("  Output %d: dims=[%d,%d,%d,%d], size=%d\n", i,
    //            app_ctx->output_attrs[i].dims[0],
    //            app_ctx->output_attrs[i].dims[1],
    //            app_ctx->output_attrs[i].dims[2],
    //            app_ctx->output_attrs[i].dims[3],
    //            app_ctx->output_attrs[i].size);
    // }

    if (n_output != 2)
    {
        printf("[YOLO26] ERROR: Expected 2 outputs (boxes + scores), got %d\n", n_output);
        return -1;
    }

    // Parse 2-output format: boxes + scores
    int boxes_dim1 = app_ctx->output_attrs[0].dims[1];
    int boxes_dim2 = app_ctx->output_attrs[0].dims[2];

    int scores_dim1 = app_ctx->output_attrs[1].dims[1];
    int scores_dim2 = app_ctx->output_attrs[1].dims[2];

    if (boxes_dim1 == 4)
    {
        // [1, 4, N] boxes format
        yolo_ctx->num_predictions = boxes_dim2;
        yolo_ctx->boxes_transposed = true;
        yolo_ctx->num_classes = scores_dim1;
        printf("[YOLO26] Format: boxes [1,4,%d], scores [1,%d,%d]\n",
               boxes_dim2, scores_dim1, scores_dim2);
    }
    else if (boxes_dim2 == 4)
    {
        // [1, N, 4] boxes format
        yolo_ctx->num_predictions = boxes_dim1;
        yolo_ctx->boxes_transposed = false;
        yolo_ctx->num_classes = scores_dim2;
        printf("[YOLO26] Format: boxes [1,%d,4], scores [1,%d,%d]\n",
               boxes_dim1, scores_dim1, scores_dim2);
    }
    else
    {
        printf("[YOLO26] ERROR: Invalid boxes dimensions [1,%d,%d], expected [1,4,N] or [1,N,4]\n",
               boxes_dim1, boxes_dim2);
        return -1;
    }

    printf("[YOLO26] Predictions: %d, Classes: %d\n",
           yolo_ctx->num_predictions, yolo_ctx->num_classes);

    return 0;
}

void deinit_post_process(){
    // Nothing to cleanup currently
}

int post_process(rknn_app_context_t *app_ctx, yolo_context_t *yolo_ctx,
                        void *outputs, letterbox_t *letter_box,
                        float conf_threshold, float nms_threshold,
                        object_detect_result_list *od_results){
    if (!app_ctx || !yolo_ctx || !outputs || !letter_box || !od_results)
    {
        return -1;
    }

    rknn_tensor_mem **output_mems = (rknn_tensor_mem **)outputs;

    int model_width = app_ctx->model_width;
    int model_height = app_ctx->model_height;

    // Parse 2-output format
    std::vector<yolo26_detection_t> raw_detections = parse_yolo26_2output(
        output_mems,
        app_ctx->output_attrs,
        model_width, model_height,
        conf_threshold,
        yolo_ctx->num_classes > 0 ? yolo_ctx->num_classes : 1,
        yolo_ctx->boxes_transposed);

    // Apply NMS
    std::vector<yolo26_detection_t> final_detections = nms(raw_detections, nms_threshold);

    // Convert to output format with letterbox coordinate mapping
    memset(od_results, 0, sizeof(object_detect_result_list));
    int count = 0;

    for (const auto &det : final_detections)
    {
        if (count >= OBJ_NUMB_MAX_SIZE)
            break;

        // Map coordinates back to original image space
        float x1 = det.x1 - letter_box->x_pad;
        float y1 = det.y1 - letter_box->y_pad;
        float x2 = det.x2 - letter_box->x_pad;
        float y2 = det.y2 - letter_box->y_pad;

        // Scale back to original coordinates
        od_results->results[count].box.left = (int)(clamp(x1, 0, model_width) / letter_box->scale);
        od_results->results[count].box.top = (int)(clamp(y1, 0, model_height) / letter_box->scale);
        od_results->results[count].box.right = (int)(clamp(x2, 0, model_width) / letter_box->scale);
        od_results->results[count].box.bottom = (int)(clamp(y2, 0, model_height) / letter_box->scale);
        od_results->results[count].prop = det.confidence;
        od_results->results[count].cls_id = det.class_id;

        count++;
    }

    od_results->count = count;

    return 0;
}

char *coco_cls_to_name(int cls_id){

    if (cls_id >= OBJ_CLASS_NUM)
    {
        return "null";
    }

    if (labels[cls_id])
    {
        return labels[cls_id];
    }

    return "null";
}