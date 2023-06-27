/* FILE: stringstore.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * Provides an API for a simple database that can  add/remove/edit key:value
 * pairs upon request.
 */

#include <stdlib.h>
#include <string.h>
#include "stringstore.h"

#define INIT_BUFFERSIZE 20

// A struct for the key:value database
struct StringStore {
    char** keys;
    char** vals;
    int numKeys;
    int bufferSize;
};

// Creates a new StringStore instance and returns a pointer to it.
StringStore* stringstore_init(void) {
    StringStore* store = malloc(sizeof(StringStore));
    store->numKeys = 0;
    store->bufferSize = INIT_BUFFERSIZE;
    store->keys = malloc(sizeof(char) * store->bufferSize);
    store->vals = malloc(sizeof(char) * store->bufferSize);
    return store;
}

// Free all memort associated with the given StringStore and return NYLL
StringStore* stringstore_free(StringStore* store) {
    // Must free all the keys and vals in the stringstore...
    for (int i = 0; i < store->numKeys; i++) {
        free(store->keys[i]);
        free(store->vals[i]);
    }
    free(store->keys);
    free(store->vals);
    free(store);
    return NULL;
}

/* Add the given 'key'/'value' pair to the StringStore 'store'. The 'key' and
 * 'value' strings are copied with strdup() before being added to the database.
 * Returns 1 on success, 0 on failure (e.g. iif strdup() fails).
 *
 * Params:
 *      store: The StringStore pointer to add the key/val pair to.
 *      key: The key in the key/val pair.
 *      val: The val in the key/val pair.
 */
int stringstore_add(StringStore* store, const char* key, const char* value) {
    char* key2Add = strdup(key);
    char* val2Add = strdup(value);

    if (!key2Add || !val2Add) {
        return 0;
    }

    // Check if the key already exists
    for (int i = 0; i < store->numKeys; i++) {
        if (!strcmp(store->keys[i], key)) {
            free(store->vals[i]);
            free(key2Add);
            store->vals[i] = val2Add;
            return 1;
        }
    }

    // Check if need to increase the buffer
    if (store->numKeys == store->bufferSize) {
        // Why won's reallocarray work???
        store->keys = realloc(store->keys,
                sizeof(char*) * 2 * store->bufferSize);
        store->vals = realloc(store->vals,
                sizeof(char*) * 2 * store->bufferSize);
        if (!store->keys || !store->vals) {
            // realloc failed
            return 0;
        }

        store->bufferSize *= 2;
    }

    store->keys[store->numKeys] = key2Add;
    store->vals[store->numKeys] = val2Add;
    store->numKeys++;
    return 1;
}

/* Attempt to retrieve the value associated with a particular 'key' in the
 * StringStore 'store'.
 *
 * Params:
 *      store: The database to delete the key from.
 *      key: The key for the key value pair to retrieve.
 * 
 * Return:
 *      The string representing the value associated with the key. If the key
 *      does not exist in the StringStore database then a NULL pointer is
 *      returned instead.
 */
const char* stringstore_retrieve(StringStore* store, const char* key) {
    // Perhaps more likely to get keys which have been recently added to end
    // of list of keys
    for (int i = store->numKeys - 1; i >= 0; i--) {
        if (!strcmp(store->keys[i], key)) {
            const char* val = store->vals[i];
            return val;
        }
    }
    return NULL;
}

/* Attempt to delete the key/value pair associated with a particular 'key' in
 * the StringStore 'store'.
 *
 * Params:
 *      store: The StringStore database to delte the key/val from.
 *      key: The key of the key/val pair to be deleted from store.
 *
 * Return:
 *      1 if the key exists and deletion succeeds or 0 otherwise.
 */
int stringstore_delete(StringStore* store, const char* key) {
    // Check the rest of the keys
    for (int i = 0; i < store->numKeys; i++) {
        if (!strcmp(store->keys[i], key)) {
            store->numKeys--;
            free(store->keys[i]);
            free(store->vals[i]);
            
            // Replace the deleted key/val with the key/val at the end
            if (i != store->numKeys) {
                store->keys[i] = store->keys[store->numKeys];
                store->vals[i] = store->vals[store->numKeys];
            }

            store->keys[store->numKeys] = NULL;
            store->vals[store->numKeys] = NULL;
            return 1;
        }
    }
            
    // Key doesn't exist
    return 0;
}
