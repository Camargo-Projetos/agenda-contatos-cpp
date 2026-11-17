# Relatório Técnico: Agenda de Contatos em C++

Programa de terminal em arquivo único (`src/main.cpp`, 800 linhas, C++17), que usa apenas `<iostream>` e `<string>`. Todas as estruturas de dados foram implementadas manualmente, sem contêineres da biblioteca padrão.

## 1. Estruturas de dados e justificativas

| Estrutura | Uso no sistema | Motivo da escolha | Custo principal |
|---|---|---|---|
| Vetor de ponteiros (`Contato* contatos[MAX]`, `MAX = 1000`) | Cadastro de contatos | Acesso por índice em O(1); permite ordenação e busca binária; ordenar troca só ponteiros, não objetos | Exclusão O(n) (deslocamento); limite de 1000 contatos |
| Lista simplesmente encadeada | Grupos e membros de cada grupo | Número de grupos e membros é imprevisível; remover no meio não exige deslocar elementos | Busca O(n); inserção no fim O(n) |
| Pilha (LIFO) | Histórico de chamadas | A chamada mais recente é a primeira a ser consultada ou desfeita (`empilharChamada`, `consultarTopo`, `desempilharChamada`) | Todas as operações O(1) |
| Fila (FIFO) | Retornos de ligação | Quem pediu retorno primeiro é atendido primeiro; o ponteiro `fim` torna o enfileiramento direto | Atender O(1); verificar duplicado ao enfileirar O(n) |
| Árvore binária de busca (chave: nome) | Acesso ordenado aos contatos | Busca por nome em O(log n) em média; o percurso in-order lista os contatos em ordem alfabética sem ordenar o vetor | Pior caso O(n), pois não há balanceamento |

Decisões complementares:

- A capacidade fixa do vetor evita realocação e deixa o código mais simples de explicar.
- A pilha e a fila guardam **cópia** do nome e do telefone, por isso continuam válidas se o contato for excluído. Os grupos e a árvore guardam **ponteiros**, sem duplicar dados.
- Nomes iguais vão para a subárvore da direita, o que permite homônimos.
- Ao excluir um contato, a árvore é reconstruída a partir do vetor, em vez de remover um nó. É menos eficiente, porém mais curto e menos sujeito a erro.
- A árvore é recursiva (inserção, busca, percurso in-order e liberação em pós-ordem).

## 2. Algoritmos de ordenação e busca

**Ordenação: Insertion Sort**, escrito do zero (`insertionSort`). Escolhido por ser o mais curto e o mais fácil de explicar. É estável, trabalha no próprio vetor e permite ordenar por nome, telefone, e-mail ou empresa. Para `n` ≤ 1000, o custo quadrático é aceitável (no máximo da ordem de 10⁶ comparações).

| Algoritmo | Melhor caso | Caso médio | Pior caso | Memória extra | Estável |
|---|---|---|---|---|---|
| Insertion Sort | O(n) (vetor já ordenado) | O(n²) | O(n²) (ordem inversa) | O(1) | Sim |
| Busca sequencial | O(1) | O(n) | O(n) | O(1) | — |
| Busca binária (vetor ordenado) | O(1) | O(log n) | O(log n) | O(1) | — |
| Árvore binária de busca | O(1) | O(log n) | O(n) | O(1) | — |

A busca binária exige o vetor ordenado. O sistema ordena uma **cópia** dos ponteiros (`copiarOrdenado`), mantendo a ordem original da agenda. Esse custo ocorre uma vez e compensa quando há várias buscas.

**Demonstração por contagem de comparações** (menu 8, com 503 contatos, procurando o último contato do vetor):

| Método | Comparações |
|---|---|
| Sequencial | 503 |
| Binária | 9 |
| Árvore | 13 |

Ordenar a cópia para a busca binária custou cerca de 64 mil comparações, valor próximo de n²/4, esperado para o caso médio do Insertion Sort. Os números são reproduzíveis com `tests/roteiro_completo.txt`.

## 3. Gerenciamento de memória

- **Alocação e liberação:** toda memória dinâmica usa `new` e `delete` (contatos e nós de grupo, membro, pilha, fila e árvore). Não há `malloc` nem contêineres da biblioteca padrão.
- **Quem é dono de quê:** o vetor `contatos` é o único dono dos objetos `Contato`. Grupos e árvore apenas apontam para eles e nunca os apagam. Pilha e fila têm dados próprios.
- **Exclusão de um contato:** antes do `delete`, o contato é retirado de todos os grupos, removido do vetor e a árvore é reconstruída. Isso evita ponteiros soltos.
- **Encerramento (`liberarAgenda`):** libera primeiro grupos, pilha, fila e árvore, e **por último** os contatos, porque as demais estruturas apontam para eles. A ordem é essencial: liberar os contatos antes faria os grupos lerem memória já liberada.
- **Verificação:** o programa compila sem avisos com `-Wall -Wextra -pedantic`. O comando `make teste` compila com `-fsanitize=address,undefined` e executa o roteiro que percorre todos os menus, sem erros e sem vazamentos. O programa também foi testado com a entrada encerrada em qualquer ponto do roteiro.

## 4. Limitações conhecidas

- Capacidade máxima de 1000 contatos.
- Os dados não são gravados em arquivo; ao sair do programa, eles são perdidos.
- A árvore não é balanceada. Se o vetor estiver ordenado por nome no momento em que um contato for excluído, a reconstrução gera uma árvore em forma de lista (altura n) e a busca nela passa a O(n).
- Não há validação de formato: nome e telefone só precisam não estar vazios, e e-mail e empresa são opcionais.
