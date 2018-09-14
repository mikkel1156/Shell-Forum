#include <pwd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#define FORUM_DIR "/home/ShellForum/Forum"

int num_of_files(const char* path) {
    DIR *dir;
    int counter = 0;
    struct dirent *de;

    dir = opendir(path);
    if (dir == NULL) {
        printf("%s:\n", path);
		fprintf(stderr, "server - num_of_files: could not open directory\n");
		return -1;
	}

    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0 || de->d_type != DT_DIR)
            continue;

        counter++;

        char *filepath = de->d_name;
        char *newDir = malloc(strlen(path) + strlen(filepath) + 2);
        if (newDir != NULL) {
            strcpy(newDir, path);
            strcat(newDir, "/");
            strcat(newDir, filepath);
        }

        counter += num_of_files(newDir);
    }

    closedir(dir);
    return counter;
}

int read_directory(char **files, const char* directory, int *iterator) {
    DIR *dir = opendir(directory);
	if (dir == NULL) {
		fprintf(stderr, "shell: read_directory() could not open current directory\n");
		return -1;
	}

    int i = 0;
    if (iterator)
        i = *iterator;

    struct dirent *de;
	while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char *filename = de->d_name;
        char *path = malloc(strlen(directory) + strlen(filename) + 2);
        if (path != NULL) {
            strcpy(path, directory);
            strcat(path, "/");
            strcat(path, filename);
        } else {
            continue;
        }

        files[i] = malloc((strlen(path)+1) * sizeof(char));
        strcpy(files[i], path);

        i++;
        if (iterator)
            (*iterator)++;

        if (de->d_type == DT_DIR) {
            read_directory(files, path, &i);
        }
    }

    closedir(dir);
	return 0;
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void doprocessing(int sock) {
    char buffer[256];
    ssize_t bytes_read = 0;
    while((bytes_read = read(sock, buffer, sizeof(buffer))) > 0) {
        if (strncmp(buffer, "Action: list", 12) == 0) {
            printf("Action: list\n");

            char **subforums = NULL;
            int count = num_of_files(FORUM_DIR);
            subforums = malloc(count * sizeof(char*));
            read_directory(subforums, FORUM_DIR, NULL);

            write(sock, "Action: list", 12);
            for (int i = 0; i < count; i++) {
                char *subforum = malloc((strlen(subforums[i])-strlen(FORUM_DIR)) + 1);
                int lenForumDir = strlen(FORUM_DIR);
                strncpy(subforum, &subforums[i][lenForumDir+1], strlen(subforums[i])-lenForumDir);
                printf("Subforum (%i): %s\n", strlen(subforum), subforum);

                write(sock, subforum, strlen(subforum));
            }
            break;
        } else {
            printf("--Recieved--\n%s\n-------\n", buffer);
            char *line;
            line = strtok(buffer, "\t\r\n\a");

            char *user;

            FILE *file;
            char *filepath;
            char *fileData = malloc(sizeof(char));

            int i = 0;
            while (line != NULL) {
                if (i == 1) {
                    filepath = &line[10];
                } else if (i == 2) {
                    user = &line[6];
                } else if (i == 3) {
                    char *forum = &line[7];

                    char *path = malloc(strlen(FORUM_DIR) + strlen(forum) + strlen(filepath) + 3);
                    if (path != NULL) {
                        strcpy(path, FORUM_DIR);
                        strcat(path, "/");
                        strcat(path, forum);
                        strcat(path, "/");
                        if( access(path, F_OK) != -1 && is_regular_file(path) == 0) {
                            strcat(path, filepath);
                            filepath = malloc(strlen(path) + 1);
                            strcpy(filepath, path);
                        } else {
                            write(sock, "Error: The supplied forum does not exist.", 41);
                            close(sock);
                            break;
                        }
                    }
                } else if (i > 3) {
                    //  Strip the 'Data: ' bit.
                    if (i == 4)
                        line = &line[6];

                    char *lineData = malloc(strlen(line) + 2);
                    strcpy(lineData, line);
                    strcat(lineData, "\n");

                    fileData = realloc(fileData, strlen(fileData)+strlen(lineData) + 1);
                    strcat(fileData, lineData);
                }

                i++;
                line = strtok(NULL, "\t\r\n\a");
            }

            file = fopen(filepath, "w+");
            if (file != NULL) {
                fputs(fileData, file);
            } else {
                write(sock, "Error: Could not write to the file.", 35);
                close(sock);
                break;
            }
            fclose(file);

            uid_t uid = getpwnam(user)->pw_uid;
            gid_t gid = getpwnam(getenv("USER"))->pw_gid;
            printf("UID: %i\nGID: %i\n", uid, gid);
            chown(filepath, uid, gid);
        }
        break;
    }
    if(bytes_read == -1)
        printf("Could not read any data.\n");

    close(sock);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n, pid;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 2155;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here
     * process will go in sleep mode and will wait
     * for the incoming connection
     */

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        /* Create child process */
        pid = fork();

        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0) {
            //  Child process.
            close(sockfd);
            doprocessing(newsockfd);
            exit(0);
        } else {
            close(newsockfd);
        }

    }
}