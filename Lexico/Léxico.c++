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

// Função para determinar se um símbolo é simples ou composto
string getTipoSimbolo(const string& simbolo) {

    unordered_set<string> simbolosCompostos = { ":=", "<=", ">=", "<>", "==", ".." };

    if (simbolosCompostos.count(simbolo)) {
        return "Simbolo composto";
    } else {
        return "Simbolo simples";
    }
}

int main() {
    // Abre o arquivo de entrada
    ifstream arquivo("C:\\Users\\fortu\\Downloads\\Unespar\\Unespar\\Unespar-3ano\\Compiladores\\1Bim\\teste.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo. Verifique o caminho: C:\\Users\\fortu\\Downloads\\Unespar\\Unespar\\Unespar-3ano\\Compiladores\\1Bim\\teste.txt" << endl;
        return 1;
    }

    // Cria o arquivo de saída
    ofstream saida("C:\\Users\\fortu\\Downloads\\Unespar\\Unespar\\Unespar-3ano\\Compiladores\\1Bim\\saida.txt");
    if (!saida.is_open()) {
        cout << "Erro ao criar o arquivo de saída. Verifique as permissões de escrita: C:\\Users\\fortu\\Downloads\\Unespar\\Unespar\\Unespar-3ano\\Compiladores\\1Bim\\saida.txt" << endl;
        return 1;
    }

    // Conjunto de palavras reservadas, agora com 'label', 'type', 'array', 'of' e 'real'
    unordered_set<string> palavrasReservadas = {
        "program", "Program", "var", "function", "begin", "end", "read", "write",
    "if", "then", "else", "integer", "boolean", "double", "while",
    "procedure", "goto", "for", "do", "not", "and", "or", "to", "downto",
    "label", "type", "array", "of", "real", "div", "mod" 
    };

    string linha; // Variável para armazenar cada linha do arquivo
    int numeroLinha = 0; // Contador de linhas

    // Configuração da tabela para impressão no console e arquivo
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
        string token_buffer; 
        
        // Adiciona um espaço no final da linha para garantir que o último token seja processado.
        linha += ' '; 

        for (size_t i = 0; i < linha.size(); ++i) {
            char c = linha[i]; // Caractere atual

            // Ignora comentários de bloco {}
            if (c == '{') {
                while (i < linha.size() && linha[i] != '}') { 
                    ++i;
                }
                continue; // Continua para o próximo caractere após o fim do comentário
            }

            // Ignora comentários de bloco (* ... *)
            if (c == '(' && i + 1 < linha.size() && linha[i + 1] == '*') {
                // Avança o índice para depois do '(*'
                i += 2; 
                // Loop para encontrar o fechamento do comentário '*)'
                while (i < linha.size()) {
                    if (linha[i] == '*' && i + 1 < linha.size() && linha[i+1] == ')') {
                        i++; // Pula o ')' para continuar a análise após o comentário
                        break;
                    }
                    i++;
                }
                continue; // Continua para o próximo caractere após o fim do comentário
            }

            // Ignora comentários de linha //
            if (c == '/') {
                if (i + 1 < linha.size() && linha[i + 1] == '/') {
                    break; // Sai do loop da linha, ignorando o resto da linha
                }
            }
            // Ignora strings entre aspas simples ''
            if (c == '\'') {
                // Processa qualquer token antes da string (se houver)
                if (!token_buffer.empty()) {
                    string tipo;
                    if (palavrasReservadas.count(token_buffer)) {
                        tipo = "Palavra reservada";
                    } else if (all_of(token_buffer.begin(), token_buffer.end(), ::isdigit)) {
                        tipo = "Numero";
                    } else if (isalpha(token_buffer[0])) {
                        tipo = "Identificador";
                    }
                    if (!tipo.empty()) { // Apenas imprime se o token for reconhecido
                        cout << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << numeroLinha << endl;
                        saida << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << endl;
                    }
                    token_buffer.clear();
                }
                // Avança o índice para pular a string literal sem processá-la como tokens
                i++; // Pula a primeira aspa
                while (i < linha.size() && linha[i] != '\'') {
                    i++;
                }
                continue; 
            }

            // Se for espaço em branco, processa o token atual se não estiver vazio
            if (isspace(c)) {
                if (!token_buffer.empty()) {
                    string tipo;
                    if (palavrasReservadas.count(token_buffer)) {
                        tipo = "Palavra reservada";
                    } else if (all_of(token_buffer.begin(), token_buffer.end(), ::isdigit)) {
                        tipo = "Numero";
                    } else if (!token_buffer.empty() && isalpha(token_buffer[0])) { 
                        tipo = "Identificador";
                    }
                    
                    if (!tipo.empty()) { // Apenas imprime se o token for reconhecido
                        cout << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << numeroLinha << endl;
                        saida << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << endl;
                    }
                    token_buffer.clear();
                }
            }
            // Se for um símbolo, processa o token_buffer atual e depois o símbolo
            else if (isSymbol(c)) {
                if (!token_buffer.empty()) {
                    string tipo;
                    if (palavrasReservadas.count(token_buffer)) {
                        tipo = "Palavra reservada";
                    } else if (all_of(token_buffer.begin(), token_buffer.end(), ::isdigit)) {
                        tipo = "Numero";
                    } else if (isalpha(token_buffer[0])) {
                        tipo = "Identificador";
                    }

                    if (!tipo.empty()) { // Apenas imprime se o token for reconhecido
                        cout << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << numeroLinha << endl;
                        saida << left << setw(lexemaWidth) << token_buffer << setw(tipoWidth) << tipo << endl;
                    }
                    token_buffer.clear();
                }

                // Processa o símbolo (potencialmente composto, como ':=', '<=', '..')
                string simbolo_str(1, c);
                if (i + 1 < linha.size()) {
                    char prox_c = linha[i + 1];
                    string possivel_composto = string(1, c) + prox_c;

                    // Lista de símbolos compostos que devem ser tratados como uma única unidade léxica
                    unordered_set<string> simbolosCompostosDuplos = { ":=", "<=", ">=", "<>", "==", ".." };

                    if (simbolosCompostosDuplos.count(possivel_composto)) {
                        simbolo_str = possivel_composto;
                        ++i; // Avança o índice para consumir o segundo caractere do símbolo composto
                    }
                }
                
                string tipoSimbolo = getTipoSimbolo(simbolo_str);
                cout << left << setw(lexemaWidth) << simbolo_str << setw(tipoWidth) << tipoSimbolo << numeroLinha << endl;
                saida << left << setw(lexemaWidth) << simbolo_str << setw(tipoWidth) << tipoSimbolo << endl;
            }
            // Se não for espaço nem símbolo, adiciona o caractere ao buffer do token
            else {
                token_buffer += c;
            }
        }
    }

    arquivo.close();
    saida.close();
    return 0;
}
