#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>   //manipulare directoare
#include <string.h>
#include <sys/stat.h> //metadata
#include <time.h>     //timp
#include <unistd.h>   //fork() si pid_t
#include <sys/wait.h> //waitpid() si WIFEXITED

#define LUNGIME_CALE_MAX 50
#define NR_LINII_MAX 3
#define NR_CUV_MAX 1000
#define NR_CARACT_MAX 2000

//structura pentru stocarea metadatelor fiecărei intrări din snapshot
struct intrareSnapshot{
    char tip;
    char nume[256];
    long dimensiune;
    char ultimaModificare[50];
};

//structura stocare informatii fisier
struct informatiiFisier{
    char nume[256];
    mode_t permisiuni; //
};

//functie creare snapshot intr un proces copil separat
void creareSnapshot(const char *caleDirector, const char *caleIesire){ //caleIesire=calea catre directorul in care voi stoca toate snapshoturile
    DIR *dir;                       //director
    struct dirent *intrare;         //intrare director
    struct tm *timp;                //info timp
    struct stat infoFisier;         //info fisier
    char caleFisier[1024];          //stocheaza calea catre fisiere/dir
    time_t t;                       //retine timpul ultimei modificari a unui fisier
    char ultimTimpModificat[50];
    
    dir=opendir(caleDirector);      //deschidere director
    if(dir==NULL){
        perror("nu se poate deschide directorul");
        exit(EXIT_FAILURE);
    }
    
    snprintf(caleFisier, sizeof(caleFisier), "%s/Snapshot.txt", caleIesire); //construieste calea catre snapshot in functie de calea iesire
    FILE *fisierSnapshot=fopen(caleFisier,"a+"); //append+ pentru a permite adaugarea de date la sfarsitul fisierului existent
    if(fisierSnapshot==NULL){
        perror("nu se poate folosi fisierul snapshot");
        exit(EXIT_FAILURE);
    }
    
    
    int nrIntrariVechi=0;
    struct intrareSnapshot *snapshotVechi=NULL; //vector de structuri care stocheaza intrari
    
    //citire nr intrari din snapshot vechi
    char linie[256];
    while (fgets(linie, sizeof(linie), fisierSnapshot)!=NULL){
        nrIntrariVechi++;
    }
    
    //alocare memorie vector intrari pt ca nu stiu cate sunt
    snapshotVechi=malloc(nrIntrariVechi*sizeof(struct intrareSnapshot));
    if(snapshotVechi==NULL){
        perror("eroare la alocarea memoriei pt snapshot vechi");
        exit(EXIT_FAILURE);
    }
    
    rewind(fisierSnapshot); //repunere cursor de fisier la inceput pt a citi din nou  fisierul

    //citire intrari si stocare in snapshotul vechi din fisier
    nrIntrariVechi=0; //resestez nr de intrari
    while(fgets(linie, sizeof(linie), fisierSnapshot)!=NULL){ //citesc fiecare linie din fisier si stochez in buferul "linie" pana la NULL
        struct intrareSnapshot intrare;
        sscanf(linie, "%c %s %ld %s\n", &intrare.tip, intrare.nume, &intrare.dimensiune, intrare.ultimaModificare); //stocheaza in structuri
        snapshotVechi[nrIntrariVechi++]=intrare; 
    }
    
    
    while((intrare=readdir(dir))!=NULL){  //daca inca exista intrari de citit din director
        if (strcmp(intrare->d_name, ".")==0 || strcmp(intrare->d_name, "..")==0) {   //verifica dc numele intrarii citite este dir. curent sau dir. parinte ".."
            continue; // ignoră directorul curent și directorul părinte
        }
        
        snprintf(caleFisier, sizeof(caleFisier), "%s/%s", caleDirector, intrare->d_name); //construieste calea catre fiecare intrare din dir
        
        //obtinere metadate pt fiecare intrare
        if(stat(caleFisier, &infoFisier)==0){
            t=infoFisier.st_mtime;
            timp=localtime(&t);
            strftime(ultimTimpModificat, sizeof(ultimTimpModificat), "%Y-%m-%d %H:%M:%S", timp); //2024-05-26 15:25:36 formateaza data si ora ultimei modificari
            
            
            //verific daca intrarea e director sau fisier
            char tip;
            if (S_ISDIR(infoFisier.st_mode)) { //verifică dacă intrarea curentă din director este un director
                tip='D';
            } 
            else {
                //dacă intrarea nu este un director, se considera un fișier
                tip='F';
            }
            
            int gasit=0;  //itrare curenta gasita?
            for(int i=0;i<nrIntrariVechi;i++){
                if (strcmp(intrare->d_name, snapshotVechi[i].nume)==0){
                    gasit=1;  //daca s  gasit intrare
                    
                    //se compara metadatele din snapshotul vechi cu cele din snapshotul nou
                    if(tip==snapshotVechi[i].tip && infoFisier.st_size==snapshotVechi[i].dimensiune && strcmp(ultimTimpModificat, snapshotVechi[i].ultimaModificare)==0){
                        //nu s a schimbat nimic, nu facem nimic
                    }
                    
                    //daca sunt diferite metadatele atunci se modifica intrarea cu noile info
                    else{
                        //actualizam snapshotul (suprapunere), intrare modificata
                        fprintf(fisierSnapshot, "%c %s %ld %s\n", tip, intrare->d_name, infoFisier.st_size, ultimTimpModificat);
                    }
                    break;
                }
            }
            
            //daca nu s a gasit intrare in snapshotul vechi
            if(!gasit){
                //intrare noua, se adauga la snapshot
                fprintf(fisierSnapshot,"%c %s %ld %s\n", tip, intrare->d_name, infoFisier.st_size, ultimTimpModificat);
            }
        }
    }
    
    free(snapshotVechi);
    fclose(fisierSnapshot);
    closedir(dir);
}

//identificare semne maleficienta sau coruptie
void analizaSintactica(const char *caleFisier, const char *caleIzolare, const char *scriptAnaliza){
    FILE *fisier=fopen(caleFisier,"r");   //deschidere fisier
    if(fisier==NULL){
        perror("nu se poate deschide fisierul");
        exit(EXIT_FAILURE);
    }
    
    //verific cuvintele cheie daca exista
    char *cuvinteCheie[] = {"corrupted", "dangerous", "risk", "attack", "malware", "malicious"};
    char linie[1024];
    
    while (fgets(linie, sizeof(linie), fisier)!=NULL){   //citesc fiecare linie din fisier si stochez in buferul "linie" pana la NULL
        for (int i=0; i<sizeof(cuvinteCheie)/sizeof(cuvinteCheie[0]); i++){    //parcurge fiecare cuv cheie
            if (strstr(linie, cuvinteCheie[i])!=NULL){   //verific daca linia curent contine un cuv cheie
                // fisierul contine unul dintre cuvintele cheie si se izoleaza
                char *numeFisier=strrchr(caleFisier, '/');    //caut ultima aparitie a caracterului '/' in caleFisier
                if (numeFisier==NULL){  //daca nu gasesc caracterul, egalez numeFisier cu calea sa
                    numeFisier=caleFisier;
                }
                else{  //altfel
                    numeFisier++; // Sarim peste caracterul '/'
                }
                
                //calea completa pt fisier izolat
                char caleIzolat[1024];
                snprintf(caleIzolat, sizeof(caleIzolat), "%s/%s", caleIzolare, numeFisier); //concatenez calea de izolare cu numele fisierului

                // Mutam fisierul in directorul de izolare
                if (rename(caleFisier, caleIzolat)==-1){ //fct rename primeste calea originala a fisierului caleFisier calea unde fisierul trebuie mutat, adica caleFisier
                    perror("eroare la izolare fisier");
                    exit(EXIT_FAILURE);
                }

                printf("fisierul %s a fost izolat cu succes in directorul %s\n", numeFisier, caleIzolare);
                fclose(fisier);
                exit(EXIT_SUCCESS);
            }
        }
    }

    // Inchidem fisierul dupa analiza
    fclose(fisier);
    exit(EXIT_SUCCESS);
}

//functie pentru verificare drepturi lipsa si creare proces pt analiza fisiere periculoase
void verificareAnaliza(const char *caleDirector, const char *caleIzolare, const char*scriptAnaliza){
    DIR *dir;                       //director
    struct dirent *intrare;         //intrare director
    struct stat infoFisier;         //info fisier
    
    dir=opendir(caleDirector);      //deschidere director
    if(dir==NULL){
        perror("nu se poate deschide directorul");
        exit(EXIT_FAILURE);
    }
    
    while((intrare=readdir(dir))!=NULL){  //citesc fiecare intrare din directorul dir
        char caleFisier[1024];
        snprintf(caleFisier, sizeof(caleFisier), "%s%s", caleDirector, intrare->d_name); //construieste calea catre fiecare intrare din dir
        
        //verific daca calea curentaeste un fisier
        if (stat(caleFisier, &infoFisier)==0 && S_ISREG(infoFisier.st_mode)){
            // verific drepturile lipsa ale fisierului
            if ((infoFisier.st_mode & S_IRUSR)==0 && (infoFisier.st_mode & S_IWUSR)==0 && (infoFisier.st_mode & S_IXUSR)==0 &&
                (infoFisier.st_mode & S_IRGRP)==0 && (infoFisier.st_mode & S_IWGRP)==0 && (infoFisier.st_mode & S_IXGRP)==0 &&
                (infoFisier.st_mode & S_IROTH)==0 && (infoFisier.st_mode & S_IWOTH)==0 && (infoFisier.st_mode & S_IXOTH)==0) {
                // fisierul are toate drepturile lipsa, cream un proces pt analiza sa
                pid_t pid = fork(); //creare proces nou prin fork()
                if (pid==-1){
                    perror("eroare la creare proces");
                    exit(EXIT_FAILURE);
                }
                else if(pid==0){
                    //suntem in procesul fiu, deci efectuam analiza sintactica
                    analizaSintactica(caleFisier, caleIzolare, scriptAnaliza);
                }
                else{
                    // suntem in procesul parinte, asteptam terminarea procesului fiu
                    int status;
                    waitpid(pid, &status, 0);
                    
                    if (WIFEXITED(status)){
                        printf("Procesul cu PID-ul %d s-a incheiat cu codul %d\n", pid, WEXITSTATUS(status));
                    }
                }
            }
        }
    }

    closedir(dir);
}

//SAPTAMANA10
void analizaSintactica10(const char *caleFisier){
    FILE *fisier=fopen(caleFisier, "r");
    if(fisier==NULL){
        perror("nu se poate deschide fisierul");
        exit(EXIT_FAILURE);
    }
    
    int nrLinii=0, nrCuv=0, nrCaract=0;
    char linie[1024];
    
    //numar liniile, cuv si caract
    while(fgets(linie,sizeof(linie),fisier)!=NULL){
        nrLinii++;
        char *token=strtok(linie," \t\n"); //dupa spatii, taburi si linie noua
        while(token!=NULL){
            nrCuv++;
            nrCaract=nrCaract+strlen(token);
            token=strtok(NULL, " \t\n");
        }
    }
    
    fclose(fisier);
    
    //verific criterile de siguranta
    if(nrLinii<NR_LINII_MAX && nrCuv>NR_CUV_MAX && nrCaract>NR_CARACT_MAX){
        printf("fisierul %s e suspect\n", caleFisier);
    }
    else{
        printf("fisierul %s este in regula\n", caleFisier);
    }
}

void verificareAnaliza10(const char *caleDirector)
{
    DIR *dir;                       //director
    struct dirent *intrare;         //intrare director
    struct stat infoFisier;         //info fisier
    
    dir=opendir(caleDirector);      //deschidere director
    if(dir==NULL){
        perror("nu se poate deschide directorul");
        exit(EXIT_FAILURE);
    }
    
    while((intrare=readdir(dir))!=NULL){  //citesc fiecare intrare din directorul dir
        char caleFisier[1024];
        snprintf(caleFisier, sizeof(caleFisier), "%s%s", caleDirector, intrare->d_name); //construieste calea catre fiecare intrare din dir
        
        //verific daca calea curenta este un fisier
        if (stat(caleFisier, &infoFisier)==0 && S_ISREG(infoFisier.st_mode)){
            // verific drepturile lipsa ale fisierului
            if ((infoFisier.st_mode & S_IRUSR)==0 && (infoFisier.st_mode & S_IWUSR)==0 && (infoFisier.st_mode & S_IXUSR)==0 &&
                (infoFisier.st_mode & S_IRGRP)==0 && (infoFisier.st_mode & S_IWGRP)==0 && (infoFisier.st_mode & S_IXGRP)==0 &&
                (infoFisier.st_mode & S_IROTH)==0 && (infoFisier.st_mode & S_IWOTH)==0 && (infoFisier.st_mode & S_IXOTH)==0) {
                // fisierul are toate drepturile lipsa, cream un proces pt analiza sa
                pid_t pid = fork(); //creare proces nou prin fork()
                if (pid==-1){
                    perror("eroare la creare proces");
                    exit(EXIT_FAILURE);
                }
                else if(pid==0){
                    //suntem in procesul fiu, deci efectuam analiza sintactica
                    analizaSintactica10(caleFisier);
                }
                else{
                    // suntem in procesul parinte, asteptam terminarea procesului fiu
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
        }
    }

    closedir(dir);
}


int main(int argc, char *argv[]){
    //verif argumente linie comanda, daca programul a fost apelat cu cel putin 5 argumente
    if(argc<5){
        //daca nu, eroare
        fprintf(stderr, "Utilizare: %s -o output_dir -s izolated_space_dir director1 [director2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    ////////////SAPTAMANA 10
    if(argc<3){
        fprintf(stderr, "Utilizare: %s director1 [director2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    for(int i=1;i<argc;i++){
        verificareAnaliza10(argv[i]);
    }
    
    char *caleIesire=malloc(LUNGIME_CALE_MAX);
    if(caleIesire==NULL){
        perror("eroare la alocare de memorie pt cale iesire");
        exit(EXIT_FAILURE);
    }
    
    int index=1;
    char *caleIzolare=NULL;
    char *caleIzolareFinala=NULL;
    char *caleDirector=NULL;
    char *scriptAnaliza="verify_for_malicious.sh"; // script pt analiza sintactica
    
    // iterez prin argumente linie comanda
    for (int i=1; i<argc; i++){
        if (strcmp(argv[i], "-s")==0){
            // urmatorul argument este directorul de izolare
            caleIzolare=argv[++i];
            continue;
        }
        else if (strcmp(argv[i], "-o")==0){
            // ignor urmatorul argument, care este directorul de iesire
            i++;
            continue;
        }

        caleDirector = argv[i]; //dir curent
        
        // verific daca dir curent este directorul de izolare
        if (caleIzolare!=NULL && strcmp(caleDirector, caleIzolare)==0){
            caleIzolareFinala=caleIzolare;
            continue;
        }

        // verificam daca avem un director de izolare final
        if (caleIzolareFinala == NULL) {
            fprintf(stderr, "eroare: directorul de izolare nu a fost specificat corect.\n");
            return EXIT_FAILURE;
        }

        // verific daca avem directorul de izolare si directorul de lucru
        if (caleIzolare!=NULL && caleDirector!=NULL){
            // efectuez verificarea si analiza pentru fisierul cu drepturi lipsa
            verificareAnaliza(caleDirector, caleIzolareFinala, scriptAnaliza);
        }
    }
    
    //iterez prin directoare si creez snapshoturi icepand de la index specificat, adica 3(primul director, dir1 ca in exemplu)
    int nrProcese=0;
    for(int i=index; i<argc;i++){
        nrProcese++;
        pid_t pid=fork();
        
        if(pid==-1){
            perror("Eroare la crearea procesului copil");
            exit(EXIT_FAILURE);
        }
        else if(pid==0){ //ne aflam in procesul copil si cream snapshotul
            creareSnapshot(argv[i], caleIesire);
            exit(EXIT_SUCCESS);
        }
        else{
            //asteptam pana la final deoarece copiii se ruleaza in paralel
        }
    }
    
    for(int i=0;i<nrProcese;i++){
        int status;
        int pidIesire=wait(&status); //asteptam terminarea procesului copil
            
        //cod pt a crea si a astepta un proces copil
        if(WIFEXITED(status)){
            printf("procesul cu PID-ul %d s-a încheiat cu codul %d\n", pidIesire, WEXITSTATUS(status));
        }
        else{
            printf("procesul cu PID-ul %d s-a încheiat cu eroare\n", pidIesire);
        }
    }
    
    free(caleIesire);
    return EXIT_SUCCESS;
}



/*     permisiuni
proprietar S_IRUSR, S_IWUSR, S_IXUSR
grup       S_IRGRP, S_IWGRP, S_IXGRP
altii      S_IROTH, S_IWOTH, S_IXOTH
*/