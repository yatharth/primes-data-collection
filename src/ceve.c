// Send as FLOODER

#include <unistd.h>

#include "common.h"
#include "common.c"

int main() {
    printf("sleeping... ");
    fflush(stdout);
    sleep(START_DELAY);
    printf("done\n");

    send_agnostic(EVE_CONFIG);
    return 0;
}
