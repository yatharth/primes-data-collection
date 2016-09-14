// Provide command line interface as alternative to putting configuration in common.h

#include "common.h"
#include "common.c"

int main(int argc, char *argv[]) {

    if (argc != 8) {
        printf("incorrect number of arguments");
        exit(3);
    }

    char *type = argv[1];
    vish_config.hostname = argv[2];
    vish_config.serverport = argv[3];

    char *protocol = argv[4];
    if (strcmp(protocol, "udp") == 0) {
        vish_config.use_tcp = false;
    } else if (strcmp(protocol, "tcp") == 0) {
        vish_config.use_tcp = true;
        vish_config.tcp_no_delay = false;
    } else if (strcmp(protocol, "tcpnodelay") == 0) {
        vish_config.use_tcp = true;
        vish_config.tcp_no_delay = true;
    } else {
        printf("invalid protocol argument");
        exit(3);
    }

    vish_config.packet_size = atoi(argv[5]);
    vish_config.packet_squash_threshold = atoi(argv[6]);

    char* outfile = argv[7];

    if (strcmp(argv[1], "send") == 0) {
        vish_config.outfile_send = outfile;
        send_agnostic(vish_config);

    } else if (strcmp(argv[1], "receive") == 0) {
        vish_config.outfile = outfile;
        receive_agnostic(vish_config);

    } else {
        printf("first argument must be 'send' or 'receive'");
        exit(3);
    }

    return 0;
}
