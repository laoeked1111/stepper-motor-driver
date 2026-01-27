#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

///////////////////////////////////////////////////////////////

// Control PWM 1
#define LED_PIN_1            5
#define LED_TIMER_1          LEDC_TIMER_0
#define LED_CHANNEL_1        LEDC_CHANNEL_0
#define LED_FREQ_1           1000 // [Hz]

// Control PWM 2
#define LED_PIN_2            18
#define LED_TIMER_2          LEDC_TIMER_1
#define LED_CHANNEL_2        LEDC_CHANNEL_1
#define LED_FREQ_2           1000 // [Hz]

// Control PWM 3
#define LED_PIN_3            19
#define LED_TIMER_3          LEDC_TIMER_2
#define LED_CHANNEL_3        LEDC_CHANNEL_2
#define LED_FREQ_3           1000 // [Hz]

// all the timers/channels can share these
#define LED_DUTY_RES         LEDC_TIMER_13_BIT
#define LED_DUTY_50_PERCENT  4096 // 50% of 2^13
#define LED_MODE             LEDC_LOW_SPEED_MODE

#define TICK_PERIOD_MS       10


///////////////////////////////////////////////////////////////


static const char *TAG = "MUSIC_PLAYER";

// Note Frequencies
const float NOTE_REST = 0;
const float NOTE_Gsh2 = 103.83, NOTE_A2 = 110.00, NOTE_Ash2 = 116.54, NOTE_B2 = 123.47;
const float NOTE_C3 = 130.81, NOTE_Csh3 = 138.59, NOTE_D3 = 146.83, NOTE_Dsh3 = 155.56, NOTE_E3 = 164.81, NOTE_F3 = 174.61, NOTE_Fsh3 = 185.00, NOTE_G3 = 196.00, NOTE_Gsh3 = 207.65, NOTE_A3 = 220.00, NOTE_Ash3 = 233.08, NOTE_B3 = 246.94;
const float NOTE_C4 = 261.63, NOTE_Csh4 = 277.18, NOTE_D4 = 293.66, NOTE_Dsh4 = 311.13, NOTE_E4 = 329.63, NOTE_F4 = 349.23, NOTE_Fsh4 = 369.99, NOTE_G4 = 392.00, NOTE_Gsh4 = 415.30, NOTE_A4 = 440.00, NOTE_Ash4 = 466.16, NOTE_B4 = 493.88;
const float NOTE_C5 = 523.25, NOTE_Csh5 = 554.37, NOTE_D5 = 587.33, NOTE_Dsh5 = 622.25, NOTE_E5 = 659.26, NOTE_F5 = 698.46, NOTE_Fsh5 = 739.99, NOTE_G5 = 783.99, NOTE_Gsh5 = 830.61, NOTE_A5 = 880.00, NOTE_Ash5 = 932.33, NOTE_B5 = 987.77;
const float NOTE_C6 = 1046.50, NOTE_Csh6 = 1108.73, NOTE_D6 = 1174.66, NOTE_Dsh6 = 1244.51, NOTE_E6 = 1318.51, NOTE_F6 = 1396.91, NOTE_Fsh6 = 1479.98, NOTE_G6 = 1567.98, NOTE_Gsh6 = 1661.22, NOTE_A6 = 1760.00, NOTE_Ash6 = 1864.66, NOTE_B6 = 1975.53;

const int TEMPO = 126;
const int QUARTER_NOTE = (int) 60000.0 / TEMPO; // ms

// tunes 
typedef struct {
  float freq;
  float duration_ms;
} note_t;

const note_t tune_1[] = {
  {NOTE_REST, QUARTER_NOTE * 8}, {NOTE_REST, QUARTER_NOTE * 8}, {NOTE_REST, QUARTER_NOTE * 8},
  {NOTE_G4, QUARTER_NOTE * 3/4}, {NOTE_A4, QUARTER_NOTE / 4}, {NOTE_Ash4, QUARTER_NOTE * 2}, {NOTE_G4, QUARTER_NOTE * 3/4}, {NOTE_A4, QUARTER_NOTE / 4}, 
  {NOTE_Ash4, QUARTER_NOTE * 2}, {NOTE_A4, QUARTER_NOTE * 2},
  {NOTE_D5, QUARTER_NOTE * 2}, {NOTE_C5, QUARTER_NOTE * 2},
  {NOTE_REST, QUARTER_NOTE * 5},
  {NOTE_F4, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE},
  {NOTE_G4, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_G4, QUARTER_NOTE},
  {NOTE_F4, QUARTER_NOTE * 7}, 
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_D5, QUARTER_NOTE}, {NOTE_C5, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE},
  {NOTE_A4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE},
  {NOTE_F4, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE},
  {NOTE_G4, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_G4, QUARTER_NOTE},
  {NOTE_F4, QUARTER_NOTE * 7}, 
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_D5, QUARTER_NOTE}, {NOTE_C5, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE},
  {NOTE_A4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE}, {NOTE_F4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE},
  {NOTE_F4, QUARTER_NOTE * 6}, 

  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_G4, QUARTER_NOTE * 3/4}, {NOTE_G4, QUARTER_NOTE / 4},
  {NOTE_G4, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_A4, QUARTER_NOTE / 2}, {NOTE_A4, QUARTER_NOTE / 2},
  {NOTE_A4, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE * 2/3}, {NOTE_Ash4, QUARTER_NOTE * 2/3}, {NOTE_Ash4, QUARTER_NOTE * 2/3},
  {NOTE_Ash4, QUARTER_NOTE * 4}, {NOTE_Ash4, QUARTER_NOTE}, {NOTE_C5, QUARTER_NOTE},
  {NOTE_C5, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_A4, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE}, {NOTE_C5, QUARTER_NOTE},
  {NOTE_D5, QUARTER_NOTE * 2}, {NOTE_F4, QUARTER_NOTE}, {NOTE_G4, QUARTER_NOTE / 2},
  {NOTE_G4, QUARTER_NOTE * 5/2}, {NOTE_Dsh5, QUARTER_NOTE}, {NOTE_D5, QUARTER_NOTE},
  {NOTE_G4, QUARTER_NOTE * 2}, {NOTE_G4, QUARTER_NOTE / 2}, {NOTE_Dsh5, QUARTER_NOTE * 3/2},
  {NOTE_Dsh5, QUARTER_NOTE / 2}, {NOTE_D5, QUARTER_NOTE}, {NOTE_C5, QUARTER_NOTE}, {NOTE_Ash4, QUARTER_NOTE * 3/2},
  {NOTE_F4, QUARTER_NOTE * 4},
  {-1, 1}
};

const note_t tune_2[] = {
  {NOTE_REST, QUARTER_NOTE * 8},
  {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_Dsh4, QUARTER_NOTE * 2}, {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4},
  {NOTE_Dsh4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_C4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_Dsh4, QUARTER_NOTE * 2}, {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4},
  {NOTE_Dsh4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_C4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_Dsh4, QUARTER_NOTE * 2}, {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4},
  {NOTE_Dsh4, QUARTER_NOTE * 2}, {NOTE_D4, QUARTER_NOTE * 2},
  {NOTE_D4, QUARTER_NOTE * 2}, {NOTE_C4, QUARTER_NOTE * 2},
  {NOTE_REST, QUARTER_NOTE * 5}, {NOTE_F3, QUARTER_NOTE}, {NOTE_Ash3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_D4, QUARTER_NOTE * 7},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_F4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE}, {NOTE_D4, QUARTER_NOTE},
  {NOTE_C4, QUARTER_NOTE}, {NOTE_Ash3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE}, {NOTE_G3, QUARTER_NOTE},
  {NOTE_F3, QUARTER_NOTE * 2}, {NOTE_D4, QUARTER_NOTE * 3/4}, {NOTE_Dsh4, QUARTER_NOTE / 4}, {NOTE_F4, QUARTER_NOTE / 2}, 
  {NOTE_F4, QUARTER_NOTE * 3/2}, 
  {NOTE_REST, QUARTER_NOTE * 5}, {NOTE_C4, QUARTER_NOTE * 3/4}, {NOTE_D4, QUARTER_NOTE / 4}, {NOTE_Dsh4, QUARTER_NOTE / 2}, 
  {NOTE_Dsh4, QUARTER_NOTE * 3/2}, {NOTE_Dsh4, QUARTER_NOTE / 2}, {NOTE_D4, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE * 3/2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_Ash3, QUARTER_NOTE * 3/4}, {NOTE_C4, QUARTER_NOTE / 4}, {NOTE_D4, QUARTER_NOTE / 2},
  {NOTE_D4, QUARTER_NOTE * 3/2}, {NOTE_D4, QUARTER_NOTE / 2}, {NOTE_C4, QUARTER_NOTE}, {NOTE_Ash3, QUARTER_NOTE},
  {NOTE_C4, QUARTER_NOTE * 9/2},
  {NOTE_REST, QUARTER_NOTE * 4},
  {NOTE_Ash3, QUARTER_NOTE * 6},

  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_Ash3, QUARTER_NOTE * 3/4}, {NOTE_Ash3, QUARTER_NOTE / 4},
  {NOTE_Ash3, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_D4, QUARTER_NOTE / 2}, {NOTE_D4, QUARTER_NOTE / 2},
  {NOTE_D4, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE * 2/3}, {NOTE_Ash3, QUARTER_NOTE * 2/3}, {NOTE_Ash3, QUARTER_NOTE * 2/3},
  {NOTE_Ash3, QUARTER_NOTE * 4}, {NOTE_Ash3, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_C4, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE}, {NOTE_Ash3, QUARTER_NOTE}, {NOTE_C4, QUARTER_NOTE},
  {NOTE_D4, QUARTER_NOTE * 2}, {NOTE_Ash3, QUARTER_NOTE * 3/2},
  {NOTE_Ash3, QUARTER_NOTE * 9/2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_Ash3, QUARTER_NOTE / 2}, {NOTE_Dsh4, QUARTER_NOTE * 3/2},
  {NOTE_Dsh4, QUARTER_NOTE / 2}, {NOTE_Dsh4, QUARTER_NOTE}, {NOTE_Dsh4, QUARTER_NOTE}, {NOTE_Ash3, QUARTER_NOTE * 3/2},
  {NOTE_C4, QUARTER_NOTE * 4},
  {-1, -1}
};

const note_t tune_3[] = {
  {NOTE_REST, QUARTER_NOTE * 8},
  {NOTE_C3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4}, {NOTE_Dsh3, QUARTER_NOTE * 2}, {NOTE_C3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4},
  {NOTE_Dsh3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4}, {NOTE_C3, QUARTER_NOTE}, {NOTE_Dsh3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4}, {NOTE_C3, QUARTER_NOTE},
  {NOTE_Dsh3, QUARTER_NOTE * 3/4}, {NOTE_F3, QUARTER_NOTE / 4}, {NOTE_G3, QUARTER_NOTE * 2}, {NOTE_Dsh3, QUARTER_NOTE * 3/4}, {NOTE_F3, QUARTER_NOTE / 4},
  {NOTE_G3, QUARTER_NOTE * 3/4}, {NOTE_F3, QUARTER_NOTE / 4}, {NOTE_Dsh3, QUARTER_NOTE}, {NOTE_G3, QUARTER_NOTE * 3/4}, {NOTE_F3, QUARTER_NOTE / 4}, {NOTE_Dsh3, QUARTER_NOTE},
  {NOTE_G3, QUARTER_NOTE * 3/4}, {NOTE_A3, QUARTER_NOTE / 4}, {NOTE_Ash3, QUARTER_NOTE * 2}, {NOTE_G3, QUARTER_NOTE * 3/4}, {NOTE_A3, QUARTER_NOTE / 4},
  {NOTE_Ash3, QUARTER_NOTE * 2}, {NOTE_A3, QUARTER_NOTE * 2},
  {NOTE_D4, QUARTER_NOTE * 2}, {NOTE_C4, QUARTER_NOTE * 2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_D3, QUARTER_NOTE * 3/4}, {NOTE_Dsh3, QUARTER_NOTE / 4}, {NOTE_F3, QUARTER_NOTE / 2}, 
  {NOTE_F3, QUARTER_NOTE * 3/2}, 
  {NOTE_REST, QUARTER_NOTE * 5}, {NOTE_C3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4}, {NOTE_Dsh3, QUARTER_NOTE / 2},
  {NOTE_Dsh3, QUARTER_NOTE * 3/2}, {NOTE_Dsh3, QUARTER_NOTE / 2}, {NOTE_D3, QUARTER_NOTE}, {NOTE_C3, QUARTER_NOTE * 3/2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_Ash2, QUARTER_NOTE * 3/4}, {NOTE_C3, QUARTER_NOTE / 4}, {NOTE_D3, QUARTER_NOTE / 2},
  {NOTE_D3, QUARTER_NOTE * 3/2}, {NOTE_D3, QUARTER_NOTE / 2}, {NOTE_C3, QUARTER_NOTE}, {NOTE_Ash2, QUARTER_NOTE}, 
  {NOTE_C3, QUARTER_NOTE * 9/2},
  {NOTE_REST, QUARTER_NOTE * 6}, {NOTE_D3, QUARTER_NOTE * 3/4}, {NOTE_Dsh3, QUARTER_NOTE / 4}, {NOTE_F3, QUARTER_NOTE / 2},
  {NOTE_F3, QUARTER_NOTE * 3/2}, 
  {NOTE_REST, QUARTER_NOTE * 5}, {NOTE_C3, QUARTER_NOTE * 3/4}, {NOTE_D3, QUARTER_NOTE / 4}, {NOTE_Dsh3, QUARTER_NOTE / 2},
  {NOTE_Dsh3, QUARTER_NOTE * 3/2}, {NOTE_Dsh3, QUARTER_NOTE / 2}, {NOTE_D3, QUARTER_NOTE}, {NOTE_C3, QUARTER_NOTE * 3/2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_Ash2, QUARTER_NOTE * 3/4}, {NOTE_C3, QUARTER_NOTE / 4}, {NOTE_D3, QUARTER_NOTE / 2},
  {NOTE_D3, QUARTER_NOTE * 3/2}, {NOTE_D3, QUARTER_NOTE / 2}, {NOTE_C3, QUARTER_NOTE}, {NOTE_Ash2, QUARTER_NOTE}, 
  {NOTE_C3, QUARTER_NOTE * 9/2},
  {NOTE_REST, QUARTER_NOTE * 4},
  {NOTE_Ash2, QUARTER_NOTE * 6},

  {NOTE_REST, QUARTER_NOTE * 3}, {NOTE_G3, QUARTER_NOTE * 3/4}, {NOTE_G3, QUARTER_NOTE / 4},
  {NOTE_G3, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE/2}, {NOTE_A3, QUARTER_NOTE/2},
  {NOTE_A3, QUARTER_NOTE * 6},
  {NOTE_REST, QUARTER_NOTE * 3}, 
  {NOTE_Dsh3, QUARTER_NOTE * 3}, {NOTE_Dsh3, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE},
  {NOTE_F3, QUARTER_NOTE * 4},
  {NOTE_REST, QUARTER_NOTE}, {NOTE_F3, QUARTER_NOTE}, {NOTE_G3, QUARTER_NOTE}, {NOTE_A3, QUARTER_NOTE},
  {NOTE_Ash3, QUARTER_NOTE * 2}, {NOTE_D3, QUARTER_NOTE}, {NOTE_D3, QUARTER_NOTE/2},
  {NOTE_Dsh3, QUARTER_NOTE * 9/2},
  {NOTE_REST, QUARTER_NOTE * 2}, {NOTE_G3, QUARTER_NOTE/2}, {NOTE_Ash3, QUARTER_NOTE * 3/2},
  {NOTE_Ash3, QUARTER_NOTE/2}, {NOTE_Ash3, QUARTER_NOTE}, {NOTE_G3, QUARTER_NOTE}, {NOTE_G3, QUARTER_NOTE * 3/2},
  {NOTE_C3, QUARTER_NOTE * 4},
  {-1, -1}
};

typedef struct {
  const note_t* song;
  int index;
  int ticks_left;
  ledc_timer_t timer;
  ledc_channel_t channel;
} voice_state_t;

///////////////////////////////////////////////////////////////

// initialize hardware
static void ledc_init(void)
{
  ledc_timer_config_t timer_cfg_1 = {
    .speed_mode       = LED_MODE,
    .timer_num        = LED_TIMER_1,
    .duty_resolution  = LED_DUTY_RES,
    .freq_hz          = LED_FREQ_1,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg_1));

  ledc_channel_config_t chan_cfg_1 = {
    .speed_mode     = LED_MODE,
    .channel        = LED_CHANNEL_1,
    .timer_sel      = LED_TIMER_1,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = LED_PIN_1,
    .duty           = 0, 
    .hpoint         = 0
  };
  ESP_ERROR_CHECK(ledc_channel_config(&chan_cfg_1));

  ledc_timer_config_t timer_cfg_2 = {
    .speed_mode       = LED_MODE,
    .timer_num        = LED_TIMER_2,
    .duty_resolution  = LED_DUTY_RES,
    .freq_hz          = LED_FREQ_2,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg_2));

  ledc_channel_config_t chan_cfg_2 = {
    .speed_mode     = LED_MODE,
    .channel        = LED_CHANNEL_2,
    .timer_sel      = LED_TIMER_2,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = LED_PIN_2,
    .duty           = 0,
    .hpoint         = 0
  };
  ESP_ERROR_CHECK(ledc_channel_config(&chan_cfg_2));

  ledc_timer_config_t timer_cfg_3 = {
    .speed_mode       = LED_MODE,
    .timer_num        = LED_TIMER_3,
    .duty_resolution  = LED_DUTY_RES,
    .freq_hz          = LED_FREQ_3,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg_3));

  ledc_channel_config_t chan_cfg_3 = {
    .speed_mode     = LED_MODE,
    .channel        = LED_CHANNEL_3,
    .timer_sel      = LED_TIMER_3,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = LED_PIN_3,
    .duty           = 0,
    .hpoint         = 0
  };
  ESP_ERROR_CHECK(ledc_channel_config(&chan_cfg_3));
}

// task for playing music and synchronization
void music_task(void *pvParameters)
{
  voice_state_t v1 = {tune_1, 0, 0, LED_TIMER_1, LED_CHANNEL_1};
  voice_state_t v2 = {tune_2, 0, 0, LED_TIMER_2, LED_CHANNEL_2};
  voice_state_t v3 = {tune_3, 0, 0, LED_TIMER_3, LED_CHANNEL_3};

  while (1) {

    // tune 1
    if (v1.ticks_left <= 0 && v1.song[v1.index].freq != -1) { // start new note?
      note_t n = v1.song[v1.index];
      v1.ticks_left = (int)roundf(n.duration_ms / TICK_PERIOD_MS);
    
      if (n.freq != NOTE_REST) {
        ledc_set_freq(LED_MODE, v1.timer, (uint32_t)n.freq);
        ledc_set_duty(LED_MODE, v1.channel, LED_DUTY_50_PERCENT);
        ledc_update_duty(LED_MODE, v1.channel);
      }
      v1.index++;
    }

    // tune 2
    if (v2.ticks_left <= 0 && v2.song[v2.index].freq != -1) {
      note_t n = v2.song[v2.index];
      v2.ticks_left = (int)roundf(n.duration_ms / TICK_PERIOD_MS);

      if (n.freq != NOTE_REST) {
        ledc_set_freq(LED_MODE, v2.timer, (uint32_t)n.freq);
        ledc_set_duty(LED_MODE, v2.channel, LED_DUTY_50_PERCENT);
        ledc_update_duty(LED_MODE, v2.channel);
      }
      v2.index++;
    }

    // tune 3
    if (v3.ticks_left <= 0 && v3.song[v3.index].freq != -1) {
      note_t n = v3.song[v3.index];
      v3.ticks_left = (int)roundf(n.duration_ms / TICK_PERIOD_MS);

      if (n.freq != NOTE_REST) {
        ledc_set_freq(LED_MODE, v3.timer, (uint32_t)n.freq);
        ledc_set_duty(LED_MODE, v3.channel, LED_DUTY_50_PERCENT);
        ledc_update_duty(LED_MODE, v3.channel);
      }
      v3.index++;
    }
        
    // articulation between notes
    if (v1.ticks_left == 2) { ledc_set_duty(LED_MODE, v1.channel, 0); ledc_update_duty(LED_MODE, v1.channel); }
    if (v2.ticks_left == 2) { ledc_set_duty(LED_MODE, v2.channel, 0); ledc_update_duty(LED_MODE, v2.channel); }
    if (v3.ticks_left == 2) { ledc_set_duty(LED_MODE, v3.channel, 0); ledc_update_duty(LED_MODE, v3.channel); }

    vTaskDelay(pdMS_TO_TICKS(TICK_PERIOD_MS));
    if (v1.ticks_left > 0) v1.ticks_left--;
    if (v2.ticks_left > 0) v2.ticks_left--;
    if (v3.ticks_left > 0) v3.ticks_left--;

    // exit
    if (
      v1.song[v1.index].freq == -1 && v1.ticks_left <= 0 &&
      v2.song[v2.index].freq == -1 && v2.ticks_left <= 0 &&
      v3.song[v3.index].freq == -1 && v3.ticks_left <= 0
    ) { break; }
  } 

  // song done; cleanup
  ledc_set_duty(LED_MODE, LED_CHANNEL_1, 0);
  ledc_set_duty(LED_MODE, LED_CHANNEL_2, 0);
  ledc_set_duty(LED_MODE, LED_CHANNEL_3, 0);
  ledc_update_duty(LED_MODE, LED_CHANNEL_1);
  ledc_update_duty(LED_MODE, LED_CHANNEL_2);
  ledc_update_duty(LED_MODE, LED_CHANNEL_3);

  ESP_LOGI(TAG, "Playback finished.");
  vTaskDelete(NULL);
}

void app_main(void) 
{
  ledc_init();
  ESP_LOGI(TAG, "Hardware initialized. Starting music...");
  xTaskCreate(music_task, "music_task", 4096, NULL, 5, NULL);
}
