#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<arpa/inet.h> 
#include<sys/socket.h>
#include <stdarg.h>

#define  MAX_SAMPLES 10000
#define MAX_FEATURES 100
#define STRING_BUFFER_LIMIT 100
int PORT_NUMBER = 60000;
int PREPROC_THREAD_LIMIT = 128;
int COEFF_THREAD_LIMIT = 128;


double A[MAX_SAMPLES][MAX_FEATURES];
double b[MAX_SAMPLES];
double beta[MAX_FEATURES-1];

int socket_desc, new_socket, c;
struct sockaddr_in server,client;


typedef struct CSVTable{

    //CSV name
    char name[50];
    //number of rows
    int row;
    //number of columns
    int column;
    //raw CSV data
    char array[MAX_SAMPLES][MAX_FEATURES][STRING_BUFFER_LIMIT];
    //column adlarını tutan array
    char columnName[MAX_FEATURES][STRING_BUFFER_LIMIT];
    //column type tutan array (0-->numeric, 1-->categorical vs.)
    int columnType[MAX_FEATURES];
    //all normalized data X
    double normalized_x[MAX_SAMPLES][MAX_FEATURES-1];
    //bias normalized
    double normalized_x_bias[MAX_SAMPLES][MAX_FEATURES];
    //all normalized data of person
    double normalized_x_person[MAX_FEATURES-1];
    //bias normalized for person
    double normalized_x_person_bias[MAX_FEATURES];
    //raw data of person
    double raw_x_person[MAX_FEATURES-1];

    //all min values for attributes
    double minValue[MAX_FEATURES-1];
    //all max values for attributes
    double maxValue[MAX_FEATURES-1];

    //normalized data TARGET
    double normalized_y[MAX_SAMPLES];
    double MAX_y;
    double MIN_y;
    //after encoding all data are numeric and ready for normalization (last column TARGET's normalized values should be written in normalizedY)
    double numericData[MAX_SAMPLES][MAX_FEATURES];
    //validation
    int valid;
    


} CSVTable;

typedef struct{
    CSVTable *dataset;
    int columnIndex;
    int tIndex;
}ThreadInfo;





int checkCSV();
void selectDataset(char *dataset);
void loadDataSet(CSVTable* dataset);
void parseCsv(CSVTable *dataset);
void categorial_encoding(CSVTable * dataset);
void* categorial_encoding_thread(void *arg);
int findCategoricalNumberOfThreads(CSVTable * dataset);
void convertToNumeric(CSVTable * dataset);
void normalization(CSVTable* dataset);
void* normalization_thread(void* arg);
void normalize_bias(CSVTable* table);
void equation(CSVTable * dataset);
void* equation_thread(void* arg);
void finding_betas(CSVTable* dataset);
void solve_beta_gauss(int p);
void get_data(CSVTable* dataset);
void person_data_normalize(CSVTable* dataset);
void* person_data_normalize_thread(void* arg);
void y_norm(CSVTable* table);
ssize_t read_line(int fd, char *buf, size_t maxlen);
int ask_continue();


int main(int argc, char *argv[]) {

    
    

    printf("Check if CSVs exist in directory...\n");
    if(checkCSV()){
        return 0;
    }
    
    char *message;

    socket_desc = socket(AF_INET,SOCK_STREAM,0);
    if(socket_desc== -1){
        puts("Could not create socket");
        return 1;
    }
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port = htons(PORT_NUMBER);
    int opt = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
        puts("Bind failed");
        return 1;
    }
    puts("Socket is binded");

    listen(socket_desc,3);

    puts("Waiting for incoming connections...\n");

    c = sizeof(struct sockaddr_in);
    while(new_socket = 
        accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)){

        puts("Connection accepted");
            
           
      



   
    while(1){

    char* message="WELCOME TO PRICE PREDICTION SERVER\n\n";
    write(new_socket,message,strlen(message));
    

   
    
    
    char dataset[100];
    selectDataset(dataset);

    CSVTable *table = malloc(sizeof(CSVTable));
    
    strcpy(table->name, dataset);
    table->valid=1;

    

    loadDataSet(table);

    categorial_encoding(table);
    convertToNumeric(table);
    normalization(table);
    normalize_bias(table);
    equation(table);
    get_data(table);
    person_data_normalize(table);
    y_norm(table);
    if (!ask_continue()) {
        const char *bye = "\nConnection closed. Goodbye.\n";
        write(new_socket, bye, strlen(bye));
        close(new_socket);
        break;
    }

    }
}

    
    return 0;   


    
}
//check if all CSVs are in directory
int checkCSV(){
    char name1[12]="";
    char name2[24]="";
    char name3[39]="";
    int boolean=0;

    FILE *f = fopen("Housing.csv", "r");
    if (f == NULL) {
        strcpy(name1, "Housing.csv");
        boolean = 1;
    } else {
        fclose(f);
    }

    f = fopen("Student_Performance.csv","r");
    if(f==NULL){
        strcpy(name2,"Student_Performance.csv");
        boolean=1;

    }
    else{
        fclose(f);
    }

    f = fopen("multiple_linear_regression_dataset.csv","r");
    if(f==NULL){
        strcpy(name3,"multiple_linear_regression_dataset.csv");
        boolean=1;
    }
    else{
        fclose(f);
    }
    if(boolean == 1){
        printf("ERROR: Dataset file(s) %s  %s  %s  not found!\n",name1,name2,name3);
    }

    return boolean;

}
//selection of CSV
void selectDataset(char *dataset){
    char input[100];
    char messageError[100]="Invalid dataset name.\n";
    

    while(1){
        char* message = "Enter CSV file name to load:\n\n";
        write(new_socket, message, strlen(message));
        

        int n = read(new_socket, input, sizeof(input)-1);
         if (n <= 0) return;
         input[n] = '\0';
        //scanf("%s",input);

        //tolowercase eklenebilir

        if(strncasecmp(input, "Housing.csv", strlen("Housing.csv"))==0){
            strcpy(dataset,"Housing.csv");
            break;
        }

        else if(strncasecmp(input, "Student_Performance.csv",
            strlen("Student_Performance.csv"))==0){
            strcpy(dataset,"Student_Performance.csv");
            break;
        }

        else if(strncasecmp(input, "multiple_linear_regression_dataset.csv",
            strlen("multiple_linear_regression_dataset.csv"))==0){
            strcpy(dataset,"multiple_linear_regression_dataset.csv");
            break;
        }

        else{

            write(new_socket, messageError, strlen(messageError));
        }
    }
    snprintf(messageError, sizeof(messageError), "[OK] File %s found\n", dataset);
    write(new_socket, messageError, strlen(messageError));
    //printf("[OK]File %s found\n",dataset);
    

}

void loadDataSet(CSVTable* dataset){
    
    
        parseCsv(dataset);
    
}

void parseCsv(CSVTable* dataset){  


    
    FILE* fp = fopen(dataset->name,"r");
    if(!fp)
    {
        char* message="Error: Can't open the file\n";
        write(new_socket,message,strlen(message));
        return;
    }

    int row=-1;
    int column;
    int columnNo=0;
    char buffer[4096];
    
    while(fgets(buffer,sizeof(buffer),fp) && row < MAX_SAMPLES){
        column=0;
        char* token = strtok(buffer,",\n");
        while(token!=NULL && column < MAX_FEATURES){
            //for attribute array (names of columns) //first row
            if(row==-1){
                strncpy(dataset->columnName[column],token,STRING_BUFFER_LIMIT-1);
                dataset->columnName[column][STRING_BUFFER_LIMIT-1]='\0';
                
            }
            //for raw data array 
            else{
             strncpy(dataset->array[row][column] , token , STRING_BUFFER_LIMIT-1);
             dataset->array[row][column][STRING_BUFFER_LIMIT-1] ='\0';
            }
            //return (\r) FROM WINDOWS
            token = strtok(NULL,",\r\n"); 
            column++;
        }
        
        
        if(row==-1){
            columnNo=column;
            
        }

        row++;


        
        
        
        
        
        
    }
    // memory limits (MAX_SAMPLES, MAX_FEATURES) are exceeded
    if(row>MAX_SAMPLES || columnNo>MAX_FEATURES){
        printf("Memory limits (MAX_SAMPLES, MAX_FEATURES) are exceeded\n");
        exit(0);
    }
    
   
    
    char* endptr;
    //if categorical or numerical
    for(int i =0; i<columnNo;i++ ){
        
        double value = strtod(dataset->array[1][i], &endptr);
        if(*endptr!='\0'){
            dataset->columnType[i]=1;
        }
        else{
            dataset->columnType[i]=0;
        }


    }

    //printing information about columns
    char  messageError[256];
         snprintf(messageError, sizeof(messageError),"Reading file...\n      %d rows loaded.\n      %d columns detected.\n\nColumn analysis: \n",row,columnNo);
          write(new_socket, messageError, strlen(messageError));


          for(int i=0;i <columnNo;i++){

            if(dataset->columnType[i]==0){
                if(i+1==columnNo){
                    char b[256];
                    snprintf(b, sizeof(b),"%s        : numeric(TARGET)\n",dataset->columnName[i]);
                     write(new_socket, b, strlen(b));
                }
                else{
                char a[256];
                snprintf(a, sizeof(a),"%s        : numeric\n",dataset->columnName[i]);
                 write(new_socket, a, strlen(a));
                }
                
            }
            else{
                if(strcmp(dataset->columnName[i], "furnishingstatus") == 0){
                    char a[256];
                     snprintf(a, sizeof(a),"%s        : categorical (furnished, semi-furnished,unfurnished)\n",dataset->columnName[i]);
                    write(new_socket, a, strlen(a));
                }
                else{
                    char a[256];
                     snprintf(a, sizeof(a),"%s        : categorical (yes/no)\n",dataset->columnName[i]);
                    write(new_socket, a, strlen(a));
                }
            }

          }


    
    

    dataset->row=row;
    dataset->column=columnNo;

    fclose(fp);
}

//categorial encoding thread creation
void categorial_encoding(CSVTable * dataset){
    
    char* header= "Starting attribute-level categorical encoding...\n\n";
    write(new_socket,header,strlen(header));


    int threadNumber= findCategoricalNumberOfThreads(dataset);
    if(threadNumber > PREPROC_THREAD_LIMIT){
        exit(0);
    }

    pthread_t tid[threadNumber];
    int tIndex=0;
    for(int i =0;i < dataset->column;i++){
        //if column categorical
        if(dataset->columnType[i]==1){
            //cant send 2 parameters at once so create structure to send it together (to thread function)
            ThreadInfo *args = malloc(sizeof(ThreadInfo));
            args->columnIndex=i;
            args->dataset=dataset;
            args->tIndex=tIndex;
            int err = pthread_create(&(tid[tIndex]), NULL, &categorial_encoding_thread,(void*)args);
            if(err!=0){
                char  messageError[256];
                snprintf(messageError, sizeof(messageError), "Can't create thread :[%s]\n",strerror(err));
                 write(new_socket, messageError, strlen(messageError));
                
            }
            tIndex++;
        }
        

    }
    //MAIN waiting to finish all threads
    for(int i=0;i < threadNumber;i++){

        pthread_join(tid[i],NULL);

    }

    char * message ="[OK] All categorical encoding threads completed.\n";
    write(new_socket,message,strlen(message));
    




}
//critical section code but not requering mutex
void* categorial_encoding_thread(void *arg){
    ThreadInfo *args=(ThreadInfo *) arg;
    int columnIndex=args->columnIndex;
    int rowNo = args->dataset->row;
    int columnNo=args->dataset->column;
    int tIndex=args->tIndex;
    //thread checks if its furnishingstatus encoding because its different from yes/no
    if(strcmp(args->dataset->columnName[columnIndex], "furnishingstatus") == 0){
        
                char  messageError[256];
                
                snprintf(messageError, sizeof(messageError), "Thread [C%d] |  %s (SPECIAL RULE):\nfurnished->2\nsemi-furnished->1\nunfurnished->0\n\n", tIndex+1,args->dataset->columnName[columnIndex]);
                 write(new_socket, messageError, strlen(messageError));
        
            
        
        for(int i=0; i<args->dataset->row; i++){
            
            if(strcmp(args->dataset->array[i][columnIndex], "furnished") == 0){
                args->dataset->numericData[i][columnIndex]=2;
            }
            else if(strcmp(args->dataset->array[i][columnIndex], "semi-furnished") == 0){
                args->dataset->numericData[i][columnIndex]=1;
            }
            else if(strcmp(args->dataset->array[i][columnIndex], "unfurnished") == 0){
                args->dataset->numericData[i][columnIndex]=0;
            }

        }
    }
    //yes-no encoding
    else{
        
        char messageError[150];
        snprintf(messageError, sizeof(messageError), "Thread [C%d] |  %s: (yes/no)->1/0\n", tIndex+1,args->dataset->columnName[columnIndex]);
         write(new_socket, messageError, strlen(messageError));
        
        
        //filling categorical columns for numericData with new encoded values
        for(int i=0;i<args->dataset->row; i++){
            //Yes
            if(strcasecmp(args->dataset->array[i][columnIndex],"yes") ==0){
                args->dataset->numericData[i][columnIndex]=1;
            }
            //No
            else if(strcasecmp(args->dataset->array[i][columnIndex],"no")==0){
                args->dataset->numericData[i][columnIndex]=0;
            }
            //Except yes and no
            else{
                char* message= "Can't accept such input from CSV!";
                write(new_socket,message,strlen(message));
                
                exit(0);
            }
        }
    }   
    


    free(args);
    pthread_exit(NULL);



}
//finding categorical number of threads
int findCategoricalNumberOfThreads(CSVTable * dataset){
    int count=0;
    for(int i =0; i<dataset->column;i++){
        if(dataset->columnType[i]==1){
            count++;
        }
    }
    return count;
}
//raw string data numeric-> double numeric data
void convertToNumeric(CSVTable * dataset){

    for(int i =0; i<dataset->row;i++){
        for(int j=0; j<dataset->column;j++){
            //convert numeric string data to double numeric data
            if(dataset->columnType[j] == 0){
                dataset->numericData[i][j]=atof(dataset->array[i][j]);
            }

        }
    }


}
//normalization main function
void normalization(CSVTable* dataset){

    int numberOfCategorical = findCategoricalNumberOfThreads(dataset);
    int threadNumber=dataset->column - numberOfCategorical;
    if(threadNumber > PREPROC_THREAD_LIMIT){
        exit(0);
    }
    char * message="\n\nStarting numeric normalization threads...\n";
    write(new_socket,message,strlen(message));
    pthread_t tid[threadNumber];
    int tIndex=0;
    for(int i =0;i < dataset->column;i++){
        
            if(dataset->columnType[i]==0){
                //cant send 2 parameters at once so create structure to send it together (to thread function)
                 ThreadInfo *args = malloc(sizeof(ThreadInfo));
                 args->columnIndex=i;
                 args->dataset=dataset;
                 args->tIndex=tIndex;
                 int err = pthread_create(&(tid[tIndex]), NULL, &normalization_thread,(void*)args);
                 if(err!=0){
                        printf("Can't create thread :[%s]\n",strerror(err));
                    }
                  tIndex++;
             }
        
        

    }
    //MAIN waiting to finish all threads
    for(int i=0;i < threadNumber;i++){

        pthread_join(tid[i],NULL);

    }
    //categorical values should not be normalized just move them
    for(int i=0;i<dataset->column-1;i++){
        if(dataset->columnType[i]==1){
            for(int j=0; j<dataset->row;j++){
                dataset->normalized_x[j][i]=dataset->numericData[j][i];
            }
            
        }
    }
    char *message1="[OK] All normalization threads completed.\n";
    char *message2 = "Building normalized feature matrix X_norm..\n";
    char* message3="Building normalized target vector y_norm...\n";
    write(new_socket,message1,strlen(message1));
    write(new_socket,message2,strlen(message2));
    write(new_socket,message3,strlen(message3));
    


}
//normalization thread
void* normalization_thread(void* arg){

    ThreadInfo *args=(ThreadInfo *) arg;
    int columnIndex=args->columnIndex;
    int rowNo = args->dataset->row;
    int columnNo=args->dataset->column;
    int tIndex=args->tIndex;

    double xMin= args->dataset->numericData[0][columnIndex];
    double xMax = args->dataset->numericData[0][columnIndex];
    double yMin = args->dataset->numericData[0][columnIndex];
    double yMax= args->dataset->numericData[0][columnIndex];


        //finding x min and x max
        if((columnIndex+1) != args->dataset->column){
            for(int i=0; i < args->dataset->row; i++){
                if(args->dataset->numericData[i][columnIndex]< xMin){
                    xMin=args->dataset->numericData[i][columnIndex];
                }
                if(args->dataset->numericData[i][columnIndex]> xMax){
                    xMax=args->dataset->numericData[i][columnIndex];
                }
             }
             args->dataset->maxValue[columnIndex]=xMax;
             args->dataset->minValue[columnIndex]=xMin;
             char  messageError[256];
                
            snprintf(messageError, sizeof(messageError), "Thread[%d] normalizing %s... xmin=%.f xmax=%.f\n",tIndex+1, args->dataset->columnName[columnIndex],xMin,xMax);
             write(new_socket, messageError, strlen(messageError));
             

             for(int i=0; i<args->dataset->row;i++){
                //normalization column
                if(xMin!=xMax){
                    args->dataset->normalized_x[i][columnIndex]=(args->dataset->numericData[i][columnIndex]-xMin)/(xMax-xMin);
                }
                //xmin=xmax
                else{
                    args->dataset->normalized_x[i][columnIndex]=0;
                }

             }
             
         }
         //finding ymin and ymax
         else{
            for(int i=0; i < args->dataset->row; i++){
                if(args->dataset->numericData[i][columnIndex]< yMin){
                    yMin=args->dataset->numericData[i][columnIndex];
                }
                if(args->dataset->numericData[i][columnIndex]> yMax){
                    yMax=args->dataset->numericData[i][columnIndex];
                }
             }
             args->dataset->MAX_y=yMax;
             args->dataset->MIN_y=yMin;
             char messageError[256];
             snprintf(messageError, sizeof(messageError), "Thread[%d] normalizing %s(TARGET) ymin=%.f ymax=%.f\n",tIndex+1, args->dataset->columnName[columnIndex],yMin,yMax);
             write(new_socket, messageError, strlen(messageError));
             

             for(int i=0; i<args->dataset->row;i++){
                //normalization column
                if(yMin!=yMax){
                    args->dataset->normalized_y[i]=(args->dataset->numericData[i][columnIndex]-yMin)/(yMax-yMin);
                }
                //ymin=ymax
                else{
                    args->dataset->normalized_y[i]=0;
                }
             }
         }
         free(args);
         pthread_exit(NULL);



}
//first column should be 1.00
void normalize_bias(CSVTable* table){

    for(int i=0;i<table->row;i++){
        table->normalized_x_bias[i][0]=1.0;
    }

    for(int i=0; i < table->row;i++){
        for(int j=0; j < table->column-1;j++){

            table->normalized_x_bias[i][j+1]=table->normalized_x[i][j];
        }
    }
}
//equation function
void equation(CSVTable * dataset){

    int numberOfThreads = dataset->column;
    pthread_t tid[numberOfThreads];
    if(numberOfThreads > COEFF_THREAD_LIMIT){
        exit(0);
    }
    int tIndex=0;
    
    char * s="Spawning coefficient calculation threads...\n";
        write(new_socket,s,strlen(s));

    for(int i=0; i<dataset->column;i++){

        
        ThreadInfo *args = malloc(sizeof(ThreadInfo));
        args->dataset=dataset;
        args->columnIndex=i;
        args->tIndex=tIndex;


        int err = pthread_create(&(tid[tIndex]), NULL, &equation_thread,(void*)args);
        if(err!=0){
            char  messageError[256];
                snprintf(messageError, sizeof(messageError), "Can't create thread :[%s]\n",strerror(err));
                 write(new_socket, messageError, strlen(messageError));;
        }

        tIndex++;
    }

    //joining threads
    for(int i=0; i<numberOfThreads;i++){
        pthread_join(tid[i],NULL);
    }
    char *message1="All coefficient threads joined.\n";
    char * message2="Solving (XᵀX)β = Xᵀy ...\n";
    write(new_socket,message1,strlen(message1));
    write(new_socket,message2,strlen(message2));
    
    //solving gauss elimination
    solve_beta_gauss(numberOfThreads);
    char* message3= "Training completed\n";
    write(new_socket,message3,strlen(message3));
    char* message4="FINAL MODEL (Normalized Form)\n\n";
    write(new_socket,message4,strlen(message4));
    
    char  mE1[150];
    snprintf(mE1, sizeof(mE1), "%s_norm =\n",dataset->columnName[dataset->column-1]);
    write(new_socket, mE1, strlen(mE1));;

    char  mE2[150];
    snprintf(mE2, sizeof(mE2), "%f\n", beta[0]);
    write(new_socket, mE2, strlen(mE2));;
    

    //writing info about betas
    for(int i=1; i<dataset->column;i++){
        char  mE3[150];
        snprintf(mE3, sizeof(mE3), "+ %.4f * %s_norm\n",beta[i],dataset->columnName[i-1]);
        write(new_socket, mE3, strlen(mE3));;
        
       
        
    }


}
void* equation_thread(void* arg){

    ThreadInfo *args=(ThreadInfo *) arg;

    int j = args->columnIndex;
    int tIndex=args->tIndex;
    

    //Create at least one thread per coefficient βj that is responsible for producing the j-th row (A MATRIX)
    for(int k=0; k < args->dataset->column;k++){
        A[j][k]=0.0;
        for(int i=0; i< args->dataset->row;i++){
            A[j][k]+=args->dataset->normalized_x_bias[i][j]*args->dataset->normalized_x_bias[i][k];
        }

    }
    //b MATRIX
    b[j] = 0.0;
    for (int i = 0; i < args->dataset->row; i++) {
        b[j] += args->dataset->normalized_x_bias[i][j]
              * args->dataset->normalized_y[i];
    }

    if(j==args->dataset->column-1){
        char  messageError[256];
                
            snprintf(messageError, sizeof(messageError), "[β-Thread %d] Calculating β%d (bias))\n",tIndex+1,tIndex);
             write(new_socket, messageError, strlen(messageError));
        
    }
    else{
        char  messageError[256];
                
            snprintf(messageError, sizeof(messageError), "[β-Thread %d] Calculating β%d (%s)\n",tIndex+1,tIndex,args->dataset->columnName[j]);
             write(new_socket, messageError, strlen(messageError));
        
    }





    free(args);
    pthread_exit(NULL);
}
void solve_beta_gauss(int p) {
    // İleri eliminasyon: A'yı üst üçgensel hale getir
    for (int k = 0; k < p ; k++) {

        // pivot
        if (fabs(A[k][k]) < 1e-12) {
            int pivot = -1;
            for (int r = k + 1; r < p; r++) {
                if (fabs(A[r][k]) > 1e-12) {
                    pivot = r;
                    break;
                }
            }
            if (pivot == -1) {
                printf("ERROR: Singular matrix (no valid pivot at column %d)\n", k);
                return;
            }
            // A'da satır takası
            for (int c = 0; c < p; c++) {
                double tmp = A[k][c];
                A[k][c] = A[pivot][c];
                A[pivot][c] = tmp;
            }
            // b'de satır takası
            double tmpb = b[k];
            b[k] = b[pivot];
            b[pivot] = tmpb;
        }

        // k altındaki satırları temizle
        for (int i = k + 1; i < p; i++) {
            double factor = A[i][k] / A[k][k];

            // A satırını güncelle
            for (int j = k; j < p; j++) {
                A[i][j] -= factor * A[k][j];
            }

            // b'yi güncelle
            b[i] -= factor * b[k];
        }
    }

    // (back substitution)
    for (int i = p - 1; i >= 0; i--) {
        double sum = b[i];
        for (int j = i + 1; j < p; j++) {
            sum -= A[i][j] * beta[j];
        }
        if (fabs(A[i][i]) < 1e-12) {
            printf("ERROR: Zero diagonal at row %d\n", i);
            return;
        }
        beta[i] = sum / A[i][i];
    }
}

void get_data(CSVTable* dataset){

    char *header = "\nEnter new instance for prediction:\n\n";
    write(new_socket, header, strlen(header));
    char input[STRING_BUFFER_LIMIT];

    for(int i=0; i<dataset->column-1;i++){

        //numeric
        if(dataset->columnType[i]==0){
            
            char msg[256];
            snprintf(msg,sizeof(msg),"%s (xmin=%.f xmax=%.f):\n\n", dataset->columnName[i],dataset->minValue[i], dataset->maxValue[i]);
            write(new_socket,msg,strlen(msg));
            
            int n = read_line(new_socket, input, sizeof(input)-1);
            if(n <= 0){ 
                char* messageE= "Incorrect input!\n";
                write(new_socket,messageE,strlen(messageE));
                exit(0);
            }
            input[n] = '\0';
            

            // string -> double parse 
            char *endptr;
            double val = strtod(input, &endptr);


            // (strtod hiç parse edemediyse endptr == input olur)
            if(endptr == input){
                const char *err = "Invalid numeric value. Please enter a number.\n";
                write(new_socket, err, strlen(err));
                exit(0);
            }

            dataset->raw_x_person[i] = val;

        }
        //categorical
        else if(dataset->columnType[i]==1){

                
            
            //if furnishingstatus corresponding output
            if(strcmp(dataset->columnName[i], "furnishingstatus") == 0){

                    char msg[256];
                    snprintf(msg,sizeof(msg),"%s (furnished, semi-furnished, unfurnished):\n\n",dataset->columnName[i]);
                    write(new_socket,msg,strlen(msg));
                    char tmp[STRING_BUFFER_LIMIT];
                    int n = read_line(new_socket, input, sizeof(input)-1);
                    if(n <= 0){ 
                        char* messageE= "Incorrect input!\n";
                        write(new_socket,messageE,strlen(messageE));
                        exit(0);
                    }
                    input[n] = '\0';
                    
                    
                    
                    
                    if(strncasecmp(input, "furnished", strlen("furnished")) == 0){
                        dataset->normalized_x_person[i]=2;
                    }
                    else if(strncasecmp(input, "semi-furnished", strlen("semi-furnished")) == 0){
                        dataset->normalized_x_person[i]=1;
                    }
                    else if(strncasecmp(input, "unfurnished", strlen("unfurnished")) == 0){
                        dataset->normalized_x_person[i]=0;
                    }
                    else{
                        char *err = "Invalid input. Please type: furnished / semi-furnished / unfurnished\n";
                        write(new_socket, err, strlen(err));
                    }
        
                
            }
            else{
                
                char msg[256];
                snprintf(msg,sizeof(msg),"%s (yes/no):\n\n",dataset->columnName[i]);
                write(new_socket,msg,strlen(msg));

                int n = read_line(new_socket, input, sizeof(input)-1);
                if(n <= 0){ 
                    char* messageE= "Incorrect input!\n";
                    write(new_socket,messageE,strlen(messageE));
                    exit(0);
                }
                input[n] = '\0';
                
                

                if(strncasecmp(input, "yes", strlen("yes")) == 0){
                    dataset->normalized_x_person[i]=1;
                    
                }
                else if(strncasecmp(input, "no", strlen("no")) == 0){
                    dataset->normalized_x_person[i]=0;
                }
                else{
                    const char *err = "Invalid input. Please enter yes or no.\n";
                    write(new_socket, err, strlen(err));
                    exit(0);
                }

            }



        }
    }



}
//for new socket writing 
ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    while (i + 1 < maxlen) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) return n;          // 0: closed, -1: error
        if (c == '\r') continue;       // CRLF toleransı
        if (c == '\n') break;          // satır bitti
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

//normalization person's data
void person_data_normalize(CSVTable* dataset){


    int numberOfCategorical = findCategoricalNumberOfThreads(dataset);
    int threadNumber=dataset->column - numberOfCategorical;
    pthread_t tid[threadNumber];
    int tIndex=0;
    for(int i =0;i < dataset->column;i++){
        
            if(dataset->columnType[i]==0){
                //cant send 2 parameters at once so create structure to send it together (to thread function)
                 ThreadInfo *args = malloc(sizeof(ThreadInfo));
                 args->columnIndex=i;
                 args->dataset=dataset;
                 args->tIndex=tIndex;
                 int err = pthread_create(&(tid[tIndex]), NULL, &person_data_normalize_thread,(void*)args);
                 if(err!=0){
                     char msg[256];
                        snprintf(msg,sizeof(msg),"Can't create thread :[%s]\n",strerror(err));
                        write(new_socket,msg,strlen(msg));
                        
                    }
                  tIndex++;
             }
        
        

    }
    //MAIN waiting to finish all threads
    for(int i=0;i < threadNumber;i++){

        pthread_join(tid[i],NULL);

    }

    //biasing
    
    dataset->normalized_x_person_bias[0]=1.0;
    for(int i=0; i < dataset->column-1;i++){
        

        dataset->normalized_x_person_bias[i+1]=dataset->normalized_x_person[i];
        
    }

    for(int i =0; i<dataset->column-1;i++){
        
        char msg[256];
        snprintf(msg,sizeof(msg),"%s_norm   =  %.4f\n",dataset->columnName[i], dataset->normalized_x_person[i]);
       write(new_socket,msg,strlen(msg));
      

    }
   

    


}
//normalization person's data thread
void* person_data_normalize_thread(void* arg){

    ThreadInfo *args=(ThreadInfo *) arg;
    int index = args->columnIndex;

    int threadIndex=args->tIndex;

    
    double min= args->dataset->minValue[index];
    double max = args->dataset-> maxValue[index];

    if(index!=args->dataset->column-1){

     if(min!=max){
        args->dataset->normalized_x_person[index] = (args->dataset->raw_x_person[index] - min) / (max - min);
     }
     //min=max
     else{
        args->dataset->normalized_x_person[index]=0.0;
        }
        
        char msg[256];
         snprintf(msg,sizeof(msg),"[Thread N%d] Normalizing %s...\n", threadIndex+1,args->dataset->columnName[index]);
        write(new_socket,msg,strlen(msg));
      
    }
    else{
        char msg[256];
         snprintf(msg,sizeof(msg),"[Thread N%d] Normalizing %s(TARGET)... ymin=%.f ymax=%.f\n", threadIndex+1,args->dataset->columnName[index],args->dataset->MIN_y,args->dataset->MAX_y);
        write(new_socket,msg,strlen(msg));
        
    }

    
    free(args);
    pthread_exit(NULL);

}
//final outputs
void y_norm(CSVTable* table){

    double yNorm=0.0;
    for(int i = 0; i<table->column;i++){
        yNorm+=beta[i]* table->normalized_x_person_bias[i];
    }

    char msg[256];
    snprintf(msg,sizeof(msg),"Predicted Normalized Price : %f\n\n", yNorm );
    write(new_socket,msg,strlen(msg));
    char* header="Reverse-normalizing target...\n\n";
    write(new_socket,header,strlen(header));

    double yMin = table->MIN_y;
    double yMax = table->MAX_y;

    double y_reverse=yNorm*(yMax-yMin)+yMin;
    char msg1[256];
    snprintf(msg1,sizeof(msg1),"price= %.4f * (%.f - %.f) + %.f\n",yNorm,yMax,yMin,yMin);
    write(new_socket,msg1,strlen(msg1));

    char msg2[256];
    snprintf(msg2,sizeof(msg2),"price ≈ %f\n\n",y_reverse);
    write(new_socket,msg2,strlen(msg2));

    char* header1="PRICE PREDICTION RESULTS: \n\n";
    write(new_socket,header1,strlen(header1));
    char msg3[256];
    snprintf(msg3,sizeof(msg3),"Normalized prediction :  %f\n",yNorm);
    write(new_socket,msg3,strlen(msg3));
    char msg4[256];
    snprintf(msg4,sizeof(msg4),"Real-scale prediction :  %f TL",y_reverse);
    write(new_socket,msg4,strlen(msg4));
    printf("Real-scale prediction : %f\n", y_reverse);

}
//ask if want to continue
int ask_continue() {
    char buf[16];

    const char *q = "\nDo you want to continue? (y/n): ";
    write(new_socket, q, strlen(q));

    int n = read_line(new_socket, buf, sizeof(buf)-1);
    if (n <= 0) return 0;

    buf[n] = '\0';

    if (buf[0] == 'y' || buf[0] == 'Y')
        return 1;

    if (buf[0] == 'n' || buf[0] == 'N')
        return 0;

    const char *err = "Invalid input. Please type only y or n.\n";
    write(new_socket, err, strlen(err));
    exit(0);
}


