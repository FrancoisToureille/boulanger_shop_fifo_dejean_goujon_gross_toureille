#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>

using namespace std;
// 3 clients dans la file
const int KmaxClientsFile = 3;
const int KnombreClientsJournee = 12;

mutex mtx;
condition_variable boulangerCondition;

vector<thread> clients;
queue<int> fileAttente;

bool boulangerTravailleActuellement = false;
bool finDeLaJournee = false;

void clientRentre(int idClient) {
    cout << "Client " << idClient << " arrive mais la boulangerie est pleine a craquer, il rentre chez lui." << endl;

}

void clientThread(int idClient) {
    unique_lock<mutex> lock(mtx);

    // Si la boulangerie n'est pas pleine, le client attend dans la file d'attente.
    if (fileAttente.size() < KmaxClientsFile) {
        fileAttente.push(idClient);
        cout << "Client " << idClient << " arrive a la boulangerie. Clients en attente de prise de commandes : " << fileAttente.size() << endl;
    } else {
        // la boulangerie est pleine et le client part
        clientRentre(idClient);
        return;
    }

    if (!boulangerTravailleActuellement) {
        //le boulanger doit recevoir le premier client de la file
        boulangerCondition.notify_one();
    }
}

void boulangerThread() {
    //la boucle se répète jusqu'à ce que tous les clients de la journée soient passés à la boulangerie
    while (true) {
        //le boulanger ne peut traiter qu'un client à la fois
        unique_lock<mutex> lock(mtx);

        while (fileAttente.empty() && !finDeLaJournee && !boulangerTravailleActuellement) {
            cout << "Le boulanger est libre." << endl;
            //le boulanger attend l'unlock et le premier client
            boulangerCondition.wait(lock);
        }

        if (finDeLaJournee) {
            break;
        }

        int client_id;
        bool abscenceClient = false;
        // s'il y a un client à servir, on le traite et on le retire de la file d'attente
        if (!fileAttente.empty()) {
            client_id = fileAttente.front();
            fileAttente.pop();
        }
            // file vide pour le moment
        else {
            abscenceClient = true;
        }

        boulangerTravailleActuellement = true;
        //unlock pour permettre l'écoute de la commande
        lock.unlock();

        if (!abscenceClient) {
            cout << "Le boulanger ecoute la demande du client " << client_id << endl;
        }
        this_thread::sleep_for(chrono::seconds (3));
        // lock pour la mise à jour de l'état du boulanger
        lock.lock();
        //fin du traitement du client, le boulanger peut passer au suivant
        boulangerTravailleActuellement = false;
        if (!abscenceClient) {
            cout << "Le boulanger a termine de servir le client " << client_id << ", le client sort" << endl;
        }

        if (!fileAttente.empty()) {
            //prevenir le client suivant
            boulangerCondition.notify_one();
        }
    }
}

int main() {
    thread boulanger(boulangerThread);
    srand(time(nullptr));


    for (int i = 1; i <= KnombreClientsJournee; i++) {
        //on lance le thread du client et on l'ajoute dans le vecteur des threads clients
        clients.emplace_back(clientThread, i);
        int waitTime = (rand() % 3 + 1); // Attente aléatoire entre 1 et 3 secondes
        this_thread::sleep_for(chrono::seconds (waitTime));  // Les clients n'arrivent pas tous d'un coup
    }

    for (auto& client : clients) {
        //on attend la fin du thread de chaque client
        client.join();
    }

    unique_lock<mutex> lock(mtx);
    while (!fileAttente.empty()) {
        boulangerCondition.wait(lock);
    }

    finDeLaJournee = true;
    //dire au thread boulanger de sortir du while true
    boulangerCondition.notify_one();
    lock.unlock();
    //on attend la fin du thread du boulanger
    boulanger.join();

    return 0;
}
