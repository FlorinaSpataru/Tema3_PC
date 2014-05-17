// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 3 PC // TCP Multiplex

   Pentru a putea face diferenta intre mesajele pe care le trimit mi-am facut o
lista de tipuri si mi-am creat o structura care are campurile tip si payload.
Basically, cand primesc un mesaj, ii verific tipul si apoi vad ce fac cu
payload-ul.

/* Type of messages sent from client:
 *      listclients                     23
 *      infoclient <client_name>        24
 *      message <client_name> <message> 25
 *      broadcast <message>             26
 *      sendfile <dest_client> <file>   27
 *      quit                            28
 */

/* Message types
 *      accept                          42
 *      kick/reject                     100
 *      no such client                  113
 */

   O sa incep cu ce se intampla in server.
Imi deschid socketul de administrare, cel pe care primesc noi conexiuni, si
ascult pe el. Apoi am un while (1) in care se intampla totul:
- Fac un select pe multimea file descriptorilor
- Parcurg fd-urile intoarse de select
- Daca am primit ceva pe socketul de administrare, inseamna ca am o noua
conexiune cu un client. Il accept temporar; primesc informatii despre acel
client si verific daca mai am vreun client cu acelasi nume (nu e nevoie sa
verific portul pentru ca oricum imi intoarce eroare la bind). Daca este un nume
duplicat, serverul va trimite un mesaj de reject(100), altfel unul de
accept(42).
- Daca am primit ceva pe socketul 0 inseamna ca s-au introdus date de la
tastatura. Asa ca voi verifica ce comanda primesc.
   - Daca e status, afisez clientii conectati in acel moment
   - Daca e kick <nume>, ii trimit clientului in cauza mesaj de kick(100)
   - Daca e quit, trimit tuturor clientilor mesaje de kick(100)
- Altfel, inseamna ca un client imi trimite ceva
   - Rezultatul lui recv imi va spune daca acel client mai e alive
   - Verific ce mesaj mi-a trimis clientul folosid un case pe tip
   - Daca primesc listclients(23), ii trimit clientului inapoi o lista a
clientilor conectati in acel moment
   - Daca primesc infoclient <nume>(24), caut acel client in lista si trimit
raspuns cu informatiile despre acesta intr-un (24), sau, daca acel client nu a
fost gasit, trimit (113) - client not found
   - Daca primesc message <nume>(25), fac acelasi lucru ca la infoclient

   Chestii specifice clientului:
La inceput voi deschide doi socketi - cel cu care ma conectez cu serverul si
cel de administrare, prin care voi comunica cu ceilalti clienti. Voi adauga in
multimea de listen si socketul pe care comunic cu serverul. Apoi am un while(1):
   - msg_in_progress este un vector de strcturi de tipul chat. Aici voi retine
mesajele pe care nu le-am trimis inca. Fac asta pentru ca mesajele nu se trimit
instant, intai trebuie sa fac o cerere la server sa aflu informatiile despre
clientul respectiv, sa creez o conexiune cu acesta, sa-l adaug in multimea
pentru select() si mai apoi sa ii trimit mesajul in cauza
   - Deci, primul lucru facut in while(1) este sa verific daca pot sa trimit
vreunul din mesajele aflate in asteptare. Initial, cand adaug elemente in acest
vecotor, voi avea portul 0, pt ca il voi afla mai tarziu prin intermediul
serverului
   - Daca gasesc un mesaj ce poate fi trimis (cunosc portul clientului si nu a
mai fost trimis), voi verifica daca exista deja o conexiune cu clientul in
cauza
      - Daca nu exista conexiune, o creez si adaug acel client in client_list
      - Dupa ce creez conexiunea, ii trimit clientului respectiv datele mele,
ca sa ma adauge si el in client_list-ul lui. Astfel, stiu ca s-a realizat
conexiunea cu succes
      - Apoi urmeaza trimiterea mesajului efectiv catre acel client

      - Daca exista deja conexiunea, doar trimit mesajul
   - Golesc msg_in_progress de mesaje ce au fost deja trimise
   - Apoi urmeaza select()
   - Daca primesc ceva pe socketul 0, inseamna comanda de la tastatura
      - listclients -> trimit la server o cerere de tipul(23)
      - infoclient -> tipul(24)
      - Daca primesc message, o sa adaug in primul rand, un mesaj incomplet in
msg_in_progress. Incomplet insemnand ca eu nu cunosc portul acelui client si
nici nu stiu daca suntem conectati sau nu (aceste verificari se fac mai sus,
dupa cum am zis)
      - quit -> trimit tuturor clientilor conectati cu mine si serverului un
mesaj ca dau quit(28)
   - Daca primesc ceva pe socketul de administrare, inseamna ca un nou client
vrea sa se conecteze la mine
      - Ii dau accept si, in felul in care am implementat eu, primesc imediat
date despre acel client ca sa il adaug in lista mea de conexiuni(client_list)
   - Daca primesc ceva pe socketul de la server inseamna ca serverul imi
trimite chestii
      - Din nou, verific ca serverul sa fie inca alive
      - Apare din nou codificarea mesajelor
         - kick -> serverul ma da afara(100)
         - listclients -> imi trimite raspuns la o cerere anterioara(23)
         - infoclient -> la fel, tipul(24)
         - tipul(113) -> am cerut infoclient pentru un client care nu exista
         - message -> tipul(25), functioneaza la fel ca infoclient. Folosesc
informatiile primite pentru a actualiza msg_in_progress, adaugand portul.
   - Altfel, primesc mesaje de la clienti
      - Verific daca acel client este alive
      - Daca da, singurul tip de mesaje pe care il pot primi este(25) deci
afisez pe ecran mesajul trimit de catre clientul respectiv

   Cam asta e toata implementarea. Urmeaza sa argumentez anumite alegeri pe care
le-am facut in privinta implementarii, sa mentionez lucrurile care nu merg asa
bine, sa ma plang de problemele pe care le-am intampinat, alte diverse chestii
si mai apoi feedback.

   In primul rand, am vazut pe forum ca ai fi vrut sa trimitem intai un mesaj cu
dimensiunea si apoi mesajul efectiv. Well, avand in vedere ca nu am facut
alocari dinamice pentru buffere, nu prea avea sens, ba chiar s-ar fi complicat
mai mult. Am incercat o prima abordare in felul asta, si imi facusem o functie
send() ce trimitea doua mesaje intr-unul si la fel un receive(). Dar, din nou
zic, mai mult m-am incurcat.

   Am folosit tipuri pentru mesaje pentru ca am vrut o structura "generica" pe
care sa o folosesc pentru trimitere. Cred ca mi-a iesit destul de bine treaba
asta, cu mentiunea ca, intr-adevar, primul send din client este cu un buffer,
nu o structura de tip mesaj. Deja aveam prea multe codificari si nu mai avea
sens sa introduc un tip nou pentru ceva ce trimit o singura data in tot
programul.

   Ce nu merge asa bine - message. Problema este ca primul mesaj trimis catre un
client, atunci cand nu exista o conexiune, nu este primit. Restul mesajelor se
trimit fara probleme. Nu mi-am putut da seama de ce face treaba asta. In rest,
totul merge perfect, as far as I know.

   Dificultatile pe care le-am intampinat ar fi lipsa timpului. Este o tema
laborioasa, nu foarte grea, si ar fi putut sa iasa ceva chiar frumos, dar nu am
avut timpul necesar.

   Mentionez ca am folosit scheletul de cod de la laborator.

   M-am gandit si la cum as fi implementat celelalte cerinte. In primul rand,
broadcast iesea usor daca ar fi mers message perfect. Ar fi fost si mai usor
daca as fi avut o functie pentru message si una pentru listclients. Astfel, in
broadcast doar le-as fi apelat pe acestea doua si gata.
   Nici trimiterea de fisiere nu cred ca era grea, probabil as fi avut un vector
ca msg_in_progress, eventual files_in_progress, un vector de structuri in care
sa pastrez file descriptorul fisierului, clientul care mi-l trimite, portul,
dimensiunea totala si received_so_far. As fi pus acel select cu timeout ca sa
imi permita sa primesc si comenzi in paralel. In mare, ar fi functionat tot cam
ca message.

   Ca feedback, as zice ca tema a fost chiar interesanta, si mi-as fi dorit sa o
termin.