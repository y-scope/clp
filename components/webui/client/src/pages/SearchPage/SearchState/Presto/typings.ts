/**
 * Presto SQL interface types.
 */
export enum PRESTO_SQL_INTERFACE {
    /**
     * Guided interface with limited SQL dialect.
     */
    GUIDED = "guided",

    /**
     * Freeform interface with SQL editor.
     */
    FREEFORM = "freeform",
}
