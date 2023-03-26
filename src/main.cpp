#include <WiFi.h>
#include "WiFiClient.h"
#include "esp_camera.h"
#include "lwip/sockets.h"
#include "sensor.h"
#include <Arduino.h>
#include <cstring>
#include <secrets.h>
#include <sys/socket.h>
#include <errno.h>
#include <string>
/*Pins {
			D0: 4,
			D1: 5,
			D2: 18,
			D3: 19,
			D4: 36,
			D5: 39,
			D6: 34,
			D7: 35,
			XCLK: 21,
			PCLK: 22,
			VSYNC: 25,
			HREF: 23,
			SDA: 26,
			SCL: 27,
			RESET: -1,
			PWDN: -1
		}*/
//WROVER-KIT PIN Map
#define CAM_PIN_PWDN    -1 //power down is not used
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    21
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      19
#define CAM_PIN_D2      18
#define CAM_PIN_D1       5
#define CAM_PIN_D0       4
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22


static camera_config_t camera_config_dumb = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB444,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1, //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

static camera_config_t camera_config_full = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1, //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

const char* streamTo = "192.168.4.2";

esp_err_t camera_init(bool jpeg){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

		esp_err_t err;
    //initialize the camera in grayscale mode
		if(!jpeg) {
			err = esp_camera_init(&camera_config_dumb);
		} else {
			err = esp_camera_init(&camera_config_full);
		}

    if (err != ESP_OK) {
        Serial.println("Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

void stream_video() {
	WiFiClient client;
	IPAddress addr;
	if (addr.fromString(streamTo)) {
		Serial.println("Couldn't parse IP");
		return;
	}
	if(!client.connect(addr, 8001)) {
		Serial.println("Failed to connect");
		return;
	}

	esp_camera_deinit();
	camera_init(true);
	
	// stream video
	for(int i = 0; i < 2; i++) {
		camera_fb_t * fb = esp_camera_fb_get();

		if(fb == nullptr) {
			Serial.println("Framebuffer was a null pointer");
			return;
		}

		// simple protocol: image length followed by image
		//client.write(fb->len);
		client.write("hello");
		client.write(fb->len);
		Serial.printf("Wrote length %d in %d bytes\n", fb->len, sizeof(fb->len));

		// JPEG image
		//client.write(fb->buf, fb->len);
	}
	//client.print("END");
	client.stop();
	esp_camera_deinit();
	//delay(1000);
	camera_init(false);
}
int detect_red(int thresh) {
	camera_fb_t * fb = esp_camera_fb_get();
	
	if(!fb) {
		Serial.println("Camera Capture Failed");
		return 0;
	}
	int32_t sum = 0;
	int32_t nonsum = 0;

	for(int i = 0; i<fb->len; i +=1) {
		if(i%2 == 0) {
			sum += (fb->buf[i] & 248) >> 3; 
			nonsum += (fb->buf[i] & 7) << 2;  
		} else {
			nonsum += (fb->buf[i] & 224) >> 6;  
			nonsum += (fb->buf[i] & 31);  
		}
	}

	sum = sum - (nonsum/2);
	esp_camera_fb_return(fb);
	return sum;
}

bool motion_detect(int thresh) {
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();

    if (!fb) {
        Serial.println("Camera Capture Failed");
				return false;
    }

		// copy photo into new memory
		uint8_t *fb1mem = (uint8_t*) malloc(fb->len);
		std::memcpy(fb1mem, fb->buf, fb->len);

		// return memory
    esp_camera_fb_return(fb);
		if (!fb) {
        Serial.println("Camera Capture Failed");
				return false;
    }

		// acquire another frame
    fb = esp_camera_fb_get();

		int sum = 0;
		for(int i=1; i<fb->len; i++) {
			sum += fb1mem[i] - fb->buf[i] > 0 ? fb1mem[i] - fb->buf[i] :  fb->buf[i] - fb1mem[i];
		}
		free(fb1mem);

    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
		if(sum > thresh) {
			Serial.printf("Motion threshold reached %d\n", sum);
		} 
		return sum > thresh;
}

void loop(){
		int thresh = detect_red(100);
		
	
		Serial.printf("Red Detection Threshold Reached at:%d\n", thresh);
		stream_video();
		
  
   // return ESP_OK;
}



void setup() {
  Serial.begin(115200);
  Serial.println();
	camera_init(false);

	// set up wifi or connect to network
	WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi failure");
    delay(5000);

		// reconfigure WiFi for soft AP
		WiFi.persistent(true);
		WiFi.mode(WIFI_AP);
		WiFi.softAP("James's iPhone", "poopcamera");
		Serial.print("http://");
		Serial.println(WiFi.softAPIP());
  } else {
		Serial.print("http://");
		Serial.println(WiFi.localIP());
	}
  Serial.println("WiFi connected");
}
