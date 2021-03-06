// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../components/http_server/my_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "ota_server.h"

#include "Arduino.h"
#include "ros2arduino.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "camera.h"
#include "bitmap.h"
#include "led.h"
#include "driver/i2c.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "ulp-util.h"      // my ulp_init(), ulp_start()

#include "qr_recoginize.h"

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

#undef USE_SPI_MODE

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13
#endif //USE_SPI_MODE

static void handle_grayscale_pgm(http_context_t http_ctx, void* ctx);
static void handle_rgb_bmp(http_context_t http_ctx, void* ctx);
static void handle_rgb_bmp_stream(http_context_t http_ctx, void* ctx);
static void handle_jpg(http_context_t http_ctx, void* ctx);
static void handle_jpg_stream(http_context_t http_ctx, void* ctx);
static esp_err_t event_handler(void *ctx, system_event_t *event);
static void initialise_wifi(void);

sdmmc_card_t* card = NULL;
bool sdcard_ready = true;

bool camera_ready = true;

static const char* TAG = "MAIN";

static const char* STREAM_CONTENT_TYPE =
		"multipart/x-mixed-replace; boundary=123456789000000000000987654321";

static const char* STREAM_BOUNDARY = "--123456789000000000000987654321";

EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static ip4_addr_t s_ip_addr = {};
static uint8_t s_ip_addr_changed = 1;
static camera_pixelformat_t s_pixel_format;

#define CAMERA_PIXEL_FORMAT CAMERA_PF_JPEG
#define CAMERA_FRAME_SIZE CAMERA_FS_QVGA

static void ota_server_task(void * param)
{
	xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ota_server_start();
	vTaskDelete(NULL);
}

static uint8_t sd_test_buf[32768] = {0};
static void sd_test_task(void * param)
{
	if ( sdcard_ready == true && card != NULL )
	{
		// Card has been initialized, print its properties
		sdmmc_card_print_info(stdout, card);

		while(1)
		{
			// Use POSIX and C standard library functions to work with files.
			// First create a file.
			FILE* f = fopen("/sdcard/hello.bin", "w");
			if (f == NULL) {
				ESP_LOGE(TAG, "Failed to open file for writing");
			}
			else
			{
				uint32_t i=0;
				uint32_t s = 1024*1000;
				int64_t t = esp_timer_get_time();
				while(i<s)
				{
					fwrite(sd_test_buf,1,sizeof(sd_test_buf),f);
					i += sizeof(sd_test_buf);
				}
				if( (esp_timer_get_time()-t) != 0 )
				{
					ESP_LOGI(TAG, "File written %dkB/s",(int)((1000*i)/(esp_timer_get_time()-t)));
				}
				fclose(f);
			}

#if 1
			{
				f = fopen("/sdcard/hello.bin", "r");
				if (f == NULL) {
					ESP_LOGE(TAG, "Failed to open file for reading");
				}
				else
				{
					int64_t t = esp_timer_get_time();
					size_t i = 0;
					while( !feof(f) )
					{
						i += fread(sd_test_buf,1,sizeof(sd_test_buf),f);
					}
					if( (esp_timer_get_time()-t) != 0 )
					{
						ESP_LOGI(TAG, "File read %dkB/s",(int)((1000*i)/(esp_timer_get_time()-t)));
					}
					fclose(f);
				}
			}
#endif
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);
}

#define I2C_BUS_PORT 0
#if 1
static void zumo_motor_set_speed(int16_t l, int16_t r)
{
	i2c_cmd_handle_t CommandHandle = NULL;
	if ( ( CommandHandle = i2c_cmd_link_create( ) ) != NULL )
	{
		i2c_master_start( CommandHandle );
		i2c_master_write_byte( CommandHandle, ( 8 << 1 ) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte( CommandHandle, 3, true);
		i2c_master_write_byte( CommandHandle, (l>>8)&0xff, true);
		i2c_master_write_byte( CommandHandle, (l>>0)&0xff, true);
		i2c_master_write_byte( CommandHandle, (r>>8)&0xff, true);
		i2c_master_write_byte( CommandHandle, (r>>0)&0xff, true);
		i2c_master_stop( CommandHandle );
		i2c_master_cmd_begin((i2c_port_t)I2C_BUS_PORT, CommandHandle, pdMS_TO_TICKS( 10 ));
		i2c_cmd_link_delete( CommandHandle );
	}
}


static void zumo_led(uint8_t r, uint8_t y, uint8_t g)
{
	i2c_cmd_handle_t CommandHandle = NULL;
	if ( ( CommandHandle = i2c_cmd_link_create( ) ) != NULL )
	{
		i2c_master_start( CommandHandle );
		i2c_master_write_byte( CommandHandle, ( 8 << 1 ) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte( CommandHandle, 7, true);
		i2c_master_write_byte( CommandHandle, ((r<<0)|(y<<1)|(g<<1))&0xff, true);
		i2c_master_stop( CommandHandle );
		i2c_master_cmd_begin((i2c_port_t)I2C_BUS_PORT, CommandHandle, pdMS_TO_TICKS( 10 ));
		i2c_cmd_link_delete( CommandHandle );
	}
}

static void zumo_beep(uint16_t freq,uint16_t duration, uint8_t volume)
{
	i2c_cmd_handle_t CommandHandle = NULL;
	if ( ( CommandHandle = i2c_cmd_link_create( ) ) != NULL )
	{
		i2c_master_start( CommandHandle );
		i2c_master_write_byte( CommandHandle, ( 8 << 1 ) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte( CommandHandle, 1, true);
		i2c_master_write_byte( CommandHandle, (freq>>8)&0xff, true);
		i2c_master_write_byte( CommandHandle, (freq>>0)&0xff, true);
		i2c_master_write_byte( CommandHandle, (duration>>8)&0xff, true);
		i2c_master_write_byte( CommandHandle, (duration>>0)&0xff, true);
		i2c_master_write_byte( CommandHandle, (volume>>0)&0xff, true);
		i2c_master_stop( CommandHandle );
		i2c_master_cmd_begin((i2c_port_t)I2C_BUS_PORT, CommandHandle, pdMS_TO_TICKS( 10 ));
		i2c_cmd_link_delete( CommandHandle );
	}
}

void stm32_show_ip(uint32_t ip)
{
	i2c_cmd_handle_t CommandHandle = NULL;
	if ( ( CommandHandle = i2c_cmd_link_create( ) ) != NULL )
	{
		uint8_t tmpbuf[4];
		tmpbuf[3] = (ip>>24)&0xff;
		tmpbuf[2] = (ip>>16)&0xff;
		tmpbuf[1] = (ip>>8)&0xff;
		tmpbuf[0] = (ip>>0)&0xff;
		i2c_master_start( CommandHandle );
		i2c_master_write_byte( CommandHandle, ( 0x10 << 1 ) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte( CommandHandle, 0x4a, true);
		i2c_master_write(CommandHandle, (uint8_t*)&tmpbuf[0], 4, true);
		i2c_master_stop( CommandHandle );
		i2c_master_cmd_begin((i2c_port_t)I2C_BUS_PORT, CommandHandle, pdMS_TO_TICKS( 10 ));
		i2c_cmd_link_delete( CommandHandle );
	}
}

static void i2c_task(void * param)
{
	char tmpstr[64] = "";
	int tmpcnt = 0;

	zumo_led(0,0,0);
	zumo_motor_set_speed(10,10);
	zumo_beep(400,2000,15);

	while(1)
	{
		if( s_ip_addr_changed != 0 )
		{
			s_ip_addr_changed = 0;
			stm32_show_ip(s_ip_addr.addr);
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
#endif

static vprintf_like_t my_deflog = NULL;
static char mylog_linebuf[64];
int my_i2clog(const char *format, va_list args)
{
	if( my_deflog != NULL )
	{
		my_deflog(format,args);
	}

	vsnprintf (mylog_linebuf, sizeof(mylog_linebuf)-1, format, args);
	//sprintf(mylog_linebuf,"Hallo 123\n");
	int n = strlen(mylog_linebuf);
	if(n > 0 )
	{
		i2c_cmd_handle_t CommandHandle = NULL;
		if ( ( CommandHandle = i2c_cmd_link_create( ) ) != NULL )
		{
			i2c_master_start( CommandHandle );
			i2c_master_write_byte( CommandHandle, ( 0x10 << 1 ) | I2C_MASTER_WRITE, true);
			i2c_master_write_byte( CommandHandle, 0xff, true);
			i2c_master_write(CommandHandle, (uint8_t*)&mylog_linebuf[0], n, true);
			i2c_master_stop( CommandHandle );
			i2c_master_cmd_begin((i2c_port_t)I2C_BUS_PORT, CommandHandle, pdMS_TO_TICKS( 10 ));
			i2c_cmd_link_delete( CommandHandle );
			return n;
		}
	}
	return 0;
}

void app_main()
{
	/* Print chip information */
	esp_log_level_set("wifi", ESP_LOG_WARN);
	esp_log_level_set("gpio", ESP_LOG_WARN);

	ESP_LOGI(TAG,"init ULP ...");
	ulp_init();
	ulp_start();

#if 1
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
			chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
					(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

	ESP_LOGI(TAG, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	esp_err_t err = nvs_flash_init();
	if (err != ESP_OK) {
		ESP_ERROR_CHECK( nvs_flash_erase() );
		ESP_ERROR_CHECK( nvs_flash_init() );
	}

	initArduino();

	initialise_wifi();
	xTaskCreate(&ota_server_task, "ota_server_task", 4096, NULL, 5, NULL);

#ifndef USE_SPI_MODE
	ESP_LOGI(TAG, "Using SDMMC peripheral");
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();

	host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// To use 1-line SD mode, uncomment the following line:
	slot_config.width = 1;

	// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
	// Internal pull-ups are not sufficient. However, enabling internal pull-ups
	// does make a difference some boards, so we do that here.
	gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
	gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
	//gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
	//gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
	gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

#else
	ESP_LOGI(TAG, "Using SPI peripheral");

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck  = PIN_NUM_CLK;
	slot_config.gpio_cs   = PIN_NUM_CS;
	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
#endif //USE_SPI_MODE

	// Options for mounting the filesystem.
	// If format_if_mount_failed is set to true, SD card will be partitioned and
	// formatted in case when mounting fails.
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
			.format_if_mount_failed = true,
			.max_files = 5,
			.allocation_unit_size = 16 * 1024
	};

	// Use settings defined above to initialize SD card and mount FAT filesystem.
	// Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
	// Please check its source code and implement error recovery when developing
	// production applications.
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount filesystem. "
					"If you want the card to be formatted, set format_if_mount_failed = true.");
		} else {
			ESP_LOGE(TAG, "Failed to initialize the card (%s). "
					"Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
		}
		sdcard_ready = false;
	}

	xTaskCreate(&sd_test_task, "sd_test_task", 4096, NULL, 5, NULL);

	ESP_LOGI(TAG, "main() i2c init ...");
	static i2c_config_t Config;
	memset( &Config, 0, sizeof( i2c_config_t ) );
	Config.mode = I2C_MODE_MASTER;
	//Config.sda_io_num = (gpio_num_t)4;
	Config.sda_io_num = (gpio_num_t)13;
	Config.sda_pullup_en = GPIO_PULLUP_ENABLE;
	Config.scl_io_num = (gpio_num_t)12;
	Config.scl_pullup_en = GPIO_PULLUP_ENABLE;
	Config.master.clk_speed = 400000;
	i2c_param_config( (i2c_port_t)I2C_BUS_PORT, &Config );
	i2c_driver_install( (i2c_port_t)I2C_BUS_PORT, Config.mode, 0, 0, 0 );
	i2c_set_timeout((i2c_port_t)I2C_BUS_PORT, (I2C_APB_CLK_FREQ / Config.master.clk_speed)*1024);
	xTaskCreate(&i2c_task, "i2c_task", 4096, NULL, 5, NULL);

	my_deflog = esp_log_set_vprintf(my_i2clog);

#if 0
	camera_config_t camera_config = {
			.ledc_channel = LEDC_CHANNEL_0,
			.ledc_timer = LEDC_TIMER_0,
			.pin_d0 = CONFIG_D0,
			.pin_d1 = CONFIG_D1,
			.pin_d2 = CONFIG_D2,
			.pin_d3 = CONFIG_D3,
			.pin_d4 = CONFIG_D4,
			.pin_d5 = CONFIG_D5,
			.pin_d6 = CONFIG_D6,
			.pin_d7 = CONFIG_D7,
			.pin_xclk = CONFIG_XCLK,
			.pin_pclk = CONFIG_PCLK,
			.pin_vsync = CONFIG_VSYNC,
			.pin_href = CONFIG_HREF,
			.pin_sscb_sda = CONFIG_SDA,
			.pin_sscb_scl = CONFIG_SCL,
			.pin_reset = CONFIG_RESET,
			.xclk_freq_hz = CONFIG_XCLK_FREQ,
	};

	camera_model_t camera_model;
	err = camera_probe(&camera_config, &camera_model);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Camera probe failed with error 0x%x", err);
		camera_ready = false;
	}

	if ( camera_ready == true ) {
		if (camera_model == CAMERA_OV7725) {
			s_pixel_format = CAMERA_PIXEL_FORMAT;
			camera_config.frame_size = CAMERA_FRAME_SIZE;
			ESP_LOGI(TAG, "Detected OV7725 camera, using %s bitmap format",
					CAMERA_PIXEL_FORMAT == CAMERA_PF_GRAYSCALE ?
							"grayscale" : "RGB565");
		} else if (camera_model == CAMERA_OV2640) {
			ESP_LOGI(TAG, "Detected OV2640 camera, using JPEG format");
			s_pixel_format = CAMERA_PIXEL_FORMAT;
			camera_config.frame_size = CAMERA_FRAME_SIZE;
			if (s_pixel_format == CAMERA_PF_JPEG)
				camera_config.jpeg_quality = 15;
		} else {
			ESP_LOGE(TAG, "Camera not supported");
		}
	}

	if ( camera_ready == true ) {
		camera_config.pixel_format = s_pixel_format;
		err = camera_init(&camera_config);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
			camera_ready = false;
		}
	}

	if ( camera_ready == true ) {
		http_server_t server;
		http_server_options_t http_options = HTTP_SERVER_OPTIONS_DEFAULT();
		ESP_ERROR_CHECK( http_server_start(&http_options, &server) );

		if (s_pixel_format == CAMERA_PF_GRAYSCALE) {
			ESP_ERROR_CHECK( http_register_handler(server, "/pgm", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_grayscale_pgm, &camera_config) );
			ESP_LOGI(TAG, "Open http://" IPSTR "/pgm for a single image/x-portable-graymap image", IP2STR(&s_ip_addr));
		}
		if (s_pixel_format == CAMERA_PF_RGB565) {
			ESP_ERROR_CHECK( http_register_handler(server, "/bmp", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp, NULL) );
			ESP_LOGI(TAG, "Open http://" IPSTR "/bmp for single image/bitmap image", IP2STR(&s_ip_addr));
			ESP_ERROR_CHECK( http_register_handler(server, "/bmp_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp_stream, NULL) );
			ESP_LOGI(TAG, "Open http://" IPSTR "/bmp_stream for multipart/x-mixed-replace stream of bitmaps", IP2STR(&s_ip_addr));
		}
		if (s_pixel_format == CAMERA_PF_JPEG) {
			ESP_ERROR_CHECK( http_register_handler(server, "/jpg", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg, NULL) );
			ESP_LOGI(TAG, "Open http://" IPSTR "/jpg for single image/jpg image", IP2STR(&s_ip_addr));
			ESP_ERROR_CHECK( http_register_handler(server, "/jpg_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg_stream, NULL) );
			ESP_LOGI(TAG, "Open http://" IPSTR "/jpg_stream for multipart/x-mixed-replace stream of JPEGs", IP2STR(&s_ip_addr));
		}
	}
#endif

#endif

#if 0
	/* Configure the IOMUX register for pad BLINK_GPIO (some pads are
  muxed to GPIO on reset already, but some default to other
  functions and need to be switched to GPIO. Consult the
  Technical Reference for a list of pads and their default
  functions.)
	 */
#define TESTGPIO 33
	gpio_pad_select_gpio(TESTGPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(TESTGPIO, GPIO_MODE_OUTPUT);
	while(1) {
		/* Blink off (output low) */
		printf("Turning off the LED\n");
		gpio_set_level(TESTGPIO, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		/* Blink on (output high) */
		printf("Turning on the LED\n");
		gpio_set_level(TESTGPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
#endif

	//ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
	//esp_deep_sleep_start();

	ESP_LOGI(TAG, "... init done. free heap: %u", xPortGetFreeHeapSize());
}

static esp_err_t write_frame(http_context_t http_ctx)
{
	http_buffer_t fb_data = {
			.data = camera_get_fb(),
			.size = camera_get_data_size(),
			.data_is_persistent = true
	};
	return http_response_write(http_ctx, &fb_data);
}

static void handle_grayscale_pgm(http_context_t http_ctx, void* ctx)
{
	esp_err_t err = camera_run();
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
		return;
	}
	char* pgm_header_str;
	asprintf(&pgm_header_str, "P5 %d %d %d\n",
			camera_get_fb_width(), camera_get_fb_height(), 255);
	if (pgm_header_str == NULL) {
		return;
	}

	size_t response_size = strlen(pgm_header_str) + camera_get_data_size();
	http_response_begin(http_ctx, 200, "image/x-portable-graymap", response_size);
	http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.pgm");
	http_buffer_t pgm_header = { .data = pgm_header_str };
	http_response_write(http_ctx, &pgm_header);
	free(pgm_header_str);

	write_frame(http_ctx);
	http_response_end(http_ctx);
	ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());
#if CONFIG_QR_RECOGNIZE
	camera_config_t *camera_config = ctx;
	xTaskCreate(qr_recoginze, "qr_recoginze", 111500, camera_config, 5, NULL);
#endif
}

static void handle_rgb_bmp(http_context_t http_ctx, void* ctx)
{
	esp_err_t err = camera_run();
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
		return;
	}

	bitmap_header_t* header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
	if (header == NULL) {
		return;
	}

	http_response_begin(http_ctx, 200, "image/bmp", sizeof(*header) + camera_get_data_size());
	http_buffer_t bmp_header = {
			.data = header,
			.size = sizeof(*header)
	};
	http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.bmp");
	http_response_write(http_ctx, &bmp_header);
	free(header);

	write_frame(http_ctx);
	http_response_end(http_ctx);
}

static void handle_jpg(http_context_t http_ctx, void* ctx)
{
	if(get_light_state())
		led_open();
	esp_err_t err = camera_run();
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
		return;
	}

	http_response_begin(http_ctx, 200, "image/jpeg", camera_get_data_size());
	http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.jpg");
	write_frame(http_ctx);
	http_response_end(http_ctx);
	led_close();
}


static void handle_rgb_bmp_stream(http_context_t http_ctx, void* ctx)
{
	http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);
	bitmap_header_t* header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
	if (header == NULL) {
		return;
	}
	http_buffer_t bmp_header = {
			.data = header,
			.size = sizeof(*header)
	};


	while (true) {
		esp_err_t err = camera_run();
		if (err != ESP_OK) {
			ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
			return;
		}

		err = http_response_begin_multipart(http_ctx, "image/bitmap",
				camera_get_data_size() + sizeof(*header));
		if (err != ESP_OK) {
			break;
		}
		err = http_response_write(http_ctx, &bmp_header);
		if (err != ESP_OK) {
			break;
		}
		err = write_frame(http_ctx);
		if (err != ESP_OK) {
			break;
		}
		err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
		if (err != ESP_OK) {
			break;
		}
	}

	free(header);
	http_response_end(http_ctx);
}

static void handle_jpg_stream(http_context_t http_ctx, void* ctx)
{
	http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);
	if(get_light_state())
		led_open();
	while (true) {
		esp_err_t err = camera_run();
		if (err != ESP_OK) {
			ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
			return;
		}
		err = http_response_begin_multipart(http_ctx, "image/jpg",
				camera_get_data_size());
		if (err != ESP_OK) {
			break;
		}
		err = write_frame(http_ctx);
		if (err != ESP_OK) {
			break;
		}
		err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
		if (err != ESP_OK) {
			break;
		}
	}
	http_response_end(http_ctx);
	led_close();
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
		s_ip_addr = event->event_info.got_ip.ip_info.ip;
		s_ip_addr_changed = 1;
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
		s_ip_addr.addr = 0;
		s_ip_addr_changed = 1;
		break;
	default:
		break;
	}
	return ESP_OK;
}

static void initialise_wifi(void)
{
	tcpip_adapter_init();
	s_wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	//    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	wifi_config_t wifi_config = {
			.sta = {
					.ssid = CONFIG_WIFI_SSID,
					.password = CONFIG_WIFI_PASSWORD,
			},
	};
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA ,"zumoros");
	ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE) );
	ESP_LOGI(TAG, "Connecting to \"%s\"", wifi_config.sta.ssid);
	xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(TAG, "Connected");
}
