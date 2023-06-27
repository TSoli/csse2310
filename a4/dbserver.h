/* FILE: dbserver.h
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * A simple that can add/remove/edit key:value pairs upon request and 
 * return them to a client when asked.
 */

#ifndef DBSERVER_H
#define DB_SERVER_H

#define AUTH_POS 1
#define NUM_CONNEX_POS 2
#define PORT_POS 3
#define MIN_ARGS 3
#define MAX_ARGS 4
#define MIN_PORT 1024
#define MAX_PORT 65535
#define USAGE_MSG "Usage: dbserver authfile connections [portnum]\n"
#define USAGE_EXIT_CODE 1
#define AUTH_MSG "dbserver: unable to read authentication string\n"
#define AUTH_EXIT_CODE 2
#define PORT_MSG "dbserver: unable to open socket for listening\n"
#define PORT_EXIT_CODE 3
#define DEFAULT_PORT "0"    // Use the ephemeral port by default
#define MAX_CONNEX_Q 10
#define NUM_METHODS 3
#define DB_PUBLIC "public"
#define DB_PRIVATE "private"
#define DB_POS 1
#define KEY_POS 2
#define MIN_ADDR_FIELDS 3

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <csse2310a3.h>
#include <csse2310a4.h>
#include <stringstore.h>
#include <signal.h>
#include "readCommline.h"
#include "utilities.h"

/* A struct that stores usages information about the server.*/
typedef struct Stats Stats;

/* A struct to store the arguments to pass to the client thread.*/
typedef struct ClientArgs ClientArgs;

/* Functions used to send a HTTP response */
typedef void (*HandleHttpReq)(FILE*, StringStore*, pthread_mutex_t* dbLock,
        Stats* stats, char*, HttpHeader**, char*);

/* Initialise the ClientArgs struct.
 *
 * Params:
 *      clientArgs: A pointer to a clientArgs struct to be initialised.
 *      fd: The file dscriptor used to communicate with the client.
 *      authstring: The authorisation string needed to access the privatedDb.
 *      publicDb: A pointer to the public StringStore instance.
 *      privateDb: A pointer to the private StringStore instance.
 *      pubLock: A pointer to the mutex for the publicDb.
 *      privLock: A pointer to the mutex for the privDb.
 *      stats: A pointer to a Stats struct used to record server usage info.
 */
void client_args_init(ClientArgs* clientArgs, int fd, const char* authstring,
        StringStore* publicDb, StringStore* privateDb,
        pthread_mutex_t* pubLock, pthread_mutex_t* privLock, Stats* stats);

/* Perform checks on the commandline arguments and check if they are valid.
 * If not valid, print an error message and exit the program with the
 * appropriate status code.
 *
 * Params:
 *      argc: the number of arguments passed to the program.
 *      argv: The arguments passed to the program.
 */
void check_args(int argc, char* argv[]);

/* Checks that the command line arguments are valid (correct number of
 * connections, valid port number, valid number of connections).
 *
 * Params:
 *      argc: The number of commandline arguments.
 *      argv: An array of the commandline arguments.
 * 
 * Return:
 *      true if the commandline arguments are valid
 */
bool is_valid_commandline(int argc, char* argv[]);

/* Attempt to open the authentication file. Exits appropriately if the file
 * does not exist, can't be opened or is empty. Otherwise returns the
 * authorisation string that is conatined in the file.
 *
 * Params:
 *      authfile: The path to the authentication file.
 *
 * Return:
 *      The authstring in the file (found on the first line).
 */
const char* get_authstring(char* authfile);

/* Get a file descriptor for a socket to listen for connections on.
 *
 * Params:
 *      port: The port number to listen on ("0" for an ephemeral port).
 *      portNum: The actual port number being listened on will be saved to this
 *      (useful if an an ephemeral port was requested).
 * 
 * Return:
 *      The port number if it is valid or -1 if not.
 */
int open_listen(char* port, uint16_t* portNum);

/* Process connection requests. Once a connection request is received, a new
 * thread will be created to handle requests from the client so that the server
 * can continue to listen for connections. Public and private databases are
 * initialised here and may be updated via requests from clients.
 *
 * Params:
 *      fdServer: The file descriptor for the server to listen on.
 *      maxConnex: The maximum number of concurrent connections supported
 *      by the server.
 *      authstring: The authorisation string required to access the private
 *      database.
 */
void process_connections(int fdServer, const int maxConnex,
        const char* authstring);

/* Disconnects the client and responds with 503 (Service Unavailable). This
 * is to be used if the max connections is reached.
 *
 * Params:
 *      fd: The file descriptor used to communicate with the client.
 *      stats: A pointer to the Stats struct to report server usage stats to.
 */
void disconnect_max_connex(int fd, Stats* stats);

/* Process a HTTP request by taking any necessary actions and sending the
 * appropriate response.
 *
 * Params:
 *      from: The file pointer used to receive a request from the client.
 *      to: The file pointer used to send information back to the client.
 *      clientArgs: A clientArgs struct that contains the necessary parameters
 *      for the client_thread.
 *
 * Return:
 *      true if the request could be processed and a response was made or false
 *      if a badly formed request was received.
 */
bool process_request(FILE* from, FILE* to, ClientArgs clientArgs); 

/* A thread used to handle client requests. If a badly formed request is
 * received, the thread will exit. Otherwise, the thread will respond to the
 * client with the appropriate responses.
 *
 * Params:
 *      arg: Contrains a pointer to a ClientArgs struct with relevant
 *      information for the client thread.
 */
void* client_thread(void* arg);

/* Extracts the database and key from the address string. If either are illegal
 * then the function returns a NULL pointer.
 *
 * Params:
 *      address: The address string in the format /db/key
 *
 * Return:
 *      An array containing the db in the first position and the key in the
 *      second or NULL if the key or database was illegal.
 */
char** get_db_key(char* address);

/* Handles a GET request from the client by sending the appropriate response.
 *
 * Params:
 *      to: The file descriptor to send the response to.
 *      db: The database to GET from.
 *      dbLock: A mutex used when accessing the db.
 *      stats: A pointer to a Stats struct that contains server usage info.
 *      key: The key for the value to GET.
 *      headers: The headers from the HTTP request.
 *      body: The body of the HTTP request (not used)
 */
void handle_get_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body);

/* Handles a PUT request from the client by sending the appropriate response.
 *
 * Params:
 *      to: The file descriptor to send the response to.
 *      db: The database to PUT the key value pair in.
 *      dbLock: A mutex used when accessing the db.
 *      stats: A pointer to a Stats struct that contains server usage info.
 *      key: The key for the value to PUT.
 *      headers: The headers from the HTTP request.
 *      body: The body of the HTTP request which should just contain the value
 *      to PUT.
 */
void handle_put_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body);

/* Handles a DELETE request from the client by sending the appropriate response
 *
 * Params:
 *      to: The file descriptor to send the response to.
 *      db: The database to PUT the key value pair in.
 *      dbLock: A mutex used when accessing the db.
 *      stats: A pointer to a Stats struct that contains server usage info.
 *      key: The key for the value to DELETE.
 *      headers: The headers from the HTTP request.
 *      body: The body of the HTTP request (not used).
 */
void handle_delete_req(FILE* to, StringStore* db, pthread_mutex_t* dbLock,
        Stats* stats, char* key, HttpHeader** headers, char* body);

/* Checks if the user is authorised. The user is authorised if their request
 * contains the Authorization header with the correct authstring or they are
 * attempting to access the public database.
 *
 * Params:
 *      headers: The HTTP headers from the request.
 *      db: The database they are trying to access.
 *      authstring: The correct authorisation string.
 *
 * Return:
 *      true if the user is authorised to access the specified database.
 */
bool is_authorised(HttpHeader** headers, char* db, const char* authstring);

/* Sends the client a response if an unauthorised user attempts to access
 * privileged information and records it to the Stats struct.
 *
 * Params:
 *      to: The file pointer used to communicate with the client.
 *      stats: A pointer to a Stats struc that records server usage info.
 */
void unauthorised_connection(FILE* to, Stats* stats);

/* Initialises and returns a pointer to a Stats struct that is used to record
 * server usage statistics to be reported later.
 */
Stats* stats_init(void);

/* Print the current usage stats for the server. This is intended to be called
 * when the process receives SIGHUP.
 *
 * Params:
 *      stats: A pointer to a Stats struct containing server usage stats.
 */
void print_stats(Stats* stats);

/* Sets up the signal handling for dbserver. In this case a handler for SIGHUP
 * that prints some server usage statistics to stderr is implemeted.
 *
 * Params:
 *      stats: A pointer to a Stats struct that contains usage info about the
 *      server.
 */
void setup_sig_handling(Stats* stats);

/* A thread used to handle SIGHUP and report usage stats.
 *
 * Params:
 *      arg: Contains a pointer to the Stats struct that will be printed.
 */
void* report_thread(void* arg);

#endif
