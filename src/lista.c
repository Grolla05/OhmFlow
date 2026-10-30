/*
 * lista.c — implementação da Lista Dinâmica Ordenada de resistores.
 *
 * Lista simplesmente encadeada com nó descritor: o descritor guarda o ponteiro
 * para o primeiro elemento (NULL se vazia) e os totais de tipos e peças,
 * atualizados a cada operação. Os nós ficam em ordem crescente de resistência.
 *
 * Nó encadeado não tem acesso direto ao meio, então o descritor mantém também
 * `indice`, um vetor dinâmico com o endereço de cada nó na mesma ordem. É ele
 * que permite a busca binária: toda busca custa O(log n) comparações, e só
 * inserir ou remover paga O(n) para deslocar o vetor.
 */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "lista.h"

struct elemento {
    Resistor         dados;
    struct elemento *prox;
};
typedef struct elemento Elem;

struct descritor {
    Elem     *inicio; /* primeiro nó, ou NULL se vazia     */
    Elem    **indice; /* indice[i] = i-ésimo nó em ordem crescente */
    int       capacidade; /* posições alocadas em indice   */
    int       tipos;  /* quantidade de nós                 */
    long long pecas;  /* soma das quantidades (long long: int * nós não cabe em long de 32 bits) */
    int       passos; /* nós examinados pela última busca  */
};

#define TOLERANCIA_RELATIVA 1e-9
#define CAPACIDADE_INICIAL  8

int mesma_resistencia(double a, double b)
{
    double maior = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return fabs(a - b) <= TOLERANCIA_RELATIVA * maior;
}

/*
 * Busca binária em `indice`: olha o termo do meio e descarta a metade que não
 * pode conter `ohms`, porque à esquerda as resistências diminuem e à direita
 * aumentam. Devolve a posição do nó (*achou = 1) ou, se ele não existe, a
 * posição onde deveria entrar (*achou = 0), que também serve para inserir.
 */
static int localiza(Lista *li, double ohms, int *achou)
{
    int ini = 0, fim = li->tipos - 1;
    li->passos = 0;
    *achou = 0;
    while (ini <= fim) {
        int meio = ini + (fim - ini) / 2;
        double valor = li->indice[meio]->dados.resistencia;
        li->passos++;
        if (mesma_resistencia(valor, ohms)) {
            *achou = 1;
            return meio;
        }
        if (valor < ohms) ini = meio + 1; /* direita: resistências maiores  */
        else              fim = meio - 1; /* esquerda: resistências menores */
    }
    return ini;
}

/* Nó com a resistência pedida, ou NULL */
static Elem *busca(Lista *li, double ohms)
{
    int achou;
    int pos = localiza(li, ohms, &achou);
    return achou ? li->indice[pos] : NULL;
}

/* Garante vaga para mais um nó em `indice`, dobrando a capacidade se preciso */
static int garante_vaga(Lista *li)
{
    Elem **novo;
    int capacidade;
    if (li->tipos < li->capacidade) return 1;
    capacidade = li->capacidade > 0 ? li->capacidade * 2 : CAPACIDADE_INICIAL;
    novo = realloc(li->indice, (size_t)capacidade * sizeof *novo);
    if (novo == NULL) return 0;
    li->indice = novo;
    li->capacidade = capacidade;
    return 1;
}

static int resistencia_valida(double ohms)
{
    return !isnan(ohms)
        && ohms >= RESISTENCIA_MIN * (1 - TOLERANCIA_RELATIVA)
        && ohms <= RESISTENCIA_MAX * (1 + TOLERANCIA_RELATIVA);
}

static void libera_nos(Lista *li)
{
    Elem *no;
    while ((no = li->inicio) != NULL) {
        li->inicio = no->prox;
        free(no);
    }
    li->tipos = 0;
    li->pecas = 0;
}

/* ---- criação / destruição ------------------------------------------ */

Lista *cria_lista(void)
{
    Lista *li = malloc(sizeof(Lista));
    if (li != NULL) {
        li->inicio = NULL;
        li->indice = NULL;
        li->capacidade = 0;
        li->tipos = 0;
        li->pecas = 0;
        li->passos = 0;
    }
    return li;
}

void libera_lista(Lista *li)
{
    if (li == NULL) return;
    libera_nos(li);
    free(li->indice);
    free(li);
}

int reinicializa_lista(Lista *li)
{
    if (li == NULL) return LISTA_ERRO;
    libera_nos(li);
    return LISTA_OK;
}

/* ---- informações (O(1): lidas do descritor) ------------------------- */

int tamanho_lista(const Lista *li)
{
    return li == NULL ? 0 : li->tipos;
}

long long total_pecas(const Lista *li)
{
    return li == NULL ? 0 : li->pecas;
}

int lista_vazia(const Lista *li)
{
    return li == NULL || li->inicio == NULL;
}

int lista_passos(const Lista *li)
{
    return li == NULL ? 0 : li->passos;
}

void percorre_lista(const Lista *li, void (*f)(const Resistor *r, void *ctx), void *ctx)
{
    const Elem *no;
    if (li == NULL || f == NULL) return;
    for (no = li->inicio; no != NULL; no = no->prox) f(&no->dados, ctx);
}

/* ---- inserção ordenada --------------------------------------------- */

int insere_lista_ordenada(Lista *li, Resistor r)
{
    Elem *novo, *anterior;
    int pos, achou;
    if (li == NULL) return LISTA_ERRO;
    if (!resistencia_valida(r.resistencia) || r.quantidade < 0) return LISTA_INVALIDO;

    pos = localiza(li, r.resistencia, &achou);
    if (achou) return LISTA_DUPLICADO;

    /* vaga primeiro: se faltar memória, nada mudou na lista */
    if (!garante_vaga(li)) return LISTA_ERRO;
    novo = malloc(sizeof(Elem));
    if (novo == NULL) return LISTA_ERRO;
    novo->dados = r;

    /* encadeia depois do vizinho da esquerda (ou no início) */
    anterior = pos > 0 ? li->indice[pos - 1] : NULL;
    novo->prox = anterior != NULL ? anterior->prox : li->inicio;
    if (anterior != NULL) anterior->prox = novo;
    else li->inicio = novo;

    /* abre a posição no vetor, deslocando quem é maior */
    memmove(&li->indice[pos + 1], &li->indice[pos],
            (size_t)(li->tipos - pos) * sizeof *li->indice);
    li->indice[pos] = novo;
    li->tipos++;
    li->pecas += r.quantidade;
    return LISTA_OK;
}

/* ---- remoção ------------------------------------------------------- */

int remove_lista(Lista *li, double ohms)
{
    Elem *no;
    int pos, achou;
    if (li == NULL) return LISTA_ERRO;
    pos = localiza(li, ohms, &achou);
    if (!achou) return LISTA_NAO_ENCONTRADO;
    no = li->indice[pos];

    /* o anterior (ou o início) passa a apontar para o seguinte */
    if (pos > 0) li->indice[pos - 1]->prox = no->prox;
    else li->inicio = no->prox;

    /* fecha a posição no vetor */
    memmove(&li->indice[pos], &li->indice[pos + 1],
            (size_t)(li->tipos - pos - 1) * sizeof *li->indice);
    li->tipos--;
    li->pecas -= no->dados.quantidade;
    free(no);
    return LISTA_OK;
}

/* ---- consulta e movimentação de estoque ---------------------------- */

int consulta_lista(Lista *li, double ohms, int *qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (qtd != NULL) *qtd = no->dados.quantidade;
    return LISTA_OK;
}

int retira_estoque(Lista *li, double ohms, int qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    if (qtd <= 0) return LISTA_INVALIDO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (qtd > no->dados.quantidade) return LISTA_SEM_ESTOQUE;
    no->dados.quantidade -= qtd; /* pode zerar: o tipo continua cadastrado */
    li->pecas -= qtd;
    return LISTA_OK;
}

int repor_estoque(Lista *li, double ohms, int qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    if (qtd <= 0) return LISTA_INVALIDO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (no->dados.quantidade > INT_MAX - qtd) return LISTA_INVALIDO; /* overflow */
    no->dados.quantidade += qtd;
    li->pecas += qtd;
    return LISTA_OK;
}
