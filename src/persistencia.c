/*
 * persistencia.c — carga e salvamento do estoque em SQLite (ver persistencia.h).
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* mkdir com -std=c99 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "persistencia.h"

#ifdef _WIN32
#include <direct.h>
#define CRIA_PASTA(p) _mkdir(p)
#define SEP "\\"
#else
#include <sys/stat.h>
#define CRIA_PASTA(p) mkdir((p), 0755)
#define SEP "/"
#endif

#define TAM_CAMINHO 512

/* database/database.sql como string C; gerado pelo Makefile em build/ */
static const char SCHEMA[] =
#include "schema_sql.h"
    ;

/* Estado do salvamento, passado a percorre_lista */
typedef struct {
    sqlite3_stmt *insere;
    int           ok;
} Salvamento;

int persistencia_caminho(char *buf, size_t n)
{
    const char *env = getenv("OHMFLOW_DB");
    int r;
    if (env != NULL && env[0] != '\0')
        r = snprintf(buf, n, "%s", env);
    else {
#ifdef OHM_DADOS_APPDATA
        const char *appdata = getenv("APPDATA");
        if (appdata != NULL && appdata[0] != '\0')
            r = snprintf(buf, n, "%s" SEP "OhmFlow" SEP "ohmflow.db", appdata);
        else
#endif
            r = snprintf(buf, n, "database" SEP "ohmflow.db");
    }
    return r > 0 && (size_t)r < n;
}

/* Cria as pastas de `caminho` que faltarem, sem o nome do arquivo (erros são
 * ignorados: quem decide se deu certo é o sqlite3_open) */
static void cria_pastas_do_arquivo(const char *caminho)
{
    char tmp[TAM_CAMINHO];
    char *p;
    int r = snprintf(tmp, sizeof tmp, "%s", caminho);
    if (r <= 0 || (size_t)r >= sizeof tmp) return;
    for (p = tmp + 1; *p; p++) {
        if (*p != '/' && *p != '\\') continue;
        if (p[-1] == ':') continue; /* "C:\" não é pasta a criar */
        *p = '\0';
        CRIA_PASTA(tmp);
        *p = SEP[0];
    }
}

/* Abre o banco e garante a tabela; NULL em erro */
static sqlite3 *abre_banco(const char *caminho)
{
    sqlite3 *db = NULL;
    cria_pastas_do_arquivo(caminho);
    if (sqlite3_open(caminho, &db) != SQLITE_OK
        || sqlite3_exec(db, SCHEMA, NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_close(db); /* aceita NULL e libera a conexão que falhou */
        return NULL;
    }
    return db;
}

int persistencia_carregar(Lista *li, const char *caminho)
{
    sqlite3 *db;
    sqlite3_stmt *consulta = NULL;
    int r, ok = 1;

    if (li == NULL || caminho == NULL) return PERSIST_ERRO;
    db = abre_banco(caminho);
    if (db == NULL) return PERSIST_ERRO;

    /* em ordem crescente, cada inserção cai no fim do vetor da lista */
    if (sqlite3_prepare_v2(db,
            "SELECT resistencia, quantidade FROM resistores ORDER BY resistencia;",
            -1, &consulta, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return PERSIST_ERRO;
    }
    while ((r = sqlite3_step(consulta)) == SQLITE_ROW) {
        Resistor item;
        item.resistencia = sqlite3_column_double(consulta, 0);
        item.quantidade = sqlite3_column_int(consulta, 1);
        if (insere_lista_ordenada(li, item) != LISTA_OK) {
            ok = 0;
            break;
        }
    }
    if (r != SQLITE_DONE && r != SQLITE_ROW) ok = 0;
    sqlite3_finalize(consulta);
    sqlite3_close(db);
    return ok ? PERSIST_OK : PERSIST_ERRO;
}

static void salva_resistor(const Resistor *r, void *ctx)
{
    Salvamento *s = ctx;
    if (!s->ok) return;
    sqlite3_bind_double(s->insere, 1, r->resistencia);
    sqlite3_bind_int(s->insere, 2, r->quantidade);
    if (sqlite3_step(s->insere) != SQLITE_DONE) s->ok = 0;
    sqlite3_reset(s->insere);
}

int persistencia_salvar(const Lista *li, const char *caminho)
{
    sqlite3 *db;
    Salvamento s;

    if (li == NULL || caminho == NULL) return PERSIST_ERRO;
    db = abre_banco(caminho);
    if (db == NULL) return PERSIST_ERRO;

    s.insere = NULL;
    s.ok = sqlite3_exec(db, "BEGIN IMMEDIATE;", NULL, NULL, NULL) == SQLITE_OK
        && sqlite3_exec(db, "DELETE FROM resistores;", NULL, NULL, NULL) == SQLITE_OK
        && sqlite3_prepare_v2(db,
               "INSERT INTO resistores (resistencia, quantidade) VALUES (?, ?);",
               -1, &s.insere, NULL) == SQLITE_OK;
    if (s.ok) percorre_lista(li, salva_resistor, &s);
    sqlite3_finalize(s.insere); /* aceita NULL */

    /* sem COMMIT, o banco fica como estava antes do DELETE */
    if (s.ok) s.ok = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL) == SQLITE_OK;
    if (!s.ok) sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    sqlite3_close(db);
    return s.ok ? PERSIST_OK : PERSIST_ERRO;
}
