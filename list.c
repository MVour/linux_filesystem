#include <stdio.h>
#include <stdlib.h>    
#include "list.h"
#include <string.h>

    infonodePtr list_create(){
        infonodePtr new_list = malloc(sizeof(struct infonode));
        new_list->head = NULL;
        new_list->tail = NULL;
        new_list->size = 0;
        return new_list;
    }

    void list_pushlast(infonodePtr mylist,unsigned int adress_toinsert){
        list_node to_insert = malloc(sizeof(struct node_list));
        to_insert->free_adress = adress_toinsert;
        if (mylist->size == 0){
            mylist->head = to_insert;
            mylist->tail = to_insert;
            mylist->size++;
        }
        else {
            mylist->tail->next = to_insert;
            mylist->tail = to_insert;
            mylist->size++;
        }
        return;
    }

    unsigned int list_popfirst(infonodePtr mylist){
        if (mylist->size == 0)
            return -1;
        list_node newHead = mylist->head->next;
        int address = mylist->head->free_adress;
        free(mylist->head);
        mylist->head = newHead;
        mylist->size --;
        // printf("cvgainei apo list\n");
        return address;
    }

    void list_print(infonodePtr mylist){
        list_node printer;
        printer = mylist->head;
        printf("Im printing adress list\n");
        while (printer != NULL){
            printf("%d \n",printer->free_adress);
            printer = printer->next;
        }
    }

    int list_returnsize(infonodePtr mylist){
        if (mylist == NULL){
            return -1;
        }
        return mylist->size;
    }
