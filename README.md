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
| 0 | Sair |

## Estruturas de dados

| Estrutura | Onde é usada | Tipo |
|---|---|---|
| Vetor de ponteiros (capacidade fixa `MAX`) | Contatos (cada `Contato` criado com `new`) | `Contato* contatos[MAX]` |

## Requisitos da avaliação

| Req. | Tema | Onde aparece (funções) |
|---|---|---|
| R1 | Funções, passagem por valor/referência, parâmetro default | `lerTexto` (default `obrigatorio = true`), `listarContatos` (default `maximo = MAX`), `adicionarContato(Agenda&, const Contato&)`, `buscaSequencial` (default `inicio = 0`) |
| R2 | Ponteiros | `Contato* contatos[MAX]`, `buscaSequencial` (`long* comparacoes`), `valorDoCampo` (`const Contato*` e retorno `const string&`) |
| R4 | `new` / `delete` | `adicionarContato` (`new Contato`), `excluirContato` e `liberarAgenda` (`delete`) |
