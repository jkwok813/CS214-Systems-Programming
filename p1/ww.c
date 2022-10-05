#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int fileWordWrap(int argc, char* argv[argc+1]){
/*
	count counts the total number of characters as we add to a line
	len is the length of the word we created
	index allows us to add to the char array a
	whitespace is used to determine if an empty line exists, otherwise this program will print out 1 less empty line
*/
	int width = atoi(argv[1]), count = 0, len = 0, index = 0, whitespace = 0;
/*
	a will hold our words
	line will hold the entire line
*/
	char a[width*2], line[10000];
	char newline = '\n';
	char emptyspace = ' ';

    
   	if(argv[2] == NULL){
   		int bytes;
		bytes = read(0, line, 1000000);
		while(bytes > 0){
			index = 0;
			//checks for empty lines
			if(line[0] == '\n'){
				write(1, &newline, 1);
				whitespace = 1;
				count = 0;
				bytes--;
			}else if(whitespace == 1){
				write(1, &newline, 1);
				whitespace = 0;
				bytes--;
			}
			
			//traverses line until null terminator is found (end of line)
			while(line[index] != '\0'){
				
				//skips when an empty space is found
				if(line[index] == ' '){
					index++;
					bytes--;
					continue;
				}
	
				//fills char array a with characters until one of these three things is found
				while(1 == 1){
					if(line[index] == '\n' || line[index] == '\0' || line[index] == ' ')break;
					a[len] = line[index];
					len++;
					index++;
					bytes--;
       	        		 //printf("\n%d, %d", len, index);
				}
	
				//Prints out a word. A space is included depending on whether it is the first word or not
				if((count == 0 && len + count <= width) || len + count + 1 <= width){
               			 //printf("%d, %d\n", count, len);
					//The first word in a line
					if(count == 0){
						int x = 0;
			                	while(x<len){
                       					write(1, &a[x], 1);
                       					x++;
                    				}
						count += len;
					//not the first word in a line
					}else{
						int x = 0;
                 				write(1, &emptyspace, 1);
                 				while(x<len){
							write(1, &a[x], 1);
                       					x++;
                    				}
                    				count+= len+1;
					}

				//If the word won't fit in the current line, we make a new line
				}else{
					write(1, &newline, 1);
					int x = 0;
					while(x<len){
                    				write(1, &a[x], 1);
                    				x++;
                			}
					count = len;
				}
			
				//clears char array a, and continues going through the rest of the line
				memset(a, '\0', strlen(a));
				index++;
				bytes--;
				len = 0;
			}
		}
	//Adds a new line at the end
	write(1, &newline, 1);

	//closes the file
	return EXIT_SUCCESS;
	}





/////////////////////////////////////////////////
	else{
		FILE* f = fopen(argv[2], "r");

		if(f == 0){//prints out an error if the file doesn't exist
			perror("Error");
	        fclose(f);
       			return EXIT_FAILURE;
		}

		//goes through file line by line
		while(fgets(line, 1000, f) != NULL){
    			//printf("String is %s\n", line);
			index = 0;

			//checks for empty lines
			if(line[0] == '\n'){
				printf("\n");
				whitespace = 1;
				count = 0;
			}else if(whitespace == 1){
				printf("\n");
				whitespace = 0;
			}
		
			//traverses line until null terminator is found (end of line)
			while(line[index] != '\0'){
			
				//skips when an empty space is found
				if(line[index] == ' '){
					index++;
					continue;
				}
	
				//fills char array a with characters until one of these three things is found
				while(1 == 1){
					if(line[index] == '\n' || line[index] == '\0' || line[index] == ' ')break;
					a[len] = line[index];
					len++;
					index++;
		                //printf("\n%d, %d", len, index);
				}
	
				//Prints out a word. A space is included depending on whether it is the first word or not
				if((count == 0 && len + count <= width) || len + count + 1 <= width){
       	 	        		//printf("%d, %d\n", count, len);
					//The first word in a line
					if(count == 0){
						int x = 0;
						while(x<len){
       	                 				printf("%c", a[x]);
       	              		   			x++;
       	             				}
						count += len;
					//not the first word in a line
					}else{
						int x = 0;
       	            				printf(" ");
       	             				while(x<len){
       	                 				printf("%c", a[x]);
       	                 				x++;
       	             				}
       	             				count+= len+1;
					}
	
				//If the word won't fit in the current line, we make a new line
				}else{
					printf("\n");
					int x = 0;
       	         			while(x<len){
       	             				printf("%c", a[x]);
       	             				x++;
       	        			}
					count = len;
				}
				
				//clears char array a, and continues going through the rest of the line
				memset(a, '\0', strlen(a));
				index++;
				len = 0;
			}
	}
	//Adds a new line at the end
	printf("\n");

	//closes the file
	fclose(f);
	return EXIT_SUCCESS;
    }
}

int main(int argc, char* argv[argc+1]){

	struct stat buf;
	stat(argv[2], &buf);

	//not a directory
	if(argv[2] == NULL || !(S_ISDIR(buf.st_mode))){
		return fileWordWrap(argc, argv);
	//a directory
	}else{

		DIR *dir = opendir(argv[2]);
		struct dirent *sd;
		
		//creates the name of the wrap. file
		char rename[50];		
		
		while((sd = readdir(dir)) != NULL){
			if(strcmp(sd->d_name, ".") == 0 || strcmp(sd->d_name, "..") == 0 || strstr(sd->d_name, "wrap.") != NULL)continue;
			printf(">> %s\n", sd->d_name);

			//fills in the name of the wrap. file
			strcat(rename, "wrap.");
			strcat(rename, sd->d_name);
			
			//allows us to go into the directory
			chdir(argv[2]);			

			int width = atoi(argv[1]), count = 0, len = 0, index = 0, whitespace = 0;

			char a[width*2], line[10000];

			if(argv[2] == NULL){////////////////////////////////////////////////////////////////////////////////////////////////////////
				FILE* w = fopen(rename, "w");
				
				int bytes;
				bytes = read(0, line, 100000);

				while(bytes > 0){

					index = 0; 

					if(line[0] == '\n'){
						fprintf(w, "%c", '\n');
						whitespace = 1;
						count = 0;
						bytes--;
					}else if(whitespace == 1){
						fprintf(w, "%c", '\n');
						whitespace = 0;
						bytes--;
					}

					while(line[index] != '\0'){

						if(line[index] == ' '){
							index++;
							bytes--;
							continue;
						}

						while(1 == 1){
							if(line[index] == '\n' || line[index] == '\0' || line[index] == ' ')break;
							a[len] = line[index];
							len++;
							index++;
							bytes--;
						}

						if((count == 0 && len + count <= width) || len + count + 1 <= width){
							if(count == 0){
								int x = 0;
								while(x < len){
									fprintf(w, "%c", a[x]);
									x++;
								}
								count += len;
							}else{
								int x = 0;
								fprintf(w, "%c", ' ');
								while(x < len){
									fprintf(w, "%c", a[x]);
									x++;
								}
								count += len+1;
							}
						}else{
							fprintf(w, "%c", '\n');
							int x = 0;
							while(x < len){
								fprintf(w, "%c", a[x]);
								x++;
							}
							count = len;
						}//end of the big count if statement
						
						memset(a, '\0', strlen(a));
						index++;
						bytes--;
						len = 0;

					}//end of nested line[index] while loop

				}//end of big fgets while loop

				
				fprintf(w, "%c", '\n');
				fclose(w);
				
			}else{//end of big if statement//////////////////////////////////////////////////////////////////////////////////////////////

				FILE* f = fopen(sd->d_name, "r");
				FILE* w = fopen(rename, "w");

				if(f == 0 || w == 0){
					perror("Error");
					fclose(f);
					fclose(w);
					return EXIT_FAILURE;
				}

				while(fgets(line, 10000, f) != NULL){
					
					index = 0;

					if(line[0] == '\n'){
						fprintf(w, "%c", '\n');
						whitespace = 1;
						count = 0;
					}else if(whitespace == 1){
						fprintf(w, "%c", '\n');
						whitespace = 0;
					}

					while(line[index] != '\0'){

						if(line[index] == ' '){
							index++;
							continue;
						}

						while(1 == 1){
							if(line[index] == '\n' || line[index] == '\0' || line[index] == ' ')break;
							a[len] = line[index];
							len++;
							index++;
						}

						if((count == 0 && len + count <= width) || len + count + 1 <= width){

							if(count == 0){
								int x = 0;
								while(x < len){
									fprintf(w, "%c", a[x]);
									x++;
								}
								count += len;
							}else{
								int x = 0;
								fprintf(w, "%c", ' ');
								while(x < len){
									fprintf(w, "%c", a[x]);
									x++;
								}
								count += len + 1;
							}
						}else{
						
							fprintf(w, "%c", '\n');
							int x = 0;
							while(x < len){
								fprintf(w, "%c", a[x]);
								x++;
							}
							count = len;
						}//end of big count if statement

						memset(a, '\0', strlen(a));
						index++;
						len = 0;

					}//end of nested line[index] while loop
					
				}//end of big fgets while loop
		
				fprintf(w, "%c", '\n');
				fclose(f);
				fclose(w);				

			}//end of big ifelse statement/////////////////////////////////////////////////////////////////////////////////////////////////////////

			//allows us to exit the directory;
			chdir("..");

			memset(rename, '\0', 50);

		}//end of file while loop

		closedir(dir);
		
	}//end else
	return EXIT_SUCCESS;
}//end main






