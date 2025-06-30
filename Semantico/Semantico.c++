#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>
#include <stack>      
#include <stdexcept>  

using namespace std;

//================================================================================
// ESTRUTURAS DE DADOS (Token e AST) 
//================================================================================

struct Token {
    string lexema;
    string tipo;
    int linha;
};

struct ASTNode {
    string tipo;
    string valor;
    vector<ASTNode*> filhos;
    int linha; 

    ASTNode(string t, string v = "", int l = 0) : tipo(t), valor(v), linha(l) {}

    ~ASTNode() {
        for (ASTNode* filho : filhos) {
            delete filho;
        }
    }
};

//================================================================================
// <<< NOVO: NÚCLEO DA ANÁLISE SEMÂNTICA >>>
//================================================================================

// Enum para representar os tipos de dados da nossa linguagem de forma mais segura
enum class TipoDado {
    INDEFINIDO,
    INTEIRO,
    BOOLEANO,
    REAL,
    STRING,
    PROGRAMA,
    FUNCAO,
    PROCEDIMENTO,
    TIPO_DEFINIDO // Para quando se cria um novo tipo (ex: type MeuArray = ...)
};

// Estrutura para uma entrada na tabela de símbolos
struct Simbolo {
    string nome;
    string categoria; // "variavel", "funcao", "tipo", etc.
    TipoDado tipoDado;
    int linhaDeclaracao;
    // Pode adicionar: número de parâmetros, tipo de retorno, etc.
};

// Classe para gerenciar a Tabela de Símbolos com escopos
class TabelaDeSimbolos {
private:
    // Pilha de escopos: cada escopo é um mapa de identificadores para símbolos
    stack<map<string, Simbolo>> pilhaDeEscopos;

public:
    TabelaDeSimbolos() {
        entrarEscopo(); // Inicia com o escopo global para evitar erros de uso de símbolos antes de declarar
    }

    // Ao entrar em uma função ou bloco, um novo escopo é criado na pilha
    void entrarEscopo() {
        pilhaDeEscopos.push({});
    }

    // Ao sair de uma função ou bloco, o escopo atual (topo da pilha) é destruído
    void sairEscopo() {
        if (!pilhaDeEscopos.empty()) {
            pilhaDeEscopos.pop();
        }
    }

    // Declara um novo símbolo no escopo atual (o topo da pilha).
    void adicionarSimbolo(const Simbolo& s) {
        if (pilhaDeEscopos.top().count(s.nome)) {
            // ERRO SEMÂNTICO: Tentativa de declarar um identificador que já existe no mesmo escopo.
            throw runtime_error("Erro Semantico na linha " + to_string(s.linhaDeclaracao) + ": Identificador '" + s.nome + "' ja foi declarado neste escopo.");
        }
        pilhaDeEscopos.top()[s.nome] = s;
    }

    // Busca um símbolo pelo nome, percorrendo os escopos da pilha.
    Simbolo buscarSimbolo(const string& nome, int linhaUso) {
        stack<map<string, Simbolo>> temp = pilhaDeEscopos;
        while (!temp.empty()) {
            if (temp.top().count(nome)) {
                return temp.top().at(nome);
            }
            temp.pop();
        }
        // ERRO SEMÂNTICO: Identificador não encontrado em nenhum escopo.
        throw runtime_error("Erro Semantico na linha " + to_string(linhaUso) + ": Identificador '" + nome + "' nao foi declarado.");
    }
};

// Classe para realizar a análise semântica da AST
class AnalisadorSemantico {
public:
    TabelaDeSimbolos tabela;

    // Método principal que inicia a análise semântica
    void analisar(ASTNode* noRaiz) {
        try {
            visitar(noRaiz);
            cout << "\nAnalise semantica concluida com sucesso." << endl;
        } catch (const runtime_error& e) {
            cout << "\n" << e.what() << endl;
        }
    }

private:
    // Converte uma string de tipo para nosso enum
    TipoDado stringParaTipoDado(const string& tipoStr) {
        if (tipoStr == "integer") return TipoDado::INTEIRO;
        if (tipoStr == "boolean") return TipoDado::BOOLEANO;
        if (tipoStr == "real") return TipoDado::REAL;
        if (tipoStr == "string") return TipoDado::STRING;
        return TipoDado::INDEFINIDO;
    }

    // Método "dispatcher": decide qual função de visita chamar com base no tipo do nó
    void visitar(ASTNode* no) {
        if (!no) return;

        if (no->tipo == "programa") visitarPrograma(no);
        else if (no->tipo == "bloco") visitarBloco(no);
        else if (no->tipo == "declaracao_variaveis") visitarDeclaracaoVariaveis(no);
        else if (no->tipo == "funcao") visitarDeclaracaoFuncao(no);
        else if (no->tipo == "atribuicao") visitarAtribuicao(no);
        else if (no->tipo == "identificador") visitarIdentificador(no);
        // Adicionar outros 'else if' para cada tipo de nó 
        else {
            // Se o nó não precisa de uma ação especial, apenas visita seus filhos recursivamente
            for (ASTNode* filho : no->filhos) {
                visitar(filho);
            }
        }
    }

    // Visita o nó do programa
    void visitarPrograma(ASTNode* no) {
        Simbolo s = {no->valor, "programa", TipoDado::PROGRAMA, no->linha};
        tabela.adicionarSimbolo(s);
        visitar(no->filhos[0]); // Visita o bloco principal
    }

    // Visita um bloco, criando e destruindo um novo escopo
    void visitarBloco(ASTNode* no) {
        tabela.entrarEscopo();
        for (ASTNode* filho : no->filhos) {
            visitar(filho);
        }
        tabela.sairEscopo();
    }

    // Visita uma declaração de função
    void visitarDeclaracaoFuncao(ASTNode* no) {
        // 1. Adiciona o nome da função ao escopo atual ANTES de processar o corpo
        Simbolo s = {no->valor, "funcao", TipoDado::FUNCAO, no->linha};
        tabela.adicionarSimbolo(s);

        // 2. Cria um novo escopo para os parâmetros e variáveis locais da função
        tabela.entrarEscopo();
        
        // 3. Processa os parâmetros (se houver) e o bloco da função
        for (ASTNode* filho : no->filhos) {
            visitar(filho);
        }

        // 4. Sai do escopo da função
        tabela.sairEscopo();
    }

    // Visita uma declaração de variáveis
    void visitarDeclaracaoVariaveis(ASTNode* no) {
        for (ASTNode* decl : no->filhos) { // Para cada 'grupo' de declaração (ex: a,b:integer;)
            ASTNode* listaIds = decl->filhos[0];
            ASTNode* noTipo = decl->filhos[1];

            TipoDado tipo = stringParaTipoDado(noTipo->valor);
            if (tipo == TipoDado::INDEFINIDO) {
                 // ERRO SEMÂNTICO: O tipo usado na declaração não é válido.
                throw runtime_error("Erro Semantico na linha " + to_string(noTipo->linha) + ": Tipo '" + noTipo->valor + "' desconhecido.");
            }

            // Para cada identificador na lista 
            for (ASTNode* id : listaIds->filhos) {
                Simbolo s = {id->valor, "variavel", tipo, id->linha};
                tabela.adicionarSimbolo(s); // Ação Semântica: Adiciona à tabela
            }
        }
    }

    // Visita um nó de atribuição
    void visitarAtribuicao(ASTNode* no) {
        // O filho da esquerda é a variável que recebe o valor
        ASTNode* variavelNode = no->filhos[0];
        // O filho da direita é a expressão
        ASTNode* expressaoNode = no->filhos[1];

        // Ação Semântica: Verifica se a variável do lado esquerdo foi declarada.
        // O método buscarSimbolo já lança um erro se não encontrar.
        tabela.buscarSimbolo(variavelNode->valor, variavelNode->linha);

        // Agora, visita a expressão do lado direito para checar seus identificadores
        visitar(expressaoNode);

        // <<<CHECAGEM DE TIPOS >>>
        // Aqui você adicionaria a lógica para comparar o tipo da variável
        // com o tipo resultante da expressão.
        // Ex: TipoDado tipoVar = tabela.buscarSimbolo(variavelNode->valor).tipoDado;
        //     TipoDado tipoExpr = avaliarTipo(expressaoNode);
        //     if (tipoVar != tipoExpr) { ... erro ... }
    }

    // Visita um nó de identificador (quando usado em uma expressão, por exemplo)
    void visitarIdentificador(ASTNode* no) {
        // Ação Semântica: Apenas verifica se o identificador foi declarado.
        // O método buscarSimbolo já faz a verificação e lança um erro se necessário.
        tabela.buscarSimbolo(no->valor, no->linha);
    }
};

vector<Token> tokens;
int pos = 0;
set<string> declaredLabels;

// Protótipos das funções do parser
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


Token atual() {
    if (pos < (int)tokens.size()) return tokens[pos];
    return {"", "FIM_DE_ARQUIVO", -1};
}

bool fimTokens() {
    return pos >= (int)tokens.size();
}

ASTNode* syntaxError(const string& mensagem) {
    if (!fimTokens()) {
        cout << "Erro na linha " << atual().linha << ": " << mensagem << ". Token encontrado: '" << atual().lexema << "' do tipo '" << atual().tipo << "'\n";
    } else {
        cout << "Erro: " << mensagem << " no final do arquivo.\n";
    }
    exit(1);
    return nullptr;
}

Token expect(const string& esperado, bool isType = false) {
    if (fimTokens()) return {"", "", -1};
    if (isType) {
        if (atual().tipo == esperado) {
            return tokens[pos++];
        }
    } else {
        if (atual().lexema == esperado) {
            return tokens[pos++];
        }
    }
    return {"", "", -1};
}

ASTNode* programa() {
    Token progToken = expect("Program");
    if (progToken.lexema.empty()) {
        return syntaxError("um programa deve comecar com a palavra-chave 'Program'");
    }
    Token idToken = expect("Identificador", true);
    if (idToken.lexema.empty()) {
        return syntaxError("esperado um nome de identificador para o programa");
    }
    string nomePrograma = idToken.lexema;

    if (atual().lexema == "(") {
        pos++;
        while (atual().lexema != ")" && !fimTokens()) {
            if(expect("Identificador", true).lexema.empty()) {
                return syntaxError("esperado identificador na lista de parametros do programa");
            }
            if(atual().lexema == ",") pos++;
        }
        if (expect(")").lexema.empty()) {
            return syntaxError("esperado ')' para fechar a lista de parametros do programa");
        }
    }

    if (expect(";").lexema.empty()) {
        return syntaxError("esperado ';' apos o cabecalho do programa");
    }
    ASTNode* noBloco = bloco();
    if (!noBloco) return nullptr;

    if (expect(".").lexema.empty()) {
        return syntaxError("esperado '.' no final do programa");
    }

    if (atual().tipo != "FIM_DE_ARQUIVO" && !fimTokens()) {
        return syntaxError("tokens inesperados '" + atual().lexema + "' apos o final do programa");
    }

    ASTNode* noPrograma = new ASTNode("programa", nomePrograma, progToken.linha);
    noPrograma->filhos.push_back(noBloco);
    return noPrograma;
}

ASTNode* bloco() {
    ASTNode* noBloco = new ASTNode("bloco", "", atual().linha);
    if (atual().lexema == "label") {
        noBloco->filhos.push_back(declaracao_rotulos());
    }
    if (atual().lexema == "type") {
        noBloco->filhos.push_back(declaracao_tipos());
    }
    if (atual().lexema == "var") {
        noBloco->filhos.push_back(declaracao_variaveis());
    }
    while (atual().lexema == "function" || atual().lexema == "procedure") {
        ASTNode* noSubRotina = nullptr;
        if (atual().lexema == "function") {
            noSubRotina = declaracao_funcao();
        } else {
            noSubRotina = declaracao_procedimento();
        }
        if (!noSubRotina) return nullptr;
        noBloco->filhos.push_back(noSubRotina);
    }
    if (expect("begin").lexema.empty()) {
        return syntaxError("esperado 'begin' para iniciar o bloco de comandos");
    }
    noBloco->filhos.push_back(lista_comandos());

    if (expect("end").lexema.empty()) {
        return syntaxError("esperado 'end' para finalizar o bloco");
    }
    return noBloco;
}

// Declaracao de função e procedimento
ASTNode* declaracao_funcao() { return new ASTNode("declaracao_funcao_stub"); } {
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

ASTNode* declaracao_procedimento() { return new ASTNode("declaracao_procedimento_stub"); } {
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

ASTNode* parametros() { return new ASTNode("parametros_stub"); }  {
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

ASTNode* lista_comandos() { return new ASTNode("lista_comandos_stub"); } {
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

ASTNode* comando() { return new ASTNode("comando_stub"); } {
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

ASTNode* variavel() { return new ASTNode("variavel_stub"); } {
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

ASTNode* expressao() { return new ASTNode("expressao_stub"); } {
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

ASTNode* declaracao_rotulos() { return new ASTNode("declaracao_rotulos_stub"); } {
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

ASTNode* declaracao_tipos() { return new ASTNode("declaracao_tipos_stub"); } {
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

ASTNode* declaracao_variaveis() {
    expect("var");
    ASTNode* noVars = new ASTNode("declaracao_variaveis", "", atual().linha);
    while (atual().tipo == "Identificador") {
        ASTNode* noLista = lista_identificadores();
        if (expect(":").lexema.empty()) return syntaxError("esperado ':'");
        ASTNode* noTipo = tipo();
        if (expect(";").lexema.empty()) return syntaxError("esperado ';'");
        ASTNode* noDecl = new ASTNode("declaracao_variavel", "", noLista->linha);
        noDecl->filhos.push_back(noLista);
        noDecl->filhos.push_back(noTipo);
        noVars->filhos.push_back(noDecl);
    }
    return noVars;
}
ASTNode* lista_identificadores() {
    ASTNode* noLista = new ASTNode("lista_identificadores", "", atual().linha);
    do {
        Token idToken = expect("Identificador", true);
        if (idToken.lexema.empty()) return syntaxError("esperado um identificador");
        noLista->filhos.push_back(new ASTNode("identificador", idToken.lexema, idToken.linha));
        if (atual().lexema != ",") break;
        pos++;
    } while (true);
    return noLista;
}

ASTNode* tipo() {
    Token tipoToken = tokens[pos++];
    return new ASTNode("tipo_primitivo", tipoToken.lexema, tipoToken.linha);
}

ASTNode* declaracao_funcao() {
    Token funcToken = expect("function");
    Token idToken = expect("Identificador", true);
    ASTNode* noFunc = new ASTNode("funcao", idToken.lexema, funcToken.linha);
    if (atual().lexema == "(") {
        pos++; // (
        if (atual().lexema != ")") noFunc->filhos.push_back(parametros());
        expect(")");
    }
    expect(":");
    noFunc->filhos.push_back(tipo());
    expect(";");
    noFunc->filhos.push_back(bloco());
    expect(";");
    return noFunc;
}

ASTNode* declaracao_procedimento() { return new ASTNode("declaracao_procedimento_stub"); } {
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

ASTNode* parametros() { return new ASTNode("parametros_stub"); } {
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

ASTNode* lista_comandos() {
    ASTNode* lista = new ASTNode("lista_comandos", "", atual().linha);
    while (atual().lexema != "end") {
        ASTNode* cmd = comando();
        if (cmd) lista->filhos.push_back(cmd);
        if (atual().lexema == ";") pos++;
        else break;
    }
    return lista;
}

ASTNode* comando() {
    if (atual().tipo == "Identificador") {
        ASTNode* lhs = variavel();
        if (atual().lexema == ":=") {
            expect(":=");
            ASTNode* rhs = expressao();
            ASTNode* assignNode = new ASTNode("atribuicao", "", lhs->linha);
            assignNode->filhos.push_back(lhs);
            assignNode->filhos.push_back(rhs);
            return assignNode;
        }
        // É uma chamada de procedimento, etc.
        pos--; // backtrack
        delete lhs;
        return chamada_subrotina();
    }
    return nullptr; // Comando vazio ou não reconhecido
}
ASTNode* variavel() {
    Token idToken = expect("Identificador", true);
    if(idToken.lexema.empty()) return nullptr;
    return new ASTNode("identificador", idToken.lexema, idToken.linha);
}
ASTNode* expressao() { return termo_logico(); } {
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

ASTNode* termo_logico() { return expressao_relacional(); } {
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

ASTNode* expressao_relacional() { return expressao_aritmetica(); } {
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

ASTNode* expressao_aritmetica() { return termo_aritmetico(); } {
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

ASTNode* termo_aritmetico() { return fator(); } 
{
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

ASTNode* fator() {
    if (atual().tipo == "Numero") {
        Token numToken = tokens[pos++];
        return new ASTNode("numero", numToken.lexema, numToken.linha);
    }
    if (atual().tipo == "Identificador") {
        return variavel();
    }
    if (atual().lexema == "(") {
        expect("(");
        ASTNode* exp = expressao();
        expect(")");
        return exp;
    }
    return nullptr;
}

ASTNode* chamada_subrotina() {
    Token idToken = expect("Identificador", true);
    ASTNode* callNode = new ASTNode("chamada_subrotina", idToken.lexema, idToken.linha);
    if (atual().lexema == "(") {
        expect("(");
        if (atual().lexema != ")") callNode->filhos.push_back(lista_argumentos());
        expect(")");
    }
    return callNode;
}

ASTNode* lista_argumentos() {
    ASTNode* args = new ASTNode("lista_argumentos", "", atual().linha);
    do {
        args->filhos.push_back(expressao());
        if (atual().lexema != ",") break;
        expect(",");
    } while (true);
    return args;
}


// Imprime a AST para depuração
void imprimirAST(ASTNode* node, int nivel = 0) {
    if (!node) return;
    for (int i = 0; i < nivel; ++i) cout << "   ";
    cout << "+--" << node->tipo;
    if (!node->valor.empty()) cout << " (" << node->valor << ")";
    cout << " (linha " << node->linha << ")" << "\n";
    for (ASTNode* filho : node->filhos) {
        imprimirAST(filho, nivel + 1);
    }
}

//================================================================================
// FUNÇÃO PRINCIPAL ATUALIZADA
//================================================================================

int main() {
    // --- FASE 1: Análise Léxica (Lendo os tokens do arquivo) ---
    ifstream arquivo("C:\\Compiladores\\saida.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo 'saida.txt'.\n";
        return 1;
    }
    string linha;
    getline(arquivo, linha); getline(arquivo, linha); // Pula cabeçalho

    int linhaDoCodigoFonte = 1; 
    while (getline(arquivo, linha)) {
        if (linha.find_first_not_of(" \t\r\n") == string::npos) continue;
        stringstream ss(linha);
        Token t;
        ss >> t.lexema;
        string tipoCompleto;
        getline(ss, tipoCompleto);
        size_t first = tipoCompleto.find_first_not_of(" \t");
        if (string::npos != first) {
            t.tipo = tipoCompleto.substr(first, tipoCompleto.find_last_not_of(" \t") - first + 1);
        } else {
            t.tipo = "";
        }
        if (t.lexema == "read" || t.lexema == "write") t.tipo = "Identificador";
        t.linha = linhaDoCodigoFonte++;
        tokens.push_back(t);
    }
    arquivo.close();

    if (tokens.empty()) {
        cout << "Nenhum token foi lido do arquivo 'saida.txt'.\n";
        return 1;
    }

    // --- FASE 2: Análise Sintática (Construção da AST) ---
    ASTNode* raiz = nullptr;
    try {
        raiz = programa();
        if (raiz) {
            cout << "\nAnalise sintatica concluida com sucesso.\n";
        } else if (!fimTokens()) {
            syntaxError("Codigo fonte invalido ou o parser nao consumiu todos os tokens.");
        }
    } catch (const exception& e) {
        cerr << "Uma excecao ocorreu durante a analise sintatica: " << e.what() << '\n';
        if(raiz) delete raiz;
        return 1;
    }
    
    // --- FASE 3: Análise Semântica (Percorrendo a AST) ---
    if (raiz) {
        AnalisadorSemantico analisador;
        analisador.analisar(raiz);
    }

    delete raiz;
    
    return 0;
}
