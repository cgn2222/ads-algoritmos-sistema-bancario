#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./build/sistema_bancario}"

echo "==> Testando sessao completa da CLI (incluindo entradas invalidas)"
OUT="$(
  "$BIN" <<'EOF'
abc
9
1
2
250,50
3
2000.00
3
150.25
4
0
EOF
)"

grep -q "Opcao invalida" <<<"$OUT"
grep -q "Saldo atual disponivel: R\$ 1000.00" <<<"$OUT"
grep -q "Deposito de R\$ 250.50 realizado com sucesso!" <<<"$OUT"
grep -q "Saldo insuficiente" <<<"$OUT"
grep -q "Saque de R\$ 150.25 realizado com sucesso!" <<<"$OUT"
grep -q "DEPOSITO" <<<"$OUT"
grep -q "SAQUE" <<<"$OUT"
grep -q "Saldo final da conta : R\$ 1100.25" <<<"$OUT"
echo "[OK] CLI bancaria validada com sucesso"
