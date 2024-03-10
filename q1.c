#include <stdio.h>
#include <mpi.h>
#include <limits.h>
#include <stdbool.h>


//function to swap two elements
void swap(int *a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp;
}

//function to reverse the elements of the array
void reverse(int* first, int* last)
{
    while((first!=last) && (first!=--last))
    {
        swap(first, last);
        ++first;
    }
}

bool next_permutation(int* first, int* last)
{
    if(first==last)
        return false;

    int* i = first;
    ++i;
    if(i==last)
        return false;
    i = last;
    --i;

    while(1)
    {
        int* ii = i;
        --i;
        if(*i < *ii)
        {
            int* j = last;
            while(!(*i < *--j))
                ;
            swap(i, j);
            reverse(ii, last);
            return true;
        }
        if(i==first)
        {
            reverse(first, last);
            return false;
        }
    }


}

//function which will be called by each mpi processs to search on its subset of vertices/permutation
int searchMinPath(int graph[4][4], int n, int s, int *vertices, int rank, int* optimalPath){

    int min_pathCost = INT_MAX;

    do {
        int current_pathCost = 0;
        current_pathCost = graph [s][vertices[0]]; //start path with starting vertex
        for (int i = 0; i < n-2; i++) {
            current_pathCost += graph[vertices[i]][vertices[i] + 1];
        }

        current_pathCost += graph[vertices[n-2]][s];  //return to starting vertex
        if (current_pathCost < min_pathCost) {
            min_pathCost = current_pathCost;
            for (int i = 0; i < n-1; i++) {
                optimalPath[i] = vertices[i];
            }
        }
    } while (next_permutation(vertices, vertices + n -1 ));

    return min_pathCost;
}

int main(int argc, char** argv) 
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    
    int n = 4;
    int graph[4][4] = { {0, 10, 15, 20},
                        {10, 0, 35, 25},
                        {15, 35, 0, 30},
                        {20, 25, 30, 0}
                      };
    int s = 0;
    int vertices[4] = {0, 1, 2, 3};

    //domain decomposition of graph into different subsets
    int vertex_per_process = (n-1) / size;
    int remaining_vertex = (n-1) % size;
    int start = rank * vertex_per_process;
    if (rank < remaining_vertex) {
        start = start + rank;
    } else {
        start = start + remaining_vertex;
    }
    int end = start + vertex_per_process -1 ;
    if (rank < remaining_vertex) {
        end++;
    }
    if (end >= n-1) {
    end = n - 2; // end index boundary
    }

    int min_cost = INT_MAX;
    int optimalPath[n-1];
    for(int i=start; i<=end; i++){
        printf("Process %d: %d\n", rank, vertices[i]);
        //each process explores different paths
        min_cost = searchMinPath(graph, n, vertices[i], vertices, rank, optimalPath);
        printf("Minimum path from process %d is %d\n", rank, min_cost); 
    }
    MPI_Barrier(MPI_COMM_WORLD);
    int final_min_cost;
    MPI_Allreduce(&min_cost, &final_min_cost, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    
    if (min_cost == final_min_cost && rank != 0) { // This process has the optimal path and is not the root
        // Send the optimal path to the root process
        MPI_Send(optimalPath, n-1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    else if (min_cost == final_min_cost && rank == 0) { // This process has the optimal path and is the root
        printf("--Final Minimum Cost: %d\n", final_min_cost);
        printf("--Optimal path from process %d: ", rank);
        for (int j = 0; j < n-1; j++) {
            printf("%d ", optimalPath[j]);
        }
        printf("\n");
    }


    if(rank==0 && final_min_cost != min_cost){
        printf("--Final Minimum Cost: %d\n", final_min_cost);
        MPI_Recv(optimalPath, n-1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("--Optimal path from process %d: ", rank);
        for (int j = 0; j < n-1; j++) {
            printf("%d ", optimalPath[j]);
        }
    }
  
    MPI_Finalize();
    return 0;
}

//the travelling salesman problem using MPI
//implemented the permutation function and the minimum path function
//Domain decomposition of the vertices
//each process will calculate the minimum path for each part
//all processes will print the result
//use the MPI_Allreduce function to find the minimum path and cost
