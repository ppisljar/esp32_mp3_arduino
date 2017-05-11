#ifndef AudioPlayer_h
#define AudioPlayer_h

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

typedef enum {
    IDLE, PLAYING, BUFFER_UNDERRUN, FINISHED, STOPPED
} player_state_t;

typedef enum {
     RENDER_ACTIVE, RENDER_STOPPED
} renderer_state_t;

typedef struct
{
    int sample_rate;
    float sample_rate_modifier;
    i2s_bits_per_sample_t bit_depth;
    i2s_port_t i2s_num;
} renderer_config_t;

typedef struct {
    volatile player_state_t state;
    renderer_config_t *renderer_config;
} player_t;

typedef struct {
    char *url;
    player_t *player_config;
} web_radio_t;

extern "C" {
  void set_dac_sample_rate(int rate);
  void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels);
}

#ifdef __cplusplus

class AudioPlayer
{
  public:
    AudioPlayer();
    void init(web_radio_t* config);
    void start();
    void stop();

  private:
    uint8_t _state;
};
#endif

#endif
