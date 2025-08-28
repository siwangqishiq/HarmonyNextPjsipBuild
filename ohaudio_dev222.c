/*
 * ohaudio_dev.c  —  HarmonyOS NEXT OH_Audio driver for PJSIP
 * 2024-06-01  clean build
 */
#include <pjmedia-audiodev/audiodev_imp.h>
#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiorenderer.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>

#define THIS_FILE   "ohaudio_dev.c"

/* ---------- 数据结构 ---------- */
typedef struct oha_stream {
    pjmedia_aud_stream   base;
    pj_pool_t           *pool;
    pjmedia_aud_param    param;
    pj_bool_t            running;

    /* 采集 */
    OH_AudioCapturer    *capturer;
    pjmedia_aud_rec_cb   rec_cb;
    void                *rec_user_data;

    /* 播放 */
    OH_AudioRenderer    *renderer;
    pjmedia_aud_play_cb  play_cb;
    void                *play_user_data;
} oha_stream;

/* ---------- 工厂函数 ---------- */
static pj_status_t oha_factory_init(pjmedia_aud_dev_factory *f);
static pj_status_t oha_factory_destroy(pjmedia_aud_dev_factory *f);
static unsigned    oha_factory_get_dev_count(pjmedia_aud_dev_factory *f);
static pj_status_t oha_factory_get_dev_info(pjmedia_aud_dev_factory *f,
                                            unsigned index,
                                            pjmedia_aud_dev_info *info);
static pj_status_t oha_factory_default_param(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_param *param);
static pj_status_t oha_factory_create_stream(pjmedia_aud_dev_factory *f,
                                             const pjmedia_aud_param *param,
                                             pjmedia_aud_rec_cb rec_cb,
                                             pjmedia_aud_play_cb play_cb,
                                             void *user_data,
                                             pjmedia_aud_stream **p_strm);

/* ---------- 流操作 ---------- */
static pj_status_t oha_stream_start(pjmedia_aud_stream *strm);
static pj_status_t oha_stream_stop (pjmedia_aud_stream *strm);
static pj_status_t oha_stream_destroy(pjmedia_aud_stream *strm);

/* ---------- 工厂操作表 ---------- */
static pjmedia_aud_dev_factory_op oha_factory_op = {
    &oha_factory_init,
    &oha_factory_destroy,
    &oha_factory_get_dev_count,
    &oha_factory_get_dev_info,
    &oha_factory_default_param,
    &oha_factory_create_stream,
    NULL
};

static pjmedia_aud_stream_op oha_stream_op = {
    NULL, NULL, NULL,
    &oha_stream_start,
    &oha_stream_stop,
    &oha_stream_destroy
};

/* ---------- 工厂入口 ---------- */
PJ_DEF(pjmedia_aud_dev_factory*) pjmedia_ohaudio_factory(pj_pool_factory *pf)
{
    pj_pool_t *pool = pj_pool_create(pf, "ohaudio", 512, 512, NULL);
    pjmedia_aud_dev_factory *f = PJ_POOL_ZALLOC_T(pool, pjmedia_aud_dev_factory);
    f->op = &oha_factory_op;
    // f->pf = pf;
    return f;
}

static pj_status_t oha_factory_init(pjmedia_aud_dev_factory *f)
{
    PJ_LOG(4,(THIS_FILE, "OHAudio factory initialized"));
    return PJ_SUCCESS;
}

static pj_status_t oha_factory_destroy(pjmedia_aud_dev_factory *f)
{
    // pj_pool_t *pool = ((char*)f) - sizeof(pj_pool_t);
    // pj_pool_release(pool);
    return PJ_SUCCESS;
}

static unsigned oha_factory_get_dev_count(pjmedia_aud_dev_factory *f)
{
    return 1;
}

static pj_status_t oha_factory_get_dev_info(pjmedia_aud_dev_factory *f,
                                            unsigned index,
                                            pjmedia_aud_dev_info *info)
{
    PJ_ASSERT_RETURN(index == 0, PJ_EINVAL);
    pj_bzero(info, sizeof(*info));
    strcpy(info->name, "OHAudio");
    strcpy(info->driver, "OHAudio");
    info->input_count  = 1;
    info->output_count = 1;
    info->default_samples_per_sec = 48000;
    return PJ_SUCCESS;
}

static pj_status_t oha_factory_default_param(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_param *param)
{
    PJ_ASSERT_RETURN(index == 0, PJ_EINVAL);
    pj_bzero(param, sizeof(*param));
    param->dir           = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param->rec_id        = 0;
    param->play_id       = 0;
    param->clock_rate    = 48000;
    param->channel_count = 2;
    param->samples_per_frame = 480;   /* 10 ms @ 48 kHz */
    param->bits_per_sample   = 16;
    return PJ_SUCCESS;
}

/* ---------- 流创建 ---------- */
static pj_status_t oha_factory_create_stream(pjmedia_aud_dev_factory *f,
                                             const pjmedia_aud_param *param,
                                             pjmedia_aud_rec_cb rec_cb,
                                             pjmedia_aud_play_cb play_cb,
                                             void *user_data,
                                             pjmedia_aud_stream **p_strm)
{
    // // pj_pool_t *pool = pj_pool_create(f->pf, "oha_stream", 1024, 1024, nullptr);
    // pj_pool_t *pool = NULL;
    // oha_stream *strm = PJ_POOL_ZALLOC_T(pool, oha_stream);
    // strm->pool = pool;
    // strm->param = *param;
    // strm->rec_cb = rec_cb;
    // strm->rec_user_data = user_data;
    // strm->play_cb = play_cb;
    // strm->play_user_data = user_data;

    // OH_AudioStreamBuilder *builder;

    // /* 采集 */
    // if (param->dir & PJMEDIA_DIR_CAPTURE) {
    //     // OH_AudioCapturer_Callbacks cbs = {
    //     //     .OH_AudioCapturer_OnReadData =
    //     //         [](OH_AudioCapturer*, void *usr, void *buf, int32_t len) -> int32_t {
    //     //             oha_stream *s = (oha_stream*)usr;
    //     //             pjmedia_frame frame = {PJMEDIA_FRAME_TYPE_AUDIO, buf, (unsigned)len};
    //     //             s->rec_cb(s->rec_user_data, &frame);
    //     //             return 0;
    //     //         },
    //     //     .OH_AudioCapturer_OnError = nullptr
    //     // };
    //     OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_CAPTURER);
    //     OH_AudioStreamBuilder_SetSamplingRate(builder, param->clock_rate);
    //     OH_AudioStreamBuilder_SetChannelCount(builder, param->channel_count);
    //     OH_AudioStreamBuilder_SetSampleFormat(builder, AUDIOSTREAM_SAMPLE_S16LE);
    //     OH_AudioStreamBuilder_SetCapturerCallback(builder, NULL, strm);
    //     // OH_AudioStreamBuilder_CreateCapturer(builder, &strm->capturer);
    // }

    // /* 播放 */
    // if (param->dir & PJMEDIA_DIR_PLAYBACK) {
    //     // OH_AudioRenderer_Callbacks cbs = {
    //     //     .OH_AudioRenderer_OnWriteData =
    //     //         [](OH_AudioRenderer*, void *usr, void *buf, int32_t len) -> int32_t {
    //     //             oha_stream *s = (oha_stream*)usr;
    //     //             pjmedia_frame frame = {PJMEDIA_FRAME_TYPE_AUDIO, buf, (unsigned)len};
    //     //             s->play_cb(s->play_user_data, &frame);
    //     //             return 0;
    //     //         },
    //     //     .OH_AudioRenderer_OnError = nullptr
    //     // };
    //     OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER);
    //     OH_AudioStreamBuilder_SetSamplingRate(builder, param->clock_rate);
    //     OH_AudioStreamBuilder_SetChannelCount(builder, param->channel_count);
    //     OH_AudioStreamBuilder_SetSampleFormat(builder, AUDIOSTREAM_SAMPLE_S16LE);
    //     OH_AudioStreamBuilder_SetLatencyMode(builder, AUDIOSTREAM_LATENCY_MODE_FAST);
    //     OH_AudioStreamBuilder_SetRendererCallback(builder, NULL, strm);
    //     // OH_AudioStreamBuilder_CreateRenderer(builder, &strm->renderer);
    // }

    // strm->base.op = &oha_stream_op;
    // *p_strm = &strm->base;
    return PJ_SUCCESS;
}

/* ---------- 流控制 ---------- */
static pj_status_t oha_stream_start(pjmedia_aud_stream *strm)
{
    // oha_stream *s = (oha_stream*)strm;
    // if (s->param.dir & PJMEDIA_DIR_CAPTURE)
    //     OH_AudioCapturer_Start(s->capturer);
    // if (s->param.dir & PJMEDIA_DIR_PLAYBACK)
    //     OH_AudioRenderer_Start(s->renderer);
    // s->running = PJ_TRUE;
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_stop(pjmedia_aud_stream *strm)
{
    // oha_stream *s = (oha_stream*)strm;
    // if (s->param.dir & PJMEDIA_DIR_CAPTURE)
    //     OH_AudioCapturer_Stop(s->capturer);
    // if (s->param.dir & PJMEDIA_DIR_PLAYBACK)
    //     OH_AudioRenderer_Stop(s->renderer);
    // s->running = PJ_FALSE;
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_destroy(pjmedia_aud_stream *strm)
{
    // oha_stream *s = (oha_stream*)strm;
    // oha_stream_stop(strm);
    // if (s->capturer) OH_AudioCapturer_Stop(s->capturer);
    // if (s->renderer) OH_AudioRenderer_Stop(s->renderer);
    // pj_pool_release(s->pool);
    return PJ_SUCCESS;
}