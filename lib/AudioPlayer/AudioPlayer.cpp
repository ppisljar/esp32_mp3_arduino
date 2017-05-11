#include "AudioPlayer.h"
#include "../mp3_decoder/mp3_decoder.h"
#include "../url_parser/url_parser.h"
#include "../fifo/spiram_fifo.h"
#include "../http/http.h"

#define PRIO_MAD configMAX_PRIORITIES - 2

static int t;
static bool mad_started = false;

static renderer_config_t *curr_config;
static renderer_state_t state;

static TaskHandle_t *reader_task;

static short convert_16bit_stereo_to_8bit_stereo(short left, short right)
{
    unsigned short sample = (unsigned short) left;
    sample = (sample << 8 & 0xff00) | (((unsigned short) right >> 8) & 0x00ff);
    return sample;
}

static int convert_16bit_stereo_to_16bit_stereo(short left, short right)
{
    unsigned int sample = (unsigned short) left;
    sample = (sample << 16 & 0xffff0000) | ((unsigned short) right);
    return sample;
}

/* render callback for the libmad synth */
void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{

    // if mono: simply duplicate the left channel
    if(num_channels == 1) {
        sample_buff_ch1 = sample_buff_ch0;
    }

    // max delay: 50ms instead of portMAX_DELAY
    TickType_t delay = 50 / portTICK_PERIOD_MS;

    switch(curr_config->bit_depth) {

        case I2S_BITS_PER_SAMPLE_8BIT:
            for (int i=0; i < num_samples; i++) {

                if(state == RENDER_STOPPED)
                    break;

                short samp8 = convert_16bit_stereo_to_8bit_stereo(sample_buff_ch0[i], sample_buff_ch1[i]);
                int bytes_pushed = i2s_push_sample(curr_config->i2s_num,  (char *)&samp8, delay);

                // DMA buffer full - retry
                if(bytes_pushed == 0) {
                    i--;
                }
            }
            break;

        case I2S_BITS_PER_SAMPLE_16BIT:
            for (int i=0; i < num_samples; i++) {

                if(state == RENDER_STOPPED)
                    break;

                int samp16 = convert_16bit_stereo_to_16bit_stereo(sample_buff_ch0[i], sample_buff_ch1[i]);
                int bytes_pushed = i2s_push_sample(curr_config->i2s_num,  (char *)&samp16, delay);

                // DMA buffer full - retry
                if(bytes_pushed == 0) {
                    i--;
                }
            }
            break;

        case I2S_BITS_PER_SAMPLE_24BIT:
            // TODO
            break;

        case I2S_BITS_PER_SAMPLE_32BIT:
            // TODO
            break;
    }
}

// Called by the NXP modifications of libmad. Sets the needed output sample rate.
static int prevRate;
void set_dac_sample_rate(int rate)
{
    if(rate == prevRate)
        return;
    prevRate = rate;

    // modifier will usually be 1.0
    rate = rate * curr_config->sample_rate_modifier;

    i2s_set_sample_rates(curr_config->i2s_num, rate);
}

/* pushes bytes into the FIFO queue, starts decoder task if necessary */
int audio_stream_consumer(char *recv_buf, ssize_t bytes_read, void *user_data)
{
    player_t *player = (player_t*)user_data;

    // don't bother consuming bytes if stopped
    if(player->state == STOPPED) {
        Serial.println("player is stopped");
        // TODO: add proper synchronization, this is just an assumption
        mad_started = false;
        return -1;
    }

    if (bytes_read > 0) {
        spiRamFifoWrite(recv_buf, bytes_read);
    }

    int bytes_in_buf = spiRamFifoFill();

    // seems 4k is enough to prevent initial buffer underflow
    if (!mad_started && player->state == PLAYING && (bytes_in_buf > 4096) )
    {
        mad_started = true;
        //Buffer is filled. Start up the MAD task.
        // TODO: 6300 not enough?
        if (xTaskCreatePinnedToCore(&mp3_decoder_task, "tskmad", 8000, player, PRIO_MAD, NULL, 1) != pdPASS)
        {
            printf("ERROR creating MAD task! Out of memory?\n");
        } else {
            printf("created MAD task\n");
        }
    }


    t = (t+1) & 255;
    if (t == 0) {
        // printf("Buffer fill %d, buff underrun ct %d\n", spiRamFifoFill(), (int)bufUnderrunCt);
        uint8_t fill_level = (bytes_in_buf * 100) / spiRamFifoLen();
        printf("Buffer fill %u%%, %d bytes\n", fill_level, bytes_in_buf);
    }

    return 0;
}

static void http_get_task(void *pvParameters)
{
    web_radio_t *radio_conf = (web_radio_t*)pvParameters;

    /* parse URL */
    url_t *url = url_create(radio_conf->url);

    // blocks until end of stream
    int result = http_client_get(
            url->host, url->port, url->path,
            audio_stream_consumer,
            radio_conf->player_config);


    printf("http_client_get stack: %d\n", uxTaskGetStackHighWaterMark(NULL));


    url_free(url);

    vTaskDelete(NULL);
}

// starts task that gets data from the server and pushes it to the buffer
// also starts mp3_decoder task, which reads the buffer and sends data to
// audio render (writes to i2s)

AudioPlayer::AudioPlayer() {

}

// inits the player to the provided config
web_radio_t* _radio_config;
void AudioPlayer::init(web_radio_t* config) {
  _radio_config = config;
  curr_config = _radio_config->player_config->renderer_config;
  state = RENDER_STOPPED;
  if (!spiRamFifoInit()) {
      printf("\n\nSPI RAM chip fail!\n");
      while(1);
  }
}

// starts all the tasks
void AudioPlayer::start() {
  _radio_config->player_config->state = PLAYING;
  state = RENDER_ACTIVE;
  i2s_start(curr_config->i2s_num);
  i2s_zero_dma_buffer(curr_config->i2s_num);
  xTaskCreatePinnedToCore(&http_get_task, "http_get_task", 2048, _radio_config, 20, reader_task, 0);
  //xTaskCreatePinnedToCore(&mp3_decoder_task, "tskmad", 8000, config->player_config, PRIO_MAD, NULL, 1);
}

// stops all the tasks
void AudioPlayer::stop() {
  state = RENDER_STOPPED;
  i2s_stop(curr_config->i2s_num);
}
