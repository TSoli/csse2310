/* FILE: dbserver.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * A simple server that can add/remove/edit key:value pairs upon request and 
 * return them to a client when asked.
 */

#include "dbserver.h"

/* An array of the supported HTTP methods.*/
const char* methodNames[] = {
        "GET", "PUT", "DELETE"};

/* An array of function pointers to handle HTTP requests and respond
 * appropriately. */
const HandleHttpReq methodHandlers[NUM_METHODS] = {
        handle_get_req, handle_put_req, handle_delete_req};

struct Stats {
    int currConnected;
    int totalDisconnected;
    int authFails;
    int numGets;
    int numPuts;
    int numDeletes;
    pthread_mutex_t* statsLock;
};

struct ClientArgs {
    int fd;
    StringStore* publicDb;
    StringStore* privateDb;
    pthread_mutex_t* pubLock;
    pthread_mutex_t* privLock;
    const char* authstring;
    Stats* stats;
};

void print_stats(Stats* stats) {
    fprintf(stderr, "Connected clients:%d\n", stats->currConnected);
    fprintf(stderr, "Completed clients:%d\n", stats->totalDisconnected);
    fprintf(stderr, "Auth failures:%d\n", stats->authFails);
    fprintf(stderr, "GET operations:%d\n", stats->numGets);
    fprintf(stderr, "PUT operations:%d\n", stats->numPuts);
    fprintf(stderr, "DELETE operations:%d\n", stats->numDeletes);
}

void setup_sig_handling(Stats* stats) {
    sigset_t* set = malloc(sizeof(sigset_t));
    sigemptyset(set);
    sigaddset(set, SIGHUP);
    int s = pthread_sigmask(SIG_BLOCK, set, NULL);
    pthread_t threadId;

    if (s != 0) {
        errno = s;
        perror("pthread_sigmask\n");
        exit(EXIT_FAILURE);
    }
    
    s = pthread_create(&threadId, NULL, report_thread, (void*) stats);
    if (s != 0) {
        errno = s;
        perror("pthread_create\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(threadId);
}

void client_args_init(ClientArgs* clientArgs, int fd, const char* authstring,
        StringStore* publicDb, StringStore* privateDb,
        pthread_mutex_t* pubLock, pthread_mutex_t* privLock, Stats* stats) {
    clientArgs->fd = fd;
    clientArgs->publicDb = publicDb;
    clientArgs->privateDb = privateDb;
    clientArgs->pubLock = pubLock;
    clientArgs->privLock = privLock;
    clientArgs->authstring = authstring;
    clientArgs->stats = stats;
}

Stats* stats_init(void) {
    Stats* stats = malloc(sizeof(Stats));
    pthread_mutex_t* statsLock = malloc(sizeof(pthread_mutex_t));
    int err = pthread_mutex_init(statsLock, NULL);
    if (err != 0) {
        errno = err;
        perror("statsLock mutex init failed");
    }

    stats->currConnected = 0;
    stats->totalDisconnected = 0;
    stats->authFails = 0;
    stats->numGets = 0;
    stats->numPuts = 0;
    stats->numDeletes = 0;
    stats->statsLock = statsLock;
    return stats;
}

int main(int argc, char* argv[]) {
    check_args(argc, argv);
    const char* authstring = get_authstring(argv[AUTH_POS]);
    const int maxConnex = atoi(argv[NUM_CONNEX_POS]);

    char* port = DEFAULT_PORT;   // Use ephemeral port by default
    if (argc > PORT_POS) {
        port = argv[PORT_POS]; // Already checked validity
    }

    uint16_t portNum;
    int fdServer = open_listen(port, &portNum);
    if (fdServer < 0) {
        fprintf(stderr, PORT_MSG);
        return PORT_EXIT_CODE;
    }

    // Server opened
    fprintf(stderr, "%u\n", portNum);
    process_connections(fdServer, maxConnex, authstring);

    return 0;
}

void check_args(int argc, char* argv[]) {
    bool isValidCommandline = is_valid_commandline(argc, argv);
    if (!isValidCommandline) {
        fprintf(stderr, USAGE_MSG);
        exit(USAGE_EXIT_CODE);
    }
}

bool is_valid_commandline(int argc, char* argv[]) {
    int invalidNumArgs = check_num_args(argc, MIN_ARGS, MAX_ARGS);
    if (invalidNumArgs) {
        return false;
    }

    if (!is_int(argv[NUM_CONNEX_POS])) {
        return false;
    }
    
    long numConnex = strtol(argv[NUM_CONNEX_POS], NULL, 0);
    if (numConnex < 0 || errno == ERANGE) {
        return false;
    }

    if (argc > PORT_POS) {
        if (!is_int(argv[PORT_POS])) {
            return false;
        }

        long portNum = strtol(argv[PORT_POS], NULL, 0);
        if (check_num_in_range((int)portNum, MIN_PORT, MAX_PORT) ||
                errno == ERANGE) {
            return false;
        }
    }

    return true;
}

const char* get_authstring(char* authfile) {
    FILE* file = fopen(authfile, "r");
    if (!file) {
        fprintf(stderr, AUTH_MSG);
        exit(AUTH_EXIT_CODE);
    }

    const char* authstring = read_line(file);
    if (!authstring) {
        fprintf(stderr, AUTH_MSG);
        exit(AUTH_EXIT_CODE);
    }
    return authstring;
}

int open_listen(char* port, uint16_t* portNum) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        return -1;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            &optVal, sizeof(int)) < 0) {
        freeaddrinfo(ai);
        return -1;
    }

    if (bind(listenfd, (struct sockaddr*)ai->ai_addr,
            sizeof(struct sockaddr)) < 0) {
        freeaddrinfo(ai);
        return -1;
    }
    freeaddrinfo(ai);

    if (listen(listenfd, MAX_CONNEX_Q) < 0) {
        return -1;
    }

    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenfd, (struct sockaddr*)&ad, &len)) {
        return -1;
    }
    *portNum = ntohs(ad.sin_port); 

    return listenfd;
}

void process_connections(int fdServer, const int maxConnex,
        const char* authstring) {
    Stats* stats = stats_init();
    setup_sig_handling(stats);

    StringStore* publicDb = stringstore_init();
    StringStore* privateDb = stringstore_init();
    pthread_mutex_t* pubLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_t* privLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pubLock, NULL);
    pthread_mutex_init(privLock, NULL);

    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;

    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            perror("Error accepting connection");
        }
        
        pthread_mutex_lock(stats->statsLock);
        stats->currConnected++;

        if (stats->currConnected > maxConnex) {
            disconnect_max_connex(fd, stats);
        } else {
            pthread_mutex_unlock(stats->statsLock);

            ClientArgs* clientArgs = malloc(sizeof(ClientArgs));
            client_args_init(clientArgs, fd, authstring,
                    publicDb, privateDb, pubLock, privLock, stats);

            pthread_t threadId;
            pthread_create(&threadId, NULL, client_thread, clientArgs);
            pthread_detach(threadId);
        }
    }
}

void disconnect_max_connex(int fd, Stats* stats) {
    char* response = construct_HTTP_response(503,
            "Service Unavailable", NULL, NULL);
    FILE* to = fdopen(fd, "w");
    fprintf(to, response);
    fflush(to);
    fclose(to);
    stats->currConnected--;
    pthread_mutex_unlock(stats->statsLock);
}

void* report_thread(void* arg) {
    Stats* stats = (Stats*)arg;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    int s, sig;

    while (1) {
        s = sigwait(&set, &sig);
        if (s != 0) {
            errno = s;
            perror("sigwait");
        }
        pthread_mutex_lock(stats->statsLock);
        print_stats(stats);
        pthread_mutex_unlock(stats->statsLock);
    }
}

void* client_thread(void* arg) {
    ClientArgs clientArgs = *(ClientArgs*)arg;
    free(arg);

    int fd = clientArgs.fd;
    int fd2 = dup(fd);
    FILE* to = fdopen(fd, "w");
    FILE* from = fdopen(fd2, "r");
    Stats* stats = clientArgs.stats;

    while (process_request(from, to, clientArgs)) {
        // Continue processing
    }

    pthread_mutex_lock(stats->statsLock);
    stats->currConnected--;
    stats->totalDisconnected++;
    pthread_mutex_unlock(stats->statsLock);

    fclose(to);
    fclose(from);

    pthread_exit(NULL);
}

bool process_request(FILE* from, FILE* to, ClientArgs clientArgs) {
    Stats* stats = clientArgs.stats;
    char* method;
    char* address;
    HttpHeader** headers;
    char* body;
    if (!get_HTTP_request(from, &method, &address, &headers, &body)) {
        // request could not be processed
        return false;
    }

    for (int methodNum = 0; methodNum < NUM_METHODS; methodNum++) {
        if (!strcmp(method, methodNames[methodNum])) {
            char** dbAndKey = get_db_key(address);
            if (!dbAndKey) {
                break;
            }
            char* key = dbAndKey[KEY_POS];
            char* db = dbAndKey[DB_POS];

            if (!is_authorised(headers, db, clientArgs.authstring)) {
                unauthorised_connection(to, stats); 
                return true;
            }

            // Check which db is authorised
            StringStore* authorisedDb = (!strcmp(db, DB_PUBLIC)) ? 
                    clientArgs.publicDb : clientArgs.privateDb;
            pthread_mutex_t* dbLock = (!strcmp(db, DB_PUBLIC)) ?
                    clientArgs.pubLock : clientArgs.privLock;

            // Handler functions
            methodHandlers[methodNum](to, authorisedDb, dbLock, stats,
                    key, headers, body);
            return true;
        }
    }

    // Bad request
    char* response = construct_HTTP_response(400, "Bad Request", NULL, NULL);
    fprintf(to, response);
    fflush(to);
    return true;
}

void unauthorised_connection(FILE* to, Stats* stats) {
    pthread_mutex_lock(stats->statsLock);
    stats->authFails++;
    pthread_mutex_unlock(stats->statsLock);

    char* response = construct_HTTP_response(401, "Unauthorized",
            NULL, NULL);
    fprintf(to, response);
    fflush(to);
}

char** get_db_key(char* address) {
    char** dbAndKey = split_by_char(address, '/', 0);
    int numItems = 0;
    while (dbAndKey[numItems]) {
        numItems++;
    }

    if (numItems < MIN_ADDR_FIELDS) {
        return NULL;
    }

    char* db = dbAndKey[DB_POS];
    if (strcmp(db, DB_PUBLIC) && strcmp(db, DB_PRIVATE)) {
        return NULL;
    }

    return dbAndKey;
}

void handle_get_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body) {
    pthread_mutex_lock(dbLock);
    const char* val = stringstore_retrieve(db, key);
    pthread_mutex_unlock(dbLock);
    char* response;

    if (!val) {
        // Key not found
        response = construct_HTTP_response(404, "Not Found", NULL, NULL);
    } else {
        pthread_mutex_lock(stats->statsLock);
        stats->numGets++;
        pthread_mutex_unlock(stats->statsLock);
        response = construct_HTTP_response(200, "OK", NULL, val);
    }

    fprintf(to, response);
    fflush(to);
}

void handle_put_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body) {
    char* response;
    pthread_mutex_lock(dbLock);
    int addSuccess = stringstore_add(db, key, body);
    pthread_mutex_unlock(dbLock);

    if (addSuccess) {
        pthread_mutex_lock(stats->statsLock);
        stats->numPuts++;
        pthread_mutex_unlock(stats->statsLock);

        response = construct_HTTP_response(200, "OK", NULL, NULL);
    } else {
        response = construct_HTTP_response(500, "Internal Server Error",
                NULL, NULL);
    }

    fprintf(to, response);
    fflush(to);
}

void handle_delete_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body) {
    // Not found need to implement the stringstore library
    char* response;
    pthread_mutex_lock(dbLock);
    int deleteSuccess = stringstore_delete(db, key);
    pthread_mutex_unlock(dbLock);

    if (deleteSuccess) {
        pthread_mutex_lock(stats->statsLock);
        stats->numDeletes++;
        pthread_mutex_unlock(stats->statsLock);

        response = construct_HTTP_response(200, "OK", NULL, NULL);
    } else {
        response = construct_HTTP_response(404, "Not Found", NULL, NULL);
    }

    fprintf(to, response);
    fflush(to);
}

bool is_authorised(HttpHeader** headers, char* db, const char* authstring) {
    // User are always allowed public access
    if (!strcmp(db, DB_PUBLIC)) {
        return true;
    }

    int headerNum = 0;
    while (headers[headerNum]) {
        // Find the Authorization header
        if (!strcmp(((*headers[headerNum]).name), "Authorization")) {
            // Check if the user is authorised
            char* userstring = (*headers[headerNum]).value;
            if (!strcmp(userstring, authstring)) {
                return true;
            } else { // Do not allow multiple authorisation attempts
                break;
            }
        }

        headerNum++;
    }
    return false;
}

