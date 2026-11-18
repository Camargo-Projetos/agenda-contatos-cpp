// Agenda de contatos no terminal (arquivo unico).
// Compilar: g++ -std=c++17 -Wall -Wextra -pedantic -o agenda src/main.cpp
#include <iostream>
#include <string>

using namespace std;

const int MAX = 1000;  // o vetor de contatos tem tamanho fixo
const int ID_MAXIMO = 999999999;

// Campos que podem ser usados nas buscas e na ordenacao
enum Campo { CAMPO_NOME = 1, CAMPO_TELEFONE, CAMPO_EMAIL, CAMPO_EMPRESA };
const char* const CAMPOS[] = {"Nome", "Telefone", "E-mail", "Empresa"};
const int TOTAL_TIPOS = 3;
const char* const TIPOS[TOTAL_TIPOS] = {"Realizada", "Recebida", "Perdida"};

// Estruturas de dados

struct Contato {
    int id = 0;
    string nome, telefone, email, empresa;
};

// Os grupos formam uma lista, e cada grupo tem a sua propria lista de membros.
// Os membros sao so ponteiros, entao nenhum contato e copiado.
struct NoMembro {
    Contato* contato;
    NoMembro* proximo;
};
struct NoGrupo {
    int id;
    string nome;
    NoMembro* membros;  // primeiro membro da lista
    NoGrupo* proximo;
};

// A pilha do historico e a fila de retornos guardam uma COPIA do nome e do telefone.
// Assim elas continuam funcionando mesmo que o contato seja excluido depois.
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
    NoRetorno* inicio = nullptr;  // quem vai ser atendido primeiro
    NoRetorno* fim = nullptr;     // quem entrou por ultimo
};

// No da arvore binaria de busca, organizada pelo nome.
// Guarda so o ponteiro do contato, sem copiar nada.
struct NoArvore {
    Contato* contato;
    NoArvore* esquerda;
    NoArvore* direita;
};

// Junta todos os dados da agenda num lugar so. Ela e passada por referencia (&)
// para que as funcoes mexam no original e nao numa copia.
struct Agenda {
    Contato* contatos[MAX] = {};  // vetor de ponteiros; cada Contato e criado com new
    int total = 0;
    int proximoId = 1;
    NoGrupo* grupos = nullptr;  // primeiro grupo da lista
    int proximoGrupo = 1;
    Pilha historico;
    int proximaChamada = 1;
    Fila retornos;
    NoArvore* raiz = nullptr;  // arvore de contatos organizada pelo nome
};

// Entrada de dados

// Le uma linha de texto. Com obrigatorio = true (o padrao), insiste ate o usuario
// digitar alguma coisa. Se a entrada acabar (Ctrl+D), sai do laco e devolve "".
string lerTexto(const string& rotulo, bool obrigatorio = true) {
    string linha;
    cout << rotulo << ": ";
    while (getline(cin, linha)) {
        if (!linha.empty() || !obrigatorio) return linha;
        cout << "  Campo obrigatorio.\n" << rotulo << ": ";
    }
    return "";
}

// Le um inteiro entre minimo e maximo e pergunta de novo enquanto o valor nao servir.
// Se a entrada acabar, devolve o minimo.
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

// Pergunta sim ou nao. So 's' ou 'S' conta como sim.
bool confirmar(const string& pergunta) {
    string resposta = lerTexto(pergunta + " (s/n)");
    return !resposta.empty() && (resposta[0] == 's' || resposta[0] == 'S');
}

// Comparacao e campos

// Passa a letra para minuscula (so vale para ASCII), pra comparar sem ligar pra maiuscula.
char minuscula(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// Compara dois textos ignorando maiusculas e minusculas.
// Devolve -1 se a vem antes de b, 0 se sao iguais e 1 se a vem depois.
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

// Devolve o campo pedido do contato. O retorno e uma referencia (&) pra nao copiar a string,
// e o contato chega como ponteiro const (*) porque aqui a gente so le, nunca altera.
const string& valorDoCampo(const Contato* c, int campo) {
    if (campo == CAMPO_TELEFONE) return c->telefone;
    if (campo == CAMPO_EMAIL) return c->email;
    if (campo == CAMPO_EMPRESA) return c->empresa;
    return c->nome;
}

// Mostra os campos numerados e le qual o usuario escolheu.
int lerCampo(const string& rotulo) {
    for (int i = CAMPO_NOME; i <= CAMPO_EMPRESA; i++) cout << "  " << i << ") " << CAMPOS[i - 1] << "\n";
    return lerInteiro(rotulo, CAMPO_NOME, CAMPO_EMPRESA);
}

// Mostra um contato em uma linha so.
void exibirContato(const Contato* c) {
    cout << "  [" << c->id << "] " << c->nome << " | " << c->telefone << " | "
         << (c->email.empty() ? "-" : c->email) << " | " << (c->empresa.empty() ? "-" : c->empresa) << "\n";
}

// Grupos: lista simplesmente encadeada [R5]

// Coloca um grupo no FIM da lista. 'lista' e uma referencia (&) ao ponteiro da cabeca:
// se a lista estiver vazia, o ponteiro de quem chamou passa a apontar pro novo no.
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

// Anda pela lista ate achar o grupo com aquele ID. 'lista' vai por valor, entao
// andar com ele nao muda a cabeca da lista de quem chamou. Devolve o no ou nullptr.
NoGrupo* buscarGrupo(NoGrupo* lista, int id) {
    while (lista != nullptr && lista->id != id) lista = lista->proximo;
    return lista;
}

// Tira um contato da lista de membros do grupo (o contato em si NAO e apagado).
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

// Poe um contato no fim da lista de membros. Devolve false se ele ja estava la.
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

// Percorre a lista mostrando cada grupo com os seus membros.
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

// Pilha (LIFO): historico de chamadas [R6]

// push: o novo no aponta pro antigo topo e passa a ser o topo. 'p' vai por referencia (&)
// porque o topo muda.
void empilharChamada(Pilha& p, int numero, const Contato* c, const string& tipo) {
    p.topo = new NoChamada{numero, c->nome, c->telefone, tipo, p.topo};
}

// peek: da uma olhada no topo sem tirar nada. Devolve nullptr se a pilha estiver vazia.
const NoChamada* consultarTopo(const Pilha& p) {
    return p.topo;
}

// pop: tira o topo (a chamada mais recente). Devolve false se a pilha estiver vazia.
bool desempilharChamada(Pilha& p) {
    if (p.topo == nullptr) return false;
    NoChamada* removido = p.topo;
    p.topo = removido->proximo;
    delete removido;
    return true;
}

// Mostra uma chamada em uma linha so.
void exibirChamada(const NoChamada* n) {
    cout << "  #" << n->numero << " | " << n->tipo << " | " << n->nome << " (" << n->telefone << ")\n";
}

// Lista da mais recente (topo) para a mais antiga, sem mexer na pilha.
void listarHistorico(const Pilha& p) {
    if (p.topo == nullptr) {
        cout << "  Historico vazio.\n";
        return;
    }
    for (const NoChamada* n = p.topo; n != nullptr; n = n->proximo) exibirChamada(n);
}

// Esvazia a pilha, liberando os nos.
void liberarPilha(Pilha& p) {
    while (desempilharChamada(p)) {}
}

// Fila (FIFO): retornos de ligacao [R7]

// enqueue: poe no FIM da fila. Devolve false se o contato ja estava esperando.
bool enfileirarRetorno(Fila& f, const Contato* c) {
    for (const NoRetorno* n = f.inicio; n != nullptr; n = n->proximo)
        if (n->idContato == c->id) return false;
    NoRetorno* novo = new NoRetorno{c->id, c->nome, c->telefone, nullptr};
    if (f.fim == nullptr) f.inicio = novo;
    else f.fim->proximo = novo;
    f.fim = novo;
    return true;
}

// dequeue: tira o PRIMEIRO da fila (quem chegou antes). Devolve false se a fila estiver vazia.
bool desenfileirarRetorno(Fila& f) {
    if (f.inicio == nullptr) return false;
    NoRetorno* removido = f.inicio;
    f.inicio = removido->proximo;
    if (f.inicio == nullptr) f.fim = nullptr;  // a fila esvaziou, entao o fim tambem zera
    delete removido;
    return true;
}

// Mostra um retorno pendente em uma linha so.
void exibirRetorno(const NoRetorno* n) {
    cout << "[" << n->idContato << "] " << n->nome << " (" << n->telefone << ")\n";
}

// Lista os retornos na ordem em que vao ser atendidos.
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

// Esvazia a fila, liberando os nos.
void liberarFila(Fila& f) {
    while (desenfileirarRetorno(f)) {}
}

// Arvore binaria de busca (recursiva) [R3, R8]
// Por que uma arvore: organizada pelo nome, a busca custa O(log n) em media, e o percurso
// in-order ja entrega os contatos em ordem alfabetica sem precisar ordenar o vetor.
// Nomes iguais vao para a direita (homonimos).

// Insere um contato de forma recursiva. Devolve a raiz da subarvore depois da insercao.
NoArvore* inserirNaArvore(NoArvore* raiz, Contato* c) {
    if (raiz == nullptr) return new NoArvore{c, nullptr, nullptr};
    if (comparar(c->nome, raiz->contato->nome) < 0) raiz->esquerda = inserirNaArvore(raiz->esquerda, c);
    else raiz->direita = inserirNaArvore(raiz->direita, c);
    return raiz;
}

// Busca recursiva pelo nome, achando todos os homonimos. 'comparacoes' e um ponteiro (*)
// pra que o mesmo contador seja somado em todas as chamadas recursivas; 'exibir' vale true
// por padrao. Devolve quantos contatos foram encontrados.
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

// Libera os nos recursivamente (pos-ordem). Os contatos NAO sao apagados.
// 'raiz' vai por referencia (&) pra ficar nullptr no final.
void liberarArvore(NoArvore*& raiz) {
    if (raiz == nullptr) return;
    liberarArvore(raiz->esquerda);
    liberarArvore(raiz->direita);
    delete raiz;
    raiz = nullptr;
}

// Ordenacao [R9]

// Insertion Sort: pega cada elemento e encaixa na posicao certa da parte que ja esta ordenada.
// Complexidade (n = quantidade de contatos):
//   melhor caso O(n)    - vetor ja ordenado (so 1 comparacao por elemento)
//   caso medio  O(n^2)
//   pior caso   O(n^2)  - vetor em ordem inversa
//   memoria extra O(1)  - ordena no proprio vetor, e e estavel.
// 'v' e um vetor de ponteiros (Contato**): so os ponteiros trocam de lugar, nao os contatos.
// 'comparacoes' e um ponteiro (*) pra quem chamou receber a contagem.
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

// Busca [R10]

// Busca sequencial: anda pelo vetor do comeco ao fim. Melhor caso O(1), medio e pior O(n),
// e funciona mesmo com o vetor desordenado. 'inicio' vale 0 por padrao e serve pra achar
// as proximas ocorrencias. Devolve o indice da primeira ocorrencia ou -1.
int buscaSequencial(Contato** v, int n, int campo, const string& chave, long* comparacoes, int inicio = 0) {
    for (int i = inicio; i < n; i++) {
        (*comparacoes)++;
        if (comparar(valorDoCampo(v[i], campo), chave) == 0) return i;
    }
    return -1;
}

// Busca binaria: compara com o elemento do meio e joga fora metade do vetor a cada passo.
// Complexidade: melhor O(1), medio e pior O(log n). Precisa do vetor ORDENADO pelo mesmo campo.
// Quando acha, continua pela esquerda pra devolver a PRIMEIRA ocorrencia. Devolve o indice ou -1.
int buscaBinaria(Contato** v, int n, int campo, const string& chave, long* comparacoes) {
    int esquerda = 0, direita = n - 1, achado = -1;
    while (esquerda <= direita) {
        int meio = esquerda + (direita - esquerda) / 2;
        (*comparacoes)++;
        int cmp = comparar(valorDoCampo(v[meio], campo), chave);
        if (cmp == 0) {
            achado = meio;
            direita = meio - 1;
        } else if (cmp < 0) {
            esquerda = meio + 1;
        } else {
            direita = meio - 1;
        }
    }
    return achado;
}

// Copia os ponteiros da agenda pra 'destino' e ordena so a COPIA, entao a ordem da agenda nao muda.
void copiarOrdenado(const Agenda& a, Contato** destino, int campo, long* comparacoes) {
    for (int i = 0; i < a.total; i++) destino[i] = a.contatos[i];
    insertionSort(destino, a.total, campo, comparacoes);
}

// Faz a busca (metodo 1 = sequencial, 2 = binaria) e mostra todos os contatos que acharem.
// Devolve quantos foram encontrados.
int buscarEExibir(Contato** v, int n, int campo, const string& chave, int metodo, long* comparacoes) {
    int achados = 0;
    int pos = (metodo == 1) ? buscaSequencial(v, n, campo, chave, comparacoes)
                            : buscaBinaria(v, n, campo, chave, comparacoes);
    while (pos != -1) {
        exibirContato(v[pos]);
        achados++;
        if (metodo == 1) {
            pos = buscaSequencial(v, n, campo, chave, comparacoes, pos + 1);
        } else {  // no vetor ordenado, os repetidos ficam um do lado do outro
            pos++;
            (*comparacoes)++;
            if (pos >= n || comparar(valorDoCampo(v[pos], campo), chave) != 0) pos = -1;
        }
    }
    return achados;
}

// Operacoes da agenda

// Procura em que posicao do vetor esta o contato com aquele ID. Devolve o indice ou -1.
int indiceDoContato(const Agenda& a, int id) {
    for (int i = 0; i < a.total; i++)
        if (a.contatos[i]->id == id) return i;
    return -1;
}

// Refaz a arvore do zero a partir do vetor. Usado na exclusao, que e mais simples
// do que tirar um no da arvore.
void reconstruirArvore(Agenda& a) {
    liberarArvore(a.raiz);
    for (int i = 0; i < a.total; i++) a.raiz = inserirNaArvore(a.raiz, a.contatos[i]);
}

// Cria o contato com new e guarda no vetor ('dados' vai por referencia const &, sem copia).
// Devolve o ponteiro do contato criado ou nullptr se a agenda estiver cheia.
Contato* adicionarContato(Agenda& a, const Contato& dados) {
    if (a.total >= MAX) return nullptr;
    Contato* novo = new Contato(dados);
    novo->id = a.proximoId++;
    a.contatos[a.total++] = novo;
    a.raiz = inserirNaArvore(a.raiz, novo);
    return novo;
}

// Exclui um contato: tira dos grupos e da arvore e so depois libera a memoria com delete.
// Devolve false se o ID nao existe.
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

// Cria contatos de teste ("Contato N") so pra comparar as buscas. A conta embaralha os
// numeros pra arvore nao virar uma lista.
void gerarContatosTeste(Agenda& a, int quantidade) {
    for (int k = 0; k < quantidade; k++) {
        int n = (a.proximoId * 7919) % 10007;
        Contato c;
        c.nome = "Contato " + to_string(n);
        c.telefone = "9" + to_string(100000000 + n);
        c.email = "contato" + to_string(n) + "@teste.com";
        c.empresa = "Empresa " + to_string(n % 20);
        adicionarContato(a, c);
    }
}

// Libera toda a memoria da agenda. Os contatos ficam por ultimo, porque grupos e arvore apontam pra eles.
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

// Lista os contatos do vetor (no maximo 'maximo'; por padrao, todos).
void listarContatos(const Agenda& a, int maximo = MAX) {
    if (a.total == 0) {
        cout << "  Nenhum contato cadastrado.\n";
        return;
    }
    for (int i = 0; i < a.total && i < maximo; i++) exibirContato(a.contatos[i]);
    cout << "  Total: " << a.total << " contato(s).\n";
}

// Menus

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
    if (!cin) return;  // a entrada acabou no meio do cadastro
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

// Busca por nome, telefone, e-mail ou empresa, com busca sequencial ou binaria.
void menuBuscar(Agenda& a) {
    if (a.total == 0) {
        cout << "  Nenhum contato cadastrado.\n";
        return;
    }
    int campo = lerCampo("Buscar por");
    string chave = lerTexto("Valor procurado (exato, sem diferenciar maiusculas)");
    int metodo = lerInteiro("Metodo (1 = sequencial, 2 = binaria)", 1, 2);
    if (!cin) return;
    Contato* copia[MAX];
    Contato** v = a.contatos;
    long comparacoes = 0, custoOrdenar = 0;
    if (metodo == 2) {  // a binaria so funciona com o vetor ordenado
        copiarOrdenado(a, copia, campo, &custoOrdenar);
        v = copia;
    }
    int achados = buscarEExibir(v, a.total, campo, chave, metodo, &comparacoes);
    cout << "  Encontrados: " << achados << " | Comparacoes: " << comparacoes << "\n";
    if (metodo == 2) cout << "  (ordenar a copia custou " << custoOrdenar << " comparacoes)\n";
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

// Compara as tres buscas com o mesmo valor, olhando quantas comparacoes cada uma fez.
void compararBuscas(Agenda& a) {
    int campo = lerCampo("Campo");
    string chave = lerTexto("Valor procurado");
    if (!cin) return;
    Contato* copia[MAX];
    long seq = 0, bin = 0, arv = 0, custoOrdenar = 0;
    copiarOrdenado(a, copia, campo, &custoOrdenar);
    int iSeq = buscaSequencial(a.contatos, a.total, campo, chave, &seq);
    int iBin = buscaBinaria(copia, a.total, campo, chave, &bin);
    cout << "  n = " << a.total << " | campo: " << CAMPOS[campo - 1] << " | valor: " << chave << "\n";
    cout << "  Sequencial: " << seq << " comparacoes (achou: " << (iSeq != -1 ? "sim" : "nao") << ")\n";
    cout << "  Binaria:    " << bin << " comparacoes (achou: " << (iBin != -1 ? "sim" : "nao") << ")\n";
    if (campo == CAMPO_NOME) {  // a arvore so e organizada pelo nome
        int achados = buscarNaArvore(a.raiz, chave, &arv, false);
        cout << "  Arvore:     " << arv << " comparacoes (achou: " << (achados > 0 ? "sim" : "nao") << ")\n";
    }
    cout << "  Ordenar para a binaria custa " << custoOrdenar << " comparacoes, uma unica vez.\n";
}

void menuComparar(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== COMPARAR BUSCAS ===\n1. Gerar contatos de teste\n"
             << "2. Comparar sequencial, binaria e arvore\n0. Voltar\n";
        opcao = lerInteiro("Opcao", 0, 2);
        if (opcao == 1 && a.total >= MAX) {
            cout << "  Agenda cheia.\n";
        } else if (opcao == 1) {
            int quantidade = lerInteiro("Quantos contatos gerar", 1, MAX - a.total);
            if (!cin) continue;
            gerarContatosTeste(a, quantidade);
            cout << "  " << quantidade << " gerados. Total: " << a.total << ". Ultimo: " << a.contatos[a.total - 1]->nome << ".\n";
        } else if (opcao == 2) {
            if (a.total == 0) cout << "  Nenhum contato cadastrado.\n";
            else compararBuscas(a);
        }
    } while (opcao != 0);
}

void menuPrincipal(Agenda& a) {
    int opcao;
    do {
        cout << "\n=== AGENDA DE CONTATOS ===\n1. Contatos\n2. Buscar contatos\n";
        cout << "3. Grupos\n4. Historico de chamadas\n5. Fila de retorno\n";
        cout << "6. Ordenar contatos\n7. Arvore binaria (por nome)\n";
        cout << "8. Comparar buscas\n";
        cout << "0. Sair\n";
        opcao = lerInteiro("Opcao", 0, 8);
        if (opcao == 1) menuContatos(a);
        else if (opcao == 2) menuBuscar(a);
        else if (opcao == 3) menuGrupos(a);
        else if (opcao == 4) menuHistorico(a);
        else if (opcao == 5) menuFila(a);
        else if (opcao == 6) menuOrdenar(a);
        else if (opcao == 7) menuArvore(a);
        else if (opcao == 8) menuComparar(a);
    } while (opcao != 0);
    cout << "\nEncerrando...\n";
}

int main() {
    Agenda agenda;
    menuPrincipal(agenda);
    liberarAgenda(agenda);
    return 0;
}
