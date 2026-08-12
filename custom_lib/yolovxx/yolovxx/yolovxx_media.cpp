#include "yolovxx_media.hpp"
#include <iostream>

#include "luckfox_mpi.h"
#include "rtsp_demo.h"

struct media_internal_ctx {
    int width, height;
    MB_POOL src_pool;
    MB_BLK src_blk;
    unsigned char* dma_buffer;
    
    rtsp_demo_handle g_rtsplive;
    rtsp_session_handle g_rtsp_session;
    
    VENC_STREAM_S stFrame;  
    VIDEO_FRAME_INFO_S h264_frame;
    RK_U32 time_ref;
};

yolovxx_media_context yolovxx_media_init(int width, int height, int port, const char* rtsp_path) {
    media_internal_ctx* ctx = new media_internal_ctx();
    memset(ctx, 0, sizeof(media_internal_ctx));
    ctx->width = width; 
    ctx->height = height; 
    ctx->time_ref = 0;

    // 1. Initialize ISP Hardware (MIPI Camera)
    int s32Ret = SAMPLE_COMM_ISP_Init(0, RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, "/etc/iqfiles");
    s32Ret |= SAMPLE_COMM_ISP_Run(0);
    if (s32Ret != RK_SUCCESS) {
        std::cerr << "SAMPLE_COMM_ISP_Init/Run failed!" << std::endl;
        delete ctx;
        return nullptr;
    }

    // 2. Initialize Rockchip MPI Subsystem (MUST BE CALLED BEFORE vi_dev_init!)
    if (RK_MPI_SYS_Init() != RK_SUCCESS) {
        std::cerr << "RK_MPI_SYS_Init failed!" << std::endl;
        delete ctx;
        return nullptr;
    }

    // 3. Initialize VI Device and Channel
    vi_dev_init();
    vi_chn_init(0, width, height);

    // 4. Create VENC DMA Buffer Pool
    MB_POOL_CONFIG_S PoolCfg;
    memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    PoolCfg.u64MBSize = width * height * 3;
    PoolCfg.u32MBCnt = 1;
    PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
    ctx->src_pool = RK_MPI_MB_CreatePool(&PoolCfg);
    ctx->src_blk = RK_MPI_MB_GetMB(ctx->src_pool, width * height * 3, RK_TRUE);
    ctx->dma_buffer = (unsigned char*)RK_MPI_MB_Handle2VirAddr(ctx->src_blk);

    if (!ctx->dma_buffer) {
        std::cerr << "Failed to allocate DMA memory buffer!" << std::endl;
        delete ctx;
        return nullptr;
    }

    // 5. Setup Encoder Frame Wrapper
    memset(&ctx->h264_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    ctx->h264_frame.stVFrame.u32Width = width;
    ctx->h264_frame.stVFrame.u32Height = height;
    ctx->h264_frame.stVFrame.u32VirWidth = width;
    ctx->h264_frame.stVFrame.u32VirHeight = height;
    ctx->h264_frame.stVFrame.enPixelFormat = RK_FMT_RGB888; 
    ctx->h264_frame.stVFrame.u32FrameFlag = 160;
    ctx->h264_frame.stVFrame.pMbBlk = ctx->src_blk;
    ctx->stFrame.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S));

    // 6. Setup RTSP & Encoder
    ctx->g_rtsplive = create_rtsp_demo(port);
    ctx->g_rtsp_session = rtsp_new_session(ctx->g_rtsplive, rtsp_path);
    rtsp_set_video(ctx->g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_sync_video_ts(ctx->g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());
    venc_init(0, width, height, RK_VIDEO_ID_AVC);

    return static_cast<yolovxx_media_context>(ctx);
}

cv::Mat yolovxx_media_get_buffer(yolovxx_media_context ctx) {
    if (!ctx) return cv::Mat();
    media_internal_ctx* mctx = static_cast<media_internal_ctx*>(ctx);
    return cv::Mat(mctx->height, mctx->width, CV_8UC3, mctx->dma_buffer);
}

bool yolovxx_media_read_camera(yolovxx_media_context ctx, cv::Mat& dma_frame) {
    if (!ctx) return false;
    media_internal_ctx* mctx = static_cast<media_internal_ctx*>(ctx);
    VIDEO_FRAME_INFO_S stViFrame;
    
    // Pull hardware frame from camera sensor
    if (RK_MPI_VI_GetChnFrame(0, 0, &stViFrame, -1) == RK_SUCCESS) {
        void *vi_data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);
        if (!vi_data) {
            RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame);
            return false;
        }
        
        // Convert NV12 Hardware YUV directly into our target BGR DMA Buffer
        cv::Mat yuv420sp(mctx->height + mctx->height / 2, mctx->width, CV_8UC1, vi_data);
        cv::cvtColor(yuv420sp, dma_frame, cv::COLOR_YUV420sp2BGR);
        
        RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame);
        return true;
    }
    return false;
}

void yolovxx_media_broadcast(yolovxx_media_context ctx) {
    if (!ctx) return;
    media_internal_ctx* mctx = static_cast<media_internal_ctx*>(ctx);
    mctx->h264_frame.stVFrame.u32TimeRef = mctx->time_ref++;
    mctx->h264_frame.stVFrame.u64PTS = TEST_COMM_GetNowUs();

    RK_MPI_SYS_MmzFlushCache(mctx->src_blk, RK_FALSE);    
    RK_MPI_VENC_SendFrame(0, &mctx->h264_frame, -1);
    
    if(RK_MPI_VENC_GetStream(0, &mctx->stFrame, -1) == RK_SUCCESS) {
        if(mctx->g_rtsplive && mctx->g_rtsp_session) {
            void* pData = RK_MPI_MB_Handle2VirAddr(mctx->stFrame.pstPack->pMbBlk);
            rtsp_tx_video(mctx->g_rtsp_session, (uint8_t*)pData, mctx->stFrame.pstPack->u32Len, mctx->stFrame.pstPack->u64PTS);
            rtsp_do_event(mctx->g_rtsplive);
        }
    }
    RK_MPI_VENC_ReleaseStream(0, &mctx->stFrame);
}

void yolovxx_media_release(yolovxx_media_context ctx) {
    if (ctx) {
        media_internal_ctx* mctx = static_cast<media_internal_ctx*>(ctx);
        RK_MPI_VI_DisableChn(0, 0);
        RK_MPI_VI_DisableDev(0);
        SAMPLE_COMM_ISP_Stop(0);
        RK_MPI_MB_ReleaseMB(mctx->src_blk);
        RK_MPI_MB_DestroyPool(mctx->src_pool);
        RK_MPI_VENC_StopRecvFrame(0);
        RK_MPI_VENC_DestroyChn(0);
        if (mctx->stFrame.pstPack) free(mctx->stFrame.pstPack);
        if (mctx->g_rtsplive) rtsp_del_demo(mctx->g_rtsplive);
        RK_MPI_SYS_Exit();
        delete mctx;
    }
}