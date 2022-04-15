#include "scoff.h"

#include <stdio.h>
#include <stdlib.h>

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

void free_audio_data(const audio_data_t ad)
{
    free(ad.data);
}

int decode_audio_file(const char* in_path, audio_data_t* out_data)
{
    AVFormatContext* in_format;
    AVStream* in_stream;
    AVCodecContext* in_codec;
    AVPacket in_packet;
    AVFrame* in_frame;
    struct SwrContext* swr;

    // initialize all for libavformat
    av_register_all();

    // get format from audio file
    in_format = avformat_alloc_context();
    if (avformat_open_input(&in_format, in_path, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file '%s'\n", in_path);
        avformat_free_context(in_format);
        return -1;
    }
    if (avformat_find_stream_info(in_format, NULL) < 0) {
        fprintf(stderr, "Could not retrieve stream info from file '%s'\n", in_path);
        avformat_free_context(in_format);
        return -1;
    }

    // Find the index of the (first) audio stream
    int stream_index = -1;
    for (int i=0; i < in_format->nb_streams; i++) {
        if (in_format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }
    if (stream_index == -1) {
        fprintf(stderr, "Could not retrieve audio stream from file '%s'\n", in_path);
        avformat_free_context(in_format);
        return -1;
    }
    in_stream = in_format->streams[stream_index];

    // find and open codec
    in_codec = in_stream->codec;
    if (avcodec_open2(in_codec, avcodec_find_decoder(in_codec->codec_id), NULL) < 0) {
        fprintf(stderr, "Failed to open decoder for stream #%u in file '%s'\n", stream_index, in_path);
        avformat_free_context(in_format);
        return -1;
    }

    if (out_data->sample_rate < 0) {
        out_data->sample_rate = in_codec->sample_rate;
    }
    if (out_data->channels < 0) {
        out_data->channels = 2;
    }

    // prepare resampler
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_count", in_codec->channels, 0);
    av_opt_set_int(swr, "out_channel_count", out_data->channels, 0);
    av_opt_set_int(swr, "in_channel_layout", in_codec->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(swr, "in_sample_rate", in_codec->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", out_data->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", in_codec->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_DBL, 0);
    swr_init(swr);
    if (!swr_is_initialized(swr)) {
        fprintf(stderr, "Resampler has not been properly initialized\n");
        swr_free(swr);
        avformat_free_context(in_format);
        return -1;
    }

    // prepare to read data
    av_init_packet(&in_packet);
    in_frame = av_frame_alloc();
    if (!in_frame) {
        fprintf(stderr, "Error allocating the frame\n");
        return -1;
    }

    // iterate through frames
    out_data->data = NULL;
    out_data->size = 0;
    while (av_read_frame(in_format, &in_packet) >= 0) {
        // decode one frame
        int gotFrame;
        if (avcodec_decode_audio4(in_codec, in_frame, &gotFrame, &in_packet) < 0) {
            break;
        }
        if (!gotFrame) {
            continue;
        }
        // resample frames
        double* buffer;
        av_samples_alloc((uint8_t **) &buffer, NULL, out_data->channels, in_frame->nb_samples, AV_SAMPLE_FMT_DBL, 0);
        int frame_count = swr_convert(swr, (uint8_t**) &buffer, in_frame->nb_samples, (const uint8_t**) in_frame->data, in_frame->nb_samples);
        // append resampled frames to data
        out_data->data = (double*) realloc(out_data->data, (out_data->size + in_frame->nb_samples) * sizeof(double));
        memcpy(out_data->data + out_data->size, buffer, frame_count * sizeof(double));
        out_data->size += frame_count;
    }

    // clean up
    av_frame_free(&in_frame);
    swr_free(&swr);
    avcodec_close(in_codec);
    avformat_free_context(in_format);

    // success
    return 0;
}

int encode_video_file(const char* out_path, const audio_data_t in_data)
{

}
