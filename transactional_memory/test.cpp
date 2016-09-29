
#include "tm_core_link.h"
#include "tm_module_link.h"
#include "multithreadedtransactionmanager.h"
#include "unistd.h"

int main(){

  double a=0.2;
 
  double b=0.1;
 
  initTransactionManager(1);
  scheduleReplaceTransaction(&a,&b);
  complex c(5,6);
  complex d(4,4);
  int tr1[10];
  int tr2[5];
  tr1[0]=10;
  bzero(&tr2,5*sizeof(int));
  scheduleReplaceTransaction(&c,&d);
  scheduleReplaceTransaction(tr1,tr2,5);
  commitTransactions();
  printf("All sana %lf %lf\n",a,c.Im());
  for(int i=0;i<5;i++){
  
    printf("%d\n",tr1[i]);
  }
  MultiThreadedTranscationManager *m=new MultiThreadedTranscationManager(2);
  sleep(2);
  printf("created!!!\n");
  tr1[0]=10;
  ReplaceTransaction<int> *r=new ReplaceTransaction<int>(tr2,tr1,5);
  printf("scheduing a transaction!");
  m->scheduleTranscation(r);
  m->commit();
  printf("%d\n",tr2[0]);
  
  delete m;
  
  return 0;
}