# CHANGES

## Persistência SQLite, tela cheia, menu com banner e busca binária

### 1. Persistência

- `database/database.sql`: schema com tabela única `resistores` (`resistencia`, `quantidade`).
- `database/README.md`: como o banco foi elaborado, por que existe, quando é acessado, onde fica e limitações.
- `src/persistencia.h/.c`: `persistencia_caminho`, `persistencia_carregar`, `persistencia_salvar`. Conexão aberta e fechada em cada chamada; salvamento é foto completa numa transação.
- `src/main.c`: `carrega_estoque` depois de `log_iniciar`; `salva_estoque` depois do menu. O banco não é tocado em nenhum outro ponto. Carga com erro avisa e desliga o salvamento da sessão, preservando o arquivo. Linhas `BANCO` no log.
- Caminho do banco: `OHMFLOW_DB` > `%APPDATA%\OhmFlow\ohmflow.db` (`-DOHM_DADOS_APPDATA`, exe do instalador) > `.\database\ohmflow.db`.
- `Makefile`: SQLite ligado de forma estática (exe sem DLL extra, conferido com `objdump`); gera `build/schema_sql.h` a partir do `.sql`; `test_persistencia` em `TESTES`; `cppcheck` depende do header gerado; e2e usa `build/e2e.db` e roda uma segunda sessão.
- `.github/workflows/ci.yml`: pacote `sqlite3:p`. `.gitignore`: `database/*.db*`.

### 2. Tela cheia ao abrir

- `src/ui/ui.c/.h`: `ui_tela_cheia`, `ui_sair_tela_cheia`, `ui_tipo_terminal`.
- Conhost: `SetConsoleDisplayMode`, com `SW_MAXIMIZE` como reserva. Windows Terminal: F11 sintético, só se a janela em foco for `CASCADIA_HOSTING_WINDOW_CLASS` e não cobrir o monitor. Terminal do VS Code e outros: sem ação.
- Só em modo interativo. Ao sair, desfaz apenas o que o programa ativou.

### 3. Remoção do Diferencial

- Removidos `OPCAO_DIFERENCIAL`, `tela_diferencial`, `ALTURA_DIFERENCIAL` e `docs/funcionalidades/08-diferencial.md`.
- `[8] Histórico` (`OPCAO_HISTORICO = 8`, `OPCAO_TOTAL = 9`); `navegar_menu` virou grade 4×2 pura, com volta na mesma coluna; tecla `9` ignorada; dica `[1-8]`.
- `tests/e2e/entrada.txt`: Histórico pela opção 8.

### 4. Menu com banner

- `imprime_banner` extraído de `boas_vindas_banner` e reutilizado no menu.
- `quadro_menu`: banner logo acima do card, os dois centralizados como um bloco, sem título na borda. `menu_com_banner` decide pelo tamanho do terminal (≥ 65 colunas e 22 linhas); fora disso, layout compacto.
- Card com largura `MENU_LARGURA` e linha `Tipos · Peças` centralizada.
- Conferido em terminal real com um programa descartável que chama `tela_menu`.

### 5. Busca binária

- `src/lista.c`: descritor ganha `indice` (vetor dinâmico de `Elem *`) e `capacidade`; `localiza` virou busca binária e devolve a posição do nó ou o ponto de inserção; inserção e remoção ajustam `prox` e deslocam o vetor com `memmove`; `garante_vaga` dobra a capacidade antes de criar o nó. API pública sem mudança.
- Custo: busca O(log n); inserir e remover pagam O(n) para deslocar o vetor.

### Testes

- `tests/test_lista.c`: passos reescritos; limite de 10 passos com 1.023 tipos; ordem com inserção crescente, decrescente e embaralhada; remoção mantendo a busca; reuso após reinicializar.
- `tests/test_persistencia.c` (novo, 7 testes), `tests/test_ui.c` (tipo de terminal, tela cheia sem terminal), `tests/test_telas.c` (navegação, Histórico em 8, `menu_com_banner`).
- `tests/e2e/entrada_persistencia.txt` e `esperado_persistencia.txt` (novos): segunda sessão confere a volta do estoque.
- `tests/setup/test_setup.ps1`: confere `ohmflow.db` no APPDATA e nada na pasta de instalação.

### Documentação

- Nova `docs/modulos/persistencia.md`.
- Atualizados `docs/modulos/lista.md`, `main.md`, `telas.md`, `ui.md`, `checkpoint.md`, `README.md`; `docs/01` a `06`, `08` a `10`; `docs/funcionalidades/*`; `README.md`; `AGENTS.md`.
- Decisões: D4 e D7 reescritas, D16 ajustada, D18 (diferencial) e D19 (tela cheia) novas.

### Validação

- `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` e `make teste-setup`.
- Não verificado: entrada e saída da tela cheia em janela real do Windows Terminal e do conhost.
