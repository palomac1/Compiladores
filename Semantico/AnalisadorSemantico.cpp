#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <unordered_map>

using namespace std;

// Classe base (Nó da Árvore)
class Node {
public:
    virtual ~Node() {}
    virtual void analisarSemantica(unordered_map<string, string>& tabelaSimbolos) = 0;
};

// Nó: Declaração de Variável
class NoDeclaracaoVariavel : public Node {
public:
    string nome;
    string tipo;

    NoDeclaracaoVariavel(string n, string t) : nome(n), tipo(t) {}

    void analisarSemantica(unordered_map<string, string>& tabelaSimbolos) override {
        if (tabelaSimbolos.count(nome)) {
            cout << "Erro semântico: Variável '" << nome << "' já declarada." << endl;
        } else {
            tabelaSimbolos[nome] = tipo;
        }
    }
};

// Nó: Uso de Variável
class NoUsoVariavel : public Node {
public:
    string nome;

    NoUsoVariavel(string n) : nome(n) {}

    void analisarSemantica(unordered_map<string, string>& tabelaSimbolos) override {
        if (!tabelaSimbolos.count(nome)) {
            cout << "Erro semântico: Variável '" << nome << "' não declarada antes do uso." << endl;
        }
    }
};

// Pilha de ponteiros (elo único, simples)
struct PilhaNode {
    Node* dado;
    PilhaNode* proximo;

    PilhaNode(Node* n, PilhaNode* p) : dado(n), proximo(p) {}
};

class Pilha {
private:
    PilhaNode* topo;
public:
    Pilha() : topo(nullptr) {}

    void push(Node* n) {
        topo = new PilhaNode(n, topo);
    }

    Node* pop() {
        if (!topo) return nullptr;
        Node* temp = topo->dado;
        PilhaNode* antigoTopo = topo;
        topo = topo->proximo;
        delete antigoTopo;
        return temp;
    }

    bool isEmpty() {
        return topo == nullptr;
    }
};

// Função para verificar se é uma palavra-chave de tipo (exemplo simples)
bool isTipoValido(const string& palavra) {
    return palavra == "integer" || palavra == "boolean" || palavra == "double";
}

int main() {
    unordered_map<string, string> tabelaSimbolos; // Tabela de símbolos
    Pilha pilha; // Pilha de nós (max 3 ponteiros ativos por vez)

    ifstream arquivo("C:\\Users\\palom\\OneDrive\\Documentos\\AnalisadorLexico\\teste.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo de entrada." << endl;
        return 1;
    }

    string linha;
    while (getline(arquivo, linha)) {
        size_t posVar = linha.find("var ");
        if (posVar != string::npos) {
            // Parse da declaração
            size_t nomeInicio = posVar + 4;
            size_t nomeFim = linha.find(":", nomeInicio);
            if (nomeFim != string::npos) {
                string nomeVar = linha.substr(nomeInicio, nomeFim - nomeInicio);
                nomeVar.erase(remove(nomeVar.begin(), nomeVar.end(), ' '), nomeVar.end()); // Remove espaços

                size_t tipoInicio = nomeFim + 1;
                size_t tipoFim = linha.find(";", tipoInicio);
                string tipoVar = linha.substr(tipoInicio, tipoFim - tipoInicio);
                tipoVar.erase(remove(tipoVar.begin(), tipoVar.end(), ' '), tipoVar.end());

                if (isTipoValido(tipoVar)) {
                    pilha.push(new NoDeclaracaoVariavel(nomeVar, tipoVar));
                } else {
                    cout << "Erro semântico: Tipo inválido para variável '" << nomeVar << "'." << endl;
                }
            }
        } else {
            // Considera qualquer linha com ":=" como um uso de variável
            size_t posAtribuicao = linha.find(":=");
            if (posAtribuicao != string::npos) {
                string nomeVar = linha.substr(0, posAtribuicao);
                nomeVar.erase(remove(nomeVar.begin(), nomeVar.end(), ' '), nomeVar.end());
                pilha.push(new NoUsoVariavel(nomeVar));
            }
        }
    }

    // Processa toda a pilha
    while (!pilha.isEmpty()) {
        Node* no = pilha.pop();
        if (no) {
            no->analisarSemantica(tabelaSimbolos);
            delete no;
        }
    }

    arquivo.close();
    return 0;
}
