// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * This is an Arduino-based Azure IoT Hub sample for ESPRESSIF ESP32 boards.
 * It uses our Azure Embedded SDK for C to help interact with Azure IoT.
 * For reference, please visit https://github.com/azure/azure-sdk-for-c.
 *
 * To connect and work with Azure IoT Hub you need an MQTT client, connecting, subscribing
 * and publishing to specific topics to use the messaging features of the hub.
 * Our azure-sdk-for-c is an MQTT client support library, helping composing and parsing the
 * MQTT topic names and messages exchanged with the Azure IoT Hub.
 *
 * This sample performs the following tasks:
 * - Synchronize the device clock with a NTP server;
 * - Initialize our "az_iot_hub_client" (struct for data, part of our azure-sdk-for-c);
 * - Initialize the MQTT client (here we use ESPRESSIF's esp_mqtt_client, which also handle the tcp
 * connection and TLS);
 * - Connect the MQTT client (using server-certificate validation, SAS-tokens for client
 * authentication);
 * - Periodically send telemetry data to the Azure IoT Hub.
 *
 * To properly connect to your Azure IoT Hub, please fill the information in the `iot_configs.h`
 * file.
 */

// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <HTTPClient.h>
#include <mqtt_client.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// Additional sample headers
#include "AzIoTSasToken.h"
#include "SerialLogger.h"
#include "iot_configs.h"

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

#define SERIAL_PORT_RX 16 // GPIO16 is used as RX
#define SERIAL_PORT_TX 17 // GPIO17 is used as TX

// Translate iot_configs.h defines into variables used by the sample
static const char* ssid = IOT_CONFIG_WIFI_SSID;
static const char* password = IOT_CONFIG_WIFI_PASSWORD;
static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char* mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint32_t telemetry_send_count = 0;
static String telemetry_payload = "{}";
static String time_current;
static uint32_t count=0;

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

// Auxiliary functions
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

const size_t arraySize = 47;  // Adjust the size as needed
uint8_t value[arraySize] = {0};

typedef struct
{
  uint16_t light;
  uint16_t temp;
  uint16_t humid;
  uint16_t Voc;
  uint16_t Nox;
  uint16_t O2;
  uint16_t NDIR;
  uint16_t S302;
  uint16_t S402;
  uint16_t S502;
  uint16_t S602;
  uint16_t S702;
  uint16_t S802;
  uint16_t Voc2;
  uint16_t heater;
}Message_t;

Message_t message;

bool parseDataPacket(const char *packet, uint8_t *value, size_t maxSize) 
{
  if (strncmp(packet, "Buf:", 4) != 0) {
    return false; // Invalid packet format
  }

  packet += 4;
  size_t index = 0;
  char *endPtr;
  char *token = strtok_r((char*)packet, ",", &endPtr);
  while (token != nullptr && index < maxSize) {
    value[index++] = (uint8_t)atoi(token);
    token = strtok_r(nullptr, ",", &endPtr);
  }

  return true;
}

static void connectToWiFi()
{
  Logger.Info("Connecting to WIFI SSID " + String(ssid));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Logger.Info("WiFi connected, IP address: " + WiFi.localIP().toString());
}

static void initializeTime()
{
  Logger.Info("Setting time using SNTP");

  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  Logger.Info("Time initialized!");
}

void receivedCallback(char* topic, byte* payload, unsigned int length)
{
  Logger.Info("Received [");
  Logger.Info(topic);
  Logger.Info("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
    int i, r;

    case MQTT_EVENT_ERROR:
      Logger.Info("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      Logger.Info("MQTT event MQTT_EVENT_CONNECTED");

      r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
      if (r == -1)
      {
        Logger.Error("Could not subscribe for cloud-to-device messages.");
      }
      else
      {
        Logger.Info("Subscribed for cloud-to-device messages; message id:" + String(r));
      }

      break;
    case MQTT_EVENT_DISCONNECTED:
      Logger.Info("MQTT event MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      Logger.Info("MQTT event MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      Logger.Info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      Logger.Info("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      Logger.Info("MQTT event MQTT_EVENT_DATA");

      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
      {
        incoming_data[i] = event->topic[i];
      }
      incoming_data[i] = '\0';
      Logger.Info("Topic: " + String(incoming_data));

      for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
      {
        incoming_data[i] = event->data[i];
      }
      incoming_data[i] = '\0';
      Logger.Info("Data: " + String(incoming_data));

      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Logger.Info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      Logger.Error("MQTT event UNKNOWN");
      break;
  }

  return ESP_OK;
}

static void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
  {
    Logger.Error("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Logger.Error("Failed getting client id");
    return;
  }

  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    Logger.Error("Failed to get MQTT clientId, return code");
    return;
  }

  Logger.Info("Client ID: " + String(mqtt_client_id));
  Logger.Info("Username: " + String(mqtt_username));
}

static int initializeMqttClient()
{
#ifndef IOT_CONFIG_USE_X509_CERT
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    Logger.Error("Failed generating SAS token");
    return 1;
  }
#endif

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;

#ifdef IOT_CONFIG_USE_X509_CERT
  Logger.Info("MQTT client using X509 Certificate authentication");
  mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
  mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
#else // Using SAS key
  mqtt_config.password = (const char*)az_span_ptr(sasToken.Get());
#endif

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*)ca_pem;
  Logger.Info("Password: "+ String(mqtt_config.password));
  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    Logger.Error("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Logger.Error("Could not start mqtt client; error code:" + start_result);
    return 1;
  }
  else
  {
    Logger.Info("MQTT client started");
    return 0;
  }
}

/*
 * @brief  Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */
static uint32_t getEpochTimeInSecs() { return (uint32_t)time(NULL); }

static void establishConnection()
{
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

static void generateTelemetryPayload()
{
  // You can generate the JSON using any lib you want. Here we're showing how to do it manually, for simplicity.
  // This sample shows how to generate the payload using a syntax closer to regular delevelopment for Arduino, with
  // String type instead of az_span as it might be done in other samples. Using az_span has the advantage of reusing the 
  // same char buffer instead of dynamically allocating memory each time, as it is done by using the String type below.
  // telemetry_payload = "{\"deviceName\":\"Unit3\",\"s302\":2,\"s402\":1,\"s502\":3,\"s602\":4,\"s702\":5,\"s802\":6,\"temp\":7,\"ndir\":8,\"nox\":9,\"voc\":10,\"voc2\":11,\"o2\":12,\"humid\":13,\"light\":14,\"gpsx\":115.83794,\"gpsy\":-32.06731,\"ef\":null,\"postedTime\":\"2024-6-6T13:20:36.301\",\"note\":\"completed\"},";
  
  telemetry_payload = String("{\"deviceName\":\"Unit3\",") +
                          "\"s302\":" + String(message.S302) + "," +
                          "\"s402\":" + String(message.S402) + "," +
                          "\"s502\":" + String(message.S502) + "," +
                          "\"s602\":" + String(message.S602) + "," +
                          "\"s702\":" + String(message.S702) + "," +
                          "\"s802\":" + String(message.S802) + "," +
                          "\"temp\":" + String(message.temp) + "," +
                          "\"ndir\":" + String(message.NDIR) + "," +
                          "\"nox\":" + String(message.Nox) + "," +
                          "\"voc\":" + String(message.Voc) + "," +
                          "\"voc2\":" + String(message.Voc2) + "," +
                          "\"o2\":" + String(message.O2) + "," +
                          "\"humid\":" + String(message.humid) + "," +
                          "\"light\":" + String(message.light) + "," +
                          "\"gpsx\":115.83794," +
                          "\"gpsy\":-32.06731," +
                          "\"ef\":null," +
                          "\"postedTime\":\"" + String(time_current) + "\"," +  // Ensure time is in quotes
                          "\"note\":\"completed\"}";
  
  Logger.Info(telemetry_payload);
  delay(150);
}

static void sendTelemetry()
{
  Logger.Info("Sending telemetry ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Logger.Error("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  generateTelemetryPayload();

  if (esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char*)telemetry_payload.c_str(),
          telemetry_payload.length(),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == 0)
  {
    Logger.Error("Failed publishing");
  }
  else
  {
    Logger.Info("Message published successfully");
  }
}
void http_hit()
{
  // HTTPClient http;
  struct tm* ptm;
  time_t now = time(NULL);
  now += GMT_OFFSET;
  ptm = gmtime(&now);

  int year = ptm->tm_year + UNIX_EPOCH_START_YEAR;
  int month = ptm->tm_mon + 1;
  int mday = ptm->tm_mday;
  if (ptm->tm_hour < 10)
  {
    // Serial.print(0);
    int hour = 0;
  }
  int hour = ptm->tm_hour;
  if (ptm->tm_min < 10)
  {
    // Serial.print(0);
    int min = 0;
  }
  int min = ptm->tm_min;
  if (ptm->tm_sec < 10)
  {
    // Serial.print(0);
    int sec = 0;
  }
  int sec = ptm->tm_sec;
  time_current = (String) year+"-"+month+"-"+mday+"T"+hour+":"+min+":"+sec;
  // String url = "http://cms9.southeastasia.cloudapp.azure.com/IOTApi/api/DeviceInfo/UpdateFumigationStart/"+device+"/"+time_current;

  // Serial.println("OPEN " + url);

  // http.begin(url);
  // http.addHeader("Content-Type", "text/plain");
  // http.addHeader("Content-Length", "0");
  // Serial.println("SEND");
  // int httpResponseCode = http.PUT("");

  // if (httpResponseCode > 0) {
  //   Serial.printf("HTTP Response code: %d\n", httpResponseCode);
  // } else {
  //   Serial.printf("Error code: %d\n", httpResponseCode);
  // }

  // Serial.println("CLOSE");
  // http.end();
}

// Arduino setup and loop main functions.

void setup()
{ 
  pinMode(27, INPUT_PULLDOWN);
  establishConnection();
  Serial1.begin(115200, SERIAL_8N1, SERIAL_PORT_RX, SERIAL_PORT_TX); // Serial1 with baud rate 9600
  Serial.print("Received: ");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
#ifndef IOT_CONFIG_USE_X509_CERT
  else if (sasToken.IsExpired())
  {
    Logger.Info("SAS token expired; reconnecting with a new one.");
    (void)esp_mqtt_client_destroy(mqtt_client);
    initializeMqttClient();
  }
#endif
  else if (millis() > next_telemetry_send_time_ms)
  {
    // delay(100);
    // if(count ==0 && digitalRead(27))
    // {
    //   for(int i=10;i<=12;i++)
    //     http_hit((String) i);
    //   count++;
    // }

    if(digitalRead(27))
    {
      if (Serial1.available()) 
      {
        // Print the incoming data to the Serial Monitor
        String ch=Serial1.readStringUntil('\n');
        Serial.println(ch);
        
        // Parse the data packet
        if (parseDataPacket(ch.c_str(), value, arraySize))
        {
          // Print the parsed value for debugging
          Serial.println("Value Parsed!");
          message.light=(value[15]<<8)|value[16]; message.temp=(value[18]<<8)|value[18];  
          message.humid=(value[19]<<8)|value[20]; message.Voc=(value[21]<<8)|value[22];   
          message.O2=(value[23]<<8)|value[24];    message.NDIR=(value[25]<<8)|value[26];  
          message.S302=(value[27]<<8)|value[28];  message.S402=(value[29]<<8)|value[30];  
          message.S502=(value[31]<<8)|value[32];  message.S602=(value[33]<<8)|value[34];  
          message.S702=(value[35]<<8)|value[36];  message.S802=(value[37]<<8)|value[38];  
          message.Voc2=(value[39]<<8)|value[40];  message.heater=(value[41]<<8)|value[42];
          http_hit();
          sendTelemetry();
        } 
        else
        {
          Serial.println("Invalid packet format.");
        }
      }
      next_telemetry_send_time_ms = millis() + TELEMETRY_FREQUENCY_MILLISECS;
    }
  }
}
