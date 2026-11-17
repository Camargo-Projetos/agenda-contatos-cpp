// Agenda de contatos no terminal (arquivo unico).
// Compilar: g++ -std=c++17 -Wall -Wextra -pedantic -o agenda src/main.cpp
#include <iostream>
#include <string>

using namespace std;

const int MAX = 1000;  // capacidade fixa do vetor de contatos
const int ID_MAXIMO = 999999999;

// Campos usados nas buscas e na ordenacao.
enum Campo { CAMPO_NOME = 1, CAMPO_TELEFONE, CAMPO_EMAIL, CAMPO_EMPRESA };
const char* const CAMPOS[] = {"Nome", "Telefone", "E-mail", "Empresa"};
const int TOTAL_TIPOS = 3;
const char* const TIPOS[TOTAL_TIPOS] = {"Realizada", "Recebida", "Perdida"};

// ---------- Estruturas de dados ----------

struct Contato {
    int id = 0;
    string nome, telefone, email, empresa;
};

// Lista de grupos; cada grupo tem a sua lista de membros (ponteiros, sem copiar os contatos).
struct NoMembro {
    Contato* contato;
    NoMembro* proximo;
};
struct NoGrupo {
    int id;
    string nome;
    NoMembro* membros;  // cabeca da lista de membros
    NoGrupo* proximo;
};

// Pilha do historico e fila de retornos: guardam COPIA do nome e telefone,
// entao continuam validas mesmo se o contato for excluido.
struct NoChamada {
    int numero;
    string nome, telefone, tipo;
    NoChamada* proximo;
};
struct Pilha {
    NoChamada* topo = nullptr;  // chamada mais recente
};
struct NoRetorno {
    int idContato;
    string nome, telefone;
    NoRetorno* proximo;
};
struct Fila {
    NoRetorno* inicio = nullptr;  // proximo a ser atendido
    NoRetorno* fim = nullptr;     // ultimo a entrar
};

// No da arvore binaria de busca (chave = nome). Guarda o ponteiro do contato, sem copiar.
struct NoArvore {
    Contato* contato;
    NoArvore* esquerda;
    NoArvore* direita;
};

// Reune todos os dados. E passada por referencia (&) para as funcoes alterarem o original.
struct Agenda {
    Contato* contatos[MAX] = {};  // vetor de ponteiros; cada Contato e criado com new
    int total = 0;
    int proximoId = 1;
    NoGrupo* grupos = nullptr;  // cabeca da lista de grupos
    int proximoGrupo = 1;
    Pilha historico;
    int proximaChamada = 1;
    Fila retornos;
    NoArvore* raiz = nullptr;  // arvore de contatos indexada pelo nome
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

// Mostra os campos numerados e le a escolha.
int lerCampo(const string& rotulo) {
    for (int i = CAMPO_NOME; i <= CAMPO_EMPRESA; i++) cout << "  " << i << ") " << CAMPOS[i - 1] << "\n";
    return lerInteiro(rotulo, CAMPO_NOME, CAMPO_EMPRESA);
}

// Mostra um contato em uma linha.
void exibirContato(const Contato* c) {
    cout << "  [" << c->id << "] " << c->nome << " | " << c->telefone << " | "
         << (c->email.empty() ? "-" : c->email) << " | " << (c->empresa.empty() ? "-" : c->empresa) << "\n";
}

// ---------- Grupos: lista simplesmente encadeada [R5] ----------

// Insere um grupo no FIM da lista. 'lista' e referencia (&) ao ponteiro da cabeca:
// se a lista estiver vazia, o ponteiro do chamador passa a apontar para o novo no.
NoGrupo* criarGrupo(NoGrupo*& lista, int id, const string& nome) {
    NoGrupo* novo = new NoGrupo{id, nome, nullptr, nullptr};
    if (lista == nullptr) {
        lista = novo;
    } else {
        NoGrupo* ultimo = lista;
        while (ultimo->proximo != nullptr) ultimo = ultimo->proximo;
        ultimo->proximo = novo;
    }
    return novo;
}

// Travessia da lista procurando um grupo pelo ID. 'lista' vai por valor: andar com ele
// nao muda a cabeca do chamador. Retorno: o no ou nullptr.
NoGrupo* buscarGrupo(NoGrupo* lista, int id) {
    while (lista != nullptr && lista->id != id) lista = lista->proximo;
    return lista;
}

// Remove um contato da lista de membros do grupo (o contato NAO e apagado).
bool removerMembro(NoGrupo* g, int idContato) {
    NoMembro* anterior = nullptr;
    NoMembro* atual = g->membros;
    while (atual != nullptr && atual->contato->id != idContato) {
        anterior = atual;
        atual = atual->proximo;
    }
    if (atual == nullptr) return false;
    if (anterior == nullptr) g->membros = atual->proximo;
    else anterior->proximo = atual->proximo;
    delete atual;
    return true;
}

// Adiciona um contato no fim da lista de membros. Retorno: false se ja era membro.
bool adicionarMembro(NoGrupo* g, Contato* c) {
    NoMembro* ultimo = nullptr;
    for (NoMembro* m = g->membros; m != nullptr; m = m->proximo) {
        if (m->contato == c) return false;
        ultimo = m;
    }
    NoMembro* novo = new NoMembro{c, nullptr};
    if (ultimo == nullptr) g->membros = novo;
    else ultimo->proximo = novo;
    return true;
}

// Remove um grupo e libera os nos dos membros (os contatos continuam existindo).
bool removerGrupo(NoGrupo*& lista, int id) {
    NoGrupo* anterior = nullptr;
    NoGrupo* atual = lista;
    while (atual != nullptr && atual->id != id) {
        anterior = atual;
        atual = atual->proximo;
    }
    if (atual == nullptr) return false;
    if (anterior == nullptr) lista = atual->proximo;
    else anterior->proximo = atual->proximo;
    while (atual->membros != nullptr) removerMembro(atual, atual->membros->contato->id);
    delete atual;
    return true;
}

// Travessia: mostra cada grupo e seus membros.
void listarGrupos(const NoGrupo* lista) {
    if (lista == nullptr) {
        cout << "  Nenhum grupo cadastrado.\n";
        return;
    }
    for (const NoGrupo* g = lista; g != nullptr; g = g->proximo) {
        cout << "Grupo [" << g->id << "] " << g->nome << "\n";
        if (g->membros == nullptr) cout << "  (sem membros)\n";
        for (const NoMembro* m = g->membros; m != nullptr; m = m->proximo) exibirContato(m->contato);
    }
}

// ---------- Pilha (LIFO): historico de chamadas [R6] ----------

// push: o novo no aponta para o antigo topo e vira o topo. 'p' vai por referencia (&) porque o topo muda.
void empilharChamada(Pilha& p, int numero, const Contato* c, const string& tipo) {
    p.topo = new NoChamada{numero, c->nome, c->telefone, tipo, p.topo};
}

// peek: consulta o topo sem remover. Retorno: nullptr se vazia.
const NoChamada* consultarTopo(const Pilha& p) {
    return p.topo;
}

// pop: remove o topo (a chamada mais recente). Retorno: false se vazia.
bool desempilharChamada(Pilha& p) {
    if (p.topo == nullptr) return false;
    NoChamada* removido = p.topo;
    p.topo = removido->proximo;
    delete removido;
    return true;
}

// Mostra uma chamada em uma linha.
void exibirChamada(const NoChamada* n) {
    cout << "  #" << n->numero << " | " << n->tipo << " | " << n->nome << " (" << n->telefone << ")\n";
}

// Lista do mais recente (topo) para o mais antigo, sem alterar a pilha.
void listarHistorico(const Pilha& p) {
    if (p.topo == nullptr) {
        cout << "  Historico vazio.\n";
        return;
    }
    for (const NoChamada* n = p.topo; n != nullptr; n = n->proximo) exibirChamada(n);
}

// Esvazia a pilha liberando os nos.
void liberarPilha(Pilha& p) {
    while (desempilharChamada(p)) {}
}

// ---------- Fila (FIFO): retornos de ligacao [R7] ----------

// enqueue: insere no FIM da fila. Retorno: false se o contato ja esta aguardando.
bool enfileirarRetorno(Fila& f, const Contato* c) {
    for (const NoRetorno* n = f.inicio; n != nullptr; n = n->proximo)
        if (n->idContato == c->id) return false;
    NoRetorno* novo = new NoRetorno{c->id, c->nome, c->telefone, nullptr};
    if (f.fim == nullptr) f.inicio = novo;
    else f.fim->proximo = novo;
    f.fim = novo;
    return true;
}

// dequeue: remove o PRIMEIRO da fila (quem entrou antes). Retorno: false se vazia.
bool desenfileirarRetorno(Fila& f) {
    if (f.inicio == nullptr) return false;
    NoRetorno* removido = f.inicio;
    f.inicio = removido->proximo;
    if (f.inicio == nullptr) f.fim = nullptr;  // a fila ficou vazia
    delete removido;
    return true;
}

// Mostra um retorno pendente em uma linha.
void exibirRetorno(const NoRetorno* n) {
    cout << "[" << n->idContato << "] " << n->nome << " (" << n->telefone << ")\n";
}

// Lista os retornos na ordem de atendimento.
void listarRetornos(const Fila& f) {
    if (f.inicio == nullptr) {
        cout << "  Nenhum retorno pendente.\n";
        return;
    }
    int posicao = 1;
    for (const NoRetorno* n = f.inicio; n != nullptr; n = n->proximo) {
        cout << "  " << posicao++ << ". ";
        exibirRetorno(n);
    }
}

// Esvazia a fila liberando os nos.
void liberarFila(Fila& f) {
    while (desenfileirarRetorno(f)) {}
}

// ---------- Arvore binaria de busca (recursiva) [R3, R8] ----------
// Por que arvore: indexada pelo nome, a busca e O(log n) em media e o percurso in-order entrega
// os contatos em ordem alfabetica sem ordenar o vetor. Nome igual vai para a direita (homonimos).

// Insere um contato (recursivo). Retorno: a raiz da subarvore depois da insercao.
NoArvore* inserirNaArvore(NoArvore* raiz, Contato* c) {
    if (raiz == nullptr) return new NoArvore{c, nullptr, nullptr};
    if (comparar(c->nome, raiz->contato->nome) < 0) raiz->esquerda = inserirNaArvore(raiz->esquerda, c);
    else raiz->direita = inserirNaArvore(raiz->direita, c);
    return raiz;
}

// Busca recursiva pelo nome (acha todos os homonimos). 'comparacoes' e ponteiro (*) para o mesmo
// contador ser somado em todas as chamadas recursivas; 'exibir' tem valor padrao true.
// Retorno: quantidade de contatos encontrados.
int buscarNaArvore(const NoArvore* raiz, const string& nome, long* comparacoes, bool exibir = true) {
    if (raiz == nullptr) return 0;
    (*comparacoes)++;
    int cmp = comparar(nome, raiz->contato->nome);
    if (cmp < 0) return buscarNaArvore(raiz->esquerda, nome, comparacoes, exibir);
    int achados = 0;
    if (cmp == 0) {
        achados = 1;
        if (exibir) exibirContato(raiz->contato);
    }
    return achados + buscarNaArvore(raiz->direita, nome, comparacoes, exibir);
}

// Percurso in-order (esquerda, raiz, direita): mostra os contatos em ordem alfabetica.
void percorrerEmOrdem(const NoArvore* raiz) {
    if (raiz == nullptr) return;
    percorrerEmOrdem(raiz->esquerda);
    exibirContato(raiz->contato);
    percorrerEmOrdem(raiz->direita);
}

// Libera os nos (recursivo, pos-ordem); os contatos NAO sao apagados. 'raiz' por referencia (&).
void liberarArvore(NoArvore*& raiz) {
    if (raiz == nullptr) return;
    liberarArvore(raiz->esquerda);
    liberarArvore(raiz->direita);
    delete raiz;
    raiz = nullptr;
}

// ---------- Ordenacao [R9] ----------

// Insertion Sort: insere cada elemento na posicao certa da parte ja ordenada.
// Complexidade (n = quantidade de contatos):
//   melhor caso O(n)    - vetor ja ordenado (1 comparacao por elemento)
//   caso medio  O(n^2)
//   pior caso   O(n^2)  - vetor em ordem inversa
//   memoria extra O(1)  - ordena no proprio vetor; e estavel.
// 'v' e um vetor de ponteiros (Contato**): so os ponteiros trocam de lugar, nao os contatos.
// 'comparacoes' e ponteiro (*) para o chamador receber a contagem.
void insertionSort(Contato** v, int n, int campo, long* comparacoes) {
    for (int i = 1; i < n; i++) {
        Contato* atual = v[i];
        int j = i - 1;
        while (j >= 0) {
            (*comparacoes)++;
            if (comparar(valorDoCampo(v[j], campo), valorDoCampo(atual, campo)) <= 0) break;
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = atual;
    }
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

// Reconstroi a arvore a partir do vetor (usado ao excluir, no lugar de remover um no).
void reconstruirArvore(Agenda& a) {
    liberarArvore(a.raiz);
    for (int i = 0; i < a.total; i++) a.raiz = inserirNaArvore(a.raiz, a.contatos[i]);
}

// Cria o contato com new e o guarda no vetor ('dados' por referencia const &: sem copia).
// Retorno: ponteiro do contato criado ou nullptr se a agenda estiver cheia.
Contato* adicionarContato(Agenda& a, const Contato& dados) {
    if (a.total >= MAX) return nullptr;
    Contato* novo = new Contato(dados);
    novo->id = a.proximoId++;
    a.contatos[a.total++] = novo;
    a.raiz = inserirNaArvore(a.raiz, novo);
    return novo;
}

// Exclui um contato: tira dos grupos e da arvore e depois libera a memoria com delete.
// Retorno: false se o ID nao existe.
bool excluirContato(Agenda& a, int id) {
    int pos = indiceDoContato(a, id);
    if (pos < 0) return false;
    Contato* alvo = a.contatos[pos];
    for (NoGrupo* g = a.grupos; g != nullptr; g = g->proximo) removerMembro(g, id);  // evita ponteiro solto
    for (int i = pos; i < a.total - 1; i++) a.contatos[i] = a.contatos[i + 1];
    a.total--;
    a.contatos[a.total] = nullptr;
    reconstruirArvore(a);
    delete alvo;
    return true;
}

// Libera toda a memoria da agenda. Os contatos vao por ultimo: grupos e arvore apontam para eles.
void liberarAgenda(Agenda& a) {
    while (a.grupos != nullptr) removerGrupo(a.grupos, a.grupos->id);
    liberarPilha(a.historico);
    liberarFila(a.retornos);
    liberarArvore(a.raiz);
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

NoGrupo* lerGrupo(Agenda& a) {
    int id = lerInteiro("ID do grupo", 1, ID_MAXIMO);
    if (!cin) return nullptr;
    NoGrupo* g = buscarGrupo(a.grupos, id);
    if (g == nullptr) cout << "  Grupo nao encontrado.\n";
    return g;
}

void criarGrupoMenu(Agenda& a) {
    string nome = lerTexto("Nome do grupo");
    if (cin) cout << "  Grupo criado com ID " << criarGrupo(a.grupos, a.proximoGrupo++, nome)->id << ".\n";
}

void removerGrupoMenu(Agenda& a) {
    NoGrupo* g = lerGrupo(a);
    if (g == nullptr) return;
    removerGrupo(a.grupos, g->id);
    cout << "  Grupo removido (os contatos continuam cadastrados).\n";
}

void adicionarAoGrupoMenu(Agenda& a) {
    NoGrupo* g = lerGrupo(a);
    if (g == nullptr) return;
    Contato* c = lerContato(a);
    if (c == nullptr) return;
    if (adicionarMembro(g, c)) cout << "  " << c->nome << " adicionado ao grupo " << g->nome << ".\n";
    else cout << "  Esse contato ja pertence ao grupo.\n";
}

void removerDoGrupoMenu(Agenda& a) {
    NoGrupo* g = lerGrupo(a);
    if (g == nullptr) return;
    int idContato = lerInteiro("ID do contato a remover do grupo", 1, ID_MAXIMO);
    if (!cin) return;
    if (removerMembro(g, idContato)) cout << "  Contato removido do grupo.\n";
    else cout << "  Esse contato nao pertence ao grupo.\n";
}

void menuGrupos(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== GRUPOS ===\n1. Criar grupo\n2. Remover grupo\n3. Adicionar contato ao grupo\n"
             << "4. Remover contato do grupo\n5. Listar grupos e membros\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 5);
        if (opcao == 1) criarGrupoMenu(a);
        else if (opcao == 2) removerGrupoMenu(a);
        else if (opcao == 3) adicionarAoGrupoMenu(a);
        else if (opcao == 4) removerDoGrupoMenu(a);
        else if (opcao == 5) listarGrupos(a.grupos);
    } while (opcao != 0);
}

void registrarChamadaMenu(Agenda& a) {
    Contato* c = lerContato(a);
    if (c == nullptr) return;
    for (int i = 0; i < TOTAL_TIPOS; i++) cout << "  " << (i + 1) << ") " << TIPOS[i] << "\n";
    int tipo = lerInteiro("Tipo da chamada", 1, TOTAL_TIPOS);
    if (!cin) return;
    empilharChamada(a.historico, a.proximaChamada++, c, TIPOS[tipo - 1]);
    cout << "  Chamada registrada.\n";
}

void menuHistorico(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== HISTORICO DE CHAMADAS ===\n1. Registrar chamada\n2. Ver a ultima chamada\n"
             << "3. Desfazer (remover) a ultima chamada\n4. Listar historico\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 4);
        const NoChamada* topo = consultarTopo(a.historico);
        if (opcao == 1) registrarChamadaMenu(a);
        else if (opcao == 4) listarHistorico(a.historico);
        else if ((opcao == 2 || opcao == 3) && topo == nullptr) cout << "  Historico vazio.\n";
        else if (opcao == 2) exibirChamada(topo);
        else if (opcao == 3) {
            exibirChamada(topo);  // mostra qual chamada esta sendo desfeita
            desempilharChamada(a.historico);
            cout << "  Chamada removida.\n";
        }
    } while (opcao != 0);
}

void menuFila(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== FILA DE RETORNO ===\n1. Adicionar contato a fila\n2. Atender proximo da fila\n"
             << "3. Listar retornos pendentes\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 3);
        if (opcao == 1) {
            Contato* c = lerContato(a);
            if (c == nullptr) continue;
            if (enfileirarRetorno(a.retornos, c)) cout << "  " << c->nome << " entrou na fila.\n";
            else cout << "  Esse contato ja esta na fila.\n";
        } else if (opcao == 2 && a.retornos.inicio == nullptr) {
            cout << "  Nenhum retorno pendente.\n";
        } else if (opcao == 2) {
            cout << "  Atendido: ";
            exibirRetorno(a.retornos.inicio);
            desenfileirarRetorno(a.retornos);
        } else if (opcao == 3) {
            listarRetornos(a.retornos);
        }
    } while (opcao != 0);
}

void menuOrdenar(Agenda& a) {
    if (a.total < 2) {
        cout << "  E preciso ter pelo menos 2 contatos.\n";
        return;
    }
    int campo = lerCampo("Ordenar por");
    if (!cin) return;
    long comparacoes = 0;
    insertionSort(a.contatos, a.total, campo, &comparacoes);
    cout << "  " << a.total << " contatos ordenados por " << CAMPOS[campo - 1]
         << " (Insertion Sort, " << comparacoes << " comparacoes).\n";
    listarContatos(a, 10);
}

void menuArvore(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== ARVORE BINARIA DE BUSCA ===\n1. Listar em ordem alfabetica (in-order)\n"
             << "2. Buscar por nome\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 2);
        if (opcao == 1) {
            if (a.raiz == nullptr) cout << "  Arvore vazia.\n";
            else percorrerEmOrdem(a.raiz);
        } else if (opcao == 2) {
            string nome = lerTexto("Nome (exato, sem diferenciar maiusculas)");
            long comparacoes = 0;
            int achados = buscarNaArvore(a.raiz, nome, &comparacoes);
            cout << "  Encontrados: " << achados << " | Comparacoes: " << comparacoes << "\n";
        }
    } while (opcao != 0);
}

void menuPrincipal(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== AGENDA DE CONTATOS ===\n1. Contatos\n2. Buscar contatos\n";
        cout << "3. Grupos\n4. Historico de chamadas\n5. Fila de retorno\n";
        cout << "6. Ordenar contatos\n7. Arvore binaria (por nome)\n";
        cout << "0. Sair\n";
        opcao = lerInteiro("Opcao", 0, 7);
        if (opcao == 1) menuContatos(a);
        else if (opcao == 2) menuBuscar(a);
        else if (opcao == 3) menuGrupos(a);
        else if (opcao == 4) menuHistorico(a);
        else if (opcao == 5) menuFila(a);
        else if (opcao == 6) menuOrdenar(a);
        else if (opcao == 7) menuArvore(a);
    } while (opcao != 0);
    cout << "\nEncerrando...\n";
}

int main() {
    Agenda agenda;
    menuPrincipal(agenda);
    liberarAgenda(agenda);
    return 0;
}
