#ifndef SHELL_BUILTINS
#define SHELL_BUILTINS

// SHELL BUILTINS: Implemented using user and system level functions only!
/**
 * @brief For each shell run, the background processors are checked and signaled
 */
void* b_background_poll(void* arg);

/**
 * @brief The ususal `cat` program.
 *
 * If `files` arg is provided, concatenate these files and print to stdout
 * If `files` arg is *not* provided, read from stdin and print back to stdout
 *
 * Example Usage: cat f1 f2 (concatenates f1 and f2 and print to stdout)
 * Example Usage: cat f1 f2 < f3 (concatenates f1 and f2 and prints to stdout,
 * ignores f3) Example Usage: cat < f3 (concatenates f3, prints to stdout)
 */
void* b_cat(void* arg);

/**
 * @brief Sleep for `n` seconds.
 *
 * Note that you'll have to convert the number of seconds to the correct number
 * of ticks.
 *
 * Example Usage: sleep 10
 */
void* b_sleep(void* arg);

/**
 * @brief Busy wait indefinitely.
 * It can only be interrupted via signals.
 *
 * Example Usage: busy
 */
void* b_busy(void* arg);

/**
 * @brief Echo back an input string.
 *
 * Example Usage: echo Hello World
 */
void* b_echo(void* arg);

/**
 * @brief Lists all files in the working directory.
 * For extra credit, it should support relative and absolute file paths.
 *
 * Example Usage: ls (regular credit)
 * Example Usage: ls ../../foo/./bar/sample (only for EC)
 */
void* b_ls(void* arg);

/**
 * @brief For each file, create an empty file if it doesn't exist, else update
 * its timestamp.
 *
 * Example Usage: touch f1 f2 f3 f4 f5
 */
void* b_touch(void* arg);

/**
 * @brief Rename a file. If the `dst_file` file already exists, overwrite it.
 *
 * Print appropriate error message if:
 * - `src_file` is not a file that exists
 * - `src_file` does not have read permissions
 * - `dst_file` file already exists but does not have write permissions
 *
 * Example Usage: mv src_file dst_file
 */
void* b_mv(void* arg);

/**
 * Copy a file. If the `dst_file` file already exists, overwrite it.
 *
 * Print appropriate error message if:
 * - `src_file` is not a file that exists
 * - `src_file` does not have read permissions
 * - `dst_file` file already exists but does not have write permissions
 *
 * Example Usage: cp src_file dst_file
 */
void* b_cp(void* arg);

/**
 * @brief Remove a list of files.
 * Treat each file in the list as a separate transaction. (i.e. if removing
 * file1 fails, still attempt to remove file2, file3, etc.)
 *
 * Print appropriate error message if:
 * - `file` is not a file that exists
 *
 * Example Usage: rm f1 f2 f3 f4 f5
 */
void* b_rm(void* arg);

/**
 * @brief Change permissions of a file.
 * There's no need to error if a permission being added already exists, or
 * if a permission being removed is already not granted.
 *
 * Print appropriate error message if:
 * - `file` is not a file that exists
 * - `perms` is invalid
 *
 * Example Usage: chmod +x file (adds executable permission to file)
 * Example Usage: chmod +rw file (adds read + write permissions to file)
 * Example Usage: chmod -wx file (removes write + executable permissions from
 * file)
 */
void* b_chmod(void* arg);

/**
 * @brief List all processes on PennOS, displaying PID, PPID, priority, status,
 * and command name.
 *
 * Example Usage: ps
 */
void* b_ps(void* arg);

/**
 * @brief Sends a specified signal to a list of processes.
 * If a signal name is not specified, default to "term".
 * Valid signals are -term, -stop, and -cont.
 *
 * Example Usage: kill 1 2 3 (sends term to processes 1, 2, and 3)
 * Example Usage: kill -term 1 2 (sends term to processes 1 and 2)
 * Example Usage: kill -stop 1 2 (sends stop to processes 1 and 2)
 * Example Usage: kill -cont 1 (sends cont to process 1)
 */
void* b_kill(void* arg);

// SHELL BUILTINS THAT DON'T SPAWN PROCESSES

/**
 * @brief Spawn a new process for `command` and set its priority to
 * `priority`.
 * 2. Adjust the priority level of an existing process.
 *
 * Example Usage: nice 2 cat f1 f2 f3 (spawns cat with priority 2)
 */
void* b_nice(void* arg);

/**
 * @brief Adjust the priority level of an existing process.
 *
 * Example Usage: nice_pid 0 123 (sets priority 0 to PID 123)
 */
void* b_nice_pid(void* arg);

/**
 * @brief Lists all available commands.
 *
 * Example Usage: man
 */
void* b_man(void* arg);

/**
 * @brief Resumes the most recently stopped job in the background, or the job
 * specified by job_id.
 *
 * Example Usage: bg
 * Example Usage: bg 2 (job_id is 2)
 */
void* b_bg(void* arg);

/**
 * @brief Brings the most recently stopped or background job to the foreground,
 * or the job specified by job_id.
 *
 * Example Usage: fg
 * Example Usage: fg 2 (job_id is 2)
 */
void* b_fg(void* arg);

/**
 * @brief Lists all jobs.
 *
 * Example Usage: jobs
 */
void* b_jobs(void* arg);

/**
 * @brief Exits the shell and shutsdown PennOS.
 *
 * Example Usage: logout
 */
void* b_logout(void* arg);

// SHELL BUILTINS TO TEST ZOMBIE + ORPHANS
/**
 * @brief Used to test zombifying functionality of your kernel.
 *
 * Example Usage: zombify
 */
void* b_zombify(void* arg);

/**
 * @brief Helper for zombify.
 */
void* b_zombie_child(void* arg);

/**
 * @brief Used to test orphanifying functionality of your kernel.
 *
 * Example Usage: orphanify
 */
void* b_orphanify(void* arg);

/**
 * @brief Helper for orphanify.
 */
void* b_orphan_child(void* arg);

#endif