//16522077 Senlin Xiao
//AIM-CW

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include<string.h>


int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 5;
static int POP_SIZE = 300; //global parameters
int MAX_NUM_OF_GEN = 200;//100; //max number of generations
int MAX_TIME;  //max amount of time permited (in sec)
float CROSSOVER_RATE = 0.8;//0.8
float MUTATION_RATE = 0.2;//0.2
int PROB_NUM = 0; // number of problem

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
    int BEST_OBJEXTIVE;
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

void init_problem(int n, int dim, int bst_sln, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = malloc(sizeof(struct problem_struct));
    new_prob->n=n; 
    new_prob->dim=dim;
    new_prob->BEST_OBJEXTIVE = bst_sln;
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
    // else{
        
    //      free(&(dest_sln->cap_left));
    //      free(&(dest_sln->x));
    // }
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
                sln->feasibility = -1; //exceeding capacity
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
// load problem information into problem structure
struct problem_struct** load_problems(const char* arg)
{
	FILE *fp = fopen(arg,"r");//use 'r' to add content int teh end of the file without overlay
	if(fp == NULL){
		printf("no such file exist.\n");
		return NULL;
	}
    int i,j,k;
    best_sln.objective = 0;//initialize the objective of best_sln structure 
    fscanf(fp,"%d",&PROB_NUM);
    if(PROB_NUM == 0) return NULL;
    struct problem_struct** my_problems = malloc(sizeof(struct problem_struct*)*PROB_NUM);
    for(k=0; k<PROB_NUM; k++)
    {
        int n, dim,bst_sln;
    	fscanf(fp,"%d",&n);
    	fscanf(fp,"%d",&dim);
    	fscanf(fp,"%d",&bst_sln);
        init_problem(n, dim,bst_sln, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].dim = dim;
            fscanf(fp,"%d",&my_problems[k]->items[j].p);  
        }
        //load the size information into problem structure
        for(i=0; i<dim; i++){
        	for(j = 0;j < n;j++){
        		fscanf(fp,"%d",&my_problems[k]->items[j].size[i]);
        	}
        }
        for(i = 0;i < dim;i++){
        	fscanf(fp,"%d",&my_problems[k]->capacities[i]); 
        }
    }
    fclose(fp);
    return my_problems;
}

//generate a new population
void cross_over(struct solution_struct* curt_pop, struct solution_struct* new_pop)
{
	//calculate the sum of objective of each solution, used to choose solutions for cross over process
    long int div = 0;
    for(int q = 0;q < POP_SIZE;q++){
        div += new_pop[q].objective;
    }
    // randomly choose two solutions while the solution with higher objective may have higher possibility to be chose
    for(int d = 0;d<POP_SIZE/2;d++){
        int j = 0,j1 = 0;
        int pob = rand_01()*div;
        int count = 0;
        for(int q1 = 0;q1 < POP_SIZE;q1++){
            count += new_pop[q1].objective;
            if(count>=pob){
                j = q1;
                break;
            }
        }
        int pob1 = rand_01()*div;
        int count1 = 0;
        for(int q1 = 0;q1 < POP_SIZE;q1++){
            count1 += new_pop[q1].objective;
            if(count1>=pob1){
                j1 = q1;
                break;
            }
        }
        //under the cross over rate, do cross over process in the two chosen solutions
  		    if(rand_01() < CROSSOVER_RATE){
  			   int xpt = rand_int(1,curt_pop[0].prob->n-2);
  			   for(;xpt<curt_pop[0].prob->n;xpt++){
  				  int indx = new_pop[j].x[xpt];
  				  new_pop[j].x[xpt] = new_pop[j1].x[xpt];
  				  new_pop[j1].x[xpt] = indx;
  			   }
  			evaluate_solution(&new_pop[j]);
            evaluate_solution(&new_pop[j1]);
  		}
    }
}

//apply mutation to a population
void mutation(struct solution_struct* pop)
{
    //loop for each solutions and every position on the chromosome of the solution could be mutated
    for(int i = 0;i<POP_SIZE;i++){
        int inh = rand_int(0,pop[i].prob->n-1);
    	if(rand_01()< MUTATION_RATE){
    		if(pop[i].x[inh] == 0){
    			pop[i].x[inh] = 1;
    		}else{
    			pop[i].x[inh] = 0;
    			 }
    	}
    	evaluate_solution(&pop[i]);
    }
}



//modify the solutions that violate the capacity constraints 
void feasibility_repair(struct solution_struct* pop)
{
    int num = pop[0].prob->n;
    //loop for each solutions
    for(int i = 0;i<POP_SIZE;i++){
        //repair the solution which the feasibility is a negative number.
        if(pop[i].feasibility < 0){
            while(true){
                int indx = 0;
                //find the dimension that the knapsack is exceed
                for(int j1 = 0;j1<pop[i].prob->dim;j1++){
                    if(pop[i].cap_left[j1]<0){
                        indx = j1;
                        break;
                    }
                }
                double pw = 0;
                int new_indx =  0;
                //calculate teh price/volume rate for each item in the exceeding dimension,and then drop items one by one from the item with smallest p/v rate until the knapsack capacity is enough
                for(int j2 = 0;j2 < num-1;j2++){
                    if(pop[i].x[j2] == 1){
                        pw = (double)pop[i].prob->items[j2].p/(double)pop[i].prob->items[j2].size[indx];
                        new_indx = j2;
                        if(pw > (double)pop[i].prob->items[j2+1].p/(double)pop[i].prob->items[j2+1].size[indx]){
                            pw = (double)pop[i].prob->items[j2+1].p/(double)pop[i].prob->items[j2+1].size[indx];
                            new_indx = j2;
                        }
                    }
                }
                pop[i].x[new_indx] = 0;
                evaluate_solution(&pop[i]);
                if(pop[i].feasibility > 0){
                    break;
                }
            }
        }
    }
}

//local search
void local_search_first_descent(struct solution_struct* pop)
{
    int num = pop[0].prob->n;
    int dimen = pop[0].prob->dim;
   for(int i = 0;i < POP_SIZE;i++){
        int p1 = 0;
        int p0 = 0;
        int indx0 = -1;
        int indx1 = -1;
        int arr_v0[dimen];//used to hold the size of the item in five dimensions
        int arr_v1[dimen];// used to hold teh size of the item in five dimensions
        for(int j = 0;j<num;j++){
            //find the dirst item in teh knapsack and first item that is not in the knapsack
           if(pop[i].x[j] == 1){
                if(indx1 == -1){
                    p1 = pop[i].prob->items[j].p;
                    for(int r = 0; r<dimen;r++) arr_v1[r] = pop[i].prob-> items[j].size[r];
                    indx1 = j;
                } 
           }
           else
           {
                if(indx0 == -1){
                    p0 = pop[i].prob->items[j].p;
                    for(int r = 0; r<dimen;r++) arr_v0[r] = pop[i].prob-> items[j].size[r];
                    indx0 = j;
                }
                
           }
           //**when two items are found, compare their price, if the price of the item out of the knapsack is better than that is in teh knapsack, 
           //**then if teh capacity of teh knapsack is not excess when drop the item with less price and put the item with larger price into the knapsack
           //** exhange these two items.
           if(indx0 != -1 && indx1 != -1){
                if(p0 > p1){
                    int check = 0;
                    for(int w = 0;w<dimen;w++){
                        if(pop[i].cap_left[w] - arr_v0[w] + arr_v1[w] < 0){
                            check = 1;
                            break;
                        } 
                    }
                    if(check == 1){
                        if(rand_int(0,9) < 2) indx1 = -1;// there is chance that the item first found in the knapsack may be changed, this is because if as first time, the item with the biggest price is found then none of teh item can be used to exchange with it.
                        j = indx0;
                        indx0 = -1;
                    }
                    else
                    {
                        //exchange steps
                        pop[i].x[indx0] = 1;
                        pop[i].x[indx1] = 0;
                        evaluate_solution(&pop[i]);
                        break;
                    }
                }
                else
                {
                    j = indx0;
                    indx0 = -1;
                }
           }
        }
   }
}

//replacement
void replacement(struct solution_struct* curt_pop, struct solution_struct* new_pop)
{
    struct solution_struct big[2*POP_SIZE];
    int a = 0;
    int b = 0;
    // put all the solutions into one array for sorting
    for(int i = 0;i<2*POP_SIZE-1;i+=2){
        copy_solution(&big[i],&curt_pop[a++]);
        copy_solution(&big[i+1],&new_pop[b++]);
    } 
    int check = 0;
    while(check == 0){
        check = 1;
        for(int i = 0;i<2*POP_SIZE-1;i++){
            if(big[i].objective < big[i+1].objective){
                struct solution_struct t;
                copy_solution(&t,&big[i]);
                free(big[i].x);
                free(big[i].cap_left);
                copy_solution(&big[i],&big[i+1]);
                free(big[i+1].x);
                free(big[i+1].cap_left);
                copy_solution(&big[i+1],&t);
                free(t.x);
                free(t.cap_left);
                check = 0;
            }
        }
    }
    //choose the first half solutions to be the population for next evolution.
    for(int j = 0 ;j < POP_SIZE;j++){
        free(curt_pop[j].x);
        free(curt_pop[j].cap_left);
        copy_solution(&curt_pop[j],&big[j]);
        free(new_pop[j].x);
        free(new_pop[j].cap_left);
        copy_solution(&new_pop[j],&big[j]);
    }
    free_population(big,2*POP_SIZE);
}

void output_solution(struct solution_struct* sln, const char* out_file)
{
    FILE *fp = fopen(out_file,"a+");
    if(fp == NULL){
        printf("Failed to open the file.");
        return;
    }else{
        fprintf(fp,"%d\n",(int)sln->objective);
        for(int i = 0;i<sln->prob->n;i++){
            fprintf(fp,"%d ",sln->x[i]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
    if(sln->prob->BEST_OBJEXTIVE != 0){
        printf("sln.feas=%d, sln.obj=%f,accurency: %f\n", sln->feasibility, sln->objective,sln->objective/sln->prob->BEST_OBJEXTIVE);
    
    }else{
        printf("sln.feas=%d, sln.obj=%f\n", sln->feasibility, sln->objective);
    
    }
    free(best_sln.cap_left); 
    free(best_sln.x);
    best_sln.objective = 0;
}

//update global best solution with best solution from pop if better
void update_best_solution(struct solution_struct* pop)
{
     double cur = pop[0].objective;
     int indx = 0;
     for(int i = 1 ;i < POP_SIZE;i++){
        if(cur < pop[i].objective){
            cur = pop[i].objective;
            indx = i;
        }
    }
    if(best_sln.x!=NULL && best_sln.cap_left!=NULL){
        if(best_sln.objective < pop[indx].objective){
            copy_solution(&best_sln,&pop[indx]);
        }
    }else{
        copy_solution(&best_sln,&pop[indx]);
    }
}

int MA(struct problem_struct* prob)
{
    struct solution_struct curt_pop[POP_SIZE];
    struct solution_struct new_pop[POP_SIZE];
    init_population(prob, curt_pop);
    init_population(prob, new_pop);
    int gen=0;
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    while(gen<MAX_NUM_OF_GEN && time_spent < MAX_TIME)
    {
        cross_over(curt_pop, new_pop);
        mutation(new_pop);
        feasibility_repair(new_pop);
        local_search_first_descent(new_pop);
        replacement(curt_pop, new_pop);
        gen++;
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
    }
    update_best_solution(curt_pop);
    printf("%d\n",(int)curt_pop[0].objective);
    free_population(curt_pop, POP_SIZE);
    free_population(new_pop, POP_SIZE);  
    return 0;
}

int main(int argc, char const *argv[])
{
    char prob_set[50];
    char output_set[50];
    for(int i = 1;i<argc-1;i+=2){
        if(strcmp(argv[i],"-o") == 0){
            strcpy(output_set,argv[i+1]);           
        }else
            if(strcmp(argv[i],"-s") == 0){
            strcpy(prob_set,argv[i+1]);
        }else
            if(strcmp(argv[i],"-t") == 0){
            MAX_TIME = atoi(argv[i+1]);
            }
    }
	FILE *fp = fopen(output_set,"w");  
    if(fp == NULL){
        printf("Failed to open the file.");
        return 0;
    }
	struct problem_struct** my_problems = load_problems(prob_set);
    fprintf(fp,"%d\n",PROB_NUM);
    fclose(fp);
    for(int k=0; k<PROB_NUM; k++)
    {
        printf("----%d\n",k);
        for(int run=0; run<NUM_OF_RUNS; run++)
        {
            printf("===%d\n",run);
            srand(time(NULL));
            MA(my_problems[k]); //call MA

        }
        output_solution(&best_sln, output_set);
        free_problem(my_problems[k]); //free problem data memory
    }
    free(my_problems); //free problems array
	return 0;
}