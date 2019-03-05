#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define PEND_CON 5 //indicates the number of pending connections
                   //for listen function

const char *responsemessage = "Hello from server!";


int main(int argc,char *argv[]){

    //structure for providing timeout on select
    struct timeval timeout;

    //loop variable for checking the whole working set of descriptors
    int i;

    //total number of fds and fd
    int fd;

    //maximum file descriptor that will be used in select
    int maxfd;

    //indication that current connection closed
    bool close_con = false;

    //created client socket from accept
    int client_socket;

    //client request buffer
    char request[256];

    //number of ready descriptors
    int ready = 0;

    //setting up the timeout to 3 mins
    timeout.tv_sec = 3*60;
    timeout.tv_usec = 0;

    //fd_set for adding the server socket for reading
    fd_set masterfdset;

    //the current working fd_set
    fd_set workingset;
    
    /*create a UNIX domain stream socket
      UNIX DOMAIN, socket stream and for protocol type "0",
      this will be our master socket*/
    int server_socket = socket(AF_UNIX,SOCK_STREAM,0);


    //check that the socket has been succesfully created
    if(server_socket==-1)
    {
        perror("SERVER SOCKET FAILURE");
        exit(EXIT_FAILURE);
    }

    //structure that is related to UNIX domain socket
    struct sockaddr_un un_addr;

    //socket address is a pathname
    char *socket_address = argv[1];

    if(socket_address != NULL){

        if(strlen(socket_address) > sizeof(un_addr.sun_path)-1)
        {
            fprintf(stderr,"Server socket path too long\n");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        /*****************************************************/
        /*remove any existing file with the same pathname for*/
        /*socket, errno value: NO SUCH FILE OR DIRECTORY *****/
        /*****************************************************/
        if(remove(socket_address)==-1 && errno == ENOENT){
           perror("REMOVAL OF SOCKET");
           //close(server_socket);
           //exit(EXIT_FAILURE);
        }

    

        /*
          Set socket to be nonblocking. All of the sockets for
          the incoming connections will also be nonblocking since
          they will inherit that state from the listening socket.
       */
       //take the status flags for fd server_socket
       int flags = fcntl(server_socket,F_GETFL,0);
       
       if(flags != -1)
       {
           int retval = fcntl(server_socket,F_SETFL, flags | O_NONBLOCK);

           if(retval==-1)
           {
               perror("FCNTL ERROR");
               close(server_socket);
               exit(EXIT_FAILURE);
           }
       }
       else
       {
           perror("FCNTL ERROR");
           close(server_socket);
           exit(EXIT_FAILURE);
       }

      

    

        //initialize the fields of UNIX domain socket structure
        memset(&un_addr,0,sizeof(struct sockaddr_un));
        un_addr.sun_family = AF_UNIX; //it is a UNIX domain
        strncpy(un_addr.sun_path,socket_address,sizeof(un_addr.sun_path)-1); //socket address is a pathname

        //bind an address (pathname) to server socket
        if(bind(server_socket,(struct sockaddr *)&un_addr,sizeof(struct sockaddr_un))==-1){
            perror("BINDING FAILURE");
            //not necessary to check again for the succesfull
            close(server_socket);
            exit(EXIT_FAILURE);
        }


        //listen to that socket
        if(listen(server_socket,PEND_CON)==-1){
            perror("LISTENING FAILURE");
            close(server_socket);
            exit(EXIT_FAILURE);
        }


        //initialize the masterfdset with the fd of server
        //clear master fd socket set
        FD_ZERO(&masterfdset);
        //add server socket to masterfdset
        FD_SET(server_socket, &masterfdset);

        //Record maximum fd + 1
        //this is needed for select system call
        maxfd = server_socket;

    
        //waiting for any connections from clients to this server
        while(1){


            /**********************************************************/
            /* Copy the master fd_set over to the working fd_set.     */
            /**********************************************************/
            memcpy(&workingset, &masterfdset, sizeof(masterfdset));

            printf("Waiting for a client connection!\n");

            //save the number of ready descriptors
            ready = select(maxfd+1,&workingset,NULL,NULL,&timeout);


            if(ready==-1)
            {
                perror("SELECT FAILURE");
                exit(EXIT_FAILURE);
            }
            else if(ready==0)
            {
                fprintf(stderr,"Timeout hapened before any descriptor becomes ready!\n");
                close(server_socket);
                unlink(socket_address);
                exit(EXIT_SUCCESS);
            }

            for(i=0;i<=maxfd;i++)
            {
		     /*****************************************************/
                     /*we check the master socket for incoming connections*/
                     /*in other words if there is an activity**************/
                     /*****************************************************/
                    if(FD_ISSET(i,&workingset))
                    {

                       
                       /***************************************************/
                       /*******if the ready socket is the master one*******/
                       /***************************************************/
                       if(i==server_socket)
                       {
                         printf("Master socket is readable\n");

                         /*************************************************/
                         /* Accept all incoming connections that are      */
                         /* queued up on the listening socket before we   */
                         /* loop back and call select again.              */
                         /*************************************************/
                         do
                         {
                            /**********************************************/
                            /* Accept each incoming connection.  If       */
                            /* accept fails with EWOULDBLOCK, then we     */
                            /* have accepted all of them.  Any other      */
                            /* failure on accept will cause us to end the */
                            /* server.                                    */
                            /**********************************************/
                            client_socket = accept(server_socket, NULL, NULL);
                  
                            if (client_socket < 0)
                            {
                                if (errno != EWOULDBLOCK)
                                {
                                    perror("Accept failed");
                                    
                                    int fd;

                                    for( fd=0; fd<=maxfd; fd++ )
                                    {
                                       if( FD_ISSET(fd, &masterfdset) )
                                          close(fd);
                                    }

                                    unlink(socket_address);

                                    exit(EXIT_FAILURE);
                                }

                                break;
                            }

                       

                            /**********************************************/
                            /* Add the new incoming connection to the     */
                            /* master read set                            */
                            /**********************************************/
                            printf("New incoming connection - %d\n", client_socket);
                            FD_SET(client_socket, &masterfdset);

                            /***********************************************/
                            /* we should check if maxfd needs to be updated*/
			    /* because of the new insertion                */
                            /***********************************************/ 
                            if (client_socket > maxfd)
                                maxfd = client_socket;

                            /**********************************************/
                            /* Loop back up and accept another incoming   */
                            /* connection                                 */
                            /**********************************************/
                          } while (client_socket != -1);                 
                       }
                       else
                       {

			   close_con = false;

                           while(1)
                           {

				int bytes=recv(i,request,sizeof(request),0);

                                if( bytes > 0)
                                {

                                    printf("Request from client socket %d is received with data:%s\n",i,request);

                                    /***********************************************************************/
                                    /*we use the "client_socket" retrieved from accept system call in order*/
                                    /*to reply back to peer socket with "messagefromserver" ****************/
                                    /***********************************************************************/
                                    if(send(i,responsemessage,(strlen(responsemessage)+1),0) > 0)
                                    {
                                        printf("Replying back to client\n");
                                    }
                                    else
                                    {
                                        perror("SEND PROBLEM");
                                        
                                        //indicate that connection should be closed
                                        close_con = true;
                                        break;
                                    }
                                }
                                /**************************************************************************/
                                /**If no messages are available to be received and the peer has performed**/ 
                                /**an orderly shutdown, recv() shall return 0.*****************************/
                                /**************************************************************************/
				else if(bytes==0)
				{


                                  fprintf(stderr, "Connection from client has been closed\n");

                                  //indicate that connection should be closed
			          close_con = true;
                                  break;
                                  
				}
                                else
                                {
                                    if(errno!=EWOULDBLOCK)
                                    {
                                        perror("SERVER RECEIVE PROBLEM");
                                        close_con = true;
                                    }


                                    break;
                                }
                            }

                            /*****************************************************************************/
                            /******we should remove the descriptor from working set and close it at the***/ 
                            /****** same time, moreover we should update maxfd value if needed************/
                            /*****************************************************************************/
                            if(close_con)
                            {
                              close(i);

                              //remove it from the master set
                              FD_CLR(i, &masterfdset);
                                  
			      if(i==maxfd)
                              {
                                 while( !FD_ISSET(maxfd, &workingset) )
                                       maxfd-=1;
                              }
                            }


                         }
                       
                    }//end of if(FD_ISSET(i,&workingset))
                }//end of for statement
            }//end of while statement
    }
    else{ //No Server socket path has been given
        printf("Wrong usage!\n");
        printf("Right Usage: ./unix_domain_server \"socket_path\" \n");

    }
    exit(EXIT_SUCCESS);


}
