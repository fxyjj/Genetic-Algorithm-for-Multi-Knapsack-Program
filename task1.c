#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>


int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 5;
static int POP_SIZE = 100; //global parameters
int MAX_NUM_OF_GEN = 100; //max number of generations
int MAX_TIME = 60;  //max amount of time permited (in sec)
float CROSSOVER_RATE = 0.8;
float MUTATION_RATE = 0.2;
int PROB_NUM = 0;

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

struct problem_struct** load_problems(const char* arg)
{
	FILE *fp = fopen(arg,"r");
	if(fp == NULL){
		printf("no such file exist.\n");
		return NULL;
	}
    int i,j,k;
    //int num_of_probs;
    fscanf(fp,"%d",&PROB_NUM);
     //printf("%d\n",num_of_probs);
    if(PROB_NUM == 0) return NULL;
    
    struct problem_struct** my_problems = malloc(sizeof(struct problem_struct*)*PROB_NUM);
    for(k=0; k<PROB_NUM; k++)
    {
        int n, dim,bst_sln;
    	fscanf(fp,"%d",&n);
    	fscanf(fp,"%d",&dim);
    	fscanf(fp,"%d",&bst_sln);
        best_sln.objective = bst_sln;
    	 //printf("%d %d %d\n",n,dim,bst_sln); 
        init_problem(n, dim, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].dim = dim;
            fscanf(fp,"%d",&my_problems[k]->items[j].p);
            // printf("+++%d %d ", my_problems[k]->items[j].p,my_problems[k]->items[j].dim);
           
        }
         //printf("\n");
        for(i=0; i<dim; i++){
        	for(j = 0;j < n;j++){
        		fscanf(fp,"%d",&my_problems[k]->items[j].size[i]);
        		 //printf("---%d ", my_problems[k]->items[j].size[i]);
           
        	}
        }
        for(i = 0;i < dim;i++){

        	fscanf(fp,"%d",&my_problems[k]->capacities[i]); 
        	  //printf("%d\n",my_problems[k]->capacities[i]); 

        }
    }
    fclose(fp);
    return my_problems;
}

//for test
void getChromesome(struct solution_struct* solve){
    int num = solve->prob->n;
    printf("chromesome infor:");
    for(int i = 0;i<num;i++){
        printf("%d ",solve->x[i]);
    }
    printf("items size infor:\n");
    for(int i = 0;i<num;i++){
        printf("items%d:----\n",i);
        for(int a = 0;a<solve->prob->dim;a++){
             printf("%d ",solve->prob->items[i].size[a]);
        }
        printf("%d ",solve->prob->items[i].p);
        printf("\n");
       
    }
    
    printf("objective infor:");
    
        printf("%f ",solve->objective);
    
    printf("\n");
}

//generate a new population
void cross_over(struct solution_struct* curt_pop, struct solution_struct* new_pop)
{
	// struct solution_struct* copy_cpop = curt_pop;
  	for(int i = 0 ;i < POP_SIZE;i++){
       int w = rand()%(100-i)+i;
        //printf("|%d|\n",w)
  		struct solution_struct t = new_pop[i];
        //printf("1\n");
  		new_pop[i] = new_pop[w];
         //printf("2\n");
  		new_pop[w] = t;
         //printf("3\n");
  	}
    int domain = 0;
    if(POP_SIZE%2 == 0){
        domain = POP_SIZE;
    }else{
        domain = POP_SIZE-1;
    }
  	for(int j = 0; j<domain-1;j+=2){
         // printf("%d %d Before cross over:\n ",j,j+1);
         // getChromesome(&new_pop[j]);
         // getChromesome(&new_pop[j+1]);
  		if(rand_int(0,9) < 8){
  			int xpt = rand_int(1,curt_pop[0].prob->n-1);
            //printf("change point: %d\n",xpt);
  			for(;xpt<curt_pop[0].prob->n;xpt++){
  				int indx = new_pop[j].x[xpt];
  				new_pop[j].x[xpt] = new_pop[j+1].x[xpt];
  				new_pop[j+1].x[xpt] = indx;
  			}
  			evaluate_solution(&new_pop[j]);
            evaluate_solution(&new_pop[j+1]);
  		}
        // printf("%d %d After cross over:\n ",j,j+1);
        //  getChromesome(&new_pop[j]);
        //  getChromesome(&new_pop[j+1]);
        //  printf("---------------------------\n");
  	}	
}

//apply mutation to a population
void mutation(struct solution_struct* pop)
{
    for(int i = 0;i<POP_SIZE;i++){
        // printf("%d  Before mutation:\n ",i);
        //  getChromesome(&pop[i]);
    	for(int j = 0;j<pop[i].prob->n;j++){
    		if(rand_int(0,9) == 0){
                 //printf("change point: %d\n",j);
    			if(pop[i].x[j] == 0){
    				pop[i].x[j] = 1;
    			}else{
    				pop[i].x[j] = 0;
    			}
    		}
    		evaluate_solution(&pop[i]);

    	}
         // printf("%d After mutation: %d\n ",i,pop[i].feasibility);
         // getChromesome(&pop[i]);
    }
}

//modify the solutions that violate the capacity constraints //TODO remove the items with smallest price
void feasibility_repair(struct solution_struct* pop)
{
    int num = pop[0].prob->n;
    for(int i = 0;i<POP_SIZE;i++){
        if(pop[i].feasibility < 0){
              // printf("%d Before repair: \n ",i);
              // getChromesome(&pop[i]);
              
            for(int j = 0;j<num;j++){
                
                if(pop[i].x[j] == 1){
                    pop[i].x[j] = 0;
                    
                    evaluate_solution(&pop[i]);
                    if(pop[i].feasibility > 0){
                        
                        // printf("%lf\n",pop[i].objective);
                        break;
                    }
                }
            }
            
             // printf("%d After repair: \n ",i);
             //  getChromesome(&pop[i]);
        }
    }
}

//local search
void local_search_first_descent(struct solution_struct* pop)
{
    int num = pop[0].prob->n;
    int dimen = pop[0].prob->dim;
   for(int i = 0;i < POP_SIZE;i++){
             // printf("%d Before search: objective val %lf\n ",i,pop[i].objective);
             // getChromesome(&pop[i]);
             // double com = pop[i].objective;
        int p0 = 0;
        int p1 = 0;
        int indx0 = -1;
        int indx1 = -1;
        int arr_v0[dimen];
        int arr_v1[dimen];
        for(int j = 0;j<num;j++){

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
                        if(rand_int(0,9) < 5) indx1 = -1;
                        j = indx0;
                        indx0 = -1;
                    }
                    else
                    {
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
        // printf("%d After search: objective val %lf\n ",i,pop[i].objective);
        //      getChromesome(&pop[i]);
        //      printf("\nResult:%lf - %lf = %lf++++++++++++++++++\n\n",pop[i].objective,com,(-com+pop[i].objective));
   }
}

//replacement
void replacement(struct solution_struct* curt_pop, struct solution_struct* new_pop)
{
    struct solution_struct big[2*POP_SIZE];
    int a = 0;
    int b = 0;
    //  printf("Before replacement---------\ncurt_pop infor:\n");
    // for(int x = 0 ;x < POP_SIZE;x++){
    //     printf("%d, -%lf-\n",x,curt_pop[x].objective);
    // }
    //  printf("Before replacement---------\nnew_pop infor:\n");
    // for(int x = 0 ;x < POP_SIZE;x++){
    //     printf("%d, -%lf-\n",x,new_pop[x].objective);
    // }
    for(int i = 0;i<2*POP_SIZE-1;i+=2){
        big[i] = curt_pop[a++];
        big[i+1] = new_pop[b++];
    } 
    int check = 0;
    while(check == 0){
        check = 1;
        for(int i = 0;i<2*POP_SIZE-1;i++){
            if(big[i].objective < big[i+1].objective){
                //printf("before: %lf,%lf\n",big[i].objective,big[i+1].objective);
                struct solution_struct t = big[i];
                big[i] = big[i+1]; 
                big[i+1] = t;
                check = 0;
                //printf("after: %lf,%lf\n",curt_pop[i].objective,curt_pop[i+1].objective);
            }
        }
    }
    
    // printf("Before replacement---------\nbig infor:\n");
    // for(int x = 0 ;x < 2*POP_SIZE;x++){
    //     printf("%d, -%lf-\n",x,big[x].objective);
    // }
    
    
    
    for(int j = 0 ;j < POP_SIZE;j++){
            curt_pop[j] = big[j];
            new_pop[j] = big[j];
    }
    // printf("After replacement: \n");
    //  for(int x = 0 ;x < POP_SIZE;x++){
    //     printf("%d, curt_pop %lf |-----| new_pop %lf\n",x,curt_pop[x].objective,new_pop[x].objective);
    //     getChromesome(&curt_pop[x]);
    // }
    
    
}

void output_solution(struct solution_struct* sln, const char* out_file)
{
    FILE *fp = fopen(out_file,"a");
    if(fp == NULL){
        printf("Failed to open the file.");
        return;
    }else{
        fprintf(fp,"%f\n",sln->objective);
        for(int i = 0;i<sln->prob->n;i++){
            fprintf(fp,"%d ",sln->x[i]);
        }
        fprintf(fp,"\n");
    }
    //fclose(fp);

    printf("sln.feas=%d, sln.obj=%f\n", sln->feasibility, sln->objective);
    struct solution_struct a;
    best_sln = a;
}

//update global best solution with best solution from pop if better
void update_best_solution(struct solution_struct* pop)
{
    double cur = pop[0].objective;
    int indx = 0;
   
     //if(best_sln.x==NULL && best_sln.cap_left==NULL)  best_sln = pop[0];
    //     if(!copy_solution(&best_sln,&pop[0])){
    //         printf("copy failed.\n");
    //     }
    
    
    for(int i = 1 ;i < POP_SIZE;i++){
        //getChromesome(&pop[i]);
        if(cur < pop[i].objective){
            cur = pop[i].objective;
            indx = i;
        }
    }
    if(best_sln.x!=NULL && best_sln.cap_left!=NULL){
        if(best_sln.objective < pop[indx].objective){
        best_sln = pop[indx];
        }
    }else{
        best_sln = pop[indx];
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
    //getChromesome(&best_sln);
    
     // free_population(curt_pop, POP_SIZE);
     // free_population(new_pop, POP_SIZE);
    
    return 0;
}

int main(int argc, char const *argv[])
{
	

    // struct problem_struct** my_problems = load_problems(argv[1]);
    // struct solution_struct curt_pop[POP_SIZE];
    // struct solution_struct new_pop[POP_SIZE];
    // init_population(my_problems[0], curt_pop);
    // init_population(my_problems[0], new_pop);
    // cross_over(curt_pop, new_pop);
    // mutation(new_pop);
    // feasibility_repair(new_pop);
    // local_search_first_descent(new_pop);
    // replacement(curt_pop, new_pop);
    // update_best_solution(curt_pop);
    // getChromesome(&best_sln);
	 FILE *fp = fopen("best_sln.txt","w+");
    if(fp == NULL){
        printf("Failed to open the file.");
        return 0;
    }

	struct problem_struct** my_problems = load_problems(argv[1]);
     fprintf(fp,"%d\n",PROB_NUM);
     //fclose(fp);
    for(int k=0; k<PROB_NUM; k++)
    {
       
        printf("----%d\n",k);
        for(int run=0; run<NUM_OF_RUNS; run++)
        {
            printf("%d\n",run);
            srand(RAND_SEED[run]);
            MA(my_problems[k]); //call MA
              //getChromesome(&best_sln);
            

        }
        output_solution(&best_sln, "best_sln.txt");
        // if(best_sln.x!=NULL && best_sln.cap_left!=NULL){ free(best_sln.cap_left); free(best_sln.x);} //free global

         free_problem(my_problems[k]); //free problem data memory
    }
    
     fclose(fp);
    // //printf("%lf\n",best_sln.objective);
     // free(my_problems); //free problems array
     // if(best_sln.x!=NULL && best_sln.cap_left!=NULL){ free(best_sln.cap_left); free(best_sln.x);} //free global

	return 0;
}