#include "conta.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ler_linha(char *buffer, size_t tamanho) {
    if (fgets(buffer, (int)tamanho, stdin) == NULL) {
        return false;
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[--len] = '\0';
    }
    return true;
}

static bool ler_opcao_menu(int *opcao_out) {
    char linha[64];
    if (!ler_linha(linha, sizeof(linha))) {
        return false;
    }

    char *p = linha;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        *opcao_out = -1;
        return true;
    }

    errno = 0;
    char *fim = NULL;
    long val = strtol(p, &fim, 10);
    while (fim != NULL && *fim != '\0' && isspace((unsigned char)*fim)) {
        fim++;
    }

    if (errno != 0 || fim == p || (fim != NULL && *fim != '\0') || val < 0 || val > 99) {
        *opcao_out = -1;
        return true;
    }

    *opcao_out = (int)val;
    return true;
}

static void exibir_extrato(const ContaBancaria *conta) {
    char valor_buf[32];
    char saldo_buf[32];

    printf("\n--- EXTRATO DA CONTA ---\n");
    if (conta->total_historico == 0) {
        printf("Nenhuma movimentacao registrada nesta sessao.\n");
        return;
    }

    printf("%-4s | %-10s | %14s | %14s\n", "#", "OPERACAO", "VALOR", "SALDO APOS");
    printf("---------------------------------------------------\n");
    for (size_t i = 0; i < conta->total_historico; i++) {
        const RegistroTransacao *reg = &conta->historico[i];
        conta_formatar_reais(reg->valor_centavos, valor_buf, sizeof(valor_buf));
        conta_formatar_reais(reg->saldo_apos_centavos, saldo_buf, sizeof(saldo_buf));
        printf("%-4zu | %-10s | R$ %11s | R$ %11s\n",
               i + 1,
               conta_tipo_transacao_texto(reg->tipo),
               valor_buf,
               saldo_buf);
    }
    printf("---------------------------------------------------\n");
    printf("Total de operacoes na sessao: %zu\n", conta->total_operacoes);
}

int main(void) {
    ContaBancaria conta;
    conta_inicializar(&conta, CONTA_SALDO_INICIAL_CENTAVOS);

    char valor_buf[32];
    char saldo_buf[32];
    char entrada[128];
    int opcao = -1;

    printf("====================================================\n");
    printf("       BEM-VINDO AO SISTEMA BANCARIO DIGITAL        \n");
    printf("====================================================\n");
    printf("Consultas de saldo, depositos, saques e extrato     \n");
    printf("com aritmetica exata em centavos.                   \n");
    printf("----------------------------------------------------\n");

    do {
        printf("\n========== MENU PRINCIPAL ==========\n");
        printf("1 - Consultar Saldo\n");
        printf("2 - Realizar Deposito\n");
        printf("3 - Realizar Saque\n");
        printf("4 - Consultar Extrato\n");
        printf("0 - Sair do Sistema\n");
        printf("====================================\n");
        printf("Digite a opcao desejada: ");

        if (!ler_opcao_menu(&opcao)) {
            printf("\n[AVISO] Fim da entrada detectado. Encerrando sessao...\n");
            break;
        }

        switch (opcao) {
            case 1:
                conta_formatar_reais(conta.saldo_centavos, saldo_buf, sizeof(saldo_buf));
                printf("\n--- CONSULTA DE SALDO ---\n");
                printf("Saldo atual disponivel: R$ %s\n", saldo_buf);
                break;

            case 2: {
                printf("\n--- REALIZAR DEPOSITO ---\n");
                printf("Informe o valor para deposito (R$): ");
                if (!ler_linha(entrada, sizeof(entrada))) {
                    opcao = 0;
                    break;
                }
                int64_t centavos = 0;
                ContaStatus st = conta_parse_centavos(entrada, &centavos);
                if (st == CONTA_OK) {
                    st = conta_depositar(&conta, centavos);
                }
                if (st == CONTA_OK) {
                    conta_formatar_reais(centavos, valor_buf, sizeof(valor_buf));
                    conta_formatar_reais(conta.saldo_centavos, saldo_buf, sizeof(saldo_buf));
                    printf("Deposito de R$ %s realizado com sucesso!\n", valor_buf);
                    printf("Novo saldo atual: R$ %s\n", saldo_buf);
                } else {
                    printf("[ALERTA] %s\n", conta_status_mensagem(st));
                }
                break;
            }

            case 3: {
                printf("\n--- REALIZAR SAQUE ---\n");
                printf("Informe o valor para saque (R$): ");
                if (!ler_linha(entrada, sizeof(entrada))) {
                    opcao = 0;
                    break;
                }
                int64_t centavos = 0;
                ContaStatus st = conta_parse_centavos(entrada, &centavos);
                if (st == CONTA_OK) {
                    st = conta_sacar(&conta, centavos);
                }
                if (st == CONTA_OK) {
                    conta_formatar_reais(centavos, valor_buf, sizeof(valor_buf));
                    conta_formatar_reais(conta.saldo_centavos, saldo_buf, sizeof(saldo_buf));
                    printf("Saque de R$ %s realizado com sucesso!\n", valor_buf);
                    printf("Saldo restante: R$ %s\n", saldo_buf);
                } else if (st == CONTA_ERR_SALDO_INSUFICIENTE) {
                    conta_formatar_reais(conta.saldo_centavos, saldo_buf, sizeof(saldo_buf));
                    conta_formatar_reais(centavos, valor_buf, sizeof(valor_buf));
                    printf("[ALERTA] %s\n", conta_status_mensagem(st));
                    printf("Saldo disponivel: R$ %s | Tentativa: R$ %s\n", saldo_buf, valor_buf);
                } else {
                    printf("[ALERTA] %s\n", conta_status_mensagem(st));
                }
                break;
            }

            case 4:
                exibir_extrato(&conta);
                break;

            case 0:
                printf("\nEncerrando a sessao bancaria com seguranca...\n");
                break;

            default:
                printf("\n[ERRO] Opcao invalida! Digite um numero entre 0 e 4.\n");
                break;
        }
    } while (opcao != 0);

    conta_formatar_reais(conta.saldo_centavos, saldo_buf, sizeof(saldo_buf));
    printf("\n====================================================\n");
    printf("         SESSAO ENCERRADA COM SUCESSO               \n");
    printf("====================================================\n");
    printf("Saldo final da conta : R$ %s\n", saldo_buf);
    printf("Operacoes realizadas : %zu\n", conta.total_operacoes);
    printf("Obrigado por utilizar o Sistema Bancario Digital!\n");
    printf("====================================================\n");

    return EXIT_SUCCESS;
}
