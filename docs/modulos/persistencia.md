# Módulo Persistência — estoque em SQLite

← [Módulos](README.md) · [Main](main.md)

**Arquivos:** [src/persistencia.h](../../src/persistencia.h) (interface) · [src/persistencia.c](../../src/persistencia.c) (implementação) · [database/database.sql](../../database/database.sql) (schema)
**Testes:** [tests/test_persistencia.c](../../tests/test_persistencia.c), segunda sessão do `make e2e`

> **Resumo:** carrega o estoque do banco ao abrir o programa e grava tudo de volta ao sair. Entre esses dois momentos, nenhuma conexão fica aberta.

---

## 1. Explicação para leigos

Pense na lista como a **bancada** e no banco como o **armário**. Ao chegar, você tira tudo do armário e põe na bancada; trabalha o dia inteiro ali; ao ir embora, guarda tudo de volta. Ninguém vai ao armário a cada peça usada.

## 2. Funções

**Tabela 1 — API**

| Função | Faz | Devolve |
| --- | --- | --- |
| `persistencia_caminho(buf, n)` | Resolve o caminho: `OHMFLOW_DB`, `%APPDATA%\OhmFlow\ohmflow.db` (com `-DOHM_DADOS_APPDATA`) ou `database\ohmflow.db` | 1, ou 0 se não couber |
| `persistencia_carregar(li, caminho)` | Cria pasta, arquivo e tabela se faltarem; insere cada linha na lista | `PERSIST_OK` / `PERSIST_ERRO` |
| `persistencia_salvar(li, caminho)` | Substitui todo o conteúdo do banco pela lista, numa transação | `PERSIST_OK` / `PERSIST_ERRO` |

O módulo não imprime nada: quem decide a mensagem é [main.c](main.md).

## 3. Algoritmos

**Carregar:** `SELECT ... ORDER BY resistencia` e `insere_lista_ordenada` por linha. Em ordem crescente, cada inserção cai no fim do vetor da lista e não desloca nada. Uma linha recusada pela lista (fora da faixa, por exemplo) conta como erro.

**Salvar:** `BEGIN IMMEDIATE`, `DELETE`, um `INSERT` preparado reaproveitado em cada visita de `percorre_lista`, `COMMIT`. Qualquer falha vira `ROLLBACK`, e o banco fica como estava antes.

## 4. Integração com `main`

`carrega_estoque` roda depois de `log_iniciar` e antes das boas-vindas, sem passar pelo checkpoint: o que veio do banco é o ponto 0, e desfazer não o apaga. `salva_estoque` roda depois do laço do menu e antes de `tela_encerramento`.

Se a carga falhar, `main` esvazia a lista e **desliga o salvamento da sessão**. Sem isso, uma lista vazia sobrescreveria um banco que talvez ainda possa ser recuperado.

Cada carga e salvamento gera uma linha `BANCO` no [log](log.md).

## 5. Por que foi feito assim

| Decisão | Motivo | Alternativa descartada |
| --- | --- | --- |
| SQLite ligado de forma estática | Sem servidor e sem DLL extra; instalador continua simples | `libsqlite3-0.dll` junto do exe |
| Conexão só no início e no fim | Pedido do projeto; o menu nunca espera por disco | Gravar a cada operação |
| Foto completa ao salvar | Não precisa rastrear mudanças; cobre Remover e Reinicializar | `UPDATE` por operação |
| Schema embutido na compilação | Uma única fonte (`database.sql`), e o exe instalado não depende de arquivo solto | Ler o `.sql` em tempo de execução |
| Não salvar após carga com erro | Protege os dados existentes | Sobrescrever com a lista vazia |

## 6. Testes

[tests/test_persistencia.c](../../tests/test_persistencia.c) cobre banco novo, ida e volta, salvamento substituindo tudo, lista vazia esvaziando o banco, arquivo corrompido preservado, argumentos `NULL` e resolução do caminho. O `make e2e` roda uma segunda sessão com o mesmo banco e confere que o estoque voltou.

---

← [Módulos](README.md) · [Main](main.md)
