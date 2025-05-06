#include <iostream>
#include <fstream> //Lê o arquivo
#include <string>
#include <cctype> //Retorna se é um char(letra ou numero)

using namespace std;

int main(){

    ifstream arquivo("teste.txt");
    if (!arquivo.is_open()) { // Verifica se o arquivo foi aberto corretamente
        cout << "Erro ao abrir o arquivo." << endl;
        return 1;
    }

    string linha;
    while (getline(arquivo, linha)) { // Lê cada linha do arquivo
        for (char c : linha) { // Itera sobre cada caractere da linha
            if (isalpha(c)) { // Verifica se é uma letra
                cout << c; // Imprime a letra
            }
        }
        cout << endl; // Nova linha após cada linha do arquivo
    }

    arquivo.close(); // Fecha o arquivo
    return 0;

}

