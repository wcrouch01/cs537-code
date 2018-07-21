#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>


#include "fs.h"

const char* ERR_INODE = "ERROR: bad inode.";
const char* ERR_ADDR_BND = "ERROR: bad direct address in inode.";
const char* ERR_ADDR_BIND = "ERROR: bad indirect address in inode.";
const char* ERR_DIR_ROOT = "ERROR: root directory does not exist.";
const char* ERR_DIR_FMT = "ERROR: directory not properly formatted.";
const char* ERR_DIR_PARENT = "ERROR: parent directory mismatch.";
const char* ERR_DBMP_FREE = "ERROR: address used by inode but marked free in bitmap.";
const char* ERR_DBMP_USED = "ERROR: bitmap marks block in use but it is not in use.";
const char* ERR_ADDR_DUP = "ERROR: direct address used more than once.";
const char* ERR_ADDR_INDUP = "ERROR: indirect address used more than once.";
const char* ERR_ITBL_USED = "ERROR: inode marked use but not found in a directory.";
const char* ERR_ITBL_FREE = "ERROR: inode referred to in directory but marked free.";
const char* ERR_FILE_REF = "ERROR: bad reference count for file.";
const char* ERR_DIR_REF = "ERROR: directory appears more than once in file system.";

// structure pointers used
void *img;
void *data;
struct dinode *mydinode;
struct superblock *sblck;
char *bitmap;


// Check address
void findaddr(char *nbitmap, int indirect_flag, uint addr) {
    if(addr == 4294967295) {
        if (indirect_flag == 1) {
            fprintf(stderr, "%s\n", ERR_ADDR_BIND);
            exit(1);
        }
        else {
            fprintf(stderr, "%s\n", ERR_ADDR_BND);
            exit(1);
        }
    }
    char byte = *(bitmap + addr / 8);
    if ((byte >> (addr % 8)) & 0x01) {
        int upper = sblck->ninodes/IPB + sblck->nblocks/BPB + 4 + sblck->nblocks;
        int lower = sblck->ninodes/IPB + sblck->nblocks/BPB + 4;
        // see if addr is in bounds
        if ((addr < lower) || (addr >= upper)) {
            if(indirect_flag == 1)
            {
                fprintf(stderr, "%s\n", ERR_ADDR_BIND);
            }
            else
            {
                fprintf(stderr, "%s\n", ERR_ADDR_BND);
            }
            exit(1);
        }
    }
    else {
        fprintf(stderr, "%s\n", ERR_DBMP_FREE);
        exit(1);
    }
}

// Get blk #
uint get(int indirect_flag , uint offset, struct dinode *current_dinode) {
    if (indirect_flag || offset / BSIZE > NDIRECT) {
        return *((uint*) (img + current_dinode->addrs[NDIRECT] * BSIZE) + offset / BSIZE - NDIRECT);
    }
    else {
        return current_dinode->addrs[offset / BSIZE];
    }
}

void check(int *reftable, char* nbitmap, int inum, int parent_inum) {
    
    int offset = 0;
    struct dinode *current_dinode = mydinode + inum;
    if (0 == current_dinode->type) {
        fprintf(stderr, "%s\n", ERR_ITBL_FREE);
        exit(1);
    }
    
    // check if . or ..
    if (current_dinode->size == offset && current_dinode->type == T_DIR) {
        fprintf(stderr, "%s\n", ERR_DIR_FMT);
    }
    
    // change reference count
    reftable[inum]++;
    if (current_dinode->type == T_DIR && reftable[inum] > 1) {
        fprintf(stderr, "%s\n", ERR_DIR_REF);
        exit(1);
    }
    
    
    //flag to check if we are accessing an indirect block
    int indflag = 0;
    
    while (offset < current_dinode->size) {
        uint addr = get(indflag, offset, current_dinode);
        
        if (!(offset / BSIZE != NDIRECT) && !indflag) {
            offset -= BSIZE;
            indflag = 1;
        }
        findaddr(nbitmap, indflag, addr);
        // Check dup and note when inode is encountered
        if (reftable[inum] == 1) {
            char byte = *(nbitmap + addr / 8);
            if (!((byte >> (addr % 8)) & 0x01)) {
                byte = byte | (0x01 << (addr % 8));
                *(nbitmap + addr / 8) = byte;
            }
            else {
                if(indflag == 0)
                    fprintf(stderr, "%s\n", ERR_ADDR_DUP);
                else
                    fprintf(stderr, "%s\n", ERR_ADDR_INDUP);
                exit(1);
            }
        }
        
        if (current_dinode->type == T_DIR) {
            struct dirent *temp = (struct dirent *) (img + addr * BSIZE);
            
            // Check . and .. in DIR
            if (offset == 0) {
                if (strcmp(temp->name, ".") || strcmp((temp + 1)->name, "..")) {
                    fprintf(stderr, "%s\n", ERR_DIR_FMT);
                    exit(1);
                }
                
                
              
                if (parent_inum != (temp + 1)->inum) {
                    if (ROOTINO != inum) {
                        fprintf(stderr, "%s\n", ERR_DIR_PARENT);
                    } else {
                        fprintf(stderr, "%s\n", ERR_DIR_ROOT);
                    }
                    exit(1);
                }
                
                temp = temp + 2;
            
            }
            
            for (; temp < (struct dirent *)(ulong)(img + (addr + 1) * BSIZE); temp++) {
                if (temp->inum == 0) {
                    continue;
                }
                
                // check through again
                check(reftable, nbitmap, temp->inum, inum);
            }
        }
        offset += BSIZE;
    }
}

int main(int argc, char** argv)
{
    int i;
    int structcheck;
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "image not found.\n");
        exit(1);
    }
    char final = 0x00;
    struct stat statcheck;
    structcheck = fstat(fd, &statcheck);
    if(structcheck != 0)
        exit(1);
    // map file
    img = mmap(NULL, statcheck.st_size, PROT_READ, MAP_PRIVATE,
                   fd, 0);
    if(img == NULL)
        exit(1);
    sblck = (struct superblock *) (img + BSIZE);
    int byte_offset = 4;
    int bit_offset = 8;
    
    //create the bitmap
    bitmap = (char *) (img + (sblck->ninodes / IPB + 3) * BSIZE);
    int data_offset = byte_offset +  sblck->nblocks/BPB + sblck->ninodes/IPB;
    int mapsz = (byte_offset + sblck->ninodes/IPB + sblck->nblocks + sblck->nblocks/BPB) / bit_offset;
    int reftable[sblck->ninodes + 1];
    
    //create the data blocks
    data = (void *) (img + BSIZE * (sblck->nblocks/BPB + sblck->ninodes/IPB + byte_offset));
    //create the inode structure
    mydinode = (struct dinode *) (img + (byte_offset / 2) * BSIZE);
    for(int j = 0; j < (sblck->ninodes + 1); j++)
    {
        reftable[j] = 0;
    }
    char nbitmap[mapsz];
    // Initialize new bitmap
    int loop = 0;
    for(; loop < mapsz; loop++)
    {
        nbitmap[loop] = (char) 0;
    }
    //append
    memset(nbitmap, 0xFF, data_offset / bit_offset);
    
    for (i = 0; i < data_offset % bit_offset; ++i) {
        final = (final << 1) | 0x01;
    }
    nbitmap[data_offset / bit_offset] = final;
    check(reftable, nbitmap, ROOTINO, ROOTINO);
    // Check root directory
    if ((mydinode + ROOTINO)->type != T_DIR || !(mydinode + ROOTINO)) {
        fprintf(stderr, "%s\n", ERR_DIR_ROOT);
        exit(1);
    }
    
    struct dinode *current_dinode = mydinode;
    
    for (i = 1; i < sblck->ninodes; i++) {
        current_dinode++;
        // check if allocated
        if (0 == current_dinode->type) {
            continue;
        }
        
        // Inode not in directory but in use
        if (reftable[i] == 0) {
            fprintf(stderr, "%s\n", ERR_ITBL_USED);
            exit(1);
        }
        
        // check if types are valid
        if (current_dinode->type != T_DIR && current_dinode->type != T_FILE
            && current_dinode->type != T_DEV) {
            fprintf(stderr, "%s\n",ERR_INODE);
            exit(1);
        }
        
        // more links in directory
        if (T_DIR == current_dinode->type && current_dinode->nlink > 1 && (i != 1)) {
            fprintf(stderr, "%s\n", ERR_DIR_REF);
            exit(1);
        }
        
        //bad reference count
        if (reftable[i] != current_dinode->nlink && (current_dinode->type == T_FILE)) {
            fprintf(stderr, "%s\n", ERR_FILE_REF);
            exit(1);
        }
    }
    //check that both the bitmaps are equal
    for (int c = 0; c < mapsz; ++c) {
        if (bitmap[c] != nbitmap[c]) {
            fprintf(stderr, "%s\n", ERR_DBMP_USED);
            exit(1);
        }
    }
    
    
    return 0;
}

