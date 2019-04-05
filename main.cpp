#include <Arduino.h>
#include <Ethernet.h>
#include "EEPROM.h"

#define DEBUG 1
#define EEPROM_DEBUG 0
#define BUFSIZE 255

extern int  __bss_end;
extern int  *__brkval;

const char json_bracket_open[] = "{";
const char json_bracket_close[] = "}";

const char string_initializing[] PROGMEM = "Initializing nutrient controller...";
const char string_dhcp_failed[] PROGMEM = "DHCP Failed";
const char string_http_200[] PROGMEM = "HTTP/1.1 200 OK";
const char string_http_404[] PROGMEM = "HTTP/1.1 404 Not Found";
const char string_http_500[] PROGMEM = "HTTP/1.1 500 Internal Server Error";
const char string_http_content_type_json[] PROGMEM = "Content-Type: application/json";
const char string_http_xpowered_by[] PROGMEM = "X-Powered-By: CropDroid v1.0";
const char string_rest_address[] PROGMEM = "REST service listening on: ";
const char string_switch_on[] PROGMEM = "Switching on";
const char string_switch_off[] PROGMEM = "Switching off";
const char string_json_key_mem[] PROGMEM = "\"mem\":";
const char string_json_key_uptime[] PROGMEM = ",\"uptime\":";
const char string_json_key_channels[] PROGMEM = ",\"channels\":{";
const char string_json_key_value[] PROGMEM =  ",\"value\":";
const char string_json_key_address[] PROGMEM =  "\"address\":";
const char string_json_bracket_open[] PROGMEM = "{";
const char string_json_bracket_close[] PROGMEM = "}";
const char string_json_error_invalid_channel[] PROGMEM = "{\"error\":\"Invalid channel\"}";
const char string_json_reboot_true PROGMEM = "{\"reboot\":true}";
const char string_json_reset_true PROGMEM = "{\"reset\":true}";
const char * const string_table[] PROGMEM = {
  string_initializing,
  string_dhcp_failed,
  string_http_200,
  string_http_404,
  string_http_500,
  string_http_content_type_json,
  string_http_xpowered_by,
  string_rest_address,
  string_switch_on,
  string_switch_off,
  string_json_key_mem,
  string_json_key_uptime,
  string_json_key_channels,
  string_json_key_value,
  string_json_key_address,
  string_json_bracket_open,
  string_json_bracket_close,
  string_json_error_invalid_channel,
  string_json_reboot_true,
  string_json_reset_true
};
int idx_initializing = 0,
    idx_dhcp_failed = 1,
	idx_http_200 = 2,
	idx_http_404 = 3,
	idx_http_500 = 4,
	idx_http_content_type_json = 5,
	idx_http_xpowered_by = 6,
	idx_rest_address = 7,
	idx_switch_on = 8,
	idx_switch_off = 9,
	idx_json_key_mem = 10,
	idx_json_key_uptime = 11,
	idx_json_key_channels = 12,
	idx_json_key_value = 13,
	idx_json_key_address = 14,
	idx_json_key_bracket_open = 15,
	idx_json_key_bracket_close = 16,
	idx_json_error_invalid_channel = 17,
	idx_json_reboot_true = 18,
	idx_json_reset_true = 19;
char string_buffer[50];
char float_buffer[10];

void handleWebRequest();
void switchOn(int pin);
void switchOff(int pin);
void send404();
void resetDefaults();
int availableMemory();
void(* resetFunc) (void) = 0;

byte defaultMac[] = { 0x04, 0x02, 0x00, 0x01, 0x00, 0x03 };

byte mac[] = {
	EEPROM.read(0),
	EEPROM.read(1),
	EEPROM.read(2),
	EEPROM.read(3),
	EEPROM.read(4),
	EEPROM.read(5)
};
IPAddress ip(
  EEPROM.read(6),
  EEPROM.read(7),
  EEPROM.read(8),
  EEPROM.read(9));

EthernetServer httpServer(80);
EthernetClient httpClient;

const int NULL_CHANNEL = 255;
const int channel_size = 16;
const uint8_t channels[channel_size] = {
	2, 3, 4, 5, 6, 7, 8, 9,
	A0, A1, A2, A3, A4, A5, A6, A7
	//19, 20, 21, 22, 23, 24, 25, 26
};

/*
struct dose {
	int channel;
	unsigned long start;
	long interval;
};
struct dose doseTable[channel_size];
*/

// channel, start, interval
unsigned long channel_table[channel_size][3] = {
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0},
   {NULL_CHANNEL, 0, 0}
};

int main(void) {
  init();
  setup();;

  for (;;)
    loop();

  return 0;
}

void setup(void) {

  #if DEBUG || EEPROM_DEBUG
	Serial.begin(115200);

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_initializing])));
	Serial.println(string_buffer);
  #endif

  analogReference(DEFAULT);

  for(int i=0; i<channel_size; i++) {
    pinMode(channels[i], OUTPUT);
    digitalWrite(channels[i], LOW);
	/*
    doseTable[i].channel = NULL_CHANNEL;
    doseTable[i].start = 0;
    doseTable[i].interval = 0;
    */
  }

  byte macByte1 = EEPROM.read(0);

  #if DEBUG || EEPROM_DEBUG
	Serial.print("EEPROM.read(0): ");
	Serial.println(macByte1);
  #endif

  if(macByte1 == 255) {
	resetDefaults();
	if(Ethernet.begin(defaultMac) == 0) {
	  #if DEBUG
		strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_dhcp_failed])));
		Serial.println(string_buffer);
	  #endif
	  return;
	}
  }
  else {
	Ethernet.begin(mac, ip);
  }

  #if DEBUG || EEPROM_DEBUG
	Serial.print("MAC: ");
	for(int i=0; i<6; i++) {
	  Serial.print(mac[i], HEX);
	}
	Serial.println();

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_rest_address])));
	Serial.print(string_buffer);
	Serial.println(Ethernet.localIP());
  #endif

  httpServer.begin();
}

void loop() {
  handleWebRequest();
  delay(100);
}

void switchOn(int pin) {
#if DEBUG
  char sPin[3];
  itoa(pin, sPin, 10);
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_switch_on])));
  strcat(string_buffer, " pin ");
  strcat(string_buffer, sPin);
  Serial.println(string_buffer);
#endif
  digitalWrite(pin, HIGH);
}

void switchOff(int pin) {
#if DEBUG
  char sPin[3];
  itoa(pin, sPin, 10);
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_switch_off])));
  strcat(string_buffer, " pin ");
  strcat(string_buffer, sPin);
  Serial.println(string_buffer);
#endif
  digitalWrite(pin, LOW);
}

void handleWebRequest() {

	unsigned long currentMillis = millis();

	for(int i=0; i<channel_size; i++) {
		if(channel_table[i][0] != NULL_CHANNEL) {
			if(currentMillis - channel_table[i][1] > channel_table[i][2]) {
				digitalWrite(channel_table[i][0], LOW);

				Serial.print("Shutdown: ");
				Serial.println(channel_table[i][0]);

/*
				#if DEBUG
				  Serial.print("Turning channel ");
				  Serial.println(channel_table[i][0]);
				  Serial.print(" off after ");
				  Serial.print(channel_table[i][2] / 1000);
				  Serial.println(" seconds");
				#endif
*/
				channel_table[i][0] = NULL_CHANNEL;
				channel_table[i][1] = 0;
				channel_table[i][2] = 0;
			}
		}
	}

	httpClient = httpServer.available();

	char clientline[BUFSIZE];
	int index = 0;

	bool reboot = false;
	char json[275];
	char sPin[3];
	char state[3];

	if (httpClient) {

		// reset input buffer
		index = 0;

		while (httpClient.connected()) {

			if (httpClient.available()) {

				char c = httpClient.read();
				if (c != '\n' && c != '\r' && index < BUFSIZE) {
					clientline[index++] = c;
					continue;
				}

				httpClient.flush();

				String urlString = String(clientline);
				String op = urlString.substring(0, urlString.indexOf(' '));
				urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));
				urlString.toCharArray(clientline, BUFSIZE);

			    char *resource = strtok(clientline, "/");
				char *param1 = strtok(NULL, "/");
				char *param2 = strtok(NULL, "/");

				#if DEBUG
				  Serial.print("Resource: ");
				  Serial.println(resource);

				  Serial.print("Param1: ");
				  Serial.println(param1);

				  Serial.print("Param2: ");
				  Serial.println(param2);
				#endif

				// /status
				if (strncmp(resource, "status", 6) == 0) {

					strcpy(json, "{");

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_mem])));
					  strcat(json, string_buffer);
					  itoa(availableMemory(), float_buffer, 10);
					  strcat(json, float_buffer);
/*
					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_uptime])));
					  strcat(json, string_buffer);
					  itoa(millis(), float_buffer, 10);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_uptime])));
					  strcat(json, string_buffer);
					  int uptime = snprintf(NULL, 0, "%llu", millis());
					  itoa(uptime, float_buffer, 10);
					  strcat(json, float_buffer);
*/

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_channels])));
					  strcat(json, string_buffer);
						for(int i=0; i<channel_size; i++) {
						  //itoa(channels[i], sPin, 10);
						  itoa(digitalRead(channels[i]), state, 10);

						  strcat(json, "\"");
						  itoa(i, float_buffer, 10);
						  strcat(json, float_buffer);
						  strcat(json, "\":");

						  strcat(json, state);
						  if(i + 1 < channel_size) {
							  strcat(json, ",");
						  }
						}
					  strcat(json, "}");

					strcat(json, "}");
				}

				// /eeprom
				else if (strncmp(resource, "eeprom", 6) == 0) {

					#if EEPROM_DEBUG
						Serial.println("/eeprom");
						Serial.println(param1);
						Serial.println(param2);
					#endif

					EEPROM.write(atoi(param1), atoi(param2));

					strcpy(json, json_bracket_open);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_address])));
					  strcat(json, string_buffer);
					  strcat(json, param1);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_value])));
					  strcat(json, string_buffer);
					  strcat(json, param2);

					strcat(json, json_bracket_close);
				}

				// /dispense
				else if (strncmp(resource, "dispense", 8) == 0) {

					if(param1 == NULL || param1 == "") {
						Serial.println("parameter required");
						//send500("parameter required");
						break;
					}

					int channel = atoi(param1);
					int duration = atoi(param2);

					if(channel > 0 && duration > 0) {

						#if DEBUG
						  Serial.print("Channel: ");
						  Serial.println(channel);

						  //Serial.print("Duration: ");
                          //Serial.println(duration);
						#endif


                        if(channel > 0 && duration > 0) {
                        	channel_table[channel][0] = channel;
                        	channel_table[channel][1] = millis();
                        	channel_table[channel][2] = duration * 1000;
                            digitalWrite(channel, HIGH);
                        }
                        else {
							#if DEBUG
							  Serial.println("Invalid channel/duration");
							#endif
						}
					}

					strcpy(json, "{");

						strcat(json, "\"channel\":");
						itoa(channel, float_buffer, 10);
						strcat(json, float_buffer);

						strcat(json, ",\"duration\":");
						itoa(duration, float_buffer, 10);
						strcat(json, float_buffer);

					strcat(json, "}");
				}

				// /reboot
				else if (strncmp(resource, "reboot", 6) == 0) {
					#if DEBUG
					  Serial.println("Rebooting");
					#endif
					strcpy(json, "{\"reboot\":true}");
					reboot = true;
				}

				// /reset
				else if (strncmp(resource, "reset", 5) == 0) {
					#if DEBUG
						Serial.println("/reset");
					#endif

					resetDefaults();

					strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_reset_true])));
					strcat(json, string_buffer);
				}

				else {
					#if DEBUG
					  Serial.println("404");
					#endif
					send404();
					break;
				}

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_200])));
				httpClient.println(string_buffer);

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_xpowered_by])));
				httpClient.println(string_buffer);

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_content_type_json])));
				httpClient.println(string_buffer);

				httpClient.println();
				httpClient.println(json);

				break;
			}
		}
	}

	// give the web browser time to receive the data
	delay(100);

	// close the connection:
	httpClient.stop();

	if(reboot)  {
	  resetFunc();
	}
}

void send404() {

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_404])));
	httpClient.println(string_buffer);

	#if DEBUG
	  Serial.println(string_buffer);
	#endif

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_xpowered_by])));
	httpClient.println(string_buffer);

	httpClient.println();
}

void resetDefaults() {
	EEPROM.write(0, defaultMac[0]);
	EEPROM.write(1, defaultMac[1]);
	EEPROM.write(2, defaultMac[2]);
	EEPROM.write(3, defaultMac[3]);
	EEPROM.write(4, defaultMac[4]);
	EEPROM.write(5, defaultMac[5]);
	EEPROM.write(6, 192);
	EEPROM.write(7, 168);
	EEPROM.write(8, 0);
	EEPROM.write(9, 93);
}

int availableMemory() {
  int free_memory;
  return ((int) __brkval == 0) ? ((int) &free_memory) - ((int) &__bss_end) :
      ((int) &free_memory) - ((int) __brkval);
}
