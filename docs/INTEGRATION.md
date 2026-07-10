# Guia De Integracion

Este documento resume como integrar Keystone DTL en un servicio que necesita
custodiar garantias de rutas con ventanas temporales y limites por operador.

## Ciclo De Vida

1. Registrar los activos que el servicio acepta.
2. Registrar las cuentas que pueden pagar, recibir, operar o liquidar.
3. Configurar politicas por operador y activo.
4. Depositar saldo liquido del pagador.
5. Abrir posiciones con terminos de ruta completos.
6. Consultar riesgo antes de aceptar nuevas rutas.
7. Mover ventanas cuando el plan operativo cambie.
8. Liquidar, cancelar o expirar posiciones.
9. Reconciliar el activo despues de cada lote.
10. Persistir los eventos necesarios para auditoria externa.

## Politicas

Cada politica se identifica por `operator_id` y `asset`. Los dos techos
principales son:

- `max_active_debt`: deuda agregada admitida para el operador y activo.
- `max_window_debt`: deuda admitida en una ventana temporal concreta.

`min_margin_bps` permite rechazar rutas que no aportan margen suficiente y
`max_open_positions` limita la fragmentacion de una ventana.

## Aperturas

Una apertura valida requiere:

- cuenta pagadora registrada;
- beneficiario registrado;
- operador registrado como cuenta;
- activo registrado;
- politica existente para el operador y activo;
- garantia positiva;
- ventana temporal con `window_end > window_start`;
- expiracion posterior o igual al final de ventana;
- nonce no usado por el pagador.

La prima de ruta, cuando existe, se transfiere al operador durante la apertura.
La garantia queda bloqueada en la cuenta del pagador hasta que la posicion llega
a un estado terminal.

## Cierres

La liquidacion paga al liquidador el menor valor entre la reclamacion calculada
y la garantia bloqueada. El remanente vuelve al pagador. La cancelacion esta
limitada a una ventana previa a la ejecucion y la expiracion devuelve cualquier
garantia no reclamada.

Los integradores deberian tratar los estados terminales como finales:

- `liquidated`;
- `cancelled`;
- `expired`.

## Vistas

`kdtl_get_position` devuelve el estado canonico de una posicion. `digest`
permite detectar cambios de contenido entre snapshots.

`kdtl_get_operator_risk` devuelve una vista agregada para admision de nuevas
rutas y supervision operativa. Los campos mas utiles para alertas son:

- `indexed_debt`;
- `visible_reserved`;
- `claimable_open`;
- `available_debt`;
- `within_policy`.

`kdtl_reconcile_asset` agrega balances liquidos, balances bloqueados, posiciones
abiertas, posiciones terminales y numero de eventos registrados.

## Codigos De Error

La API devuelve `KDTL_OK` cuando la transicion se aplica. Cualquier otro codigo
debe tratarse como rechazo de la operacion completa por parte del motor.

Codigos habituales:

- `KDTL_ERR_ARGUMENT`: parametros incompletos o incoherentes.
- `KDTL_ERR_NOT_FOUND`: cuenta, activo, politica o posicion no registrada.
- `KDTL_ERR_EXISTS`: identificador o nonce ya usado.
- `KDTL_ERR_CAPACITY`: limite interno de almacenamiento alcanzado.
- `KDTL_ERR_BALANCE`: saldo liquido o bloqueado insuficiente.
- `KDTL_ERR_STATE`: transicion incompatible con el estado actual.
- `KDTL_ERR_LIMIT`: politica economica rechaza la operacion.
- `KDTL_ERR_OVERFLOW`: aritmetica entera fuera de rango.
- `KDTL_ERR_BUFFER`: buffer de salida demasiado pequeno.

Los integradores no deben convertir estos codigos en aprobaciones parciales. Si
un flujo necesita reintento, debe reconstruir los terminos de la operacion y
volver a llamar a la API con un estado conocido.

## Versionado De Snapshots

Los JSON emitidos por la CLI no sustituyen una API de almacenamiento. Su
objetivo es estabilizar contratos de integracion y facilitar comparaciones entre
builds. Si un servicio persiste estas salidas, conviene guardar tambien:

- nombre del escenario o flujo de negocio;
- version del binario;
- timestamp de captura;
- politica activa;
- identificador del lote procesado.

Las herramientas de auditoria pueden comparar estos snapshots sin conocer los
detalles internos de almacenamiento de la libreria.

Campos recomendados para diffs automatizados:

- `position.digest`;
- `risk.within_policy`;
- `reconciliation.accounted_total`.
- `event.sequence`.

## Recomendaciones De Servicio

- Persistir los terminos originales de cada ruta junto al ID de posicion.
- Ejecutar reconciliacion despues de cada lote de mutaciones.
- Rechazar integraciones que ignoren codigos de error distintos de `KDTL_OK`.
- Evitar truncar identificadores antes de llamar a la API.
- Usar timestamps monotonicamente crecientes por entorno de ejecucion.
- Ejecutar la suite Node contra cada build nativo distribuido.

## Escenarios Incluidos

La CLI `kdtl_scenario` expone escenarios JSON pensados para tests de contrato:

- `catalog`: lista capacidades disponibles.
- `open-basic`: apertura con prima y garantia.
- `limit-update`: actualizacion de politica.
- `liquidation`: cierre por reclamacion.
- `cancellation`: cancelacion previa a ejecucion.
- `expiration`: cierre por tiempo.
- `window-rollover`: cambio de ventana.
- `event-journal`: eventos de apertura y movimiento.
- `rejections`: rechazos esperados por politica.
- `operator-overview`: catalogo de activo y utilizacion de politica.

Estos escenarios son pequenos a proposito: deben ser faciles de inspeccionar,
deterministas y estables entre plataformas.
