# Exemplo mínimo rt_log

Aplicação Zephyr completa, sem rt_diagnostics, placa ou periférico específico.
Executar na raiz do módulo: west build -b SUA_PLACA -d build examples/basic.
Requer backend console habilitado pelo alvo/aplicação; não grava automaticamente.
Exibe decimal/scientific a cada 5 s; DEBUG é compilado mas filtrado em runtime.
Intervalo é ilustrativo, não demonstra consumo elétrico.
