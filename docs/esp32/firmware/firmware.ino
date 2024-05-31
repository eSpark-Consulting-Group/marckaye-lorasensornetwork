// // Define the serial port for communication
#define SERIAL_PORT_RX 44 // GPIO16 is used as RX
#define SERIAL_PORT_TX 43 // GPIO17 is used as TX

// void setup() {
//   // Start the Serial Monitor
//   Serial.begin(115200); // Serial monitor baud rate
//   while (!Serial) {
//     // Wait for the serial port to connect
//   }

//   // Start the hardware serial communication
//   Serial1.begin(9600, SERIAL_8N1, SERIAL_PORT_RX, SERIAL_PORT_TX); // Serial1 with baud rate 9600

//   // Send the command
//   uint8_t command[] = {0x03, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x71, 0xEF};//0xFF, 0xFF, 0xFF, 
//   Serial1.write(command, sizeof(command));
//   Serial.println("Command sent.");
//   // Serial.println(command);
// }

// void loop() {
//   // Check if there's any data available on Serial1
//   if (Serial1.available()) {
//     // Read the data from Serial1
//     // int incomingByte = Serial1.read();

//     // Print the incoming data to the Serial Monitor
//     Serial.print("Received: ");
//     Serial.println(Serial1.read(), DEC);
//   }
// }

#define SERIAL_PORT_RX 16 // GPIO16 is used as RX
#define SERIAL_PORT_TX 17 // GPIO17 is used as TX

bool parseDataPacket(const char *packet, uint8_t *values, size_t maxSize) {
  // Check if the packet starts with "Buf:"
  if (strncmp(packet, "Buf:", 4) != 0) {
    return false; // Invalid packet format
  }

  // Move the pointer to the start of the data
  packet += 4;

  // Parse the data and store values in the array
  size_t index = 0;
  char *endPtr;
  char *token = strtok_r((char*)packet, ",", &endPtr);
  while (token != nullptr && index < maxSize) {
    values[index++] = (uint8_t)atoi(token);
    token = strtok_r(nullptr, ",", &endPtr);
  }

  return true; // Successfully parsed the packet
}

 void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, SERIAL_PORT_RX, SERIAL_PORT_TX); // Serial1 with baud rate 9600
  Serial.print("Received: ");
}
void loop()
{
  if (Serial1.available()) 
  {
    // Read the data from Serial1
    // int incomingByte = Serial1.read();

    // Print the incoming data to the Serial Monitor
    String ch=Serial1.readStringUntil('\n');
    Serial.println(ch);
    const size_t arraySize = 47;  // Adjust the size as needed
    uint8_t values[arraySize] = {0};

    // Parse the data packet
    if (parseDataPacket(ch.c_str(), values, arraySize))
    {
      // Print the parsed values for debugging
      Serial.println("Parsed values:");
      for (size_t i = 0; i < arraySize; i++) {
        Serial.print(values[i]);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("Invalid packet format.");
    }
  }
  
  // LOG.println("HEllo World! CH");
  // delay(1000);
}