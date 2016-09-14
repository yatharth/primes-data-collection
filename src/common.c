// Implement core functionality

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common.h"

volatile sig_atomic_t keep_going = 1;


//region Helpers

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int get_socket(const struct config *config, int server) {

    // Declare variables
    int sockfd;
    struct addrinfo ai_hint, *ais, *ai;

    // Initialize variables
    memset(&ai_hint, 0, sizeof ai_hint);
    ai_hint.ai_family = AF_INET; // IPv4
    ai_hint.ai_socktype = config->use_tcp ? SOCK_STREAM : SOCK_DGRAM;
    if (server) {
        ai_hint.ai_flags = AI_PASSIVE;
    }

    // Get addresses
    int status = getaddrinfo(config->hostname, config->serverport, &ai_hint, &ais);
    if (status != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // Get sockets until one connects
    for (ai = ais; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1) {
            perror("error: socket");
            continue;
        }

        // Conditionally set TCP server socket options
        if (server) {
            if (config->use_tcp) {
                const int YES = 1;
                int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &YES, sizeof(int));
                if (status == -1) {
                    perror("setsockopt");
                    exit(1);
                }
            }
        }

        // Conditionally disable TCP buffering using Nagle’s algorithm
        if (!server && config->use_tcp && config->tcp_no_delay) {
            setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (int[]) {1}, sizeof(int));
        }

        // If server, bind; if client, connect to socket
        if (server) {
            if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) {
                close(sockfd);
                fprintf(stderr, "errno = %d ", errno);
                perror("bind");
                continue;
            }
        } else {
            if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) == -1) {
                perror("connect");
                close(sockfd);
                continue;
            }
        }

        break;
    }

    // If no address worked, error
    if (ai == NULL) {
        fprintf(stderr, "error: socket: none worked\n");
        exit(1);
    }

    // If client, print the server IP
    if (!server) {
        char receiver_ip[INET6_ADDRSTRLEN];
        inet_ntop(ai->ai_family, get_in_addr(ai->ai_addr), receiver_ip, sizeof receiver_ip);
//        if (!receiver_ip) {
//            perror("error: inet_ntop");
//        } else {
        printf("connecting to %s\n", receiver_ip);
//        }
    }

    // Release resources and return
    freeaddrinfo(ais);
    return sockfd;
}

struct config;

FILE *pFile;

void sighandler(int signo) {
    if (signo == SIGINT) {
        fflush(pFile);
        printf("interrupted\n");
        fflush(stdout);
        exit(2);
    }
}

int generate_message(const struct config *config, char *message) {
    long packet_size = config->packet_size;
    char *pointer = message;

    // Handle empty string
    if (!config->base_string) {
        return 1;
    }

    // Compute lengths
    int part_len = 0;
    while (config->base_string[part_len] != '\0') {
        ++part_len;
    }

    // Start string
    *(message + 0) = '<';
    ++pointer;
    --packet_size;

    // Loop `reps` times
    long i, reps = (packet_size - 1) / (part_len + 1);
    for (i = 0; i < reps; ++i) {
        const char *part_pointer = config->base_string;
        *pointer++ = (char) (65 + (i % 26));
        while (*part_pointer)
            *pointer++ = *part_pointer++;
    }

    // Pad string until end
    long j, padding_len = (packet_size - 1) % (part_len + 1);
    for (j = 0; j < padding_len; ++j) {
        *pointer++ = (char) (97 + (j % 26));
    }

    // End string
    *(message + packet_size) = '>';
    *(message + packet_size + 1) = 0;

    return 0;
}

void warp_message(const struct config *config, char *message) {
    static const long STEP = 50;
    static const char alphanum[] = "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    long i;
    for (i = 1; i < config->packet_size; i += STEP) {
        *(message + i) = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}

typedef struct timeval timeval;

void print_header(FILE *pFile, struct config *config) {
    timeval time_;
    gettimeofday(&time_, NULL);
    fprintf(pFile, "%ld.%06ld,%ld\n",
            time_.tv_sec, time_.tv_usec,
            config->packet_size);
}

void print_packet(FILE *pFile, long long counter, ssize_t numbytes, long total_numbytes, char buf[]) {
    timeval time_;
    gettimeofday(&time_, NULL);

    fprintf(pFile, "%lld,%ld.%06ld,%zu,%ld\n", counter, time_.tv_sec, time_.tv_usec, numbytes, total_numbytes);
//    printf("%lld:%zu:%s\n", counter, numbytes, buf);
//    printf("%lld,\n", counter, numbytes);
//    fwrite(MESSAGE, sizeof(MESSAGE[0]), sizeof(MESSAGE) / sizeof(MESSAGE[0]), pFile);
}


//endregion


//region Sender

void random_message(const struct config *config, char *message) {
    static const char alphanum[] = "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    int i;
    for (i = 0; i < config->packet_size; ++i) {
        message[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    message[config->packet_size] = 0;
}

void send_agnostic(struct config config) {

    // Declare variables
    ssize_t numbytes;
    long long counter;
    ssize_t message_len = config.packet_size + 1;

    // Acquire resources
    int sockfd = get_socket(&config, 0);
    char message[message_len];

    if (signal(SIGINT, sighandler) == SIG_ERR) {
        printf("\ncouldn't catch SIGINT\n");
        exit(1);
    };
    printf("opening file %s... ", config.outfile_send);
    fflush(stdout);
    pFile = fopen(config.outfile_send, "wb");
    printf("done\n");

    // Prep
    print_header(pFile, &config);
    random_message(&config, message);

    // Hard code constants
    const double sleep_val_max = 3000; // in usec
    const int sleep_steps = 30;
    const int sleep_steps_extra = 5;
    const double sleep_mul_step = config.iterations / (sleep_steps + sleep_steps_extra);
    const double sleep_val_step = 50; // sleep_val_max / sleep_steps;
    float sleep_val = sleep_val_max;
    long sleep_mul = 0;
    int sleep_count = 0;

    // Loop `iterations` times
    for (counter = 1; counter <= config.iterations || config.iterations < 0; counter += 1) {

        // Send, catching errors
        numbytes = send(sockfd, message, (size_t) config.packet_size, 0);
        if (numbytes == -1) {
            perror("error: send");
            exit(1);
        }
        print_packet(pFile, counter, numbytes, numbytes, message);

        // Yuck @ hard-coding
        if (config.dyn) {
            if (counter - sleep_mul > sleep_mul_step) {
                timeval time_;
                gettimeofday(&time_, NULL);

                sleep_mul += sleep_mul_step;
                sleep_count += 1;
                printf("# stepped at %ld.%06ld... ", time_.tv_sec, time_.tv_usec);

                if (sleep_count <= sleep_steps) {
                    sleep_val -= sleep_val_step;
                    printf("to %f", sleep_val);
                } else {
                    printf("skipped");
                }
                printf("\n");
            }
            usleep(sleep_val);

        } else {
        }

        if (config.randomize) {
            random_message(&config, message);
        }
    }

    printf("exiting");
    fflush(stdout);

    // Free resources
    close(sockfd);
}

//endregion


//region Receiver


void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void receive_tcp(const struct config *config, FILE *pFile) {

    // Get and listen on socket
    int sockfd = get_socket(config, 1);
    const int BACKLOG = 10;
    int status = listen(sockfd, BACKLOG);
    if (status == -1) {
        perror("listen");
        exit(1);
    }
    printf("listening for connections\n");

    // Ensure any zombie processes will be reaped
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Loop against connections
    const int TOTAL_CONNECTIONS = 1;
    int connection;
    for (connection = 0; connection < TOTAL_CONNECTIONS || TOTAL_CONNECTIONS < 0; ++connection) {

        // Accept connections, handling errors
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // Print client IP
        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), client_ip, sizeof client_ip);
        printf("receiver_tcp: got connection from %s\n", client_ip);


        // Fork to handle connection

        // Child
        pid_t child;
        if (!(child = fork())) {
            close(sockfd);
            long long counter = 0;
            long total_numbytes = 0;

            // Keep reading until socket closed, handling errors
            char buf[config->buf_len];
            ssize_t numbytes;
            while (1) {
                numbytes = recv(new_fd, buf, (size_t) (config->buf_len - 1), 0);
                if (numbytes == 0) {
                    printf("socket closed by client\n");
                    break;
                }
                else if (numbytes == -1) {
                    perror("recv");
                    close(new_fd);
                    exit(1);
                }

                // Print packet
                counter += 1;
                buf[numbytes] = '\0';
                total_numbytes += numbytes;

                if (counter % config->packet_squash_threshold == 0) {
                    print_packet(pFile, counter, numbytes, total_numbytes, buf);
                    total_numbytes = 0;
                }
            }
            print_packet(pFile, counter, numbytes, total_numbytes, buf);

            // Release resources
            close(new_fd);
            exit(0);
        }

        else {
            // Parent
            wait(&child);
            close(new_fd);

        }
    }
    close(sockfd);
}

void receive_udp(const struct config *config, FILE *pFile) {

    // Declare variables
    char buf[config->buf_len];
    ssize_t numbytes;
    struct sockaddr_storage their_addr;
    socklen_t ADDR_LEN = sizeof their_addr;
    long long counter = 0;
    long total_numbytes = 0;

    // Get socket and keep listening
    int sockfd = get_socket(config, 1);
    printf("listening for packets\n");
    while (1) {

        // Receive, handling errors
        numbytes = recvfrom(sockfd, buf, (size_t) (config->buf_len - 1), 0, (struct sockaddr *) &their_addr, &ADDR_LEN);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }

        // Print packet
        counter += 1;
        buf[numbytes] = '\0';
        total_numbytes += numbytes;
        if (counter % config->packet_squash_threshold == 0) {
            print_packet(pFile, counter, numbytes, total_numbytes, buf);
            total_numbytes = 0;
        }
    }

    // Release resources
    close(sockfd);
}

void receive_agnostic(struct config config) {

    // Acquire resources
    if (signal(SIGINT, sighandler) == SIG_ERR) {
        printf("\ncouldn't catch SIGINT\n");
        exit(1);
    };
    printf("opening file %s... ", config.outfile);
    fflush(stdout);
    pFile = fopen(config.outfile, "wb");
    printf("done\n");

    // Print header
    print_header(pFile, &config);
    fflush(pFile);

    // Delegate receiving
    if (config.use_tcp) {
        receive_tcp(&config, pFile);
    } else {
        receive_udp(&config, pFile);
    }

    // Release resources
    fflush(pFile);
    fclose(pFile);
}


//endregion
