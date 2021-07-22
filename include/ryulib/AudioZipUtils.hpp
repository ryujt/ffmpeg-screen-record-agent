#ifndef AUDIOZIP_UTILS_HPP
#define AUDIOZIP_UTILS_HPP


const int CHANNEL = 1;
const int SAMPLE_RATE = 48000;
const int SAMPLE_SIZE = 4;
const int FRAMES_PER_BUFFER = 5760;
const int BITRATE = 64000;
const int AUDIO_DATA_SIZE = CHANNEL * FRAMES_PER_BUFFER * SAMPLE_SIZE;

// 최대 지연 시간 120 * 4 ms
const int MAX_DELAY_LIMIT_COUNT = 4;

const int ERROR_OPEN_ENCODER             = -7;
const int ERROR_OPEN_DECODE              = -8;


#endif  // AUDIOZIP_UTILS_HPP