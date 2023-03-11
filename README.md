IONESCU ELENA 336CA

In cadrul acestei teme am implementat tot, mai putin functiile so_popen si so_pclose.

Pentru rezolvarea acestei teme am implementat structura _so_file, in care am
urmatoarele campuri:

- buffer, variabila pe care o aloc dinamic, in care retin caracterele cu care lucrez;
- offset, variabila cu care iterez prin buffer, cand citesc/scriu caractere in buffer;
- file_descriptor, variabila in care retin file descriptorul fisierului asociat;
- file_size, variabila in care retin numarul de caractere pe care le adaug in fisier;
- element_size, variabila in care retin dimensiunea unui element din buffer;
- index, variabila in care retin indexul curent din fisier;
- caractere_procesate, variabila in care retin numarul de caractere scrie in fisier;
- sfarsit_de_fisier, variabila in care retin 0, daca nu am ajuns la finalul
  fisierului sau 1, daca am ajuns la finalul fisierului;
- eroare, variabila in care retin 1 daca am intalnit o eroare pe parcurs sau 
  0 in caz contrar;
- ultima_operatie, variabila in care retin 1, daca ultima operatie a fost de 
  scriere sau 0 daca a fost de citire.

	Am implementat mai intai functia so_fopen, in care verific ce tip de fisier 
trebuie sa deschid ca sa setez flag-urile in mod corect. In continuare, am implementat
functiile so_fgetc si so_fputc, iar pe baza lor am implementat so_fread si so_fwrite. 
A fost putin complicat sa inteleg cum sa leg so_fgetc de so_fread si so_fputc de 
so_fwrite, dar mi-au trecut testele, pana la urma. Pentru so_fread, compar
numarul de elemente pe care vreau sa le scriu cu dimensiunea maxima a buffer-urului,
ca sa imi dau seama cate apeluri de sistem  trebuie sa fac. Am procedat similar si la
so_fwrite. Pentru so_ferror, verific daca campul error din structura este 1 sau 0,
valoare pe are o actualizez pe parcurs daca dau de vreo eroare. Pentru so_feof,
verific daca campul sfarsit_de_fisier este 1 sau 0, valoare pe care o actualizez pe
parcurs. Pentru functia fseek, in functie de ultima operatie, fac actualizarile
cerute in cerinta si pozitionez cursorul fisierului la pozitia dorita. La functia 
fflush, daca ultima operatie a fost una de scriere, scriu datele din buffer in fisier.
La functia so_fclose, apelez so_fflush, inchid file descriptorul asociat fisierului
si eliberez memoria. In cadrul functiei so_fileno, intorc file descriptorul asociat
fisierului. In functia so_ftell, intorc pozitia curenta din fisier, reprezentata de
numarul de caractere scrise in fisier pana atunci. In ceea ce priveste functiile
so_popen si so_pclose, am avut dificultati in implementare si nu mi-au trecut testele,
motiv pentru care am sters rezolvarea, deoarece nu am inteles exact cum sunt asociati
file descriptorii din capete catre ce proces(parinte sau copil). 

Pot spune ca tema mi s-a parut accesibila, interesanta si folositoare. Este un 
punct de pornire de baza pentru formarea cunostintelor in domeniul sistemelor de operare, 
ca sa intelegem cum si cand facem apeluri de sistem. Mi-as fi dorit sa fie explicat enuntul
mai clar, sa se dea mai multe exemple concrete pentru functiile pentru care se doreste 
implementarea. Consider ca implementarea temei a fost usor naiva; se putea mai bine.

Pentru Makefile, am o regula de clean care imi sterge fisierul obiect generat si 
biblioteca dinamica generata, o regula care imi genereaza fisierul obiect si o regula 
care imi creeaza biblioteca dinamica. Pentru realizarea acestui Makefile, m-am ajutat 
de exemplele din laboratorul 1.

Link-uri folosite in rezolvarea temei:
-> m-am inspirat din laboratorul 1 ca sa realizez Makefile-ul https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-01 
-> m-am inspirat din laboratorul 2 ca sa implementez codul  https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-02

