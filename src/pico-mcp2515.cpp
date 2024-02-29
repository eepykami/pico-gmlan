#include <stdio.h>
#include "pico/stdlib.h"
#include "mcp2515/mcp2515.h"

MCP2515 can0;
struct can_frame rx;

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

    can0.setBitrate(CAN_1000KBPS, MCP_16MHZ);
    can0.setNormalMode();

    //Listen loop
    while(true) {
        if(can0.readMessage(&rx) == MCP2515::ERROR_OK) {
            printf("New frame from ID: %10x\n", rx.can_id);
        }
    }

    return 0;
}
