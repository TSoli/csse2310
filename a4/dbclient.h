/* FILE: dbclient.h
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * A simple client that can add/remove/edit key:value pairs from a server.
 */

#ifndef DBCLIENT_H
#define DBCLIENT_H

#define MIN_ARGS 3 // Includes program name
#define MAX_ARGS 0 // 0 indicates no max
#define USAGE_EXIT_CODE 1
#define INVALID_KEY_EXIT_CODE 1
#define CONNECTION_EXIT_CODE 2
#define STATUS_OK 200
#define CANT_GET_EXIT_CODE 3
#define CANT_PUT_EXIT_CODE 4
#define USAGE_MSG "Usage: dbclient portnum key [value]\n"
#define INVALID_KEY_MSG "dbclient: key must not contain spaces or newlines\n"
#define CANT_CONNECT "dbclient: unable to connect to port %s\n"
#define PORT_POS 1
#define KEY_POS 2
#define VAL_POS 3
#define SERVER_IP "localhost"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <csse2310a4.h>
#include "readCommline.h"

/* Perform checks on the commandline arguments and check if they are valid.
 * If not valid, print an error message and exit the program with the
 * appropriate status code.
 *
 * Params:
 *      argc: the number of arguments passed to the program.
 *      argv: The arguments passed to the program.
 */
void check_args(int argc, char* argv[]);

/* Connect to the SERVER_IP on the port passed and return a file descriptor for
 * the connection. If a connection cannot be made then an error message will
 * be printed to stderr and the program will exit appropriately.
 *
 * Params:
 *      port: The port number to connect to on SERVER_IP.
 * 
 * Return:
 *      A file descriptor for the connection.
 */
int connect_to_server(char* port);

/* Send a PUT request to the server using the to FILE*. Use the key
 * and val passed to the function. Await for a response and print an
 * appropriate message to stdout.
 *
 * Params:
 *      to: The FILE* that can be used to send a request to the server.
 *      from: The FILE* that can be used to receive a response from a server.
 *      key: The key to be added.
 *      val: The value for the key.
 */
void send_put(FILE* to, FILE* from, char* key, char* val);

/* Send a GET request to the server using the to FILE*. Use the key
 * and val passed to the function. Await for a response and print an
 * appropriate message to stdout.
 *
 * Params:
 *      to: The FILE* that can be used to send a request to the server.
 *      from: The FILE* that can be used to receive a response from a server.
 *      key: The key to get the value for.
 */
void send_get(FILE* to, FILE* from, char* key);

/* Await a response from the server and return the body. If there is an error,
 * exit appropriately.
 *
 * Params:
 *      from: The file stream to get a response from the server.
 *      body: A pointer to store the body of the response into.
 * 
 * Return:
 *      true if the request receives status code 200 ok. false otherwise.
 */
bool get_response(FILE* from, char** body);

#endif
