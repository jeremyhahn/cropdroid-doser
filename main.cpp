#include <Arduino.h>
#include <Ethernet.h>

#define BUFSIZE 255

byte mac[] = { 0x04, 0x02, 0x00, 0x00, 0x00, 0x03 };
IPAddress ip(192,168,0,201);
EthernetServer httpServer(80);
EthernetClient httpClient;

void handleWebRequest();
void sendHtmlHeader();
void send404();
void send500(String message);
void(* resetFunc) (void) = 0;

const int channel_size = 6;
int valid_channels[channel_size] = {2, 5, 6, 7, 8, 9};

int doseChannel = 0;
unsigned long doseStart, doseStop = 0;
long doseInterval = 0;

int main(void) {
  init();
  setup();;

  for (;;)
    loop();

  return 0;
}

void setup(void) {

  Serial.begin(115200);
  Serial.println("Initializing nute controller...");

  analogReference(DEFAULT);


  for(int i=0; i<channel_size; i++) {
	  pinMode(valid_channels[i], OUTPUT);
	  digitalWrite(valid_channels[i], LOW);
  }

  if(Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed");
    Ethernet.begin(mac, ip);
  }

  httpServer.begin();

  Serial.print("REST server listening on ");
  Serial.println(Ethernet.localIP());

  Serial.println("Starting...");
}

void loop() {
  handleWebRequest();
  delay(100);
}

void handleWebRequest() {

	unsigned long currentMillis = millis();

	if(doseInterval > 0) {
		if(currentMillis - doseStart > doseInterval) {
			doseStop = currentMillis;

			Serial.print("Turning off dosing pump on channel ");
			Serial.print(doseChannel);
			Serial.print(" after ");
			Serial.print(doseInterval / 1000);
			Serial.println(" seconds");

			digitalWrite(doseChannel, LOW);

			doseChannel = 0;
			doseStart = 0;
			doseStop = 0;
			doseInterval = 0;
		}
	}

	httpClient = httpServer.available();

	char clientline[BUFSIZE];
	int index = 0;
	bool reset = false;

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

				Serial.print("clientline: ");
				Serial.println(clientline);

				char *resource = strtok(clientline, "/");
				char *param1 = strtok(NULL, "/");
				char *param2 = strtok(NULL, "/");

				Serial.print("HTTP Resource: ");
				Serial.println(resource);

				Serial.print("Param1: ");
				Serial.println(param1);

				Serial.print("Param2: ");
				Serial.println(param2);

				String jsonOut = String();

				// /status
				if (strncmp(resource, "status", 6) == 0) {
					jsonOut += "{";
						jsonOut += "\"uptime\":\"" + String(millis()) + "\", ";
						for(int i=0; i<channel_size; i++) {
							jsonOut += "\"channel" + String(valid_channels[i]) + "\":\"" + String(digitalRead(valid_channels[i])) + "\", ";
						}
						jsonOut += "\"channel\":\"" + String(doseChannel) + "\", ";
						jsonOut += "\"start\":\"" + String(doseStart) + "\", ";
						jsonOut += "\"stop\":\"" + String(doseStop) + "\", ";
						jsonOut += "\"interval\":\"" + String(doseInterval) + "\" ";

					jsonOut += "}";
				}
				// /dispense
				else if (strncmp(resource, "dispense", 8) == 0) {

					if(param1 == NULL || param1 == "") {
						send500("parameter required");
						break;
					}

					int channel = atoi(param1);
					int duration = atoi(param2);

					if(duration > 0) {

						Serial.print("Channel: ");
						Serial.println(channel);

						Serial.print("Duration: ");
						Serial.println(duration);

						digitalWrite(channel, HIGH);

						doseInterval = duration * 1000;
						doseChannel = channel;
						doseStart = millis();
					}
					jsonOut += "{";
						jsonOut += "\"channel\":\"" + String(channel) + "\", ";
						jsonOut += "\"duration\":\"" + String(duration) + "\" ";
					jsonOut += "}";
				}
				else if (strncmp(resource, "reset", 5) == 0) {
					Serial.println("Rebooting...");
					jsonOut += "{";
						jsonOut += "\"reset\":\"true\"";
					jsonOut += "}";
					reset = true;
				}
				else {
					send404();
					break;
				}

				httpClient.println("HTTP/1.1 200 OK");
				//httpClient.println("Content-Type: text/html");
				//httpClient.println("Access-Control-Allow-Origin: *");
				//httpClient.println("X-Powered-By: Harvest Room Controller v1.0");
				httpClient.println();
				httpClient.println(jsonOut);

				break;
			}
		}
	}

	// give the web browser time to receive the data
	delay(100);

	// close the connection:
	httpClient.stop();

	if(reset)  {
	  resetFunc();
	}
}

void sendHtmlHeader() {

	httpClient.println("<h5>Harvest Room Controller v1.0</h5>");
}

void send404() {

	httpClient.println("HTTP/1.1 404 Not Found");
	//httpClient.println("Content-Type: text/html");
	//httpClient.println("X-Powered-By: Harvest Room Controller v1.0");
	httpClient.println();

	sendHtmlHeader();
	httpClient.println("<h1>Not Found</h1>");
}

void send500(String message) {

	httpClient.println("HTTP/1.1 500 Internal Server Error");
	//httpClient.println("Content-Type: text/html");
	//httpClient.println("X-Powered-By: Harvest Room Controller v1.0");
	httpClient.println();

	sendHtmlHeader();
	httpClient.print("<h1>Error: ");
	httpClient.print(message);
	httpClient.println("</h1>");
}
