#include <stdlib.h>
#include <stdio.h>
#define SIZE 4
#define BLOCK_SIZE 2

//Helper to print a SIZExSIZE matrix
void printMatrix(float *mat);
//Helper to swap two floats
void swap(float *a, float *b);
//Transpose an entry in the matrix, n is the nth entry
void transpose(float *mat, int n);
//Transpose a 2x2 block of values in the matrix
//i, j denote the 0,0 element of the block
void transposeBlock(float *mat, int i, int j);
//Transpose a 2x2 block by determining the column/row we're moving
//from n, where n corresponds to the nth transpose block
void transposeBlockN(float *mat, int n);

int main(int argc, char **argv){
	float *matrix = malloc(sizeof(float) * SIZE * SIZE);
	for (int i = 0; i < SIZE * SIZE; ++i){
			matrix[i] = 1.0f * i;
	}
	printf("Initial matrix:\n");
	printMatrix(matrix);

	//Transpose the matrix
	//As it is now this will actually transpose the matrix then transpose it back
	// for (int n = 0; n < SIZE * SIZE; ++n)
	// 	transpose(matrix, n);

	// printf("Transposed matrix:\n");
	// printMatrix(matrix);

	for (int i = 0; i < (SIZE / BLOCK_SIZE * (SIZE / BLOCK_SIZE + 1)) / 2; ++i)
		transposeBlockN(matrix, i);

	return 0;
}
void printMatrix(float *mat){
	for (int i = 0; i < SIZE * SIZE; ++i){
		if (i % SIZE == 0 && i != 0)
			printf("\n");
		printf(" %*.2f ", 5, mat[i]);
	}
	printf("\n");
}
void swap(float *a, float *b){
	float tmp = *a;
	*a = *b;
	*b = tmp;
}
void transpose(float *mat, int n){
	if (n > SIZE * SIZE){
		printf("Invalid index: %d\n", n);
		return;
	}
	//calculate row (i) and column (j) in a row major representation
	int i = n % SIZE;
	int j = n / SIZE;
	//diagonal elements don't need any swapping
	if (i == j)
		return;
	//off-diagonal: swap it with the j-i entry
	swap(&mat[i + j * SIZE], &mat[i * SIZE + j]);
}
void transposeBlock(float *mat, int i, int j){
	if (i > SIZE || j > SIZE)
		printf("Invalid i or j: %d, %d\n", i, j);
	//Blocks on diagonals we swap the off-diagonal elements
	else if (i == j);
		// swap(&mat[i][j + 1], &mat[i + 1][j]);
	//Off-diagonal blocks swap with the corresponding block on the lower-triangle
	else {
		for (int m = 0; m < 2; ++m)
			for (int n = 0; n < 2; ++n);
				// swap(&mat[i + n][j + m], &mat[j + m][i + n]);
	}
}
void transposeBlockN(float *mat, int n){
	printf("\nFinding i,j for transposeblock # %d\n", n);

	int blockPerRow = SIZE / BLOCK_SIZE;
	int row = 0, col = 0;
	//The index of the block we want to read should be offset
	//by the # of blocks to the diagonal, ie if we're reading
	//transpose block 2 in a 4x4 matrix [0-2] we should read 
	//matrix block # 3
	//Determine the matrix block number from the transpose block number
	//and also find the row
	int seenBlocks = blockPerRow;
	int tBlockPerRow = blockPerRow;
	while (n >= seenBlocks){
		seenBlocks += blockPerRow;
		--tBlockPerRow;
		row += BLOCK_SIZE;
		n += blockPerRow - tBlockPerRow;
	}
	//Find the column
	//n % tBlockPerRow * BLOCK_SIZE gives us the column relative to the diagonal
	//block, which we then offset by the offset # of blocks
	// (blockPerRow - tBlockPerRow) * BLOCK_SIZE
	col = n % tBlockPerRow * BLOCK_SIZE + (blockPerRow - tBlockPerRow) * BLOCK_SIZE;

	printf("\tmatrix block #: %d, row: %d, col: %d\n", n, row, col);
	int idx = row * SIZE + col;
	printf("\t0, 0 of source location idx: %d\n", idx);
	//Move float ptr to source block
	float *src = mat + idx;
	printf("\tvalues:\n");
	for (int i = 0; i < BLOCK_SIZE; ++i){
		for (int j = 0; j < BLOCK_SIZE; ++j)
			printf("\t%*.2f", 5, src[i * SIZE + j]);
		printf("\n");
	}
}
