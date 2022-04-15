#ifndef SCOFF_H
#define SCOFF_H

typedef struct audio_data
{
    int sample_rate;
    double* data;
    int size;
    int channels;
} audio_data_t;

void free_audio_data(const audio_data_t ad);

int decode_audio_file(const char* in_path, audio_data_t* out_data);

int encode_video_file(const char* out_path, const audio_data_t in_data);

#endif//SCOFF_H
