#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

char* populate_grid(FILE* ig, int r, int c);
void advance_generation(char** grid, int r, int c, int generations);
void print_grid(char* grid, int r, int c);
void check_argument(int dest, char* src, char* end);

int main(int argc, char* argv[]){
	char *endg, *endr, *endc;
	int rows = 12, columns = 12, generations = 10;
	char *filename = "life.txt";
	if(argc > 5){
		printf("Too many arguments. Exiting.");
		exit(1);
	}
	//Assumes the order: life rows columns filenames gen
	switch(argc){
		case 5:
			generations = strtol(argv[4], &endg, 10);
			check_argument(generations, endg, argv[4]);
		case 4:
			filename = argv[3];
		case 3:
			rows = strtol(argv[1], &endr, 10) + 2;
			columns = strtol(argv[2], &endc,10) + 2;
			check_argument(rows, endr, argv[1]);
			check_argument(columns, endc, argv[2]);
			
	}
	FILE *ig = fopen(filename, "r");
	if((ig == NULL)){
		printf("Error opening file.\n");
		exit(1);
	}
	char *grid = populate_grid(ig, rows, columns);
	advance_generation(&grid, rows, columns, generations);
	fclose(ig);
	free(grid);
}
char* populate_grid(FILE *ig, int r, int c){
	int rows = r, columns = c;
	int filechar, eof_reached = 0, nl_reached = 0;
	char* grid = (char *)malloc(rows * columns * sizeof(char));
	for(int i = 0; i < rows; i++){
		nl_reached = 0;
		for(int j = 0; j < columns; j++){
			//checking to see if a newline/EOF is reached, or if the index is
			//a part of the extra row/column added around the grid
			if((eof_reached == 1) || (nl_reached == 1) || (i == 0) || (i == rows - 1) || (j == 0) || (j == columns - 1)){
				*(grid + i * columns + j) = '-';
				continue;
			}
			filechar = fgetc(ig);
			if(filechar == EOF){
				eof_reached = 1;
				*(grid + i * columns + j) = '-';
			}       //Ascii 42 for *, 32 for SPACE, 10 for NEWLINE
			else if(filechar == 42) *(grid + i * columns + j) = filechar;
			else if(filechar == 32) *(grid + i * columns + j) = '-';
			else if(filechar == 10){
				*(grid + i * columns + j) = '-';
				nl_reached = 1;
			}
		}
	}
	return grid;
}
void advance_generation(char** grid, int r, int c, int generations){	
	int current_gen = 0;
	int live_neighbors = 0;
	int stop_index = (r * c);
	char *new_grid = (char *)malloc(r * c * sizeof(char));
	char *temp;//temp variable to help with switching pointers
	printf("================================\n");
	printf("Generation 0. Rows: %i, Columns:%d\n",r, c );
	print_grid(*grid, r, c);
	//Loop to make the following generations of cells
	while(current_gen < generations){
		for(size_t i = 0; i < stop_index; i++){
			if((i < 12) || (i >= (r-1)*c) || (i % c == 0) || (i %c == (c-1))){
				*(new_grid + i) = '-';			
				continue;
			}
			//Pointer arithmetic to check the surrounding cells
			if(*(*grid + i - (c + 1)) == 42) live_neighbors++;
			if(*(*grid + i - c) == 42) 	 live_neighbors++;
			if(*(*grid + i - (c - 1)) == 42) live_neighbors++;
			if(*(*grid + i - 1) == 42) 	 live_neighbors++;
			if(*(*grid + i + 1) == 42) 	 live_neighbors++;
			if(*(*grid + i + (c - 1)) == 42) live_neighbors++;
			if(*(*grid + i + c) == 42) 	 live_neighbors++;
			if(*(*grid + i + (c + 1)) == 42) live_neighbors++;
			
			//Adding a new live/dead cell to new_grid based on live_neighbors
			if(*(*grid + i) == 42){
				if(live_neighbors < 2) *(new_grid + i) = '-';
				else if((live_neighbors == 2) || (live_neighbors == 3)) *(new_grid + i) = '*';
				else if(live_neighbors > 3) *(new_grid + i) = '-';
			}
			else if((*(*grid + i) == 45) && (live_neighbors == 3)) *(new_grid + i) = '*';
			else *(new_grid + i) = '-';
			live_neighbors = 0;
		}
		printf("================================\n");
		printf("Generation %d. Rows: %i Columns: %i:\n", current_gen + 1, r, c);
		//swapping grid and new_grid
		temp = *grid;
		*grid = new_grid;
		new_grid = temp;
		print_grid(*grid, r, c);
		current_gen++;
	}
	free(new_grid);
}
void print_grid(char* grid, int r, int c){
	char gridletter;
	for(int i = 1; i < r - 1; i++){
		for(int j =  1; j < c - 1; j++){
			gridletter = *(grid + i * c + j);
			printf("%c", gridletter);
		}
		printf("\n");
	}
} 
void check_argument(int dest, char* src, char* end){
	//Check to see if the arguments given are legitimate. Program exits when it detects an invalid input
	errno = 0;
	if(end == src || ((dest == LONG_MAX || dest == LONG_MIN) && errno == ERANGE) || dest < 0){
		printf("Invalid input. Exiting.\n");
		exit(1);
	}
	
}
