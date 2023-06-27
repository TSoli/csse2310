/* FILE: hq.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * The entry point for hq. hq prompts the user and responds to their commands.
 * It can be used to spawn processes, check on them e.t.c.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <csse2310a3.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <math.h>

#define PROMPT "> "
#define SEP " "
#define INVALID_COMMAND "Error: Invalid command"
#define INSUFFICIENT_ARGS "Error: Insufficient arguments"
#define INVALID_SIGNAL "Error: Invalid signal"
#define INVALID_SLEEP "Error: Invalid sleep time"
#define JOB_CREATED "New Job ID [%d] created\n"
#define NUM_COMMANDS 9  // The number of possible commands listed in commands
                        // below
#define EXEC_FAIL 99 // Error code if exec fails in a child
// When using spawn the command name is the first argument
#define COMMAND_POS 0
#define SPAWN_JOB_POS 0
#define JOB_ID_POS 0
#define SIG_ID_POS 0
#define SIG_SIG_POS 1
#define RCV_ID_POS 0
#define EOF_ID_POS 0
#define SLEEP_TIME_POS 0
#define SEND_ID_POS 0
#define SEND_MSG_POS 1
#define REPORT_HEADER "[Job] cmd:status"
#define INVALID_JOB "Error: Invalid job"
#define MAX_SIG_NUM 31
#define NO_INPUT "<no input>"

/* An array of the possible commands for hq*/
const char* commandNames[] = {
    NULL,
    "spawn",
    "report",
    "signal",
    "sleep",
    "send",
    "rcv",
    "eof",
    "cleanup"
};

/* Defined the index for different possible commands for hq. These will be used
 * for the other minArgs and also to identify what command needs to be executed
 */
typedef enum CommCode {
    UNKNOWN = 0,
    SPAWN = 1,
    REPORT = 2,
    SIGNAL = 3,
    SLEEP = 4,
    SEND = 5,
    RCV = 6,
    CLOSE = 7,
    CLEANUP = 8
} CommCode;

/* Defines the minimum number of arguments for each command */
const int minArgs[] = {
        0, //UNKNWONW
        1, // SPAWN
        0, // REPORT
        2, // SIGNAL
        1, // SLEEP
        2, // SEND
        1, // RCV
        1, // CLOSE
        0}; // CLEANUP

/* Stores the command arguments for a program and the number of command
 * arguments in len. The first command argument will be the program name.
 */
typedef struct CommandArgs {
    CommCode cmd;
    char** args;
    int len;
} CommandArgs;

/* The possible statuses for a job*/
typedef enum JobStatus {
    RUNNING,
    EXITED,
    SIGNALLED
} JobStatus;

/* Stores information about processes that have been spawned to enable
 * communications with the parent.
 */
typedef struct Job {
    int id;
    pid_t pid;
    char* name;
    FILE* input;
    bool inputOpen;
    FILE* output;
    JobStatus status;
    int exitStatus;
} Job;

/* This function literally does nothing. It is to be used as a signal handler
 *
 * Params:
 *      signum: The signal number that is taken for a signal handler.
 */
void do_nothing(int signum) {
}

/* Set up a signal handler to ignore singal signum.
 *
 * Params:
 *      signum: The number of the signal to be ignored.
 */
void setup_signal_handler(int signum) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = do_nothing;
    sigaction(signum, &sa, NULL);
}

/* Checks if the string passed can be converted to an integer and returns true
 * iff it can be. The string must be well-formed (end in a '\0').
 *
 * Params:
 *      str: The string to check if it is an int
 * Return:
 *      true iff the string represents an integer (only contains digits but 
 *      may be negative). Infinite and NaN are not considered valid integers.
 *
 * Reference:
 *      This is from the worldle program I wrote for a1.
 */
bool is_int(char* str) {
    int i = 0;

    if (str[i] == '-') { // It is a negative number so start at the next digit
        i++;
    }

    if (str[i] == '\0') { // String is empty
        return false;
    }

    while (str[i] != '\0') {
        if (!isdigit(str[i])) {
            return false;
        }
        i++;
    }

    return true;
}

/* Get the command code for an input and set the CommCode in CommandArgs.
 *
 * Params:
 *      commArgs: A pointer to the CommandArgs struct that the CommCode will be
 *      set for.
 *      cmd: The command that was entered.
 *
 *      Return:
 *          false for an unknown command and true otherwise.
 */
bool set_cmd_code(CommandArgs* commArgs, char* cmd) {
    // The lowest possile cmdCode will always be 1 - NOT a magic number
    for (int cmdCode = 1; cmdCode < NUM_COMMANDS; cmdCode++) {
        if (!strcmp(commandNames[cmdCode], cmd)) {
            commArgs->cmd = cmdCode;
            return true;
        }
    }

    commArgs->cmd = UNKNOWN;
    return false;
}

/* Decompose the list of strings from split_space_not_quote() into the command
 * and argument.
 *
 * Params:
 *      cmd: Will be initialised as a malloc()'d copy of the first element of
 *      allArgs. This is the command sent.
 *
 *      allArgs: The array containing the command and arguments. The first
 *      element must point to the command. The new array and its elements will
 *      be malloc()'d so they must be freed by the caller. This also means that
 *      allArgs and its elements may be free'd if they are no longer needed.
 *
 *      len: The length allArgs array .
 * 
 * Return:
 *      args: An array containing the arguments for the command. Memory will be
 *      allocated for it. A NULL pointer will be added to the end of the array.
 */
char** decompose_args(char** cmd, char** allArgs, int len) {
    *cmd = malloc(sizeof(char) * (strlen(allArgs[COMMAND_POS]) + 1));
    memcpy(*cmd, allArgs[COMMAND_POS], (strlen(allArgs[COMMAND_POS]) + 1));

    // Discard the first element with the command in it and copy to new array
    char** args = malloc(sizeof(char*) * len); 

    int i;
    for (i = 1; i < len; i++) {
        char* arg = malloc(sizeof(char) * (strlen(allArgs[i]) + 1));
        memcpy(arg, allArgs[i], (strlen(allArgs[i]) + 1));
        args[i - 1] = arg;
    }
    args[i - 1] = NULL;
    
    return args;
}

/* Take an input line and initialise a CommandArgs struct to represent it.
 *
 * Params:
 *      line: The line entered.
 *      commArgs: A pointer to a CommandArgs struct.
 * 
 * Return:
 *      true if the CommandArgs struct could be initialised and false otherwise
 */
bool valid_commands(char* line, CommandArgs* commArgs) {
    int len;
    char** allArgs = split_space_not_quote(line, &len);
    if (!allArgs) { // empty line - should already have been checked... 
        return false;
    }

    // Decompose into command and arguments
    char* cmd = NULL;
    // Length of allArgs includes NULL
    char** args = decompose_args(&cmd, allArgs, (len)); 
    free(allArgs);
    free(line);

    // Attempt to set the command code using the command string extracted
    // The first elemet will be the command
    if (!set_cmd_code(commArgs, cmd)) {
        printf("%s\n", INVALID_COMMAND);
        fflush(stdout);
        
        free(args);
        free(cmd);
        // Invalid cmd so no need to check args
        return false;
    }
    free(cmd);

    // The arguments for the command are listed after the command name.
    commArgs->args = args;
    commArgs->len = len - 1; // Have removed an element from allArgs 
                                
    return true;
}

/* Repeatedly prompt the user for commands on stdout until a valid command is
 * entered.
 *
 * Params:
 *      commArgs: A pointer to a CommandArgs struct to store the command info
 *      in for a valid input.
 * 
 * Return:
 *      true if the CommandArgs struct could be initialised, false if EOF.
 */
bool prompt_user(CommandArgs* commArgs) {
    bool commandIsValid = false;

    while (!commandIsValid) {
        // Prompt the user
        printf(PROMPT);
        fflush(stdout);
        char* line = read_line(stdin);
        if (!line) { // EOF
            return false;
        }

        // Process the input
        commandIsValid = valid_commands(line, commArgs);
    }
    // valid command was entered
    return true;
}

/* Initialise Job attributes. This will allocate space for a name and copy it
 * into the name argument into the Job using memcpy.
 *
 * Params:
 *     job: A pointer to the Job that will have its members set.
 *     id: The id for the job.
 *     name: The name of the job.
 *     input: A file descriptor for the input to the Job.
 *     output: A file descriptor to read from the Job.
 */
void create_job(Job* job, int id, char* name, int input, int output) {
    char* nameCpy = malloc(sizeof(char) * (strlen(name) + 1));
    memcpy(nameCpy, name, strlen(name) + 1);

    job->id = id;
    job->name = nameCpy;
    job->input = fdopen(input, "w");
    job->inputOpen = true;
    job->output = fdopen(output, "r");
    job->status = RUNNING; // Job has just been created so it is running.
}

/* Free any memory allocated to CommandArgs
 *
 * Params:
 *      commArgs: The command args struct whose members will be freed.
 */
void clean_cmd_args(CommandArgs commArgs) {
    for (int i = 0; i < commArgs.len; i++) {
        free(commArgs.args[i]);
    }
    free(commArgs.args);
}

/* Free any memory allocated for Jobs in the jobs array and the array itself.
 *
 * Params:
 *      jobs: An array of the jobs.
 *      jobLen: The number of jobs.
 */
void free_jobs(Job* jobs, int jobLen) {
    for (int i = 0; i < jobLen; i++) {
        free(jobs[i].name);
        fclose(jobs[i].output);

        if (jobs[i].inputOpen) {
            fclose(jobs[i].input);
        }
    }
    free(jobs);
}

/* spawn a child process after forking
 *
 * Params:
 *      parent2Child: The pipe from parent to child.
 *      child2Parent: The pipe from child to parent.
 *      jobs: The array of jobs that need to be freed.
 *      id: The id for the current job.
 *      commArgs: The CommandArgs struct with the command info in it.
 *
 *  Return:
 *      true if the process could be executed and false otherwise.
 */
bool spawn_child(int* parent2Child, int* child2Parent,
        Job* jobs, int id, CommandArgs commArgs) {
    close(parent2Child[1]);
    close(child2Parent[0]);

    dup2(parent2Child[0], STDIN_FILENO);
    dup2(child2Parent[1], STDOUT_FILENO);

    close(parent2Child[0]);
    close(child2Parent[1]);

    free_jobs(jobs, id);

    // Run the program - the arguments for the program are listed in
    // commArgs.args after the first argument.
    if (execvp(commArgs.args[SPAWN_JOB_POS], commArgs.args) < 0) {
        clean_cmd_args(commArgs);
        return false;
    }
    clean_cmd_args(commArgs);
    return true;
}

/* fork a new process with pipes such that the child's stdin and stdout are
 * directed from/to hq and exec the requested program. $PATH will be searched
 * for executable programs. Each process will be allocated a jobID even if
 * exec() fails.
 *
 * Params:
 *      commArgs: A struct containing the information required to execute the
 *      spawn command.
 *      id: The id for the job.
 *      job: A pointer to the job to be initialised.
 * 
 * Return:
 *      true if the job was created, false otherwise.
 * 
 * Reference:
 *      pipedemo.c provided in resources/week6
 */
bool spawn(CommandArgs commArgs, int id, Job* jobs) {
    // Initialise the file descriptors
    int parent2Child[2];
    int child2Parent[2];

    if (pipe(parent2Child) < 0) {
        perror("Creating pipe: parent to child");
        return false;
    }
    if (pipe(child2Parent) < 0) {
        perror("Creating pipe: child to parent");
        close(parent2Child[0]);
        close(parent2Child[1]);
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("Unable to spawn child");
        return false;
    }
    if ((jobs[id].pid = pid)) {
        // parent
        close(parent2Child[0]);
        close(child2Parent[1]);

        create_job(&(jobs[id]), id, commArgs.args[SPAWN_JOB_POS],
                parent2Child[1], child2Parent[0]);

        printf(JOB_CREATED, jobs[id].id);
    } else {
        // child
        if (!spawn_child(parent2Child, child2Parent, jobs, id, commArgs)) {
            exit(EXEC_FAIL);
        }
        exit(0);
    }
    return true;
} 

/* Get the id from the argument provided. Id must be a positive integer < maxId
 *
 * Params:
 *      arg: The argument provided.
 *      maxId: The maximum valid id.
 *
 * Return:
 *      The id if it is valid or -1 if not valid.
 */
int get_id_arg(char* arg, int maxId) {
    if (!is_int(arg)) {
        return -1;
    }

    int id = atoi(arg);
    if (id < 0 || id > maxId) {
        return -1;
    }

    return id;
}

/* Update the status and exitStatus of a Job by waiting on it.
 *
 * Params:
 *      job: The job whose exit status will be updated.
 *      options: The options used for waiting on the child
 */
void update_job_status(Job* job, int options) {
    if (job->status != RUNNING) {
        // No need to update the status
        return;
    }

    int* wstatus = malloc(sizeof(int));
    if (waitpid(job->pid, wstatus, options)) {
        // Process exited
        if (WIFEXITED(*wstatus)) {
            job->status = EXITED;
            job->exitStatus = WEXITSTATUS(*wstatus);
        } else if (WIFSIGNALED(*wstatus)) {
            job->status = SIGNALLED;
            job->exitStatus = WTERMSIG(*wstatus);
        }
    }
    free(wstatus);
} 

/* Print out the status of the job to stdout.
 *
 * Params:
 *      job: The job whose status will be printed.
 */
void print_job_status(Job job) {
    switch (job.status) {
        case EXITED:
            printf("[%d] %s:exited(%d)\n", job.id,
                    job.name, job.exitStatus);
            break;
        case SIGNALLED:
            printf("[%d] %s:signalled(%d)\n", job.id,
                    job.name, job.exitStatus);
            break;
        default:
            printf("[%d] %s:running\n", job.id,
                    job.name);
    }
}

/* print out information about a job/s including its id, name and status.
 *
 * Params:
 *      commArgs: The CommandArgs struct with the info for report.
 *      jobs: The array of jobs indexed by their id.
 *      len: The number of jobs in the jobs array.
 */
void report(CommandArgs commArgs, Job* jobs, int jobLen) {
    if (commArgs.len == 0) { // No job id was specified
        printf("%s\n", REPORT_HEADER);

        for (int i = 0; i < jobLen; i++) {
            update_job_status(&jobs[i], WNOHANG);
            print_job_status(jobs[i]);
        } 
    } else if (commArgs.len > 0) {
        // Max id is one less than number of jobs
        int jobId = get_id_arg(commArgs.args[JOB_ID_POS], jobLen - 1);

        // Check that the argument is valid.
        if (jobId < 0) {
            printf("%s\n", INVALID_JOB);
            return;
        } 

        printf("%s\n", REPORT_HEADER);
        update_job_status(&jobs[jobId], WNOHANG);
        print_job_status(jobs[jobId]);
    } else {
        printf("Error: commArgs not initialised properly\n\
                commArgs.len < 0\n");
    }
}

/* Send a particular signal to a particular job. Arguments are specified in
 * commArgs. Validates the arguments before sending.
 *
 * Params:
 *      commArgs: A struct containing the signal number and job id. The first
 *      element of commArgs.args will be the job id and the second argument
 *      will be the signal number.
 *      jobs: The array of jobs.
 *      jobLen: Length of the jobs array.
 */
void send_signal(CommandArgs commArgs, Job* jobs, int jobLen) {
    // Max id is one less than the number of jobs
    int jobIndex = get_id_arg(commArgs.args[SIG_ID_POS], jobLen - 1);
    int sig = atoi(commArgs.args[SIG_SIG_POS]);

    if (jobIndex < 0) {
        printf("%s\n", INVALID_JOB);
        return;
    }
    if (!is_int(commArgs.args[SIG_SIG_POS]) || sig > MAX_SIG_NUM || sig < 1) {
        printf("%s\n", INVALID_SIGNAL);
        return;
    }

    pid_t pid = jobs[jobIndex].pid;
    kill(pid, sig);
    update_job_status(&jobs[jobIndex], WNOHANG);
}

/* Send the terminal to sleep for a period of time.
 *
 * Params:
 *      commmArgs: A struct containing command arguments including the time
 *      to sleep for.
 */
void sleep_terminal(CommandArgs commArgs) {
    char* remainder;
    // Convert to microseconds (using base 10) (could use zero instead?)
    long sleepTime = strtol(commArgs.args[SLEEP_TIME_POS],
            &remainder, 10) * 1e6;

    if (strlen(commArgs.args[SLEEP_TIME_POS]) == 0 || strlen(remainder) != 0 ||
            !isfinite((float)sleepTime) || sleepTime < 0) {
        printf("%s\n", INVALID_SLEEP);
        return;
    }

    usleep(sleepTime);
}

/* Attempt to send a message contained in commArgs to the job also specified
 * in commArgs.
 *
 * Params:
 *      commArgs: The commArgs struct containing info about the most recent
 *      command.
 *      jobs: An array of jobs.
 *      jobLen: The number of jobs in the jobs array.
 */
void send_msg(CommandArgs commArgs, Job* jobs, int jobLen) {
    // Max id is one less than the number of jobs
    int id = get_id_arg(commArgs.args[SEND_ID_POS], jobLen - 1);

    if (id < 0) {
        printf("%s\n", INVALID_JOB);
        return;
    }
    
    fprintf(jobs[id].input, "%s\n", commArgs.args[SEND_MSG_POS]);
    fflush(jobs[id].input);
}

/* Read a message from the relevant job and print it to stdout.
 *
 * Params:
 *      commArgs: A struct contraining the command arguments.
 *      jobs: An array of jobs that can be read from.
 *      jobLen: The number of Jobs in jobs.
 */
void rcv_msg(CommandArgs commArgs, Job* jobs, int jobLen) {
    // Max id is one less than the number of jobs
    int id = get_id_arg(commArgs.args[RCV_ID_POS], jobLen - 1);

    if (id < 0) {
        printf("%s\n", INVALID_JOB);
        return;
    }

    if (is_ready(fileno(jobs[id].output))) {
        if (!jobs[id].output) { // Previous EOF
            printf("<EOF>\n");
            return;
        }

        char* line = read_line(jobs[id].output);
        if (!line) { // EOF
            printf("<EOF>\n");
            return;
        }

        printf("%s\n", line);
        fflush(stdout);
        free(line);
    } else {
        printf("%s\n", NO_INPUT);
    }
}

/* Close the pipe to a job.
 *
 * Params:
 *      commArgs: The required command arguments
 *      jobs: A list of the jobs.
 *      jobLen: The number of jobs in jobs.
 */
void send_eof(CommandArgs commArgs, Job* jobs, int jobLen) {
    // The max id is one less than the number of jobs
    int id = get_id_arg(commArgs.args[RCV_ID_POS], jobLen - 1);

    if (id < 0) {
        printf("%s\n", INVALID_JOB);
        return;
    }

    fclose(jobs[id].input);
    jobs[id].inputOpen = false;
}

/* Clean up job processes by sending them SIGKILL and waiting on them.
 * Updates the job.status.
 *
 * Params:
 *      jobs: An array of the jobs.
 *      jobLen: The number of jobs in jobs
 */
void cleanup(Job* jobs, int jobLen) {
    for (int id = 0; id < jobLen; id++) {
        if (jobs[id].status == RUNNING) {
            pid_t pid = jobs[id].pid;
            kill(pid, SIGKILL);
            update_job_status(&jobs[id], 0);
        }
    }
}

/* Execute the command specified by the user.
 *
 * Params:
 *      commArgs: The commArgs struct containing command instructions.
 *      jobs: An array of jobs.
 *      jobLen: The number of jobs in the jobs array.
 *
 * Return:
 *      true if the command was known or false if not
 */
bool execute_command(CommandArgs commArgs, Job* jobs, int jobLen) {
    switch (commArgs.cmd) {
        case REPORT:
            report(commArgs, jobs, jobLen);
            break;
        case SIGNAL:
            send_signal(commArgs, jobs, jobLen);
            break;
        case SLEEP:
            sleep_terminal(commArgs);
            break;
        case SEND:
            send_msg(commArgs, jobs, jobLen);
            break;
        case RCV:
            rcv_msg(commArgs, jobs, jobLen);
            break;
        case CLOSE:
            send_eof(commArgs, jobs, jobLen);
            break;
        case CLEANUP:
            cleanup(jobs, jobLen);
            break;
        default:
            return false;
    }
    return true;
}

/* Entry point for hq */
int main(int argc, char* argv[]) {
    // Ignore these signals
    setup_signal_handler(SIGINT);
    setup_signal_handler(SIGPIPE);

    Job* jobs = malloc(sizeof(Job));
    int jobLen = 0;
    
    while (1) {
        // Prompt the user and get the command arguments
        CommandArgs commArgs;
        bool commandInit = prompt_user(&commArgs);
        if (!commandInit) { //EOF received
            break;
        }

        if (commArgs.len < minArgs[commArgs.cmd]) {
            printf("%s\n", INSUFFICIENT_ARGS);
            clean_cmd_args(commArgs);
            continue;
        }

        if (commArgs.cmd == SPAWN) {
            Job job;
            memset(&job, 0, sizeof(Job));
            jobs[jobLen] = job;
            spawn(commArgs, jobLen, jobs); 
            jobLen++;
            jobs = realloc(jobs, sizeof(Job) * (jobLen + 1));
        } else if (!execute_command(commArgs, jobs, jobLen)) {
            // A major bug has occurred
            printf("Error: Unknown Command\n");
            clean_cmd_args(commArgs);
            break;
        }

        clean_cmd_args(commArgs);
    }

    cleanup(jobs, jobLen);
    free_jobs(jobs, jobLen);
    return 0;
}
        
