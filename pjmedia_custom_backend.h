#ifndef __PJMEDIA_CUSTOM_AUDIO_BACKEND_H__
#define __PJMEDIA_CUSTOM_AUDIO_BACKEND_H__

#include <pjmedia-audiodev/audiodev.h>
#include <pjmedia-audiodev/audiodev_imp.h>

class CustomAudioBackend{
public:
    CustomAudioBackend(){}

    ~CustomAudioBackend(){}

    virtual pj_status_t factoryInit(pjmedia_aud_dev_factory *f) = 0;
    virtual pj_status_t factoryDestroy(pjmedia_aud_dev_factory *f) = 0;
    virtual pj_status_t factoryRefresh(pjmedia_aud_dev_factory *f) = 0;
    virtual unsigned factoryGetDevCount(pjmedia_aud_dev_factory *f) = 0;

    virtual pj_status_t factoryGetDevInfo(pjmedia_aud_dev_factory *f,
                                            unsigned index,
                                            pjmedia_aud_dev_info *info) = 0;

    virtual pj_status_t factoryDefaultParam(pjmedia_aud_dev_factory *f,
                                             unsigned index,
                                             pjmedia_aud_param *param) = 0;

    virtual pj_status_t factoryCreateStream(pjmedia_aud_dev_factory *f,
                                             const pjmedia_aud_param *param,
                                             pjmedia_aud_rec_cb rec_cb,
                                             pjmedia_aud_play_cb play_cb,
                                             void *user_data,
                                             pjmedia_aud_stream **p_strm) = 0;

    /* ---------- 流操作 ---------- */
    virtual pj_status_t streamStart(pjmedia_aud_stream *strm) = 0;

    virtual pj_status_t streamStop(pjmedia_aud_stream *strm) = 0;

    virtual pj_status_t streamDestroy(pjmedia_aud_stream *strm) = 0;

    virtual pj_status_t streamGetParam(pjmedia_aud_stream *strm,
                                        pjmedia_aud_param *param) = 0;

    virtual pj_status_t streamGetCap(pjmedia_aud_stream *strm,
                                        pjmedia_aud_dev_cap cap,
                                        void *value) = 0;

    virtual pj_status_t streamSetCap(pjmedia_aud_stream *strm,
                                        pjmedia_aud_dev_cap cap,
                                        const void *value) = 0;
};

#endif