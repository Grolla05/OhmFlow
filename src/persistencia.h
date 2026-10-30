/*
 * persistencia.h — guarda o estoque em um banco SQLite entre as execuções.
 *
 * A conexão existe só dentro de cada chamada: o programa lê o banco uma vez ao
 * abrir e grava uma vez ao sair. Durante a sessão, tudo acontece na lista em
 * memória. O schema vem de database/database.sql, embutido na compilação.
 */
#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include <stddef.h>
#include "lista.h"

/* Códigos de retorno */
enum {
    PERSIST_OK   = 1,
    PERSIST_ERRO = 0  /* argumento NULL, falha de I/O ou banco inválido */
};

/* Caminho do banco, na ordem: OHMFLOW_DB, %APPDATA%\OhmFlow\ohmflow.db (build
 * do instalador) e, por fim, database\ohmflow.db na pasta atual. 0 se não couber */
int persistencia_caminho(char *buf, size_t n);

/* Acrescenta à lista os resistores do banco em `caminho`, criando pasta, arquivo
 * e tabela se faltarem. Em erro, a lista pode ter recebido parte das linhas e
 * quem chama NÃO deve salvar por cima do banco */
int persistencia_carregar(Lista *li, const char *caminho);

/* Substitui, numa única transação, todo o conteúdo do banco pela lista */
int persistencia_salvar(const Lista *li, const char *caminho);

#endif /* PERSISTENCIA_H */
