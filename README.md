# Agenda de Contatos em C++

Agenda telefônica no terminal, em **arquivo único** (`src/main.cpp`, sem arquivos `.h`), com as estruturas de dados implementadas à mão.

## Compilar e executar

    make
    ./agenda

Sem `make`: `g++ -std=c++17 -Wall -Wextra -pedantic -o agenda src/main.cpp`

## Menu principal

| Opção | Função |
|---|---|
| 1 | Contatos: cadastrar, listar e excluir |
| 2 | Buscar contatos |
| 3 | Grupos: criar, remover, adicionar/remover contato, listar |
| 4 | Histórico de chamadas: registrar, ver a última, desfazer, listar |
| 5 | Fila de retorno: adicionar, atender o próximo, listar |
| 6 | Ordenar contatos (Insertion Sort) |
| 7 | Árvore binária de busca: listar em ordem alfabética e buscar por nome |
| 8 | Comparar buscas (sequencial x binária x árvore) |
| 0 | Sair |

## Estruturas de dados

| Estrutura | Onde é usada | Tipo |
|---|---|---|
| Vetor de ponteiros (capacidade fixa `MAX`) | Contatos (cada `Contato` criado com `new`) | `Contato* contatos[MAX]` |
| Lista simplesmente encadeada | Grupos e membros de cada grupo | `NoGrupo`, `NoMembro` |
| Pilha (LIFO) | Histórico de chamadas (a mais recente no topo) | `Pilha`, `NoChamada` |
| Fila (FIFO) | Retornos de ligação (quem entrou primeiro é atendido primeiro) | `Fila`, `NoRetorno` |
| Árvore binária de busca | Contatos indexados pelo nome | `NoArvore` |

## Requisitos da avaliação

| Req. | Tema | Onde aparece (funções) |
|---|---|---|
| R1 | Funções, passagem por valor/referência, parâmetro default | `lerTexto` (default `obrigatorio = true`), `listarContatos` (default `maximo = MAX`), `adicionarContato(Agenda&, const Contato&)`, `buscaSequencial` (default `inicio = 0`) |
| R2 | Ponteiros | `Contato* contatos[MAX]`, `buscaSequencial` (`long* comparacoes`), `valorDoCampo` (`const Contato*` e retorno `const string&`) |
| R3 | Recursividade | `inserirNaArvore`, `buscarNaArvore`, `percorrerEmOrdem`, `liberarArvore` |
| R4 | `new` / `delete` | `adicionarContato` (`new Contato`), `excluirContato` e `liberarAgenda` (`delete`) |
| R5 | Lista encadeada | `criarGrupo`, `removerGrupo`, `buscarGrupo`, `adicionarMembro`, `removerMembro`, `listarGrupos` |
| R6 | Pilha (LIFO) | `empilharChamada`, `consultarTopo`, `desempilharChamada`, `listarHistorico` |
| R7 | Fila (FIFO) | `enfileirarRetorno`, `desenfileirarRetorno`, `listarRetornos` |
| R8 | Árvore binária de busca | `inserirNaArvore`, `buscarNaArvore`, `percorrerEmOrdem` (in-order), `reconstruirArvore` |
| R9 | Ordenação com análise de complexidade | `insertionSort` (análise no comentário e na tabela abaixo) |
| R10 | Busca sequencial, binária e comparação por contagem | `buscaSequencial`, `buscaBinaria`, `copiarOrdenado`, `compararBuscas` (menu 8) |

## Análise de complexidade

| Algoritmo | Melhor caso | Caso médio | Pior caso | Memória extra | Estável |
|---|---|---|---|---|---|
| Insertion Sort | O(n) | O(n²) | O(n²) | O(1) | Sim |
| Busca sequencial | O(1) | O(n) | O(n) | O(1) | — |
| Busca binária (vetor ordenado) | O(1) | O(log n) | O(log n) | O(1) | — |
| Árvore binária de busca (sem balanceamento) | O(1) | O(log n) | O(n) | O(1) | — |

A busca binária exige o vetor ordenado: ordenar custa O(n²) com Insertion Sort, mas é feito uma única vez e depois cada busca custa O(log n). O menu 8 mostra a diferença pela contagem de comparações.

## Testes

O arquivo `tests/roteiro_completo.txt` reúne as entradas de uma sessão que passa por todos os menus (inclui a geração de contatos de teste, até encher a agenda, para comparar as buscas):

    ./agenda < tests/roteiro_completo.txt

Para procurar erros de memória e vazamentos:

    make teste
