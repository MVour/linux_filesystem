#include <time.h>
#include "functions.h"
#include "list.h"

typedef struct {
    unsigned int *datablocks;
} Datastream ;

typedef struct {
    unsigned int nodeid ;
    unsigned int size ;
    unsigned int type ;
    unsigned int parent_nodeid ;
    unsigned int parent_address;
    unsigned int elems;
    unsigned int linkCounter;
    time_t creation_time ;
    time_t access_time ;
    time_t modification_time ;
    // Datastream data ;
} MDS ;

typedef struct{
	int blockSize;
	int filenameSize;
	int maxfileSize;
	int maxDirFileNum;
    int Empty_list_size;
    int idCount;
    unsigned int root_adress;
    unsigned int block_tail;
} SuperBlock;



int cfs_workwith(char* cfsfile);
int cfs_create(char* filepath,int block_size,int filename_size,int maxfile_size,int maxdirectoryfile_number);
int cfs_mkdir(int, char*, Nav*,infonodePtr);
void cfs_ls(int fd, Nav* nav, char hid, char rev, char sort, char list, char print,char sc, char** scout, int depth);
void cfs_pwd(int fd, Nav* nav);
int cfs_touch(int fd,char* filename,Nav* navigation,infonodePtr holelist,int* flags);
void cfs_mv(int fd, Nav* nav, char** files, int filesNum, char q);
void cfs_ln(int fd, Nav *nav, char* source, char* dest);
void cfs_rm(int fd, Nav* nav, infonodePtr holelist, char** dests, int filesNum, int depth, char q, char r);
void cfs_cp(int fd, Nav* srcNav, Nav* destNav, infonodePtr holelist, char** sources, int filensNum, int depth, char r, char q, char R);
int cfs_cd(int fd,Nav* nav,char* path);
void cfs_import(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation);
void cfs_export(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation);
void cfs_close(int fd, infonodePtr holelist);
void cfs_merge(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigator);
void cfs_cat(int fd, char** sources, char* dest, infonodePtr holelist, Nav* nav, int filesNum);

unsigned int abs_or_rel(int fd,Nav* nav,char* path,int findfile);
unsigned int find_path(int fd,unsigned int from_Adress,char* path,char* mem,int files_or_not, int indicator);

void import_file(int fd , char* source,char* destination,infonodePtr holelist,Nav* navigation);
void import_directory(int fd,char* source,char* destination,infonodePtr holelist,Nav* navigation);

int is_regular_file(const char *path);
int isDirectory(const char *path);

void update_parent_sizes(int fd,unsigned int curr_adress,int size);

unsigned int find_hole(int fd,SuperBlock* SBlock,infonodePtr holelist);

