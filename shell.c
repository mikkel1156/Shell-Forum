/*
    Based on Stephen Brennan's lsh shell code (https://github.com/brenns10/lsh).
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <ncurses.h>
#include <dirent.h>
#include <regex.h>
#include <sys/socket.h>

#define arrlen(x)  (sizeof(x) / sizeof((x)[0]))

/*
    Function Declarations for built_in shell commands.
*/
int shell_cd(char **args);
int shell_help(char **args);
int shell_exec(char **args);
int shell_exit(char **args);
int shell_search(char **args);
int shell_commit(char **args);

//  Declare other functions.
void server_send_data(char *data);
int num_of_files(const char* path);
int read_directory(char **files, const char* directory, int *iterator);


/*
    List of built_in commands, followed by their corresponding functions.
*/
char *built_in_str[] = {
  "cd",
  "help",
  "exec",
  "exit",
  "search",
  "commit"
};

int (*built_in_func[]) (char **) = {
  &shell_cd,
  &shell_help,
  &shell_exec,
  &shell_exit,
  &shell_search,
  &shell_commit
};

/*
    List of all the allowed commands.
*/
char *allowed_cmds[] = {
  "vim",
  "nano",
  "cat",
  "mkdir",
  "mv",
  "rm",
  "ls",
  "clear",
  "man",
  "echo",
  "tmux"
};

/*
    built_in function implementations.
*/

/**
    @brief Bultin command: change directory.
    @param args List of args.  args[0] is "cd".  args[1] is the directory.
    @return Always returns 1, to continue executing.
*/
int shell_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("shell");
        }
    }
    return 1;
}

/**
    @brief built_in command: print help.
    @param args List of args.  Not examined.
    @return Always returns 1, to continue executing.
*/
int shell_help(char **args) {
    printw("-[Shell]-\n");
    printw("Type program names and arguments, and hit enter.\n");
    printw("The following are built-in:\n");

    int i;
    for (i = 0; i < arrlen(built_in_str); i++) {
        printw("   %s\n", built_in_str[i]);
    }

    printw("Use the man command for information on other programs.\n\n");

    printw("These are the commands you are allowed to use:\n");

    for (i = 0; i < arrlen(allowed_cmds); i++) {
        printw("   %s\n", allowed_cmds[i]);
    }
    return 1;
}

//  NOTE: Likely removal.
int shell_exec(char **args) {
    if (args[1] == NULL) {
        printw("Nothing was specified to execute.\n");
        return 1;
    }

    if (strcmp(args[1], "") == 0) {
        //  Nothing.
    } else {
        printw("Nothing to execute.\n");
    }

    return 1;
}

/**
    @brief built_in command: exit.
    @param args List of args.  Not examined.
    @return Always returns 0 to terminate execution.
*/
int shell_exit(char **args) {
    return 0;
}

void shell_search_menu(char **posts) {
    //  Clear screen before making the menu.
    clear();

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    WINDOW * menuWin = newwin(yMax-5, xMax-12, yMax-(yMax%100*0.9415), 5);
    box(menuWin, 0, 0);
    refresh();
    wrefresh(menuWin);

    keypad(menuWin, true);

    server_send_data(posts[0]);

    //const *choices[3] = {"Test 1", "Test 2", "Test 3"};
    int choice;
    int ch;
    while ((ch = getch()) != 'q') {
        //  TODO: Print number of posts relating to the height of the screen.

    }
    clear();
}

int shell_search(char **args) {
    //  Get all the files in the forum.
    char **allFiles = NULL;
    int fCount = num_of_files("/home/ShellForum/Forum");
    allFiles = malloc(fCount * sizeof(char*));
    read_directory(allFiles, "/home/ShellForum/Forum", NULL);

    int rCount = 0;
    char **results = NULL;

    for (int i = 0; i < fCount; i++) {
        int o = 0;
        char path[1024];
        strcpy(path, allFiles[i]);

        for (int ii = 0; ii < strlen(allFiles[i]); ii++) {
            if (allFiles[i][ii] == '/') {
                o++;
            }
        }

        char *token = strtok(allFiles[i], "/");
        for (int ii = 0; ii < o-1; ii++) {
            token = strtok(NULL, "/");
        }

        int ii = 1;
        while (args[ii] != NULL) {
            //  Compile regular expression.
            regex_t regex;
            int reti = regcomp(&regex, args[ii], 0);

            //  Check if there is a match.
            reti = regexec(&regex, token, 0, NULL, 0);
            if (!reti) {
                //  Allocate memory enough for all the results.
                results = malloc(rCount * sizeof(char*));

                //  Copy the path over to the list of results.
                results[rCount] = malloc((strlen(path)+1) * sizeof(char*));
                strcpy(results[rCount], path);
                rCount++;
            }
            ii++;
        }
    }
    printw("%i\n", rCount);
    shell_search_menu(allFiles);

    return 1;
}

int shell_commit(char **args) {
    char data[1024];
    memset(data, '\0', sizeof(data));

    char buffer[256];
    if (!args[1]) {
        printw("No forum or file to commit was chosen.\n");
        printw("Use ");
        attron(A_ITALIC);
        printw("commit ls");
        attroff(A_ITALIC);
        printw(" to list all the forums.\n");
        return 1;
    } else if (strcmp(args[1], "ls") != 0) {
        printw("No forum to commit to was chosen.\nUse ");
        attron(A_ITALIC);
        printw("commit ls");
        attroff(A_ITALIC);
        printw(" to list all the forums.\n");
        return 1;
    } else if (!args[2]) {
        printw("No file was chosen to commit.\n");
    }

    if (strcmp(args[1], "ls") == 0) {
        strncpy(data, "Action: list", sizeof(data));
        printw("%s\n", data);
        server_send_data(data);

        return 1;
    }

    FILE *file = fopen(args[2], "r");
    if (file) {
        //  Type of action.
        strcat(data, "Action: upload\t\r\n\a");

        //  Name of file user is commiting.
        strcat(data, "Filename: ");
        strcat(data, args[2]);
        strcat(data, "\t\r\n\a");

        //  Name of user that is commiting.
        strcat(data, "User: ");
        strcat(data, getenv("USER"));
        strcat(data, "\t\r\n\a");

        //  The category/forum to commit to.
        strcat(data, "Forum: ");
        strcat(data, args[1]);
        strcat(data, "\t\r\n\a");

        //  The actual data of the post.
        strcat(data, "Data: ");
        char line[1024];
        while (fgets(line, sizeof(line), file)) {
            strcat(data, line);
        }
        strcat(data, "\t\r\n\a");
        fclose(file);

        server_send_data(data);
    } else {
        printw("No such file.\n");
        return 1;
    }

    return 1;
}

/**
    @brief Launch a program and wait for it to terminate.
    @param args Null terminated list of arguments (including program).
    @return Always returns 1, to continue execution.
*/
int shell_launch(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        //  Child process.

        //  Check if user is trying to run an allowed program.
        for (int i = 0; i < arrlen(allowed_cmds); i++) {
            if (strcmp(allowed_cmds[i], args[0]) == 0) {
                if (execvp(args[0], args) == -1) {
                    perror("shell");
                }
            }
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        //  Error forking
        perror("shell");
    } else {
        //  Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

/**
    @brief Execute shell built-in or launch program.
    @param args Null terminated list of arguments.
    @return 1 if the shell should continue running, 0 if it should terminate
*/
int shell_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < arrlen(built_in_func); i++) {
        if (strcmp(args[0], built_in_str[i]) == 0) {
            return (*built_in_func[i])(args);
        }
    }

    return shell_launch(args);
}

void server_send_data(char *data) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    //char *buffer = malloc(256);

    portno = 2155;

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        return;
    }

    server = gethostbyname("127.0.0.1");

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        return;
    }

    write(sockfd, data, strlen(data));

    /* Now read server response */
    int flag;
    /*
        FLAGS:
        - -1: Error
        - 0: List response
        - 1: Upload response
    */
    char buffer[256];
    bzero(buffer, sizeof(buffer));
    while (recv(sockfd, buffer, sizeof(buffer), 0) != 0) {
        if (strncmp(buffer, "Action: list", 12) == 0) {
            flag = 0;
            printw("Avaiable forums:\n");
            bzero(buffer, sizeof(buffer));
            continue;
        } else if (strncmp(buffer, "Error: ", 7) == 0 ) {
            flag = -1;
            printw("%s\n", &buffer[7]);
            break;
        }

        if (flag == 0) {
            printw("- %s\n", buffer);
        }

        bzero(buffer, sizeof(buffer));
    }

    //printw("%s\n", buffer);
    close(sockfd);
}

int num_of_files(const char* path) {
    DIR *dir;
    int counter = 0;
    struct dirent *de;

    dir = opendir(path);
    if (dir == NULL) {
		fprintf(stderr, "shell: could not open current directory\n");
		return -1;
	}

    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        counter++;
        if (de->d_type == DT_DIR) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                perror("shell: getcwd() error");
                return -1;
            }
            char *filename = de->d_name;
            char *newDir = malloc(strlen(cwd) + strlen(filename) + 2);
            if (newDir != NULL) {
                strcpy(newDir, cwd);
                strcat(newDir, "/");
                strcat(newDir, filename);
            }

            counter += num_of_files(newDir);
        }
    }

    closedir(dir);
    return counter;
}

int read_directory(char **files, const char* directory, int *iterator) {
    DIR *dir = opendir(directory);
	if (dir == NULL) {
		fprintf(stderr, "shell: could not open current directory\n");
		return -1;
	}

    int i = 0;
    if (iterator)
        i = *iterator+1;

    struct dirent *de;
	while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("shell: getcwd() error");
            return -1;
        }
        char *filename = de->d_name;
        char *path = malloc(strlen(cwd) + strlen(filename) + 2);
        if (path != NULL) {
            strcpy(path, cwd);
            strcat(path, "/");
            strcat(path, filename);
        } else {
            continue;
        }

        files[i] = malloc((strlen(path)+1) * sizeof(char));
        strcpy(files[i], path);
        i++;

        if (de->d_type == DT_DIR) {
            read_directory(files, path, &i);
        }
    }

    closedir(dir);
	return 0;
}

void lineClearTo(int start, int end) {
    int x, y;
    getyx(stdscr, y, x);

    for (int i = end; i > start; i--) {
        mvdelch(y, i);
    }
}

#define SHELL_RL_BUFSIZE 1024
/**
    @brief Read a line of input from stdin.
    @return The line from stdin.
*/
char *shell_read_line(int lenPrompt) {
    int ch;

    int bufsize = SHELL_RL_BUFSIZE;
    int position = 0;
    int lenBuffer = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    if (!buffer) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *home = getenv("HOME");
    char *hFilename = "/.history";
    char *hPath = malloc(strlen(home) + strlen(hFilename) + 1);
    if (hPath != NULL) {
        strcpy(hPath, home);
        strcat(hPath, hFilename);
    }

    int hPosition = 0;
    char **historyBuffer = NULL;
    unsigned long hbCount = 0;
    FILE *hFp = fopen(hPath, "r");
    if (hFp) {
        char line[1024];

        while (fgets(line, sizeof(line), hFp)) {
            historyBuffer = (char **)realloc(historyBuffer, (hbCount+1) * sizeof(char *));
            line[strlen(line)-1] = 0;
            historyBuffer[hbCount] = strdup(line);
            hbCount++;
        }
        fclose(hFp);
    }

    while ((ch = getch()) && ch != 'Q') {
        int x, y;
        int hidePress = 0;

        //  Special keys for navigation on the line.
        if (ch == KEY_UP) {
            getyx(stdscr, y, x);

            if (hbCount > 0 && hPosition < hbCount) {
                if (position > 0) {
                    lineClearTo(x-position-1, x);
                }
                printw(historyBuffer[hPosition]);
                buffer = historyBuffer[hPosition];
                position = strlen(historyBuffer[hPosition]);
                hPosition++;
            }
            hidePress = 1;
        } else if (ch == KEY_DOWN) {
            getyx(stdscr, y, x);

            if (hbCount > 0 && hPosition > 0) {
                if (position > 0) {
                    lineClearTo(x-position-1, x);
                }
                hPosition -= 1;
                printw(historyBuffer[hPosition]);
                buffer = historyBuffer[hPosition];
                position = strlen(historyBuffer[hPosition]);
            }
            hidePress = 1;
        } else if (ch == KEY_LEFT) {
            getyx(stdscr, y, x);
            if (x != lenPrompt) {
                move(y, x-1);
                refresh();
                position--;
            }
            hidePress = 1;
        } else if (ch == KEY_RIGHT) {
            getyx(stdscr, y, x);
            if (x != lenPrompt+lenBuffer) {
                move(y, x+1);
                refresh();
                position++;
            }
            hidePress = 1;
        }  else if (ch == KEY_HOME) {
            getyx(stdscr, y, x);
            if (x != lenPrompt) {
                move(y, lenPrompt);
                position = 0;
            }
            hidePress = 1;
        } else if (ch == KEY_END) {
            getyx(stdscr, y, x);
            if (x != lenPrompt+lenBuffer) {
                move(y, lenPrompt+lenBuffer);
                position = lenBuffer;
            }
            hidePress = 1;
        }

        //  Special keys for editing the line.
        if (ch == KEY_BACKSPACE) {
            getyx(stdscr, y, x);
            if (x != lenPrompt) {
                mvdelch(y, x-1);

                position--;
                buffer[position] = 0;
            }
            hidePress = 1;
        } else if (ch == KEY_DC) {
            getyx(stdscr, y, x);
            if (x != lenBuffer) {
                mvdelch(y, x);

                position--;
                buffer[position] = 0;
            }
            hidePress = 1;
        }


        if (ch == EOF) {
            exit(EXIT_SUCCESS);
        } else if (ch == '\n') {
            buffer[lenBuffer] = '\0';

            //  Go to a new line after user input.
            printw("\n");
            refresh();

            //  Return what was typed.
            return buffer;
        } else {
            if (!hidePress) {
                if (position == lenBuffer) {
                    buffer[position] = ch;
                    printw("%c", ch);
                    position++;
                    lenBuffer++;
                } else {
                    char *temp = malloc((lenBuffer) + 3);
                    memset(temp, '\0', sizeof(temp));

                    getyx(stdscr, y, x);
                    lineClearTo(x-position-1, x);

                    position++;
                    lenBuffer++;

                    strcpy(&temp[0], buffer);
                    temp[position-1] = ch;

                    for(int i = position-1; i < lenBuffer-1; i++) {
                        temp[i+1] = buffer[i];
                    }
                    temp = realloc(temp, strlen(temp));
                    //temp[strlen(temp)+1] = '\0';
                    //printw("\n|%s(%i)|\n", temp, strlen(temp));

                    for (int i = lenBuffer; i < strlen(temp); i++) {
                        //printw("\n|%c\n", temp[i]);
                        temp[i] = 0;
                    }
                    //temp[lenBuffer+2] = '\0';

                    printw("%s", temp);
                    move(y, x+1);

                    buffer = temp;
                }
                refresh();
            }
        }

        //  If we have exceeded the buffer, reallocate.
        if (lenBuffer >= bufsize) {
            bufsize += SHELL_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"
/**
    @brief Split a line into tokens (very naively).
    @param line The line.
    @return Null-terminated array of tokens.
*/
char **shell_split_line(char *line) {
    int bufsize = SHELL_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SHELL_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += SHELL_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SHELL_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void shell_save_command(char *cmd) {
    char *home = getenv("HOME");
    char *filename = "/.history";
    char *path = malloc(strlen(home) + strlen(filename) + 1);
    if (path != NULL) {
        strcpy(path, home);
        strcat(path, filename);
    }
    FILE *fp = fopen(path, "a");
    fprintf(fp, "%s\n", cmd);
    refresh();
    fclose(fp);
    free(path);
}

//  NOTE: Likely removal if no other use for it.
int str_starts_with(const char *restrict string, const char *restrict prefix) {
    while(*prefix) {
        if(*prefix++ != *string++)
            return 0;
    }

    return 1;
}

char *str_replace(char *str, char *orig, char *rep, int start) {
    static char temp[4096];
    static char buffer[4096];
    char *p;

    strcpy(temp, str + start);

    if(!(p = strstr(temp, orig)))   //  Is 'orig' even in 'temp'?
        return temp;

    strncpy(buffer, temp, p-temp);  //  Copy characters from 'temp' start to 'orig' str.
    buffer[p-temp] = '\0';

    sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
    sprintf(str + start, "%s", buffer);

    return str;
}

int num_of_char(char *str, char ch) {
    int counter = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ch)
            counter++;
    }

    return counter;
}

void shell_get_cwd(char *pCWD) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        perror("shell: getcwd() error");

    int slashCount = num_of_char(getenv("HOME"), '/');
    char *temp = malloc(strlen(cwd)+1);
    strncpy(temp, cwd, strlen(cwd));

    char *dirUser = strtok(temp, "/");

    for (int i = 0; i < slashCount-1; i++) {
        if (dirUser != NULL)
            dirUser = strtok(NULL, "/");
    }

    int isUsersDir = 0;
    if (dirUser != NULL && strlen(dirUser) == strlen(getenv("USER"))) {
        if (strncmp(dirUser, getenv("USER"), strlen(dirUser)))
            isUsersDir = 1;
    }

    if (isUsersDir) {
        strncpy(pCWD, str_replace(cwd, getenv("HOME"), "~", 0), 1024);
    } else {
        strncpy(pCWD, cwd, 1024);
    }
}

void shell_display_motd(void) {
    FILE *file;
    //  Read MOTD from the parent directory.
    if (file = fopen("../motd", "r")) {
        char buffer[1024];

        //  Read the contents of the file.
        while (fgets(buffer, sizeof buffer, file) != NULL) {
            printw("%s", buffer);
        }

        //  Display any errors.
        if (!feof(file)) {
            perror("shell: fgets() error");
        }
        fclose(file);
    } else {
        printw("No MOTD to display.\n");
    }
}

/**
    @brief Loop getting input and executing it.
*/
void shell_loop(void) {
    char *line;
    char **args;
    int status;
    char cwd[1024];

    //execvp("/usr/bin/clear", NULL);
    do {
        //  Get current working directory.
        shell_get_cwd(cwd);

        //  Display prompt with colour.
        attron(COLOR_PAIR(2));
        printw(getenv("USER"));
        attron(COLOR_PAIR(1));
        printw(":");
        attron(COLOR_PAIR(3));
        printw(cwd);
        attron(COLOR_PAIR(1));
        printw("$ ");
        refresh();

        //  Length of the prompt.
        int lenPrompt = strlen(getenv("USER")) + strlen(cwd) + 3;

        line = shell_read_line(lenPrompt);

        printw("LINE: %s\n", line);
        refresh();

        args = shell_split_line(line);
        status = shell_execute(args);

        //  Add entered command into history.
        shell_save_command(line);

        free(line);
        free(args);
    } while (status);
}

/**
    @brief Main entry point.
    @param argc Argument count.
    @param argv Argument vector.
    @return status code
*/
int main(int argc, char **argv) {
    /*  Curses Initialisations  */
    initscr();
    start_color();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    //  Define colours.
    init_color(COLOR_BLUE, 0, 400, 740);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);

    shell_loop();

    printw("\nExiting Now\n");
    endwin();

    return EXIT_SUCCESS;
}