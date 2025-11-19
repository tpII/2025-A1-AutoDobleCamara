#ifndef CONFIG_H
#define CONFIG_H

// ESP32-CAM AI-Thinker Pin Definitions
// Camera pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// LED Flash
#define LED_GPIO_NUM       4

// Available GPIOs for general use
// Note: GPIO 1 (TX), GPIO 3 (RX) are used for serial communication
// GPIO 12, 13, 14, 15 are available but use with caution
// GPIO 2 is often used for onboard LED
#define GPIO_12           12
#define GPIO_13           13
#define GPIO_14           14
#define GPIO_15           15
#define GPIO_2             2
#define GPIO_16           16  // Also connected to PSRAM CS on some boards

// SD Card pins (if using SD card functionality)
// Note: These conflict with camera data pins, use with caution
#define SD_MMC_CMD        15
#define SD_MMC_CLK        14
#define SD_MMC_D0          2
#define SD_MMC_D1          4
#define SD_MMC_D2         12
#define SD_MMC_D3         13

#endif // CONFIG_H
