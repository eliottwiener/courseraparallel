// MP 3: Due Sunday, Dec 30, 2012 at 11:59 p.m. PST
#include    <wb.h>
#define TILE_SIZE 4
#define wbCheck(stmt) do {                                 \
		cudaError_t err = stmt;                            \
		if (err != cudaSuccess) {                          \
		    wbLog(ERROR, "Failed to run stmt ", #stmt);    \
		    return -1;                                     \
		}                                                  \
	} while(0)
// Compute C = A * B
__global__ void matrixMultiplyShared(float * A, float * B, float * C,
		                             int numARows, int numAColumns,
		                             int numBRows, int numBColumns,
		                             int numCRows, int numCColumns) {
	//@@ Insert code to implement matrix multiplication here
	//@@ You have to use shared memory for this MP
	__shared__ float ds_M[TILE_SIZE][TILE_SIZE];
	__shared__ float ds_N[TILE_SIZE][TILE_SIZE];
	int bx = blockIdx.x;
	int by = blockIdx.y;
	int tx = threadIdx.x;
	int ty = threadIdx.y;
	int row = by * TILE_SIZE + ty;
	int col = bx * TILE_SIZE + tx;
	float pVal = 0;
	for(int i = 0; i < (numAColumns-1)/TILE_SIZE+1; ++i){
		if(row < numARows && (i * TILE_SIZE + tx) < numAColumns){
			ds_M[ty][tx] = A[row * numAColumns + (i * TILE_SIZE + tx)];
		} else {
			ds_M[ty][tx] = 0;
		}
		if((i * TILE_SIZE + ty) < numBRows && col < numBColumns){
			ds_N[ty][tx] = B[(i * TILE_SIZE + ty)*numBColumns+col];
		} else {
			ds_N[ty][tx] = 0;
		}
		__syncthreads();
		if(row < numCRows && col < numCColumns) {
			for(int j = 0; j < TILE_SIZE; ++j){
				pVal += ds_M[ty][j] * ds_N[j][tx];
			}
		}
		__syncthreads();
	}
	if(row < numCRows && col < numCColumns) C[row * numCColumns + col] = pVal;
}
int main(int argc, char ** argv) {
	wbArg_t args;
	float * hostA; // The A matrix
	float * hostB; // The B matrix
	float * hostC; // The output C matrix
	float * deviceA;
	float * deviceB;
	float * deviceC;
	int numARows; // number of rows in the matrix A
	int numAColumns; // number of columns in the matrix A
	int numBRows; // number of rows in the matrix B
	int numBColumns; // number of columns in the matrix B
	int numCRows; // number of rows in the matrix C (you have to set this)
	int numCColumns; // number of columns in the matrix C (you have to set this)
	args = wbArg_read(argc, argv);
	wbTime_start(Generic, "Importing data and creating memory on host");
	hostA = (float *) wbImport(wbArg_getInputFile(args, 0), &numARows, &numAColumns);
	hostB = (float *) wbImport(wbArg_getInputFile(args, 1), &numBRows, &numBColumns);
	//@@ Set numCRows and numCColumns
	if(numAColumns != numBRows){
		wbLog(ERROR, "input matrix dimensions are invalid");
		return -1;
	}
	numCRows = numARows;
	numCColumns = numBColumns;
	//@@ Allocate the hostC matrix
	int cSize = numCRows * numCColumns * sizeof(float);
	hostC = (float *) malloc(cSize);
	wbTime_stop(Generic, "Importing data and creating memory on host");
	wbLog(TRACE, "The dimensions of A are ", numARows, "rows x ", numAColumns, "columns");
	wbLog(TRACE, "The dimensions of B are ", numBRows, "rows x ", numBColumns, "columns");
	wbLog(TRACE, "The dimensions of C are ", numCRows, "rows x ", numCColumns, "columns");
	wbTime_start(GPU, "Allocating GPU memory.");
	//@@ Allocate GPU memory here
	int aSize = numARows * numAColumns * sizeof(float);
	int bSize = numBRows * numBColumns * sizeof(float);
	wbCheck(cudaMalloc((void **) &deviceA, aSize));
	wbCheck(cudaMalloc((void **) &deviceB, bSize));
	wbCheck(cudaMalloc((void **) &deviceC, cSize));
	wbTime_stop(GPU, "Allocating GPU memory.");
	wbTime_start(GPU, "Copying input memory to the GPU.");
	//@@ Copy memory to the GPU here
	wbCheck(cudaMemcpy(deviceA, hostA, aSize, cudaMemcpyHostToDevice));
	wbCheck(cudaMemcpy(deviceB, hostB, bSize, cudaMemcpyHostToDevice));
	wbTime_stop(GPU, "Copying input memory to the GPU.");
	//@@ Initialize the grid and block dimensions here
	dim3 dimGrid(ceil(numCColumns/TILE_SIZE), ceil(numCRows/TILE_SIZE), 1);
	dim3 dimBlock(TILE_SIZE, TILE_SIZE, 1);
	wbTime_start(Compute, "Performing CUDA computation");
	//@@ Launch the GPU Kernel here
	matrixMultiplyShared<<<dimGrid,dimBlock>>>(deviceA,deviceB,deviceC,
	                                           numARows,numAColumns,
	                                           numBRows,numBColumns,
	                                           numCRows,numCColumns);
	cudaThreadSynchronize();
	wbTime_stop(Compute, "Performing CUDA computation");
	wbTime_start(Copy, "Copying output memory to the CPU");
	//@@ Copy the GPU memory back to the CPU here
	cudaMemcpy(hostC, deviceC, cSize, cudaMemcpyDeviceToHost);
	wbTime_stop(Copy, "Copying output memory to the CPU");
	wbTime_start(GPU, "Freeing GPU Memory");
	//@@ Free the GPU memory here
	wbCheck(cudaFree(deviceA));
	wbCheck(cudaFree(deviceB));
	wbCheck(cudaFree(deviceC));
	wbTime_stop(GPU, "Freeing GPU Memory");
	wbSolution(args, hostC, numCRows, numCColumns);
	free(hostA);
	free(hostB);
	free(hostC);
	return 0;
}
