//MQTT
#define THING_NAME "OBD2_TCC"
#define MQTT_PACKET_SIZE  2048

const char* mqttServer = "a2tdr5m7pix1fz-ats.iot.sa-east-1.amazonaws.com";
const int mqttPort = 8883;
const char topic[] = "arn:aws:iot:sa-east-1:979721598108:topicfilter/Sensor/OBD";
char publishPayload[MQTT_PACKET_SIZE];
