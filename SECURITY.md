# Seguridad

Keystone DTL modela garantias de rutas con contabilidad determinista. El alcance
de revision recomendado incluye la API publica, los modulos internos de limites,
la reconciliacion de balances, las reglas de liquidacion y los escenarios de
integracion.

## Invariantes Esperadas

- La suma de balances liquidos y garantias bloqueadas conserva el suministro
  depositado por activo.
- Una posicion solo puede estar en un estado terminal.
- La apertura requiere pagador, beneficiario, operador, activo y politica
  registrados.
- Los limites por operador y activo deben aplicarse antes de bloquear fondos.
- Las actualizaciones de limite no deben dejar ventanas activas fuera de
  politica.
- La liquidacion no puede pagar mas que la garantia bloqueada.
- La cancelacion y expiracion deben liberar la garantia pendiente al pagador.

## Validaciones

La suite publica cubre:

- apertura de posiciones;
- actualizacion de limites;
- liquidacion parcial y total;
- cancelacion antes de ejecucion;
- expiracion por tiempo;
- serializacion estable de vistas de posicion y riesgo.

Los scripts de CI compilan con advertencias estrictas (`-Wall -Wextra -Werror`)
y ejecutan los tests de contrato Node contra el binario de escenarios.

## Dependencias

La libreria C no tiene dependencias externas. Los tests usan modulos estandar de
Node.js y Prettier como dependencia de desarrollo opcional.

## Revision Operativa

Antes de desplegar una build en un servicio integrador se recomienda revisar:

- parametros de politica por operador y activo;
- ventanas admitidas por el servicio de routing;
- cuentas autorizadas para liquidar;
- tratamiento de codigos de error;
- persistencia de eventos;
- reconciliacion posterior a lotes;
- limites de tamano de identificadores;
- monotonicidad de timestamps;
- version del compilador usado en release;
- resultado de la suite `node:test`.

Los snapshots de posicion, riesgo, cuenta, evento y reconciliacion estan
pensados para compararse entre builds. Una diferencia de digest o de totales
debe revisarse antes de promover el binario.

## Reporte Interno

Los reportes deben incluir:

- version o commit revisado;
- escenario minimo reproducible;
- operador, activo y ventanas afectadas;
- impacto economico estimado;
- recomendacion de mitigacion;
- tests adicionales propuestos.

No se aceptan reportes basados solo en estilo, formato o supuestos que requieran
modificar codigo fuente del integrador.
