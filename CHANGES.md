# CHANGES

## docs: alinha documentação, modelos e comentários ao código atual

Só documentação, modelos do GitHub e comentários. Nenhuma mudança de comportamento: o único arquivo de `src/` alterado é um comentário em `src/checkpoint.h`, e o de `installer/` é um comentário em `ohmflow.iss`, que continua em UTF-8 com BOM. Não há teste novo, porque nada de código mudou.

### Motivo

A comparação entre repositório e documentação mostrou trechos anteriores à persistência em SQLite (commit `57cfd32`), contagens de testes erradas e referências a plataformas sem suporte.

### Build, testes e CI

- `docs/05-build-e-ambiente.md`: sqlite3 na tabela de ferramentas; `CFLAGS` e `LDLIBS` iguais aos do `Makefile`; 10 suítes; e2e com duas sessões e `build/e2e.db`; `-DOHM_DADOS_APPDATA` no `make setup`; `persistencia.c` e `schema_sql.h` na figura; `OHMFLOW_DB`; `database/*.db` nos arquivos gerados; novos problemas comuns; aviso de que `make clean` apaga `dist/` inteira.
- `docs/06-testes.md`: total corrigido de 153 para 185 e 10 suítes na figura; e2e com a segunda sessão, `OHMFLOW_DB` e os trechos `BANCO`; teste do instalador confere também o banco.
- `docs/07-ci-e-instalador.md`: sqlite3 na etapa do MSYS2; exe do instalador com `-DOHM_DADOS_APPDATA`, banco em `%APPDATA%\OhmFlow\ohmflow.db`.

### Funcionalidades e arquitetura

- `docs/02-guia-do-usuario.md`: a linha "O estoque sumiu" dizia que o estoque não é salvo; agora explica o salvamento ao sair. Nova linha para o aviso de banco ilegível.
- `docs/funcionalidades/13-sair.md`: inclui `salva_estoque` e `ui_sair_tela_cheia` na sequência de encerramento; corrige o aviso e os motivos.
- `docs/04-arquitetura.md`: `OHM_DADOS_APPDATA`, `OHMFLOW_DB` e `persistencia.c` na compilação condicional.
- `README.md`: a seção de telas usa os prints de `docs/prints/` no lugar de "print em breve".

### Referências e modelos

- `refs/BRAINSTORM.md`: marcado como decidido e implementado; removida a regra sobre o rótulo `(diferencial)`.
- `.github/CONTRIBUTING.md`: C99 em vez de C11; `-pedantic`; sqlite3; suporte só a Windows; fluxo de fork separado do fluxo da equipe na `Grolla`; `make asan` trocado por `make teste-setup`.
- `.github/COMMIT_CONVENTION.md`: escopos `pilha`, `checkpoint`, `resistor`, `log` e `persistencia`; exemplos sem Linux e Ubuntu.
- `.github/pull_request_template.md`: módulos novos; item de memória sem depender de Linux.
- `.github/ISSUE_TEMPLATE/bug_report.md` e `bug_report.yml`: exemplos só com Windows.

### Comentários

- `src/checkpoint.h`: o estoque agora é salvo; o que vale só para a sessão é o desfazer.
- `installer/ohmflow.iss`: cita também `-DOHM_DADOS_APPDATA`.

### Fora do escopo

- `AGENTS.md` também foi corrigido (caminho do `.ico`), mas é local e ignorado pelo Git.
- Continuam abertos: nova release, `VERSAO ?= 1.0.0` no `Makefile`, issues `Grolla05/OhmFlow#7` e `Grolla05/OhmFlow#8` e `CODEOWNERS`.

### Validação

`make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` com saída 0: 185 testes, 5215 asserts, 0 falhas; e2e com 70 + 7 verificações; 11 trechos de log; cppcheck sem achados. `make teste-setup` não rodou, por falta do Inno Setup na máquina; fica para o CI.
