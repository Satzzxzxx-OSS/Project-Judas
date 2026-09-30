#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

/* Diagnostic uses the same backend functions as its actual spatializer. */
float judas_audio_distance_gain(int model,float distance,float reference,float maximum,float rolloff) {
    if(model==1)return ma_attenuation_inverse(distance,reference,maximum,rolloff);
    if(model==2)return ma_attenuation_linear(distance,reference,maximum,rolloff);
    return 1.0f;
}
