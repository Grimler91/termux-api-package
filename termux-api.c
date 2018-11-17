// termux-api.c - helper binary for calling termux api classes
// Usage: termux-api ${API_METHOD} ${ADDITIONAL_FLAGS}
#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

// Function which execs "am broadcast ..".
_Noreturn void exec_am_broadcast()
{
    // Redirect stdout to /dev/null (but leave stderr open):
    close(STDOUT_FILENO);
    open("/dev/null", O_RDONLY);
    // Close stdin:
    close(STDIN_FILENO);

    char** child_argv = malloc((sizeof(char*)));

    child_argv[0] = "am";
    child_argv[1] = "broadcast";
    child_argv[2] = "--user";
    child_argv[3] = "0";
    child_argv[4] = "-n";
    child_argv[5] = "com.termux.api/.TermuxApiReceiver";


    // Use an a executable taking care of PATH and LD_LIBRARY_PATH:
    execv("/data/data/com.termux/files/usr/bin/am", child_argv);

    perror("execv(\"/data/data/com.termux/files/usr/bin/am\")");
    exit(1);
}

// Thread function which reads from stdin and writes to socket.
void* transmit_stdin_to_socket(void* arg) {
    int output_server_socket = *((int*) arg);
    struct sockaddr_un remote_addr;
    socklen_t addrlen = sizeof(remote_addr);
    //int output_client_socket = accept(output_server_socket, (struct sockaddr*) &remote_addr, &addrlen);

    ssize_t len;
    char buffer[1024];
    while (len = read(STDIN_FILENO, &buffer, sizeof(buffer)), len > 0) {
        if (write(output_server_socket, buffer, len) < 0) break;
    }
    // Close output socket on end of input:
    close(output_server_socket);
    return NULL;
}

// Main thread function which reads from input socket and writes to stdout.
void transmit_socket_to_stdout(int input_socket_fd) {
    ssize_t len;
    char buffer[1024];
    while ((len = read(input_socket_fd, &buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, len);
    }
    if (len < 0) perror("read()");
}

int main(int argc, char** argv) {
    // Do not transform children into zombies when they terminate:
    struct sigaction sigchld_action = { .sa_handler = SIG_DFL, .sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT };
    sigaction(SIGCHLD, &sigchld_action, NULL);

    char input_address_string[] = "termux-output";  // This program reads from it.
    char output_address_string[] = "termux-input"; // This program writes to it.

    struct sockaddr_un input_address = { .sun_family = AF_UNIX };
    struct sockaddr_un output_address = { .sun_family = AF_UNIX };
    // Leave struct sockaddr_un.sun_path[0] as 0 and use the UUID string as abstract linux namespace:
    strncpy(&input_address.sun_path[1], input_address_string, strlen(input_address_string));
    strncpy(&output_address.sun_path[1], output_address_string, strlen(output_address_string));

    int input_server_socket = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
    if (input_server_socket == -1) { perror("socket()"); return 1; }
    int output_server_socket = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
    if (output_server_socket == -1) { perror("socket()"); return 1; }

    if (bind(input_server_socket, (struct sockaddr*) &input_address, sizeof(sa_family_t) + strlen(input_address_string) + 1) == -1) {
        perror("bind(input)");
        return 1;
    }
    if (connect(output_server_socket, (struct sockaddr*) &output_address, sizeof(sa_family_t) + strlen(output_address_string) + 1) == -1) {
        perror("connect(output)");
       return 1;
    }

    if (listen(input_server_socket, 1) == -1) { perror("listen()"); return 1; }

    pthread_t transmit_thread;
    pthread_create(&transmit_thread, NULL, transmit_stdin_to_socket, &output_server_socket);


//    if (listen(output_server_socket, 1) == -1) { perror("listen()"); return 1; }

//    pid_t fork_result = fork();
//    switch (fork_result) {
//        case -1: perror("fork()"); return 1;
//        case 0: exec_am_broadcast(argc, argv);
//    }

    struct sockaddr_un remote_addr;
    socklen_t addrlen = sizeof(remote_addr);
    int input_client_socket = accept(input_server_socket, (struct sockaddr*) &remote_addr, &addrlen);

    transmit_socket_to_stdout(input_client_socket);

    return 0;
}

