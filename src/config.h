// WiFi credentials.
const char* ssid = "";
const char* password = "";



const char* PLAY_URL = "http://icy-e-03-boh.sharp-stream.com:80/banburysound.mp3";

#define AUDIO_OUTPUT_MODE I2S
/*ADD_DEL_SAMPLES parameter:
Size of the cumulative buffer offset before we are going to add or remove a sample
The higher this number, the more aggressive we're adjusting the sample rate. Higher numbers give
better resistance to buffer over/underflows due to clock differences, but also can result in
the music sounding higher/lower due to network issues.*/
#define ADD_DEL_BUFFPERSAMP (512)

/*ADD_DEL_SAMPLES parameter:
Same as ADD_DEL_BUFFPERSAMP but for systems without a big SPI RAM chip to buffer mp3 data in.*/
#define ADD_DEL_BUFFPERSAMP_NOSPIRAM (512)


/* Connecting an I2S codec to the ESP is the best way to get nice
16-bit sounds out of the ESP, but it is possible to run this code without the codec. For
this to work, instead of outputting a 2x16bit PCM sample the DAC can decode, we use the built-in
8-Bit DAC.*/
#define CONFIG_OUTPUT_MODE I2S // possible values: I2S, DAC_BUILT_IN

/* there is currently a bug in the SDK when using the built-in DAC - this is a temporary
   workaround until the issue is fixed */
// #define DAC_BUG_WORKAROUND


/*While a large (tens to hundreds of K) buffer is necessary for Internet streams, on a
quiet network and with a direct connection to the stream server, you can get away with
a much smaller buffer. Enabling the following switch will disable accesses to the
23LC1024 code and use a much smaller but internal buffer instead. You want to enable
this if you don't have a 23LC1024 chip connected to the ESP but still want to try
the MP3 decoder. Be warned, if your network isn't 100% quiet and latency-free and/or
the server isn't very close to your ESP, this _will_ lead to stutters in the played
MP3 stream! */
#define FAKE_SPI_BUFF
