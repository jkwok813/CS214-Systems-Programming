#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#define BACKLOG 5

// the argument we will pass to the connection-handler threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

typedef struct List {
    char *word;
    int count;
    char *value;
    struct List *next;
}List;

struct List* headptr = NULL;
int server(char *port);
void *echo(void *arg);

static struct List *insert(struct List *headptr, char* word, char* value){
	    //printf("in insert\n\n");
    	struct List *ptr = malloc(sizeof(struct List));
      if(ptr == NULL){
        return ptr;
      }
    	ptr->word = word;
	    ptr->word = strdup(word);//NEW STUFF
        ptr->value = value;
        ptr->value = strdup(value);
    	ptr->next = NULL;
    	struct List **ptr2 = &headptr;
    	/*printf("word = %s\n", word);
    	printf("ptr->word = %s\n", ptr->word);
    	printf("ptr2->word = %s\n", (*ptr2)->word);
        printf("ptr->value = %s\n", ptr->value);
    	printf("ptr2->value = %s\n", (*ptr2)->value);*/
        int bool = 0;
    	while(*ptr2 != NULL){
        	if(strcmp(ptr->word, (*ptr2)->word) == 0){//words are the same
    	            printf("ptr = %s\n", ptr->word);
    	            printf("ptr2 = %s\n", (*ptr2)->word);
                    if((*ptr2)->count == 0){
                        (*ptr2)->count++;
                    }
           		    (*ptr2)->count++;
            		printf("Words are the same, count for %s is now %d\n", (*ptr2)->word, (*ptr2)->count);
                    bool = 2;
            		break;
        	}
        	else if(strcmp(ptr->word, (*ptr2)->word) > 0){//ptr comes first
            		ptr2 = &(*ptr2)->next;
                    if((*ptr2) == NULL){
                        bool = 1;
                    }
        	}
        	else{//str2 comes first
                    bool = 1;
            		break;
        	}
    	}
        if(bool == 2){
            free(ptr->word);
            free(ptr);
            return headptr;
        }
    	ptr->next = *ptr2;//The word will be inserted before ptr2, ptr2 = d, ptr->word = c,
    	*ptr2 = ptr;//Insert ptr->word into headptr
        if(bool == 1){
            (*ptr2)->count = 1;
        }
    	return headptr;
}

int main(int argc, char **argv)/////////////////////////////////////MAIN METHOD//////////////////////////////
{
	if (argc != 2) {//server can only have 1 input: the port number
		printf("Usage: %s [port]\n", argv[0]);
		return 1;
	}

    (void) server(argv[1]);//calls the server method and send over the port number
    return 0;
}/////////////////////////////////////////////////////////////////END OF MAIN METHOD//////////////////////////


int server(char *port)/////////////////////////////////////////////SERVER METHOD/////////////////////////////
{
    struct addrinfo hint, *info_list, *info;
    struct connection *con;
    int error, sfd;
    pthread_t tid;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC; //The socket accepts any kind of domain
    hint.ai_socktype = SOCK_STREAM; //The socket will have streaming connection
    hint.ai_flags = AI_PASSIVE; //The socket will listen for the client?
    	// setting AI_PASSIVE means that we want to create a listening socket

    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &info_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        //
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (info == NULL) {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(info_list);

    // at this point sfd is bound and listening
    printf("Waiting for connection\n");
    for (;;) {
    	// create argument struct for child thread
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        	// addr_len is a read/write parameter to accept
        	// we set the initial value, saying how much space is available
        	// after the call to accept, this field will contain the actual address length

        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        	// we provide
        	// sfd - the listening socket
        	// &con->addr - a location to write the address of the remote host
        	// &con->addr_len - a location to write the length of the address
        	//
        	// accept will block until a remote host tries to connect
        	// it returns a new socket that can be used to communicate with the remote
        	// host, and writes the address of the remote hist into the provided location

        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
            perror("accept");
            continue;
        }

		// spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, echo, con);

		// if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            free(con);
            continue;
        }

		// otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);
    }

    // never reach here
    return 0;
}/////////////////////////////////////////////END OF SERVER METHOD///////////////////////////////////////////////

#define BUFSIZE 25

void *echo(void *arg)
{
    char host[100], port[10], buf[BUFSIZE + 1];
    struct connection *c = (struct connection *) arg;
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    	// we provide:
    	// the address and its length
    	// a buffer to write the host name, and its length
    	// a buffer to write the port (as a string), and its length
    	// flags, in this case saying that we want the port as a number, not a service name
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    int size = 0;
    char key[100], value[100], vsize[5], vrev[5];
    while ((nread = read(c->fd, buf, BUFSIZE)) > 0) {
        buf[nread] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, nread, buf);

        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        memset(vsize, 0, sizeof(vsize));
        memset(vrev, 0, sizeof(vrev));
        size = 0;

        if(strcmp(buf, "SET\n") == 0){//Check if it's SET GET OR DEL
            printf("SET found\n");
            if((nread = read(c->fd, buf, BUFSIZE)) > 0){
                buf[nread] = '\0';//Checks for size of the key
                if(atoi(buf) > 0){
                    size = atoi(buf);
                    printf("size = %d\n", size);
                }
                else break;
            }
            else break;
            if((nread = read(c->fd, buf, BUFSIZE)) > 0){//Checks for the key
                buf[nread] = '\0';
                strcpy(key, buf);
            }
            else break;
            if((nread = read(c->fd, buf, BUFSIZE)) > 0){//What's being inserted into the linked list
                buf[nread] = '\0';
                strcpy(value, buf);
            }
            else break;
            if(size != strlen(key) + strlen(value)){
                write(c->fd, "ERR\nLEN\n", sizeof("ERR\nLEN\n"));
                break;
            }
            else{
            struct List **head = &headptr;
                if(*head == NULL){
				    //printf("*head == NULL called\n");
                    struct List *newNode = malloc(sizeof(struct List));
                    if(newNode == NULL){
                      write(c->fd, "ERR\nSRV\n", sizeof("ERR\nSRV\n"));
                      break;
                    }
				                newNode->word = strdup(key);//New stuff
                    newNode->value = strdup(value);
                    newNode->count = 1;
                    newNode->next = NULL;
                    (*head) = newNode;//newnode and head share the same ontents
                }
                else{
                    headptr = insert(headptr, key, value);
                    if(headptr == NULL){
                      write(c->fd, "ERR\nSRV\n", sizeof("ERR\nSRV\n"));
                    }
                }
            }
            write(c->fd, "OKS\n", sizeof("OKS\n"));
        }






        else if(strcmp(buf, "GET\n") == 0){
            printf("Get\n");
            if((nread = read(c->fd, buf, BUFSIZE)) > 0){
              buf[nread] = '\0';
                if(atoi(buf) > 0){
                    size = atoi(buf);
                    printf("size = %d\n", size);
                }
                else break;
            }
            else break;

            if((nread = read(c->fd, buf, BUFSIZE)) > 0){
              buf[nread] = '\0';
              strcpy(key, buf);
            }
            else break;
            if(size != strlen(key)){
                write(c->fd, "ERR\nLEN\n", sizeof("ERR\nLEN\n"));
                break;
            }
            else{
                struct List *travel2 = headptr;
                int find = 0;
	               while(travel2 != NULL){
		            printf("word = %s, value = %s", travel2->word, travel2->value);
                    if(strcmp(travel2->word, key) == 0){
                        strcpy(value, travel2->value);
                        printf("OKG\n%ld\n%s\n", strlen(value)+1, value); //fix
                        find = 1;
                        break;
                    }
		            travel2 = travel2->next;
                }
                if(find == 0){
                  write(c->fd, "KNF\n", sizeof("KNF\n"));
                  break;
                }

	        }

          ////////////////////////////////////////prints to the client/////////////////
          int a = strlen(value), b = 0;
          while(a > 0){
            a /= 10;
            b++;
          }

          a = strlen(value);
          for(int x = 0; x < b; x++){
            vsize[x] = a%10 + '0';
            a /= 10;
          }
          vsize[b] = '\n';

          for(int x = 0; x < b; x++){
            vrev[x] = vsize[b-1-x];
          }
          vrev[b] = '\n';

          //printf("vsize = %s\n", vsize);
          //printf("vrev = %s\n", vrev);
          write(c->fd, "OKG\n", sizeof("OKG\n"));
          write(c->fd, vrev, sizeof(vrev));//write the size of the value (not key)
          write(c->fd, value, sizeof(value)); //write the name of the value
          ///////////////////////////////////////////////////////////////////////////////
        }


        //error with multiple keys. If you make two keys and ry to delete them all, a heap-use-after-free error occurs
        else if(strcmp(buf, "DEL\n") == 0){
            printf("Del\n");
            if((nread = read(c->fd, buf, BUFSIZE)) > 0){
              buf[nread] = '\0';
                if(atoi(buf) > 0){
                    size = atoi(buf);
                    printf("size = %d\n", size);
                }
                else break;
            }
            else break;

            if((nread = read(c->fd, buf, BUFSIZE)) > 0){
              buf[nread] = '\0';
              strcpy(key, buf);
            }
            else break;
            if(size != strlen(key)){
                write(c->fd, "ERR\nLEN\n", sizeof("ERR\nLEN\n"));
                break;
            }
            else{
                struct List *travel2 = headptr;
                struct List *pre = headptr;
	            while(travel2 != NULL){
		            printf("word = %s, value = %s", travel2->word, travel2->value);
                    if(strcmp(travel2->word, key) == 0){
                      strcpy(value, travel2->value);
                        printf("OKD\n%ld\n%s\n", strlen(value)+1, value);
                        if(travel2->next != NULL){
                            pre->next = travel2->next;
                            struct List *temp = travel2;
                            travel2 = travel2->next;
                            free(temp);
                            break;
                        }
                        else{
                            headptr = NULL;
                            break;
                        }
                    }
                    pre = travel2;
		            travel2 = travel2->next;
                }
	        }

          ////////////////////////////////////////prints to the client/////////////////
          int a = strlen(value), b = 0;
          while(a > 0){
            a /= 10;
            b++;
          }

          a = strlen(value);
          for(int x = 0; x < b; x++){
            vsize[x] = a%10 + '0';
            a /= 10;
          }
          vsize[b] = '\n';

          for(int x = 0; x < b; x++){
            vrev[x] = vsize[b-1-x];
          }
          vrev[b] = '\n';

          //printf("vsize = %s\n", vsize);
          //printf("vrev = %s\n", vrev);
          write(c->fd, "OKD\n", sizeof("OKD\n"));
          write(c->fd, vrev, sizeof(vrev));//write the size of the value (not key)
          write(c->fd, value, sizeof(value)); //write the name of the value
          ////////////////////////////////////////////////////////////////////////
        }




        else if(strcmp(buf, "print\n") == 0){
            struct List *travel2 = headptr;
	        while(travel2 != NULL){
		        printf("word = %s, value = %s", travel2->word, travel2->value);
		        travel2 = travel2->next;
            }
	    }
        else{
            write(c->fd, "ERR\nBAD\n", sizeof("ERR\nBAD\n"));
            break;
        }

        printf("key = %s", key);
        printf("value = %s", value);

    }
    struct List* tmp;
    while(headptr != NULL){ //Be free, list!
        tmp = headptr;
        headptr = headptr->next;
        free(tmp->word);
        free(tmp->value);
        free(tmp);
    }
    printf("[%s:%s] got EOF\n", host, port);
    close(c->fd);
    free(c);
    return NULL;
}
