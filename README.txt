
## Download (ready-to-use)

👉 https://gumroad.com/l/fastlog





FastLog

Tool da linea di comando per analizzare file di log in modo veloce.

Scritto in C, senza dipendenze particolari.
L'idea è avere qualcosa di semplice che funzioni subito da terminale.

---

Uso base

./fastlog file.log

---

Esempi

./fastlog file.log --filter ERROR
./fastlog file.log --top 5
./fastlog file.log --json
./fastlog file.log --filter ERROR --top 10 --json
./fastlog file.log --debug

---

Cosa fa

- conta quante volte compare ogni IP (assunto primo token)
- permette filtro su substring (es. ERROR)
- mostra i risultati ordinati per frequenza
- output testuale o JSON

---

Assunzioni

- IP è il primo token della riga
- formato log semplice (tipo access log base)

Se il formato cambia molto, il parsing potrebbe non funzionare bene.

---

Note implementative

- hash table con liste per gestire collisioni
- lettura riga per riga (non carica tutto in memoria)
- conversione finale in array + qsort
- gestione errori minimale

---

Limiti

- parsing molto basico
- nessuna validazione IP
- nessun supporto a formati complessi
- niente multi-threading

---

Compilazione

gcc -O2 -Wall -Wextra -o fastlog fastlog.c

---

Test veloce

./fastlog sample.log --top 5

---

Contenuto pacchetto

- fastlog (binario)
- sample.log
- README.txt

---

Autore

Matteo Calvigioni

---

Note finali

Tool semplice, scritto con approccio pragmatico.
Pensato più come utility veloce che come soluzione completa.
