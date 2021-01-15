typedef struct node_list* list_node;
typedef struct infonode* infonodePtr;

struct node_list{
    unsigned int free_adress;
    list_node next; 
};

struct infonode{
    list_node head;
    list_node tail;
    int size;
};

infonodePtr list_create();
void list_pushlast(infonodePtr,unsigned int);      //pushes to last spot
unsigned int list_popfirst(infonodePtr);       //pops first list element
void list_print(infonodePtr);
int list_returnsize(infonodePtr);
