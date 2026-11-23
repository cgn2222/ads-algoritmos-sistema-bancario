#include "../src/conta.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void test_inicializacao(void) {
    ContaBancaria c;
    assert(conta_inicializar(NULL, 0) == CONTA_ERR_ARGUMENTO_NULO);
    assert(conta_inicializar(&c, -1) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_inicializar(&c, CONTA_SALDO_INICIAL_CENTAVOS) == CONTA_OK);
    assert(c.saldo_centavos == 100000);
    assert(c.total_historico == 0);
    assert(c.total_operacoes == 0);
}

static void test_deposito_e_saque_exatos(void) {
    ContaBancaria c;
    assert(conta_inicializar(&c, 0) == CONTA_OK);

    /* Prova ausencia de drift IEEE-754: R$ 0.10 + R$ 0.20 == R$ 0.30 exatos. */
    int64_t v1 = 0, v2 = 0, v3 = 0;
    assert(conta_parse_centavos("0.10", &v1) == CONTA_OK);
    assert(conta_parse_centavos("0,20", &v2) == CONTA_OK);
    assert(conta_parse_centavos("0.30", &v3) == CONTA_OK);

    assert(conta_depositar(&c, v1) == CONTA_OK);
    assert(conta_depositar(&c, v2) == CONTA_OK);
    assert(c.saldo_centavos == 30);
    assert(conta_sacar(&c, v3) == CONTA_OK);
    assert(c.saldo_centavos == 0);
    assert(c.total_operacoes == 3);
    assert(c.total_historico == 3);
    assert(c.historico[2].tipo == TRANSACAO_SAQUE);
    assert(c.historico[2].saldo_apos_centavos == 0);
}

static void test_regras_negativas_e_overflow(void) {
    ContaBancaria c;
    assert(conta_inicializar(&c, 50000) == CONTA_OK);

    assert(conta_depositar(&c, 0) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_depositar(&c, -100) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_sacar(&c, 0) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_sacar(&c, -50) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_sacar(&c, 50001) == CONTA_ERR_SALDO_INSUFICIENTE);
    assert(c.saldo_centavos == 50000);
    assert(c.total_operacoes == 0);

    /* Guarda de overflow inteiro em deposito */
    assert(conta_inicializar(&c, INT64_MAX - 50) == CONTA_OK);
    assert(conta_depositar(&c, 100) == CONTA_ERR_OVERFLOW);
    assert(c.saldo_centavos == INT64_MAX - 50);
}

static void test_parser_monetario(void) {
    int64_t val = 0;

    assert(conta_parse_centavos("100", &val) == CONTA_OK && val == 10000);
    assert(conta_parse_centavos("  42.5  ", &val) == CONTA_OK && val == 4250);
    assert(conta_parse_centavos("19,99", &val) == CONTA_OK && val == 1999);
    assert(conta_parse_centavos("0.01", &val) == CONTA_OK && val == 1);

    assert(conta_parse_centavos("", &val) == CONTA_ERR_FORMATO_INVALIDO);
    assert(conta_parse_centavos("abc", &val) == CONTA_ERR_FORMATO_INVALIDO);
    assert(conta_parse_centavos("12.345", &val) == CONTA_ERR_FORMATO_INVALIDO);
    assert(conta_parse_centavos("12.", &val) == CONTA_ERR_FORMATO_INVALIDO);
    assert(conta_parse_centavos("12.34x", &val) == CONTA_ERR_FORMATO_INVALIDO);
    assert(conta_parse_centavos("-10.00", &val) == CONTA_ERR_VALOR_INVALIDO);
    assert(conta_parse_centavos("0.00", &val) == CONTA_ERR_VALOR_INVALIDO);
}

static void test_formatacao_e_historico_limitado(void) {
    char buf[32];
    conta_formatar_reais(100000, buf, sizeof(buf));
    assert(strcmp(buf, "1000.00") == 0);
    conta_formatar_reais(5, buf, sizeof(buf));
    assert(strcmp(buf, "0.05") == 0);
    conta_formatar_reais(-1234, buf, sizeof(buf));
    assert(strcmp(buf, "-12.34") == 0);

    ContaBancaria c;
    assert(conta_inicializar(&c, 0) == CONTA_OK);
    for (int i = 0; i < CONTA_MAX_HISTORICO + 15; i++) {
        assert(conta_depositar(&c, 100) == CONTA_OK);
    }
    assert(c.total_operacoes == CONTA_MAX_HISTORICO + 15);
    assert(c.total_historico == CONTA_MAX_HISTORICO);
    assert(c.saldo_centavos == (int64_t)(CONTA_MAX_HISTORICO + 15) * 100);
}

int main(void) {
    test_inicializacao();
    test_deposito_e_saque_exatos();
    test_regras_negativas_e_overflow();
    test_parser_monetario();
    test_formatacao_e_historico_limitado();

    printf("[OK] Todos os testes unitarios de sistema_bancario passaram.\n");
    return 0;
}
