#ifndef CONTA_H
#define CONTA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONTA_SALDO_INICIAL_CENTAVOS ((int64_t)100000) /* R$ 1000.00 */
#define CONTA_MAX_HISTORICO 100

typedef enum {
    CONTA_OK = 0,
    CONTA_ERR_ARGUMENTO_NULO = 1,
    CONTA_ERR_VALOR_INVALIDO = 2,
    CONTA_ERR_SALDO_INSUFICIENTE = 3,
    CONTA_ERR_OVERFLOW = 4,
    CONTA_ERR_FORMATO_INVALIDO = 5
} ContaStatus;

typedef enum {
    TRANSACAO_DEPOSITO = 1,
    TRANSACAO_SAQUE = 2
} TipoTransacao;

typedef struct {
    TipoTransacao tipo;
    int64_t valor_centavos;
    int64_t saldo_apos_centavos;
} RegistroTransacao;

typedef struct {
    int64_t saldo_centavos;
    RegistroTransacao historico[CONTA_MAX_HISTORICO];
    size_t total_historico;
    size_t total_operacoes;
} ContaBancaria;

ContaStatus conta_inicializar(ContaBancaria *conta, int64_t saldo_inicial_centavos);
ContaStatus conta_depositar(ContaBancaria *conta, int64_t valor_centavos);
ContaStatus conta_sacar(ContaBancaria *conta, int64_t valor_centavos);

/* Converte string monetaria ("150", "19.99", "19,99") para centavos exatos sem float. */
ContaStatus conta_parse_centavos(const char *texto, int64_t *centavos_out);

/* Formata valor inteiro em centavos como "1000.00" (ou "-50.25"). */
void conta_formatar_reais(int64_t centavos, char *buffer, size_t tamanho);

const char *conta_status_mensagem(ContaStatus status);
const char *conta_tipo_transacao_texto(TipoTransacao tipo);

#endif
