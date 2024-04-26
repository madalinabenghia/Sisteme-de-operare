#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void fileTree(char *path,int fd,int level)
{
    DIR *director;
    director=opendir(path);
    if(director == NULL)
    {
        perror("Eroare la deschidere");
        exit(2);
    }
    struct dirent *entry;
    char snapshot[1000],path2[1000];
    struct stat buf;
    int i;
    while((entry=readdir(director)) != NULL)
    {
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0)
        {
            lstat(entry->d_name,&buf);
            for(i=0;i<level;i++)
            {
                printf("\t");
                sprintf(snapshot,"\t");
                write(fd,snapshot,strlen(snapshot));
            }
            printf("%s\n",entry->d_name);
            //sprintf(snapshot,"%s %ld %d %ld %ld\n", entry->d_name, buf.st_ino, buf.st_mode, buf.st_size, buf.st_mtime, buf.tv_sec);
            write(fd,snapshot,strlen(snapshot));
            strcpy(path2,path);
            strcat(path2,"/");
            strcat(path2,entry->d_name);
            if(opendir(path2) != NULL)
            {
                fileTree(path2,fd,level+1);
            }
        }
    }
    closedir(director);
}


int deschidereFisier(char *numeFis)
{
    int idFis;
    idFis=open(numeFis,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(idFis == -1)
    {
        char mesajEroare[100]="Eroare la deshidere ";
        strcat(mesajEroare,numeFis);
        perror(mesajEroare);
        exit(3);
    }
    return idFis;

}

void editareNume(char *numeFis)
{
    int i;
    for(i=0;i<strlen(numeFis)-1;i++)
    {
        if(numeFis[i] == '/')
        {
            numeFis[i]='-';
        }
    }
}


int creareSnapshot(char *numeFis)
{
    int idFis;
    char numeSnapshot[100]="snapshot_";
    strcat(numeSnapshot,numeFis);
    strcat(numeSnapshot,".txt");
    editareNume(numeSnapshot);
    idFis=deschidereFisier(numeSnapshot);
    return idFis;
}


int cautareFisierInDirector(char *numeDir,char *numeFis)
{
    int flag=0;
    DIR *director;
    director=opendir(numeDir);
    if(director == NULL)
    {
        //return;
        perror("Eroare la deschidere director");
        exit(2);
    }
    struct dirent *entry;
    char path2[1000];
    while((entry=readdir(director)) != NULL)
    {
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0)
        {
            if(strcmp(entry->d_name,numeFis) == 0)
            {
                flag=1;
                break;
            }
            strcpy(path2,numeDir);
            strcat(path2,"/");
            strcat(path2,entry->d_name);
            if(opendir(path2) != NULL)
            {
                
                //closedir(path2);
                cautareFisierInDirector(path2,numeFis);
            }
        }
    }
    closedir(director);
    return flag;
}

int main(int argc,char **argv)
{
    int i,idFis,idFis2;
    char output[50];
    if(argc > 13)
    {
        perror("Numar de argumente incorect");
        exit(1);
    }
    for(i=0;i<argc;i++)
    {
        if(strcmp(argv[i],"-o") == 0)
        {
            mkdir(argv[i+1],0777);
            strcpy(output,argv[i+1]);
            break;
        }
    }
    for(i=1;i<argc-2;i++)
    {
        char numeSnapshot[100];
        strcpy(numeSnapshot,"snapshot_");
        strcat(numeSnapshot,argv[i]);
        strcat(numeSnapshot,".txt");
        editareNume(numeSnapshot);
        if(cautareFisierInDirector(output,numeSnapshot) == 1)
        {
            printf("Exista un snapshot pentru directorul %s\n",argv[i]);
            strcpy(numeSnapshot,"snapshot2_");
            strcat(numeSnapshot,argv[i]);
            strcat(numeSnapshot,".txt");
            editareNume(numeSnapshot);
            printf("%s\n",numeSnapshot);
            chdir(output);
            idFis2=deschidereFisier(numeSnapshot);
            chdir("..");
            fileTree(argv[i],idFis2,0);
            printf("\n");
            close(idFis2);

        }
        else
        {
            printf("Directorul %s nu a fost monitorizat pana acum\n",argv[i]);
            chdir(output);
            idFis=creareSnapshot(argv[i]);
            chdir("..");
            fileTree(argv[i],idFis,0);
            printf("\n");
            close(idFis);
        }
        
    }
    pid_t pid;
    if ( (pid=fork()) < 0)
    {
        perror("Eroare creare proces");
        exit(1);
    }
    if(pid == 0)
    {

    }
    return 0;
}