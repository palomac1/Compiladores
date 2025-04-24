#include <iostream>
#include <fstream> //Lê o arquivo
#include <string>
#include <cctype> //Retorna se é um char(letra ou numero)

int main() {
    std::ifstream file("C:\\Users\\palom\\OneDrive\\Documentos\\AnalisadorLexico\\teste.txt"); // Abre o arquivo
    if (!file.is_open()) { 
        std::cerr << "Erro ao abrir o arquivo." << std::endl;  // Verifica se o arquivo foi aberto corretamente
        return 1;
    }

    std::string linha; // Variável para armazenar a linha lida
    std::string palavras; // Variável para armazenar todas as palavras concatenadas
    std::string palavra; // Variável para armazenar a palavra lida
    string simbSimples =  {"+", "-", "*", ":", "{", "}", ";", ",", ".", "=", "<", ">", "[", "]", "^"}; // Símbolos simples

    // Lê o arquivo linha por linha
    while (std::getline(file, linha)) {
        palavras += linha + " "; // Concatena as linhas em uma única string
    }

    std::cout << "Conteúdo completo: " << palavras << std::endl;

    // Lê o arquivo palavra por palavra
    while (file >> palavra) { 

        // Verifica se a palavra é um número
        if (std::all_of(palavra.begin(), palavra.end(), ::isdigit)) {
            std::cout << "Número: " << palavra << std::endl;
        }

        //Verifica se a palavra é um simbolo simples
        for (const auto& simb : simbSimples) { // Itera sobre os símbolos simples para verificar se estão na palavra
            if (palavra.find(simb) != std::string::npos) {// Verifica se a palavra contém o símbolo simples
                std::cout << "Símbolo simples: " << simb << std::endl;
            } 
        }

        // Verifica se a palavra é uma letra
        if (std::all_of(palavra.begin(), palavra.end(), ::isalpha)) {
            std::cout << "Letra: " << palavra << std::endl;
        }

        // Verifica se a palavra é um identificador
        if (std::isalpha(palavra[0]) && std::all_of(palavra.begin() + 1, palavra.end(), ::isalnum)) {
            std::cout << "Identificador: " << palavra << std::endl;
        }
        
    }

    // Verifica se a palavra é uma palavra reservada
    if(palavra == "Program" || palavra == "read" || palavra == "write" || palavra == "integer" || palavra == "boolean" || palavra == "double" || palavra == "function" || palavra == "procedure" || palavra == "begin"
    || palavra == "end" || palavra == "and" || palavra == "array" || palavra == "case" || palavra == "const" || palavra == "div" || palavra == "do" || palavra == "downto" || palavra == "else" || palavra == "file"
    || palavra == "for" || palavra == "goto" || palavra == "if" || palavra == "in" || palavra == "label" || palavra == "mode" || palavra == "nil" || palavra == "not" || palavra == "of" || palavra == "or"
    || palavra == "packed" || palavra == "record" || palavra == "repeat" || palavra == "set" || palavra == "then" || palavra == "to" || palavra == "type" || palavra == "until" || palavra == "with" || palavra == "var" 
    || palavra == "while") {
        std::cout << "Palavra reservada: " << palavra << std::endl;
    }


    if( string simbSimples =  {"+", "-", ":", "{", "}", ";", ",", ".", "=", "<", ">", "[", "]", "^"}){


    }

    file.close();
    return 0;        

}
