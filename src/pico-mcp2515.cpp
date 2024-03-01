#include <stdio.h>
#include <string.h>
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

// CAN message to wake up GMLAN before transmitting data
can_frame gmlanInit = {
    .can_id = 0x632,
    .can_dlc = 8, // Data length (bytes)
    .data = { 0x00, 0x48, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

// CAN message to be sent (this should do a needle sweep on the Corsa-D IPC)
// TODO: we'll want this to be updated from serial
can_frame needleSweep = {
    .can_id = 0x170,
    .can_dlc = 8, // Data length (bytes)
    .data = { 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x1E }
};

// Prototypes
void setupCAN();
void processInboundSerialMessage();
void processInboundCanMessage();
void sendCanFrame(can_frame frame);

void setupCAN() {  // Init MCP2515
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
    can0.setNormalMode();
    //can0.setLoopbackMode();
}

void processInboundSerialMessage() {
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
            if(strncmp(string_buffer, "init", 4) == 0) {
                printf("Sending GMLAN initialisation packet ... ");
                sendCanFrame(gmlanInit);
            } else if(strncmp(string_buffer, "sweep", 5) == 0) {
                printf("Sending needle sweep packet ... ");
                sendCanFrame(needleSweep);
            }
            string_buffer_offset = 0;
            break;
        }
        input_character = getchar_timeout_us(0);
    }
}

void processInboundCanMessage() {
    if(can0.readMessage(&rx) == MCP2515::ERROR_OK) { // Checking for incoming CAN message
        printf("Received new frame from ID: %10x\n", rx.can_id);
        printf("Data: "); // Printing received data in hex
        for(int i = 0; i < rx.can_dlc; i++) {
            if(! (i % 16) && i) {
                printf("\n");
            }
            printf("0x%02X ", rx.data[i]);
        }
        printf("\n");
    }
}

void sendCanFrame(can_frame frame) {
    MCP2515::ERROR sendStatus = can0.sendMessage(&frame);
    if(sendStatus == MCP2515::ERROR_OK){
        printf("success!\n");
    } else {
        printf("fail! Error %d.\n", sendStatus);
    }
}

int main() {
    stdio_init_all();

    // Wait for serial connection before continuing - useful for Windows, could live without this on Linux.
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    setupCAN();

    while(true) {
        processInboundSerialMessage(); // Checking for incoming serial message
        processInboundCanMessage();
    }
    return 0;
}
