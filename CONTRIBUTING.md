# Contribuir

## Flujo

1. Cree una rama desde `main`.
2. Mantenga cambios de API, contabilidad y documentacion en el mismo PR.
3. Añada un escenario CLI y un contrato Node para comportamiento nuevo.
4. Ejecute `npm ci` y `npm run ci`.
5. Documente compatibilidad y migracion cuando cambie una estructura publica.

La revision considera aritmetica, compensaciones de error, transiciones
terminales, indices temporales, capacidad fija, procedencia de timestamps y
estabilidad de JSON. Los hallazgos de seguridad se comunican por el proceso de
[SECURITY.md](./SECURITY.md), no mediante una incidencia publica.
