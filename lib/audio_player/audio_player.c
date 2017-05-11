/*
 * audio_player.c
 *
 *  Created on: 12.03.2017
 *      Author: michaelboeckling
 */

#include <stdlib.h>
#include "freertos/FreeRTOS.h"

#include "audio_player.h"
#include "spiram_fifo.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"

#include "mp3_decoder.h"

#define TAG "audio_player"
#define PRIO_MAD configMAX_PRIORITIES - 2

static int start_decoder_task(player_t *player)
{
    TaskFunction_t task_func;
    char * task_name;
    uint16_t stack_depth;

    printf("starting decoder\n");


    ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());

    switch (player->media_stream->content_type) {
        case AUDIO_MPEG:
            task_func = mp3_decoder_task;
            task_name = "mp3_decoder_task";
            stack_depth = 8448;
            break;

        default:
            ESP_LOGE(TAG, "unknown mime type: %d", player->media_stream->content_type);
            return -1;
    }

    if (xTaskCreatePinnedToCore(task_func, task_name, stack_depth, player,
    PRIO_MAD, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "ERROR creating decoder task! Out of memory?");
        return -1;
    } else {
        player->decoder_status = RUNNING;
    }

    ESP_LOGI(TAG, "created decoder task: %s", task_name);

    return 0;
}

static int t;

/* Writes bytes into the FIFO queue, starts decoder task if necessary. */
int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read,
        void *user_data)
{
  printf("audiostreamconsumer\n");

    player_t *player = user_data;

    // don't bother consuming bytes if stopped
    if(player->command == CMD_STOP) {
        player->decoder_command = CMD_STOP;
        player->command = CMD_NONE;
        return -1;
    }

printf("audiostreamconsumer1\n");
    if (bytes_read > 0) {
      printf("audiostreamconsumer1\n");
        spiRamFifoWrite(recv_buf, bytes_read);
        printf("audiostreamconsumer1\n");
    }
    printf("audiostreamconsumer2\n");

    int bytes_in_buf = spiRamFifoFill();
    uint8_t fill_level = (bytes_in_buf * 100) / spiRamFifoLen();

    // seems 4k is enough to prevent initial buffer underflow
    uint8_t min_fill_lvl = player->buffer_pref == FAST ? 20 : 90;
    bool buffer_ok = fill_level > min_fill_lvl;
    if (player->decoder_status != RUNNING && buffer_ok) {
printf("audiostreamconsumer\n");
        // buffer is filled, start decoder
        if (start_decoder_task(player) != 0) {
            ESP_LOGE(TAG, "failed to start decoder task");
            return -1;
        }
    }

    t = (t + 1) & 255;
    if (t == 0) {
        printf("Buffer fill %u%%, %d bytes\n", fill_level, bytes_in_buf);
    }

    return 0;
}

void audio_player_init(player_t *player)
{
    // initialize I2S
    audio_renderer_init(player->renderer_config);
    player->status = INITIALIZED;
}

void audio_player_destroy(player_t *player)
{
    // halt I2S
    audio_renderer_destroy(player->renderer_config);
    player->status = UNINITIALIZED;
}

void audio_player_start(player_t *player)
{
    audio_renderer_start(player->renderer_config);
    player->status = RUNNING;
}

void audio_player_stop(player_t *player)
{
    audio_renderer_stop(player->renderer_config);
    player->status = STOPPED;
}
