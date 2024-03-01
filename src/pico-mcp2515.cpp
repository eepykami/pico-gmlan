#include <stdio.h>
#include "pico/stdlib.h"
#include "mcp2515/mcp2515.h"

// Buffer size and values for serial input
#define ENDSTDIN 255
#define CR 13
#define SERIAL_DATA_BUFFER_SIZE 500

// Variables for serial input
int string_buffer_offset = 0; 
char string_buffer[SERIAL_DATA_BUFFER_SIZE]; 
char input_character;

// Variables for MCP2515
MCP2515 can0;
struct can_frame rx;

// CAN message to be sent
// TODO: we'll want this to be updated from serial
can_frame tx = {
    .can_id = 0x500,
    .can_dlc = 8, // Data length (bytes)
    .data = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

void setupCAN() {
    // Init MCP2515
    printf("Initialising MCP2515 ... ");

    MCP2515::ERROR initStatus = can0.reset(); // Initialising SPI
    if(initStatus == MCP2515::ERROR_OK) {
        printf("success!\n");
    } else {
        printf("fail! Error %d.\n", initStatus);
        while(1) {
            // TODO: Blink LED on error?
            tight_loop_contents();
        }
    }

    can0.setBitrate(CAN_33KBPS, MCP_16MHZ); // GMLAN low speed uses 33,3kbps baudrate, we set this here.
    //can0.setNormalMode();
    can0.setLoopbackMode();
}

void processInboundSerialMessages() {
    input_character = getchar_timeout_us(0);
    while (input_character != ENDSTDIN)
    {   
        string_buffer[string_buffer_offset++] = input_character;
        if (input_character == CR)
        {
            string_buffer[string_buffer_offset] = 0;
            // do something with message in string buffer here. for now we printf it
            // TODO: plan is for a can packet to come in via serial. decode it from the string buffer, and send it via CAN.
            printf("New string from serial: %s\n", string_buffer);
            string_buffer_offset = 0;
            break;
        }
        input_character = getchar_timeout_us(0);
    }
}

int main() {
    stdio_init_all();

    // Wait for serial connection before continuing - useful for Windows, could live without this on Linux.
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    setupCAN();

    printf("GMLAN CAN test.\n");

    unsigned long previousMainLoopTime = to_ms_since_boot(get_absolute_time());

    // Listen loop
    while(true) {
        unsigned long currentMainLoopTime = to_ms_since_boot(get_absolute_time());

        processInboundSerialMessages(); // Checking for incoming serial message

        if(can0.readMessage(&rx) == MCP2515::ERROR_OK) { // Checking for incoming CAN message
            printf("New frame from ID: %10x\n", rx.can_id);
        }

        if(currentMainLoopTime - previousMainLoopTime > 5000) { // send CAN message every 1 second
            printf("Sending CAN message ... ");

            MCP2515::ERROR sendStatus = can0.sendMessage(MCP2515::TXB0, &tx);
            if(sendStatus == MCP2515::ERROR_OK){
                printf("success!\n");
            } else {
                printf("fail! Error %d.\n", sendStatus);
            }

            previousMainLoopTime = currentMainLoopTime;
        }
    }
    
    return 0;
}
