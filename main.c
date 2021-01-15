#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "MDS.h"
// #include "functions.h"
 
int main(void){
    int fd = 0;
    size_t buffer = 0;
    char* buff = NULL;
    char* tok;
    char* readnamebuff;
    readnamebuff = malloc(10 * sizeof(char));
    int BlockSize;
    int FilenameSize = 0;
    int MaxFileSize;
    int MaxDirFileNum;
    int closingFlag = 1;

    ///////// CREATE NAV
    Nav* nav = malloc(sizeof(Nav));

    infonodePtr stack = list_create();

    PrintMenu();
    getline(&buff, &buffer, stdin);
    tok = strtok(buff," \n");
    while(1){
        if (strcmp(tok,"cfs_create") == 0){
            char* name = NULL;
            while((tok = strtok(NULL," \n")) != NULL){
                if (strcmp(tok,"-bs") == 0){
                    BlockSize = atoi(strtok(NULL," \n"));
                }
                else if (strcmp(tok,"-fns") == 0){
                    FilenameSize = atoi(strtok(NULL," \n"));
                }
                else if (strcmp(tok,"-cfs") == 0){
                    MaxDirFileNum = atoi(strtok(NULL," \n"));
                }
                else if (strcmp(tok,"-mdfn") == 0){
                    MaxFileSize = atoi(strtok(NULL," \n"));
                }
                else{
                    name = tok;
                }
            }
            if((MaxFileSize%BlockSize) == 0){
                if(name == NULL){
                    cfs_create("myfiles.cfs",BlockSize,FilenameSize,MaxFileSize,MaxDirFileNum);
                    fd = cfs_workwith("myfiles.cfs");
                }
                else{
                    cfs_create(name,BlockSize,FilenameSize,MaxFileSize,MaxDirFileNum);
                    fd = cfs_workwith(name);
                }
                lseek(fd,0,SEEK_SET);
                nav->curNodeid = 1;
                nav->parent_nodeid = -1;
                nav->curAddress = 2*BlockSize;

                ////////// PRINT 
                /*SuperBlock* block = malloc(sizeof(SuperBlock));
                read(fd,block,sizeof(SuperBlock));
                printf("Block:\n\tBlockSize:%d\n",block->blockSize);

                free(block);
                */
                closingFlag = 0;
            }
            else{
                printf("BAD INPUT - TRY AGAIN !!\n");
            }

        }

        else if(strcmp(tok, "cfs_workwith") == 0){
            char* name = NULL;
            name = NULL;
            name = strtok(NULL, " \n");
            if(name == NULL){
                fd = cfs_workwith("myfiles.cfs");
            }
            else{
                fd = cfs_workwith(name);
            }
            lseek(fd,0,SEEK_SET);

                        ////////// PRINT 
            SuperBlock* block = malloc(sizeof(SuperBlock));
            read(fd,block,sizeof(SuperBlock));
            // printf("Block:\n\tBlockSize:%d\n",block->blockSize);

            BlockSize = block->blockSize;
            FilenameSize = block->filenameSize;
            MaxFileSize = block->maxfileSize;
            MaxDirFileNum = block->maxDirFileNum;

            nav->curNodeid = 1;
            nav->parent_nodeid = -1;
            nav->curAddress = 2*BlockSize;

            int listElems = 0;
            unsigned int listAddr;

            lseek(fd, BlockSize, SEEK_SET);
            read(fd, &listElems, sizeof(int));
            while(listElems > 0){
                read(fd, &listAddr, sizeof(unsigned int));
                list_pushlast(stack, listAddr);
                listElems--;
            }

            free(block);
            closingFlag = 0;
        }

        else if(strcmp(tok, "cfs_mkdir") == 0){
            tok = strtok(NULL, " \n");
            while(tok != NULL){
                cfs_mkdir(fd, tok, nav,stack);
                tok = strtok(NULL, " \n");
            }
            // char path[10] = "/mike";

            // printf(">>>>> %u\n",abs_or_rel(fd,nav, path));
        }

        else if(strncmp(tok, "0",1) == 0){
            // printf("MPHKE\n");
            free(nav);
            free(readnamebuff);
            free(tok);
            break;
        }

        else if(strcmp(tok,"cfs_ls")==0){
            tok = strtok(NULL," \n");
            char hid = 'n';
            char rev = 'n';
            char sort = 'y';
            char list = 'n';
            char print = '~';
            char sc = 'n';


            char **scout = (char**)malloc(sizeof(char*)*(MaxDirFileNum));
            int temp = 0;
            while(temp < MaxDirFileNum){
                scout[temp++] = (char*)malloc(sizeof(char)*((FilenameSize+1)));
            }
            temp = 0;

            while(tok!=NULL){
                if(strcmp(tok,"-a") == 0){
                    hid = 'y';
                }
                else if(strcmp(tok,"-r") == 0){
                    rev = 'y';
                }
                else if(strcmp(tok,"-l") == 0){
                    list = 'y';
                }
                else if(strcmp(tok,"-u") == 0){
                    sort = 'n';
                }
                else if(strcmp(tok,"-d") == 0){
                    print = 'd';
                }
                else if(strcmp(tok,"-h") == 0){
                    print = 'h';
                }
                else{
                    strcpy(scout[temp++], tok);
                    sc = 'y';
                }
                tok = strtok(NULL, " \n");
            }

            cfs_ls(fd,nav,hid,rev,sort,list,print,sc,scout, 0);
            temp = MaxDirFileNum - 1;
            while(temp >= 0){
                free(scout[temp--]);
            }
            free(scout);
            printf("\n");
        }

        else if(strcmp(tok, "cfs_pwd") == 0){
            cfs_pwd(fd,nav);
        }


        else if (strcmp(tok,"cfs_touch") == 0){
            char** filename = malloc(MaxDirFileNum*(sizeof(char*)));
            int* flags = malloc(2 * sizeof(int));                           //flags[0] = aflag flags[1] = mflag
            flags[0] = 0; flags[1] = 0;
            int temp = 0;
            while((tok = strtok(NULL," \n")) != NULL){
                if (strcmp(tok,"-a") == 0){
                    flags[0] = 1;
                }
                else if (strcmp(tok,"-m") == 0){
                    flags[1] = 1;
                }
                else{
                    filename[temp++] = strdup(tok);
                }
            }
            int count = 0;
            while(count < temp){
                cfs_touch(fd,filename[count++],nav,stack,flags);
            }

            temp--;
            while(temp >= 0){
                free(filename[temp--]);
            }
            free(filename);
            free(flags);
        }

        else if(strcmp(tok, "cfs_mv") ==0){

            char **files = (char**)malloc(sizeof(char*)*(MaxDirFileNum));
            int temp = 0;
            while(temp < MaxDirFileNum){
                files[temp++] = (char*)malloc(sizeof(char)*((99+1)));
            }
            temp = 0;

            char q = 'n';
            tok = strtok(NULL, " \n");
            while(tok != NULL){
                if(strcmp(tok, "-i") == 0){
                    q = 'y';
                    tok = strtok(NULL, " ");
                }
                else{
                    while(tok != NULL){
                        strcpy(files[temp++], tok);
                        tok = strtok(NULL, " \n");
                        // printf(">%s\n", files[temp-1]);
                    }
                }
            }
            cfs_mv(fd,nav,files,temp,q);
            // printf("vghke \n");
            temp = MaxDirFileNum - 1;
            while(temp >= 0){   
                free(files[temp--]);
            }
            free(files);
            fflush(0);
        }

        else if (strcmp(tok,"cfs_cd") == 0){
            char* path = strtok(NULL," \n");
            cfs_cd(fd,nav,path);
        }

        else if(strcmp(tok, "cfs_ln") == 0){

            tok = strtok(NULL, " \n");

            char *source = malloc(FilenameSize + 1);
            char *dest = malloc(FilenameSize + 1);

            strcpy(source, tok);
            tok = strtok(NULL, " \n");

            if(strlen(tok) <= FilenameSize){
                strcpy(dest, tok);
                cfs_ln(fd, nav, source, dest);
            }
            else{
                printf("Invalid Size Of Name - TOO BIG\n");
            }

            free(source);
            free(dest);
        }

        else if(strcmp(tok, "cfs_rm") == 0){
            tok = strtok(NULL, " \n");
            int flag = 0;

            char **dests = (char**)malloc(sizeof(char*)*(MaxDirFileNum));
            char r = 'n';
            char q = 'n';
            int temp = 0;

            while(tok != NULL){
                if(strcmp(tok, "-i") == 0){
                    q = 'y';
                }
                else if(strcmp(tok, "-r") == 0){
                    r = 'y';
                }
                else{
                    if(temp < MaxDirFileNum){
                        dests[temp] = strdup(tok);
                    // printf("dests[%d] = %s\n", temp, dests[temp]);
                        temp++;
                    }
                    else{
                        flag = 1;
                        break;
                    }
                }
                tok = strtok(NULL, " \n");
            }

            if(flag == 0){
                cfs_rm(fd, nav, stack, dests, temp-1, 0, q, r);
            }
            else{
                printf("Error ! - Too many arguments given !!\n");
            }
            // printf("VGHKE !!!!!!!\n");

            temp--;
            while(temp >= 0){
                free(dests[temp--]);
            }
            free(dests);
        }

        else if(strcmp(tok , "cfs_cp") == 0){
            char r = 'n';
            char q = 'n';
            char R = 'n';

            char** sources = (char**)malloc(MaxDirFileNum*sizeof(char*));
            int filesNum = 0;

            tok = strtok(NULL, " \n");
            while(tok != NULL){
                if(strcmp(tok , "-R") == 0){
                    R = 'y';
                }
                else if(strcmp(tok, "-i") == 0){
                    q= 'y';
                }
                else if(strcmp(tok, "-r") == 0){
                    r = 'y';
                }
                else{
                    sources[filesNum] = strdup(tok);
                    filesNum++;
                }

                tok = strtok(NULL, " \n");
            }
            cfs_cp(fd, nav, nav, stack, sources, filesNum-1, 0, r, q, R);
            filesNum--;
            while(filesNum >= 0){
                free(sources[filesNum--]);
            }
            free(sources);
        }


       else if(strcmp(tok,"cfs_import") == 0){
            char* source = NULL;
            char* destination = NULL;

            char** sources = malloc(MaxDirFileNum*sizeof(char*));
            int temp = 0;
            // printf("%s %s\n",source,destination);
            tok = strtok(NULL, " \n");
            while(tok != NULL){
                sources[temp] = strdup(tok);
                tok = strtok(NULL, " \n");
                temp++;
            }

            temp--;
            int count = 0;
            while(count < temp && temp >= 1){
                destination = strdup(sources[temp]);
                source = strdup(sources[count++]);
                // printf("source: %s\n",source);
                cfs_import(fd,source,destination,stack,nav);
                free(destination);
            }

            while(temp >= 0){
                free(sources[temp--]);
            }
            free(sources);
            free(source);
        }


        else if (strcmp(tok,"cfs_export") == 0){char* source = NULL;
            char* destination = NULL;

            char** sources = malloc(MaxDirFileNum*sizeof(char*));
            int temp = 0;
            // printf("%s %s\n",source,destination);
            tok = strtok(NULL, " \n");
            while(tok != NULL){
                sources[temp] = strdup(tok);
                tok = strtok(NULL, " \n");
                temp++;
            }

            temp--;
            int count = 0;
            
            while(count < temp && temp >= 1){
                destination = strdup(sources[temp]);
                source = strdup(sources[count++]);
                // printf("source: %s\n",source);
                cfs_export(fd,source,destination,stack,nav);
                free(destination);

            }

            while(temp >= 0){
                free(sources[temp--]);
            }
            free(sources);
            free(source);
        }

        else if(strcmp(tok, "cfs_cat") == 0){
            char* dest = malloc(FilenameSize);
            char** sources = malloc(MaxDirFileNum*sizeof(char*));

            tok = strtok(NULL, " \n");
            int temp = 0;
            while(tok != NULL){
                if(strcmp(tok, "-o") == 0){
                    tok = strtok(NULL, " \n");
                    strcpy(dest, tok);
                }
                else if(temp <= MaxDirFileNum){
                    sources[temp] = strdup(tok);
                    temp++;
                }
                tok = strtok(NULL, " \n");
            }

            temp--;
            
            cfs_cat(fd, sources, dest, stack, nav, temp);
            
            while(temp >= 0){
                free(sources[temp--]);
            }
            free(sources);
            free(dest);
        }

        else if(strcmp(tok, "cfs_close") == 0){
            cfs_close(fd,stack);
            closingFlag = 1;
            close(fd);
        }
        
        else{
            printf("BAD INPUT - TRY AGAIN !!\n");
        }

        PrintMenu();
        // list_print(stack);
        getline(&buff, &buffer, stdin);
        tok = strtok(buff," \n");
    }
    // PrintMenu();
    if(closingFlag == 0){
        cfs_close(fd,stack);
        close(fd);
    }
    // free(nav);
    // close(fd);
    free(stack);
}