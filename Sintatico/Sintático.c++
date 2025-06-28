#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>

using namespace std;

// Define a estrutura para um token léxico.
struct Token {
    string lexema;
    string tipo;
    int linha;
};

// Define a estrutura para um nó da Árvore Sintática Abstrata (AST).
struct ASTNode {
    string tipo;
    string valor;
    vector<ASTNode*> filhos;

    // Construtor para criar um novo nó da AST.
    ASTNode(string t, string v = "") : tipo(t), valor(v) {}

    // Destrutor para liberar a memória dos nós filhos recursivamente.
    ~ASTNode() {
        for (ASTNode* filho : filhos) {
            delete filho;
        }
    }
};

vector<Token> tokens; // Armazena tokens lidos.
int pos = 0;          // Posição atual no vetor de tokens.
set<string> declaredLabels; // Armazena rótulos declarados.

// Tabela de Símbolos: mapeia identificadores para seus tipos.
map<string, string> TabelaSimbolos;

// Protótipos das funções para permitir chamadas recursivas mútuas.
ASTNode* programa();
ASTNode* bloco();
ASTNode* declaracao_rotulos();
ASTNode* declaracao_tipos();
ASTNode* declaracao_variaveis();
ASTNode* lista_identificadores();
ASTNode* tipo();
ASTNode* declaracao_funcao();
ASTNode* declaracao_procedimento();
ASTNode* parametros();
ASTNode* lista_comandos();
ASTNode* comando();
ASTNode* variavel();
ASTNode* expressao();
ASTNode* termo_logico();
ASTNode* expressao_relacional();
ASTNode* expressao_aritmetica();
ASTNode* termo_aritmetico();
ASTNode* fator();
ASTNode* chamada_subrotina();
ASTNode* lista_argumentos();

// Retorna o token atual.
Token atual() {
    if (pos < (int)tokens.size()) return tokens[pos];
    return {"", "FIM_DE_ARQUIVO", -1};
}

// Verifica se todos os tokens foram consumidos.
bool fimTokens() {
    return pos >= (int)tokens.size();
}

// Imprime um erro sintático e encerra o programa.
ASTNode* syntaxError(const string& mensagem) {
    if (!fimTokens()) { // Se o erro não é no final do arquivo.
        cout << "Erro na linha " << atual().linha << ": " << mensagem << ". Token encontrado: '" << atual().lexema << "' do tipo '" << atual().tipo << "'\n";
    } else { // Se o erro é no final do arquivo.
        cout << "Erro: " << mensagem << " no final do arquivo.\n";
    }
    exit(1); // Sai do programa.
    return nullptr;
}

// Consome o token atual se corresponder ao esperado.
Token expect(const string& esperado, bool isType = false) {
    if (fimTokens()) return {"", "", -1}; // Nada a esperar no fim.
    if (isType) { // Compara pelo tipo.
        if (atual().tipo == esperado) {
            return tokens[pos++]; // Retorna token e avança.
        }
    } else { // Compara pelo lexema.
        if (atual().lexema == esperado) {
            return tokens[pos++]; // Retorna token e avança.
        }
    }
    return {"", "", -1}; // Não encontrou o token.
}

// Analisa a regra de produção para 'programa'.
ASTNode* programa() {
    if (expect("Program").lexema.empty()) { // Espera "Program".
        return syntaxError("um programa deve comecar com a palavra-chave 'Program'");
    }
    Token idToken = expect("Identificador", true); // Espera o nome do programa.
    if (idToken.lexema.empty()) {
        return syntaxError("esperado um nome de identificador para o programa");
    }
    string nomePrograma = idToken.lexema; // Armazena o nome.

    if (atual().lexema == "(") { // Se houver parênteses para parâmetros.
        pos++; // Consome '('.
        while (atual().lexema != ")" && !fimTokens()) { // Processa identificadores e vírgulas.
            if(expect("Identificador", true).lexema.empty()) {
                return syntaxError("esperado identificador na lista de parametros do programa");
            }
            if(atual().lexema == ",") pos++; // Consome a vírgula.
        }
        if (expect(")").lexema.empty()) { // Espera ')'.
            return syntaxError("esperado ')' para fechar a lista de parametros do programa");
        }
    }

    if (expect(";").lexema.empty()) { // Espera ';'.
        return syntaxError("esperado ';' apos o cabecalho do programa");
    }
    ASTNode* noBloco = bloco(); // Analisa o bloco do programa.
    if (!noBloco) return nullptr;

    if (expect(".").lexema.empty()) { // Espera '.'.
        return syntaxError("esperado '.' no final do programa");
    }

    if (atual().tipo != "FIM_DE_ARQUIVO" && !fimTokens()) { // Verifica tokens extras no fim.
        return syntaxError("tokens inesperados '" + atual().lexema + "' apos o final do programa");
    }

    ASTNode* noPrograma = new ASTNode("programa", nomePrograma); // Cria o nó AST do programa.
    noPrograma->filhos.push_back(noBloco); // Adiciona o bloco como filho.
    return noPrograma;
}

// Analisa a regra de produção para 'bloco'.
ASTNode* bloco() {
    ASTNode* noBloco = new ASTNode("bloco"); // Cria o nó AST para o bloco.
    if (atual().lexema == "label") { // Se há declaração de rótulos.
        noBloco->filhos.push_back(declaracao_rotulos());
    }
    if (atual().lexema == "type") { // Se há declaração de tipos.
        noBloco->filhos.push_back(declaracao_tipos());
    }
    if (atual().lexema == "var") { // Se há declaração de variáveis.
        noBloco->filhos.push_back(declaracao_variaveis());
    }
    while (atual().lexema == "function" || atual().lexema == "procedure") { // Processa funções/procedimentos.
        ASTNode* noSubRotina = nullptr;
        if (atual().lexema == "function") {
            noSubRotina = declaracao_funcao(); // Analisa função.
        } else {
            noSubRotina = declaracao_procedimento(); // Analisa procedimento.
        }
        if (!noSubRotina) return nullptr;
        noBloco->filhos.push_back(noSubRotina); // Adiciona a sub-rotina.
    }
    if (expect("begin").lexema.empty()) { // Espera "begin".
        return syntaxError("esperado 'begin' para iniciar o bloco de comandos");
    }
    noBloco->filhos.push_back(lista_comandos()); // Analisa a lista de comandos.

    if (expect("end").lexema.empty()) { // Espera "end".
        return syntaxError("esperado 'end' para finalizar o bloco");
    }
    return noBloco;
}

// Analisa a regra de produção para 'declaracao_rotulos'.
ASTNode* declaracao_rotulos() {
    expect("label"); // Consome "label".
    ASTNode* node = new ASTNode("declaracao_rotulos"); // Cria o nó.
    do {
        Token numToken = expect("Numero", true); // Espera um número de rótulo.
        if (numToken.lexema.empty()) return syntaxError("esperado um numero de rotulo na declaracao 'label'");
        declaredLabels.insert(numToken.lexema); // Insere o rótulo no conjunto.
        node->filhos.push_back(new ASTNode("rotulo", numToken.lexema)); // Adiciona o nó do rótulo.
        if (atual().lexema != ",") break; // Sai se não houver vírgula.
        pos++; // Consome a vírgula.
    } while (true);
    if (expect(";").lexema.empty()) return syntaxError("esperado ';' para finalizar a declaracao de rotulos"); // Espera ';'.
    return node;
}

// Analisa a regra de produção para 'declaracao_tipos'.
ASTNode* declaracao_tipos() {
    expect("type"); // Consome "type".
    ASTNode* node = new ASTNode("declaracao_tipos"); // Cria o nó.
    while (atual().tipo == "Identificador") { // Processa declarações de tipo.
        Token idToken = expect("Identificador", true); // Espera identificador.
        TabelaSimbolos[idToken.lexema] = "tipo"; // Registra o tipo.
        if (expect("=").lexema.empty()) return syntaxError("esperado '=' na declaracao de tipo"); // Espera '='.
        ASTNode* noTipo = tipo(); // Analisa o tipo.
        if (!noTipo) return nullptr;
        ASTNode* declTipo = new ASTNode("declaracao_tipo", idToken.lexema); // Cria o nó de declaração.
        declTipo->filhos.push_back(noTipo); // Adiciona o tipo como filho.
        node->filhos.push_back(declTipo);   // Adiciona ao nó pai.
        if (expect(";").lexema.empty()) return syntaxError("esperado ';' apos cada declaracao de tipo"); // Espera ';'.
    }
    return node;
}

// Analisa a regra de produção para 'declaracao_variaveis'.
ASTNode* declaracao_variaveis() {
    expect("var"); // Consome "var".
    ASTNode* noVars = new ASTNode("declaracao_variaveis"); // Cria o nó.
    while (atual().tipo == "Identificador") { // Processa grupos de variáveis.
        ASTNode* noLista = lista_identificadores(); // Analisa a lista de identificadores.
        if (!noLista) return nullptr;
        for(auto& filho : noLista->filhos) { // Registra cada variável.
            TabelaSimbolos[filho->valor] = "variavel";
        }
        if (expect(":").lexema.empty()) return syntaxError("esperado ':' entre os nomes das variaveis e o seu tipo"); // Espera ':'.
        ASTNode* noTipo = tipo(); // Analisa o tipo.
        if (!noTipo) return nullptr;
        if (expect(";").lexema.empty()) return syntaxError("esperado ';' ao final da declaracao de variavel"); // Espera ';'.
        ASTNode* noDecl = new ASTNode("declaracao_variavel"); // Cria o nó de declaração.
        noDecl->filhos.push_back(noLista); // Adiciona a lista de identificadores.
        noDecl->filhos.push_back(noTipo);  // Adiciona o tipo.
        noVars->filhos.push_back(noDecl);  // Adiciona ao nó pai.
    }
    return noVars;
}

// Analisa a regra de produção para 'lista_identificadores'.
ASTNode* lista_identificadores() {
    ASTNode* noLista = new ASTNode("lista_identificadores"); // Cria o nó.
    do {
        Token idToken = expect("Identificador", true); // Espera um identificador.
        if (idToken.lexema.empty()) return syntaxError("esperado um identificador na lista");
        noLista->filhos.push_back(new ASTNode("identificador", idToken.lexema)); // Adiciona o nó do identificador.
        if (atual().lexema != ",") break; // Sai se não houver vírgula.
        pos++; // Consome a vírgula.
    } while (true);
    return noLista;
}

// Analisa a regra de produção para 'tipo'.
ASTNode* tipo() {
    string lex = atual().lexema; // Pega o lexema.
    if (lex == "integer" || lex == "boolean" || lex == "string" || lex == "real" || lex == "numero") { // Tipos primitivos.
        pos++; // Consome o token.
        return new ASTNode("tipo_primitivo", lex); // Retorna o nó do tipo primitivo.
    }
    if (lex == "array") { // Se é um array.
        pos++; // Consome "array".
        ASTNode* noArray = new ASTNode("tipo_array"); // Cria o nó do array.
        if (expect("[").lexema.empty()) return syntaxError("esperado '[' apos a palavra 'array'"); // Espera '['.
        
        Token inicio = expect("Numero", true); // Espera índice inicial.
        if(inicio.lexema.empty()) return syntaxError("esperado numero para indice inicial do array");
        noArray->filhos.push_back(new ASTNode("numero", inicio.lexema)); // Adiciona o índice.

        if (expect("..").lexema.empty()) return syntaxError("esperado '..' para separar os indices do array"); // Espera "..".
        
        Token fim = expect("Numero", true); // Espera índice final.
        if(fim.lexema.empty()) return syntaxError("esperado numero para indice final do array");
        noArray->filhos.push_back(new ASTNode("numero", fim.lexema)); // Adiciona o índice.

        if (expect("]").lexema.empty()) return syntaxError("esperado ']' apos os indices do array"); // Espera ']'.
        if (expect("of").lexema.empty()) return syntaxError("esperado 'of' na declaracao do array"); // Espera "of".
        ASTNode* tipoElem = tipo(); // Analisa o tipo dos elementos do array.
        if (!tipoElem) return nullptr;
        noArray->filhos.push_back(tipoElem); // Adiciona o tipo do elemento.
        return noArray;
    }
    if(atual().tipo == "Identificador") { // Se é um tipo definido pelo usuário.
        string idTipo = atual().lexema;
        pos++; // Consome o identificador.
        return new ASTNode("tipo_identificador", idTipo); // Retorna o nó do tipo identificador.
    }
    return syntaxError("esperado um tipo valido"); // Tipo inválido.
}

// Analisa a regra de produção para 'declaracao_funcao'.
ASTNode* declaracao_funcao() {
    expect("function"); // Consome "function".
    Token idToken = expect("Identificador", true); // Espera o nome da função.
    if (idToken.lexema.empty()) return syntaxError("esperado um nome de identificador para a funcao");
    TabelaSimbolos[idToken.lexema] = "funcao"; // Registra como função.
    ASTNode* noFunc = new ASTNode("funcao", idToken.lexema); // Cria o nó da função.
    ASTNode* noParams = nullptr; // Inicializa parâmetros.
    if (atual().lexema == "(") { // Se há parâmetros.
        pos++; // Consome '('.
        if (atual().lexema != ")") noParams = parametros(); // Analisa parâmetros.
        if (expect(")").lexema.empty()) return syntaxError("esperado ')' para fechar a lista de parametros"); // Espera ')'.
    }
    if (expect(":").lexema.empty()) return syntaxError("esperado ':' antes do tipo de retorno da funcao"); // Espera ':'.
    ASTNode* noTipoRet = tipo(); // Analisa o tipo de retorno.
    if (!noTipoRet) return nullptr;
    if (expect(";").lexema.empty()) return syntaxError("esperado ';' apos a assinatura da funcao"); // Espera ';'.
    ASTNode* noBloco = bloco(); // Analisa o bloco da função.
    if (!noBloco) return nullptr;
    if (expect(";").lexema.empty()) return syntaxError("esperado ';' apos o 'end' do bloco da funcao"); // Espera ';'.

    noFunc->filhos.push_back(noParams ? noParams : new ASTNode("parametros_vazios")); // Adiciona parâmetros.
    noFunc->filhos.push_back(noTipoRet); // Adiciona tipo de retorno.
    noFunc->filhos.push_back(noBloco); // Adiciona o bloco.
    return noFunc;
}

// Analisa a regra de produção para 'declaracao_procedimento'.
ASTNode* declaracao_procedimento() {
    expect("procedure"); // Consome "procedure".
    Token idToken = expect("Identificador", true); // Espera o nome do procedimento.
    if (idToken.lexema.empty()) return syntaxError("esperado um nome de identificador para o procedimento");
    TabelaSimbolos[idToken.lexema] = "procedimento"; // Registra como procedimento.
    ASTNode* noProc = new ASTNode("procedimento", idToken.lexema); // Cria o nó do procedimento.
    ASTNode* noParams = nullptr; // Inicializa parâmetros.
    if (atual().lexema == "(") { // Se há parâmetros.
        pos++; // Consome '('.
        if (atual().lexema != ")") noParams = parametros(); // Analisa parâmetros.
        if (expect(")").lexema.empty()) return syntaxError("esperado ')' para fechar a lista de parametros"); // Espera ')'.
    }
    if (expect(";").lexema.empty()) return syntaxError("esperado ';' apos a assinatura do procedimento"); // Espera ';'.
    ASTNode* noBloco = bloco(); // Analisa o bloco do procedimento.
    if (!noBloco) return nullptr;
    if (expect(";").lexema.empty()) return syntaxError("esperado ';' apos o 'end' do bloco do procedimento"); // Espera ';'.

    noProc->filhos.push_back(noParams ? noParams : new ASTNode("parametros_vazios")); // Adiciona parâmetros.
    noProc->filhos.push_back(noBloco); // Adiciona o bloco.
    return noProc;
}

// Analisa a regra de produção para 'parametros'.
ASTNode* parametros() {
    ASTNode* noLista = new ASTNode("lista_parametros"); // Cria o nó.
    do {
        bool porReferencia = !expect("var").lexema.empty(); // Verifica se é por referência.
        ASTNode* idLista = lista_identificadores(); // Analisa a lista de identificadores.
        if(!idLista) return nullptr;
        if(expect(":").lexema.empty()) return syntaxError("esperado ':' na declaracao de parametro"); // Espera ':'.
        ASTNode* tipoParam = tipo(); // Analisa o tipo do parâmetro.
        if(!tipoParam) return nullptr;
        ASTNode* paramGroup = new ASTNode(porReferencia ? "grupo_parametro_ref" : "grupo_parametro_valor"); // Cria o grupo de parâmetros.
        paramGroup->filhos.push_back(idLista);  // Adiciona a lista de identificadores.
        paramGroup->filhos.push_back(tipoParam); // Adiciona o tipo.
        noLista->filhos.push_back(paramGroup); // Adiciona o grupo à lista.
        
        if (atual().lexema != ";") break; // Sai se não houver ';'.
        pos++; // Consome ';'.
    } while (true);
    return noLista;
}

// Analisa a regra de produção para 'lista_comandos'.
ASTNode* lista_comandos() {
    ASTNode* lista = new ASTNode("lista_comandos"); // Cria o nó.
    while (true) {
        if (atual().lexema == "end" || atual().lexema == "else" || atual().tipo == "FIM_DE_ARQUIVO" || fimTokens()) { // Condições de parada.
            break;
        }

        ASTNode* cmd = comando(); // Analisa um comando.
        if (!cmd) { // Se o comando é inválido.
            if (atual().lexema != "end" && atual().lexema != "else") {
                return syntaxError("comando invalido ou inesperado");
            }
            break;
        }
        lista->filhos.push_back(cmd); // Adiciona o comando à lista.

        if (atual().lexema == ";") { // Se há ';'.
            pos++; // Consome ';'.
            if (atual().lexema == "end") { // Se for "end" depois do ';'.
                break;
            }
        } else if (atual().lexema == "end" || atual().lexema == "else" || atual().tipo == "FIM_DE_ARQUIVO" || fimTokens()) { // Se for terminador.
            break;
        } else {
            return syntaxError("esperado ';' apos o comando ou um terminador de bloco (end/else)"); // Erro.
        }
    }
    return lista;
}

// Analisa a regra de produção para 'comando'.
ASTNode* comando() {
    ASTNode* noRotulo = nullptr;

    // Verifica e consome o rótulo (SE HOUVER).
    if (atual().tipo == "Numero" && static_cast<size_t>(pos) + 1 < tokens.size() && tokens[pos + 1].lexema == ":") {
        // Verifica se o rótulo foi previamente declarado na seção 'label'.
        if (declaredLabels.find(atual().lexema) == declaredLabels.end()) {
            return syntaxError("uso de rotulo '" + atual().lexema + "' que nao foi declarado na secao 'label'");
        }
        // Cria um nó na AST para representar o uso deste rótulo.
        noRotulo = new ASTNode("rotulo_uso", atual().lexema);
        pos += 2; // Consome o token do número (rótulo) e o token ':'.
    }

    // Analisa o comando real que vem a seguir (o "comando sem rótulo").
    ASTNode* noComandoReal = nullptr;
    string lex = atual().lexema;

    if (lex == "begin") {
        pos++; // Consome "begin".
        noComandoReal = lista_comandos();
        if (!noComandoReal) return nullptr;
        if (expect("end").lexema.empty()) return syntaxError("esperado 'end' apos bloco de comando aninhado");
    }
    else if (lex == "if") {
        pos++; // Consome "if".
        ASTNode* cond = expressao();
        if (!cond) return nullptr;
        if (expect("then").lexema.empty()) return syntaxError("esperado 'then' apos a condicao do 'if'");
        ASTNode* noThen = comando();
        if (!noThen) return syntaxError("esperado um comando apos 'then'");
        
        noComandoReal = new ASTNode("if");
        noComandoReal->filhos.push_back(cond);
        noComandoReal->filhos.push_back(noThen);

        if (atual().lexema == "else") {
            pos++; // Consome "else".
            ASTNode* noElse = comando();
            if (!noElse) return syntaxError("esperado um comando apos 'else'");
            noComandoReal->filhos.push_back(noElse);
        }
    }
    else if (lex == "while") {
        pos++; // Consome "while".
        ASTNode* cond = expressao();
        if (!cond) return nullptr;
        if (expect("do").lexema.empty()) return syntaxError("esperado 'do' apos a condicao do 'while'");
        ASTNode* noCmd = comando();
        if (!noCmd) return nullptr;

        noComandoReal = new ASTNode("while");
        noComandoReal->filhos.push_back(cond);
        noComandoReal->filhos.push_back(noCmd);
    }
    else if (lex == "goto") {
        pos++; // Consome "goto".
        Token label = expect("Numero", true);
        if (label.lexema.empty()) return syntaxError("esperado um numero de rotulo para o 'goto'");
        if (declaredLabels.find(label.lexema) == declaredLabels.end()) {
            return syntaxError("uso de 'goto' para rotulo nao declarado: '" + label.lexema + "'");
        }
        noComandoReal = new ASTNode("goto", label.lexema);
    }
    else if (atual().tipo == "Identificador") { // Potencialmente uma atribuição ou chamada de procedimento.
        int backtrack_pos = pos;
        ASTNode* lhs = variavel();

        if (lhs) {
            if (atual().lexema == ":=") { // É uma atribuição.
                pos++; // Consome ":=".
                ASTNode* rhs = expressao();
                if (!rhs) return syntaxError("expressao invalida apos ':='");
                
                ASTNode* assignNode = new ASTNode("atribuicao");
                assignNode->filhos.push_back(lhs);
                assignNode->filhos.push_back(rhs);

                if (lhs->tipo == "identificador" && TabelaSimbolos[lhs->valor] == "funcao") {
                    assignNode->tipo = "retorno_funcao";
                }
                noComandoReal = assignNode;
            } else { // Não é atribuição, deve ser uma chamada de procedimento.
                pos = backtrack_pos;
                delete lhs; 
                noComandoReal = chamada_subrotina();
            }
        }
    }

    // Combina o resultado.
    if (noRotulo) {
        // Se encontrado um rótulo, ele deve ser seguido por um comando válido.
        if (!noComandoReal) {
            delete noRotulo; // Limpa a memória.
            return syntaxError("esperado um comando valido apos o rotulo '" + noRotulo->valor + ":'");
        }
        // Cria um nó pai para agrupar o rótulo e o comando.
        ASTNode* comandoComRotulo = new ASTNode("comando_com_rotulo");
        comandoComRotulo->filhos.push_back(noRotulo);
        comandoComRotulo->filhos.push_back(noComandoReal);
        return comandoComRotulo;
    } else {
        // Se não havia rótulo, retorna apenas o comando que foi analisado (ou nullptr se for inválido).
        return noComandoReal;
    }
}

// Analisa a regra de produção para 'variavel'.
ASTNode* variavel() {
    if (atual().tipo != "Identificador") { // Se não é identificador.
        return nullptr;
    }
    Token idToken = tokens[pos]; // Pega o identificador.
    pos++; // Consome o identificador.

    ASTNode* varNode = new ASTNode("identificador", idToken.lexema); // Cria o nó do identificador.

    if (atual().lexema == "[") { // Se é acesso a array.
        pos++; // Consome '['.
        ASTNode* indexExpr = expressao(); // Analisa a expressão do índice.
        if (!indexExpr) return syntaxError("expressao de indice de array invalida");
        if (expect("]").lexema.empty()) return syntaxError("esperado ']' para fechar o indice do array"); // Espera ']'.

        ASTNode* accessNode = new ASTNode("acesso_array", idToken.lexema); // Cria o nó de acesso a array.
        accessNode->filhos.push_back(varNode); // O filho é o nome do array.
        accessNode->filhos.push_back(indexExpr); // O outro filho é a expressão do índice.
        return accessNode;
    }

    return varNode; // Retorna o nó do identificador.
}

// Analisa a regra de produção para 'expressao' (lógica OR).
ASTNode* expressao() {
    ASTNode* noEsq = termo_logico(); // Analisa o termo lógico esquerdo.
    if (!noEsq) return nullptr;
    while (atual().lexema == "or") { // Enquanto houver "or".
        string op = atual().lexema; // Pega o operador.
        pos++; // Consome "or".
        ASTNode* noDir = termo_logico(); // Analisa o termo lógico direito.
        if (!noDir) return syntaxError("esperado expressao apos operador '" + op + "'");
        ASTNode* noExp = new ASTNode("operador_binario", op); // Cria o nó do operador binário.
        noExp->filhos.push_back(noEsq); // Adiciona o lado esquerdo.
        noExp->filhos.push_back(noDir); // Adiciona o lado direito.
        noEsq = noExp; // Atualiza o lado esquerdo.
    }
    return noEsq;
}

// Analisa a regra de produção para 'termo_logico' (lógica AND).
ASTNode* termo_logico() {
    ASTNode* noEsq = expressao_relacional(); // Analisa a expressão relacional esquerda.
    if (!noEsq) return nullptr;
    while (atual().lexema == "and") { // Enquanto houver "and".
        string op = atual().lexema; // Pega o operador.
        pos++; // Consome "and".
        ASTNode* noDir = expressao_relacional(); // Analisa a expressão relacional direita.
        if (!noDir) return syntaxError("esperado expressao apos operador '" + op + "'");
        ASTNode* noExp = new ASTNode("operador_binario", op); // Cria o nó do operador binário.
        noExp->filhos.push_back(noEsq); // Adiciona o lado esquerdo.
        noExp->filhos.push_back(noDir); // Adiciona o lado direito.
        noEsq = noExp; // Atualiza o lado esquerdo.
    }
    return noEsq;
}

// Analisa a regra de produção para 'expressao_relacional'.
ASTNode* expressao_relacional() {
    ASTNode* noEsq = expressao_aritmetica(); // Analisa a expressão aritmética esquerda.
    if (!noEsq) return nullptr;
    static const set<string> operadoresRelacionais = {"=", "<>", "<", "<=", ">", ">="}; // Define operadores relacionais.
    if (operadoresRelacionais.count(atual().lexema)) { // Se o token atual é um operador relacional.
        string op = atual().lexema; // Pega o operador.
        pos++; // Consome o operador.
        ASTNode* noDir = expressao_aritmetica(); // Analisa a expressão aritmética direita.
        if (!noDir) return syntaxError("esperado expressao apos operador relacional '" + op + "'");
        ASTNode* noExp = new ASTNode("operador_binario", op); // Cria o nó do operador binário.
        noExp->filhos.push_back(noEsq); // Adiciona o lado esquerdo.
        noExp->filhos.push_back(noDir); // Adiciona o lado direito.
        return noExp;
    }
    return noEsq;
}

// Analisa a regra de produção para 'expressao_aritmetica'.
ASTNode* expressao_aritmetica() {
    ASTNode* noEsq = nullptr; // Inicializa o lado esquerdo.
    string op_unario = ""; // Inicializa o operador unário.
    if (atual().lexema == "+" || atual().lexema == "-") { // Se há operador unário.
        op_unario = atual().lexema; // Pega o operador.
        pos++; // Consome o operador.
    }
    noEsq = termo_aritmetico(); // Analisa o termo aritmético.
    if (!noEsq) return nullptr;
    if (!op_unario.empty()) { // Se havia operador unário.
        ASTNode* noUnario = new ASTNode("operador_unario", op_unario); // Cria o nó do operador unário.
        noUnario->filhos.push_back(noEsq); // Adiciona o operando.
        noEsq = noUnario; // Atualiza o lado esquerdo.
    }

    while (atual().lexema == "+" || atual().lexema == "-") { // Enquanto houver adição/subtração.
        string op = atual().lexema; // Pega o operador.
        pos++; // Consome o operador.
        ASTNode* noDir = termo_aritmetico(); // Analisa o termo aritmético direito.
        if (!noDir) return syntaxError("esperado termo apos operador '" + op + "'");
        ASTNode* noExp = new ASTNode("operador_binario", op); // Cria o nó do operador binário.
        noExp->filhos.push_back(noEsq); // Adiciona o lado esquerdo.
        noExp->filhos.push_back(noDir); // Adiciona o lado direito.
        noEsq = noExp; // Atualiza o lado esquerdo.
    }
    return noEsq;
}

// Analisa a regra de produção para 'termo_aritmetico'.
ASTNode* termo_aritmetico() {
    ASTNode* noEsq = fator(); // Analisa o fator esquerdo.
    if (!noEsq) return nullptr;
    while (atual().lexema == "*" || atual().lexema == "/" || atual().lexema == "div" || atual().lexema == "mod") { // Enquanto houver multiplicação/divisão.
        string op = atual().lexema; // Pega o operador.
        pos++; // Consome o operador.
        ASTNode* noDir = fator(); // Analisa o fator direito.
        if (!noDir) return syntaxError("esperado fator apos operador '" + op + "'");
        ASTNode* noExp = new ASTNode("operador_binario", op); // Cria o nó do operador binário.
        noExp->filhos.push_back(noEsq); // Adiciona o lado esquerdo.
        noExp->filhos.push_back(noDir); // Adiciona o lado direito.
        noEsq = noExp; // Atualiza o lado esquerdo.
    }
    return noEsq;
}

// Analisa a regra de produção para 'fator'.
ASTNode* fator() {
    if (atual().lexema == "not") { // Se for "not".
        pos++; // Consome "not".
        ASTNode* noNot = new ASTNode("operador_unario", "not"); // Cria o nó do operador unário.
        ASTNode* noFator = fator(); // Analisa o fator.
        if (!noFator) return syntaxError("esperado uma expressao ou fator apos 'not'");
        noNot->filhos.push_back(noFator); // Adiciona o operando.
        return noNot;
    }
    if (atual().tipo == "Numero") { // Se for um número.
        ASTNode* noNum = new ASTNode("numero", atual().lexema); // Cria o nó do número.
        pos++; // Consome o número.
        return noNum;
    }
    if (atual().tipo == "Identificador") { // Se for um identificador.
        if (static_cast<size_t>(pos) + 1 < tokens.size() && tokens[pos + 1].lexema == "(") { // Se for chamada de função.
            string id = atual().lexema; // Pega o identificador.
            if (TabelaSimbolos.count(id) && TabelaSimbolos[id] != "funcao" && id != "read" && id != "write") { // Verifica o tipo.
                return syntaxError("identificador '" + id + "' e um " + TabelaSimbolos[id] + ", nao uma funcao, e nao pode ser usado em uma expressao");
            }
            return chamada_subrotina(); // Analisa a chamada de sub-rotina.
        } else {
            return variavel(); // Analisa a variável.
        }
    }
    if (expect("(").lexema.empty() == false) { // Se for parênteses.
        ASTNode* noExp = expressao(); // Analisa a expressão interna.
        if (!noExp) return nullptr;
        if (expect(")").lexema.empty()) { // Espera ')'.
            return syntaxError("esperado ')' para fechar a expressao entre parenteses");
        }
        return noExp;
    }
    return syntaxError("fator invalido: esperado numero, identificador, 'not', ou expressao com '()'"); // Fator inválido.
}

// Analisa a regra de produção para 'chamada_subrotina'.
ASTNode* chamada_subrotina() {
    Token idToken = expect("Identificador", true); // Espera o identificador da sub-rotina.
    if(idToken.lexema.empty()) return nullptr;

    ASTNode* noCall = new ASTNode("chamada_subrotina", idToken.lexema); // Cria o nó da chamada.
    if (atual().lexema == "(") { // Se houver argumentos.
        pos++; // Consome '('.
        if (atual().lexema != ")") {
            ASTNode* noArgs = lista_argumentos(); // Analisa a lista de argumentos.
            if (!noArgs) return nullptr;
            noCall->filhos.push_back(noArgs); // Adiciona os argumentos.
        }
        if (expect(")").lexema.empty()) { // Espera ')'.
            return syntaxError("esperado ')' para fechar a lista de argumentos da chamada de '" + idToken.lexema + "'");
        }
    }
    return noCall;
}

// Analisa a regra de produção para 'lista_argumentos'.
ASTNode* lista_argumentos() {
    ASTNode* noLista = new ASTNode("lista_argumentos"); // Cria o nó.
    do {
        ASTNode* noExpr = expressao(); // Analisa uma expressão para cada argumento.
        if (!noExpr) return syntaxError("argumento invalido na lista de argumentos");
        noLista->filhos.push_back(noExpr); // Adiciona a expressão como filho.
        if (atual().lexema != ",") break; // Sai se não houver vírgula.
        pos++; // Consome a vírgula.
    } while (true);
    return noLista;
}

// Imprime a Árvore Sintática Abstrata (AST) hierarquicamente.
void imprimirAST(ASTNode* node, int nivel = 0) {
    if (!node) return; // Se o nó é nulo, retorna.
    for (int i = 0; i < nivel; ++i) cout << "   "; // Imprime indentação.
    cout << "+--" << node->tipo; // Imprime o tipo do nó.
    if (!node->valor.empty()) cout << " (" << node->valor << ")"; // Imprime o valor do nó.
    cout << "\n"; // Quebra de linha.
    for (ASTNode* filho : node->filhos) { // Para cada filho, chama a função recursivamente.
        imprimirAST(filho, nivel + 1);
    }
}

// Função principal do programa.
int main() {

    ifstream arquivo("C:\\Users\\fortu\\Downloads\\Unespar\\Unespar\\Unespar-3ano\\Compiladores\\1Bim\\saida.txt"); // Abre o arquivo.
    if (!arquivo.is_open()) { // Se não conseguir abrir.
        cout << "Erro ao abrir o arquivo 'saida.txt'. Certifique-se de que o caminho esta correto.\n";
        return 1; // Retorna erro.
    }

    string linha; // Variável para cada linha.
    if (!getline(arquivo, linha) || !getline(arquivo, linha)) { // Pula cabeçalho.
        cout << "Arquivo de tokens 'saida.txt' parece estar vazio ou com formato de cabecalho invalido.\n";
        arquivo.close(); // Fecha o arquivo.
        return 1; // Retorna erro.
    }

    int linhaDoCodigoFonte = 1; // Contador de linha para tokens.
    while (getline(arquivo, linha)) { // Lê cada linha.
        if (linha.find_first_not_of(" \t\r\n") == string::npos) { // Ignora linhas em branco.
            continue;
        }

        stringstream ss(linha); // Cria stringstream.
        Token t; // Cria um token.

        if (!(ss >> t.lexema)) { // Extrai o lexema.
            continue; // Pula linha mal formatada.
        }

        string tipoCompleto; // Extrai o resto como tipo.
        getline(ss, tipoCompleto);

        size_t first = tipoCompleto.find_first_not_of(" \t"); // Remove espaços do tipo.
        if (string::npos != first) {
            size_t last = tipoCompleto.find_last_not_of(" \t");
            t.tipo = tipoCompleto.substr(first, (last - first + 1));
        } else {
            t.tipo = "";
        }
        
        if (t.lexema == "read" || t.lexema == "write") { // Reclassifica "read"/"write".
            t.tipo = "Identificador";
        }

        t.linha = linhaDoCodigoFonte++; // Atribui linha e incrementa.
        tokens.push_back(t); // Adiciona o token.
    }
    arquivo.close(); // Fecha o arquivo.

    if (tokens.empty()) { // Se nenhum token foi lido.
        cout << "Nenhum token foi lido do arquivo. Verifique o conteudo e o formato de 'saida.txt'.\n";
        return 1; // Retorna erro.
    }

    ASTNode* raiz = nullptr; // Raiz da AST.
    try {
        raiz = programa(); // Inicia a análise sintática.
        if (raiz) { // Se bem-sucedida.
            cout << "\nAnalise sintatica concluida com sucesso.\n";
            imprimirAST(raiz); // Imprime a AST.
            delete raiz; // Libera memória.
        } else if (!fimTokens()) { // Se falhou e há tokens.
            syntaxError("Codigo fonte invalido ou o parser nao consumiu todos os tokens.");
        }
    } catch (...) { // Captura exceções.
        cout << "\nUma excecao nao tratada ocorreu durante a analise.\n";
        if(raiz) delete raiz; // Libera memória.
    }
    
    return 0;
}
