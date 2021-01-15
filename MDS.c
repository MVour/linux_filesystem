#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "MDS.h"
#define PERMS 0666

int cfs_workwith(char* cfsfile){
    int fd;
    int flags = O_RDWR;
    if ((fd = open(cfsfile,flags,PERMS)) == -1){
        write(2,"Ι've not created the file",sizeof("I've not created your file"));
        return 1;
    }
    else {
        write(1,"I've opened your file\n",sizeof("I've opened your file\n"));
    }
    return fd;
}

int cfs_create(char* filepath,int block_size,int filename_size,int maxfile_size,int maxdirectoryfile_number){

    /////////// CREATE SUPER BLOCK

    int fd = 0;
    int flags = O_RDWR | O_CREAT;
       
    if ((fd = open(filepath,flags,PERMS)) == -1){
        write(2,"Error creating file\n",sizeof("Error creating file\n"));
        exit(2);
    }
    SuperBlock* super = malloc(sizeof(SuperBlock));
    super->blockSize = block_size;
    super->filenameSize = filename_size;
    super->maxDirFileNum = maxdirectoryfile_number;
    super->maxfileSize = maxfile_size;
    super->idCount = 0;
    super->block_tail = 3 * block_size;
    super->root_adress = 2*block_size;
    write(fd,super,sizeof(SuperBlock)); //i creating superblock
   
    ////////// CREATE ROOT

    MDS* myDir = (MDS*)malloc(sizeof(MDS));
    myDir->nodeid = (super->idCount++)+1;
    myDir->size = 0;
    myDir->type = 0;
    myDir->elems = 0;
    myDir->parent_nodeid = -1;
    time(&myDir->creation_time);
    // printf("Creation Time: %s\n",asctime(localtime(&myDir->creation_time)));
    time(&myDir->access_time);
    time(&myDir->modification_time);
    lseek(fd, 2*block_size, SEEK_SET);
    write(fd,myDir,sizeof(MDS));
    write(fd,"/",super->filenameSize);


    ///////// FREE MEM
    free(myDir);
    free(super);
    close(fd);
    return 1;
   }


int cfs_mkdir(int fd, char* dirName,Nav* navigation,infonodePtr holelist){
    unsigned int to_write;
    lseek(fd,0,SEEK_SET);                                                   //initialise fd at start of file
    SuperBlock* super = malloc(sizeof(SuperBlock));                          //get superblock
    read(fd,super,sizeof(SuperBlock));

    if (strlen(dirName) + 1 > super->filenameSize){                                             //check dirname length
        printf("%d",super->filenameSize);
        printf("Name is too big.\n");
        free(super);
        return -1;
    }
    MDS* parentDir = malloc(sizeof(MDS));                                                               //get parentDir
    lseek(fd,navigation->curAddress,SEEK_SET);
    read(fd,parentDir,sizeof(MDS));
    if (parentDir->elems + 1 > super-> maxDirFileNum){
        printf("Error,cant create file in this folder ");
        // printf("elems: %d, max NUM:%d\n",parentDir->elems, super->maxDirFileNum);
        free(parentDir);
        free(super);
        return -1;
    }
    MDS* myDir = malloc(sizeof(MDS));
    myDir->nodeid = (super->idCount++)+1;


    myDir->size = 0;
    myDir->type = 0;
    myDir->elems = 0;
    myDir->parent_nodeid = navigation->curNodeid;
    myDir->parent_address = navigation->curAddress;
    time(&myDir->creation_time);
    // printf("Creation Time: %s\n",asctime(localtime(&myDir->creation_time)));
    time(&myDir->access_time);
    time(&myDir->modification_time);
    if (list_returnsize(holelist)){
        to_write = list_popfirst(holelist);
    }
    else {
        to_write = super->block_tail;
        super->block_tail = super->block_tail + super->blockSize;
    }
    // printf("TO WRITE: %u\n",to_write);
    lseek(fd,to_write,SEEK_SET);
    write(fd,myDir,sizeof(MDS));                    //write directory's metadata
    write(fd,dirName,super->filenameSize);          //write directory's name
    //we need to inform parent and save new forlders metadata
   // parentDir->size = parentDir->size + super->blockSize;
    parentDir->elems = parentDir->elems + 1;
    unsigned int Metadatasave = navigation->curAddress + sizeof(MDS) + super->filenameSize + ((parentDir->elems) - 1) * (sizeof(unsigned int)+super->filenameSize);

    lseek(fd,Metadatasave,SEEK_SET);                        //seek the array spot to write
    // printf("METADATA: %u\n", Metadatasave);
    write(fd,&to_write,sizeof(unsigned int));                //write adress
    write(fd, dirName, super->filenameSize);

    lseek(fd,navigation->curAddress,SEEK_SET);              //update parent directory's info
    write(fd,parentDir,sizeof(MDS));
    lseek(fd,0,SEEK_SET);       //rewind pointer back to start
    write(fd,super,sizeof(SuperBlock));

    free(super);
    free(myDir);
    free(parentDir);
    return to_write;
}

void cfs_ls(int fd, Nav* nav, char hid, char rev, char sort, char list, char print,char sc, char** scout, int depth){
    /////// PRINT GIVEN ARGS
    // printf("files r: %s\n",file);


    ////// GET FILENAME SIZE
    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    lseek(fd,0,SEEK_SET);
    read(fd,mySuper,sizeof(SuperBlock));

    /////// GET A COPY OF DIR
    MDS* myDir = malloc(sizeof(MDS));
    lseek(fd,nav->curAddress,SEEK_SET);
    read(fd,myDir,sizeof(MDS));

        /*      // FOR VERTIFY
            // check dirs name
        char *dirname = malloc(mySuper->filenameSize+1);
        lseek(fd,nav->curAddress + sizeof(MDS), SEEK_SET);
        read(fd, dirname, mySuper->filenameSize);
        printf("dirs name: %s\n", dirname);
        free(dirname);
 
        */

    /////// CHECK FOR EMPTY FOLDER
    if(myDir->elems == 0){
        free(myDir);
        free(mySuper);
        printf("Folder Is Empty !!\n");
        return;
    }

    // printf("ELEMENTS: %u\n", myDir->elems);

    /////// MAKE AN ARRAY WITH FILES ADDRESSES
    unsigned int *myArr = malloc(myDir->elems*sizeof(unsigned int));

    char **filenames = (char**)malloc(sizeof(char*)*(myDir->elems));
    int temp = 0;
    while(temp < myDir->elems){
        filenames[temp++] = (char*)malloc(sizeof(char)*((mySuper->filenameSize)+1));
    }

    ////// FILL THE ARRAY || fd is at dir !!
    int elemsCount = 0;
    lseek(fd, nav->curAddress + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);
    while(elemsCount < myDir->elems){
        read(fd, &myArr[elemsCount], sizeof(unsigned int));
        read(fd, filenames[elemsCount++], mySuper->filenameSize);
        // lseek(fd, mySuper->filenameSize, SEEK_CUR); //move fd at data blocks
    }
    // NOTE: read() moves the fd !!!

    // PRINT FILES FROM ARRAYS ADDRESSES
    unsigned int myOffset;

    // printf("TI SKATA SYMVAINEI \n");

    // ARRAY USED FOR SORTING 
    


    MDS **myInfo = (MDS**)malloc(sizeof(MDS*)*(myDir->elems));
    temp = 0;
    while(temp < myDir->elems){
        myInfo[temp++] = (MDS*)malloc(sizeof(MDS));
    }


    if(hid == 'n' || hid == 'y'){
        elemsCount = 0;
        while(elemsCount < myDir->elems){
            myOffset = myArr[elemsCount];

            lseek(fd, myOffset, SEEK_SET);
            read(fd, myInfo[elemsCount++], sizeof(MDS));
            // read(fd, filenames[elemsCount++], mySuper->filenameSize);

            if(sort == 'y'){
                int cur = elemsCount-1;
                char *tempstr = malloc(mySuper->filenameSize+1);
                MDS *tempMds = malloc(sizeof(MDS));
                while(cur > 0){
                    if(strcmp(filenames[cur], filenames[cur-1]) < 0){
                        strcpy(tempstr,filenames[cur-1]);
                        strcpy(filenames[cur-1], filenames[cur]);
                        strcpy(filenames[cur], tempstr);
                        
                        memcpy(tempMds, myInfo[cur-1], sizeof(MDS));
                        memcpy(myInfo[cur-1], myInfo[cur], sizeof(MDS));
                        memcpy(myInfo[cur], tempMds, sizeof(MDS));
                    }
                    cur--;
                }
                free(tempstr);
                free(tempMds);
            }
        }
    }


    //////// PRINT DIR'S ELEMS
    elemsCount = 0;
    while(elemsCount < myDir->elems){
        if(hid == 'n' && filenames[elemsCount][0] == '.'){
            elemsCount++;
            continue;
        }
        if(sc == 'y'){
            temp = 0;
            int flag = 0;
            while(temp < myDir->elems){
                if(strcmp(scout[temp++], filenames[elemsCount]) ==0){
                    flag = 1;
                    break;
                }
            }
            if(flag == 0){
                elemsCount++;
                continue;
            }
        }
        if(print != '~'){
            if(print == 'd' && myInfo[elemsCount]->type != 0){
                elemsCount++;
                continue;
            }
            else if(print == 'h' && myInfo[elemsCount]->linkCounter <= 1){
                elemsCount++;
                continue;
            }
        }
        int skatoules = 0;
        while(skatoules < depth){
            printf("\t");
            skatoules++;
        }
        if(list == 'y'){
            printf("\n");
            
            printf("CrT:%s ",asctime(localtime(&myInfo[elemsCount]->creation_time)));
            printf("AcT:%s ",asctime(localtime(&myInfo[elemsCount]->access_time)));
            printf("MoT:%s ",asctime(localtime(&myInfo[elemsCount]->modification_time)));
        }
        printf("Size: %u ", myInfo[elemsCount]->size);
        printf("Type: %u ", myInfo[elemsCount]->type);
        printf("%s\n",filenames[elemsCount++]);
    }

    //// / SHEARCH FOR -R AND CALL CFS_LS
    //// void cfs_ls(int fd, Nav* nav, char hid, char rev, char sort, char list, char print,char sc, char** scout)

    if(rev == 'y'){
        elemsCount = 0;
        while(elemsCount < myDir->elems){
            if(myInfo[elemsCount]->type == 0){
                Nav* tempNav = malloc(sizeof(Nav));
                int skata = 0;
                // lseek(fd, 0, SEEK_SET);
                char* target = malloc(mySuper->filenameSize);
                while(skata < myDir->elems){
                    lseek(fd, myArr[skata] + sizeof(MDS), SEEK_SET);
                    read(fd, target, mySuper->filenameSize);
                    if(strcmp(target, filenames[elemsCount]) == 0){
                        break;
                    }
                    skata++;
                }
                tempNav ->curAddress = myArr[skata];
                printf("\n");
                int skatoules = 0;
                while(skatoules <= depth){
                    printf("\t");
                    skatoules++;
                }
                printf("/%s :\n", filenames[elemsCount]);
                cfs_ls(fd, tempNav, hid, rev, sort, list, print, sc, scout, depth+1);
                // printf("\n");
                free(tempNav);
                free(target);
            }
            elemsCount++;
        }
    }

    // printf("\n");

    /////////// FREE MEM
    temp = myDir->elems - 1;
    while(temp >= 0){
        // printf("temp: %d\n", temp);
        free(filenames[temp]);
        // printf("otinanai\n");
        free(myInfo[temp--]);
    }

    free(filenames);
    free(myInfo);
    free(myArr);
    free(mySuper);
    free(myDir);

}

void cfs_pwd(int fd, Nav* nav){
    lseek(fd, 0 ,SEEK_SET);

    SuperBlock *mySuper = malloc(sizeof(SuperBlock));
    read(fd, mySuper, sizeof(SuperBlock));

    MDS* myMds = malloc(sizeof(MDS));
    unsigned int addr = nav->curAddress;
    int depth = 1;

    lseek(fd, nav->curAddress, SEEK_SET);
    read(fd, myMds, sizeof(MDS));
    char *name = malloc(mySuper->filenameSize+1);
    read(fd, name, mySuper->filenameSize);
    // printf(">>%s, address: %u\n", name, nav->curAddress);

        // COUNT DIRS ABOVE
    while(strcmp(name, "/") != 0){
        addr = myMds->parent_address;
        lseek(fd, addr, SEEK_SET);
        depth++;
        read(fd, myMds, sizeof(MDS));
        read(fd, name, mySuper->filenameSize);
    }

    char **dirNames = (char**)malloc(sizeof(char*)*depth);
    int temp = 0;
    while(temp < depth){
        dirNames[temp++] = (char*)malloc(sizeof(char)*((mySuper->filenameSize)+1));
    }

    lseek(fd, nav->curAddress, SEEK_SET);
    read(fd, myMds, sizeof(MDS));
    read(fd, name, mySuper->filenameSize);
    strcpy(dirNames[0],name);
    // printf("TO GAMIDI EDW PERA EINAI: %s\n", dirNames[0]);

        // READ ALL DIRS ABOVE NAMES
    temp = 1;
    while(temp < depth-1){
        // printf("depth: %d\n", depth);
        addr = myMds->parent_address;
        lseek(fd, addr, SEEK_SET);
        read(fd, myMds, sizeof(MDS));
        read(fd, name, mySuper->filenameSize);
        // printf(">%s\n", name);
        strcpy(dirNames[temp++], name);
    }

    // printf("/");
    // printf("%s",dirNames[0]);
    temp = depth-2;
    while(temp >= 0){
        // printf("temp: %d\n", temp);
        // if(temp == depth-1){
            // printf("%s", dirNames[temp--]);
        // }
        // else
            printf("/%s", dirNames[temp--]);
    }
    if(depth == 1){
        printf("/\n");
    }
    printf("\n");

    depth--;
    while(depth >= 0){
        free(dirNames[depth--]);
    }

    free(dirNames);
    free(myMds);
    free(name);
    free(mySuper);
}



unsigned int find_path(int fd,unsigned int from_Adress,char* path,char* mem,int files_or_not,int indicator){
    char* mem_;
    mem_ = mem;
    char* newpath;
    SuperBlock super;
    MDS curr_metadata;
    unsigned int retval;

    lseek(fd,0,SEEK_SET);
    read(fd,&super,sizeof(SuperBlock));
    char* name = malloc(super.filenameSize * sizeof(char)); 
    lseek(fd,from_Adress,SEEK_SET);
    read(fd,&curr_metadata,sizeof(MDS));

    if ((curr_metadata.type == 1) && (files_or_not == 1)){ //in case we dont want to find if a file exists in our directory indicator = 1
        free(name);
        return -1;
    }
    unsigned int adresses[curr_metadata.elems];
    read(fd,name,super.filenameSize);

    int elemsCount = 0; // TO FD EINAI HDH META TO NAME
    // lseek(fd, from_Adress + sizeof(MDS) + super->filenameSize, SEEK_SET);
    while(elemsCount < curr_metadata.elems){
        read(fd, &adresses[elemsCount++], sizeof(unsigned int));
        lseek(fd, super.filenameSize, SEEK_CUR); //move fd at data blocks
    }
    // read(fd,adresses,sizeof(adresses));
    
    for (int i = 0 ; i < curr_metadata.elems ; i++ ){
        // printf("add: %d\n",adresses[i]);
    }
    
    if (from_Adress == super.root_adress){ //in case we search from the root adress
        while ( (path != NULL) &&  ((strcmp(path,"..") == 0) || (strcmp(path,".") == 0)) ) {        //in case of .. and . in root folder nothing changes so we just proceed to the next token
            path = strtok_r(NULL,"/",&mem_);

        }
        if (path == NULL){                                                                                  //if there are no other tokens after all is null then we return the root adress
            free(name);
            return from_Adress;
        }
        for (int i = 0 ; i < curr_metadata.elems ; i++){                                            //in case there are more folders to check
            retval = find_path(fd,adresses[i],path,mem_,files_or_not,0);
            if (retval != -1){
                free(name);
                return retval;
            }
        }
        free(name);
        return -1;
    }

//in case we search from an adress other than the root
   while ((path != NULL) && (strcmp(path,".") == 0)){               //in case we face . token
        path = strtok_r(NULL,"/",&mem_);
    }

    if ((path != NULL) && (strcmp(path,"..") == 0)){                    //In case we need to go back up a folder
        newpath = strtok_r(NULL,"/",&mem_);
        return retval = find_path(fd,curr_metadata.parent_address,newpath,mem_,files_or_not,0);
    }
    
    if (path == NULL){
        free(name);
        return from_Adress;
    }

    if ((strcmp(path,name) == 0) || (indicator == 1)){
        if (!indicator)
            newpath = strtok_r(NULL,"/",&mem_);
        else newpath = path;
        while ((newpath != NULL) && (strcmp(newpath,".") == 0)){
            newpath = strtok_r(NULL,"/",&mem_);
        }

        if ((newpath != NULL) && (strcmp(newpath,"..") == 0)){
            newpath = strtok_r(NULL,"/",&mem_);
            retval = find_path(fd,curr_metadata.parent_address,newpath,mem_,files_or_not,0);
            return retval;
        }
        if (newpath == NULL){
            free(name);
            return from_Adress;
        }

        for (int i = 0 ; i < curr_metadata.elems ; i++){                        //if we found the folder and the path keeps on going we 
            retval = find_path(fd,adresses[i],newpath,mem_,files_or_not,0);
            if (retval != -1){
                free(name);
                return retval;
            }
        }
    }
    free(name);
    return -1;
}

unsigned int abs_or_rel(int fd,Nav* nav,char* path,int findfile){
    char* mem;
    char* newpath;
    SuperBlock super;
    int retval;

    char* weird = strdup(path);
        

    lseek(fd,0,SEEK_SET);
    read(fd,&super,sizeof(SuperBlock));
    if (weird == NULL){
        printf("The path you gave me is empty");
        free(weird);
        return -1;
    }
    if (weird[0] == '/'){                //ABSOLUTE PATH
        newpath = strtok_r(weird,"/",&mem);
        retval = find_path(fd,super.root_adress,newpath,mem,findfile,0);
        free(weird);
        return retval;
    }
    else {
        newpath = strtok_r(weird,"/",&mem);
        retval = find_path(fd,nav->curAddress,newpath,mem,findfile,1);
        free(weird);
        return retval;                   //RELATIVE PATHc
    }
}



int cfs_touch(int fd,char* filename,Nav* navigation,infonodePtr holelist,int* flags){
    
    int addr;
    if ((addr = abs_or_rel(fd,navigation,filename,0)) != -1){
        MDS* file_exists = malloc(sizeof(MDS));
        lseek(fd,addr,SEEK_SET);
        read(fd,file_exists,sizeof(MDS));
        if (flags[0]){
            time(&file_exists->access_time);
        }
        if (flags[1]){
            time(&file_exists->modification_time);
        }
        lseek(fd,addr,SEEK_SET);
        write(fd,file_exists,sizeof(MDS));  
        free(file_exists);
        printf("I've updated the file %s\n",filename);
        return -1;   
    }

    unsigned int to_write;
    lseek(fd,0,SEEK_SET);                                       //initialise fd at start of file
    SuperBlock* super = malloc(sizeof(SuperBlock));             //get superblock
    read(fd,super,sizeof(SuperBlock));

    if (strlen(filename) + 1 > super->filenameSize){            //check dirname length
        printf("%d",super->filenameSize);
        printf("Name is too big.\n");
        free(super);
        exit(2);
    }
    MDS* parentDir = malloc(sizeof(MDS));                       //get parentDir
    lseek(fd,navigation->curAddress,SEEK_SET);
    read(fd,parentDir,sizeof(MDS));


    if (parentDir->elems + 1 > super-> maxDirFileNum){
        printf("Error,cant create file in this folder ");
        free(parentDir);
        free(super);
        return -1;
    }
    MDS* myfile = malloc(sizeof(MDS));
    myfile->nodeid = (super->idCount++)+1;

    myfile->size = 0;
    myfile->type = 1;
    myfile->elems = 0;
    myfile->parent_nodeid = navigation->curNodeid;
    myfile->parent_address = navigation->curAddress;
    time(&myfile->creation_time);
    // printf("Creation Time: %s\n",asctime(localtime(&myDir->creation_time)));
    time(&myfile->access_time);
    time(&myfile->modification_time);
    if (list_returnsize(holelist)){
        to_write = list_popfirst(holelist);
    }
    else {
        to_write = super->block_tail;
        super->block_tail = super->block_tail + super->blockSize;
    }
    lseek(fd,to_write,SEEK_SET);
    write(fd,myfile,sizeof(MDS));                    //write file's metadata
    write(fd,filename,super->filenameSize);          //write file's name
    //we need to inform parent and save new file's metadata
    //parentDir->size = parentDir->size + super->blockSize;
    parentDir->elems = parentDir->elems + 1;
    unsigned int Metadatasave = navigation->curAddress + sizeof(MDS) + super->filenameSize + ((parentDir->elems) - 1) * (sizeof(unsigned int)+super->filenameSize);
    lseek(fd,Metadatasave,SEEK_SET);                        //seek the array spot to write
    write(fd,&to_write,sizeof(unsigned int));                //write adress
    write(fd, filename, super->filenameSize);


    lseek(fd,navigation->curAddress,SEEK_SET);              //update parent directory's info
    write(fd,parentDir,sizeof(MDS));            
    lseek(fd,0,SEEK_SET);       //rewind pointer back to start
    write(fd,super,sizeof(SuperBlock));

    free(parentDir);
    free(super);
    free(myfile);
    return to_write;
}


void cfs_mv(int fd, Nav* nav, char** files, int filesNum, char q){
    int count = 0;
    /*while(count < filesNum){
        printf(">%s\n",files[count++]);
    }*/

    ////////////// GET SUPER BLOCK
    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    lseek(fd, 0 ,SEEK_SET);
    read(fd, mySuper, sizeof(SuperBlock));

    ///////////// GET CURR DIR
    MDS* myDir = malloc(sizeof(MDS));
    lseek(fd, nav->curAddress, SEEK_SET);
    read(fd, myDir, sizeof(MDS));

    ///////////// MALLOC 2 ARRAYS -> 1 OF ADDRESSES, 1 OF NAMES
    count = 0;
    char** DirNames = (char**)malloc(sizeof(char*)*(myDir->elems));
    while(count < myDir->elems){
        DirNames[count++] = malloc(sizeof(char)*(mySuper->filenameSize + 1));
    }
    int malloced = myDir->elems-1;

    unsigned int* DirAddr = malloc(sizeof(unsigned int)*(myDir->elems));
    

        /// FILL THEM
    lseek(fd, mySuper->filenameSize, SEEK_CUR);
    count = 0;
    while(count < myDir->elems){
        read(fd, &DirAddr[count], sizeof(unsigned int));
        read(fd, DirNames[count], mySuper->filenameSize);
        // printf("Addr: %u, Name: %s\n", DirAddr[count], DirNames[count]);
        count++; 
    }


    ////////// CHECK FOR RE-NAME
    int rename = 0; //set as false
    unsigned int targetDir = abs_or_rel(fd,nav,files[filesNum-1],1);
    if(targetDir == -1){
        rename = 1;  // rename is needed !!
        if(filesNum > 2){
            printf("Thers is no Dir '%s' !\n",files[filesNum-1]);
            rename = 2;
        }
        else if(strlen(files[filesNum-1]) > mySuper->filenameSize + 1){
            printf("Invalid new Name - Too Long !!\n");
            rename = 2;
        }
    }
    // printf("rename: %d\n", rename);


    ///////// MALLOC 1 MDS STRUCT
    MDS* myMds = malloc(sizeof(MDS));

    ///////// RE-NAME CASE
    if(rename == 1){
        count = 0;
        while(count < myDir->elems){
            if(strcmp(DirNames[count], files[0]) == 0){
                if(q == 'y'){
                    char* user = malloc(4*sizeof(char));
                    printf("mv: overwrite '%s' ?\n", DirNames[count]);
                    read(0, user, 4*sizeof(char));
                    if(strncmp(user, "y", 1) != 0){
                        free(user);
                        break;
                    }
                    free(user);
                }
                        // CHANGE NAME AT MDS
                lseek(fd, DirAddr[count] + sizeof(MDS), SEEK_SET);
                write(fd, files[1], mySuper->filenameSize);
                        // CHANGE NAME AT DIR BLOCKS
                lseek(fd, nav->curAddress + sizeof(MDS) + mySuper->filenameSize , SEEK_SET);
                lseek(fd, count*(sizeof(unsigned int)+mySuper->filenameSize) + sizeof(unsigned int), SEEK_CUR);
                write(fd, files[filesNum-1], mySuper->filenameSize);

                // printf("DONE !!\n");

            }
            count++;
        }
    }
        //////////// MOVING CASE
    else if(rename == 0){
        count = 0;          ////// EKANA ALLAGH ANT GIA -2 -1 STHN KATW SEIRA
        while(count < filesNum-1){      // REAPEAT FOR EVERY GIVEN FILE !!!!
            int elemsCount = 0;
            while(elemsCount < myDir->elems){    // FIND FILE ADDR
                if(strcmp(DirNames[elemsCount], files[count]) == 0){
                    break;
                }
                elemsCount++;
            }
            unsigned int myAdr = abs_or_rel(fd,nav,files[count], 0);
            if(myAdr == -1){
                printf("There is no file '%s' !\n", files[count]);
                count++;
                continue;
            }
                // ADD THE FILE AT THE TARGET DIR
            MDS* nextDir = malloc(sizeof(MDS));
            lseek(fd, targetDir, SEEK_SET);
            read(fd, nextDir, sizeof(MDS));
                    //update the target dir stats
            if(nextDir->elems >= mySuper->maxDirFileNum){
                printf("Dir '%s' is full !!\n", files[filesNum-1]);
                free(nextDir);
                break;
            }

            if(q == 'y'){
                char* user = malloc(4*sizeof(char));
                printf("mv: overwrite '%s' ?\n", DirNames[count]);
                read(0, user, 4*sizeof(char));
                if(strncmp(user, "y", 1) != 0){
                    free(user);
                    break;
                }
                free(user);
            }

            nextDir->elems++;
            lseek(fd, targetDir, SEEK_SET);
            write(fd, nextDir, sizeof(MDS));
                    //write at dirs blocks
            lseek(fd, mySuper->filenameSize, SEEK_CUR);
            lseek(fd, (nextDir->elems -1)*(sizeof(unsigned int) + mySuper->filenameSize), SEEK_CUR);
            write(fd, &myAdr, sizeof(unsigned int));
            write(fd, files[count], mySuper->filenameSize);

                    //update File MDS
            lseek(fd, myAdr, SEEK_SET);
            read(fd, myMds, sizeof(MDS));
            myMds->parent_address = targetDir;
            lseek(fd, myAdr, SEEK_SET);
            write(fd, myMds, sizeof(MDS));
        
                //UPDATE ORIGINAL DIR

            lseek(fd, nav->curAddress, SEEK_SET);
            myDir->elems--;
            write(fd, myDir, sizeof(MDS));

                // OVER-WRITE DIR'S BLOCKS 
                    // find original position of the file in dir blocks
                    // THAT IS THE ELEMS_COUNT !!
                    // get the last file's in the dir name and addr
            unsigned int adrToCopy;
            char* nameToCopy = malloc(mySuper->filenameSize + 1);

            lseek(fd, mySuper->filenameSize + (myDir->elems)*(sizeof(unsigned int)+mySuper->filenameSize), SEEK_CUR);
            read(fd, &adrToCopy, sizeof(unsigned int));
            read(fd, nameToCopy, mySuper->filenameSize);

                // copy them at the original file's spot
            lseek(fd, nav->curAddress + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);
            lseek(fd, elemsCount*(sizeof(unsigned int)+mySuper->filenameSize), SEEK_CUR);
            write(fd, &adrToCopy, sizeof(unsigned int));
            write(fd, nameToCopy, mySuper->filenameSize);


            update_parent_sizes(fd, targetDir, myMds->size);    // + TARGET DIR SIZE
            update_parent_sizes(fd, nav->curAddress, -1*(myMds->size));                // - DIR SIZE

            free(nameToCopy);
            free(nextDir);
            count++;
        }
    }


        // FREE MEM
    count = malloced;
    while(count >= 0){
        free(DirNames[count--]);
    }
    free(DirNames);
    free(DirAddr);
    free(mySuper);
    free(myDir);
    free(myMds);
}

void cfs_ln(int fd, Nav *nav, char* source, char* dest){
    // printf("dest: %s\n", dest);
    // printf("source: %s\n", source);

    lseek(fd,0,SEEK_SET);
    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    read(fd,mySuper, sizeof(SuperBlock));

    unsigned int faddrs = abs_or_rel(fd,nav,source,0);
    if(faddrs == -1){
        printf("Thers is no file '%s' !\n", source);
        return;
    }

    MDS* myDir = malloc(sizeof(MDS));
    lseek(fd, nav->curAddress, SEEK_SET);
    read(fd, myDir, sizeof(MDS));


    // myDir->elems++;
    lseek(fd, mySuper->filenameSize + myDir->elems*(sizeof(unsigned int) + mySuper->filenameSize), SEEK_CUR);
    myDir->elems++;
    write(fd, &faddrs, sizeof(int));
    write(fd, dest, mySuper->filenameSize);
    lseek(fd, nav->curAddress, SEEK_SET);
    write(fd, myDir, sizeof(MDS));

        // UPDATE LINK COUNTER AT MDS
    MDS* myMds = malloc(sizeof(MDS));
    lseek(fd, faddrs, SEEK_SET);
    read(fd, myMds, sizeof(MDS));

    myMds->linkCounter++;

    lseek(fd, faddrs, SEEK_SET);
    write(fd, myMds, sizeof(MDS));


    free(myMds);
    free(mySuper);
    free(myDir);

}


void cfs_rm(int fd, Nav* nav, infonodePtr holelist, char** dests, int filesNum, int depth, char q ,char r){

    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    lseek(fd, 0 ,SEEK_SET);
    read(fd, mySuper, sizeof(SuperBlock));

    MDS* myDir = malloc(sizeof(MDS));
    lseek(fd, nav->curAddress, SEEK_SET);
    read(fd, myDir, sizeof(MDS));

    

    int count = 0;
    while(count <= filesNum){

        ///////// CHECK FOR VALID INPUT
        unsigned int exist = abs_or_rel(fd,nav,dests[count],0);
        if(exist == -1){
            printf("rm: Error !! - '%s' does not exist\n",dests[count]);
            count++;
            continue;
        }

        char **dirFileNames = (char**)malloc(sizeof(char*)*(myDir->elems)); 
        int elemsCount = 0;

        lseek(fd, nav->curAddress + sizeof(MDS), SEEK_SET);
        lseek(fd, mySuper->filenameSize + sizeof(unsigned int), SEEK_CUR);
        
        while(elemsCount < myDir->elems){
            dirFileNames[elemsCount] = malloc(mySuper->filenameSize);
            read(fd, dirFileNames[elemsCount], mySuper->filenameSize);
            lseek(fd, sizeof(unsigned int), SEEK_CUR);
            // printf("FName[%d]: %s\n", elemsCount, dirFileNames[elemsCount]);
            elemsCount++;
        }

       elemsCount = 0;
        while(elemsCount < myDir->elems){    // FIND FILE ADDR
            if(strcmp(dirFileNames[elemsCount], dests[count]) == 0){
                break;
            }
            elemsCount++;
        }

        //////// q FLAG
        if(q == 'y'){
            char* user = malloc(4*sizeof(char));
            printf("rm: delete '%s' ?\n", dests[count]);
            read(0, user, 4*sizeof(char));
            if(strncmp(user, "y", 1) != 0){
                free(user);
                elemsCount = myDir->elems - 1;
                while(elemsCount >= 0){
                    free(dirFileNames[elemsCount--]);
                }
                free(dirFileNames);
                count++;
                continue;
            }
        }


        unsigned int rmAddr = abs_or_rel(fd,nav,dests[count],1);
        if(rmAddr != -1){   ////////////////////////// ITS A DIR
            MDS* rmDir = malloc(sizeof(MDS));

            lseek(fd, rmAddr, SEEK_SET);
            read(fd, rmDir, sizeof(MDS));
            if(rmDir->elems != 0){
                if(r == 'y' || depth < 1){
                    Nav* nextNav = malloc(sizeof(Nav));
                    char** newDests = malloc(sizeof(char*)*(rmDir->elems));
                    int temp = 0;
                    lseek(fd, mySuper->filenameSize + sizeof(unsigned int), SEEK_CUR);
                    while(temp < rmDir->elems){
                        newDests[temp] = malloc(mySuper->filenameSize);
                        read(fd, newDests[temp], mySuper->filenameSize);
                        lseek(fd, sizeof(unsigned int), SEEK_CUR);
                        temp++;
                    }
                    temp--;
                    nextNav->curAddress = rmAddr;
                    cfs_rm(fd,nextNav,holelist,newDests,temp,depth+1,q,r);

                    
                    while(temp >= 0){
                        free(newDests[temp--]);
                    }
                    free(newDests);

                    if(r == 'n'){
                        free(rmDir);
                        elemsCount = myDir->elems - 1;
                        while(elemsCount >= 0){
                            free(dirFileNames[elemsCount--]);
                        }
                        free(dirFileNames);
                        count++;
                        continue;
                    }
                }
                else{
                    printf("rm: Error !! - '%s' is a Dir\n",dests[count]);
                    free(rmDir);
                    elemsCount = myDir->elems - 1;
                    while(elemsCount >= 0){
                        free(dirFileNames[elemsCount--]);
                    }
                    free(dirFileNames);
                    count++;
                    continue;
                }
            }
            free(rmDir);
        }
        else if(rmAddr == -1){  //////// ADD FILES DATABLOCKS AT LIST
            MDS* rmMds = malloc(sizeof(MDS));
            rmAddr = abs_or_rel(fd,nav,dests[count],0);
            lseek(fd, rmAddr, SEEK_SET);
            read(fd, rmMds, sizeof(MDS));
            lseek(fd, mySuper->filenameSize, SEEK_CUR);

            // update_parent_sizes(fd, rmAddr, -1*(rmMds->size));
            // printf("RM SIZE: %u\n", rmMds->size);

            if(rmMds->linkCounter > 1){
                rmMds->linkCounter--;
                lseek(fd, rmAddr, SEEK_SET);
                write(fd, rmMds, sizeof(MDS));
                free(rmMds);
                count++;
                continue;
            }
            unsigned int rmData;
            int otinanai = 0;
            while(otinanai < rmMds->elems){
                read(fd, &rmData, sizeof(unsigned int));
                lseek(fd, mySuper->filenameSize, SEEK_CUR);
                list_pushlast(holelist, rmData);
                otinanai++;
            }

            update_parent_sizes(fd, rmMds->parent_address, -1*(rmMds->size));

            free(rmMds);
        }
        //UPDATE ORIGINAL DIR
        lseek(fd, nav->curAddress, SEEK_SET);
        myDir->elems--;
        write(fd, myDir, sizeof(MDS));

            // OVER-WRITE DIR'S BLOCKS 
                // find original position of the file in dir blocks
                // THAT IS THE ELEMS_COUNT !!
                // get the last file's in the dir name and addr
        unsigned int adrToCopy;
        char* nameToCopy = malloc(mySuper->filenameSize + 1);

        lseek(fd, mySuper->filenameSize + (myDir->elems)*(sizeof(unsigned int)+mySuper->filenameSize), SEEK_CUR);
        read(fd, &adrToCopy, sizeof(unsigned int));
        read(fd, nameToCopy, mySuper->filenameSize);

            // copy them at the original file's spot
        lseek(fd, nav->curAddress + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);
        lseek(fd, elemsCount*(sizeof(unsigned int)+mySuper->filenameSize), SEEK_CUR);
        write(fd, &adrToCopy, sizeof(unsigned int));
        write(fd, nameToCopy, mySuper->filenameSize);

        list_pushlast(holelist, exist);



        elemsCount = myDir->elems - 1;
        while(elemsCount >= 0){
            free(dirFileNames[elemsCount--]);
        }
        free(dirFileNames);
        free(nameToCopy);

        count++;
    }


    
    free(mySuper);
    free(myDir);

}


void cfs_cp(int fd, Nav* srcNav, Nav* destNav, infonodePtr holelist, char** sources, int filesNum, int depth, char r, char q, char R){
    /*int trypes = 0;
    while(trypes <= filesNum){
        printf("sources[%d]: %s\n", trypes, sources[trypes]);
        trypes++;
    }*/

    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    lseek(fd, 0 , SEEK_SET);
    read(fd, mySuper, sizeof(SuperBlock));
    


    int count = 0;
    while(count < filesNum){
        
        unsigned int dirOrNOt = abs_or_rel(fd, srcNav, sources[count], 1);
        unsigned int destAddr = abs_or_rel(fd, destNav, sources[filesNum], 1);
        
        MDS* nextDir = malloc(sizeof(MDS));
        lseek(fd, destAddr, SEEK_SET);
        read(fd, nextDir, sizeof(MDS));


                // CHECK IF TARGET DIR IS FULL
        if(nextDir->elems >= mySuper->maxDirFileNum){
            // printf("cp: Error !! - not enough space in target Dir\n");
            free(nextDir);
            break;
        }

                // FIND SRC ADDR
        unsigned int srcAddr;
        if(dirOrNOt == -1)      // FILE CASE
            srcAddr = abs_or_rel(fd, srcNav, sources[count], 0);
        else                    // DIR CASE
            srcAddr = dirOrNOt;

        if(srcAddr == -1){
            printf("cp: Error !! - '%s' does not exist\n", sources[count]);
            free(nextDir);
            count++;
            continue;
        }

                // READ SRC MDS + NAME
        MDS* myMds = malloc(sizeof(MDS));       // ORIGINAL MDS
        lseek(fd, srcAddr, SEEK_SET);
        read(fd, myMds, sizeof(MDS));
        // printf("srcAddr STHN ARXH: %u\n", srcAddr);

        // MDS* copyMds = malloc(sizeof(MDS));
        // memcpy(copyMds, myMds, sizeof(MDS));

        char* srcName = malloc(mySuper->filenameSize+1);                // NAME
        read(fd, srcName, mySuper->filenameSize);
        // printf("srcName: %s OR %s, srcAddr: %u\n", srcName, sources[count], srcAddr);
        

                    // lseek(fd, srcAddr, SEEK_SET);
                    // read(fd, srcName, mySuper->filenameSize);
        Nav* newnewDestNav = malloc(sizeof(Nav));
        newnewDestNav->curAddress = destAddr;
        // printf("srcName: %s, srcAddr: %u, destAddr: %u, filensNum: %d, destNav-> %u\n", srcName, srcAddr, destAddr, filesNum, destNav->curAddress);

        if(dirOrNOt != -1){  /////////////////////////////////////////////////////// DIR CASE !!!!
            // printf("MPHKE STO DIR CASE\n");

            if(R == 'n' && r == 'n'){
                printf("cp: -r not specified; '%s' is Dir\n", sources[count]);
            }

            else{       /////  WRITE DIRS NAME AND COPY DIRS MDS WITHOUT ITS BLOCKS - AT TARGET ADDRESS !!

                cfs_mkdir(fd, srcName, newnewDestNav, holelist);

                // printf("newDirAddr: %u\n" ,newDirAddr);

                if(r == 'y' || depth < 1){
                    char** newSources = (char**)malloc((mySuper->maxDirFileNum+1)*sizeof(char*));

                    lseek(fd, srcAddr + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);

                    int elemsCount = 0;
                    while(elemsCount < myMds->elems){
                        lseek(fd, sizeof(unsigned int), SEEK_CUR);
                        newSources[elemsCount] = malloc(mySuper->filenameSize+1);
                        read(fd,newSources[elemsCount],mySuper->filenameSize);
                        elemsCount++;
                    }
                        // MALLOC ONE MORE FOR THE NEW DEST
                    newSources[elemsCount] = malloc(mySuper->filenameSize+1);

                    memcpy(newSources[elemsCount], srcName, mySuper->filenameSize);

                        // NEW NAV
                    Nav* newSrcNav = malloc(sizeof(Nav));
                    Nav* newDestNav = malloc(sizeof(Nav));
                    // memcpy(newSrcNav, srcNav, sizeof(Nav));
                    newSrcNav->curAddress = srcAddr;
                    newDestNav->curAddress = destAddr;



                    cfs_cp(fd,newSrcNav,newDestNav,holelist,newSources,elemsCount,depth+1,r,q,R);
                    
                    while(elemsCount >= 0){
                        free(newSources[elemsCount--]);
                    }
                    free(newSources);
                    free(newSrcNav);
                    free(newDestNav);
                }
            }
            // return; /// ??????????????
        }


        else{   /////////////////////////////////////////////////////////////// FILE CASE

            // printf("FILE CASE\n");
        
        // int count = 0;
        // while(count < filesNum){
            // printf("Files Count: %d\n", count);
            int flags[2];
            flags[0] = 1;
            flags[1] = 1;
            unsigned int newFileAddr = cfs_touch(fd, srcName, newnewDestNav, holelist, flags);
            // printf("newFileAddr: %u\n", newFileAddr);
            MDS* copyMds = malloc(sizeof(MDS));

            lseek(fd, newFileAddr, SEEK_SET);
            read(fd, copyMds, sizeof(MDS));

            copyMds->size = myMds->size;
            copyMds->elems = myMds->elems;
            
            lseek(fd, newFileAddr, SEEK_SET);
            write(fd, copyMds, sizeof(MDS));

            update_parent_sizes(fd, copyMds->parent_address, myMds->size);


            unsigned int *blockAddr = malloc(myMds->elems*sizeof(unsigned int));
            char **originalBlocks = (char**)malloc((myMds->elems)*sizeof(char*));    // DATA BLOCKS !!!!
            
            int temp = 0;
            lseek(fd, srcAddr + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);
            while(temp < myMds->elems){                       // ARRAY OF BLOCKS ADDRESSES
                read(fd, &blockAddr[temp], sizeof(unsigned int));
                temp++;
            }

            temp = 0;
            while(temp < myMds->elems){                       // ARRAY OF BLOCKS DATA
                originalBlocks[temp] = malloc(mySuper->blockSize);
                lseek(fd, blockAddr[temp], SEEK_SET);
                read(fd, originalBlocks[temp], mySuper->blockSize);
                // printf("diavase: %s\n", originalBlocks[temp]);

                temp++;
            }
            // printf("temp: %d\n",temp);

            // printf("READ ALL DONE !!\n");

            temp = 0;                                   // allocate new blocks, fill them -> write their addr at files data
            while(temp < myMds->elems){
                lseek(fd, 0 ,SEEK_SET);
                read(fd, mySuper, sizeof(SuperBlock));
                unsigned int tempblockAddr = find_hole(fd, mySuper, holelist);

                lseek(fd, tempblockAddr, SEEK_SET);
                write(fd, originalBlocks[temp], mySuper->blockSize);
                // printf("molis egrapse:\n\t%s\n", originalBlocks[temp]);

                memcpy(&blockAddr[temp], &tempblockAddr, sizeof(unsigned int));

                temp++;
            }

            ///////////////
            // printf("CopyMds:\n");
            // printf("\tsize: %u, type: %u, elems: %u\n", copyMds->size,copyMds->type,copyMds->elems);

            // printf("MyMds:\n");
            // printf("\tsize: %u, type: %u, elems: %u\n", myMds->size,myMds->type,myMds->elems);
            /////////

            lseek(fd, newFileAddr + sizeof(MDS) + mySuper->filenameSize, SEEK_SET);
            temp = 0;
            while(temp < myMds->elems){
                write(fd, &blockAddr[temp], sizeof(unsigned int));
                temp++;
            }

            // printf("mpainei sta free\n");
                // FREE MEM
            temp = myMds->elems - 1;
            while(temp >= 0){
                free(originalBlocks[temp]);
                temp--;
            }
            free(blockAddr);
            free(originalBlocks);

            free(copyMds);

        }

        free(newnewDestNav);


        free(myMds);
        free(srcName);
        free(nextDir);
        /////
        count++;
    }
    // printf("VGHKEEE\n");

    // // UPADATE SUPER BLOCK (IN CASE OF USING TAIL) !!
    // lseek(fd, 0 ,SEEK_SET);
    // write(fd, mySuper, sizeof(SuperBlock));

    free(mySuper);

}

int cfs_cd(int fd,Nav* nav,char* path){
    MDS metadata;
    int retval;
    retval = abs_or_rel(fd,nav,path,1);
    if (retval == -1){ 
        printf("The path you gave me does not exist\n");
        return -1;
    }
    else{
        nav->curAddress = retval;
        lseek(fd,nav->curAddress,SEEK_SET);
        read(fd,&metadata,sizeof(MDS));
        // printf("%d\n",metadata.size);
        nav->curNodeid = metadata.nodeid;
        nav->parent_nodeid = metadata.parent_nodeid;
        // printf("Current Adress %u \nCurrent Node_ID %u\nParent Node_ID %d\n",nav->curAddress,nav->curNodeid,nav->parent_nodeid);
        return retval;
    }
}


int is_regular_file(const char *path){
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void update_parent_sizes(int fd,unsigned int curr_adress,int size){
    MDS inform;
    if (curr_adress == 0)
        return;
    lseek(fd,curr_adress,SEEK_SET);
    read(fd,&inform,sizeof(MDS));
    // printf("Original size: %u\n", inform.size);
    inform.size = inform.size + size;
    // printf("Updated siez: %u\n", inform.size);
    lseek(fd,curr_adress,SEEK_SET);
    write(fd,&inform,sizeof(MDS));
    update_parent_sizes(fd,inform.parent_address,size);
    return;
}

void cfs_import(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation){
    if (isDirectory(source)){
        // printf("Dir\n");
        import_directory(fd,source,destination,holelist,navigation);
    }
    else if (is_regular_file(source)){
        // printf("File\n");
        import_file(fd,source,destination,holelist,navigation);
    }
    else printf("Error,false input information");
}



void import_file(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation){
    ssize_t bytes_Writ;
    int fd_import , to_Write;
    char* buffer;
    int newimport;
    Nav* newnav;
    newnav = malloc(sizeof(Nav));
    memcpy(newnav,navigation,sizeof(Nav));
    if (cfs_cd(fd,newnav,destination) == -1){
        printf("Error importing:no such destination\n");
        free(newnav);
        return;
    }
    if ((fd_import = open(source,O_RDONLY,PERMS)) == -1){
        printf("Error importing:no such file to import\n");
        free(newnav);
        return;
    }
    char* filename = strrchr(source,'/');
    if (filename == NULL){
        filename = source;
    }
    else { 
       filename = filename + sizeof(char);
    }
    
    int flags[2];
    flags[0] = 1; flags[1] = 1;
    if  ((newimport = cfs_touch(fd,filename,newnav,holelist,flags)) == -1){
        printf("Error importing,file with that name already exists,you have to rm it first,file is updated\n");
        free(newnav);
        return;
    }

    lseek(fd, 0 ,SEEK_SET);                 //get superblock from cfs file
    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    read(fd, mySuper, sizeof(SuperBlock));

    MDS* MDSdestin = malloc(sizeof(MDS)); //get destination folder MDS
    lseek(fd,newnav->curAddress,SEEK_SET);
    read(fd,MDSdestin,sizeof(MDS));
    
    buffer = malloc((mySuper->blockSize) * sizeof(char));  
    lseek(fd,newimport,SEEK_SET);
    MDS* fileto = malloc(sizeof(MDS));
    read(fd,fileto,sizeof(MDS)); 
    while( (  bytes_Writ = read(fd_import, buffer, mySuper->blockSize) ) > 0){
        to_Write = find_hole(fd,mySuper,holelist);  
        lseek(fd,newimport + sizeof(MDS) + (mySuper->filenameSize) + ((fileto->elems) * sizeof(unsigned int)),SEEK_SET);
        write(fd,&to_Write,sizeof(unsigned int));
        lseek(fd,to_Write,SEEK_SET);
        write(fd,buffer,bytes_Writ);
        fileto->elems++;
        fileto->size += bytes_Writ;
        MDSdestin->size += bytes_Writ;
    }
    lseek(fd,newnav->curAddress,SEEK_SET);
    write(fd,MDSdestin,sizeof(MDS));
    lseek(fd,newimport,SEEK_SET);
    write(fd,fileto,sizeof(MDS));
    update_parent_sizes(fd,MDSdestin->parent_address,fileto->size);
    free(fileto);
    free(buffer);
    free(mySuper);
    free(MDSdestin);
    free(newnav);
}



void import_directory(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation){
    DIR* import_this = opendir(source);
    Nav* newnav = malloc(sizeof(Nav));
    struct dirent *dirfile;
    memcpy(newnav,navigation,sizeof(Nav));
    if (cfs_cd(fd,newnav,destination) == -1){
        printf("No such destination\n");
        free(newnav);
        return;
    }
    char* dirname = strrchr(source,'/');

    if (dirname == NULL){
        dirname = source;
    }
    else {
        dirname = dirname + sizeof(char);
    }
    if (cfs_mkdir(fd,dirname,newnav,holelist) == -1){
        printf("Mkdir error\n");
        free(newnav);
        return;
    }
    while ((dirfile = readdir(import_this)) != NULL) {
        if ((strcmp(dirfile->d_name,".") == 0) || (strcmp(dirfile->d_name,"..") == 0)){
            continue;
        }
        char* newsrc = malloc(256 * sizeof(char));
        strcpy(newsrc,source);
        strcat(newsrc,"/");
        strcat(newsrc,dirfile->d_name);
        // printf("new: %s\n",newsrc);
        if (is_regular_file(newsrc)){
            import_file(fd,newsrc,dirname,holelist,newnav);
        }
        else if (isDirectory(newsrc)){
            import_directory(fd,newsrc,dirname,holelist,newnav);
        }
        free(newsrc);
    }
    free(newnav);
    closedir(import_this);
}


void cfs_export(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation){
    SuperBlock* Sup = malloc(sizeof(SuperBlock));
    MDS* MDcheck = malloc(sizeof(MDS));
    
    char* weird = NULL;
    weird = strdup(source);
    int addr;
    if ((addr = abs_or_rel(fd,navigation,weird,0)) == -1){
        printf("Export Error:There is no such source file or folder %s to export at %s\n",source,destination);
        free(weird);
        return;
    }
    free(weird);
    lseek(fd,0,SEEK_SET);
    read(fd,Sup,sizeof(SuperBlock));
    lseek(fd,addr,SEEK_SET);
    read(fd,MDcheck,sizeof(MDS));
    char* filename = strrchr(source,'/');
    // printf("s:%s\n",source);
    // printf("f:%s\n",filename);
    if (filename == NULL){
        filename = source;
    }
    else { 
        filename = filename + sizeof(char);
    }
 
    unsigned int* adresses = malloc((MDcheck->elems) * sizeof(unsigned int));
    
    if (MDcheck->type == 1){
        char* newdest = malloc(256 * sizeof(char));
        strcpy(newdest,destination);
        if (destination[strlen(destination)-1] != '/'){
            strcat(newdest,"/");
        }
        strcat(newdest,filename);
        char* buffer = malloc((Sup->blockSize) * sizeof(char));
        int newfilefd = open(newdest,O_CREAT|O_WRONLY,PERMS);
        int bytes_to_transfer = MDcheck->size;
        lseek(fd,addr + sizeof(MDS) + Sup->filenameSize,SEEK_SET);
        read(fd,adresses,(MDcheck->elems) * sizeof(unsigned int));
        for (int i = 0 ; i < (MDcheck->elems) ; i++){
            lseek(fd,adresses[i],SEEK_SET);
            if (bytes_to_transfer >= (Sup->blockSize)){
                read(fd,buffer,Sup->blockSize);
                write(newfilefd,buffer,Sup->blockSize);
                bytes_to_transfer -= (Sup->blockSize);
            }
            else if (bytes_to_transfer >= 0){
                read(fd,buffer,bytes_to_transfer);
                write(newfilefd,buffer,bytes_to_transfer);
                bytes_to_transfer = 0;
            }
        }
        free(newdest);
        free(buffer);
        close(newfilefd);
    }
    else if (MDcheck->type == 0){
        char* newdest = malloc(256 * sizeof(char));
        strcpy(newdest,destination);  
        strcat(newdest,"/");
        strcat(newdest,filename);      
        mkdir(newdest,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        lseek(fd,addr + sizeof(MDS) + Sup->filenameSize,SEEK_SET);

        int elemsCount = 0;
        char** filefoldname = malloc((MDcheck->elems) * sizeof(char*));
        for (int i = 0 ; i < MDcheck->elems ; i++){
            filefoldname[i] = malloc((Sup->filenameSize) * sizeof(char));
        }
        while(elemsCount < MDcheck->elems){
            read(fd, &adresses[elemsCount], sizeof(unsigned int));
            read(fd,filefoldname[elemsCount],Sup->filenameSize);
            elemsCount++;
        }
        char* newsrc = malloc(256 * sizeof(char));
        for (int i = 0 ; i < MDcheck->elems ; i++){
            strcpy(newsrc,source);
            strcat(newsrc,"/");
            strcat(newsrc,filefoldname[i]);
            // printf("new src : %s newdest : %s \n",newsrc,newdest);
            cfs_export(fd,newsrc,newdest,holelist,navigation);
        }
        free(newsrc);
        free(newdest);
        int i = MDcheck->elems -1;
        while(i >= 0){
            free(filefoldname[i--]);
        }
        free(filefoldname);
    }
}

unsigned int find_hole(int fd,SuperBlock* SBlock,infonodePtr holelist){
    int write_address;
     if (list_returnsize(holelist)){
        return write_address = list_popfirst(holelist);
    }
    else {
        write_address = SBlock->block_tail;
        SBlock->block_tail = SBlock->block_tail + SBlock->blockSize;
    }
    lseek(fd,0,SEEK_SET);
    write(fd,SBlock,sizeof(SuperBlock));
    return write_address;
}


void cfs_close(int fd, infonodePtr holelist){
    lseek(fd, 0, SEEK_SET);

    SuperBlock* mySuper = malloc(sizeof(SuperBlock));
    read(fd, mySuper, sizeof(SuperBlock));

    int size = list_returnsize(holelist);
    unsigned int addr;

    lseek(fd, mySuper->blockSize, SEEK_SET);
    write(fd, &size, sizeof(int));
    printf("(existing holes): %d\n", size);
    while(size > 0){
        addr = list_popfirst(holelist);
        write(fd, &addr, sizeof(unsigned int));
        size--;
    }
    free(mySuper);
    // printf("vghke\n");
}


void cfs_merge(int fd,char* source,char* dest,infonodePtr holelist,Nav* navigator){
    unsigned int to_write;
    int source_addr;
    int dest_addr;
    MDS src;
    MDS dst;
    int i;
    unsigned int* sourceblocks;
    printf("sour :%s\n",source);
    printf("des :%s\n",dest);
    if ((source_addr = abs_or_rel(fd,navigator,source,0)) == -1){
        printf("There is no such source address\n");
        return;
    }
    if ( (dest_addr = abs_or_rel(fd,navigator,dest,0)) == -1){
        printf("There is no such dest adress");
        return;
    }

    lseek(fd,source_addr,SEEK_SET);
    read(fd,&src,sizeof(MDS));
    if (src.type != 1){
        printf("Source is something other than a file\n");
        return;
    }
    lseek(fd,dest_addr,SEEK_SET);
    read(fd,&dst,sizeof(MDS));
    printf("Dest has %d DataBlocks\n",dst.elems);
    printf("Source has %d Datablocks\n",src.elems);
    SuperBlock* MySuper = malloc(sizeof(SuperBlock));
    lseek(fd,0,SEEK_SET);
    read(fd,MySuper,sizeof(SuperBlock));
    sourceblocks = malloc((src.elems) * sizeof(unsigned int));
    lseek(fd,source_addr + sizeof(MDS) + MySuper->filenameSize,SEEK_SET);
    read(fd,sourceblocks,src.elems * sizeof(unsigned int));
    //we now need to check whats the total size we want to transfer
    int transfersize = src.size;
    //check whats the dest size file already
    char* buffer = malloc(MySuper->blockSize * sizeof(char));
    int destsize = dst.size;
    int bytes_to_write;
    int newaddr;
    int middlepart;
    for (i = 0 ; i < src.elems ; i++){
        lseek(fd,sourceblocks[i],SEEK_SET);
        if (transfersize >= (MySuper->blockSize)){
            bytes_to_write = MySuper->blockSize;
            transfersize -= (MySuper->blockSize);
        }
        else{
            bytes_to_write = transfersize;
            transfersize = 0;
        }
        read(fd,buffer,bytes_to_write);
        if (destsize % (MySuper->blockSize) == 0){
            newaddr = find_hole(fd,MySuper,holelist);
            printf("%d",newaddr);
            lseek(fd,dest_addr + sizeof(MDS) + MySuper->filenameSize + dst.elems * sizeof(unsigned int),SEEK_SET);
            dst.elems = dst.elems + 1;
            write(fd,&newaddr,sizeof(unsigned int));
            lseek(fd,newaddr,SEEK_SET);
            write(fd,buffer,bytes_to_write);
            dst.size += bytes_to_write;
        }
        else if ((middlepart = destsize % (MySuper->blockSize)) != 0){
            lseek(fd,dest_addr + sizeof(MDS) + MySuper->filenameSize + (dst.elems - 1 ) * sizeof(unsigned int),SEEK_SET);
            read(fd,&to_write,sizeof(unsigned int));
            lseek(fd,to_write + middlepart ,SEEK_SET);
            if (bytes_to_write < (MySuper->blockSize - middlepart)) {
                write(fd,buffer,bytes_to_write);
                dst.size += bytes_to_write;
                bytes_to_write = 0;
            }
            else {
                write(fd,buffer,MySuper->blockSize - middlepart);
                dst.size += (MySuper->blockSize - middlepart);
                bytes_to_write -= (MySuper->blockSize - middlepart);
            }
            if (bytes_to_write > 0){
                newaddr = find_hole(fd,MySuper,holelist);
                printf("%d",newaddr);
                lseek(fd,dest_addr + sizeof(MDS) + MySuper->filenameSize + dst.elems * sizeof(unsigned int),SEEK_SET);
                dst.elems = dst.elems + 1;
                write(fd,&newaddr,sizeof(unsigned int));
                lseek(fd,newaddr,SEEK_SET);
                write(fd,&buffer[(MySuper->blockSize)-bytes_to_write-1],bytes_to_write);
                write(1,&buffer[(MySuper->blockSize)-bytes_to_write-1],bytes_to_write);
                dst.size += bytes_to_write;
                bytes_to_write = 0;
            }
        }
    }
    lseek(fd,dest_addr,SEEK_SET);
    write(fd,&dst,sizeof(MDS));
}  


void cfs_cat(int fd, char** sources, char* dest, infonodePtr holelist, Nav* nav, int filesNum){

// void cfs_rm(int fd, Nav* nav, infonodePtr holelist, char** dests, int filesNum, int depth, char q, char r);
// int cfs_touch(int fd,char* filename,Nav* navigation,infonodePtr holelist,int* flags);


    int exists = (int)abs_or_rel(fd, nav, dest, 0);
    if(exists != -1){
        char** s2 = malloc(1*sizeof(char*));
        s2[0] = strdup(dest);
        cfs_rm(fd, nav, holelist, s2, 1, 0, 'n', 'r');
        free(s2[0]);
        free(s2);
    }
    int flags[2];
    flags[0] = 0;
    flags[1] = 0;
    cfs_touch(fd,dest,nav,holelist,flags);
    
    int count = 0;
    while(count <= filesNum){
        cfs_merge(fd, sources[count], dest, holelist, nav);
        count++;
    }

    // merge(fd, sources[count], dest, holelist, nav);

// void cfs_merge(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigator);

}