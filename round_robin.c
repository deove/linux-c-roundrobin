#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>

#define nbMAX 21 //nombre max de processus
int q;//quantum
int c;//numbre de quantum total
int elu;//processus elu
int filetotal[12][20];//file de priorite qui stock les processus par ses priorites
int quantum_suivant[20];//les processus qui commencent dans le quantum suivant
int nbprocessus;//le numbre de processus qui sont block pendant ce quantum

struct info_processus
{
    int priorite;
    int date; //date de soumission
    int temp; //temp d'execution
    pthread_cond_t c;
    pthread_mutex_t m;//pour proteger le cond
}info[nbMAX]={0,0,0,PTHREAD_COND_INITIALIZER,PTHREAD_MUTEX_INITIALIZER};//numero unique pour chaque processus est decide par l'ordre d'entrer
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;//pour proteger les variables globals



//module 1:modifier les pourcentage pour chaque priorite
float * modifier_pourcentage()
{
    char choix;
    int j;
    float k;
    k=0;
    static float pourcentage[12];
    printf("modifier le pourcentage de chaque priorite V/F\n");
    scanf("%s",&choix);
    if(choix=='V')
    {
       printf("pourcentage de priorite0: ");
       scanf("%f",&pourcentage[0]);
       for(j=1;j<11;j++)
       {
           printf("pourcentage de priorite%d: ",j);
           scanf("%f",&pourcentage[j]);
           k=k+pourcentage[j];
           //printf("k=%f\n",k);
           if( pourcentage[j]>pourcentage[j-1] ) // plus haut priorite -> plus grande pourcentage
           {
              printf("erreur sur le pourcentage1\n");
              exit(-1);
           }
       }
       if(fabs(k+pourcentage[0]-1.0)>=1e-6)//le sum == 1
       {
          printf("erreur sur le pourcentage2\n");
          exit(-1);
       }
    }
    else if(choix=='F')//le pourcentage par defaut
    {
            pourcentage[0]= 0.15; //0 est priorite 0
            pourcentage[1]= 0.13;
            pourcentage[2]= 0.12;
            pourcentage[3]= 0.11;
            pourcentage[4]= 0.10;
            pourcentage[5]= 0.09;
            pourcentage[6]= 0.08;
            pourcentage[7]= 0.07;
            pourcentage[8]= 0.06;
            pourcentage[9]= 0.05;
            pourcentage[10]= 0.04;
    }
    return pourcentage;
}



//module 2:entrer les informations des processus
int information()
{
     int n,i,pr,da,te,y;
     char choix;
     printf("entrer les informations des processus ou les generer aleatoirement M/G\n");
     scanf("%s",&choix);
     if(choix=='M')
     {
        printf("entrer le numbre de processus (1 - 20): ");
        scanf("%d",&n);
        if(n>20 || n<1)
           exit(0);
        for(i=0;i<n;i++)
        {
            printf("entrer le priorite de processus%d (0 - 10): ",i);
            scanf("%d",&pr);
            if(pr>=0 && pr<=10)
               info[i].priorite=pr;
            else
            {
               printf("erreur sur priorite\n");
               exit(-1);
            }

            printf("entrer le date de processus%d: ",i);
            scanf("%d",&da);
            info[i].date=da;

            printf("entrer le temp de processus%d: ",i);
            scanf("%d",&te);
            info[i].temp=te;
        }
        printf("choisir le quantum: ");
        scanf("%d",&q);
        if(q==0)
        {
           printf("erreur sur q");
           exit(-1);
        }
     }
     else if(choix=='G')
     {
        srand(getpid());
        n=rand()%20+1;//selon le taille de tableau 1-20
        printf("n=%d\n",n);
        for(i=0;i<n;i++)
        {
            srand(i);
            info[i].priorite=rand()%11;//priorite 0-10
            printf("processus%d:priorite %d\n",i,info[i].priorite);
            srand(i+1);
            info[i].date=rand()%11;
            printf("processus%d:date %d\n",i,info[i].date);
            srand(i+2);
            info[i].temp=rand()%11+1;//plus que 0
            printf("processus%d:temp %d\n",i,info[i].temp);
        }
        srand(getpid()+23);
        q=rand()%10+1;//plus que 0
        printf("quantum %d\n",q);
     }
     for(i=0;i<n;i++)
     {
         y=info[i].temp/q;
         if(y*q<info[i].temp)
            y=y+1;
         c=c+y;//le nombre de quantum total est le sum de temp d'execution/quantum(arrondis)
     }
     return n;
}



//module 3: table d'allocation
int * table_allocation(int c,float a[])
{
   int i,j,k,l,origin,count;
   count=0;
   float m;
   static int b[12];
   static int d[100];
   k=0;

   for(i=0;i<11;i++)
   {
       m=(float)(a[i]*c);//a[i] est le pourcentage, c est le nombre de quantum
       origin=a[i]*c;//origin est integer
       if(fabs(m-origin)>1e-6)//par exemple 2.1 compare avec 2
          b[i]=origin+1;//le numbre de fois d'apparaitre pour chaque priorite
   }
   j=c/b[0];//l'intervalle du plus haut priorite, 0xxxxx0, intervalle est 5
   if(j*b[0]<c)
      j=j+1;
   for(i=0;i<c;i++)
   {
      if(k<j)//dans l'intervalle
      {
        if(b[k]>0)
        {
          d[i]=k;//table d'allocation,i represente l'ordre de quantum
          b[k]=b[k]-1;//le fois d'apparaitre
          k=k+1;
        }
        else
        {
          while(b[k]==0)
          {
              k=k+1;
              j=j+1;
              count=count+1;
              if(count==11)
                 break;
          }
          if(count==11)
          {
             printf("les autres pourcentages sont trop petit, ils n'existent pas\n");
             break;
          }
          //printf("k=%d\n",k);
          d[i]=k;
          b[k]=b[k]-1;
          k=k+1;
        }
      }
      else
      {
          k=0;
          d[i]=k;
          b[k]=b[k]-1;
          k=k+1;
      }
   }
   for(i=0;i<c;i++)
   {
       printf("d[%d]=%d\n",i,d[i]);
   }
   return d;
}



//module 4:execution de chaque thread
void execution(int threadid)
{
     int temps,i,j;
     printf("le temp avant execution est %d\n",info[threadid].temp);
     info[threadid].temp=info[threadid].temp-q;
     if(info[threadid].temp<=0)
        printf("le temp apres execution est 0\n");
     else
        printf("le temp apres execution est %d\n",info[threadid].temp);
     printf("le priorite avant execution est %d\n",info[threadid].priorite);
     temps=filetotal[info[threadid].priorite][0];//le processus execute
     for(j=0;j<20;j++)
     {
        filetotal[info[threadid].priorite][j]=filetotal[info[threadid].priorite][j+1];
     }
     if(info[threadid].priorite!=10)
     {
        info[threadid].priorite=info[threadid].priorite+1;
     }
     else
     {
        info[threadid].priorite=1;
     }
     printf("le priorite apres execution est %d\n",info[threadid].priorite);
     if(info[threadid].temp>0)
     {
        for(j=0;j<20;j++)
        {
            if(filetotal[info[threadid].priorite][j]==0)
            {
               filetotal[info[threadid].priorite][j]=temps;
               break;
            }
        }
     }
}

void *creation(int threadid)
{
     int i;
     printf("thread%d commence\n",threadid);
     for(i=0;i<=c;i++)
     {
         pthread_mutex_lock(&info[threadid].m);
         printf("thread%d dormir\n",threadid);
         pthread_cond_wait(&info[threadid].c,&info[threadid].m);//processus doivent attendre pour le signal
         pthread_mutex_unlock(&info[threadid].m);

         printf("thread%d reveille\n",threadid);
         pthread_mutex_lock(&mutex);
         execution(threadid);
         if(info[threadid].temp<=0)
         {
            printf("thread%d fini\n",threadid);
            pthread_mutex_unlock(&mutex);
            break;
         }
         pthread_mutex_unlock(&mutex);
     }
     pthread_exit(NULL);
}



//module 5:jury pour choisir le processus elu
void *jury(int pri)//pri est le priorite decide par table d'allocation
{
  int count,count2,i;
  count=0;
  count2=0;
  pthread_mutex_lock(&mutex);
  printf("choisir le processus elu dans ce quantum\n");
  while(1)
  {
       if(filetotal[pri][0]==0)
       {
          if(pri==10)
             pri=1;
          else
             pri=pri+1;
          count++;
          if(count==11 && filetotal[0][0]!=0)
             pri=0;
          else if(count==11 && filetotal[0][0]==0)
             break;
       }
       else
       {
          for(i=0;i<nbprocessus;i++)//les processus de quantum suivant ne peuvent pas etre choisi
          {
              if(quantum_suivant[i]==filetotal[pri][0])
              {
                 count2++;
                 break;
              }
          }
          if(count2==0)
             break;
          else
          {
             if(pri==10)//pri est la priorite elu par le table d'allocation
                pri=1;
             else
                pri=pri+1;
          }
          count2=0;
       }
  }
  for(i=0;i<nbprocessus;i++)//pour la situation, rien de processus commence dans ce quantum
  {
      quantum_suivant[i]=0;
  }
  nbprocessus=0;//les valeurs suivants peuvent couvrir les valeurs precedents
  printf("priorite elu=%d ",pri);
  printf("processus elu=%d\n",filetotal[pri][0]-1);
  elu=filetotal[pri][0]-1;
  pthread_mutex_lock(&info[elu].m);
  pthread_cond_signal(&info[elu].c);
  pthread_mutex_unlock(&info[elu].m);
  pthread_mutex_unlock(&mutex);
  pthread_exit(NULL);
}



//main
int main(int argc, char *argv[])
{
  int m,n,i,j,k,g,t,min,pri,temp,rc,base;
  int *p;
  int h[21];//le tableau de processus qui commencent dans ce quantum
  float *pourcentage;
  m=0;
  elu=21;
  base=0;
  nbprocessus=0;
  pthread_t threads[22];
//modifer le pourcentage
  pourcentage=modifier_pourcentage();
//entrer les informations pour chaque processus
  n=information();
//table d'allocation
  p=table_allocation(c,pourcentage);
//stocker les processus dans file de priorite
  min=info[0].date;//le plus petit date de soumission
  for(i=0;i<n;i++)
  {
      if(info[i].date<=min)
         min=info[i].date;
  }
  //printf("min=%d\n",min);
  for(i=0;i<n;i++)
  {
      if(info[i].date==min)
      {
         printf("Le premier processus arrive est processus%d\n",i);
         for(k=0;k<20;k++)
         {
             if(filetotal[info[i].priorite][k]==0)
             {
                filetotal[info[i].priorite][k]=i+1;
                break;
             }
         }
         printf("processus%d est stocke\n",i);
         printf("son priorite=%d\n",info[i].priorite);
         h[m]=i;
         m++;
      }
  }
  for(i=0;i<c;i++)
  {
      t=min+i*q;//quantum commence avec l'arrive de premier processus
      printf("le quantum%d\n",i);
      pri=p[i];//la priorite elu par le table d'allocation
      for(j=0;j<n;j++)//numbre de processus
      {
          if(info[j].date>t && info[j].date<=(t+q))// stocker
          {
             quantum_suivant[nbprocessus]=j+1;
             printf("processus%d ne peut pas etre choisi dans ce quantum\n",quantum_suivant[nbprocessus]-1);
             nbprocessus++;
             for(k=0;k<20;k++)
             {
                 if(filetotal[info[j].priorite][k]==0)
                 {
                    filetotal[info[j].priorite][k]=j+1;
                    break;
                 }
             }
             printf("processus%d est stocke\n",j);
             printf("son priorite=%d\n",info[j].priorite);
             h[m]=j;
             m++;
          }
      }
//creer les threads par les informations
      if(m!=0)//il y a processus qui commence dans ce quantum
      {
         for(k=0;k<m-1;k++)//tri a bulles
         {
             for(j=0;j<m-k-1;j++)
             {
                 if(info[h[j]].date>info[h[j+1]].date)
                 {
                    temp=h[j];
                    h[j]=h[j+1];
                    h[j+1]=temp;
                 }
             }
         }
         for(k=0;k<m;k++)
         {
             //printf("threadid=%d\n",base+k);
             rc = pthread_create(&threads[base+k], NULL, creation,  (void *)h[k]);
             if (rc)
             {
                 printf("Erreur Creation Threads %d\n", rc);
                 exit(-1);
             }
             sleep(1);
         }
         //printf("k=%d\n",k);
         base=base+k;//afin d'utiliser different threadid pour creer thread
         m=0;
      }
      rc = pthread_create(&threads[21], NULL, jury,  (void *)pri);
      if (rc)
      {
          printf("Erreur Creation Threads %d\n", rc);
          exit(-1);
      }
      sleep(1);
      pthread_join(threads[21],NULL);
  }
//attend la fin de toutes les threads et liberation des ressources
  for(i=0;i<n;i++)
  {
        pthread_join(threads[i],NULL);
        pthread_mutex_destroy(&info[i].m);
        pthread_cond_destroy(&info[i].c);
  }
  pthread_mutex_destroy(&mutex);
  exit(0);
}
