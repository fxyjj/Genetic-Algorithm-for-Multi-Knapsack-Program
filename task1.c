#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>


int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 5;
static int POP_SIZE = 100; //global parameters
int MAX_NUM_OF_GEN = 1000; //max number of generations
int MAX_TIME = 60;  //max amount of time permited (in sec)
float CROSSOVER_RATE = 0.8;
float MUTATION_RATE = 0.1;

struct solution_struct best_sln;  //global best solution

//return a random number between 0 and 1
float rand_01()
{
    float number;
    number = (float) rand();
    number = number/RAND_MAX;
    //printf("rand01=%f\n", number);
    return number;
}

//return a random nunber ranging from min to max (inclusive)
int rand_int(int min, int max)
{
    int div = max-min+1;
    int val =rand() % div + min;
    //printf("rand_range= %d \n", val);
    return val;
}


struct item_struct{
    int dim; //no. of dimensions
    int* size; //volume of item in all dimensions
    int p;
};

struct problem_struct{
    int n; //number of items
    int dim; //number of dimensions
    struct item_struct* items;
    int* capacities;  //knapsack capacities
};

struct solution_struct{
    struct problem_struct* prob; //maintain a shallow copy of the problem data
    float objective;
    int feasibility; //indicate the feasiblity of the solution
    int* x; //chromosome vector
    int* cap_left; //capacity left in all dimensions
};

void free_problem(struct problem_struct* prob)
{
    if(prob!=NULL)
    {
        if(prob->capacities !=NULL) free(prob->capacities);
        if(prob->items!=NULL)
        {
            for(int j=0; j<prob->n; j++)
            {
                if(prob->items[j].size != NULL)
                    free(prob->items[j].size);
            }
            free(prob->items);
        }
        free(prob);
    }
}

void init_problem(int n, int dim, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = malloc(sizeof(struct problem_struct));
    new_prob->n=n; 
    new_prob->dim=dim;
    new_prob->items=malloc(sizeof(struct item_struct)*n);
    for(int j=0; j<n; j++)
        new_prob->items[j].size= malloc(sizeof(int)*dim);
    new_prob->capacities = malloc(sizeof(int)*dim);
    *my_prob = new_prob;
}



//copy a solution from another solution
bool copy_solution(struct solution_struct* dest_sln, struct solution_struct* source_sln)
{
    if(source_sln ==NULL) return false;
    if(dest_sln==NULL)
    {
        dest_sln = malloc(sizeof(struct solution_struct));
    }
    else{
        free(dest_sln->cap_left);
        free(dest_sln->x);
    }
    int n = source_sln->prob->n;
    int m =source_sln->prob->dim;
    dest_sln->x = malloc(sizeof(int)*n);
    dest_sln->cap_left=malloc(sizeof(int)*m);
    for(int i=0; i<m; i++)
        dest_sln->cap_left[i]= source_sln->cap_left[i];
    for(int j=0; j<n; j++)
        dest_sln->x[j] = source_sln->x[j];
    dest_sln->prob= source_sln->prob;
    dest_sln->feasibility=source_sln->feasibility;
    dest_sln->objective=source_sln->objective;
    return true;
}

void free_population(struct solution_struct* pop, int size)
{
    for(int p=0; p<size; p++)
    {
        free(pop[p].x);
        free(pop[p].cap_left);
    }
}

void evaluate_solution(struct solution_struct* sln)
{
    //evaluate the feasibility and objective of the solution
    sln->objective =0; sln->feasibility = 1;
    struct item_struct* items_p = sln->prob->items;
    
    for(int i=0; i< items_p->dim; i++)
    {
        sln->cap_left[i]=sln->prob->capacities[i];
        for(int j=0; j<sln->prob->n; j++)
        {
            sln->cap_left[i] -= items_p[j].size[i]*sln->x[j];
            if(sln->cap_left[i]<0) {
                sln->feasibility = -1*i; //exceeding capacity
                return;
            }
        }
    }
    if(sln->feasibility>0)
    {
        for(int j=0; j<sln->prob->n; j++)
        {
            sln->objective += sln->x[j] * items_p[j].p;
        }
    }
}

// intialise the population with random solutions
void init_population(struct problem_struct* prob, struct solution_struct* pop)
{
    for(int p=0; p<POP_SIZE; p++)
    {
        pop[p].prob = prob;
        pop[p].x = malloc(sizeof(int)*prob->n);
        pop[p].cap_left = malloc(sizeof(int)*prob->dim);
        for(int j=0; j<prob->n; j++)    pop[p].x[j] = 0;
        for(int i=0; i<prob->dim; i++)  pop[p].cap_left[i]=prob->capacities[i];
        /* create a random initial x that is feasible */
        int j=rand_int(0, prob->n-1);
        while(true)
        {
            while(pop[p].x[j]==1)
            {
                j=rand_int(0, prob->n-1); //select an unpacked item randomly
            }
            //printf("trying item %d to pcak. \n", j);
            pop[p].x[j]=1;
            bool can_pack=true;
            for(int i=0; i< prob->dim; i++)
            {
                pop[p].cap_left[i] -= prob->items[j].size[i];
                if(pop[p].cap_left[i] <0) can_pack=false;
            }
            if(!can_pack)
            {   //unpack item i
                //printf("packing item %d failed. random initialisation stoped.\n", j);
                pop[p].x[j]=0;
                for(int i=0; i< prob->dim; i++)
                    pop[p].cap_left[i] += prob->items[j].size[i];
                break;
            }
        }
        evaluate_solution (&pop[p]);
    }
    // for(int p=0; p<POP_SIZE; p++)
    // {
    //     output_solution(&pop[p], NULL);
    // }
}

struct problem_struct** load_problems(const char* arg)
{
	FILE *fp = fopen(arg,"r");
	if(fp == NULL){
		printf("no such file exist.\n");
		return NULL;
	}
    int i,j,k;
    int num_of_probs;
    fscanf(fp,"%d",&num_of_probs);
    // printf("%d\n",num_of_probs);
    if(num_of_probs == 0) return NULL;
    
    struct problem_struct** my_problems = malloc(sizeof(struct problem_struct*)*num_of_probs);
    for(k=0; k<num_of_probs; k++)
    {
        int n, dim,bst_sln;
    	fscanf(fp,"%d",&n);
    	fscanf(fp,"%d",&dim);
    	fscanf(fp,"%d",&bst_sln);
    	// printf("%d %d %d\n",n,dim,bst_sln); 
        init_problem(n, dim, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].dim = dim;
            fscanf(fp,"%d",&my_problems[k]->items[j].p);
            // printf("+++%d %d ", my_problems[k]->items[j].p,my_problems[k]->items[j].dim);
           
        }
        // printf("\n");
        for(i=0; i<dim; i++){
        	for(j = 0;j < n;j++){
        		fscanf(fp,"%d",&my_problems[k]->items[j].size[i]);
        		// printf("---%d ", my_problems[k]->items[j].size[i]);
           
        	}
        }
        for(i = 0;i < dim;i++){

        	fscanf(fp,"%d",&my_problems[k]->capacities[i]); 
        	 // printf("%d\n",my_problems[k]->capacities[i]); 

        }
    }
    fclose(fp);
    return my_problems;
}

//generate a new population
void cross_over(struct solution_struct* curt_pop, struct solution_struct* new_pop)
{
	// struct solution_struct* copy_cpop = curt_pop;
  	for(int i = 0 ;i < POP_SIZE;i++){
  		int w = rand()%(99-i)+i;
  		int t = new_pop[i];
  		new_pop[i] = new_pop[w];
  		new_pop[w] = t;
  	}
  	for(int j = 0; j<POP_SIZE-1;j+=2){
  		if(rand_int(0,9) < 8){
  			int xpt = rand_int(1,curt_pop[0]->prob->n-1);
  			for(;xpt<curt_pop[0]->prob->n;xpt++){
  				int indx = new_pop[j]->x[xpt];
  				new_pop[j]->x[xpt] = new_pop[j+1]->x[xpt];
  				new_pop[j+1]->x[xpt] = indx;
  			}
  			
  		}
  	}	
}

//apply mutation to a population
void mutation(struct solution_struct* pop)
{
    for(int i = 0;i<POP_SIZE;i++){
    	for(int j = 0;j<pop[i]->prob->n;j++){
    		if(rand_int(0,9) == 0){
    			if(pop[i]->x[j] == 0){
    				pop[i]->x[j] = 1;
    			}else{
    				pop[i]->x[j] = 0;
    			}
    		}
    		
    	}
    }
}





int main(int argc, char const *argv[])
{
	
	
	struct problem_struct** test = load_problems(argv[1]);
	for(int i = 0;i<10;i++){
		printf("%d\n",rand_int(0,9));
	}
	
	return 0;
}