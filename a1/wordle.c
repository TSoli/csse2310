/* FILE: wordle.c
 * 
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 *      This file is the entry point for the wordle game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <csse2310a1.h>

#define MAX_INPUTS 5
#define DEFAULT_LENGTH 5
#define DEFAULT_MAX 6
#define DEFAULT_DICT "/usr/share/dict/words"
#define INITIAL_BUFFER_SIZE 5
#define LETTER_NOT_FOUND '-'

/* Stores the words from a dictionary provided.
 * Params:
 *      words: An array of words in the dictionary
 *      numWords: The length of the words array
 */
typedef struct Dictionary {
    char** words;
    int numWords;
} Dictionary;

// Function Declarations
void clean_up(Dictionary dict);
int* process_inputs(int argc, char* argv[]);
int set_option(int* params, int pos, char* option);
bool is_int(char* str);
int play_game(int wordLen, int maxGuesses, Dictionary dict);
char* read_word(FILE* file);
int is_valid_guess(char* guess, char* answer, Dictionary dict);
void invalid_guess_msg(int errCode, int wordLen);
int read_dict(char* dictPath, Dictionary* dictionary);
char* get_guess_info(char* guess, char* answer);
void word_to_lower(char* word);

/* Entry point for the wordle program*/
int main(int argc, char* argv[]) {
    int* settings = process_inputs(argc, argv);

    if (!settings) {
        fprintf(stderr, "Usage: wordle [-len word-length] [-max max-guesses] "
                "[dictionary]\n");
        return 1;
    }

    // Should use a struct for this.
    const int wordLen = settings[0];
    const int maxGuesses = settings[1];
    char* dictPath;
    
    if (!settings[2]) {
        dictPath = DEFAULT_DICT;
    } else {
        dictPath = argv[settings[2]];
    }
    free(settings);

    // Can just return a Dictionary from my function. Should change this.
    Dictionary dict;
    dict.numWords = 0;
    dict.words = NULL;
    // Populate the dictionary and get the number of words
    int dictError = read_dict(dictPath, &dict);

    if (dictError == -1) {
        return 2;
    }
    
    if (play_game(wordLen, maxGuesses, dict)) {
        clean_up(dict);
        return 3;
    }
    clean_up(dict);
    return 0;
}

/* Clean up after exiting main 
 * Params:
 *      Dictionary dict: the dictionary that was allocated
*/
void clean_up(Dictionary dict) {
    for (int i = 0; i < dict.numWords; i++) {
        free(dict.words[i]);
    }
    free(dict.words);
}

/* Checks if the parameters for the program are valid. Expected parameters are
 * optional and include -len and -max which outline the word length and maximum
 * number of guesses, respectively. The path to a dictionary can also be
 * optionally included. The function does not guarantee the dictionary path is
 * valid.
 *
 * Params:
 *      argc: the number of elements in argv.
 *      argv: An array of parameters (passed to main)
 * Returns:
 *      An array of  integers initialised with malloc. The first
 *      element is the maximum length of words. The second is the max number
 *      of guesses and the third is the index of the dictionary path (or 0 if
 *      one was not passed). 
 * Errors:
 *      If the parameters were invalid, the function will
 *      return a NULL pointer.
 */
int* process_inputs(int argc, char* argv[]) {
    int* params = malloc(sizeof(int) * 3);
    memset(params, 0, sizeof(int) * 3); // invalid state
    const int defaults[3] = {DEFAULT_LENGTH, DEFAULT_MAX, 0}; 

    if (argc > (MAX_INPUTS + 1)) { // the first argument is the program name.
        return NULL;
    }

    for (int i = 1; i < argc; i++) { // Skip the program name.
        char* param = argv[i];
;
        if (!strcmp(param, "-len")) {
            if (++i >= argc) { // there is no argument
                return NULL;
            }
            if (set_option(params, 0, argv[i])) { // returns 1 if failed
                return NULL;
            }
        } else if (!strcmp(param, "-max")) {
            if (++i >= argc) {
                return NULL;
            }
            if (set_option(params, 1, argv[i])) {
                return NULL;
            }
        // only -len or -nax are valid and entry cannot be empty
        } else if (param[0] == '-' || !strcmp(param, "")) { 
            return NULL;
        } else { 
            if (!params[2]) { // storing index in argv to the path.
                params[2] = i;
            } else { // Dictionary path has already been read.
                return NULL;
            }
        }
    }
    
    // Set default values if needed.
    for (int i = 0; i < 3; i++) {
        if (!params[i]) {
            params[i] = defaults[i];
        }
    }

    return params;
}

/* Adds a valid option to the params array at pos. A valid input has one digit
 * in range 3-9 and ensures the option has not previously been set (it is 0).
 *
 * Params:
 *      params: the array of parameters being set.
 *      pos: the position in the params array to add the option.
 *      option: the option to be validated and added.
 * 
 * Return:
 *      0 if the option was set and 1 if not.
 */
int set_option(int* params, int pos, char* option) {
    if (params[pos]) { // param has already been set!
        return 1;
    }

    if (!is_int(option)) {
        return 1;
    }
        
    int digit = atoi(option);

    if (digit >= 3 && digit <= 9) {
        params[pos] = digit;
    } else {
        return 1;
    }

    return 0;
}

/* Checks if the string passed can be converted to an integer and returns true
 * iff it can be. The string must be well-formed (end in a '\0').
 */
bool is_int(char* str) {
    int i = 0;

    if (str[i] == '-') { // It is a negative number so start at the next digit
        i++;
    }

    while (str[i] != '\0') {
        if (!isdigit(str[i])) {
            return false;
        }
        i++;
    }

    return true;
}

/* Play wordle. Returns 1 if a problem occured or 0 if not.
 * 
 * Params:
 *      wordLen: the length of the answer.
 *      maxGuesses: the maximum number of guesses before game over.
 *      dict: the dictionary of allowed words.
 */
int play_game(int wordLen, int maxGuesses, Dictionary dict) {
    char* answer = get_random_word(wordLen);
    int guesses = 0;
    char* currGuess = NULL;
    int validity;

    printf("Welcome to Wordle!\n");

    while (guesses < maxGuesses) {
        // Make into prompt function.
        if (guesses == maxGuesses - 1) {
            printf("Enter a %d letter word (last attempt):\n", wordLen);
        } else {
            printf("Enter a %d letter word (%d attempts remaining):\n",
                    wordLen, (maxGuesses - guesses));
        }

        currGuess = read_word(stdin); 
        if (!currGuess) { // An invalid string was entered
            free(currGuess);
            break;
        }

        // convert guess to lowercase only.
        word_to_lower(currGuess);

        if (!strcmp(currGuess, answer)) {
            printf("Correct!\n");
            free(currGuess);
            free(answer);
            return 0;
        }
        
        validity = is_valid_guess(currGuess, answer, dict);
        if (validity == 0) { // A valid guess was entered
            guesses++;
        } else { // Invalid guess
            invalid_guess_msg(validity, wordLen); // Print the relevant error.
        }
        free(currGuess);
    }
    // Indicates EOF or ran out of guesses
    fprintf(stderr, "Bad luck - the word is \"%s\".\n", answer);
    free(answer);

    return 1;
} 

/* Reads one string from file and returns it. No guarantees about validity.
 *
 * Params:
 *      file: file to read word from.
 *
 * Return:
 *      the word read or NULL if not a word. Words must be ended with
 *      new line or EOF.
 *
 * Reference: inspired by Exercise 3 from the labs.
 */
char* read_word(FILE* file) {
    int bufferSize = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(sizeof(char) * bufferSize);
    int numRead = 0;
    int next;

    if (feof(file)) {
        return NULL;
    }

    while (true) {
        next = fgetc(file);
        if (next == EOF && numRead == 0) {
            free(buffer);
            return NULL;
        }
        if (numRead == bufferSize - 1) {
            bufferSize += 3;
            buffer = realloc(buffer, sizeof(char) * bufferSize);
        }
        if (next == EOF || next == '\n') {
            buffer[numRead] = '\0';
            break;
        }
        buffer[numRead++] = next;
    }
    return buffer;
}

/* Checks if the guess is valid.
 *
 * Params:
 *      guess: the word to check. Should be a well-formed string with
 *      no newline char at the end.
 *      answer: The answer word.
 *      dict: the dictionary file to check.
 *
 *  Returns:
 *      - 0 if the word is valid
 *      - 1 if the word is the incorrect length
 *      - 2 if the word is the correct length but contains characters not
 *          in A-Z (regardess of case)
 *      - 3 if the word cannot be found in the provided dictionary
 */
int is_valid_guess(char* guess, char* answer, Dictionary dict) {
    int wordLen = strlen(answer);
    if (strlen(guess) != wordLen) {
        return 1;
    }
    for (int i = 0; i < wordLen; i++) { // Check words are all letters
        if (!isalpha(guess[i])) {
            return 2;
        }
    }
    for (int i = 0; i < dict.numWords; i++) {
        if (!strcmp(guess, dict.words[i])) {
            char* info = get_guess_info(guess, answer);
            printf("%s\n", info);
            free(info);
            return 0; // word found!
        }
    }
    return 3; // word not found in dict.
}

/* Prints a message explaining why a guess was invalid based on an error code.
 */
void invalid_guess_msg(int errCode, int wordLen) {
    if (errCode == 1) {
        printf("Words must be %d letters long - try again.\n", wordLen);
    } else if (errCode == 2) {
        printf("Words must contain only letters - try again.\n");
    } else if (errCode == 3) {
        printf("Word not found in the dictionary - try again.\n");
    } else {
        printf("%d: Unrecognised errCode for guess\n", errCode);
    }
}

/* Read the dictionary from the path supplied and add the words to the
 * dictionary struct.
 *
 * Params:
 *      dictPath: A path to  the dictionary file with words separated by
 *      new lines or EOF.
 *      dictionary: A pointer to the Dictionary that will store the
 *      words.
 *
 * Returns:
 *      - 0: if successful
 *      - -1: if the file couldn't be opened
 *      - 1: if the dictionary is empty
 */
int read_dict(char* dictPath, Dictionary* dictionary) {
    FILE* dictFile = fopen(dictPath, "r");

    if (!dictFile || feof(dictFile)) {
        fprintf(stderr, "wordle: dictionary file \"%s\" cannot be opened\n",
                dictPath);
        return -1;
    }

    char* word = read_word(dictFile);
    if (!word) { // Empty Dictionary file!
        return 1;
    }

    int bufferSize;
    if (!(*dictionary).words) {
        bufferSize = INITIAL_BUFFER_SIZE;
        (*dictionary).words = malloc(sizeof(char*) * bufferSize);
    } else {
        bufferSize = (*dictionary).numWords;
    }

    while (word) {
        if ((*dictionary).numWords == bufferSize - 1) {
            bufferSize *= 2;
            (*dictionary).words = realloc((*dictionary).words,
                    sizeof(char*) * bufferSize);
        }

        word_to_lower(word);
        (*dictionary).words[(*dictionary).numWords] = word;
        (*dictionary).numWords++;
        word = read_word(dictFile);
    }
    fclose(dictFile);

    return 0; 
}

/* Return information about the user's guess. '-' means the letter is not in 
 * the  answer, an UPPERCASE letter means the letter was in the correct
 * position  and a lowercase letter means the letter is in the word but in a 
 * different position. Uses malloc to get the result.
 *
 * Params:
 *      guess: The word guessed.
 *      answer: The answer to check against.
 *
 * Return:
 *      A string with information about the guess as described above.
 */
char* get_guess_info(char* guess, char* answer) {
    char* result = malloc(sizeof(char) * (strlen(answer) + 1));
    memset(result, '-', sizeof(char) * strlen(answer));
    char* tmpAns = strdup(answer);
    char* tmpGuess = strdup(guess);

    // Check for letters in the correct spot
    for (int i = 0; i < strlen(answer); i++) {
        if (tmpAns[i] == tmpGuess[i] && tmpAns[i]) {
            result[i] = toupper(tmpGuess[i]);
            tmpAns[i] = '\0';
            tmpGuess[i] = '\0';
        }
    }
    // Check for correct letters in the wrong spot
    for (int i = 0; i < strlen(guess); i++) {
        for (int j = 0; j < strlen(answer); j++) {
            if (tmpGuess[i] == tmpAns[j] && tmpGuess[i]) {
                result[i] = tolower(tmpGuess[i]);
                tmpAns[j] = '\0';
                tmpGuess[i] = '\0';
            }
        }
    }
    result[strlen(answer)] = '\0';
    free(tmpGuess);
    free(tmpAns);

    return result;
}

/* Convert the string to only lowercase chars */
void word_to_lower(char* word) {
    for (int i = 0; word[i]; i++) {
        word[i] = tolower(word[i]);
    }
}
