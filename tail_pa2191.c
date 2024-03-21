#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char buff [2048];

int writeto_file(int no){
    
    int countlines;
    for(int i=0;i<=no;i++){
        if(buff[i]!='\0'){

            
            if(buff[i]!='\n' && buff[i]!='\t' && buff[i]!='\v' && buff[i]!='\r'){
                continue;
            }
            else{
                countlines=countlines+1;
            }
        }
      
    }
    return countlines;

}

void check(int no){
      switch(no){
          default:
            if(no <0){
                printf(1, "Error unable to read file");
                exit();
            }                      
      }
  }


void reading_file(int no, int z, char *buff, int c, int line, int standby, int nooflines){
    
    int e = 1;
    do {
      for(int i=0;i<no;i++){
          
          if(c>=(nooflines)){

            if(buff[i]=='\n'){
                
                if(e == 1){
                    continue; 
                }
                e =1;
                    
             }
            else{
                e = 0;
            }
            printf(1,"%c",buff[i]);    
       
          }
          if(buff[i]=='\n'){
              
              c++;
          }
          
        }
  
  }while ((no = read(standby, buff, sizeof(buff))) > 0);
}

void tail (int fd, char *name, int line) {
  
  int nooflines;
  int no;  
  
  int c = 0;
  int z;

  int standby = open ("standby_file", O_CREATE | O_RDWR);     

  while((no = read(fd, buff, sizeof(buff))) > 0 )
  {
    write (standby, buff, no);                                 
  
    z = writeto_file(no);
   
  }
 check(no);

  if (z < line){
      nooflines =0;
  }

  else{
      nooflines = z - line;
  }


  close (standby);

  standby = open ("standby_file", 0);                       

  reading_file(no,z,buff,c, line,standby, nooflines);
   
  close (standby);
  
  unlink("standby_file");                  
}



int main(int argc, char *argv[]) {
  
  int fd = 0;	
  int default1 = 10;	
  char *name_of_file;	
  char x;

  name_of_file = ""; 
     
  if (argc <= 1) 
  {	
    tail(fd, "", default1);	
    exit();
  }

  else {
      int i =1;
      
      while (i<argc) {
          x = *argv[i];

          if(x=='-'){
              argv[i]++;
              default1 = atoi(argv[i]++);
              
          }
          else{
              
              if((fd = open(argv[i],0))<0){
                  printf(1,"Error unable to open file \n ");
                  exit();
              }
          }
          i = i+1;
        }
        tail(fd,name_of_file,default1);
        close(fd);
        exit();
  }
}



