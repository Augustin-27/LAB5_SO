#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cmath>

// Funcție pentru verificarea primarității unui număr
bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i <= std::sqrt(n); ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

int main() {
    const int NUM_PROCESSES = 10;
    const int INTERVAL_SIZE = 1000;

    int parent_to_child[NUM_PROCESSES][2]; // Pipe-uri Părinte -> Copil (trimitere interval)
    int child_to_parent[NUM_PROCESSES][2]; // Pipe-uri Copil -> Părinte (trimitere numere prime)

    // Creează pipe-urile
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(parent_to_child[i]) == -1 || pipe(child_to_parent[i]) == -1) {
            std::cerr << "Eroare la crearea pipe-urilor!\n";
            return 1;
        }
    }

    // Creează cele 10 procese copil
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Eroare la crearea procesului copil!\n";
            return 1;
        }

        if (pid == 0) { // === PROCES COPIL ===
            // Închide capetele nefolosite
            close(parent_to_child[i][1]); // Copilul doar citește din P->C
            close(child_to_parent[i][0]); // Copilul doar scrie în C->P

            int bounds[2];
            read(parent_to_child[i][0], bounds, sizeof(bounds));
            close(parent_to_child[i][0]);

            int start = bounds[0];
            int end = bounds[1];

            // Caută numerele prime și le trimite prin pipe
            for (int num = start; num <= end; ++num) {
                if (is_prime(num)) {
                    write(child_to_parent[i][1], &num, sizeof(num));
                }
            }

            // Trimite semnalul de terminare (-1)
            int sentinel = -1;
            write(child_to_parent[i][1], &sentinel, sizeof(sentinel));
            close(child_to_parent[i][1]);

            return 0; // Finalizare proces copil
        } else { // === PROCES PĂRINȚI ===
            close(parent_to_child[i][0]);
            close(child_to_parent[i][1]);
        }
    }

    // === PROCES PĂRINȚI: Trimiterea intervalelor către copii ===
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int bounds[2] = { i * INTERVAL_SIZE + 1, (i + 1) * INTERVAL_SIZE };
        write(parent_to_child[i][1], bounds, sizeof(bounds));
        close(parent_to_child[i][1]); // Închide scrierea după trimitere
    }

    // === PROCES PĂRINȚI: Citirea și afișarea numerelor prime primite ===
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        int prime;
        std::cout << "\n============================================\n";
        std::cout << " Procesul Copil " << (i + 1) << " | Interval [" 
                  << (i * INTERVAL_SIZE + 1) << " - " << ((i + 1) * INTERVAL_SIZE) << "]\n";
        std::cout << "============================================\n";

        while (read(child_to_parent[i][0], &prime, sizeof(prime)) > 0) {
            if (prime == -1) break; // Semnal de final din partea copilului
            std::cout << prime << " ";
        }
        std::cout << "\n";
        close(child_to_parent[i][0]);
    }

    // Așteaptă terminarea tuturor proceselor copil
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        wait(NULL);
    }

    std::cout << "\n[SUCCESS] Toate cele 10 procese au procesat intervalele cu succes.\n";
    return 0;
}
