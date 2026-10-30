/* test_persistencia.c — testes da carga e do salvamento em SQLite */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "lista.h"
#include "persistencia.h"

#define BANCO_TESTE "build/test_persistencia_tmp/pasta/ohmflow_teste.db"
#define BANCO_CORROMPIDO "build/test_persistencia_tmp/corrompido.db"

static Resistor R(double ohms, int qtd)
{
    Resistor r;
    r.resistencia = ohms;
    r.quantidade = qtd;
    return r;
}

static Lista *lista_salva(void)
{
    Lista *li = cria_lista();
    insere_lista_ordenada(li, R(220, 50));
    insere_lista_ordenada(li, R(4700, 10));
    insere_lista_ordenada(li, R(0.47, 0));
    return li;
}

static int quantidade(Lista *li, double ohms)
{
    int qtd = -1;
    consulta_lista(li, ohms, &qtd);
    return qtd;
}

static void teste_banco_novo_comeca_vazio(void)
{
    Lista *li = cria_lista();
    remove(BANCO_TESTE);
    ASSERT_EQ_INT(PERSIST_OK, persistencia_carregar(li, BANCO_TESTE));
    ASSERT_TRUE(lista_vazia(li));
    libera_lista(li);
}

static void teste_salva_e_carrega(void)
{
    Lista *a = lista_salva(), *b = cria_lista();
    remove(BANCO_TESTE);
    ASSERT_EQ_INT(PERSIST_OK, persistencia_salvar(a, BANCO_TESTE));
    ASSERT_EQ_INT(PERSIST_OK, persistencia_carregar(b, BANCO_TESTE));
    ASSERT_EQ_INT(3, tamanho_lista(b));
    ASSERT_EQ_INT(60, total_pecas(b));
    ASSERT_EQ_INT(50, quantidade(b, 220));
    ASSERT_EQ_INT(10, quantidade(b, 4700));
    ASSERT_EQ_INT(0, quantidade(b, 0.47));
    libera_lista(a);
    libera_lista(b);
}

/* O salvamento é uma foto: o que saiu da lista sai do banco */
static void teste_salvar_substitui_tudo(void)
{
    Lista *a = lista_salva(), *b = cria_lista();
    remove(BANCO_TESTE);
    persistencia_salvar(a, BANCO_TESTE);
    remove_lista(a, 220);
    retira_estoque(a, 4700, 4);
    persistencia_salvar(a, BANCO_TESTE);
    persistencia_carregar(b, BANCO_TESTE);
    ASSERT_EQ_INT(2, tamanho_lista(b));
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, consulta_lista(b, 220, NULL));
    ASSERT_EQ_INT(6, quantidade(b, 4700));
    libera_lista(a);
    libera_lista(b);
}

static void teste_lista_vazia_esvazia_banco(void)
{
    Lista *a = lista_salva(), *b = cria_lista();
    remove(BANCO_TESTE);
    persistencia_salvar(a, BANCO_TESTE);
    reinicializa_lista(a);
    ASSERT_EQ_INT(PERSIST_OK, persistencia_salvar(a, BANCO_TESTE));
    ASSERT_EQ_INT(PERSIST_OK, persistencia_carregar(b, BANCO_TESTE));
    ASSERT_TRUE(lista_vazia(b));
    libera_lista(a);
    libera_lista(b);
}

/* Arquivo que não é banco: erro, lista intacta e arquivo sem mudança */
static void teste_banco_corrompido(void)
{
    static const char LIXO[] = "isto nao e um banco sqlite, mas tem texto suficiente";
    char lido[sizeof LIXO] = "";
    Lista *li = cria_lista();
    FILE *f;
    persistencia_salvar(li, BANCO_TESTE); /* garante a pasta dos testes */
    f = fopen(BANCO_CORROMPIDO, "wb");
    ASSERT_TRUE(f != NULL);
    if (f == NULL) { libera_lista(li); return; }
    fwrite(LIXO, 1, sizeof LIXO - 1, f);
    fclose(f);

    ASSERT_EQ_INT(PERSIST_ERRO, persistencia_carregar(li, BANCO_CORROMPIDO));
    ASSERT_TRUE(lista_vazia(li));

    f = fopen(BANCO_CORROMPIDO, "rb");
    ASSERT_TRUE(f != NULL);
    if (f != NULL) {
        size_t n = fread(lido, 1, sizeof lido - 1, f);
        lido[n] = '\0';
        fclose(f);
    }
    ASSERT_STR_EQ(LIXO, lido);
    remove(BANCO_CORROMPIDO);
    libera_lista(li);
}

static void teste_argumentos_invalidos(void)
{
    Lista *li = cria_lista();
    ASSERT_EQ_INT(PERSIST_ERRO, persistencia_carregar(NULL, BANCO_TESTE));
    ASSERT_EQ_INT(PERSIST_ERRO, persistencia_carregar(li, NULL));
    ASSERT_EQ_INT(PERSIST_ERRO, persistencia_salvar(NULL, BANCO_TESTE));
    ASSERT_EQ_INT(PERSIST_ERRO, persistencia_salvar(li, NULL));
    libera_lista(li);
}

static void teste_caminho(void)
{
    char buf[256];
    _putenv("OHMFLOW_DB=build/outro.db");
    ASSERT_TRUE(persistencia_caminho(buf, sizeof buf));
    ASSERT_STR_EQ("build/outro.db", buf);
    _putenv("OHMFLOW_DB=");
    ASSERT_TRUE(persistencia_caminho(buf, sizeof buf));
    ASSERT_TRUE(strstr(buf, "ohmflow.db") != NULL);
    ASSERT_TRUE(!persistencia_caminho(buf, 4)); /* não cabe */
}

int main(void)
{
    printf("test_persistencia\n");
    RUN_TEST(teste_banco_novo_comeca_vazio);
    RUN_TEST(teste_salva_e_carrega);
    RUN_TEST(teste_salvar_substitui_tudo);
    RUN_TEST(teste_lista_vazia_esvazia_banco);
    RUN_TEST(teste_banco_corrompido);
    RUN_TEST(teste_argumentos_invalidos);
    RUN_TEST(teste_caminho);
    return tf_resumo("test_persistencia");
}
