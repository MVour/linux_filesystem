#include <stdio.h>

void PrintMenu(void){
	printf("\n");
	printf("0.  Exit\n");
	printf("1.  cfs_workwith <FILE>\n");
	printf("2.  cfs_mkdir <NAME>\n");
	printf("3.  cfs_touch (-a) (-m)\n");
	printf("4.  cfs_pwd\n");
	printf("5.  cfs_cd <PATH>\n");
	printf("6.  cfs_ls <OPTIONS>\n");
	printf("7.  cfs_cp <OPTIONS> <SOURCE> <DEST>\n");
	printf("9.  cfs_ln <SOURCE(S)> <DEST>\n");
	printf("8.  cfs_cat <SOURCE FILES> -o <OUTPUT FILE>\n");
	printf("10. cfs_mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> .. <DIRECTORY>\n");
	printf("11. cfs_rm <OPTIONS> <DESTS>\n");
	printf("12. cfs_import <SOURCES> <DIR>\n");
	printf("13. cfs_export <SOURCES> <DIR>'\n");
	printf("14. cfs_create -bs -fns -cfs -mdfn <NAME>\n");
	printf("\n");
}