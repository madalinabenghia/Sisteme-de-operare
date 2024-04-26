#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>


void errorMessage(char *message)
{
    perror(message);
    exit(1);
}

struct stat return_lstat(char *nume)
{
    struct stat buf;
    if(lstat(nume,&buf) == -1)
    {
        errorMessage("Eroare la lstat");
    }
    return buf;
}

int verficareDirector(char *nume)
{
    struct stat buf;
    buf=return_lstat(nume);
    return S_ISDIR(buf.st_mode);
}

void fileTree(char *path,int fd,int level)
{
    DIR *director;
    director=opendir(path);
    if(director == NULL)
    {
        //return;
        errorMessage("Eroare la deschidere director");
        
    }
    struct dirent *entry;
    char path2[1000],snapshot[1000];
    struct stat buf;
    int i;
    while((entry=readdir(director)) != NULL)
    {
        if(strcmp(entry->d_name,".") != 0 && strcmp(entry->d_name,"..") != 0)
        {
            buf=return_lstat(path);
            //lstat(entry->d_name,&buf);
            for(i=0;i<level;i++)
            {
                printf("\t");
                sprintf(snapshot,"\t");
                write(fd,snapshot,strlen(snapshot));
            }
            printf("%s\n",entry->d_name);
           //!!!!!!!!!! sprintf(snapshot,"%s %ld %d %ld %ld\n",entry->d_name,buf.st_ino,buf.st_mode,buf.st_size, buf.st_mtim.tv_sec);
            write(fd,snapshot,strlen(snapshot));
            //write(fd,entry->d_name,sizeof(entry->d_name));
            //write(fd,&buf.st_mode,sizeof(buf.st_mode));
            //write(fd,&buf.st_ino,sizeof(buf.st_ino));
            strcpy(path2,path);
            strcat(path2,"/");
            strcat(path2,entry->d_name);
            if(verficareDirector(path2) == 1)
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
        //perror("Eroare la deshidere snapshot");
        exit(3);
    }
    return idFis;
}

int deschidereCitireFisier(char *numeFis)
{
    int idFis;
    idFis=open(numeFis,O_RDONLY);
    if(idFis == -1)
    {
        char mesajEroare[100]="Eroare la deshidere ";
        strcat(mesajEroare,numeFis);
        perror(mesajEroare);
        //perror("Eroare la deshidere snapshot");
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

void comparareFisiere(char *numeFis1,char* numeFis2)
{
    
    ssize_t bytes_read1,bytes_read2;
    char buf1[100],buf2[100];
    int fd1,fd2,ok=0;
    /*
    while(((citit1=read(idFis,buf1,sizeof(buf1))) > 0) && ((citit2=read(idFis2,buf2,sizeof(buf2))) > 0))
    {
        if(citit1 != citit2 || memcmp(buf1,buf2,citit1) != 0)
        {
            printf("S-au facut modificari\n");
            break;
        }
    }
    printf("Nu s-au facut modificari\n");
    */
    if ((fd1 = open(numeFis1, O_RDONLY)) == -1)
    {
        perror("Eroare deshidere fisier");
        exit(EXIT_FAILURE);
    }
    if ((fd2 = open(numeFis2, O_RDONLY)) == -1) 
    {
        perror("Eroare deschidere fisier");
        close(fd1);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read1 = read(fd1, buf1, sizeof(buf1))) > 0 &&(bytes_read2 = read(fd2, buf2, sizeof(buf2))) > 0) 
    {
        if (bytes_read1 != bytes_read2 || memcmp(buf1, buf2, bytes_read1) != 0) 
        {
            ok=1;
            break;
            //exit(EXIT_SUCCESS);
        }
    }
    if(ok == 1)
    {
        printf("S-au facut modificari\n");
    }
    else
    {
        printf("Nu s-au facut modificari\n");
    }
    close(fd1);
    close(fd2);   
}


void redenumireStergere(char* numeFis1,char* numeFis2)
{
    strcpy(numeFis2,numeFis1);
    //printf("%s\n",numeFis2);
    if(remove(numeFis1) == -1)
    {
        errorMessage("Eroare la stergrea fisierului");
    }
    
}


int main(int argc,char **argv)
{
    int i,idFis,idFis2,wstatus;
    char output[50];
    struct stat buf;
    pid_t pid;
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
        if(verficareDirector(argv[i]) != 1)
        {
            continue;
        }
        if( ( pid=fork() ) < 0)
        {
            perror("Eroare");
            exit(1);
        }
        if(pid==0)
        {
            buf=return_lstat(argv[i]);
            char numeSnapshot[100],numeSnapshot2[100],inode[100];
            sprintf(inode,"%ld",buf.st_ino);
            strcpy(numeSnapshot,"snapshot_");
            strcat(numeSnapshot,inode);
            strcat(numeSnapshot,".txt");
            editareNume(numeSnapshot);
            if(cautareFisierInDirector(output,numeSnapshot) == 1)
            {
                printf("Exista un snapshot pentru directorul %s\n",argv[i]);
                strcpy(numeSnapshot2,"snapshot2_");
                strcat(numeSnapshot2,inode);
                strcat(numeSnapshot2,".txt");
                editareNume(numeSnapshot2);
                //printf("%s-%s\n",numeSnapshot,numeSnapshot2);
                chdir(output);
                idFis2=deschidereFisier(numeSnapshot2);
                //idFis=deschidereCitireFisier(numeSnapshot);
                chdir("..");
                fileTree(argv[i],idFis2,0);
                printf("\n");
                //comparareFisiere(idFis,idFis2);
                //close(idFis);
                close(idFis2);
                chdir(output);
                comparareFisiere(numeSnapshot,numeSnapshot2);
                //redenumireStergere(numeSnapshot,numeSnapshot2);
                chdir("..");
            }
            else
            {
                printf("Directorul %s nu a fost monitorizat pana acum\n",argv[i]);
                chdir(output);
                idFis=creareSnapshot(inode);
                chdir("..");
                fileTree(argv[i],idFis,0);
                printf("\n");
                close(idFis);
            }
            exit(0);
        }
    }
    //while(wait(NULL) > 0)
    while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus))
    {
        //printf("Procesele au fost terminate\n");
        exit(EXIT_SUCCESS);
    }
    return 0;
}