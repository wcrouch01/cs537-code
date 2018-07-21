#include <stdio.h>
#include <stdlib.h>
//By William Crouch
//Cs Login crouch
static int arr[3000] [500];

int main(int argc, char *argv[]){
        for (int c = 0; c< 500; c++){
                for (int r = 0; r < 3000; r++){
                        arr[r][c] = r+c;
                }
        }
}

