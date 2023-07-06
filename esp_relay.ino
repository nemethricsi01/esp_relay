#include <WiFi.h>
#include <WiFiClient.h>
#include <HardwareSerial.h>

#define UART_RX_PIN 16
#define UART_TX_PIN 15
#define BAUD_RATE 115200

HardwareSerial SerialUART(1);  // UART object
const char* ssid = "nemeth_wifi";
const char* password = "75000000";
const char* serverIP = "192.168.1.190"; // Replace with your server IP address
const int serverPort = 5000; // Replace with your server port

const char* server2IP = "192.168.1.190"; // Replace with your server IP address
const int server2Port = 7; // Replace with your server port


IPAddress staticIP(192, 168, 1, 100);    // Static IP address
IPAddress gateway(192, 168, 1, 1);        // Gateway IP address
IPAddress subnet(255, 255, 255, 0);       // Subnet mask

WiFiClient client;
WiFiClient client2;
WiFiServer server(80);

WiFiClient connected_client;

uint8_t syncCounter = 0;
uint8_t destId = 0;
uint16_t dataSize0;
uint16_t dataSize0Save;
uint16_t dataSize1;
uint16_t dataSize1Save;
uint16_t dataSize2; 
uint16_t dataSize2Save;
uint8_t rxBuffer0[1200];
uint8_t rxBuffer1[1200];
uint8_t rxBuffer2[1200];


uint8_t txBuffer0[1200];
uint8_t txBuffer1[1200];
uint8_t txBuffer2[1200];

uint8_t txBuff0Full = 0;
uint8_t txBuff1Full = 0;
uint8_t txBuff2Full = 0;


TaskHandle_t Task1;
void setup() {

  SerialUART.begin(BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  SerialUART.setRxBufferSize(512);
  delay(1000);
  WiFi.config(staticIP, gateway, subnet); // Set static IP configuration
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    SerialUART.println("Connecting to WiFi...");
  }
  SerialUART.println("Connected to WiFi");
  SerialUART.println(WiFi.localIP());
  client.connect(serverIP, serverPort);
  client2.connect(server2IP, server2Port);

  
  server.begin();
  delay(1000);
  connected_client= server.available();   // Listen for incoming clients
  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    (void*)&txBuff0Full,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}



void Task1code( void * pvParameters ){

  for(;;)
  {
    
    //uint8_t *temp =  (uint8_t*)pvParameters;
    if( txBuff0Full )
    {
      client.write(txBuffer0,1000);
      txBuff0Full = 0;
    }
    if( txBuff1Full )
    {
      client2.write(txBuffer1,1000);
      txBuff1Full = 0;
    }
    if( txBuff2Full )
    {
      if (connected_client) 
      {                             // If a new client connects,
        connected_client.write(txBuffer2,1000);
      }
      txBuff2Full = 0;
    }
    vTaskDelay(1);
  } 
}


void loop() {
  if(SerialUART.available())
    {
      uint8_t readTemp = SerialUART.read();
      if( ( syncCounter < 5 ) && ( readTemp != 0x55 ) )//clear the sync counter
      {
        syncCounter = 0;
      }
      if ( ( readTemp == 0x55 ) && ( syncCounter < 5 ) ) 
      {
      syncCounter++;
      
      }
      else if(syncCounter == 5)
      {
      destId = readTemp;
      
      syncCounter++;
      }
      else if(syncCounter == 6)
      {
        if(destId == 1)
        {
          dataSize0 = readTemp<<8;
          syncCounter++;
          
        }
        if(destId == 2)
        {
          dataSize1 = readTemp<<8;
          syncCounter++;
          
        }
        if(destId == 3)
        {
          dataSize2 = readTemp<<8;
          syncCounter++;
          
        }
        //TODO add more ids
        
      }
      else if(syncCounter == 7)
      {
        if(destId == 1)
        {
          dataSize0 |= readTemp;
          dataSize0Save = dataSize0;
          
          syncCounter++;
        }
        if(destId == 2)
        {
          dataSize1 |= readTemp;
          dataSize1Save = dataSize1;
          
          syncCounter++;
        }
        if(destId == 3)
        {
          dataSize2 |= readTemp;
          dataSize2Save = dataSize2;
          
          syncCounter++;
        }
        
        //TODO add more ids
      }
      else if(syncCounter == 8)
      {
        if(destId == 1)
        {
          if(dataSize0 != 0)
          {
            rxBuffer0[dataSize0Save-dataSize0] = readTemp;
            dataSize0--;
          }
          else if(dataSize0 == 0)
          {
            if(readTemp == 0xAA)
            {
              memcpy(txBuffer0,rxBuffer0,sizeof(rxBuffer0));
              memset(rxBuffer0,0,sizeof(rxBuffer0));
              txBuff0Full = 1;
              syncCounter = 0;
              dataSize0Save = 0;
              
            }
          }
        }
        if(destId == 2)
        {
          if(dataSize1 != 0)
          {
            rxBuffer1[dataSize1Save-dataSize1] = readTemp;
            dataSize1--;
          }
          else if(dataSize1 == 0)
          {
            if(readTemp == 0xAA)
            {
              memcpy(txBuffer1,rxBuffer1,sizeof(rxBuffer1));
              memset(rxBuffer1,0,sizeof(rxBuffer1));
              txBuff1Full = 1;
              syncCounter = 0;
              dataSize1Save = 0;
            }
          }
        }
        if(destId == 3)
        {
          if(dataSize2 != 0)
          {
            rxBuffer2[dataSize2Save-dataSize2] = readTemp;
            dataSize2--;
          }
          else if(dataSize2 == 0)
          {
            if(readTemp == 0xAA)
            {
              memcpy(txBuffer2,rxBuffer2,sizeof(rxBuffer2));
              memset(rxBuffer2,0,sizeof(rxBuffer2));
              txBuff2Full = 1;
              syncCounter = 0;
              dataSize2Save = 0;
            }
          }
        }
      }
    }
  
}