/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include "config.h"
#include <WiFi.h>
#include "driver/i2s.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#undef CONFIG_LOG_DEFAULT_LEVEL
#define CONFIG_LOG_DEFAULT_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
//#include "AudioPlayer.h"
#include "audio_player.h"
#include "audio_renderer.h"
#include "web_radio.h"
#include "../url_parser/url_parser.h"
#include "../mp3_decoder/mp3_decoder.h"
#include "../mad/mad.h"
#include "../mad/frame.h"
#include "../mad/stream.h"
#include "../fifo/spiram_fifo.h"
#include "../http/http.h"
#include "common_buffer.h"

void connect_to_wifi(char* ssid, char* pass) {
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if connecting failed.
    // This is due to incorrect credentials
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(pass);
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

static renderer_config_t *create_renderer_config()
{
    renderer_config_t *renderer_config = (renderer_config_t*)calloc(1, sizeof(renderer_config_t));

    renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_16BIT;
    renderer_config->i2s_num = I2S_NUM_1;
    renderer_config->sample_rate = 44100;
    renderer_config->sample_rate_modifier = 1.0;
    renderer_config->output_mode = I2S;

    return renderer_config;
}

static void start_web_radio()
{
    // init web radio
    web_radio_t *radio_config = (web_radio_t*)calloc(1, sizeof(web_radio_t));
    radio_config->url = (char*)PLAY_URL;

    // init player config
    radio_config->player_config = (player_t*)calloc(1, sizeof(player_t));
    radio_config->player_config->status = UNINITIALIZED;
    radio_config->player_config->command = CMD_NONE;
    radio_config->player_config->decoder_status = UNINITIALIZED;
    radio_config->player_config->decoder_command = CMD_NONE;
    radio_config->player_config->buffer_pref = SAFE;
    radio_config->player_config->media_stream = (media_stream_t*)calloc(1, sizeof(media_stream_t));

    // init renderer
    radio_config->player_config->renderer_config = create_renderer_config();

    // start radio
    printf("starting radio ...");
    web_radio_init(radio_config);
    web_radio_start(radio_config);
}

void setup()
{
    esp_log_level_set("*", ESP_LOG_INFO);
    Serial.begin(115200);
    delay(5000);

    // We start by connecting to a WiFi network

    connect_to_wifi((char*)ssid, (char*)password);

    if (!spiRamFifoInit()) {
        printf("\n\nSPI RAM chip fail!\n");
        while(1);
    }
    // init web radio
    start_web_radio();
}

int value = 0;

void loop()
{
}
