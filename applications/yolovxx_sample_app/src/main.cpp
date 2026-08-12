#include <iostream>

#include "yolovxx_ai.hpp"
#include "yolovxx_media.hpp"
#include "yolovxx_util.hpp"

int main() {
    int width = 720;
    int height = 480;
    system("RkLunch-stop.sh");
    std::cout << "Starting Luckfox YOLO Streaming Pipeline..." << std::endl;

    // 1. Initialize our functional subsystems
    yolovxx_media_context media_ctx = yolovxx_media_init(width, height, 554, "/live/0");
    yolovxx_ai_context yolo_ctx = yolovxx_ai_init("./model/yolov10.rknn");

    if (!yolo_ctx || !media_ctx) {
        std::cerr << "Initialization failed. Exiting." << std::endl;
        return -1;
    }

    // 2. Map OpenCV Mat directly to the VENC DMA memory for zero-copy
    cv::Mat frame = yolovxx_media_get_buffer(media_ctx);

    std::cout << "Stream available at: rtsp://<board_ip>:554/live/0" << std::endl;

    // 3. Main Application Loop
    while (true) {
        // Grab a frame from the MIPI Camera hardware directly into the DMA buffer
        if (yolovxx_media_read_camera(media_ctx, frame)) {
            
            // Run AI Inference
            std::vector<yolovxx_detection> results = yolovxx_ai_infer(yolo_ctx, frame);

            // CUSTOM POST-PROCESSING CAN BE DONE HERE
            for (const auto& det : results) {
                if (det.confidence > 0.4) {
                    yolovxx_draw_detection(frame, det);
                }
            }

            yolovxx_media_broadcast(media_ctx);
        }
    }

    yolovxx_ai_release(yolo_ctx);
    yolovxx_media_release(media_ctx);

    return 0;
}