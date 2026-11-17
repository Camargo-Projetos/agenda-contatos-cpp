// Agenda de contatos no terminal (arquivo unico).
// Compilar: g++ -std=c++17 -Wall -Wextra -pedantic -o agenda src/main.cpp
#include <iostream>
#include <string>

using namespace std;

const int MAX = 1000;  // capacidade fixa do vetor de contatos
const int ID_MAXIMO = 999999999;

// Campos usados nas buscas e na ordenacao.
enum Campo { CAMPO_NOME = 1, CAMPO_TELEFONE, CAMPO_EMAIL, CAMPO_EMPRESA };

// ---------- Estruturas de dados ----------

struct Contato {
    int id = 0;
    string nome, telefone, email, empresa;
};

// Reune todos os dados. E passada por referencia (&) para as funcoes alterarem o original.
struct Agenda {
    Contato* contatos[MAX] = {};  // vetor de ponteiros; cada Contato e criado com new
    int total = 0;
    int proximoId = 1;
};

// ---------- Entrada de dados ----------

// Le uma linha; com obrigatorio = true (valor padrao) repete ate nao ser vazia.
// Se a entrada terminar (Ctrl+D), o laco sai e devolve "".
string lerTexto(const string& rotulo, bool obrigatorio = true) {
    string linha;
    cout << rotulo << ": ";
    while (getline(cin, linha)) {
        if (!linha.empty() || !obrigatorio) return linha;
        cout << "  Campo obrigatorio.\n" << rotulo << ": ";
    }
    return "";
}

// Le um inteiro entre minimo e maximo (repete ate ser valido). Se a entrada terminar, devolve minimo.
int lerInteiro(const string& rotulo, int minimo, int maximo) {
    string texto;
    cout << rotulo << ": ";
    while (getline(cin, texto)) {
        bool valido = !texto.empty() && texto.size() <= 9;
        int valor = 0;
        for (size_t i = 0; valido && i < texto.size(); i++) {
            if (texto[i] < '0' || texto[i] > '9') valido = false;
            else valor = valor * 10 + (texto[i] - '0');
        }
        if (valido && valor >= minimo && valor <= maximo) return valor;
        cout << "  Valor invalido (de " << minimo << " a " << maximo << ").\n" << rotulo << ": ";
    }
    return minimo;
}

// Pergunta sim/nao (so 's' ou 'S' conta como sim).
bool confirmar(const string& pergunta) {
    string resposta = lerTexto(pergunta + " (s/n)");
    return !resposta.empty() && (resposta[0] == 's' || resposta[0] == 'S');
}

// ---------- Comparacao e campos ----------

// Letra minuscula (so ASCII), para comparar sem diferenciar maiusculas.
char minuscula(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// Compara dois textos sem diferenciar maiusculas. Retorno: -1 (a antes de b), 0 (iguais), 1 (a depois de b).
int comparar(const string& a, const string& b) {
    size_t i = 0;
    while (i < a.size() && i < b.size()) {
        char x = minuscula(a[i]);
        char y = minuscula(b[i]);
        if (x != y) return (x < y) ? -1 : 1;
        i++;
    }
    if (a.size() == b.size()) return 0;
    return (a.size() < b.size()) ? -1 : 1;
}

// Devolve o campo escolhido do contato. O retorno e referencia (&) para nao copiar a string;
// o contato vem por ponteiro const (*) porque a funcao so le.
const string& valorDoCampo(const Contato* c, int campo) {
    if (campo == CAMPO_TELEFONE) return c->telefone;
    if (campo == CAMPO_EMAIL) return c->email;
    if (campo == CAMPO_EMPRESA) return c->empresa;
    return c->nome;
}

// Mostra um contato em uma linha.
void exibirContato(const Contato* c) {
    cout << "  [" << c->id << "] " << c->nome << " | " << c->telefone << " | "
         << (c->email.empty() ? "-" : c->email) << " | " << (c->empresa.empty() ? "-" : c->empresa) << "\n";
}

// ---------- Busca [R10] ----------

// Busca sequencial: percorre o vetor do inicio ao fim. Melhor caso O(1), medio e pior O(n);
// funciona com vetor desordenado. 'inicio' tem valor padrao 0 (permite achar as proximas
// ocorrencias). Retorno: indice da primeira ocorrencia ou -1.
int buscaSequencial(Contato** v, int n, int campo, const string& chave, long* comparacoes, int inicio = 0) {
    for (int i = inicio; i < n; i++) {
        (*comparacoes)++;
        if (comparar(valorDoCampo(v[i], campo), chave) == 0) return i;
    }
    return -1;
}

// ---------- Operacoes da agenda ----------

// Procura a posicao de um contato no vetor pelo ID. Retorno: indice ou -1.
int indiceDoContato(const Agenda& a, int id) {
    for (int i = 0; i < a.total; i++)
        if (a.contatos[i]->id == id) return i;
    return -1;
}

// Cria o contato com new e o guarda no vetor ('dados' por referencia const &: sem copia).
// Retorno: ponteiro do contato criado ou nullptr se a agenda estiver cheia.
Contato* adicionarContato(Agenda& a, const Contato& dados) {
    if (a.total >= MAX) return nullptr;
    Contato* novo = new Contato(dados);
    novo->id = a.proximoId++;
    a.contatos[a.total++] = novo;
    return novo;
}

// Exclui um contato: tira dos grupos e da arvore e depois libera a memoria com delete.
// Retorno: false se o ID nao existe.
bool excluirContato(Agenda& a, int id) {
    int pos = indiceDoContato(a, id);
    if (pos < 0) return false;
    Contato* alvo = a.contatos[pos];
    for (int i = pos; i < a.total - 1; i++) a.contatos[i] = a.contatos[i + 1];
    a.total--;
    a.contatos[a.total] = nullptr;
    delete alvo;
    return true;
}

// Libera toda a memoria da agenda. Os contatos vao por ultimo: grupos e arvore apontam para eles.
void liberarAgenda(Agenda& a) {
    for (int i = 0; i < a.total; i++) {
        delete a.contatos[i];
        a.contatos[i] = nullptr;
    }
    a.total = 0;
}

// Lista os contatos do vetor (no maximo 'maximo'; valor padrao: todos).
void listarContatos(const Agenda& a, int maximo = MAX) {
    if (a.total == 0) {
        cout << "  Nenhum contato cadastrado.\n";
        return;
    }
    for (int i = 0; i < a.total && i < maximo; i++) exibirContato(a.contatos[i]);
    cout << "  Total: " << a.total << " contato(s).\n";
}

// ---------- Menus ----------

// Pede um ID e devolve o contato (ou nullptr, ja avisando o usuario).
Contato* lerContato(Agenda& a) {
    int id = lerInteiro("ID do contato", 1, ID_MAXIMO);
    if (!cin) return nullptr;
    int pos = indiceDoContato(a, id);
    if (pos < 0) {
        cout << "  Contato nao encontrado.\n";
        return nullptr;
    }
    return a.contatos[pos];
}

void cadastrarContatoMenu(Agenda& a) {
    Contato c;
    c.nome = lerTexto("Nome");
    c.telefone = lerTexto("Telefone");
    c.email = lerTexto("E-mail (opcional)", false);
    c.empresa = lerTexto("Empresa (opcional)", false);
    if (!cin) return;  // a entrada terminou no meio do cadastro
    Contato* novo = adicionarContato(a, c);
    if (novo == nullptr) cout << "  Agenda cheia (maximo " << MAX << ").\n";
    else cout << "  Contato cadastrado com ID " << novo->id << ".\n";
}

void excluirContatoMenu(Agenda& a) {
    Contato* alvo = lerContato(a);
    if (alvo == nullptr) return;
    exibirContato(alvo);
    if (!confirmar("Excluir este contato?")) {
        cout << "  Exclusao cancelada.\n";
        return;
    }
    excluirContato(a, alvo->id);
    cout << "  Contato excluido.\n";
}

void menuContatos(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== CONTATOS ===\n1. Cadastrar\n2. Listar\n3. Excluir\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 3);
        if (opcao == 1) cadastrarContatoMenu(a);
        else if (opcao == 2) listarContatos(a);
        else if (opcao == 3) excluirContatoMenu(a);
    } while (opcao != 0);
}

// Busca contatos pelo nome (busca sequencial) e mostra quantas comparacoes foram feitas.
void menuBuscar(Agenda& a) {
    if (a.total == 0) {
        cout << "  Nenhum contato cadastrado.\n";
        return;
    }
    string nome = lerTexto("Nome (exato, sem diferenciar maiusculas)");
    long comparacoes = 0;
    int achados = 0;
    int pos = buscaSequencial(a.contatos, a.total, CAMPO_NOME, nome, &comparacoes);
    while (pos != -1) {
        exibirContato(a.contatos[pos]);
        achados++;
        pos = buscaSequencial(a.contatos, a.total, CAMPO_NOME, nome, &comparacoes, pos + 1);
    }
    cout << "  Encontrados: " << achados << " | Comparacoes: " << comparacoes << "\n";
}

void menuPrincipal(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== AGENDA DE CONTATOS ===\n1. Contatos\n2. Buscar contatos\n";
        cout << "0. Sair\n";
        opcao = lerInteiro("Opcao", 0, 2);
        if (opcao == 1) menuContatos(a);
        else if (opcao == 2) menuBuscar(a);
    } while (opcao != 0);
    cout << "\nEncerrando...\n";
}

int main() {
    Agenda agenda;
    menuPrincipal(agenda);
    liberarAgenda(agenda);
    return 0;
}
