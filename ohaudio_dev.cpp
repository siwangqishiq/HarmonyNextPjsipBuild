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
typedef struct oha_audio_factory{
    pjmedia_aud_dev_factory      base;
    pj_pool_t                   *pool;
    pj_pool_factory             *pf;
    unsigned                     dev_count;

    pjmedia_aud_dev_info         info;
};

typedef struct oha_stream {
    pjmedia_aud_stream   base;
    pj_pool_t           *pool;
    pjmedia_aud_param    param;
    pjmedia_aud_rec_cb   rec_cb;
    pjmedia_aud_play_cb  play_cb;

    void                *user_data;
} oha_stream;

/* ---------- 工厂函数 ---------- */
static pj_status_t oha_factory_init(pjmedia_aud_dev_factory *f);
static pj_status_t oha_factory_destroy(pjmedia_aud_dev_factory *f);
static pj_status_t oha_factory_refresh(pjmedia_aud_dev_factory *f);
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

static pj_status_t oha_stream_get_param(pjmedia_aud_stream *strm,pjmedia_aud_param *param);

static pj_status_t oha_stream_get_cap(pjmedia_aud_stream *strm,pjmedia_aud_dev_cap cap,void *value);

static pj_status_t oha_stream_set_cap(pjmedia_aud_stream *strm,pjmedia_aud_dev_cap cap,const void *value);

/* ---------- 工厂操作表 ---------- */
static pjmedia_aud_dev_factory_op oha_factory_op = {
    &oha_factory_init,
    &oha_factory_destroy,
    &oha_factory_get_dev_count,
    &oha_factory_get_dev_info,
    &oha_factory_default_param,
    &oha_factory_create_stream,
    &oha_factory_refresh
};

static pjmedia_aud_stream_op oha_stream_op = {
    &oha_stream_get_param, 
    &oha_stream_get_cap, 
    &oha_stream_set_cap,
    &oha_stream_start,
    &oha_stream_stop,
    &oha_stream_destroy
};

/* ---------- 工厂入口 ---------- */

//创建工厂类
#ifdef __cplusplus
extern "C"{
#endif
PJ_DEF(pjmedia_aud_dev_factory*) pjmedia_ohaudio_factory(pj_pool_factory *pf)
{
    struct oha_audio_factory *f;
    pj_pool_t *pool = pj_pool_create(pf, "ohaudio", 1024, 1024, NULL);
    f = PJ_POOL_ZALLOC_T(pool,struct oha_audio_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &oha_factory_op;
    
    return &f->base;
}
#ifdef __cplusplus
}
#endif

static pj_status_t oha_factory_init(pjmedia_aud_dev_factory *f)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;

    oha_factory->dev_count = 1;
    
    pj_bzero(&oha_factory->info, sizeof(pjmedia_aud_dev_info));
    strcpy(oha_factory->info.name, "ohaudio device");
    strcpy(oha_factory->info.driver, "siwangqishiq");
    oha_factory->info.input_count = 1;
    oha_factory->info.output_count = 1;
    oha_factory->info.default_samples_per_sec = 48000;
    oha_factory->info.caps = 0;
    return PJ_SUCCESS;
}

static pj_status_t oha_factory_destroy(pjmedia_aud_dev_factory *f)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;
    pj_pool_safe_release(&oha_factory->pool);
    return PJ_SUCCESS;
}

static pj_status_t oha_factory_refresh(pjmedia_aud_dev_factory *f)
{
    return PJ_SUCCESS;
}

static unsigned oha_factory_get_dev_count(pjmedia_aud_dev_factory *f)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;
    return oha_factory->dev_count;
}

static pj_status_t oha_factory_get_dev_info(pjmedia_aud_dev_factory *f,
                                            unsigned index,
                                            pjmedia_aud_dev_info *info)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;
    pj_memcpy(info, &oha_factory->info, sizeof(pjmedia_aud_dev_info));
    return PJ_SUCCESS;
}

static pj_status_t oha_factory_default_param(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_param *param)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;
    pj_bzero(param, sizeof(pjmedia_aud_param));

    param->dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
    param->rec_id = index;
    param->play_id = index;

    param->clock_rate = oha_factory->info.default_samples_per_sec;
    param->channel_count = 1;
    param->samples_per_frame = oha_factory->info.default_samples_per_sec * 20 / 1000;
    param->bits_per_sample = 16;

    /* Set the device capabilities here */
    param->flags = 0;
    return PJ_SUCCESS;
}

/* ---------- 流创建 ---------- */
static pj_status_t oha_factory_create_stream(pjmedia_aud_dev_factory *f,
                                             const pjmedia_aud_param *param,
                                             pjmedia_aud_rec_cb rec_cb,
                                             pjmedia_aud_play_cb play_cb,
                                             void *user_data,
                                             pjmedia_aud_stream **p_aud_strm)
{
    struct oha_audio_factory *oha_factory = (struct oha_audio_factory *)f;
    pj_pool_t *pool;
    struct oha_stream *strm;

    pool = pj_pool_create(oha_factory->pf, "ohaudio-dev", 1024, 1024, NULL);
    strm = PJ_POOL_ZALLOC_T(pool, struct oha_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));

    strm->pool = pool;
    strm->rec_cb = rec_cb;
    strm->play_cb = play_cb;
    strm->user_data = user_data;

     /* Create player stream here */
    if (param->dir & PJMEDIA_DIR_PLAYBACK) {
    }

    /* Create capture stream here */
    if (param->dir & PJMEDIA_DIR_CAPTURE) {
    }

    /* Done */
    strm->base.op = &oha_stream_op;
    *p_aud_strm = &strm->base;

    return PJ_SUCCESS;
}

/* ---------- 流控制 ---------- */
static pj_status_t oha_stream_start(pjmedia_aud_stream *strm)
{
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_stop(pjmedia_aud_stream *strm)
{
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_destroy(pjmedia_aud_stream *strm)
{
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_get_param(pjmedia_aud_stream *strm,pjmedia_aud_param *param){
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_get_cap(pjmedia_aud_stream *strm,pjmedia_aud_dev_cap cap,void *value){
    return PJ_SUCCESS;
}

static pj_status_t oha_stream_set_cap(pjmedia_aud_stream *strm,pjmedia_aud_dev_cap cap,const void *value){
    return PJ_SUCCESS;
}