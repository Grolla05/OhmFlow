# 13 — Sair (encerramento)

← Anterior: [12 — Entrada de valores](12-entrada-de-valores.md) · [Funcionalidades](README.md) · Próximo: [14 — Resistor colorido](14-resistor-colorido.md)

**Tecla:** `Esc` ou `0` no menu (também: fim da entrada, terminal fechado)
**Código:** `menu_tecla_opcao` e `tela_encerramento` em [src/ui/telas.c](../../src/ui/telas.c); fim de `main` em [src/main.c](../../src/main.c)
**Testes:** [tests/test_telas.c](../../tests/test_telas.c), e2e, teste do instalador

---

## 1. Para que serve

Encerrar o programa **de forma limpa**: salvar o estoque no banco, mostrar o resumo final, fechar o registro da sessão e devolver ao sistema toda a memória usada, como exige o enunciado ("Sair liberando a memória").

## 2. Como usar

No menu, aperte `Esc` ou `0`. Não há confirmação.

```text
╔══════════════════════════════════════╗
║ Obrigado por usar o OhmFlow!         ║
║ Estoque final: 3 tipos · 15 peças    ║
╚══════════════════════════════════════╝
```

> ⚠️ O estoque **só é salvo por este caminho**. Ao sair pelo menu, ele vai para o [banco](../../database/README.md) e volta na próxima abertura. `Ctrl+C`, fechar a janela ou queda de energia perdem as mudanças da sessão. A linha do tempo do desfazer termina aqui em qualquer caso; o registro em log permanece.

## 3. Por dentro

1. `menu_tecla_opcao` transforma `Esc`/`0`/fim da entrada em `OPCAO_SAIR` (−2, fora da faixa do menu).
2. O laço do `main` termina e, se a carga do banco deu certo, `salva_estoque` grava uma foto completa do estoque numa transação.
3. `tela_encerramento(estoque)` lê os totais do descritor e mostra o resumo.
4. `log_encerrar()` escreve o rodapé com a duração da sessão e fecha o arquivo.
5. `telas_definir_checkpoint(NULL)`, `libera_checkpoint` (esvazia as duas pilhas, liberando cada operação e seus itens), `libera_lista` (libera cada nó, o vetor e o descritor) e `ui_sair_tela_cheia`.
6. `return EXIT_SUCCESS` (código 0), que os testes conferem.

```mermaid
flowchart LR
    A["Esc / 0"] --> S["salva_estoque"] --> B["tela_encerramento"] --> C["log_encerrar"] --> D["libera_checkpoint"] --> E["libera_lista"] --> G["ui_sair_tela_cheia"] --> F(["EXIT_SUCCESS"])
```

**Figura 1 — Sequência de encerramento**

## 4. Por que é assim

| Decisão | Motivo |
| --- | --- |
| Sair é tecla, não item do menu | Libera espaço para o `[8] Histórico`; `Esc` é o gesto universal de "sair" |
| Sem confirmação | Sair salva o estoque, então nada se perde; confirmar seria só um passo a mais |
| Salvar antes da tela final | Um erro ao gravar aparece para quem usa e fica no log antes de `log_encerrar` |
| Liberar tudo explicitamente | Requisito do enunciado e boa prática: cada `malloc` com seu `free` |
| Fim da entrada = sair | Evita laço infinito quando a entrada acaba (testes, terminal fechado) |

## 5. Testes

- `test_telas.c`: `teste_menu_esc_sai`, `teste_tecla_fim_da_entrada`.
- e2e: termina com `0` e confere *"Obrigado por usar o OhmFlow"*, *"Estoque final: 3 tipos · 15 peças"* e o rodapé *"sessão encerrada em"* no log. A segunda sessão confere que o estoque voltou do banco.
- `test_setup.ps1`: o exe instalado chega à tela de encerramento e sai com código 0.

---

← Anterior: [12 — Entrada de valores](12-entrada-de-valores.md) · [Funcionalidades](README.md) · Próximo: [14 — Resistor colorido](14-resistor-colorido.md)
