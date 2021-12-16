//DEBUG
#define UART_DEBUG

#ifdef UART_DEBUG
  #define DEBUG(x)            Serial.println("[" + String(xPortGetCoreID()) + "] " + x);
  #define DEBUGB(x)           Serial.print("[" + String(xPortGetCoreID()) + "] " + x);
#else
  #define DEBUG(x)
  #define DEBUGB(x)
#endif


#include <base64.h>
#include "var_gprs.h"
#include <OBD2UART.h>
#include <Wire.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SSLClient.h>
#include "certificates.h"
#include "var_mqtt.h"

void callback(char* topic, byte* payload, unsigned int length) {}

TinyGsm modemGSM(SerialGSM);
TinyGsmClient client(modemGSM);
SSLClient modemGSMSSL(&client);
PubSubClient mqtt(mqttServer, mqttPort, callback, modemGSMSSL);


static String OBD_COLLECTION = "";
static bool send = false;

COBD obd;

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  
  //Configuracao para o GPRS
  SerialGSM.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modemGSMSSL.setCACert(ca_aws);
  modemGSMSSL.setCertificate(my_cert);
  modemGSMSSL.setPrivateKey(my_key);
  
  for (;;) {
    delay(1000);
    byte version = obd.begin();
    DEBUGB("Adaptador OBD-II ");
    if (version > 0) {
      DEBUG("detectado ");
      DEBUGB("Versao ");
      DEBUGB(version / 10);
      DEBUGB('.');
      DEBUG(version % 10);
      break;
    } else {
      DEBUG("nao derectado");
    }
  }

  for (;;) {
    delay(1000);
    DEBUGB("Carregado protocolo OBD ");
    if (obd.init()) {
      DEBUG("incializado");
      break;
    } else {
      DEBUG("nao inicializado");
    }
  }

  
  for(;;){
    delay(1000);
    DEBUGB("Carregado Modem GSM ");
    if (conectarGprs()) {
      DEBUG("incializado");
      break;
    } else {
      DEBUG("nao inicializado");
    }
  }

  DEBUG("Inicia TaskModemRun");
  xTaskCreatePinnedToCore(TaskModemRun, "TaskModemRun", 30000, NULL, 1, NULL, 0);
  delay(500);

  DEBUG("Inicia TaskOBDRun");
  xTaskCreatePinnedToCore(TaskOBDRun, "TaskOBDRun", 20000, NULL, 1, NULL, 1);
  delay(500); 
   

}

void loop()
{ 
  //vTaskDelete(NULL);
}

void TaskModemRun(void * pvParameters)
{
  DEBUG("Modem init");
  for(;;){
    DEBUG("Modem loop"); 
    for(byte i=0;i<15;i++){
        delay(1000);
      }
      DEBUG("Modem");
      sendRequest();
      yield();
    }
  }

void TaskOBDRun(void * pvParameters)
{
  DEBUG("OBD init");
  for(;;){
    if(send){
      OBD_COLLECTION = "";
      send = false;
    }
    
    //DEBUG("OBD loop");
    static byte pids[]= {PID_THROTTLE, PID_INTAKE_MAP, PID_MAF_FLOW, PID_COOLANT_TEMP, PID_RPM, PID_SPEED};
    String data_aux = OBD_COLLECTION;
    
    for(byte i = 0; i<sizeof(pids); i++){
      String delemiter = "|";
      if (i == 0){
        delemiter = "";
        if (data_aux != ""){
          data_aux = data_aux + "#";
        } 
      }
      byte pid = pids[i];
      int value;
      if (obd.readPID(pid, value)) {
          data_aux = data_aux + delemiter + String(value);
      }else{
          data_aux = data_aux + delemiter;
      }
      
      if (obd.errors >= 2) {
          DEBUG("Reconnect");
          //setup();
      }
    }

    OBD_COLLECTION = data_aux;

    DEBUGB("Coleta OBD ");
    DEBUG(OBD_COLLECTION);
    
    yield();
  }
}

boolean conectarGprs() { 
  if(modemGSM.isNetworkConnected() && modemGSM.isGprsConnected()){
    DEBUGB("GPRS ja conectado com IP "); DEBUG(modemGSM.getLocalIP());
    return true;
  }

  DEBUGB("Realizando o reset do modem GPRS ");
  if (!modemGSM.restart())
  {
      DEBUG("falha na operacao");
      delay(1000);
      return false;
  } 
  DEBUG("restado");

  DEBUGB("GPRS modem v: "); DEBUG(modemGSM.getModemInfo());

  DEBUGB("Conectando com a rede da operadora ");
  if (!modemGSM.waitForNetwork()) {
      DEBUG("falha na operacao");
    return false;
  }
  DEBUG("conectado");

  DEBUGB("Conectando com APN ");
  if (!modemGSM.gprsConnect(GPRS_APN, GPRS_USER, GPRS_PASS)) {
      DEBUG("falha na operacao");
    delay(1000);
    return false;
  }
  DEBUG("conectado");

  DEBUGB("GPRS ja conectado com IP"); DEBUG(modemGSM.getLocalIP());
  return true;
}

void sendRequest()
{
    
  DEBUG("Configuracao inicial MQTT");
  mqtt.setBufferSize(MQTT_PACKET_SIZE);
  delay(1000);

  if (!mqtt.connected()) {
    DEBUG("Conectando com o MQTT ");
    // Attempt to connect
    if (mqtt.connect(THING_NAME)) {
      DEBUG("conectado"); 
      //String data_temp = OBD_COLLECTION;
      //data_temp = base64::encode(data_temp);
         
      String payload = "{ \"data\": \"" + OBD_COLLECTION + "\" }";
      send = true;
      DEBUG("Envia mensagem para broker MQTT");
      DEBUG(payload);
      mqtt.publish(topic, payload.c_str());
      delay(1000);
      DEBUG(OBD_COLLECTION);      
    } else {
      DEBUG("falha na operacao");
      DEBUGB("failed, rc=");
      DEBUGB(mqtt.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  } 
  mqtt.disconnect();
}
