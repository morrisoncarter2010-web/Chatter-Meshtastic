//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//   Have custom connections or functionality? Configure them in this section   //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

// Debugging
// #define GPS_DEBUG 1

// Lora
#define CHATTER_V21_HSPI
#define USERPREFS_LORACONFIG_MODEM_PRESET meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_SLOW
#define USE_LLCC68 // Original Chatter2 with LLCC68 module
// #define USE_SX1262 // Disabled for green V2.1 with stock LLCC68

#define SX126X_CS 14
#define LORA_SCK 16
#define LORA_MOSI 5
#define LORA_MISO 17
#define SX126X_RESET RADIOLIB_NC
#define SX126X_BUSY 4
#define SX126X_DIO1 18
#define SX126X_DIO2_AS_RF_SWITCH
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define SX126X_TXEN RADIOLIB_NC
#define SX126X_RXEN RADIOLIB_NC

// External notification
// FIXME: Check if EXT_NOTIFY_OUT actualy has any effect and removes the need for setting the external notication pin in the
// app/preferences
// #define EXT_NOTIFY_OUT 2

// Buzzer
#define PIN_BUZZER 19

// Buttons
// #define BUTTON_PIN 36

// I2C
// #define I2C_SCL 27
// #define I2C_SDA 26

#define SX126X_MAX_POWER 22

// -----------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------
#define HAS_SCREEN 1
#define HAS_SPI_TFT 1
#define ST7735S 1
#define ST7735_CS 15
#define ST7735_RS 33
#define ST7735_SDA 26
#define ST7735_SCK 27
#define ST7735_RESET 13
#define ST7735_MISO -1
#define ST7735_BUSY -1

// Chatter V2.1 backlight is connected to GPIO 32.
// Advertise this pin to Meshtastic's PWM backlight system so the
// on-device brightness controls can use it.
#define TFT_BL 32
#define PIN_PWM_BACKLIGHT TFT_BL

#define ST7735_SPI_HOST VSPI_HOST
#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 27000000
#define TFT_HEIGHT 160
#define TFT_WIDTH 128
#define TFT_OFFSET_X -2
#define TFT_OFFSET_Y 1
#define TFT_OFFSET_ROTATION 2
#define TFT_INVERT false
#define FORCE_LOW_RES 1
#define SCREEN_ROTATE
#define SCREEN_TRANSITION_FRAMERATE 5
#define DISPLAY_FORCE_SMALL_FONTS

// IMPORTANT: Chatter V2.1 backlight is ACTIVE LOW.
// Backlight.cpp must account for this when applying PWM.
#define TFT_BACKLIGHT_ON LOW

#define USE_TFTDISPLAY 1

// -----------------------------------------------------------------------------
// Battery
// -----------------------------------------------------------------------------
#define BATTERY_PIN 36
#define ADC_CHANNEL ADC1_CHANNEL_0
#define ADC_ATTENUATION ADC_ATTEN_DB_12
#define ADC_MULTIPLIER 1.56

// Chatter2 uses 3 AAA cells
#define OCV_ARRAY 1580, 1400, 1350, 1300, 1280, 1250, 1230, 1190, 1150, 1100, 1000
#define NUM_CELLS 3
#undef EXT_PWR_DETECT

// -----------------------------------------------------------------------------
// GPS
// -----------------------------------------------------------------------------
// Chatter 2.1 has no onboard GNSS module.
// Leave HAS_GPS undefined so Meshtastic does not start GPS probing.
// #define HAS_GPS 1
// #define PIN_GPS_EN 15
// #define GPS_EN_ACTIVE 1

#undef GPS_TX_PIN
#undef GPS_RX_PIN

// -----------------------------------------------------------------------------
// Keyboard
// -----------------------------------------------------------------------------
#define INPUTBROKER_SERIAL_TYPE 1
#define KB_LOAD 21
#define KB_CLK 22
#define KB_DATA 23

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//   You should have no need to modify the code below, nor in pins_arduino.h   //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

#define LORA_CS SX126X_CS

#define LORA_DIO1 SX126X_DIO1
