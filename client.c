#include <stdio.h>      //  for prinf scanf
#include <stdlib.h>     // for memory location
#include <string.h>     // for string manipulation of the data
#include <unistd.h>     // used ofr read, close , socket input/output
#include <sys/socket.h> // used specifically for the function of socket like bind, connect , accept etc
#include <netinet/in.h> // to access and define the sockaddr_in structure of the socket
#include <arpa/inet.h>  //used for function of manipulating the internet address and information .. like inet_pton

#define BUFFER_SIZE 1024 // a buffersize use can accept // any message or variable to store data in it

// Function to verify command syntax which is send by the client to the server , mirror1 and mirror2 .........
int check_syntex_cmd(const char *order)
{
    // Check if the command starts with "w24fn " followed by a filename
    if (strncmp(order, "w24fn ", 6) == 0 && strlen(order) > 6)
    {
        return 1; // thissss sys that the cmd gvn by the clint is correct/...... yeahhhhhhhh
    }
    // Check if the command starts with "w24fz " followed by a two size given by the client to get data between those two size defined
    else if (strncmp(order, "w24fz ", 6) == 0 && strlen(order) > 6)
    {
        return 1; // thissss sys that the cmd gvn by the clint is correct/...... yeahhhhhhhh
    }
    // Check if the command starts with "w24fdb " followed by a date given by the client to get data lesss(chota) thn the data defines by the client
    else if (strncmp(order, "w24fdb ", 6) == 0 && strlen(order) > 6)
    {
        return 1; // thissss sys that the cmd gvn by the clint is correct/...... yeahhhhhhhh
    }
    // Check if the command starts with "w24fdb " followed by a date given by the client to get data greaterrr(bada) thn the data defines by the client
    else if (strncmp(order, "w24fda ", 6) == 0 && strlen(order) > 6)
    {
        return 1; // thissss sys that the cmd gvn by the clint is correct/...... yeahhhhhhhh
    }
    // Check if the command starts with "w24ft " followed by a three extention given by the client to get data based on the extention file given by the client
    else if (strncmp(order, "w24ft ", 6) == 0 && strlen(order) > 6)
    {
        return 1; // thissss sys that the cmd gvn by the clint is correct/...... yeahhhhhhhh
    }
    // Check if the command starts with "dirlist -a  " which is given by the client to get list of subdirecotry anf filename in the home direcotory in the accending order like a,b,c,d... of alphabets
    else if (strncmp(order, "dirlist -a ", 10) == 0)
    {
        return 1; // Syntax is correct
    }
    // Check if the command starts with "dirlist -a  " which is given by the client to get list of subdirecotry anf filename in the home direcotory in the order they are created by the time ie the one which is created 1st will be displayed 1st and so on
    else if (strncmp(order, "dirlist -t ", 10) == 0)
    {
        return 1; // Syntax is correct
    }
    // if no command and the command which is not from the above one then it will give this error
    else
    {
        printf("Error: Invalid command syntax. \n");
        return 0; // Syntax is incorrect
    }
}

// this finction is used for redirecting the connectin with the mirror
// the predefines port is 6031
// the function is taking the parameter of ipAddress which is send from the main function
void connectToPort6301(const char *server_ip)
{
    // definngi thwo variable one for mirror_socket file descriptor and the othe rfor portNumber
    int mirror_s, pNum;
    // to specify the port and sfferess which will be sored at this address
    struct sockaddr_in mirror_addr;

    // mirror_socket will create socket
    // teo family - af_inet , af_uniz
    // tpye 2 - sock_steam , sock_dgram
    // default prototype
    mirror_s = socket(AF_INET, SOCK_STREAM, 0);
    if (mirror_s < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // the port number is static and that is 6301 which it has to connect
    pNum = 6301;
    mirror_addr.sin_family = AF_INET;   // the family os af_inet
    mirror_addr.sin_port = htons(pNum); // assigning the port in the affress

    // check if the address given by the client is the same for server ot not
    if (inet_pton(AF_INET, server_ip, &mirror_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(3);
    }

    // if all gies well then it will connect the to the socket
    if (connect(mirror_s, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) < 0)
    {
        perror("Connection failed");
        exit(3);
    }
}

// this finction is used for redirecting the connectin with the mirror
// the predefines port is 3011
// the function is taking the parameter of ipAddress which is send from the main function
void connectToPort3011(const char *server_ip)
{
    // definngi thwo variable one for mirror_socket file descriptor and the othe rfor portNumber
    int mirror_s, pNum;
    // to specify the port and sfferess which will be sored at this address
    struct sockaddr_in mirror_addr;

    // mirror_socket will create socket
    // teo family - af_inet , af_uniz
    // tpye 2 - sock_steam , sock_dgram
    // default prototype
    mirror_s = socket(AF_INET, SOCK_STREAM, 0);
    if (mirror_s < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // the port number is static and that is 6301 which it has to connect
    pNum = 3011;
    mirror_addr.sin_family = AF_INET;   // the family os af_inet
    mirror_addr.sin_port = htons(pNum); // assigning the port in the affress

    // check if the address given by the client is the same for server ot not
    if (inet_pton(AF_INET, server_ip, &mirror_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(3);
    }

    // if all gies well then it will connect the to the socket
    if (connect(mirror_s, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) < 0)
    {
        perror("Connection failed");
        exit(3);
    }
}

int main(int argc, char *argv[])
{

    // declared the variables ofr client and the server socket and the port at what iniitaially it is going to connect to the server 
    int client_socket, server_socket, port;
    // to specify the port and sfferess which will be sored at this address
    struct sockaddr_in server_addr, mirror_addr;

// if the arguments from the client side are not 3 then 
    if (argc != 3)
    {
        printf("Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

//this will create the socket with family, type,default underline protocol
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // will take the 2nd argument hich is paded by the client and store that in port variable
    sscanf(argv[2], "%d", &port);

    // Initialize server address structure
    server_addr.sin_family = AF_INET;// will initialize the family
    server_addr.sin_port = htons(port);// will initialize port in address

    // check if the address given by the client is the same for server ot not
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // this to connect to the serrvr iwth declating all the things like its socket_fd , address ssreucture , and its length
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    char stt_data[500];// this is checking the status by the server
    memset(stt_data, 0, sizeof(stt_data));// it will set thh the things to zero
    int the_data_value = read(client_socket, stt_data, sizeof(stt_data)); // thiswill read  the value from the client_cocket of status
    int num_prt_stt = atoi(stt_data);//the status value will be convertes into integer 


    // if the value is 6031 then go inside
    if (num_prt_stt == 6301)
    {
        // first close the current socket and then call the function with argv[1] ir that ip+address
        close(client_socket);
        connectToPort6301(argv[1]);
    }
    // if the value is 3011 then go inside
    else if (num_prt_stt == 3011)
    {
        // first close the current socket and then call the function with argv[1] ir that ip+address
        close(client_socket);
        connectToPort3011(argv[1]);
    }

    char order[BUFFER_SIZE];

    while (1)
    {
        // Get command from user
        printf("Enter command: ");
        fgets(order, BUFFER_SIZE, stdin);

        // to modift and elemniate the neeline and nullc haracters 
        if (order[strlen(order) - 1] == '\n')
            order[strlen(order) - 1] = '\0';

        //if the command passed by the client is quit then  send the message to the server and then terminate 
        if (strcmp(order, "quitc") == 0)
        {
            // Send 'quitc' command to server
            send(client_socket, order, strlen(order), 0);
            break;
        }

        // Check command syntax
        if (!check_syntex_cmd(order))
        {
            continue;
        }

        
        // this will now send the command to the server after validating 
        send(client_socket, order, strlen(order), 0);

        // this buffer will get the data from the server and then it iwll retrive that and display in the client side
        char data_get[BUFFER_SIZE];
        // the bytes of data recives from the server and if it is greater than 0 then it will print all the dtaa in the clint sode
        int data_get_size = recv(client_socket, data_get, BUFFER_SIZE, 0);
        if (data_get_size > 0)
        {
            data_get[data_get_size] = '\0';
            printf("Server response: %s\n", data_get);
        }
        else
        {
            perror("recv");
            break;
        }
    }

    close(client_socket);

    return 0;
}
