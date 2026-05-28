/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavassrenderer.h"
#include "qavcodec_p.h"

#include <QDebug>

extern "C" {
#include <ass/ass.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

class QAVASSRendererPrivate
{
    Q_DECLARE_PUBLIC(QAVASSRenderer)
public:
    QAVASSRendererPrivate(QAVASSRenderer *q)
        : q_ptr(q)
    {
    }

    QAVASSRenderer *q_ptr = nullptr;
    ASS_Library *library = nullptr;
    ASS_Renderer *renderer = nullptr;
    ASS_Track *track = nullptr;
    QImage image;
};

QAVASSRenderer::QAVASSRenderer()
    : d_ptr(new QAVASSRendererPrivate(this))
{
}

QAVASSRenderer::~QAVASSRenderer()
{
    unload();
}

int QAVASSRenderer::load(const QAVStream &stream)
{
    Q_D(QAVASSRenderer);
    unload();
    d->library = ass_library_init();
    if (!d->library) {
        qWarning() << "Could not initialize libass";
        return AVERROR(EINVAL);
    }
    d->renderer = ass_renderer_init(d->library);
    if (!d->renderer) {
        qWarning() << "Could not initialize libass renderer";
        return AVERROR(EINVAL);
    }
    ass_set_fonts(d->renderer, NULL, NULL, 1, NULL, 1);
    d->track = ass_new_track(d->library);
    if (!d->track) {
        qWarning() << "Could not create a libass track";
        return AVERROR(EINVAL);
    }
    auto dec_ctx = stream.codec()->avctx();
    if (dec_ctx->subtitle_header) {
        ass_process_codec_private(d->track,
                                  (char*)dec_ctx->subtitle_header,
                                  dec_ctx->subtitle_header_size);
    }
    return 0;
}

void QAVASSRenderer::unload()
{
    Q_D(QAVASSRenderer);
    if (d->track)
        ass_free_track(d->track);
    if (d->renderer)
        ass_renderer_done(d->renderer);
    if (d->library)
        ass_library_done(d->library);
    d->track = nullptr;
    d->renderer = nullptr;
    d->library = nullptr;
}

QImage QAVASSRenderer::toImage(const QAVSubtitleFrame &frame, int width, int height)
{
    Q_D(QAVASSRenderer);
    if (!d->renderer)
        return d->image;
    auto sub = frame.subtitle();
    const int64_t start_time = av_rescale_q(sub->pts, AV_TIME_BASE_Q, av_make_q(1, 1000));
    const int64_t duration = sub->end_display_time;
    ass_set_frame_size(d->renderer, width, height);
    for (size_t i = 0; i < sub->num_rects; ++i) {
        char *ass_line = sub->rects[i]->ass;
        if (!ass_line)
            break;
        ass_process_chunk(d->track, ass_line, strlen(ass_line), start_time, duration);
    }
    int detect_change = 0;
    ASS_Image *image = ass_render_frame(d->renderer, d->track, start_time, &detect_change);
    if (detect_change == 0)
        return d->image;
    d->image = {width, height, QImage::Format_RGBA8888_Premultiplied};
    d->image.fill(Qt::transparent);

    for (ASS_Image *i = image; i; i = i->next) {
        int r = (i->color >> 24) & 0xFF;
        int g = (i->color >> 16) & 0xFF;
        int b = (i->color >> 8) & 0xFF;
        int a = 255 - (i->color & 0xFF); // libass uses inverted alpha

        for (int y = 0; y < i->h; ++y) {
            for (int x = 0; x < i->w; ++x) {
                int src_alpha = i->bitmap[y * i->stride + x];
                if (src_alpha == 0) continue;
                int dst_x = i->dst_x + x;
                int dst_y = i->dst_y + y;
                if (dst_x < 0 || dst_y < 0 || dst_x >= width || dst_y >= height)
                    continue;

                QColor dst = d->image.pixelColor(dst_x, dst_y);
                float alpha = (src_alpha / 255.0f) * (a / 255.0f);
                int out_r = r * alpha + dst.red() * (1 - alpha);
                int out_g = g * alpha + dst.green() * (1 - alpha);
                int out_b = b * alpha + dst.blue() * (1 - alpha);
                d->image.setPixelColor(dst_x, dst_y, QColor(out_r, out_g, out_b, 255));
            }
        }
    }

    return d->image;
}

void QAVASSRenderer::flush()
{
    Q_D(QAVASSRenderer);
    if (d->track)
        ass_flush_events(d->track);
    d->image = {};
}

QT_END_NAMESPACE
