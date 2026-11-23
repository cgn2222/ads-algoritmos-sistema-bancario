#include "conta.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void registrar_transacao(ContaBancaria *conta,
                                TipoTransacao tipo,
                                int64_t valor_centavos) {
    RegistroTransacao reg = {
        .tipo = tipo,
        .valor_centavos = valor_centavos,
        .saldo_apos_centavos = conta->saldo_centavos
    };

    if (conta->total_historico < CONTA_MAX_HISTORICO) {
        conta->historico[conta->total_historico++] = reg;
    } else {
        /* Mantem os CONTA_MAX_HISTORICO registros mais recentes. */
        memmove(&conta->historico[0],
                &conta->historico[1],
                (CONTA_MAX_HISTORICO - 1) * sizeof(RegistroTransacao));
        conta->historico[CONTA_MAX_HISTORICO - 1] = reg;
    }
    conta->total_operacoes++;
}

ContaStatus conta_inicializar(ContaBancaria *conta, int64_t saldo_inicial_centavos) {
    if (conta == NULL) {
        return CONTA_ERR_ARGUMENTO_NULO;
    }
    if (saldo_inicial_centavos < 0) {
        return CONTA_ERR_VALOR_INVALIDO;
    }
    memset(conta, 0, sizeof(*conta));
    conta->saldo_centavos = saldo_inicial_centavos;
    return CONTA_OK;
}

ContaStatus conta_depositar(ContaBancaria *conta, int64_t valor_centavos) {
    if (conta == NULL) {
        return CONTA_ERR_ARGUMENTO_NULO;
    }
    if (valor_centavos <= 0) {
        return CONTA_ERR_VALOR_INVALIDO;
    }
    if (conta->saldo_centavos > INT64_MAX - valor_centavos) {
        return CONTA_ERR_OVERFLOW;
    }

    conta->saldo_centavos += valor_centavos;
    registrar_transacao(conta, TRANSACAO_DEPOSITO, valor_centavos);
    return CONTA_OK;
}

ContaStatus conta_sacar(ContaBancaria *conta, int64_t valor_centavos) {
    if (conta == NULL) {
        return CONTA_ERR_ARGUMENTO_NULO;
    }
    if (valor_centavos <= 0) {
        return CONTA_ERR_VALOR_INVALIDO;
    }
    if (valor_centavos > conta->saldo_centavos) {
        return CONTA_ERR_SALDO_INSUFICIENTE;
    }

    conta->saldo_centavos -= valor_centavos;
    registrar_transacao(conta, TRANSACAO_SAQUE, valor_centavos);
    return CONTA_OK;
}

ContaStatus conta_parse_centavos(const char *texto, int64_t *centavos_out) {
    if (texto == NULL || centavos_out == NULL) {
        return CONTA_ERR_ARGUMENTO_NULO;
    }

    const char *p = texto;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return CONTA_ERR_FORMATO_INVALIDO;
    }
    if (*p == '-') {
        return CONTA_ERR_VALOR_INVALIDO;
    }
    if (*p == '+') {
        p++;
    }

    int64_t reais = 0;
    int64_t centavos = 0;
    bool leu_digito_inteiro = false;

    while (isdigit((unsigned char)*p)) {
        leu_digito_inteiro = true;
        int digito = *p - '0';
        if (reais > (INT64_MAX - digito) / 10) {
            return CONTA_ERR_OVERFLOW;
        }
        reais = reais * 10 + digito;
        p++;
    }

    if (*p == '.' || *p == ',') {
        p++;
        int casas = 0;
        while (isdigit((unsigned char)*p)) {
            if (casas >= 2) {
                return CONTA_ERR_FORMATO_INVALIDO;
            }
            centavos = centavos * 10 + (*p - '0');
            casas++;
            p++;
        }
        if (casas == 0) {
            return CONTA_ERR_FORMATO_INVALIDO;
        }
        if (casas == 1) {
            centavos *= 10;
        }
    }

    if (!leu_digito_inteiro) {
        return CONTA_ERR_FORMATO_INVALIDO;
    }

    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p != '\0') {
        return CONTA_ERR_FORMATO_INVALIDO;
    }

    if (reais > (INT64_MAX - centavos) / 100) {
        return CONTA_ERR_OVERFLOW;
    }

    int64_t total = reais * 100 + centavos;
    if (total <= 0) {
        return CONTA_ERR_VALOR_INVALIDO;
    }

    *centavos_out = total;
    return CONTA_OK;
}

void conta_formatar_reais(int64_t centavos, char *buffer, size_t tamanho) {
    if (buffer == NULL || tamanho == 0) {
        return;
    }
    bool negativo = centavos < 0;
    uint64_t abs_val = negativo
                           ? (uint64_t)(-(centavos + 1)) + UINT64_C(1)
                           : (uint64_t)centavos;
    uint64_t reais = abs_val / UINT64_C(100);
    uint64_t frac = abs_val % UINT64_C(100);

    snprintf(buffer, tamanho, "%s%" PRIu64 ".%02" PRIu64,
             negativo ? "-" : "", reais, frac);
}

const char *conta_status_mensagem(ContaStatus status) {
    switch (status) {
        case CONTA_OK:
            return "Operacao realizada com sucesso.";
        case CONTA_ERR_ARGUMENTO_NULO:
            return "Argumento nulo recebido.";
        case CONTA_ERR_VALOR_INVALIDO:
            return "Operacao nao realizada: o valor deve ser maior que zero.";
        case CONTA_ERR_SALDO_INSUFICIENTE:
            return "Saldo insuficiente para realizar este saque.";
        case CONTA_ERR_OVERFLOW:
            return "Valor excede o limite suportado pelo sistema.";
        case CONTA_ERR_FORMATO_INVALIDO:
            return "Formato numerico invalido. Use ate 2 casas decimais (ex.: 150.50).";
    }
    return "Erro desconhecido.";
}

const char *conta_tipo_transacao_texto(TipoTransacao tipo) {
    switch (tipo) {
        case TRANSACAO_DEPOSITO:
            return "DEPOSITO";
        case TRANSACAO_SAQUE:
            return "SAQUE";
    }
    return "DESCONHECIDO";
}
