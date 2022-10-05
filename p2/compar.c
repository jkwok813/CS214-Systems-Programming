#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#ifndef QSIZE
#define QSIZE 8
#endif
pthread_mutex_t mutex;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
	char* filename[QSIZE];//queue holds 8 files
	unsigned count;//number of items
	unsigned head;//location of head of queue(more items, higher number)
	int open;//0 = false, 1 = true
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
}queue_t;

int init(queue_t *Q){
	Q->count = 0;
	Q->head = 0;
	Q->open = 1;//1 is true, 0 is false
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);

	return 0;
}

int destroy(queue_t *Q){
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);

	return 0;
}

int enqueue(queue_t *Q, char *item){
	pthread_mutex_lock(&Q->lock);
	
	while(Q->count == QSIZE && Q->open){
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}

	if(!Q->open){//checks if current queue is open
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	unsigned i = Q->head + Q->count;
	if(i >= QSIZE){
		i -= QSIZE;
	}
	
	Q->filename[i] = item;
	++Q->count;

	pthread_cond_signal(&Q->read_ready);
	pthread_mutex_unlock(&Q->lock);

	return 0;
}

int dequeue(queue_t *Q, char **item){

	pthread_mutex_lock(&Q->lock);
	while(Q->count == 0 && Q->open){
		pthread_cond_wait(&Q->read_ready, &Q->lock);	
	}

	if(Q->count == 0){//the queue is empty?
		pthread_mutex_unlock(&Q->lock);
	}
	
	*item = Q->filename[Q->head];
	--Q->count;
	++Q->head;
	
	if(Q->head == QSIZE){
		Q->head = 0;
	}
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}

int qclose(queue_t *Q){
	pthread_mutex_lock(&Q->lock);
	Q->open = 0;
	pthread_cond_broadcast(&Q->read_ready);
	pthread_cond_broadcast(&Q->write_ready);
	pthread_mutex_unlock(&Q->lock);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
	char* filename[QSIZE];
	unsigned count;
	unsigned head;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} queue_t2;

int init2(queue_t2 *Q)
{
	Q->count = 0;
	Q->head = 0;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);
	
	return 0;
}

int destroy2(queue_t2 *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);

	return 0;
}

// add item to end of queue
// if the queue is full, block until space becomes available
int enqueue2(queue_t2 *Q, char *item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == QSIZE) {
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}
	
	unsigned i = Q->head + Q->count;
	if (i >= QSIZE) i -= QSIZE;
	
	Q->filename[i] = item;
	++Q->count;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}


int dequeue2(queue_t2 *Q, char **item)
{
	pthread_mutex_lock(&Q->lock);
	
	while (Q->count == 0) {
		pthread_cond_wait(&Q->read_ready, &Q->lock);
	}


	//printf("current item in dequeue2 is: %s\n", Q->filename[Q->head]);
	*item = Q->filename[Q->head];
	--Q->count;
	++Q->head;
	if (Q->head == QSIZE) Q->head = 0;
	
	pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}

///////////////////////////////////////

typedef struct List {
    int file;
    char *word;
    int count;
    struct List *next;
}List;

typedef struct twofile{
    int fileOne;
    int fileTwo;
}twofile;

queue_t fqueue;
queue_t2 dqueue;
struct List* listarray[65535];
double jsd;
char* sc = ".txt";
int running_threads = 0;

static struct List *insert(struct List *headptr, char* word, int file){
        //printf("A\n");
    	struct List *ptr = malloc(sizeof(struct List));
    	ptr->word = word;
	    ptr->word = strdup(word);//NEW STUFF
    	ptr->next = NULL;
    	struct List **ptr2 = &headptr;  
        int bool = 0;
    	while(*ptr2 != NULL){
        	if(strcmp(ptr->word, (*ptr2)->word) == 0){//words are the same
    	            /*printf("ptr = %s\n", ptr->word);
    	            printf("ptr2 = %s\n", (*ptr2)->word);*/
                    if((*ptr2)->count == 0){
                        (*ptr2)->count++;
                    }
           		    (*ptr2)->count++;
            		//printf("Words are the same, count for %s is now %d\n", (*ptr2)->word, (*ptr2)->count);
                    bool = 2;
            		break;
        	}
        	else if(strcmp(ptr->word, (*ptr2)->word) > 0){//ptr comes first     
            		ptr2 = &(*ptr2)->next; 
                    //printf("Scooch\n");
                    if((*ptr2) == NULL){
                        //printf("Bawk null\n");
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

static struct List *add(char filename[1000], struct List *headptr, int file){
    	char line[10000], a[10000];
    	//printf("In add method\n");
    	int fd = open(filename, O_RDONLY);
    	if(fd == -1){
        	perror("Failure: ");
        	return headptr;
    	}
    	read(3, line, 65535);
    	int len = 0;
    	int index = 0;
        //printf("The text of the file is %s\n", line);
    	while(1 == 1){
            //printf(";%s\n", a);
            if(isalpha(line[index]) == 0){//skip all punc
                while(isalpha(line[index]) == 0 && line[index] != ' ' && line[index] != '\n' && line[index] != '\0'){                 
                    index++;
                }
            }
		    if(isspace(line[index]) != 0){//end of string, either space or \n or some other.
             	struct List **head = &headptr;
              	if(*head == NULL){          
                    //printf("Head = null \n"); 
                 	struct List *newNode = malloc(sizeof(struct List));
				    newNode->word = strdup(a);//New stuff
                    newNode->count = 1;
                    newNode->next = NULL;
                    (*head) = newNode;//newnode and head share the same ontents
                    len = 0; index++;
				    memset(a, '\0', strlen(a));
                }
                else{
                    headptr = insert(headptr, a, file);
                    memset(a, '\0', strlen(a));
                    len = 0;
                    index++;
				    memset(a, '\0', strlen(a));
                }
            }
            else if(line[index] == '\0'){//EOF
                //printf("Final string is %s\n", a);
                    if(a[0] == '\0'){
                        break;
                    }
                    struct List **head = &headptr;
              	    if(*head == NULL){         
                        //printf("head == NULL called\n");     
                     	struct List *newNode = malloc(sizeof(struct List));
				        newNode->word = strdup(a);//New stuff
                        newNode->count = 0;
                        newNode->next = NULL;
                        (*head) = newNode;//newnode and head share the same ontents
                        len = 0; index++;
				        memset(a, '\0', strlen(a));
                    }
               		else{
                        headptr = insert(headptr, a, file);
                    	memset(a, '\0', strlen(a));
                    	len = 0;
            			//printf("END OF FILE headptr->word = %s\n", headptr->word);
                	}
                	break;
            }
            if(isalpha(line[index]) == 0){//skip all punc
                //printf("The crrent string is %s\n", a);
                while(isalpha(line[index]) == 0 && line[index] != ' ' && line[index] != '\n' && line[index] != '\0'){                 
                    //printf("Punctuation is %c\n", line[index]);
                    index++;
                }
            }            		    
		    a[len] = line[index];
            a[len] = toupper(a[len]);
		    len++;
		    index++;
	    }
        memset(line, '\0', strlen(line));
	    //printf("the head is %s\n", headptr->word);
    	close(fd);    
        //printf("closing\n");
    	return headptr;
}

void* directories(void *arg){
	pthread_mutex_lock(&mutex);

	//printf("###### VOID DIRECTORIES CALLED ########\n");

	char *dirname = arg;
	//printf("dirname = %s\n", dirname);

	char dirfile[100][1000];
	int count = 0;
	
	struct stat buf;
	stat(dirname, &buf);
	if(!S_ISDIR(buf.st_mode)){
		return NULL;
	}

	DIR *dir = opendir(dirname);
	struct dirent *sd;
	
	while((sd = readdir(dir)) != NULL){
		//printf("sd->d_name = %s\n", sd->d_name);
		if(strcmp(sd->d_name, ".") == 0 || strcmp(sd->d_name, "..") == 0){
			continue;
		}
		if(strstr(sd->d_name, sc)){
			strcat(dirfile[count], dirname);
			strcat(dirfile[count], "/");
			strcat(dirfile[count], sd->d_name);
			count++;
		}
		else{
			strcat(dirfile[count], dirname);
			strcat(dirfile[count], "/");
			strcat(dirfile[count], sd->d_name);
			count++;
		}
	}
	closedir(dir);
	
	char** result = malloc((count+1)*sizeof(char*));
	char key[100];
	strcat(key, "JustinIsBadAtLOL");
	for(int x = 0; x < count+1; x++){
		result[x] = malloc(sizeof(char)*1000);
		memcpy(result[x], dirfile[x], strlen(dirfile[x])+1);
	}
	memcpy(result[count], key, strlen(key)+1);
	
	memset(dirfile, '\0', sizeof(dirfile[0][0])*100*1000);
	memset(key, '\0', sizeof(key[0])*100);

	pthread_mutex_unlock(&mutex);
	return (void*)result;
}

void* files(void * arg){
    if(arg == NULL){
            pthread_mutex_lock(&running_mutex);
            running_threads--;
            pthread_mutex_unlock(&running_mutex);
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    //sleep(2);
    struct List* headptr = NULL;
    int file = 1;
    char *fname = (char*) arg;
    headptr = add(fname, headptr, file);
    //printf("Thread done\n"); 
    //printf("Wow\n");
    free(arg);
    pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&running_mutex);
            running_threads--;
            pthread_mutex_unlock(&running_mutex);
    return headptr;     
}

void* analy(void * arg){
    if(arg == NULL){
        //free(arg);
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    //printf("In analysis thread\n");
    struct twofile *point = (struct twofile *)arg; 
    int b = point->fileOne; //first file 
    int c = point->fileTwo; //second file
    int ccc = 2;
    int d = 0;
    //printf("%d %d\n", b, c);
    double totals[2] = {0}; 
    //printf("calcing totals\n");
    while(ccc > 0){
        //printf("ccc = %d\n", ccc);
        double total = 0;
        struct List *travel = NULL;
        if(ccc == 2){
            travel = listarray[0];
        }
        else{
            travel = listarray[1];
        }
	    int i = 0;
        int z = 0;
	    //printf("Contents of headptr:\n");
	    while(travel != NULL){
            //printf("%s\n", travel->word);
            total+=travel->count;
		    travel = travel->next;
		    i++;
	    }
        totals[d] = total;
        d++;
        z++;
        travel = listarray[z];
        ccc--;
    }
    //printf("Total %f and %f\n", totals[0], totals[1]); 
    double kld = 0;
    struct List *travel = NULL;
    struct List *travel2 = NULL;
    travel = listarray[b];
    travel2 = listarray[c];
    //printf("HELP\n");
    while(travel != NULL){                
        while(travel2 != NULL){
            if(strcmp(travel->word, travel2->word) == 0){
                //printf("if statement travel->word %s and travel2->word %s are the same\n", travel->word, travel2->word);
                double first = travel->count/totals[0]; //wfd of word 1  
                double second = travel2->count/totals[1]; //wfd of word 2
                double ftotal = (first+second)/2;  
                //printf("first = %f, second = %f, ftotal = %f\n", first, second, ftotal);
                double calc = (log(first/ftotal)/log(2));
                calc = first*calc;
                //printf("calc in anal = %f\n", calc);
                kld += calc;
                //printf("Kld = is %f\n", kld);
                travel->file = 1; travel2->file = 1;
            }
            else if(travel2->next == NULL){
                if(travel->file != 1){
                    //printf("else statement travel->word %s and travel2->word %s are the same\n", travel->word, travel2->word);
                    double first = travel->count/totals[0];
                    double ftotal = (first)/2;
                    double calc = (log(first/ftotal)/log(2));
                    calc = first*calc;
                    kld += calc;
                    //printf("Kld = is %f\n", kld);
                    travel->file = 1;  
                }              
            }
            travel2 = travel2->next;
        }
        travel2 = listarray[c];
        travel = travel->next;
    }
    travel = listarray[b];
    travel2 = listarray[c];
    while(travel2 != NULL){
        while(travel != NULL){
            if(strcmp(travel->word, travel2->word) == 0){
                double first = travel2->count/totals[1];
                double second = travel->count/totals[0];
                double ftotal = (first+second)/2;
                double calc = (log(first/ftotal)/log(2));
                calc = first*calc;
                kld += calc;
                travel->file = 1; travel2->file = 1;
            }
            else if(travel->next == NULL){
                if(travel2->file != 1){
                    double first = travel2->count/totals[1];
                    double ftotal = first/2;
                    double calc = (log(first/ftotal)/log(2));
                    calc = first*calc;
                    kld+= calc;
                    travel2->file = 1;
                }
            }
            travel = travel->next;
        }
        travel = listarray[b];
        travel2 = travel2->next;
    }             
    double* kld2 = malloc(sizeof(double));
    *kld2 = kld;
    //printf("THE FINAL KLD IS %f\n", kld);
    pthread_mutex_unlock(&mutex);
    free(arg);
    return (void*)kld2;
}     

int main(int argc, char* argv[]){
    //char filename[65535][65535];
    int dc = 1;
    int fc = 1;
    int ac = 1;
	if(init(&fqueue) != 0)return 1;
	if(init2(&dqueue) != 0) return 1;
	for(int x = 1; argv[x] != NULL; x++){//Looks for optional inputs
        if(argv[x][0] == '-'){//optional input
            if(argv[x][1] == 's'){//File name suffix
                //printf("s detected\n");
                char *c = argv[x]+2;
                sc = c;
                    //printf("sc %s\n", sc);
                continue;
                }        
                else{
                    printf("Invalid or missing argument.\n");  
                }   
            if(argv[x][2] != '\0'){
                if(argv[x][1] == 'd'){ //directory thread
                    if(argv[x][2] == '0'){
                        printf("Invalid or missing argument.\n");
                        return EXIT_FAILURE;
                    }               
                    else if(isdigit(argv[x][2]) != 0){//limit the amount of directory threads
                        char *b = argv[x]+2;
                        dc = atoi(b);
                    }
                    else{
                        printf("Invalid or missing argument.\n");  
                        return EXIT_FAILURE;
                    }
                }
                if(argv[x][1] == 'f'){//file threads
                    if(argv[x][2] == '0'){
                       printf("Invalid or missing argument.\n");  
                        return EXIT_FAILURE;
                    }                    
                    else if(isdigit(argv[x][2]) != 0){//limit the amount of file threads
                        char *b = argv[x]+2;
                        fc = atoi(b);
                    }
                    else{
                        printf("Invalid or missing argument.\n");  
                        return EXIT_FAILURE;
                    }
                }
                if(argv[x][1] == 'a'){//analysis threads
                    if(argv[x][2] == '0'){
                        printf("Invalid or missing argument.\n");  
                        return EXIT_FAILURE;
                    }                    
                    else if(isdigit(argv[x][2]) != 0){//limit the number of analysis threads 
                        char *b = argv[x]+2;
                        ac = atoi(b);
                    }
                    else{
                        printf("Invalid or missing argument.\n");  
                        return EXIT_FAILURE;
                    }
                }  
            }
            else{
                printf("Invalid or missing argument.\n"); 
                return EXIT_FAILURE;
            }       
        }
    }

    //printf("Due to optional arguments, dc is now %d fc is now %d and ac is now %d suffix is %s\n", dc, fc, ac, sc);
	//adds to file and directory queue
    int fcount = 0;
    int dcount = 0;
	struct stat buf;
	for(int x = 1; argv[x] != NULL; x++){
        if(argv[x][0] == '-'){
            continue;
        }
		stat(argv[x], &buf);
		//printf("argv[%d] = %s\n", x, argv[x]);
        if(strcmp(sc, "") == 0){
            printf("null sc detected\n");
            if(S_ISDIR(buf.st_mode)){
                dcount++;
                enqueue2(&dqueue, argv[x]);
            }
            else{
                fcount++;
                enqueue(&fqueue, argv[x]);
            }
        }    
		else if(strstr(argv[x], sc) != NULL){
			//printf("File Found! argv[%d] = %s\n", x, argv[x]);
			fcount++;
			enqueue(&fqueue, argv[x]);
			//enqueue file
		}else if(S_ISDIR(buf.st_mode)){
			//printf("Directory Found! argv[%d] = %s\n", x, argv[x]);
			dcount++;
			enqueue2(&dqueue, argv[x]);
			//enqueue directory
		}
	}

    //Create directory threads
    pthread_t directory[dc];	
	char** dirfile;
	char* dirname;
	int tcount = 0;
	char** completeList = malloc(1000*sizeof(char*));;
	
	while(dcount > 0){//while dqueue has content
		//printf("dqueue.count = %d\n", dqueue.count);
		for(int x = 0; x < dc; x++){
			if(dqueue.count > 0){
				//printf("before dcount = %d\n", dqueue.count);
				
				dequeue2(&dqueue, &dirname);
				dcount--;			
				
				pthread_create(&directory[x], NULL, &directories, dirname);
			}
			else{
				pthread_create(&directory[x], NULL, &directories, NULL);
			}
		}
		for(int x = 0; x < dc; x++){
			pthread_join(directory[x], (void**)&dirfile);
			int c = 0;
			for(c = 0; dirfile != NULL && strcmp(dirfile[c], "JustinIsBadAtLOL") != 0; c++){
				//printf("dirfile[%d] = %s\n", c, dirfile[c]);

				//completeList[tcount] = strdup(dirfile[c]);
				stat(dirfile[c], &buf);
				if(strstr(dirfile[c], sc) != NULL){
					//printf("File Found!\n");
					fcount++;
					completeList[tcount] = malloc(sizeof(char)*1000);
					completeList[tcount] = strdup(dirfile[c]);
					enqueue(&fqueue, completeList[tcount]);
					tcount++;

				}else if(S_ISDIR(buf.st_mode)){
					//printf("Directory Found!\n");
					dcount++;
					completeList[tcount] = malloc(sizeof(char)*1000);
					completeList[tcount] = strdup(dirfile[c]);
					enqueue2(&dqueue, completeList[tcount]);
					tcount++;
				}
				free(dirfile[c]);
			}
			//printf("before if NUll\n");
			if(dirfile != NULL){
				//printf("if NULL called\n");
				free(dirfile[c]);
				free(dirfile);
			}
			//printf("after if NULL\n");
		}//end of for loop
		//printf("for loop ended\n");
	}//end of big while loop


	//printf("tcount = %d\n", tcount);
	free(completeList);

    //create file threads
    pthread_t filethread[fc];
    char farr[100][1000];
    int farrc;
    int y = 0;
    int cc = 0;
    while(dqueue.count != 0 || fqueue.count != 0){
        for(int wow = 0; wow < fc; wow++){
            pthread_mutex_lock(&running_mutex);
            running_threads++;
            pthread_mutex_unlock(&running_mutex);
            if(fqueue.count > 0){
                char *fnames;
                dequeue(&fqueue, &fnames);
                char *fname = malloc(sizeof(char)*1000);
                strncpy(fname, fnames, 500);
                strcpy(farr[farrc], fname);
                farrc++;
                //printf("fname is %s\n", fname);
                if(pthread_create(&filethread[wow], NULL, &files, fname) != 0){
                    perror("Failed\n");
                }
                //printf("PLSDS\n");
            }
            else{
                pthread_create(&filethread[wow], NULL, &files, NULL);
            }
        }
        for(int wow = 0; wow < fc; wow++){
            struct List* helpme = NULL;
            if(pthread_join(filethread[wow], (void**) &helpme) != 0){
                perror("Failed\n");
            }
            //printf("List array %d\n", y);
            listarray[y] = helpme;
            struct List* travel = helpme;
            while(travel != NULL){
                //printf("%s\n", travel->word);
                travel = travel->next;
            }
            cc++;
            y++;
        }
    }
    //printf("running threads: %d\n", running_threads);
    /*while(running_threads > 0){
        printf("Sleeping..\n");
        sleep(1);
    }*/
    //Analysis threads
    //printf("anaylssi \n");
    pthread_t analysis[ac];
    int anac = 0;//compare every file to every other file, cc = total number of files
    int anac2 = 0;
    double* kld = 0;
    while(anac < cc){
        //printf("Cc = %d\n", cc);
        for(int wowz = 0; wowz < ac; wowz++){
                //printf("ha anac is %d and anac2 is %d\n", anac, anac2);
                struct twofile *twofiles = malloc(sizeof(struct twofile));
                anac2++;
                kld = 0;
                twofiles->fileOne = anac;
                twofiles->fileTwo = anac2;
                //printf("%d %d %d\n", anac, anac2, cc);
                if(anac2 < cc && anac < cc){
                    if(pthread_create(&analysis[wowz], NULL, &analy, (void *)twofiles) != 0){
                        perror("Failed\n");
                    }
                }
                else{
                    //printf("NULL\n");
                    pthread_create(&analysis[wowz], NULL, &analy, NULL);
                    free(twofiles);
                }
        }
        for(int wowz = 0; wowz < ac; wowz++){
            if(pthread_join(analysis[wowz], (void**) &kld) != 0){
                perror("Failed\n");
            }
            if(kld == NULL){
               //free(kld);
               continue;
            } 
            //printf("kld = %f\n", *kld);
            *kld = *kld*.5;
            jsd += *kld;
            free(kld);
        }            
        anac++;
        anac2++;
    }                         
    //printf("save me\n");
    jsd = sqrt(jsd);
    printf("%f ", jsd);
    for(int x = 0; x < farrc; x++){
        printf("%s ", farr[x]);
    }
    printf("\n");
    struct List* tmp;
    while(cc >= 0){
        while(listarray[cc] != NULL){ //Be free, list! 
            tmp = listarray[cc];
            listarray[cc] = listarray[cc]->next;
            free(tmp->word);
            free(tmp);
        }
        cc--;
        //printf("cc = %d\n", cc);
    }
    //free(listarray[0]);
    pthread_mutex_destroy(&mutex);
    //printf("EOF\n");
    qclose(&fqueue);
    destroy(&fqueue);
    destroy2(&dqueue);
    return EXIT_SUCCESS;
}
