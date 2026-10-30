/*
 * OhmFlow — controle de estoque de resistores de um almoxarifado.
 *
 * Projeto 01 de Estrutura de Dados e Recuperação da Informação: a estrutura
 * central é uma Lista Dinâmica Ordenada (lista.h), ordenada pela resistência.
 * Este arquivo apenas cria a lista, carrega o estoque do banco, conduz o
 * roteiro de telas (ui/telas.h), salva o estoque e libera a memória ao final.
 */
#include <stdio.h>
#include <stdlib.h>
#include "lista.h"
#include "checkpoint.h"
#include "log.h"
#include "persistencia.h"
#include "ui.h"
#include "telas.h"

#define TAM_CAMINHO_BANCO 512

/* Lê o banco para a lista. Devolve 1 se ao sair pode salvar: com banco ilegível
 * a lista não representa o que há no disco, e salvar apagaria os dados */
static int carrega_estoque(Lista *estoque, char *banco, size_t n)
{
    if (!persistencia_caminho(banco, n)) {
        log_acao(LOG_ERRO, "BANCO", NULL, NULL, "CAMINHO INVÁLIDO");
        ui_mensagem(UI_AVISO, "Caminho do banco inválido; o estoque não será salvo.");
        return 0;
    }
    if (persistencia_carregar(estoque, banco) != PERSIST_OK) {
        reinicializa_lista(estoque); /* descarta o que veio pela metade */
        log_acao(LOG_ERRO, "BANCO", NULL, banco, "FALHA NA CARGA");
        ui_mensagem(UI_AVISO, "Não foi possível ler o banco (%s); "
                    "o arquivo foi preservado e esta sessão não será salva.", banco);
        return 0;
    }
    log_acao(LOG_INFO, "BANCO", NULL, "carga", "OK");
    return 1;
}

/* Grava a sessão no banco; ponto único de escrita, ao sair */
static void salva_estoque(const Lista *estoque, const char *banco)
{
    if (persistencia_salvar(estoque, banco) == PERSIST_OK) {
        log_acao(LOG_INFO, "BANCO", NULL, "salvamento", "OK");
    } else {
        log_acao(LOG_ERRO, "BANCO", NULL, banco, "FALHA AO SALVAR");
        ui_mensagem(UI_ERRO, "Não foi possível salvar o estoque em %s.", banco);
    }
}

int main(void)
{
    Lista *estoque;
    Checkpoint *checkpoint;
    char banco[TAM_CAMINHO_BANCO];
    int opcao, pode_salvar;

    ui_init();
    ui_bufferizar_saida(); /* quadros do menu saem numa escrita só */
    ui_definir_centralizado(ui_interativo()); /* telas no centro só no terminal */
    ui_tela_cheia();
    estoque = cria_lista(); /* inicialização da lista (vazia) */
    /* pilhas de desfazer/refazer da sessão, usadas pelo [8] Histórico */
    checkpoint = cria_checkpoint();
    if (estoque == NULL || checkpoint == NULL) {
        fprintf(stderr, "Erro: memória insuficiente para criar a lista.\n");
        libera_lista(estoque);
        libera_checkpoint(checkpoint);
        ui_sair_tela_cheia();
        return EXIT_FAILURE;
    }
    telas_definir_checkpoint(checkpoint);
    /* cada execução é uma sessão no log do dia (logs/ohmflow_AAAA-MM-DD.log) */
    if (!log_iniciar())
        ui_mensagem(UI_AVISO, "Não foi possível abrir o arquivo de log; "
                    "seguindo sem registro.");
    /* o banco só é acessado aqui e ao sair; depois da carga, para o checkpoint
     * começar vazio e desfazer não apagar o que veio de sessões anteriores */
    pode_salvar = carrega_estoque(estoque, banco, sizeof banco);

    tela_boas_vindas();
    while ((opcao = tela_menu(estoque)) != OPCAO_SAIR) {
        switch (opcao) {
        case OPCAO_INSERIR:       tela_inserir(estoque);       break;
        case OPCAO_REMOVER:       tela_remover(estoque);       break;
        case OPCAO_LISTAR:        tela_listar(estoque);        break;
        case OPCAO_CONSULTAR:     tela_consultar(estoque);     break;
        case OPCAO_RETIRAR:       tela_retirar(estoque);       break;
        case OPCAO_REPOR:         tela_repor(estoque);         break;
        case OPCAO_REINICIALIZAR: tela_reinicializar(estoque); break;
        case OPCAO_HISTORICO:     tela_historico(estoque);     break;
        default:                                               break;
        }
    }
    if (pode_salvar) salva_estoque(estoque, banco);
    tela_encerramento(estoque);
    log_encerrar();

    telas_definir_checkpoint(NULL);
    libera_checkpoint(checkpoint);
    libera_lista(estoque);
    ui_sair_tela_cheia();
    return EXIT_SUCCESS;
}
