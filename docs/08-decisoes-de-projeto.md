# 08 — Decisões de projeto

← Anterior: [07 — CI e instalador](07-ci-e-instalador.md) · [Índice](README.md) · Próximo: [09 — Contribuindo](09-contribuindo.md)

> Este módulo reúne **o porquê** das escolhas importantes, no formato de registros de decisão: contexto, decisão, alternativas e consequências. Serve para quem quer entender o projeto e para quem precisa mudar algo sem desfazer uma escolha consciente.

---

## D1 — Lista encadeada ordenada como estrutura central

- **Contexto:** o enunciado exige uma Lista Dinâmica Ordenada adaptada da desordenada vista em aula.
- **Decisão:** lista simplesmente encadeada, inserção já na posição certa, chave = resistência.
- **Alternativas:** vetor ordenado puro (deixaria de ser lista dinâmica); árvore de busca (fora do escopo).
- **Consequências:** listagem sem ordenação; a busca é binária graças ao vetor de ponteiros da [D4](#d4--busca-binária-com-vetor-de-ponteiros).

## D2 — Nó descritor com totais

- **Contexto:** o menu mostra tipos e peças e é redesenhado a cada tecla.
- **Decisão:** um struct descritor com `inicio`, `indice`, `capacidade`, `tipos`, `pecas` e `passos`, atualizado a cada operação.
- **Alternativa:** recontar a lista a cada desenho (O(n) por tecla).
- **Consequências:** totais em O(1); toda operação precisa manter os contadores (garantido por `teste_contadores_invariantes`).

## D3 — Tipos opacos (TAD)

- **Decisão:** `Lista`, `Pilha` e `Checkpoint` são declarados no `.h` sem os campos.
- **Por quê:** ninguém fora do módulo consegue quebrar a ordenação, os totais ou a posse da memória.
- **Custo:** acesso só por funções (inclusive `percorre_lista` com callback para listar).

## D4 — Busca binária com vetor de ponteiros

- **Contexto:** comparar termo a termo ignora que a lista está sempre ordenada, mas nó encadeado não tem acesso direto ao meio.
- **Decisão:** manter os nós encadeados e acrescentar ao descritor `indice`, um vetor dinâmico com o endereço de cada nó em ordem. `localiza` faz a busca binária nele e devolve a posição do nó ou o ponto de inserção.
- **Alternativas:** vetor puro de `Resistor` (deixa de ser lista dinâmica); busca binária andando pelos nós (menos comparações, mas O(n) passos até cada meio, sem ganho real). Substituiu o ponteiro para ponteiro usado antes em `localiza`.
- **Consequências:** toda busca custa O(log n); inserir e remover ainda pagam O(n) para deslocar o vetor; a lista tem duas representações que precisam andar juntas, o que os testes de ordem e remoção conferem.

## D5 — Resistência em `double`, em ohms, com chave canônica

- **Contexto:** valores de 0,001 a 1.000.000.000 Ω, com casas decimais.
- **Decisão:** guardar em `double` sempre em Ω; converter o texto com um único `strtod` sobre a forma "A.Be<exp>"; comparar com tolerância relativa 1e-9.
- **Alternativas:** inteiro em mΩ (1000 MΩ = 10¹² mΩ não cabe em `int`); multiplicar por 0,001 (gera `double` diferentes para a mesma grafia).
- **Consequências:** `470m` e `0.47` são a mesma chave; nenhum duplicado invisível.

## D6 — Regra de negócio separada da interface

- **Decisão:** `lista`, `pilha`, `checkpoint` e `formatacao` não imprimem; devolvem códigos. `telas` traduz para mensagens.
- **Consequências:** módulos testáveis isoladamente; interface trocável; relatório mais claro.

## D7 — Persistência em SQLite só ao abrir e ao sair

- **Contexto:** um controle de estoque que esquece tudo ao fechar não serve para uso real. O enunciado pede um item diferencial, e a persistência faz parte dele.
- **Decisão:** a lista continua sendo a estrutura de trabalho; o banco ([database/](../database/README.md)) é lido uma vez ao abrir e regravado por inteiro, numa transação, ao sair. SQLite ligado de forma estática, schema embutido a partir de `database/database.sql`.
- **Alternativas:** gravar a cada operação (mais acessos ao disco, foi descartado de propósito); arquivo próprio (exigiria formato e parser).
- **Consequências:** o menu nunca espera por disco; `Ctrl+C`, fechar a janela ou queda de energia perdem a sessão; banco ilegível desliga o salvamento para não apagar os dados. O desfazer continua valendo só para a sessão.

## D8 — Desfazer com duas pilhas de operações

- **Decisão:** registrar a **operação** (com o necessário para invertê-la) em vez de cópias do estoque; duas pilhas, desfazer e refazer.
- **Alternativas:** foto do estoque a cada ação (memória × ações); ler o log (valores já formatados, imprecisos).
- **Consequências:** memória proporcional às ações; segunda estrutura de dados no projeto; regra de segurança: se faltar memória para registrar, o histórico é descartado (nunca uma inversa errada).

## D9 — Log em texto, um arquivo por dia, "best effort"

- **Decisão:** linhas de colunas fixas em UTF-8, `fflush` a cada linha, falhas de I/O viram no-op.
- **Por quê:** legível por qualquer pessoa; sobrevive a fechamentos bruscos; nunca atrapalha o controle de estoque.

## D10 — Interface de terminal própria, sem bibliotecas

- **Alternativas:** `ncurses`/`PDCurses`, interface gráfica.
- **Decisão:** primitivas próprias (`ui.c`) com códigos ANSI e a API de console do Windows.
- **Consequências:** zero dependências; exe autossuficiente; modo não interativo permite testes e e2e; o projeto controla o alinhamento com UTF-8 e cores.

## D11 — Somente Windows

- **Contexto:** a turma e o laboratório usam Windows; manter três plataformas custaria CI e testes.
- **Decisão:** suporte oficial só a Windows 10/11 (MSYS2 UCRT64). Os ramos `#ifndef _WIN32` continuam no código, mas não são testados.
- **Consequências:** CI com um único job; instalador nativo; Linux/macOS "podem funcionar", sem garantia.

## D12 — Subsistema console (CUI)

- **Contexto:** um build com subsistema GUI fazia o `cmd`/PowerShell não esperarem o programa (issue nº 11).
- **Decisão:** manter o subsistema console padrão e verificá-lo no e2e com `objdump`.

## D13 — Instalador por usuário, logs em `%APPDATA%`

- **Decisão:** instalar sem administrador por padrão; o exe do instalador grava logs em `%APPDATA%\OhmFlow\logs`.
- **Por quê:** a maioria das pessoas não tem (ou não quer usar) senha de administrador; `Program Files` não aceita escrita.

## D14 — Mini framework de testes próprio

- **Alternativas:** Unity, CMocka, Check.
- **Decisão:** ~80 linhas em `test_framework.h`.
- **Por quê:** zero dependências, roda em qualquer lugar onde o gcc rode, fácil de entender para a turma.

## D15 — Tudo em português

- **Decisão:** nomes de funções, comentários, mensagens, commits e documentação em pt-BR.
- **Por quê:** projeto acadêmico de turma brasileira; o código serve de material de estudo.

## D16 — Sair como tecla, Histórico como item 8

- **Decisão:** `Esc`/`0` saem a qualquer momento do menu. O Histórico ocupa a posição `[8]`, liberada com a remoção do placeholder do diferencial, e o menu virou uma grade 4×2 sem linha extra.
- **Por quê:** acesso direto ao histórico sem aumentar o menu; `Esc` é o gesto universal de sair.

## D17 — Código de cores dentro do Novo, desenho só no terminal com cores

- **Contexto:** o código de cores era uma das ideias de diferencial; ele faz mais sentido no momento do cadastro, com o componente na mão.
- **Decisão:** módulo `resistor` separado (lógica pura testável) e desenho em pixel-art de meio-blocos com cor de 24 bits na caixa da tela Novo; desligado sem terminal, sem cores ou com `-DOHM_ASCII`; modo forçável (`telas_definir_resistor`) para os testes.
- **Consequências:** o e2e e os testes de texto não mudam.

## D18 — Diferencial do grupo fora do menu

- **Contexto:** o enunciado pede um item diferencial; a opção `[8]` era um placeholder "Em breve".
- **Decisão:** remover o placeholder. O diferencial do grupo é o conjunto de persistência em SQLite, checkpoint com desfazer/refazer e busca binária.
- **Consequências:** o enunciado fala em "item no Menu"; o Histórico (`[8]`), que dá acesso ao checkpoint, cumpre esse papel.

## D19 — Tela cheia ao abrir

- **Decisão:** no conhost, `SetConsoleDisplayMode`; no Windows Terminal, um F11 sintético, enviado só se a janela em foco for mesmo dele e ainda não cobrir o monitor (F11 alterna). Ao sair, desfaz apenas o que o programa ativou.
- **Alternativa:** só maximizar (mantém barra de título e de tarefas).
- **Consequências:** nada muda sem terminal interativo; terminais de editores não são tocados; `Ctrl+C` encerra sem desfazer a tela cheia.

---

← Anterior: [07 — CI e instalador](07-ci-e-instalador.md) · [Índice](README.md) · Próximo: [09 — Contribuindo](09-contribuindo.md)
