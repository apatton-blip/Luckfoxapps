#ifndef YOLOVXX_MEDIA_H
#define YOLOVXX_MEDIA_H

#include "yolovxx.hpp"

// Initializes the MIPI VI (Camera), ISP, VENC, and RTSP stream.
yolovxx_media_context yolovxx_media_init(int width, int height, int port, const char* rtsp_path);

// Fetches a cv::Mat bound directly to the VENC DMA memory for zero-copy streaming.
cv::Mat yolovxx_media_get_buffer(yolovxx_media_context ctx);

// Pulls a frame from the MIPI camera hardware and writes it into the target DMA buffer.
bool yolovxx_media_read_camera(yolovxx_media_context ctx, cv::Mat& dma_frame);

// Compresses the DMA buffer via hardware VENC and broadcasts it to the network.
void yolovxx_media_broadcast(yolovxx_media_context ctx);

// Releases camera and networking hardware.
void yolovxx_media_release(yolovxx_media_context ctx);

#endif // YOLOVXX_MEDIA_H