#include "SDLMovieAudioFactory.h"

#include <stdexcept>

#include <SDL_audio.h>


class SDLMovieAudioDecoder : public Video::MovieAudioDecoder
{
public:
    virtual void adjustAudioSettings(AVSampleFormat& sampleFormat, uint64_t& channelLayout, int& sampleRate);

    ~SDLMovieAudioDecoder();
    SDLMovieAudioDecoder(Video::VideoState* videoState);
};

void MyAudioCallback(void*, Uint8*, int);

void SDLMovieAudioDecoder::adjustAudioSettings(AVSampleFormat& sampleFormat, uint64_t& channelLayout, int& sampleRate)
{
    // For simplicity, we use fixed settings here and have SDL handle resampling if necessary.
    // This is not optimal, since we might have resampling done on SDL's side *and* the audio decoder side in the worst case.
    
    // We could also write the function parameters to the "want" struct,
    // then adjust the function parameters based on the "obtained" struct SDL_OpenAudio returns us.
    // This means SDL wouldn't have to do any resampling. The MovieAudioDecoder may still have to do resampling if the HW doesn't support the wanted format.

    sampleFormat = AV_SAMPLE_FMT_FLT;
    channelLayout = AV_CH_LAYOUT_STEREO;

    SDL_AudioSpec want;
    want.freq = sampleRate;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 4096;
    want.userdata = this;
    want.callback = MyAudioCallback;

    SDL_AudioSpec have;

    if (SDL_OpenAudio(&want, &have) < 0)
        throw std::runtime_error(std::string("Failed to open audio: ") + SDL_GetError());
    else
        SDL_PauseAudio(0);
}

SDLMovieAudioDecoder::~SDLMovieAudioDecoder()
{
    std::cout << "close audio " << std::endl;
    SDL_CloseAudio();
}

SDLMovieAudioDecoder::SDLMovieAudioDecoder(Video::VideoState* videoState)
    : MovieAudioDecoder(videoState)
{
}

boost::shared_ptr<Video::MovieAudioDecoder> SDLMovieAudioFactory::createDecoder(Video::VideoState* videoState)
{
    boost::shared_ptr<Video::MovieAudioDecoder> ptr (new SDLMovieAudioDecoder(videoState));
    return ptr;
}


void MyAudioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLMovieAudioDecoder *decoder = (SDLMovieAudioDecoder *)userdata;
    size_t read = decoder->read((char*)stream, len);
    if (read < len)
    {
        // we don't have enough data, which means EOF was reached
        // fill the rest of the buffer with silence
        size_t sampleSize = av_get_bytes_per_sample(decoder->getOutputSampleFormat());
        Uint8* data[1];
        data[0] = stream;
        av_samples_set_silence((uint8_t**)stream+read, 0, (len-read)/sampleSize, 1, decoder->getOutputSampleFormat());
    }
}