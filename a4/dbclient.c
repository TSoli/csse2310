/* FILE: dbclient.h
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * A simple client that can add/remove/edit key:value pairs from a server.
 */

#include "dbclient.h"

/* Entry point to dbclient*/
int main(int argc, char* argv[]) {
    // Do client things
    check_args(argc, argv);

    int sock = connect_to_server(argv[PORT_POS]);
    int sock2 = dup(sock);
    FILE* to = fdopen(sock, "w");
    FILE* from = fdopen(sock2, "r");
    char* key = argv[KEY_POS];
    
    if (argc > VAL_POS) {
        char* val = argv[VAL_POS];
        send_put(to, from, key, val);
    } else {
        send_get(to, from, key);
    }

    fclose(to);
    fclose(from);
    
    return 0;
}

void check_args(int argc, char* argv[]) {
    int tooManyArgs = check_num_args(argc, MIN_ARGS, MAX_ARGS);
    if (tooManyArgs < 0) {
        fprintf(stderr, USAGE_MSG);
        exit(USAGE_EXIT_CODE);
    }
    
    for (int i = 0; i < strlen(argv[KEY_POS]); i++) {
        if (isspace(argv[KEY_POS][i])) {
            fprintf(stderr, INVALID_KEY_MSG);
            exit(INVALID_KEY_EXIT_CODE);
        }
    }
}

int connect_to_server(char* port) {
    struct addrinfo* ai = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err;
    if ((err = getaddrinfo(SERVER_IP, port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, CANT_CONNECT, port); 
        exit(CONNECTION_EXIT_CODE);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
        fprintf(stderr, CANT_CONNECT, port);
        exit(CONNECTION_EXIT_CODE);
    }

    return fd;
}

void send_put(FILE* to, FILE* from, char* key, char* val) {
    int contentLen = strlen(val); // Each char is a byte
    fprintf(to, "PUT /public/%s HTTP/1.1\r\n", key);
    fprintf(to, "Content-Length: %d\r\n\r\n", contentLen);
    fprintf(to, "%s", val);
    fflush(to);
    // Check for response
    char* body;
    if (!get_response(from, &body)) {
        fclose(to);
        fclose(from);
        exit(CANT_PUT_EXIT_CODE);
    }
}

void send_get(FILE* to, FILE* from, char* key) {
    fprintf(to, "GET /public/%s HTTP/1.1\r\n\r\n", key);
    fflush(to);

    char* body;
    if (!get_response(from, &body)) {
        fclose(to);
        fclose(from);
        exit(CANT_GET_EXIT_CODE);
    }

    printf("%s\n", body);
    fflush(stdout);
}

bool get_response(FILE* from, char** body) {
    int status;
    char* statusExplain; 
    HttpHeader** headers;
    
    if (!get_HTTP_response(from, &status, &statusExplain, &headers, body)) {
        return false;
    }
    if (status != STATUS_OK) {
        return false;
    }

    return true;
}
