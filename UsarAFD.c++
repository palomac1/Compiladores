#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <iomanip>

using namespace std;

// Função para verificar se um caractere é um símbolo válido 
bool isSymbol(char c) {
    string symbols = "+-*/:=<>{}();.,[]^";
    return symbols.find(c) != string::npos;
}

// Função para verificar se um caractere é válido (não é um símbolo isolado inválido)
bool isValidChar(char c) {
    return isSymbol(c) || isalnum(c) || isspace(c) || c == '\'';
}

// Função para determinar se um símbolo é simples ou composto
string getTipoSimbolo(const string& simbolo) {
    unordered_set<string> simbolosCompostos = { ":=", "<=", ">=", "<>", "==" };
    
    if (simbolosCompostos.count(simbolo)) {
        return "Simbolo composto";
    } else {
        return "Simbolo simples";
    }
}

int main() {
    // Abre o arquivo de entrada
    ifstream arquivo("teste.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo." << endl;
        return 1;
    }

    // Cria o arquivo de saída
    ofstream saida("saida.txt");
    if (!saida.is_open()) {
        cout << "Erro ao criar o arquivo de saída." << endl;
        return 1;
    }

    // Conjunto de palavras reservadas 
    unordered_set<string> palavrasReservadas = {
        "Program", "var", "function", "begin", "end", "read", "write",
        "if", "then", "else", "integer", "boolean", "double", "while",
        "procedure", "for", "do", "not", "and", "or", "to", "downto"
    };

    string linha; // Variável para armazenar cada linha do arquivo
    int numeroLinha = 0; // Contador de linhas
    
    // Configuração da tabela
    const int lexemaWidth = 20; // Largura da coluna "Lexema"
    const int tipoWidth = 25; // Largura da coluna "Tipo"

    // Cabeçalho do arquivo de saída (com linha de separação)
    saida << left << setw(lexemaWidth) << "Lexema" << setw(tipoWidth) << "Tipo" << endl;
    saida << string(lexemaWidth + tipoWidth, '-') << endl;  // Linha de separação

    // Cabeçalho do console (com linha e número da linha)
    cout << left << setw(lexemaWidth) << "Lexema" << setw(tipoWidth) << "Tipo" << "Linha" << endl;
    cout << string(lexemaWidth + tipoWidth + 10, '-') << endl;

    // Lê o arquivo linha por linha
    while (getline(arquivo, linha)) {
        numeroLinha++;
        string token;

        // Remove espaços em branco do início e do fim da linha
        for (size_t i = 0; i < linha.size(); ++i) {
            char c = linha[i]; // Caractere atual

            // Verifica se o caractere é inválido (símbolo isolado não reconhecido)
            if (!isValidChar(c)) {
                cout << "ERRO LÉXICO: Símbolo inválido '" << c << "' na linha " << numeroLinha << endl;
                saida << "ERRO LÉXICO: Símbolo inválido '" << c << "' na linha " << numeroLinha << endl;
                continue;
            }

            // Verifica se o caractere é um espaço em branco ou um símbolo
            // Se for espaço em branco, processa o token atual se não estiver vazio
            // Se for símbolo, processa o token atual e adiciona o símbolo à saída
            if (isspace(c)) {
                if (!token.empty()) {
                    string tipo;
                    if (palavrasReservadas.count(token)) {
                        tipo = "Palavra reservada";
                    } else if (all_of(token.begin(), token.end(), ::isdigit)) {
                        tipo = "Numero";
                    } else if (isalpha(token[0])) {
                        tipo = "Identificador";
                    }
                    
                    // Se o token não for vazio, printa e grava na saída
                    if (!tipo.empty()) {
                        cout << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << numeroLinha << endl;
                        saida << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << endl;  // Sem "Linha"
                    }
                    token.clear();
                }
            }
            else if (c == '{') { // Verifica se o caractere é '{' e ignora o comentário
                while (i < linha.size() && linha[i] != '}') { 
                    ++i;
                }
            }
            else if (c == '/') { // Verifica se o caractere é '/' e ignora o comentário de linha
                if (i + 1 < linha.size() && linha[i + 1] == '/') {
                    break;
                }
            }
            else if (c == '\'') { // Verifica se o caractere é "'" e ignora a string entre aspas simples
                while (i < linha.size() && linha[i] != '\'') {
                    ++i;
                }
            }
            // Verifica se o caractere é um símbolo, se for, processa o token atual e adiciona o símbolo à saída
            // Se não for símbolo, adiciona o caractere ao token atual
            else if (isSymbol(c)) {
                if (!token.empty()) {
                    string tipo;
                    if (palavrasReservadas.count(token)) {
                        tipo = "Palavra reservada";
                    } else if (all_of(token.begin(), token.end(), ::isdigit)) {
                        tipo = "Numero";
                    } else if (isalpha(token[0])) {
                        tipo = "Identificador";
                    }
                    
                    if (!tipo.empty()) {
                        cout << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << numeroLinha << endl;
                        saida << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << endl;
                    }
                    token.clear();
                }

                // Processa o símbolo e adiciona à saída para o console e o arquivo
                // Verifica se o símbolo é parte de um operador de dois caracteres (como :=, <=, >=, <>)
                string simbolo(1, c);
                if ((c == ':' || c == '<' || c == '>') && i + 1 < linha.size()) {
                    char prox = linha[i + 1];
                    if (prox == '=' || (c == '<' && prox == '>')) {
                        simbolo += prox;
                        ++i;
                    }
                }
                
                // Determina se o símbolo é simples ou composto
                string tipoSimbolo = getTipoSimbolo(simbolo);
                cout << left << setw(lexemaWidth) << simbolo << setw(tipoWidth) << tipoSimbolo << numeroLinha << endl;
                saida << left << setw(lexemaWidth) << simbolo << setw(tipoWidth) << tipoSimbolo << endl;
            }
            else {
                token += c;
            }
        }

        // Processa o último token da linha, se houver
        if (!token.empty()) {
            string tipo;
            if (palavrasReservadas.count(token)) {
                tipo = "Palavra reservada";
            } else if (all_of(token.begin(), token.end(), ::isdigit)) {
                tipo = "Numero";
            } else if (isalpha(token[0])) {
                tipo = "Identificador";
            }
            
            // Se o token não for vazio, printa e grava na saída
            if (!tipo.empty()) {
                cout << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << numeroLinha << endl;
                saida << left << setw(lexemaWidth) << token << setw(tipoWidth) << tipo << endl;
            }
        }
    }

    arquivo.close();
    saida.close();
    return 0;
}