/*
 * Name of program:
 * Authors:Trevor Weber
 * Description:
 **********************************************************/

/* These are the included libraries.  You may need to add more. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

/* Put any symbolic constants (defines) here */
#define True 1  /* C has no booleans! */
#define False 0

#define MAX_CMD 80

uint32_t root_addr;
uint32_t cwd;
char cwdName[11];
int info(uint16_t BPB_BytesPerSec, uint16_t BPB_SecPerClus, uint16_t BPB_RsvdSecCnt, uint16_t BPB_NumFATS, 	uint16_t BPB_FATSz32);

int volume(FILE *fp);
int stats(FILE *fp, char *inputName);
int ls(FILE *FP, int BPB_RsvdSecCnt, int BPB_BytesPerSec);
int cd(FILE *fp, char *dirName);
int readFile(FILE *fp, char *fileName, int position, int numBytes, int BPB_RsvdSecCnt, int BPB_BytesPerSec);

//helper fumctions
char* parseText(char *unParsedText);
int getNextCluster(FILE *fp, int BPB_RsvdSecCnt, int BPB_BytesPerSec, int FATOffset);
void remove_spaces(char *name);
/* This is the main function of your project, and it will be run
 * first before all other functions.
 */
int main(int argc, char *argv[])
{
	//Define the info variables
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATS;
	uint32_t BPB_FATSz32;
	
	//Define variables used for root address calculation	
	uint32_t BPB_RootClus;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_FATSz16;
	uint32_t FATSz;
	uint32_t RootDirSectors;
	uint32_t FirstDataSector;
	uint32_t FirstSectorofCluster;

	char cmd_line[MAX_CMD];
	FILE *fp;
	
	/* Parse args and open our image file */
	fp = fopen(argv[1],"r");
	
	/* Parse boot sector and get information */
	//Get BPB_BytesPerSec
	fseek(fp, 11, SEEK_SET);
	fread(&BPB_BytesPerSec, 1, 2, fp);
	BPB_BytesPerSec = le16toh(BPB_BytesPerSec);

	//Get BPB_SecPerClus
	fseek(fp, 13, SEEK_SET);
	fread(&BPB_SecPerClus, 1, 1, fp);
	BPB_SecPerClus = le16toh(BPB_SecPerClus);

	//Get BPB_RsvdSecCnt
	fseek(fp, 14, SEEK_SET);
	fread(&BPB_RsvdSecCnt, 1, 2, fp);
	BPB_RsvdSecCnt = le16toh(BPB_RsvdSecCnt);

	//Get BPB_NumFATS
	fseek(fp, 16, SEEK_SET);
	fread(&BPB_NumFATS, 1, 1, fp);
	BPB_NumFATS = le16toh(BPB_NumFATS);
	
	//Get BPB_FATSz32
	fseek(fp, 36, SEEK_SET);
	fread(&BPB_FATSz32, 1, 4, fp);	
	BPB_FATSz32 = le32toh(BPB_FATSz32);

	/* Get root directory address */	
	//Get BPB_RootClus
	fseek(fp, 44, SEEK_SET);
	fread(&BPB_RootClus, 1, 4, fp);	
	BPB_RootClus = le32toh(BPB_RootClus);
	
	//Get BPB_RootEntCnt
	fseek(fp, 17, SEEK_SET);
	fread(&BPB_RootEntCnt, 1, 2, fp);	
	BPB_RootEntCnt = le16toh(BPB_RootEntCnt);
	
	//Get BPB_FATSz16, should be 0 for FAT32
	fseek(fp, 22, SEEK_SET);
	fread(&BPB_FATSz16, 1, 2, fp);	
	BPB_FATSz16 = le16toh(BPB_FATSz16);
	
	/*Calculate the count of sectors occupied by the root directory. This should equal 0 on 	FAT32 since BPB_RootEntCnt is always 0 on FAT32.*/
	RootDirSectors = (((BPB_RootEntCnt * 32) + (BPB_BytesPerSec - 1)) / BPB_BytesPerSec);

	//Check for FAT32 or FAT16...should be FAT32
	if(BPB_FATSz16 != 0){
		FATSz = BPB_FATSz16;
	}else{
		FATSz = BPB_FATSz32;
	}
	
	//Calculate the start of the data region.
	FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATS * FATSz) + RootDirSectors;

	//Calculate the sector number of the first sector of the cluster
	FirstSectorofCluster = ((BPB_RootClus - 2) * BPB_SecPerClus) + FirstDataSector;
	//Calculate root_addr
	root_addr = FirstSectorofCluster * BPB_BytesPerSec;
	cwd = root_addr;
	strcpy(cwdName,"/");

	printf("Root addr is 0x%x\n", root_addr);


	/* Main loop.  You probably want to create a helper function
       for each command besides quit. */
	while(True) {
		bzero(cmd_line, MAX_CMD);
		//printf("%s]",cwdName[1]);
		printf("%s]",cwdName);
		fgets(cmd_line,MAX_CMD,stdin);		
		/* Start comparing input */
		if(strncmp(cmd_line,"info",4)==0) {
			info(BPB_BytesPerSec, BPB_SecPerClus, BPB_RsvdSecCnt, BPB_NumFATS, BPB_FATSz32);

		}

		else if(strncmp(cmd_line,"volume",6)==0) {
			volume(fp);
		}
		
		else if(strncmp(cmd_line,"stat",4)==0) {
			int i = 0;
			char *token;
			char *args[3];
			memset(args,0,sizeof(args));
			token = strtok(cmd_line, " ");
			while(token != NULL){
				args[i] = token;
				i++;
				token = strtok(NULL," ");
			}
			char name[strlen(args[1])];
			strcpy(name,args[1]);
			for(int i=0;i<=strlen(args[1]);i++){
				if (name[i] == '\n'){
					name[i]='\0';
				}
			}
			args[1] = name;
			stats(fp,args[1]);
		}

		else if(strncmp(cmd_line,"cd",2)==0) {
			int i = 0;
			char *token;
			char *args[3];
			memset(args,0,sizeof(args));
			token = strtok(cmd_line, " ");
			while(token != NULL){
				args[i] = token;
				i++;
				token = strtok(NULL," ");
			}
				
			cd(fp,args[2]);
		}


		else if(strncmp(cmd_line,"ls",2)==0) {
			ls(fp,BPB_RsvdSecCnt, BPB_BytesPerSec);
		}

		else if(strncmp(cmd_line,"read",4)==0) {	
			int i = 0;
			char *token;
			char *args[5];
			memset(args,0,sizeof(args));
			token = strtok(cmd_line, " ");
			while(token != NULL){
				args[i] = token;
				i++;
				token = strtok(NULL," ");
			}
				
			readFile(fp,args[1],atoi(args[2]),atoi(args[3]),BPB_RsvdSecCnt, BPB_BytesPerSec);
		}
		
		else if(strncmp(cmd_line,"quit",4)==0) {
			printf("Quitting.\n");
			break;
		}
		else
			printf("Unrecognized command.\n");


	}


	/* Close the file */
	fclose(fp);
	return 0; /* Success */
}

int info(uint16_t BPB_BytesPerSec, uint16_t BPB_SecPerClus, uint16_t BPB_RsvdSecCnt, uint16_t BPB_NumFATS, 	uint16_t BPB_FATSz32)
{
		//Print out the values of each in hex and base 10
		printf("BPB_BytesPerSec is 0x%x, %i\n",BPB_BytesPerSec,BPB_BytesPerSec);
		printf("BPB_SecPerClus is 0x%x, %i\n",BPB_SecPerClus,BPB_SecPerClus);
		printf("BPB_RsvdSecCnt is 0x%x, %i\n",BPB_RsvdSecCnt,BPB_RsvdSecCnt);
		printf("BPB_NumFATS is 0x%x, %i\n",BPB_NumFATS,BPB_NumFATS);
		printf("BPB_FATSz32 is 0x%x, %i\n",BPB_FATSz32,BPB_FATSz32);
		return 1;

}

int volume(FILE *fp){
	
	char name[10];
	
	fseek(fp, root_addr, SEEK_SET);
	fread(&name, 1, 11, fp);
	if(strncmp(name, "NO NAME",7) == 0){
		printf("Error: volume name not found.");
	}else{
		
		char *token; 
		token = strtok(name," ");
		printf("Volume Name: %s\n",token);
	}
	return 1;
}

int stats(FILE *fp, char *inputName){
	char DIR_Name[10];
	int i = 0;
	int flag = 1;
	uint8_t DIR_Attr;
	uint32_t DIR_FileSize;
	//used for next cluster
	uint16_t DIR_FstClusLO = 0;
	uint16_t DIR_FstClusHI = 0;
        uint32_t result_num = 0;


	while(flag){
		//This must seek from the cwd directory
		fseek(fp,cwd + (64 * i),SEEK_SET);
		fread(&DIR_Name, 1, 11, fp);	
		if(DIR_Name[0] == 0xE5){
			flag = 1;
		}else if(DIR_Name[0] == 0x00){
			printf("Error: file/directory does not exist\n");
			flag = 0;
		}else{
				//This must seek from the cwd directory
				fseek(fp,((cwd + (64 * i)) + 11),SEEK_SET);
				fread(&DIR_Attr, 1, 1, fp);
		  		//Get File size
				fseek(fp,((cwd + (64 * i)) + 28),SEEK_SET);	
				fread(&DIR_FileSize, 1, 4, fp);
				DIR_FileSize = le32toh(DIR_FileSize);

				//get high clust
				fseek(fp,((cwd+ (64 * i)) + 20),SEEK_SET);	
				fread(&DIR_FstClusHI, 1, 2, fp);
				DIR_FstClusHI = le16toh(DIR_FstClusHI);
				//get low clust
				fseek(fp,((cwd + (64 * i)) + 26),SEEK_SET);	
				fread(&DIR_FstClusLO, 1, 2, fp);
				DIR_FstClusLO = le16toh(DIR_FstClusLO);
				//Calc full clust
			        result_num = ((DIR_FstClusHI << 16) + DIR_FstClusLO);
        			result_num = le32toh(result_num);
			
				if(DIR_Attr == 0x20){	
					strcpy(DIR_Name, parseText(DIR_Name));
					if(strncmp(DIR_Name,inputName,10)==0){
						printf("Size is %i\n",DIR_FileSize);
						printf("Attributes: ATTR_ARCHIVE\n");
						printf("Next cluster number is 0x%x\n",result_num);
						break;
					}
				//if its a directory
				}else if(DIR_Attr == 0x10){
					char *token; 
					token = strtok(DIR_Name," ");
					if(strncmp(token,inputName,10)==0){
						printf("Size is %i\n",DIR_FileSize);
						printf("Attributes: ATTR_DIRECTORY\n");
						printf("Next cluster number is 0x%x\n",result_num);
						break;
					}
				}
			
		}
		i++;
	}
	return 1;
}

int ls(FILE *fp,int BPB_RsvdSecCnt, int BPB_BytesPerSec){

	char DIR_Name[10];
	int i = 0;
	int flag = 1;
	uint8_t DIR_Attr;

	while(flag){
		fseek(fp,cwd + (64 * i),SEEK_SET);
		fread(&DIR_Name, 1, 11, fp);	
		if(DIR_Name[0] == 0xE5){
			flag = 1;
		}else if(DIR_Name[0] == 0x00){
			//used for next cluster
			uint16_t DIR_FstClusLO = 0;
			uint16_t DIR_FstClusHI = 0;
			uint32_t result_num = 0;
			//get high clust
			fseek(fp,((root_addr + (64 * i)) + 20),SEEK_SET);	
			fread(&DIR_FstClusHI, 1, 2, fp);
			DIR_FstClusHI = le16toh(DIR_FstClusHI);
			//get low clust
			fseek(fp,((root_addr + (64 * i)) + 26),SEEK_SET);	
			fread(&DIR_FstClusLO, 1, 2, fp);
			DIR_FstClusLO = le16toh(DIR_FstClusLO);
			//Calc full clust
			result_num = ((DIR_FstClusHI << 16) + DIR_FstClusLO);
			result_num = le32toh(result_num);
			uint32_t FATOffset;
			uint32_t nextClustNum;
			FATOffset = result_num * 4; 
			nextClustNum = getNextCluster(fp,BPB_RsvdSecCnt, BPB_BytesPerSec, FATOffset);
			if(nextClustNum >= 0x0FFFFFF8){
				flag = 0;
			}else{
				fseek(fp,(nextClustNum* 512),SEEK_SET);	
				cwd = nextClustNum*512;
				ls(fp,BPB_RsvdSecCnt,BPB_BytesPerSec);
			}
			
			flag = 0;
		}else{
			fseek(fp,((cwd + (64 * i)) + 11),SEEK_SET);
			fread(&DIR_Attr, 1, 1, fp);
			//if its a file print it
			if(DIR_Attr == 0x20){
				strcpy(DIR_Name, parseText(DIR_Name));	
				printf("%s   ",DIR_Name);
			//if its a dir print it
			}else if(DIR_Attr == 0x10){
				char *token; 
				token = strtok(DIR_Name," ");
				printf("%s   ",token);
			}
		}
		
		
		i++;
	}
	printf("\n");
	return 1;
}

int getNextCluster(FILE *fp, int BPB_RsvdSecCnt, int BPB_BytesPerSec, int FATOffset){
	uint32_t nextClustNum;
	uint32_t ThisFATSecNum;
	uint32_t ThisFATEntOffset;
	ThisFATSecNum = BPB_RsvdSecCnt + (FATOffset / BPB_BytesPerSec);
	ThisFATEntOffset = FATOffset % BPB_BytesPerSec;	
	fseek(fp,(ThisFATSecNum*512),SEEK_SET);
	fseek(fp,ThisFATEntOffset,SEEK_CUR);
	fread(&nextClustNum, 1, 4, fp);
	nextClustNum = le32toh(nextClustNum);
	return nextClustNum;
	
}

int cd(FILE *fp, char *dirName){

	char DIR_Name[10];
	int i = 0;
	int flag = 1;
	uint8_t DIR_Attr;
	uint32_t DIR_FileSize;
	//used for next cluster
	uint16_t DIR_FstClusLO = 0;
	uint16_t DIR_FstClusHI = 0;
        uint32_t result_num = 0;

	char inputName[10] = "DIR";
	while(flag){
		fseek(fp,cwd + (64 * i),SEEK_SET);
		fread(&DIR_Name, 1, 11, fp);	
		
		if(DIR_Name[0] == 0xE5){
			flag = 1;
		}else if(DIR_Name[0] == 0x00){
			printf("Error: file/directory does not exist\n");
			flag = 0;
		}else{
				//This must seek from the cwd directory
				fseek(fp,((cwd + (64 * i)) + 11),SEEK_SET);
				fread(&DIR_Attr, 1, 1, fp);
		  		//Get File size
				fseek(fp,((cwd + (64 * i)) + 28),SEEK_SET);	
				fread(&DIR_FileSize, 1, 4, fp);
				DIR_FileSize = le32toh(DIR_FileSize);

				//get high clust
				fseek(fp,((cwd + (64 * i)) + 20),SEEK_SET);	
				fread(&DIR_FstClusHI, 1, 2, fp);
				DIR_FstClusHI = le16toh(DIR_FstClusHI);
				//get low clust
				fseek(fp,((cwd + (64 * i)) + 26),SEEK_SET);	
				fread(&DIR_FstClusLO, 1, 2, fp);
				DIR_FstClusLO = le16toh(DIR_FstClusLO);
				//Calc full clust
			    result_num = ((DIR_FstClusHI << 16) + DIR_FstClusLO);
        		result_num = le32toh(result_num);
			
					
				//if its a directory find where its located and set the 
				//cwd to it
				if(DIR_Attr == 0x10){
					char *token; 
					token = strtok(DIR_Name," ");
					if(strncmp(token,inputName,10)==0){
						uint32_t FirstSectorofCluster;
						FirstSectorofCluster = ((result_num - 2) * 1) +2050;
						cwd = FirstSectorofCluster*512;
						strcpy(cwdName,token);
						break;
					}
				}
			
		}
		i++;
	}
	return 1;
}
int readFile(FILE *fp, char *fileName, int position, int numBytes, int BPB_RsvdSecCnt, int BPB_BytesPerSec){
	FILE *file;
	char fileData[511];
	char DIR_Name[10];
	int i = 0;
	int flag = 1;
	uint8_t DIR_Attr;
	uint32_t DIR_FileSize;
	//used for next cluster
	uint16_t DIR_FstClusLO = 0;
	uint16_t DIR_FstClusHI = 0;
        uint32_t result_num = 0;
	int numBytesLeft;
	uint32_t FirstSectorofCluster;


	while(flag){
		//This must seek from the cwd directory
		fseek(fp,cwd + (64 * i),SEEK_SET);
		fread(&DIR_Name, 1, 11, fp);	
		
		if(DIR_Name[0] == 0xE5){
			flag = 1;
		}else if(DIR_Name[0] == 0x00){
			printf("Error: file does not exist\n");
			flag = 0;
		}else{
				//This must seek from the cwd directory
				fseek(fp,((cwd + (64 * i)) + 11),SEEK_SET);
				fread(&DIR_Attr, 1, 1, fp);
		  		//Get File size
				fseek(fp,((cwd + (64 * i)) + 28),SEEK_SET);	
				fread(&DIR_FileSize, 1, 4, fp);
				DIR_FileSize = le32toh(DIR_FileSize);

				//get high clust
				fseek(fp,((cwd + (64 * i)) + 20),SEEK_SET);	
				fread(&DIR_FstClusHI, 1, 2, fp);
				DIR_FstClusHI = le16toh(DIR_FstClusHI);
				//get low clust
				fseek(fp,((cwd + (64 * i)) + 26),SEEK_SET);	
				fread(&DIR_FstClusLO, 1, 2, fp);
				DIR_FstClusLO = le16toh(DIR_FstClusLO);
				//Calc full clust
			        result_num = ((DIR_FstClusHI << 16) + DIR_FstClusLO);
        			result_num = le32toh(result_num);
			
				if(DIR_Attr == 0x20){	
					strcpy(DIR_Name, parseText(DIR_Name));
					if(strncmp(DIR_Name,fileName,10)==0){
							FirstSectorofCluster = ((result_num - 2) * 1) +2050;
						//if we are reading more than one cluster we need to 
						//find the next cluster
						if(numBytes > 512){
							fseek(fp,(FirstSectorofCluster* 512) + position,SEEK_SET);
							fread(&fileData, 1, 512, fp);
							numBytesLeft = numBytes - 512;
						
							uint32_t FATOffset;
							uint32_t nextClustNum;
							FATOffset = result_num * 4; 

							nextClustNum = getNextCluster(fp,BPB_RsvdSecCnt, BPB_BytesPerSec, FATOffset);
							printf("nextClustNum: 0x%x\n",nextClustNum*512);
							if(nextClustNum >= 0x0FFFFFF8){
								flag = 0;
								printf("eoc: %s\n",fileData);
							}else{
								fseek(fp,(nextClustNum* 512),SEEK_SET);	
								memset(fileData,0,sizeof(fileData));
								fread(&fileData, 1, numBytesLeft, fp);
								printf("second: %s\n",fileData);
							}
						//otherwise we can just read from the file	
						}else{		
							fseek(fp,(FirstSectorofCluster* 512) + position,SEEK_SET);
							fread(&fileData, 1, numBytes, fp);
							printf("%s\n",fileData);
							break;
						}
					}
				}
		}
		i++;
	}
	return 1;
}
//parses file names so thye have a period in them
char* parseText(char *unParsedName){
	char *token;
	static char firstName[10];
	token = strtok(unParsedName, " ");
	strcpy(firstName, token);
	strncat(firstName,".",1);
	while(token != NULL){
		if(strncmp(firstName,token,strlen(token)) == 0){
			token = strtok(NULL," ");
		
		}else{
			strncat(firstName,token,3);
			token = strtok(NULL," ");
		}
	}
	return firstName;
}
