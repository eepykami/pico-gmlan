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
            printf("%s\n", string_buffer);
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

    printf("GMLAN CAN test.\n");

    // Init MCP2515
    printf("Initialising MCP2515 ... ");

    int initStatus = can0.reset(); // Initialising SPI
    if(initStatus == MCP2515::ERROR_OK) {
        printf("success!\n");
    } else {
        printf("fail! Error %d.\n", initStatus);
        while(1) {
            tight_loop_contents();
        }
    }

    can0.setBitrate(CAN_33KBPS, MCP_16MHZ); // GMLAN low speed uses 33,3kbps baudrate, we set this here.
    can0.setNormalMode();

    // Listen loop
    printf("Listening for GMLAN messages ...\n");
    while(true) {
        processInboundSerialMessages();
        if(can0.readMessage(&rx) == MCP2515::ERROR_OK) {
            printf("New frame from ID: %10x\n", rx.can_id);
        }
    }
    
    return 0;
}
