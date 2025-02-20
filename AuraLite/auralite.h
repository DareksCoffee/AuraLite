/*
A LightWeight WAV sound player.


Contributors:
    - Darek (Main Developer)
    - My cat Pichu (Emotional Support)
*/


#ifndef AURALITE_H
#define AURALITE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
#elif __linux__
    #include <alsa/asoundlib.h>
#elif __APPLE__
    #include <CoreAudio/CoreAudio.h>
    #include <AudioToolbox/AudioToolbox.h>
#endif

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BITS_PER_SAMPLE 16

#define AURALITE_ERROR(msg) do { fprintf(stderr, "[AURALITE ERROR] %s\n", msg); exit(1); } while(0)
#define AURALITE_CHECK(cond, msg) do { if (!(cond)) AURALITE_ERROR(msg); } while(0)

typedef struct {
    uint8_t* data;
    uint32_t length;
    uint32_t position;
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
} AuraLite;

AuraLite load_wav(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        exit(1);
    }

    char chunkID[4];
    fread(chunkID, 1, 4, file);
    if (memcmp(chunkID, "RIFF", 4) != 0) {
        printf("Not a valid WAV file\n");
        fclose(file);
        exit(1);
    }

    fseek(file, 22, SEEK_SET);
    
    AuraLite sound;
    fread(&sound.channels, sizeof(uint16_t), 1, file);       
    fread(&sound.sampleRate, sizeof(uint32_t), 1, file);     

    fseek(file, 34, SEEK_SET);
    fread(&sound.bitsPerSample, sizeof(uint16_t), 1, file); 

    fseek(file, 40, SEEK_SET);
    fread(&sound.length, sizeof(uint32_t), 1, file);        

    sound.data = (uint8_t*)malloc(sound.length);
    if (!sound.data) {
        printf("Failed to allocate memory for sound data.\n");
        fclose(file);
        exit(1);
    }

    fread(sound.data, sound.length, 1, file);
    sound.position = 0;

    fclose(file);

    printf("Loaded WAV: %d Hz, %d-bit, %d channels\n",
           sound.sampleRate, sound.bitsPerSample, sound.channels);

    return sound;
}

#ifdef _WIN32
HWAVEOUT hWaveOut;
WAVEHDR waveHeader;

void Auralite_Init(AuraLite* sound) {
    WAVEFORMATEX waveFormat = {0};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = sound->channels;
    waveFormat.nSamplesPerSec = sound->sampleRate;
    waveFormat.wBitsPerSample = sound->bitsPerSample;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        printf("Failed to open Windows audio device\n");
        exit(1);
    }
}

void Auralite_Play(AuraLite* sound) {
    #ifdef _WIN32
        memset(&waveHeader, 0, sizeof(WAVEHDR));
        waveHeader.lpData = (LPSTR)sound->data;
        waveHeader.dwBufferLength = sound->length;
    
        waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR));
    
        Sleep((sound->length * 1000) / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8)));
    
    #elif __linux__
        int frames = sound->length / (CHANNELS * (BITS_PER_SAMPLE / 8));
        snd_pcm_writei(pcm_handle, sound->data, frames);
    
        sleep((sound->length * 1000) / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8)) / 1000);
    
    #elif __APPLE__
        Auralite_AudioCallback(sound, audioQueue, audioBuffer);
        AudioQueueStart(audioQueue, NULL);
    
        sleep((sound->length * 1000) / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8)) / 1000);
    
    #endif
    }
    
double Auralite_GetCurrentTime(AuraLite* sound) {
    return (double)sound->position / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8));
}

void Auralite_Jump(AuraLite* sound, double seconds) {

    uint32_t bytePosition = (uint32_t)(seconds * sound->sampleRate * sound->channels * (sound->bitsPerSample / 8));

    if (bytePosition >= sound->length) {
        sound->position = sound->length; 
    } else {
        sound->position = bytePosition;
    }
}

void Auralite_Close() {
    waveOutUnprepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
}

#elif __linux__
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;

void Auralite_Init(AuraLite* sound) {
    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        printf("Failed to open ALSA device\n");
        exit(1);
    }

    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);
}

void Auralite_Play(AuraLite* sound) {
    int frames = sound->length / (CHANNELS * (BITS_PER_SAMPLE / 8));
    snd_pcm_writei(pcm_handle, sound->data, frames);
}

double Auralite_GetCurrentTime(AuraLite* sound) {
    return (double)sound->position / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8));
}

void Auralite_Close() {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
}

#elif __APPLE__
AudioQueueRef audioQueue;
AudioQueueBufferRef audioBuffer;

void Auralite_AudioCallback(void* userdata, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    AuraLite* sound = (AuraLite*)userdata;

    if (sound->position + buffer->mAudioDataBytesCapacity > sound->length) {
        buffer->mAudioDataByteSize = sound->length - sound->position;
    } else {
        buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
    }

    memcpy(buffer->mAudioData, sound->data + sound->position, buffer->mAudioDataByteSize);
    sound->position += buffer->mAudioDataByteSize;

    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

void Auralite_Init(AuraLite* sound) {
    AudioStreamBasicDescription format = {0};
    format.mSampleRate = SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBytesPerPacket = (BITS_PER_SAMPLE / 8) * CHANNELS;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = (BITS_PER_SAMPLE / 8) * CHANNELS;
    format.mChannelsPerFrame = CHANNELS;
    format.mBitsPerChannel = BITS_PER_SAMPLE;

    AudioQueueNewOutput(&format, Auralite_AudioCallback, sound, NULL, NULL, 0, &audioQueue);

    AudioQueueAllocateBuffer(audioQueue, 4096, &audioBuffer);
}

void Auralite_Play(AuraLite* sound) {
    Auralite_AudioCallback(sound, audioQueue, audioBuffer);
    AudioQueueStart(audioQueue, NULL);
}

double Auralite_GetCurrentTime(AuraLite* sound) {
    return (double)sound->position / (sound->sampleRate * sound->channels * (sound->bitsPerSample / 8));
}
void Auralite_Close() {
    AudioQueueStop(audioQueue, true);
    AudioQueueDispose(audioQueue, true);
}

#endif

#endif // AURALITE_H
