// Define core functionality and hold configurtion constants

#ifndef PRIMES_COMMON
#define PRIMES_COMMON

#include <stdbool.h>

struct config {
    // common
    char *hostname;
    char *serverport;
    int use_tcp;
    int tcp_no_delay;

    // server / receive
    long buf_len;
    char *outfile;
    int packet_squash_threshold;

    // client / send
    bool dyn;
    char *outfile_send;
    long iterations;
    long packet_size;
    int randomize;
    char *base_string;
};

#define BUFLEN 1000000
#define USE_TCP 0
#define TCP_NO_DELAY 0
#define BASE_STRING "123456789"
#define RANDOMIZE 0
#define ITERATIONS XXX
#define PACKET_SQUASH_THRESHOLD 5

#define PACKET_SIZE XXX
#define DUMP_DIR XXX
#define DUMP_IDENTIFIER XXX
#define ALICEHOST XXX
#define EVEHOST XXX

const unsigned int START_DELAY = 0;

const struct config ALICE_CONFIG = {
        .hostname = ALICEHOST,
        .serverport = "4999",
        .use_tcp = USE_TCP,
        .tcp_no_delay = TCP_NO_DELAY,

        .buf_len = BUFLEN,
        .outfile = DUMP_DIR DUMP_IDENTIFIER ".alice",
        .packet_squash_threshold = PACKET_SQUASH_THRESHOLD,

        .dyn = false,
        .outfile_send = DUMP_DIR DUMP_IDENTIFIER ".calice",
        .iterations = -1,
        .base_string = BASE_STRING,
        .randomize = RANDOMIZE,
        .packet_size = PACKET_SIZE,
};

const struct config EVE_CONFIG = {
        .hostname = EVEHOST,
        .serverport = "4998",
        .use_tcp = USE_TCP,
        .tcp_no_delay = TCP_NO_DELAY,

        .buf_len = BUFLEN,
        .outfile = DUMP_DIR DUMP_IDENTIFIER ".eve",
        .packet_squash_threshold = PACKET_SQUASH_THRESHOLD,

        .dyn = false,
        .outfile_send = DUMP_DIR DUMP_IDENTIFIER ".ceve",
        .iterations = -1,
        .base_string = BASE_STRING,
        .randomize = RANDOMIZE,
        .packet_size = PACKET_SIZE,
};

struct config vish_config = {
        .buf_len = BUFLEN,
        .dyn = false,
        .iterations = -1,
        .base_string = BASE_STRING,
        .randomize = false,
};


void send_agnostic(struct config config);

void receive_agnostic(struct config config);

#endif
