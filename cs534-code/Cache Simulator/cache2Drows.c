#include <stdio.h>
#include <stdlib.h>

static int arr[3000] [500];
//By William Crouch
//Cs Login crouch
int main(int argc, char *argv[]){
	for (int r = 0; r < 3000; r++){
		for (int c = 0; c < 500; c++){
			arr[r][c] = r+c;
		}
	}
}
