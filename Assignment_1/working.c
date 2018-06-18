#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* populate_grid(FILE* ig, int r, int c);
void advance_generation(char* grid, int r, int c, int generations);
void print_grid(char* grid, int r, int c);
int main(int argc, char* argv[]){
	/*By default there should be a 10x10 grid, but adding an extra row and
	  column on each side will help in avoiding edge cases
	*/
	
	FILE *ig = fopen("life.txt", "r");
	if(ig == NULL){
		printf("Error opening file.\n");
		exit(1);
	}
	int rows, columns;
	rows = 12;
	columns = 12;
	char *grid = populate_grid(ig, rows, columns);
	advance_generation(grid, rows, columns, 1);
	/*
		1  2  3  4  5  6  7
		8  9  10 11 12 13 14
		15 16 17 18 19 20 21
	*/
}
char* populate_grid(FILE *ig, int r, int c){
	int rows = r;
	int columns = c;
	int filechar, eof_reached, nl_reached;
	eof_reached = 0;
	nl_reached = 0;

	char *grid = (char *)malloc(rows * columns * sizeof(char));
	for(int i = 0; i < rows; i++){
		nl_reached = 0;
		for(int j = 0; j < columns; j++){
			if((eof_reached == 1) || (nl_reached == 1) || (i == 0) || (i == rows - 1) || (j == 0) || (j == columns - 1)){
				*(grid + i * columns + j) = '-';
				continue;
			}
			filechar = fgetc(ig);
			if(filechar == EOF){
				eof_reached = 1;
				*(grid + i * columns + j) = '-';
			}       //Ascii 42 for *, 32 for SPACE, 10 for NEWLINE
			else if(filechar == 42)
				*(grid + i * columns + j) = filechar;
			else if(filechar == 32)
				*(grid + i * columns + j) = '-';
			else if(filechar == 10){
				*(grid + i * columns + j) = '-';
				nl_reached = 1;
			}
		}
	}
	return grid;
}
void advance_generation(char* grid, int r, int c, int generations){	
	int current_gen = 0;
	int live_neighbors = 0;
	int start_index = c + 2;
	int stop_index = ((r - 1) * c) - 1;
	// - (c+-1), +-1, + (c+-1)
	printf("================================\n");
	printf("Generation 0");
	print_grid(grid, r, c);
	while(current_gen < generations){
		for(size_t i = start_index; i < stop_index; i++){
			if(i % c == 0)
				continue;
			if(*(grid + i - (c + 1)) == 42)
				live_neighbors++;
			if(*(grid + i - c) == 42)
				live_neighbors++;
			if(*(grid + i - (c + 1)) == 42)
				live_neighbors++;
			if(*(grid + i - 1) == 42)
				live_neighbors++;
			if(*(grid + i + 1) == 42)
				live_neighbors++;
			if(*(grid + i + (c - 1)) == 42)
				live_neighbors++;
			if(*(grid + i + c) == 42)
				live_neighbors++;
			if(*(grid + i + (c + 1)) == 42)
				live_neighbors++;
			
			if(*(grid + i) == 42){
				if(live_neighbors < 2)
					*(grid + i) = '-';
				else if((live_neighbors == 2) || (live_neighbors == 3))
					*(grid + i) = '*';
				else if(live_neighbors > 3)
					*(grid + i) = '-';
			}
			else if((*(grid + i) == 45) && (live_neighbors == 3))			
				*(grid + i) = '*';
			live_neighbors = 0;
		}
		printf("================================\n");
		printf("Generation %d:\n", current_gen + 1);
		print_grid(grid, r, c);
		current_gen++;
	}
}
void print_grid(char* grid, int r, int c){
	char gridletter;
	for(int i = 1; i < r - 1; i++){
		for(int j =  1; j < c - 1; j++){
			gridletter = *(grid + i * c + j);
			printf("%c ", gridletter);
		}
		printf("\n");
	}
} 
