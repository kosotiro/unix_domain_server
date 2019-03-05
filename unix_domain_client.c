#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>

//this will be the message that will be sent to client
const char *messagefromclient = "Hello World!";


int main(int argc,char *argv[]){

        //server address that will be used
        char *server_address = argv[1];

        //response from the server
        char response[256];

        //structure for UNIX domain socket
        struct sockaddr_un un_addr;

        if(server_address!=NULL)
        {
            //create client socket, UNIX domain stream socket
            int client_socket = socket(AF_UNIX,SOCK_STREAM,0);

            //check that client socket has been succesfully created
            if(client_socket==-1){
                perror("CLIENT SOCKET CREATION");
                exit(EXIT_FAILURE);
            }

            //initialize structure for UNIX domain socket
            memset(&un_addr,0,sizeof(struct sockaddr_un));
            un_addr.sun_family = AF_UNIX; //address family for UNIX domain
            strncpy(un_addr.sun_path,server_address,sizeof(un_addr.sun_path)-1);


            if(connect(client_socket,(struct sockaddr *)&un_addr,sizeof(struct sockaddr_un))==-1){
                perror("CLIENT CONNECTION PROBLEM");
                exit(EXIT_FAILURE);
            }

       
            if(send(client_socket,messagefromclient,strlen(messagefromclient)+1,0)>0){
                printf("Client is sending request towards server by using socket:%d\n",client_socket);
            }
            else{
                perror("Problem with data sending!");
                exit(EXIT_FAILURE);
            }

            if(recv(client_socket,response,sizeof(response),0)>0)
                printf("Data received from server...:%s\n",response);
            else
            {
                perror("Problem with data received from server!");
                exit(EXIT_FAILURE);
            }

            if(close(client_socket)==-1)
            {
                perror("CLIENT SOCKET CLOSE ERROR!");
                exit(EXIT_FAILURE);
            }

        }
        else{ //No Server socket path has been given
              printf("Wrong usage!\n");
              printf("Right Usage: ./unix_domain_client \"socket_path\"\n");

        }    


      

       exit(EXIT_SUCCESS);

}


