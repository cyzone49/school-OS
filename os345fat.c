// os345fat.c - file management system	2017-06-28
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
//
//		11/19/2011	moved getNextDirEntry to P6
//
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "os345.h"
#include "os345fat.h"

// ***********************************************************************
// ***********************************************************************
//	functions to implement in Project 6
//
int fmsCloseFile(int);
int fmsDefineFile(char*, int);
int fmsDeleteFile(char*);
int fmsOpenFile(char*, int);
int fmsReadFile(int, char*, int);
int fmsSeekFile(int, int);
int fmsWriteFile(int, char*, int);

// ***********************************************************************
// ***********************************************************************
//	Support functions available in os345p6.c
//
extern int fmsGetDirEntry(char* fileName, DirEntry* dirEntry);
extern int fmsGetNextDirEntry(int *dirNum, char* mask, DirEntry* dirEntry, int dir);

extern int fmsMount(char* fileName, void* ramDisk);

extern void setFatEntry(int FATindex, unsigned short FAT12ClusEntryVal, unsigned char* FAT);
extern unsigned short getFatEntry(int FATindex, unsigned char* FATtable);

extern int fmsMask(char* mask, char* name, char* ext);
extern void setDirTimeDate(DirEntry* dir);
extern int isValidFileName(char* fileName);
extern void printDirectoryEntry(DirEntry*);
extern void fmsError(int);

extern int fmsReadSector(void* buffer, int sectorNumber);
extern int fmsWriteSector(void* buffer, int sectorNumber);

// ***********************************************************************
// ***********************************************************************
// fms variables
//
// RAM disk
unsigned char RAMDisk[SECTORS_PER_DISK * BYTES_PER_SECTOR];

// File Allocation Tables (FAT1 & FAT2)
unsigned char FAT1[NUM_FAT_SECTORS * BYTES_PER_SECTOR];
unsigned char FAT2[NUM_FAT_SECTORS * BYTES_PER_SECTOR];

char dirPath[128];							// current directory path
FCB OFTable[NFILES];						// open file table

extern bool diskMounted;					// disk has been mounted
extern TCB tcb[];							// task control block
extern int curTask;							// current task #


// ***********************************************************************
// ***********************************************************************
// peripherals functions
//
int fmsGetFCB(DirEntry* dirEntry);
int getFreeRootCluster();
int getFreeDataCluster();
int getFreeCluster(int begin, int end);
int fmsUpdateDirEntry(FCB* fcbEntry);
int getFreeDirEntry(int* dirNum, DirEntry* dirEntry, int* dirSector);
int flushBuffer(FCB* fcbEntry);
int getDirEntry(int* dirNum, char* mask, DirEntry* dirEntry, int* dirSector);

// ***********************************************************************
// ***********************************************************************
// This function closes the open file specified by fileDescriptor.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
//	Return 0 for success, otherwise, return the error number.
//
int fmsCloseFile(int fileDescriptor)
{
	// ?? add code here
	// printf("\nfmsCloseFile Not Implemented");

	int error;
	FCB* fcbEntry;
	fcbEntry = &OFTable[fileDescriptor];

	if (fcbEntry->name[0] == 0) return ERR63;						// file not open error

	// file was potentially altered and dirInfo needs to be updated
	if (fcbEntry->mode != OPEN_READ)								// error
	{
		if ((error = flushBuffer(fcbEntry))) return error;			// buffer error
		if ((error = fmsUpdateDirEntry(fcbEntry))) return error;	// update error		
	}

	fcbEntry->name[0] = 0;
	
	return 0;

	//return ERR63;
} // end fmsCloseFile



// ***********************************************************************
// ***********************************************************************
// If attribute=DIRECTORY, this function creates a new directory
// file directoryName in the current directory.
// The directory entries "." and ".." are also defined.
// It is an error to try and create a directory that already exists.
//
// else, this function creates a new file fileName in the current directory.
// It is an error to try and create a file that already exists.
// The start cluster field should be initialized to cluster 0.  In FAT-12,
// files of size 0 should point to cluster 0 (otherwise chkdsk should report an error).
// Remember to change the start cluster field from 0 to a free cluster when writing to the
// file.
//
// Return 0 for success, otherwise, return the error number.
//
int fmsDefineFile(char* fileName, int attribute)
{
	// ?? add code here
	// printf("\nfmsDefineFile Not Implemented");
	// return ERR72;

	DirEntry dirEntry, curDir, parentDir;

	char* s;
	char* dot = ".";
	char* token;
	char buffer[BYTES_PER_SECTOR];

	int cluster, sector;
	int dirNumber = 0, dirIndex, error, dirSector, i;

	if (!fmsGetDirEntry(fileName, &dirEntry)) return ERR60;					// file already defined error	
	if ((error = getFreeDirEntry(&dirNumber, &dirEntry, &dirSector))) return error;

	dirIndex = dirNumber % ENTRIES_PER_SECTOR;								// same as process in fmsGetNextDir
	
	switch (attribute)														// there is a free dir entry
	{
		case 0:

		case ARCHIVE:
			attribute = ARCHIVE;

			if (!isValidFileName(fileName)) return ERR50;					// invalid file name error
		
			for (i = 0; i < strlen(fileName); ++i)							// fileName -> all caps
			{
				fileName[i] = (char)toupper(fileName[i]);					
			}

			token = strtok(fileName, dot);
			sprintf(dirEntry.name, "%-8s", token);
			sprintf(dirEntry.extension, "%-3s", strtok(NULL, dot));

			dirEntry.startCluster = 0;
			break;

		case DIRECTORY:
			s = strchr(fileName, '.');

			if (s != NULL || strlen(fileName) > 8) return ERR50;			// invalid file name, no extensions allowed
		
			for (i = 0; i < strlen(fileName); i++)							// fileName -> all caps
			{
				fileName[i] = (char)toupper(fileName[i]);
			}
			
			if ((error = getFreeDataCluster()) < 0) return error;

			cluster = error;

			sprintf(dirEntry.name, "%-8s", fileName);
			sprintf(dirEntry.extension, "   ");
			dirEntry.startCluster = (uint16)cluster;

			setFatEntry(cluster, FAT_EOC, FAT1);
			setFatEntry(cluster, FAT_EOC, FAT2);
			
			sector = C_2_S(cluster);
			if ((error = fmsReadSector(buffer, sector))) return error;

			// Set "." entry
			memcpy(&curDir, &buffer[0], sizeof(DirEntry));
			sprintf(curDir.name, "%-8s", ".");
			sprintf(curDir.extension, "   ");

			curDir.attributes = DIRECTORY;
			curDir.startCluster = (uint16)cluster;
			memcpy(&buffer[0], &curDir, sizeof(DirEntry));

			// Set ".." entry
			memcpy(&parentDir, &buffer[sizeof(DirEntry)], sizeof(DirEntry));
			sprintf(parentDir.name, "%-8s", "..");
			strcpy(parentDir.extension, "   ");

			parentDir.attributes = DIRECTORY;
			parentDir.startCluster = (uint16)CDIR;
			memcpy(&buffer[sizeof(DirEntry)], &parentDir, sizeof(DirEntry));
			
			fmsWriteSector(&buffer, sector);						// Write in new sector entries
			break;

		default:
			return ERR52;											// invalid file descriptor
	}

	dirEntry.fileSize = 0;
	dirEntry.attributes = (uint8)attribute;

	setDirTimeDate(&dirEntry);

	if ((error = fmsReadSector(buffer, dirSector))) return error;	// error reading sectorNumber into buffer RAM disk

	memcpy(&buffer[dirIndex * sizeof(DirEntry)], &dirEntry, sizeof(DirEntry));

	if ((error = fmsWriteSector(buffer, dirSector))) return error;	// error write sector

	return 0;

} // end fmsDefineFile



// ***********************************************************************
// ***********************************************************************
// This function deletes the file fileName from the current director.
// The file name should be marked with an "E5" as the first character and the chained
// clusters in FAT 1 reallocated (cleared to 0).
// Return 0 for success; otherwise, return the error number.
//
int fmsDeleteFile(char* fileName)
{
	// ?? add code here
	// printf("\nfmsDeleteFile Not Implemented");
	// return ERR61;

	DirEntry dirEntry, fileEntry;
	
	char buffer[BYTES_PER_SECTOR];
	char anyFile[6] = "?*.?*";

	int dirNumber = 0, subDir = 0, dirIndex, error, dirSector, endCluster, nextCluster;
	
	if ((error = getDirEntry(&dirNumber, fileName, &dirEntry, &dirSector))) return error;

	if (dirEntry.attributes & DIRECTORY) 
	{// make sure that all the files are gone		
		if (!(error = fmsGetNextDirEntry(&subDir, anyFile, &fileEntry, dirEntry.startCluster)))
			return ERR69;												// delete error at least 1 file exists		
	}

	dirNumber--;														// fmsGetNextDirEntry increments preemptively
	dirIndex = dirNumber % ENTRIES_PER_SECTOR;							// same as in fmsGetNextDir process	
	dirEntry.name[0] = 0xe5;											// Update potential changes of dirEntry	
	if ((error = fmsReadSector(buffer, dirSector))) return error;		// read sector. return error if failed	
	
	memcpy(&buffer[dirIndex * sizeof(DirEntry)], &dirEntry, sizeof(DirEntry)); //update dirEntiry in sector
	
	if ((error = fmsWriteSector(&buffer, dirSector)) < 0) return error;	// write back entire sector

	if ( dirEntry.startCluster ) 
	{
		endCluster = dirEntry.startCluster;

		// completed writing
		while ((nextCluster = getFatEntry(endCluster, FAT1)) != FAT_EOC)// while chain not ended
		{																// set the further links free
			setFatEntry(endCluster, 0x00, FAT1);
			setFatEntry(endCluster, 0x00, FAT2);
			endCluster = nextCluster;
		}

		setFatEntry(endCluster, 0x00, FAT1);
		setFatEntry(endCluster, 0x00, FAT2);

	}
	return 0;
} // end fmsDeleteFile



// ***********************************************************************
// ***********************************************************************
// This function opens the file fileName for access as specified by rwMode.
// It is an error to try to open a file that does not exist.
// The open mode rwMode is defined as follows:
//    0 - Read access only.
//       The file pointer is initialized to the beginning of the file.
//       Writing to this file is not allowed.
//    1 - Write access only.
//       The file pointer is initialized to the beginning of the file.
//       Reading from this file is not allowed.
//    2 - Append access.
//       The file pointer is moved to the end of the file.
//       Reading from this file is not allowed.
//    3 - Read/Write access.
//       The file pointer is initialized to the beginning of the file.
//       Both read and writing to the file is allowed.
// A maximum of 32 files may be open at any one time.
// If successful, return a file descriptor that is used in calling subsequent file
// handling functions; otherwise, return the error number.
//
int fmsOpenFile(char* fileName, int rwMode)
{
	// ?? add code here
	// printf("\nfmsOpenFile Not Implemented");
	// return ERR61;
	int fdIndex;
	DirEntry dirEntry;

	if (fmsGetDirEntry(fileName, &dirEntry)) {
		return ERR61; // file was not found
	}

	if (dirEntry.attributes & DIRECTORY) {
		return ERR50; //invalid file name
	}

	switch (rwMode) {
	case OPEN_READ:
		break;
	case OPEN_WRITE:
	case OPEN_APPEND:
	case OPEN_RDWR:
		if (!(dirEntry.attributes & READ_ONLY)) { // checks that file is not read only
			break;
		}
	default:
		return ERR85; //Illegal access
	}

	if ((fdIndex = fmsGetFCB(&dirEntry)) < 0) {
		//error finding fdEntry return that error
		return fdIndex;
	}

	//prepare the File Descriptor entry
	FCB* fdEntry = &OFTable[fdIndex];
	strncpy(fdEntry->name, dirEntry.name, 8);
	strncpy(fdEntry->extension, dirEntry.extension, 3);
	fdEntry->attributes = dirEntry.attributes;
	fdEntry->directoryCluster = (uint16)CDIR;
	fdEntry->startCluster = dirEntry.startCluster;
	fdEntry->currentCluster = 0;
	fdEntry->fileSize = (rwMode == OPEN_WRITE ? 0 : dirEntry.fileSize);
	fdEntry->pid = curTask;
	fdEntry->mode = (char)rwMode;
	fdEntry->flags = 0x00;
	fdEntry->fileIndex = (rwMode != OPEN_APPEND ? 0 : dirEntry.fileSize);
	memset(fdEntry->buffer, 0, BYTES_PER_SECTOR * sizeof(char));
	// If file is being appended to
	if (rwMode == OPEN_APPEND && fdEntry->startCluster > 0) {
		unsigned short nextCluster;
		int error;
		fdEntry->currentCluster = fdEntry->startCluster;
		// fast-forward currentCluster to the end of the file
		while ((nextCluster = getFatEntry(fdEntry->currentCluster, FAT1)) != FAT_EOC)
			fdEntry->currentCluster = nextCluster;
		// load the end of the file into the buffer
		if ((error = fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster)))) return error;
	}

	return fdIndex;
} // end fmsOpenFile



// ***********************************************************************
// ***********************************************************************
// This function reads nBytes bytes from the open file specified by fileDescriptor into
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// After each read, the file pointer is advanced.
// Return the number of bytes successfully read (if > 0) or return an error number.
// (If you are already at the end of the file, return EOF error.  ie. you should never
// return a 0.)
//
int fmsReadFile(int fileDescriptor, char* buffer, int nBytes)
{
	// ?? add code here
	// printf("\nfmsReadFile Not Implemented");
	// return ERR63;

	int error; uint16 nextCluster;
	FCB* fdEntry;
	int numBytesRead = 0;
	unsigned int bytestLeft, bufferIndex;
	fdEntry = &OFTable[fileDescriptor];
	if (fdEntry->name[0] == 0) {
		return ERR63;
	} // file not open
	if ((fdEntry->mode == OPEN_WRITE) || (fdEntry->mode == OPEN_APPEND)) {
		//        printf("\ninvalid access");
		return ERR85;
	}

	while (nBytes > 0) {
		if (fdEntry->fileSize == fdEntry->fileIndex) {
			//            printf("\nfileSize == fileIndex");
			return (numBytesRead ? numBytesRead : ERR66); //number of bytes read or EOF error
		}
		bufferIndex = fdEntry->fileIndex % BYTES_PER_SECTOR;
		if ((bufferIndex == 0) && (fdEntry->fileIndex || !fdEntry->currentCluster)) {
			if (fdEntry->currentCluster == 0) { //file has been lazy opened and buffer is not initialized
				if (fdEntry->startCluster == 0) {
					//                    printf("\nunitialized file can't read");
					return ERR66;
				}
				nextCluster = fdEntry->startCluster;
				fdEntry->fileIndex = 0;
			}
			else {
				nextCluster = getFatEntry(fdEntry->currentCluster, FAT1);
				if (nextCluster == FAT_EOC) {
					return numBytesRead;
				}
			}
			if ((error = flushBuffer(fdEntry))) return error;
			if ((error = fmsReadSector(fdEntry->buffer, C_2_S(nextCluster)))) {
				//                printf("\nFailed reading");
				return error;
			}
			fdEntry->currentCluster = nextCluster;
		}
		bytestLeft = BYTES_PER_SECTOR - bufferIndex;
		if (bytestLeft > nBytes) {
			bytestLeft = nBytes;
		}
		if (bytestLeft > (fdEntry->fileSize - fdEntry->fileIndex)) {
			bytestLeft = fdEntry->fileSize - fdEntry->fileIndex;
		}
		memcpy(buffer, &fdEntry->buffer[bufferIndex], bytestLeft);
		fdEntry->fileIndex += bytestLeft;
		numBytesRead += bytestLeft;
		buffer += bytestLeft;
		nBytes -= bytestLeft;
	}

	return numBytesRead;

} // end fmsReadFile



// ***********************************************************************
// ***********************************************************************
// This function changes the current file pointer of the open file specified by
// fileDescriptor to the new file position specified by index.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// The file position may not be positioned beyond the end of the file.
// Return the new position in the file if successful; otherwise, return the error number.
//
int fmsSeekFile(int fileDescriptor, int index)
{
	// ?? add code here
	// printf("\nfmsSeekFile Not Implemented");
	// return ERR63;
	int error, nextCluster, clusters;
	FCB* fdEntry;
	fdEntry = &OFTable[fileDescriptor];
	if (fdEntry->name[0] == 0) return ERR63; // file not open
	if (fdEntry->mode == OPEN_WRITE || fdEntry->mode == OPEN_APPEND) return ERR85;

	if (index > fdEntry->fileSize || index < 0) return ERR80;

	if ((error = flushBuffer(fdEntry))) return error;

	fdEntry->fileIndex = 0;
	fdEntry->currentCluster = fdEntry->startCluster;
	clusters = index / BYTES_PER_SECTOR;
	if (index % BYTES_PER_SECTOR == 0) {
		clusters--;
	}

	while ((clusters > 0) && (nextCluster = getFatEntry(fdEntry->currentCluster, FAT1)) != FAT_EOC) {
		if (nextCluster == FAT_BAD) return ERR54;
		if (nextCluster < 2) return ERR54;
		fdEntry->currentCluster = nextCluster;
		clusters--;
	}

	fdEntry->fileIndex = (uint16)index;
	//    int sector = C_2_S(fdEntry->currentCluster);
	//    printf("\nCluster found in seek %d sector %d", fdEntry->currentCluster, sector);
	fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster));

	return fdEntry->fileIndex;
} // end fmsSeekFile



// ***********************************************************************
// ***********************************************************************
// This function writes nBytes bytes to the open file specified by fileDescriptor from
// memory pointed to by buffer.
// The fileDescriptor was returned by fmsOpenFile and is an index into the open file table.
// Writing is always "overwriting" not "inserting" in the file and always writes forward
// from the current file pointer position.
// Return the number of bytes successfully written; otherwise, return the error number.
//
int fmsWriteFile(int fileDescriptor, char* buffer, int nBytes)
{
	// ?? add code here
	// printf("\nfmsWriteFile Not Implemented");
	// return ERR63;

	int error;
	int nextCluster, endCluster;
	FCB* fdEntry;
	int numBytesWritten = 0;
	unsigned int bytestLeft, bufferIndex;
	fdEntry = &OFTable[fileDescriptor];
	if (fdEntry->name[0] == 0) return ERR63; // file not open
	if (fdEntry->mode == OPEN_READ) return ERR85; //illegal access
//    bool debug = fdEntry->fileIndex == 1024 || fdEntry->fileIndex == 2000 || fdEntry->fileIndex == 510;
	bool debug = FALSE;
	if (debug) { printf("\n\tBegin writing buffer |%s| to fileIndex %d", buffer, fdEntry->fileIndex); }

	while (nBytes > 0) {
		bufferIndex = fdEntry->fileIndex % BYTES_PER_SECTOR;
		if (debug) { printf("\n\tbufferIndex %d", bufferIndex); }
		if ((bufferIndex == 0) && (fdEntry->fileIndex || !fdEntry->currentCluster)) {
			if (fdEntry->currentCluster == 0) { //file has been lazy opened and buffer is not initialized
				if (debug) { printf("\n\tlazy loading"); }
				if (fdEntry->startCluster == 0) {
					if (debug) { printf("\n\tunitiialized"); }
					if ((nextCluster = getFreeDataCluster()) < 2) return nextCluster;
					if (debug) { printf("\ngot new cluster to append %d when writing |%s|", nextCluster, buffer); }
					fdEntry->startCluster = (uint16)nextCluster;
					//                    printf("\nAssigning cluster %d to file %s", nextCluster, fdEntry->name);
					setFatEntry(nextCluster, FAT_EOC, FAT1);
					setFatEntry(nextCluster, FAT_EOC, FAT2);
				}
				nextCluster = fdEntry->startCluster;
				fdEntry->fileIndex = 0;
			}
			else {
				if (debug) { printf("\n\tneed to load the next cluster"); }
				nextCluster = getFatEntry(fdEntry->currentCluster, FAT1);
				if (nextCluster == FAT_EOC) {
					if (debug) { printf("\n\tno more clusters"); }
					//we have reached the end of the cluster but still have more to write
					//get a new cluster and append it to the end of the cluster chain
					if ((nextCluster = getFreeDataCluster())) {
						if (debug) { printf("\n\tgot a new one %d", nextCluster); }
						if (nextCluster < 2) {
							return nextCluster; // error
						}
						setFatEntry(fdEntry->currentCluster, nextCluster, FAT1);
						setFatEntry(fdEntry->currentCluster, nextCluster, FAT2);
						setFatEntry(nextCluster, FAT_EOC, FAT1);
						setFatEntry(nextCluster, FAT_EOC, FAT2);
					}

				}
				if (debug) { printf("\nnext cluster is %d", nextCluster); }
			}
			if (debug) { printf("\n\twill flush old buffer? %s", fdEntry->flags & BUFFER_ALTERED ? "true" : "false"); }
			if (debug && fdEntry->flags & BUFFER_ALTERED) { printf("\n\tflushing |%s|", fdEntry->buffer); }
			if ((error = flushBuffer(fdEntry))) return error;

			fdEntry->currentCluster = (uint16)nextCluster;
		}
		if (!(fdEntry->flags & BUFFER_ALTERED)) {
			if (debug) { printf("\n\tloading next cluster"); }
			if ((error = fmsReadSector(fdEntry->buffer, C_2_S(fdEntry->currentCluster)))) return error;
		}

		bytestLeft = BYTES_PER_SECTOR - bufferIndex;
		if (bytestLeft > nBytes) {
			bytestLeft = nBytes;
		}
		if (debug) { printf("\n\twriting %d bytes |%s| to index %d", bytestLeft, buffer, bufferIndex); }
		memcpy(&fdEntry->buffer[bufferIndex], buffer, bytestLeft);
		fdEntry->fileIndex += bytestLeft;
		numBytesWritten += bytestLeft;
		buffer += bytestLeft;
		nBytes -= bytestLeft;
		fdEntry->flags |= BUFFER_ALTERED;
		fdEntry->fileSize = fdEntry->mode != OPEN_RDWR || fdEntry->fileIndex > fdEntry->fileSize ? fdEntry->fileIndex : fdEntry->fileSize;
		if (debug) { printf("\n\tend loop with nBytes = %d", nBytes); }
	}

	if (fdEntry->mode != OPEN_RDWR || fdEntry->fileIndex > fdEntry->fileSize) {
		endCluster = fdEntry->currentCluster;
		while ((nextCluster = getFatEntry(endCluster, FAT1)) != FAT_EOC) {
			//        printf("\nNext Cluster %d", nextCluster);
			//        we have completed writing and the chain hasn't ended
			//        set the further links free
			setFatEntry(endCluster, 0x00, FAT1);
			setFatEntry(endCluster, 0x00, FAT2);
			endCluster = nextCluster;
		}
		setFatEntry(fdEntry->currentCluster, FAT_EOC, FAT1);
		setFatEntry(fdEntry->currentCluster, FAT_EOC, FAT2);
	}
	if (debug) { printf("\n\twrote %d bytes", numBytesWritten); }
	if (debug) { printf("\n\tend writing buffer |%s|", buffer); }

	return numBytesWritten;
} // end fmsWriteFile


// ***********************************************************************
// ***********************************************************************
// Peripherals functions
// 
int getFreeCluster(int begin, int end)
{
	int cluster, error, sector;
	char buffer[BYTES_PER_SECTOR];
	memset(buffer, 0, BYTES_PER_SECTOR * sizeof(char));

	for (cluster = begin; cluster < end; cluster++)
	{

		if (!getFatEntry(cluster, FAT1)) // if its an Unused cluster
		{
			// overwrite any data that may have been in the sector associated with cluster
			sector = C_2_S(cluster);
			if ((error = fmsWriteSector(buffer, sector)) < 0) return error;
			return cluster;
		}
	}

	return ERR65; // No empty entries found, so file space is full
} // end getFreeCluster

int getFreeRootCluster()
{
	return getFreeCluster(BEG_ROOT_SECTOR, BEG_DATA_SECTOR);
}

int getFreeDataCluster()
{
	return getFreeCluster(BEG_DATA_SECTOR, SECTORS_PER_DISK);
}

int getDirEntry(int* dirNum, char* mask, DirEntry* dirEntry, int* dirSector)
{
	char buffer[BYTES_PER_SECTOR];

	int loop = *dirNum / ENTRIES_PER_SECTOR;

	int dir = CDIR;
	int dirCluster = dir, nextCluster;
	int dirIndex, error;

	while (1)																// load directory sector
	{
		if (dir)
		{
			while (loop--)
			{
				nextCluster = getFatEntry(dirCluster, FAT1);

				if (nextCluster == FAT_EOC)									// need to expand the dir
				{
					if ((nextCluster = getFreeDataCluster()) < 2) return nextCluster;

					setFatEntry(dirCluster, nextCluster, FAT1);
					setFatEntry(dirCluster, nextCluster, FAT2);

					setFatEntry(nextCluster, FAT_EOC, FAT1);
					setFatEntry(nextCluster, FAT_EOC, FAT2);
				}

				if (nextCluster == FAT_BAD) return ERR54;					// error Invalid FAT Chain
				if (nextCluster < 2) return ERR54;							// error Invalid FAT Chain

				dirCluster = nextCluster;
			}

			*dirSector = C_2_S(dirCluster);
		}// sub directory

		else
		{
			*dirSector = (*dirNum / ENTRIES_PER_SECTOR) + BEG_ROOT_SECTOR;
			if (*dirSector >= BEG_DATA_SECTOR) return ERR67;
		}// root directory

		if ((error = fmsReadSector(buffer, *dirSector))) return error;		// read sector into directory buffer

		while (1)															// find next matching directory entry
		{
			// read directory entry
			dirIndex = *dirNum % ENTRIES_PER_SECTOR;
			memcpy(dirEntry, &buffer[dirIndex * sizeof(DirEntry)], sizeof(DirEntry));
			if (dirEntry->name[0] == 0) return ERR67;						// EOD
			(*dirNum)++;                        							// prepare for next read

			if (dirEntry->name[0] == 0xe5);     							// deleted entry. keep going
			else if (dirEntry->attributes == LONGNAME);
			else if (fmsMask(mask, dirEntry->name, dirEntry->extension)) return 0; // return if valid			  

			if ((*dirNum % ENTRIES_PER_SECTOR) == 0) break;					// break if sector boundary
		}
		loop = 1;															// next directory sector/cluster
	}
	return 1;
} // end getDirEntry

int getFreeDirEntry(int* dirNum, DirEntry* dirEntry, int* dirSector)
{
	int dir = CDIR;
	char buffer[BYTES_PER_SECTOR];
	int dirIndex, error;
	int loop = *dirNum / ENTRIES_PER_SECTOR;
	int dirCluster = dir, nextCluster;

	while (1)
	{	// load directory sector
		if (dir)
		{	// sub directory
			while (loop--)
			{
				nextCluster = getFatEntry(dirCluster, FAT1);
				if (nextCluster == FAT_EOC) {
					// we want to expand the dir then
					if ((nextCluster = getFreeDataCluster()) < 2) return nextCluster;
					setFatEntry(dirCluster, nextCluster, FAT1);
					setFatEntry(dirCluster, nextCluster, FAT2);
					setFatEntry(nextCluster, FAT_EOC, FAT1);
					setFatEntry(nextCluster, FAT_EOC, FAT2);
				}
				if (nextCluster == FAT_BAD) return ERR54;
				if (nextCluster < 2) return ERR54;
				dirCluster = nextCluster;
			}
			*dirSector = C_2_S(dirCluster);
		}
		else
		{	// root directory
			*dirSector = (*dirNum / ENTRIES_PER_SECTOR) + BEG_ROOT_SECTOR;
			if (*dirSector >= BEG_DATA_SECTOR) return ERR67;
		}

		// read sector into directory buffer
		if ((error = fmsReadSector(buffer, *dirSector))) return error;

		// find next matching directory entry
		while (1)
		{	// read directory entry
			dirIndex = *dirNum % ENTRIES_PER_SECTOR;
			memcpy(dirEntry, &buffer[dirIndex * sizeof(DirEntry)], sizeof(DirEntry));
			if (dirEntry->name[0] == 0 || dirEntry->name[0] == 0xe5) {
				return 0;
			}	// found a free dirEntry
			(*dirNum)++;                        		// prepare for next read
			if ((*dirNum % ENTRIES_PER_SECTOR) == 0) break;
		}
		// next directory sector/cluster
		loop = 1;
	}
	return 1;
}



// ***********************************************************************
// Search OFTable to find empty slot for the given file
// returns 0 if sucess, else if
//						- another file is found with matching name and ext
//						- too many openned files
int fmsGetFCB(DirEntry* dirEntry)
{
	int i, entryInd = -1;
	FCB* fcbEntry;

	for (i = 0; i < NFILES; i++) 
	{
		fcbEntry = &OFTable[i];
		if (!strncmp(fcbEntry->name, dirEntry->name, 8) && !strncmp(fcbEntry->extension, dirEntry->extension, 3)) 
			return ERR62;												//both the name and extension match
		
		if (entryInd == -1 && fcbEntry->name[0] == 0) entryInd = i;		
	}

	return entryInd > -1 ? entryInd : ERR70; //if we found a entry return it otherwise too many files are open
} // end fmsGetFCB

// ***************************************************************************************
// Finds a dirEntry related to the fcbEntry and updates it accordingly
int fmsUpdateDirEntry(FCB* fcbEntry)
{
	DirEntry dirEntry;

	char fileName[32];
	char buffer[BYTES_PER_SECTOR];

	int dirNum = 0, dirIndex, error, dirSector;

	strcpy(fileName, fcbEntry->name);

	if ((error = getDirEntry(&dirNum, fileName, &dirEntry, &dirSector))) return error;

	dirNum--;
	dirIndex = dirNum % ENTRIES_PER_SECTOR;

	// Update potential changes of dirEntry
	dirEntry.fileSize = fcbEntry->fileSize;
	dirEntry.startCluster = fcbEntry->startCluster;
	dirEntry.attributes = fcbEntry->attributes;
	setDirTimeDate(&dirEntry);

	if ((error = fmsReadSector(buffer, dirSector))) return error;				// Read in entire sector

	memcpy(&buffer[dirIndex * sizeof(DirEntry)], &dirEntry, sizeof(DirEntry));	// update dirEntiry in sector

	if ((error = fmsWriteSector(&buffer, dirSector)) < 0) return error;			// write back entire sector

	fcbEntry->name[0] = 0;														// release fcbEntry

	return 0;
} // end fmsUpdateDirEntry

// ***********************************************************************
int flushBuffer(FCB* fcbEntry)
{
	int error;
	if (fcbEntry->flags & BUFFER_ALTERED)
	{		
		if ((error = fmsWriteSector(fcbEntry->buffer, C_2_S(fcbEntry->currentCluster)))) return error;
		fcbEntry->flags &= ~BUFFER_ALTERED;		
	}
	return 0;
} // end flushBuffer
