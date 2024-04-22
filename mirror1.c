#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define PORT 6301
#define BUFFER_SIZE 1024
int found_flag = 0;     // Global flag to indicate file is found
int filterType;         // Initialize filterType as 0 for filtering by size
long long size1, size2; // Global variables for file size range
time_t date, dateA;

//THe function is created for sorting and comparison of two strings,
int compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}
//The structure for the Directory entry is created which includes the name of the directory and the creation time
struct Directory
{
    char name[256];
    time_t creation_time;
};
//Te function is created for sorting and comparison of Directories based on its creation time,
int compare_two(const void *a, const void *b)
{
    return ((struct Directory *)a)->creation_time - ((struct Directory *)b)->creation_time;
}
//Function is created in which the directory tree is recursively traversed beginning from the specified path,listing all the files and copying it into temprary directory.
void listFiles(const char *path, const char *tempPath, int client_socket, long long size1, long long size2, time_t date, time_t dateA)
{
    char message[BUFFER_SIZE];//Error message storage
    DIR *dir;
    struct dirent *entry;//Structure for Directory entry
    struct stat fileStat;//Structure for status of file
    //Directory is being opened using 'opendir'
    if (!(dir = opendir(path)))
    {    //Sending the error message to the client if the directory dosen't open
        sprintf(message, "Cannot open directory '%s'\n", path, strerror(errno));
        send(client_socket, message, strlen(message), 0);
        return;
    }
    //Each entry in the directory is being iterated
    while ((entry = readdir(dir)) != NULL)
    {
        char filePath[1024];//Storing full path of a file or directory 
        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);
        //neglecting . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
         //neglecting the temp directory
        if (strcmp(filePath, tempPath) == 0) 
            continue;
          //Fetching the status of the file
        if (stat(filePath, &fileStat) < 0)
        {
            continue;
        }
        //Call the listFiles on the entry recursively in case it is a directory.
        if (S_ISDIR(fileStat.st_mode))
        {
            listFiles(filePath, tempPath, client_socket, size1, size2, date, dateA); // Recursive call for directories
        }
        // If the entry is a regular file, verify if it satisfies the size or date filtering criteria.
        else
        {
            if ((size1 != -1 && size2 != -1 && fileStat.st_size >= size1 && fileStat.st_size <= size2) ||
                (date != -1 && fileStat.st_mtime <= date) || (dateA != -1 && fileStat.st_mtime >= dateA))
            {
                char cpCommand[2048];//Copying the files to temporary directory
                snprintf(cpCommand, sizeof(cpCommand), "cp -p \"%s\" \"%s/\"", filePath, tempPath);
                if (system(cpCommand) != 0)
                {
                    sprintf(message, "Error copying file '%s' to temp directory\n", filePath, strerror(errno));
                    continue;
                }
            }
        }
    }//Directory is being closed
    closedir(dir);
}
//Function is created for creating the temporary directory, stores it with files according to filtering criteria and compresses the contents into a tar file
void createTempDirectory(const char *cwd, int client_socket, int filterType, long long size1, long long size2, time_t date, time_t dateA)
{
    char message[BUFFER_SIZE];//Storing of status message

    char tempDir[1024];//Storing the temporary directory path
    snprintf(tempDir, sizeof(tempDir), "%s/temp", cwd);

    struct stat st;
    //To verify if a file or directory having the same name as the temporary directory already exists.
    if (stat(tempDir, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            // Directory exists, remove it and its contents
            char rmCommand[2048];
            //Removing the directory if already exists using "rm -rf"
            snprintf(rmCommand, sizeof(rmCommand), "rm -rf \"%s\"", tempDir);
            if (system(rmCommand) != 0)
            {
                sprintf(message, "Error removing existing directory '%s': %s\n", tempDir, strerror(errno));
                exit(1);
            }
            sprintf(message, "Existing directory '%s' removed successfully.\n", tempDir);
            
        }
        else
        {
            // File already exists with the same name, cannot proceed futher
            sprintf(message, "Error: A file with the same name as the temp directory already exists.\n", tempDir);
            
            exit(1);
        }
    }

    // Creation of temporary directory
    if (mkdir(tempDir, 0777) != 0)
    {
        sprintf(message, "Error creating directory '%s': %s\n", tempDir, strerror(errno));
        exit(1);
    }

    sprintf(message, "Directory 'temp' created successfully.\n");
    

    // Files are listed and copied based on the filter type, to the newly created temporary directory
    if (filterType == 0)
    {
        listFiles(cwd, tempDir, client_socket, size1, size2, -1, -1);
    }
    else if (filterType == 1)
    {
        listFiles(cwd, tempDir, client_socket, -1, -1, date, -1);
    }
    else if (filterType == 2)
    {
        listFiles(cwd, tempDir, client_socket, -1, -1, -1, dateA);
    }
    else
    {   //Program is exited in case of incorrect value
        sprintf(message, "Invalid filter type.\n");
        
        exit(1);
    }

    // Change directory to 'temp'
    if (chdir(tempDir) != 0)
    {
        sprintf(message, "Error changing directory to '%s': %s\n", tempDir, strerror(errno));
        send(client_socket, message, strlen(message), 0);
        exit(1);
    }

    // Creating the tar file by executing the tar command
    char tarCommand[2048];
    snprintf(tarCommand, sizeof(tarCommand), "tar -czf ~/w24project/temp.tar.gz .");
    if (system(tarCommand) != 0)
    {
        sprintf(message, "Error creating tar file: %s\n", strerror(errno));
        send(client_socket, message, strlen(message), 0);
        exit(1);
    }


    sprintf(message, "Tar file 'temp.tar.gz' created successfully in the home directory.\n");
    send(client_socket, message, strlen(message), 0);

    // moving back to the orignal directory
    if (chdir(cwd) != 0)
    {
        sprintf(message, "Error changing directory to '%s': %s\n", cwd, strerror(errno));
        send(client_socket, message, strlen(message), 0);
        exit(1);
    }
}
//The function created for retrieving information of a given file
void print_file_info(const char *filename, int client_socket)
{   //Structure for storing information of file
    struct stat file_stat;
    if (stat(filename, &file_stat) == -1)
    {    //Sending the error message if file is not located
        char error_message[] = "Error: File not found.\n";
        send(client_socket, error_message, strlen(error_message), 0);
        return;
    }
    //The file information string is being formatted
    char file_info[BUFFER_SIZE];//Storing of information of file
    //file permissions is being added to the  file file information string
    snprintf(file_info, BUFFER_SIZE, "File: %s\nSize: %ld bytes\nDate created: %s\nPermissions: ",
             filename, (long)file_stat.st_size, ctime(&file_stat.st_ctime));
    strcat(file_info, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    strcat(file_info, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
    strcat(file_info, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
    strcat(file_info, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
    strcat(file_info, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
    strcat(file_info, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
    strcat(file_info, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
    strcat(file_info, (file_stat.st_mode & S_IROTH) ? "r" : "-");
    strcat(file_info, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
    strcat(file_info, (file_stat.st_mode & S_IXOTH) ? "x\n" : "-\n");
    //Sending the information of the file to the client
    send(client_socket, file_info, strlen(file_info), 0);
}
/*Function is created in which the directory tree is recursively traversed beginning from the specified path
searching all the files with a given name inside the directories and subdirectories*/
int search_file(const char *dir_name, const char *filename, int client_socket)
{
    DIR *dir;
    struct dirent *entry;//Structure for Directory entry
    //Directory is being opened using 'opendir'
    if (!(dir = opendir(dir_name)))
    {
        perror("Error");
        exit(EXIT_FAILURE);
    }
    //Each entry in the directory is being iterated
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {   //neglecting . and .. directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char path[1024];
            //Full path of subdirectory is being created
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
            //File is being recursively searched inside subdirectory
            search_file(path, filename, client_socket);
        }
        else
        {   // If the entry is a regular file, Verify that the filename matches and that it hasn't been discovered yet.
            if (strcmp(entry->d_name, filename) == 0 && !found_flag)
            {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, entry->d_name);
                //Sending the information of the file to the client
                print_file_info(full_path, client_socket);
                found_flag = 1; // file located
            }
        }
    }
    closedir(dir);
    return found_flag;
}
//Function is created  which recursively searches through a directory and all of its subdirectories for files with a given extension.
void listFilesForExtension(const char *dirPath, const char **extensions, int numExtensions, const char *temp)
{   //Directory is being opened using 'opendir'
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }
    //Structure for Directory entry
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        { //if it is a regular file ,extracting the file extension
            char *fileExt = strrchr(entry->d_name, '.');
            if (fileExt != NULL)
            {  
                char extBuffer[16];
                strncpy(extBuffer, fileExt + 1, sizeof(extBuffer)); 
                extBuffer[sizeof(extBuffer) - 1] = '\0';            
                for (int i = 0; i < numExtensions; i++)
                {    //Verifying if file extension matches any of the given extension
                    if (strcmp(extBuffer, extensions[i]) == 0)
                    {
                        char srcPath[1024];
                        snprintf(srcPath, sizeof(srcPath), "%s/%s", dirPath, entry->d_name);
                        char destPath[1024];
                        snprintf(destPath, sizeof(destPath), "%s/%s", temp, entry->d_name);
                        FILE *srcFile = fopen(srcPath, "rb");//Source file opened for reading
                        if (srcFile == NULL)
                        {
                            perror("Unable to open source file");
                            exit(EXIT_FAILURE);
                        }//Destination file opened for writing
                        FILE *destFile = fopen(destPath, "wb");
                        if (destFile == NULL)
                        {
                            perror("Unable to open destination file");
                            fclose(srcFile);
                            exit(EXIT_FAILURE);
                        }
                        char buffer[4096];
                        size_t bytesRead;
                        //Write to the destination file and read from the source file.
                        while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0)
                        {
                            fwrite(buffer, 1, bytesRead, destFile);
                        }
                        fclose(srcFile);//Source file is being closed
                        fclose(destFile);//Destination file is being closed
                        break;
                    }
                }
            }
        }
        else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            // calling listFilesForExtension recursively for the subdirectory incase the entry is a Directory
            char newPath[1024];
            snprintf(newPath, sizeof(newPath), "%s/%s", dirPath, entry->d_name);
            listFilesForExtension(newPath, extensions, numExtensions, temp);
        }
    }
    //Directory is being closed
    closedir(dir);
}
//Function is created for handling and processing Incoming client requests. 
void crequest(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int bytes_received;
    //Data is being received from the client
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        //Verify that if the message received starts with "w24fn"
        if (strncmp(buffer, "w24fn ", 6) == 0)
        {   //obtain path to user home directory
            char *home = getenv("HOME");
            if (home == NULL)
            {
                perror("Error");
                return EXIT_FAILURE;
            }

            char dir_name[1024];
            //Directory path is being created
            snprintf(dir_name, sizeof(dir_name), "%s/", home);
             //filename is being extracted from the message
            char *filename = buffer + 6;
            // trailing newline character is removed if present
            if (filename[strlen(filename) - 1] == '\n')
                filename[strlen(filename) - 1] = '\0';

            found_flag = 0;
           // searching all the files with a given name inside the directory
            search_file(dir_name, filename, client_socket);

            //Resetting the flag
            if (found_flag == 0)
            {    //Sending the error message if file is not located
                char message[] = "No file found";
                if (write(client_socket, message, strlen(message)) < 0)
                {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
            }
        }
        //Verify that if the message received starts with "w24fz"
        else if (strncmp(buffer, "w24fz ", 6) == 0)
        {
            //the size boundary is extracted from the message
            if (sscanf(buffer + 6, "%ld %ld", &size1, &size2) == 2)
            {
                char message[BUFFER_SIZE];
                //Verify the size boundary
                if (size1 < 0 || size2 < 0 || size1 > size2)
                {
                    char *message = "Give proper size ";
                    send(client_socket, message, strlen(message), 0);
                }
                //obtain path to user home directory
                char *cwd = getenv("HOME");
                // Creation of path to temporary directory
                char tempDir[1024];
                snprintf(tempDir, sizeof(tempDir), "%s/temp", cwd);
                //filtertype set to 0 for size
                filterType = 0;
                //Create a temporary directory and copy files that meets the size criteria into it.
                createTempDirectory(cwd, client_socket, filterType, size1, size2, -1, -1);

            }
        }
        //Verify that if the message received starts with "w24fdb"
        else if (strncmp(buffer, "w24fdb ", 7) == 0)
        {    //this command filters files based on specific date

            char message[BUFFER_SIZE];
            //the date is extracted from the message
            char *date_str = buffer + 6;

            struct tm tm = {0}; //In Struct tm Initialize variables to all zeros
            //Using strptime to parse the date string into a struct tm.
            if (strptime(date_str, "%Y-%m-%d", &tm) == NULL)
            {
                printf("Invalid date format. Please provide date in the format YYYY-MM-DD.\n");
                return 1;
            }//parsed time converted intotime_t value using mktime
            date = mktime(&tm);
            //obtain path to user home directory
            char *cwd = getenv("HOME");
            if (cwd == NULL)
            {
                printf("HOME environment variable is not set.\n");
                return 1;
            }
            // Creation of path to temporary directory
            char tempDir[1024];
            snprintf(tempDir, sizeof(tempDir), "%s/temp", cwd);
            //filtertype set to 1 for date
            filterType = 1;
           // Create a temporary directory and copy files that have been modified on or before the specified date into it.
            createTempDirectory(cwd, client_socket, filterType, -1, -1, date, -1);
        }
        //Verify that if the message received starts with "w24fda"
        else if (strncmp(buffer, "w24fda ", 7) == 0)
        {
            //this command filters files based on specific date after the specified date
            char message[BUFFER_SIZE];
             //the date is extracted from the message
            char *date_str = buffer + 6;

            struct tm tm = {0}; //In Struct tm Initialize variables to all zeros
            //Using strptime to parse the date string into a struct tm.
            if (strptime(date_str, "%Y-%m-%d", &tm) == NULL)
            {
                printf("Invalid date format. Please provide date in the format YYYY-MM-DD.\n");
                return 1;
            }//parsed time converted intotime_t value using mktime
            dateA = mktime(&tm);
            //obtain path to user home directory
            char *cwd = getenv("HOME");
            if (cwd == NULL)
            {
                printf("HOME environment variable is not set.\n");
                return 1;
            } // Creation of path to temporary directory

            char tempDir[1024];
            snprintf(tempDir, sizeof(tempDir), "%s/temp", cwd);
            //filtertype set to 2 for dateA
            filterType = 2;
            // Create a temporary directory and copy files that have been modified on or after the specified date into it.
            createTempDirectory(cwd, client_socket, filterType, -1, -1, -1, dateA);
        }
        //Verify that if the message received starts with "w24ft"
        else if (strncmp(buffer, "w24ft ", 6) == 0)
        {    //this command filters files based on specified file extension
            char message[BUFFER_SIZE];

            char *args = buffer + 6;
            char *token = strtok(args, " ");

            if (token != NULL)
            {
                // Check the extension format
                if (token[0] != '\0' && token[0] != '-')
                {
                    const char *extensions[3];
                    int numExtensions = 0;
                    while (token != NULL && numExtensions < 3)
                    {   //Put every token for an extension into the extensions array.
                        extensions[numExtensions++] = token;
                        token = strtok(NULL, " ");
                    }
                    // Checking to ensure at least one extension is provided
                    if (numExtensions > 0)
                    {   // Creation of  temporary directory
                        char temp[1024];
                        snprintf(temp, sizeof(temp), "%s/temp", getenv("HOME"));
                        //Removing the directory if already exists using "rm -rf"
                        if (system("rm -rf ~/temp") != 0 && errno != ENOENT)
                        {
                            perror("Unable to remove existing temp folder");
                            exit(EXIT_FAILURE);
                        }
                         // Creation of new temporary directory
                        if (mkdir(temp, 0777) != 0)
                        {
                            perror("Unable to create temp folder");
                            exit(EXIT_FAILURE);
                        }
                         //obtain path to user home directory
                        char dirPath[1024];
                        snprintf(dirPath, sizeof(dirPath), "%s", getenv("HOME"));
                        // Copy the files listed with the specified extensions to the temporary directory.
                        listFilesForExtension(dirPath, extensions, numExtensions, temp);
                        //Check if files with given extension located at temp directory
                        int fileFound = 0;
                        DIR *tempDir = opendir(temp);
                        if (tempDir != NULL)
                        {
                            struct dirent *entry;
                            while ((entry = readdir(tempDir)) != NULL)
                            {
                                for (int i = 0; i < numExtensions; i++)
                                {
                                    if (strstr(entry->d_name, extensions[i]) != NULL)
                                    {
                                        fileFound = 1;
                                        break;
                                    }
                                }
                                if (fileFound)
                                {
                                    break;
                                }
                            }
                            closedir(tempDir);
                        }

                        if (!fileFound)
                        {
                            printf("No file found.\n");
                            send(client_socket, "No file found.\n", strlen("No file found.\n"), 0);
                            return;
                        }

                        // Existing tar file is being removed
                        char existingTarFile[1024];
                        snprintf(existingTarFile, sizeof(existingTarFile), "%s.tar.gz", temp);
                        if (remove(existingTarFile) != 0 && errno != ENOENT)
                        {
                            perror("Unable to remove existing tar file");
                            exit(EXIT_FAILURE);
                        }
                        //Creation of tar file consisting of temp directory
                        char tarCommand[1024];
                        snprintf(tarCommand, sizeof(tarCommand), "tar -czf %s.tar.gz -C %s temp", temp, getenv("HOME"));
                        if (system(tarCommand) != 0)
                        {
                            perror("Unable to create tar file");
                            send(client_socket, "Error: Unable to create tar file\n", strlen("Error: Unable to create tar file\n"), 0);
                            exit(EXIT_FAILURE);
                        }

                        sprintf(message, "Tar file 'temp.tar.gz' created successfully in the home directory.\n");
                    
                         //Tra file redirected to w24project directory
                        char destPath[BUFFER_SIZE] = "~/w24project";

                        char moveCommand[1024];
                        snprintf(moveCommand, sizeof(moveCommand), "mv %s.tar.gz %s/%s.tar.gz", temp, destPath, "temp");

                        if (system(moveCommand) != 0)
                        {
                            perror("Unable to move tar file");
                            send(client_socket, "Error: Unable to move tar file\n", strlen("Error: Unable to move tar file\n"), 0);
                            exit(EXIT_FAILURE);
                        }
                    
                        sprintf(message, "Tar file 'temp.tar.gz' is in w24Project.\n");
                        send(client_socket, message, strlen(message), 0);
                    }
                    else
                    {    //Error message sent,if no extensions specified
                        char error_message[] = "Error: Insufficient arguments.\n";
                        send(client_socket, error_message, strlen(error_message), 0);
                    }
                }
                else
                {   //Error message if invalid command format specified
                    char error_message[] = "Error: Unsupported command\n";
                    send(client_socket, error_message, strlen(error_message), 0);
                }
            }
            else
            {
                char error_message[] = "Error: Insufficient arguments.\n";
                send(client_socket, error_message, strlen(error_message), 0);
            }
        }
       //Verify that if the message received starts with "dirlist -a"
        else if (strncmp(buffer, "dirlist -a", 10) == 0)
        {   //this command lists all subdirectories under its home directory in alphabetical order
            const char *homedir = getenv("HOME");
            if (homedir == NULL)
            {
                char error_message[] = "Error: HOME environment variable not set.\n";
                send(client_socket, error_message, strlen(error_message), 0);
                return;
            }

            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/", homedir);

            DIR *dir;
            struct dirent *entry;
            char result_buffer[1024] = ""; //  Storing the directory list
            char *directories[1024];       // Storing the directory names

            int count = 0;

            if ((dir = opendir(path)) != NULL)
            {
                while ((entry = readdir(dir)) != NULL)
                {   //Create a directory name in memory and store it in the directories array.
                    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    {
                        directories[count] = strdup(entry->d_name);
                        if (directories[count] == NULL)
                        {
                            // Handle memory allocation failure
                            closedir(dir);
                            char error_message[] = "Error: Memory allocation failed.\n";
                            send(client_socket, error_message, strlen(error_message), 0);
                            return;
                        }
                        count++;
                    }
                }
                closedir(dir);
            }
            else
            {   //Error message sent to the client if failure in opening directory
                char error_message[] = "Error: Failed to open directory.\n";
                send(client_socket, error_message, strlen(error_message), 0);
                return;
            }

            if (count == 0)
            {
                char no_directory_message[] = "No directories found.\n";
                send(client_socket, no_directory_message, strlen(no_directory_message), 0);
                return;
            }
             //Directory names sorted alphabeticaly
            qsort(directories, count, sizeof(char *), compare);

            //  sorted directory names is concatenated the into result_buffer
            for (int i = 0; i < count; i++)
            {
                strcat(result_buffer, directories[i]);
                strcat(result_buffer, "\n");
                free(directories[i]); // Free memory allocated by strdup
            }
            //Directory list is being sent to client
            send(client_socket, result_buffer, strlen(result_buffer), 0);
        }

        else if (strncmp(buffer, "dirlist -t", 10) == 0)
        {
            char *home_dir = getenv("HOME");

            if (home_dir == NULL)
            {
                char error_message[] = "Error: HOME environment variable not set.\n";
                send(client_socket, error_message, strlen(error_message), 0);
                return;
            }

            DIR *dir;
            struct dirent *entry;
            struct Directory *directories = NULL;
            int num_directories = 0;

            // Open the directory
            if ((dir = opendir(home_dir)) == NULL)
            {
                perror("opendir");
                return 1;
            }

            // Read each entry in the directory
            while ((entry = readdir(dir)) != NULL)
            {
                // Check if entry is a directory
                if (entry->d_type == DT_DIR)
                {
                    // Ignore "." and ".." directories
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;

                    // Allocate memory for new directory
                    struct Directory *temp = realloc(directories, (num_directories + 1) * sizeof(struct Directory));
                    if (temp == NULL)
                    {
                        fprintf(stderr, "Memory allocation failed\n");
                        closedir(dir);
                        return 1;
                    }
                    directories = temp;

                    // Construct full path of the directory
                    snprintf(directories[num_directories].name, sizeof(directories[num_directories].name), "%s/%s", home_dir, entry->d_name);

                    // Get creation time of directory
                    struct stat st;
                    if (stat(directories[num_directories].name, &st) == 0)
                        directories[num_directories].creation_time = st.st_ctime;
                    else
                        directories[num_directories].creation_time = 0; // Default to 0 if unable to get creation time

                    num_directories++;
                }
            }

            // Sort directories based on creation time in ascending order
            qsort(directories, num_directories, sizeof(struct Directory), compare_two);

            // Print sorted directories

            char result_buffer[1024] = "";
            for (int i = 0; i < num_directories; i++)
            {
                strcat(result_buffer, directories[i].name);
                strcat(result_buffer, "\n");
            }

            // Send the entire result_buffer to the client
            send(client_socket, result_buffer, strlen(result_buffer), 0);
        }

        else if (strncmp(buffer, "quitc", 5) == 0)
        {
            printf("The client Terminated\n");
            close(client_socket);
            exit(0);
        }
        else
        {
            char error_message[] = "Error: Invalid command.\n";
            send(client_socket, error_message, strlen(error_message), 0);
        }
    }

    close(client_socket);
    exit(0);
}
int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // printf("Server listening on port %d...\n", PORT);

    // Main server loop
    while (1)
    {
        // Accept incoming connection
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)NULL, NULL);
        int id1 = fork();
        if (id1 == 0)
        {
            close(server_socket);
            crequest(client_socket);
        }
        else if (id1 > 0)
        {
            close(client_socket);
        }

        // Handle client request
        // crequest(client_socket);
    }

    // Close server socket
    // close(server_socket);

    return 0;
}
