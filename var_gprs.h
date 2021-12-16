
//GPRS
#define TINY_GSM_MODEM_SIM800 //Varivel obrigatoria para o SIM800L
#define TINY_GSM_RX_BUFFER   1024
#define SerialGSM           Serial1
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

const char GPRS_APN[]  = "claro.com.br";
const char GPRS_USER[] = "claro";
const char GPRS_PASS[] = "claro";
