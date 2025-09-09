// @ts-nocheck
// Generated from SqlBase.g4 by ANTLR 4.13.2

import {ParseTreeVisitor} from 'antlr4';


import { SingleStatementContext } from "./SqlBaseParser.js";
import { StandaloneExpressionContext } from "./SqlBaseParser.js";
import { StandaloneRoutineBodyContext } from "./SqlBaseParser.js";
import { StatementDefaultContext } from "./SqlBaseParser.js";
import { UseContext } from "./SqlBaseParser.js";
import { CreateSchemaContext } from "./SqlBaseParser.js";
import { DropSchemaContext } from "./SqlBaseParser.js";
import { RenameSchemaContext } from "./SqlBaseParser.js";
import { CreateTableAsSelectContext } from "./SqlBaseParser.js";
import { CreateTableContext } from "./SqlBaseParser.js";
import { DropTableContext } from "./SqlBaseParser.js";
import { InsertIntoContext } from "./SqlBaseParser.js";
import { DeleteContext } from "./SqlBaseParser.js";
import { TruncateTableContext } from "./SqlBaseParser.js";
import { RenameTableContext } from "./SqlBaseParser.js";
import { RenameColumnContext } from "./SqlBaseParser.js";
import { DropColumnContext } from "./SqlBaseParser.js";
import { AddColumnContext } from "./SqlBaseParser.js";
import { AddConstraintContext } from "./SqlBaseParser.js";
import { DropConstraintContext } from "./SqlBaseParser.js";
import { AlterColumnSetNotNullContext } from "./SqlBaseParser.js";
import { AlterColumnDropNotNullContext } from "./SqlBaseParser.js";
import { SetTablePropertiesContext } from "./SqlBaseParser.js";
import { AnalyzeContext } from "./SqlBaseParser.js";
import { CreateTypeContext } from "./SqlBaseParser.js";
import { CreateViewContext } from "./SqlBaseParser.js";
import { RenameViewContext } from "./SqlBaseParser.js";
import { DropViewContext } from "./SqlBaseParser.js";
import { CreateMaterializedViewContext } from "./SqlBaseParser.js";
import { DropMaterializedViewContext } from "./SqlBaseParser.js";
import { RefreshMaterializedViewContext } from "./SqlBaseParser.js";
import { CreateFunctionContext } from "./SqlBaseParser.js";
import { AlterFunctionContext } from "./SqlBaseParser.js";
import { DropFunctionContext } from "./SqlBaseParser.js";
import { CallContext } from "./SqlBaseParser.js";
import { CreateRoleContext } from "./SqlBaseParser.js";
import { DropRoleContext } from "./SqlBaseParser.js";
import { GrantRolesContext } from "./SqlBaseParser.js";
import { RevokeRolesContext } from "./SqlBaseParser.js";
import { SetRoleContext } from "./SqlBaseParser.js";
import { GrantContext } from "./SqlBaseParser.js";
import { RevokeContext } from "./SqlBaseParser.js";
import { ShowGrantsContext } from "./SqlBaseParser.js";
import { ExplainContext } from "./SqlBaseParser.js";
import { ShowCreateTableContext } from "./SqlBaseParser.js";
import { ShowCreateSchemaContext } from "./SqlBaseParser.js";
import { ShowCreateViewContext } from "./SqlBaseParser.js";
import { ShowCreateMaterializedViewContext } from "./SqlBaseParser.js";
import { ShowCreateFunctionContext } from "./SqlBaseParser.js";
import { ShowTablesContext } from "./SqlBaseParser.js";
import { ShowSchemasContext } from "./SqlBaseParser.js";
import { ShowCatalogsContext } from "./SqlBaseParser.js";
import { ShowColumnsContext } from "./SqlBaseParser.js";
import { ShowStatsContext } from "./SqlBaseParser.js";
import { ShowStatsForQueryContext } from "./SqlBaseParser.js";
import { ShowRolesContext } from "./SqlBaseParser.js";
import { ShowRoleGrantsContext } from "./SqlBaseParser.js";
import { ShowFunctionsContext } from "./SqlBaseParser.js";
import { ShowSessionContext } from "./SqlBaseParser.js";
import { SetSessionContext } from "./SqlBaseParser.js";
import { ResetSessionContext } from "./SqlBaseParser.js";
import { StartTransactionContext } from "./SqlBaseParser.js";
import { CommitContext } from "./SqlBaseParser.js";
import { RollbackContext } from "./SqlBaseParser.js";
import { PrepareContext } from "./SqlBaseParser.js";
import { DeallocateContext } from "./SqlBaseParser.js";
import { ExecuteContext } from "./SqlBaseParser.js";
import { DescribeInputContext } from "./SqlBaseParser.js";
import { DescribeOutputContext } from "./SqlBaseParser.js";
import { UpdateContext } from "./SqlBaseParser.js";
import { QueryContext } from "./SqlBaseParser.js";
import { WithContext } from "./SqlBaseParser.js";
import { TableElementContext } from "./SqlBaseParser.js";
import { ColumnDefinitionContext } from "./SqlBaseParser.js";
import { LikeClauseContext } from "./SqlBaseParser.js";
import { PropertiesContext } from "./SqlBaseParser.js";
import { PropertyContext } from "./SqlBaseParser.js";
import { SqlParameterDeclarationContext } from "./SqlBaseParser.js";
import { RoutineCharacteristicsContext } from "./SqlBaseParser.js";
import { RoutineCharacteristicContext } from "./SqlBaseParser.js";
import { AlterRoutineCharacteristicsContext } from "./SqlBaseParser.js";
import { AlterRoutineCharacteristicContext } from "./SqlBaseParser.js";
import { RoutineBodyContext } from "./SqlBaseParser.js";
import { ReturnStatementContext } from "./SqlBaseParser.js";
import { ExternalBodyReferenceContext } from "./SqlBaseParser.js";
import { LanguageContext } from "./SqlBaseParser.js";
import { DeterminismContext } from "./SqlBaseParser.js";
import { NullCallClauseContext } from "./SqlBaseParser.js";
import { ExternalRoutineNameContext } from "./SqlBaseParser.js";
import { QueryNoWithContext } from "./SqlBaseParser.js";
import { QueryTermDefaultContext } from "./SqlBaseParser.js";
import { SetOperationContext } from "./SqlBaseParser.js";
import { QueryPrimaryDefaultContext } from "./SqlBaseParser.js";
import { TableContext } from "./SqlBaseParser.js";
import { InlineTableContext } from "./SqlBaseParser.js";
import { SubqueryContext } from "./SqlBaseParser.js";
import { SortItemContext } from "./SqlBaseParser.js";
import { QuerySpecificationContext } from "./SqlBaseParser.js";
import { GroupByContext } from "./SqlBaseParser.js";
import { SingleGroupingSetContext } from "./SqlBaseParser.js";
import { RollupContext } from "./SqlBaseParser.js";
import { CubeContext } from "./SqlBaseParser.js";
import { MultipleGroupingSetsContext } from "./SqlBaseParser.js";
import { GroupingSetContext } from "./SqlBaseParser.js";
import { NamedQueryContext } from "./SqlBaseParser.js";
import { SetQuantifierContext } from "./SqlBaseParser.js";
import { SelectSingleContext } from "./SqlBaseParser.js";
import { SelectAllContext } from "./SqlBaseParser.js";
import { RelationDefaultContext } from "./SqlBaseParser.js";
import { JoinRelationContext } from "./SqlBaseParser.js";
import { JoinTypeContext } from "./SqlBaseParser.js";
import { JoinCriteriaContext } from "./SqlBaseParser.js";
import { SampledRelationContext } from "./SqlBaseParser.js";
import { SampleTypeContext } from "./SqlBaseParser.js";
import { AliasedRelationContext } from "./SqlBaseParser.js";
import { ColumnAliasesContext } from "./SqlBaseParser.js";
import { TableNameContext } from "./SqlBaseParser.js";
import { SubqueryRelationContext } from "./SqlBaseParser.js";
import { UnnestContext } from "./SqlBaseParser.js";
import { LateralContext } from "./SqlBaseParser.js";
import { ParenthesizedRelationContext } from "./SqlBaseParser.js";
import { ExpressionContext } from "./SqlBaseParser.js";
import { LogicalNotContext } from "./SqlBaseParser.js";
import { PredicatedContext } from "./SqlBaseParser.js";
import { LogicalBinaryContext } from "./SqlBaseParser.js";
import { ComparisonContext } from "./SqlBaseParser.js";
import { QuantifiedComparisonContext } from "./SqlBaseParser.js";
import { BetweenContext } from "./SqlBaseParser.js";
import { InListContext } from "./SqlBaseParser.js";
import { InSubqueryContext } from "./SqlBaseParser.js";
import { LikeContext } from "./SqlBaseParser.js";
import { NullPredicateContext } from "./SqlBaseParser.js";
import { DistinctFromContext } from "./SqlBaseParser.js";
import { ValueExpressionDefaultContext } from "./SqlBaseParser.js";
import { ConcatenationContext } from "./SqlBaseParser.js";
import { ArithmeticBinaryContext } from "./SqlBaseParser.js";
import { ArithmeticUnaryContext } from "./SqlBaseParser.js";
import { AtTimeZoneContext } from "./SqlBaseParser.js";
import { DereferenceContext } from "./SqlBaseParser.js";
import { TypeConstructorContext } from "./SqlBaseParser.js";
import { SpecialDateTimeFunctionContext } from "./SqlBaseParser.js";
import { SubstringContext } from "./SqlBaseParser.js";
import { CastContext } from "./SqlBaseParser.js";
import { LambdaContext } from "./SqlBaseParser.js";
import { ParenthesizedExpressionContext } from "./SqlBaseParser.js";
import { ParameterContext } from "./SqlBaseParser.js";
import { NormalizeContext } from "./SqlBaseParser.js";
import { IntervalLiteralContext } from "./SqlBaseParser.js";
import { NumericLiteralContext } from "./SqlBaseParser.js";
import { BooleanLiteralContext } from "./SqlBaseParser.js";
import { SimpleCaseContext } from "./SqlBaseParser.js";
import { ColumnReferenceContext } from "./SqlBaseParser.js";
import { NullLiteralContext } from "./SqlBaseParser.js";
import { RowConstructorContext } from "./SqlBaseParser.js";
import { SubscriptContext } from "./SqlBaseParser.js";
import { SubqueryExpressionContext } from "./SqlBaseParser.js";
import { BinaryLiteralContext } from "./SqlBaseParser.js";
import { CurrentUserContext } from "./SqlBaseParser.js";
import { ExtractContext } from "./SqlBaseParser.js";
import { StringLiteralContext } from "./SqlBaseParser.js";
import { ArrayConstructorContext } from "./SqlBaseParser.js";
import { FunctionCallContext } from "./SqlBaseParser.js";
import { ExistsContext } from "./SqlBaseParser.js";
import { PositionContext } from "./SqlBaseParser.js";
import { SearchedCaseContext } from "./SqlBaseParser.js";
import { GroupingOperationContext } from "./SqlBaseParser.js";
import { BasicStringLiteralContext } from "./SqlBaseParser.js";
import { UnicodeStringLiteralContext } from "./SqlBaseParser.js";
import { NullTreatmentContext } from "./SqlBaseParser.js";
import { TimeZoneIntervalContext } from "./SqlBaseParser.js";
import { TimeZoneStringContext } from "./SqlBaseParser.js";
import { ComparisonOperatorContext } from "./SqlBaseParser.js";
import { ComparisonQuantifierContext } from "./SqlBaseParser.js";
import { BooleanValueContext } from "./SqlBaseParser.js";
import { IntervalContext } from "./SqlBaseParser.js";
import { IntervalFieldContext } from "./SqlBaseParser.js";
import { NormalFormContext } from "./SqlBaseParser.js";
import { TypesContext } from "./SqlBaseParser.js";
import { TypeContext } from "./SqlBaseParser.js";
import { TypeParameterContext } from "./SqlBaseParser.js";
import { BaseTypeContext } from "./SqlBaseParser.js";
import { WhenClauseContext } from "./SqlBaseParser.js";
import { FilterContext } from "./SqlBaseParser.js";
import { OverContext } from "./SqlBaseParser.js";
import { WindowFrameContext } from "./SqlBaseParser.js";
import { UnboundedFrameContext } from "./SqlBaseParser.js";
import { CurrentRowBoundContext } from "./SqlBaseParser.js";
import { BoundedFrameContext } from "./SqlBaseParser.js";
import { UpdateAssignmentContext } from "./SqlBaseParser.js";
import { ExplainFormatContext } from "./SqlBaseParser.js";
import { ExplainTypeContext } from "./SqlBaseParser.js";
import { IsolationLevelContext } from "./SqlBaseParser.js";
import { TransactionAccessModeContext } from "./SqlBaseParser.js";
import { ReadUncommittedContext } from "./SqlBaseParser.js";
import { ReadCommittedContext } from "./SqlBaseParser.js";
import { RepeatableReadContext } from "./SqlBaseParser.js";
import { SerializableContext } from "./SqlBaseParser.js";
import { PositionalArgumentContext } from "./SqlBaseParser.js";
import { NamedArgumentContext } from "./SqlBaseParser.js";
import { PrivilegeContext } from "./SqlBaseParser.js";
import { QualifiedNameContext } from "./SqlBaseParser.js";
import { TableVersionContext } from "./SqlBaseParser.js";
import { TableversionasofContext } from "./SqlBaseParser.js";
import { TableversionbeforeContext } from "./SqlBaseParser.js";
import { CurrentUserGrantorContext } from "./SqlBaseParser.js";
import { CurrentRoleGrantorContext } from "./SqlBaseParser.js";
import { SpecifiedPrincipalContext } from "./SqlBaseParser.js";
import { UserPrincipalContext } from "./SqlBaseParser.js";
import { RolePrincipalContext } from "./SqlBaseParser.js";
import { UnspecifiedPrincipalContext } from "./SqlBaseParser.js";
import { RolesContext } from "./SqlBaseParser.js";
import { UnquotedIdentifierContext } from "./SqlBaseParser.js";
import { QuotedIdentifierContext } from "./SqlBaseParser.js";
import { BackQuotedIdentifierContext } from "./SqlBaseParser.js";
import { DigitIdentifierContext } from "./SqlBaseParser.js";
import { DecimalLiteralContext } from "./SqlBaseParser.js";
import { DoubleLiteralContext } from "./SqlBaseParser.js";
import { IntegerLiteralContext } from "./SqlBaseParser.js";
import { ConstraintSpecificationContext } from "./SqlBaseParser.js";
import { NamedConstraintSpecificationContext } from "./SqlBaseParser.js";
import { UnnamedConstraintSpecificationContext } from "./SqlBaseParser.js";
import { ConstraintTypeContext } from "./SqlBaseParser.js";
import { ConstraintQualifiersContext } from "./SqlBaseParser.js";
import { ConstraintQualifierContext } from "./SqlBaseParser.js";
import { ConstraintRelyContext } from "./SqlBaseParser.js";
import { ConstraintEnabledContext } from "./SqlBaseParser.js";
import { ConstraintEnforcedContext } from "./SqlBaseParser.js";
import { NonReservedContext } from "./SqlBaseParser.js";


/**
 * This interface defines a complete generic visitor for a parse tree produced
 * by `SqlBaseParser`.
 *
 * @param <Result> The return type of the visit operation. Use `void` for
 * operations with no return type.
 */
export default class SqlBaseVisitor<Result> extends ParseTreeVisitor<Result> {
	/**
	 * Visit a parse tree produced by `SqlBaseParser.singleStatement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSingleStatement?: (ctx: SingleStatementContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.standaloneExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitStandaloneExpression?: (ctx: StandaloneExpressionContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.standaloneRoutineBody`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitStandaloneRoutineBody?: (ctx: StandaloneRoutineBodyContext) => Result;
	/**
	 * Visit a parse tree produced by the `statementDefault`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitStatementDefault?: (ctx: StatementDefaultContext) => Result;
	/**
	 * Visit a parse tree produced by the `use`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUse?: (ctx: UseContext) => Result;
	/**
	 * Visit a parse tree produced by the `createSchema`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateSchema?: (ctx: CreateSchemaContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropSchema`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropSchema?: (ctx: DropSchemaContext) => Result;
	/**
	 * Visit a parse tree produced by the `renameSchema`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRenameSchema?: (ctx: RenameSchemaContext) => Result;
	/**
	 * Visit a parse tree produced by the `createTableAsSelect`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateTableAsSelect?: (ctx: CreateTableAsSelectContext) => Result;
	/**
	 * Visit a parse tree produced by the `createTable`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateTable?: (ctx: CreateTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropTable`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropTable?: (ctx: DropTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `insertInto`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitInsertInto?: (ctx: InsertIntoContext) => Result;
	/**
	 * Visit a parse tree produced by the `delete`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDelete?: (ctx: DeleteContext) => Result;
	/**
	 * Visit a parse tree produced by the `truncateTable`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTruncateTable?: (ctx: TruncateTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `renameTable`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRenameTable?: (ctx: RenameTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `renameColumn`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRenameColumn?: (ctx: RenameColumnContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropColumn`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropColumn?: (ctx: DropColumnContext) => Result;
	/**
	 * Visit a parse tree produced by the `addColumn`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAddColumn?: (ctx: AddColumnContext) => Result;
	/**
	 * Visit a parse tree produced by the `addConstraint`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAddConstraint?: (ctx: AddConstraintContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropConstraint`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropConstraint?: (ctx: DropConstraintContext) => Result;
	/**
	 * Visit a parse tree produced by the `alterColumnSetNotNull`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAlterColumnSetNotNull?: (ctx: AlterColumnSetNotNullContext) => Result;
	/**
	 * Visit a parse tree produced by the `alterColumnDropNotNull`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAlterColumnDropNotNull?: (ctx: AlterColumnDropNotNullContext) => Result;
	/**
	 * Visit a parse tree produced by the `setTableProperties`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSetTableProperties?: (ctx: SetTablePropertiesContext) => Result;
	/**
	 * Visit a parse tree produced by the `analyze`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAnalyze?: (ctx: AnalyzeContext) => Result;
	/**
	 * Visit a parse tree produced by the `createType`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateType?: (ctx: CreateTypeContext) => Result;
	/**
	 * Visit a parse tree produced by the `createView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateView?: (ctx: CreateViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `renameView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRenameView?: (ctx: RenameViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropView?: (ctx: DropViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `createMaterializedView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateMaterializedView?: (ctx: CreateMaterializedViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropMaterializedView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropMaterializedView?: (ctx: DropMaterializedViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `refreshMaterializedView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRefreshMaterializedView?: (ctx: RefreshMaterializedViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `createFunction`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateFunction?: (ctx: CreateFunctionContext) => Result;
	/**
	 * Visit a parse tree produced by the `alterFunction`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAlterFunction?: (ctx: AlterFunctionContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropFunction`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropFunction?: (ctx: DropFunctionContext) => Result;
	/**
	 * Visit a parse tree produced by the `call`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCall?: (ctx: CallContext) => Result;
	/**
	 * Visit a parse tree produced by the `createRole`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCreateRole?: (ctx: CreateRoleContext) => Result;
	/**
	 * Visit a parse tree produced by the `dropRole`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDropRole?: (ctx: DropRoleContext) => Result;
	/**
	 * Visit a parse tree produced by the `grantRoles`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitGrantRoles?: (ctx: GrantRolesContext) => Result;
	/**
	 * Visit a parse tree produced by the `revokeRoles`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRevokeRoles?: (ctx: RevokeRolesContext) => Result;
	/**
	 * Visit a parse tree produced by the `setRole`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSetRole?: (ctx: SetRoleContext) => Result;
	/**
	 * Visit a parse tree produced by the `grant`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitGrant?: (ctx: GrantContext) => Result;
	/**
	 * Visit a parse tree produced by the `revoke`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRevoke?: (ctx: RevokeContext) => Result;
	/**
	 * Visit a parse tree produced by the `showGrants`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowGrants?: (ctx: ShowGrantsContext) => Result;
	/**
	 * Visit a parse tree produced by the `explain`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExplain?: (ctx: ExplainContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCreateTable`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCreateTable?: (ctx: ShowCreateTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCreateSchema`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCreateSchema?: (ctx: ShowCreateSchemaContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCreateView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCreateView?: (ctx: ShowCreateViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCreateMaterializedView`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCreateMaterializedView?: (ctx: ShowCreateMaterializedViewContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCreateFunction`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCreateFunction?: (ctx: ShowCreateFunctionContext) => Result;
	/**
	 * Visit a parse tree produced by the `showTables`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowTables?: (ctx: ShowTablesContext) => Result;
	/**
	 * Visit a parse tree produced by the `showSchemas`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowSchemas?: (ctx: ShowSchemasContext) => Result;
	/**
	 * Visit a parse tree produced by the `showCatalogs`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowCatalogs?: (ctx: ShowCatalogsContext) => Result;
	/**
	 * Visit a parse tree produced by the `showColumns`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowColumns?: (ctx: ShowColumnsContext) => Result;
	/**
	 * Visit a parse tree produced by the `showStats`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowStats?: (ctx: ShowStatsContext) => Result;
	/**
	 * Visit a parse tree produced by the `showStatsForQuery`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowStatsForQuery?: (ctx: ShowStatsForQueryContext) => Result;
	/**
	 * Visit a parse tree produced by the `showRoles`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowRoles?: (ctx: ShowRolesContext) => Result;
	/**
	 * Visit a parse tree produced by the `showRoleGrants`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowRoleGrants?: (ctx: ShowRoleGrantsContext) => Result;
	/**
	 * Visit a parse tree produced by the `showFunctions`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowFunctions?: (ctx: ShowFunctionsContext) => Result;
	/**
	 * Visit a parse tree produced by the `showSession`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitShowSession?: (ctx: ShowSessionContext) => Result;
	/**
	 * Visit a parse tree produced by the `setSession`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSetSession?: (ctx: SetSessionContext) => Result;
	/**
	 * Visit a parse tree produced by the `resetSession`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitResetSession?: (ctx: ResetSessionContext) => Result;
	/**
	 * Visit a parse tree produced by the `startTransaction`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitStartTransaction?: (ctx: StartTransactionContext) => Result;
	/**
	 * Visit a parse tree produced by the `commit`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCommit?: (ctx: CommitContext) => Result;
	/**
	 * Visit a parse tree produced by the `rollback`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRollback?: (ctx: RollbackContext) => Result;
	/**
	 * Visit a parse tree produced by the `prepare`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitPrepare?: (ctx: PrepareContext) => Result;
	/**
	 * Visit a parse tree produced by the `deallocate`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDeallocate?: (ctx: DeallocateContext) => Result;
	/**
	 * Visit a parse tree produced by the `execute`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExecute?: (ctx: ExecuteContext) => Result;
	/**
	 * Visit a parse tree produced by the `describeInput`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDescribeInput?: (ctx: DescribeInputContext) => Result;
	/**
	 * Visit a parse tree produced by the `describeOutput`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDescribeOutput?: (ctx: DescribeOutputContext) => Result;
	/**
	 * Visit a parse tree produced by the `update`
	 * labeled alternative in `SqlBaseParser.statement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUpdate?: (ctx: UpdateContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.query`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQuery?: (ctx: QueryContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.with`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitWith?: (ctx: WithContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.tableElement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTableElement?: (ctx: TableElementContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.columnDefinition`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitColumnDefinition?: (ctx: ColumnDefinitionContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.likeClause`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLikeClause?: (ctx: LikeClauseContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.properties`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitProperties?: (ctx: PropertiesContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.property`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitProperty?: (ctx: PropertyContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.sqlParameterDeclaration`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSqlParameterDeclaration?: (ctx: SqlParameterDeclarationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.routineCharacteristics`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRoutineCharacteristics?: (ctx: RoutineCharacteristicsContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.routineCharacteristic`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRoutineCharacteristic?: (ctx: RoutineCharacteristicContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.alterRoutineCharacteristics`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAlterRoutineCharacteristics?: (ctx: AlterRoutineCharacteristicsContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.alterRoutineCharacteristic`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAlterRoutineCharacteristic?: (ctx: AlterRoutineCharacteristicContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.routineBody`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRoutineBody?: (ctx: RoutineBodyContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.returnStatement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitReturnStatement?: (ctx: ReturnStatementContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.externalBodyReference`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExternalBodyReference?: (ctx: ExternalBodyReferenceContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.language`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLanguage?: (ctx: LanguageContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.determinism`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDeterminism?: (ctx: DeterminismContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.nullCallClause`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNullCallClause?: (ctx: NullCallClauseContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.externalRoutineName`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExternalRoutineName?: (ctx: ExternalRoutineNameContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.queryNoWith`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQueryNoWith?: (ctx: QueryNoWithContext) => Result;
	/**
	 * Visit a parse tree produced by the `queryTermDefault`
	 * labeled alternative in `SqlBaseParser.queryTerm`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQueryTermDefault?: (ctx: QueryTermDefaultContext) => Result;
	/**
	 * Visit a parse tree produced by the `setOperation`
	 * labeled alternative in `SqlBaseParser.queryTerm`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSetOperation?: (ctx: SetOperationContext) => Result;
	/**
	 * Visit a parse tree produced by the `queryPrimaryDefault`
	 * labeled alternative in `SqlBaseParser.queryPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQueryPrimaryDefault?: (ctx: QueryPrimaryDefaultContext) => Result;
	/**
	 * Visit a parse tree produced by the `table`
	 * labeled alternative in `SqlBaseParser.queryPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTable?: (ctx: TableContext) => Result;
	/**
	 * Visit a parse tree produced by the `inlineTable`
	 * labeled alternative in `SqlBaseParser.queryPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitInlineTable?: (ctx: InlineTableContext) => Result;
	/**
	 * Visit a parse tree produced by the `subquery`
	 * labeled alternative in `SqlBaseParser.queryPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSubquery?: (ctx: SubqueryContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.sortItem`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSortItem?: (ctx: SortItemContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.querySpecification`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQuerySpecification?: (ctx: QuerySpecificationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.groupBy`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitGroupBy?: (ctx: GroupByContext) => Result;
	/**
	 * Visit a parse tree produced by the `singleGroupingSet`
	 * labeled alternative in `SqlBaseParser.groupingElement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSingleGroupingSet?: (ctx: SingleGroupingSetContext) => Result;
	/**
	 * Visit a parse tree produced by the `rollup`
	 * labeled alternative in `SqlBaseParser.groupingElement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRollup?: (ctx: RollupContext) => Result;
	/**
	 * Visit a parse tree produced by the `cube`
	 * labeled alternative in `SqlBaseParser.groupingElement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCube?: (ctx: CubeContext) => Result;
	/**
	 * Visit a parse tree produced by the `multipleGroupingSets`
	 * labeled alternative in `SqlBaseParser.groupingElement`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitMultipleGroupingSets?: (ctx: MultipleGroupingSetsContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.groupingSet`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitGroupingSet?: (ctx: GroupingSetContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.namedQuery`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNamedQuery?: (ctx: NamedQueryContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.setQuantifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSetQuantifier?: (ctx: SetQuantifierContext) => Result;
	/**
	 * Visit a parse tree produced by the `selectSingle`
	 * labeled alternative in `SqlBaseParser.selectItem`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSelectSingle?: (ctx: SelectSingleContext) => Result;
	/**
	 * Visit a parse tree produced by the `selectAll`
	 * labeled alternative in `SqlBaseParser.selectItem`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSelectAll?: (ctx: SelectAllContext) => Result;
	/**
	 * Visit a parse tree produced by the `relationDefault`
	 * labeled alternative in `SqlBaseParser.relation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRelationDefault?: (ctx: RelationDefaultContext) => Result;
	/**
	 * Visit a parse tree produced by the `joinRelation`
	 * labeled alternative in `SqlBaseParser.relation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitJoinRelation?: (ctx: JoinRelationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.joinType`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitJoinType?: (ctx: JoinTypeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.joinCriteria`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitJoinCriteria?: (ctx: JoinCriteriaContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.sampledRelation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSampledRelation?: (ctx: SampledRelationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.sampleType`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSampleType?: (ctx: SampleTypeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.aliasedRelation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAliasedRelation?: (ctx: AliasedRelationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.columnAliases`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitColumnAliases?: (ctx: ColumnAliasesContext) => Result;
	/**
	 * Visit a parse tree produced by the `tableName`
	 * labeled alternative in `SqlBaseParser.relationPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTableName?: (ctx: TableNameContext) => Result;
	/**
	 * Visit a parse tree produced by the `subqueryRelation`
	 * labeled alternative in `SqlBaseParser.relationPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSubqueryRelation?: (ctx: SubqueryRelationContext) => Result;
	/**
	 * Visit a parse tree produced by the `unnest`
	 * labeled alternative in `SqlBaseParser.relationPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnnest?: (ctx: UnnestContext) => Result;
	/**
	 * Visit a parse tree produced by the `lateral`
	 * labeled alternative in `SqlBaseParser.relationPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLateral?: (ctx: LateralContext) => Result;
	/**
	 * Visit a parse tree produced by the `parenthesizedRelation`
	 * labeled alternative in `SqlBaseParser.relationPrimary`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitParenthesizedRelation?: (ctx: ParenthesizedRelationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.expression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExpression?: (ctx: ExpressionContext) => Result;
	/**
	 * Visit a parse tree produced by the `logicalNot`
	 * labeled alternative in `SqlBaseParser.booleanExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLogicalNot?: (ctx: LogicalNotContext) => Result;
	/**
	 * Visit a parse tree produced by the `predicated`
	 * labeled alternative in `SqlBaseParser.booleanExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitPredicated?: (ctx: PredicatedContext) => Result;
	/**
	 * Visit a parse tree produced by the `logicalBinary`
	 * labeled alternative in `SqlBaseParser.booleanExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLogicalBinary?: (ctx: LogicalBinaryContext) => Result;
	/**
	 * Visit a parse tree produced by the `comparison`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitComparison?: (ctx: ComparisonContext) => Result;
	/**
	 * Visit a parse tree produced by the `quantifiedComparison`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQuantifiedComparison?: (ctx: QuantifiedComparisonContext) => Result;
	/**
	 * Visit a parse tree produced by the `between`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBetween?: (ctx: BetweenContext) => Result;
	/**
	 * Visit a parse tree produced by the `inList`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitInList?: (ctx: InListContext) => Result;
	/**
	 * Visit a parse tree produced by the `inSubquery`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitInSubquery?: (ctx: InSubqueryContext) => Result;
	/**
	 * Visit a parse tree produced by the `like`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLike?: (ctx: LikeContext) => Result;
	/**
	 * Visit a parse tree produced by the `nullPredicate`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNullPredicate?: (ctx: NullPredicateContext) => Result;
	/**
	 * Visit a parse tree produced by the `distinctFrom`
	 * labeled alternative in `SqlBaseParser.predicate`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDistinctFrom?: (ctx: DistinctFromContext) => Result;
	/**
	 * Visit a parse tree produced by the `valueExpressionDefault`
	 * labeled alternative in `SqlBaseParser.valueExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitValueExpressionDefault?: (ctx: ValueExpressionDefaultContext) => Result;
	/**
	 * Visit a parse tree produced by the `concatenation`
	 * labeled alternative in `SqlBaseParser.valueExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConcatenation?: (ctx: ConcatenationContext) => Result;
	/**
	 * Visit a parse tree produced by the `arithmeticBinary`
	 * labeled alternative in `SqlBaseParser.valueExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitArithmeticBinary?: (ctx: ArithmeticBinaryContext) => Result;
	/**
	 * Visit a parse tree produced by the `arithmeticUnary`
	 * labeled alternative in `SqlBaseParser.valueExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitArithmeticUnary?: (ctx: ArithmeticUnaryContext) => Result;
	/**
	 * Visit a parse tree produced by the `atTimeZone`
	 * labeled alternative in `SqlBaseParser.valueExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitAtTimeZone?: (ctx: AtTimeZoneContext) => Result;
	/**
	 * Visit a parse tree produced by the `dereference`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDereference?: (ctx: DereferenceContext) => Result;
	/**
	 * Visit a parse tree produced by the `typeConstructor`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTypeConstructor?: (ctx: TypeConstructorContext) => Result;
	/**
	 * Visit a parse tree produced by the `specialDateTimeFunction`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSpecialDateTimeFunction?: (ctx: SpecialDateTimeFunctionContext) => Result;
	/**
	 * Visit a parse tree produced by the `substring`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSubstring?: (ctx: SubstringContext) => Result;
	/**
	 * Visit a parse tree produced by the `cast`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCast?: (ctx: CastContext) => Result;
	/**
	 * Visit a parse tree produced by the `lambda`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitLambda?: (ctx: LambdaContext) => Result;
	/**
	 * Visit a parse tree produced by the `parenthesizedExpression`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitParenthesizedExpression?: (ctx: ParenthesizedExpressionContext) => Result;
	/**
	 * Visit a parse tree produced by the `parameter`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitParameter?: (ctx: ParameterContext) => Result;
	/**
	 * Visit a parse tree produced by the `normalize`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNormalize?: (ctx: NormalizeContext) => Result;
	/**
	 * Visit a parse tree produced by the `intervalLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitIntervalLiteral?: (ctx: IntervalLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `numericLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNumericLiteral?: (ctx: NumericLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `booleanLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBooleanLiteral?: (ctx: BooleanLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `simpleCase`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSimpleCase?: (ctx: SimpleCaseContext) => Result;
	/**
	 * Visit a parse tree produced by the `columnReference`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitColumnReference?: (ctx: ColumnReferenceContext) => Result;
	/**
	 * Visit a parse tree produced by the `nullLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNullLiteral?: (ctx: NullLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `rowConstructor`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRowConstructor?: (ctx: RowConstructorContext) => Result;
	/**
	 * Visit a parse tree produced by the `subscript`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSubscript?: (ctx: SubscriptContext) => Result;
	/**
	 * Visit a parse tree produced by the `subqueryExpression`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSubqueryExpression?: (ctx: SubqueryExpressionContext) => Result;
	/**
	 * Visit a parse tree produced by the `binaryLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBinaryLiteral?: (ctx: BinaryLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `currentUser`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCurrentUser?: (ctx: CurrentUserContext) => Result;
	/**
	 * Visit a parse tree produced by the `extract`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExtract?: (ctx: ExtractContext) => Result;
	/**
	 * Visit a parse tree produced by the `stringLiteral`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitStringLiteral?: (ctx: StringLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `arrayConstructor`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitArrayConstructor?: (ctx: ArrayConstructorContext) => Result;
	/**
	 * Visit a parse tree produced by the `functionCall`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitFunctionCall?: (ctx: FunctionCallContext) => Result;
	/**
	 * Visit a parse tree produced by the `exists`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExists?: (ctx: ExistsContext) => Result;
	/**
	 * Visit a parse tree produced by the `position`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitPosition?: (ctx: PositionContext) => Result;
	/**
	 * Visit a parse tree produced by the `searchedCase`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSearchedCase?: (ctx: SearchedCaseContext) => Result;
	/**
	 * Visit a parse tree produced by the `groupingOperation`
	 * labeled alternative in `SqlBaseParser.primaryExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitGroupingOperation?: (ctx: GroupingOperationContext) => Result;
	/**
	 * Visit a parse tree produced by the `basicStringLiteral`
	 * labeled alternative in `SqlBaseParser.string`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBasicStringLiteral?: (ctx: BasicStringLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `unicodeStringLiteral`
	 * labeled alternative in `SqlBaseParser.string`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnicodeStringLiteral?: (ctx: UnicodeStringLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.nullTreatment`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNullTreatment?: (ctx: NullTreatmentContext) => Result;
	/**
	 * Visit a parse tree produced by the `timeZoneInterval`
	 * labeled alternative in `SqlBaseParser.timeZoneSpecifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTimeZoneInterval?: (ctx: TimeZoneIntervalContext) => Result;
	/**
	 * Visit a parse tree produced by the `timeZoneString`
	 * labeled alternative in `SqlBaseParser.timeZoneSpecifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTimeZoneString?: (ctx: TimeZoneStringContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.comparisonOperator`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitComparisonOperator?: (ctx: ComparisonOperatorContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.comparisonQuantifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitComparisonQuantifier?: (ctx: ComparisonQuantifierContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.booleanValue`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBooleanValue?: (ctx: BooleanValueContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.interval`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitInterval?: (ctx: IntervalContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.intervalField`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitIntervalField?: (ctx: IntervalFieldContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.normalForm`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNormalForm?: (ctx: NormalFormContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.types`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTypes?: (ctx: TypesContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.type`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitType?: (ctx: TypeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.typeParameter`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTypeParameter?: (ctx: TypeParameterContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.baseType`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBaseType?: (ctx: BaseTypeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.whenClause`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitWhenClause?: (ctx: WhenClauseContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.filter`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitFilter?: (ctx: FilterContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.over`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitOver?: (ctx: OverContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.windowFrame`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitWindowFrame?: (ctx: WindowFrameContext) => Result;
	/**
	 * Visit a parse tree produced by the `unboundedFrame`
	 * labeled alternative in `SqlBaseParser.frameBound`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnboundedFrame?: (ctx: UnboundedFrameContext) => Result;
	/**
	 * Visit a parse tree produced by the `currentRowBound`
	 * labeled alternative in `SqlBaseParser.frameBound`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCurrentRowBound?: (ctx: CurrentRowBoundContext) => Result;
	/**
	 * Visit a parse tree produced by the `boundedFrame`
	 * labeled alternative in `SqlBaseParser.frameBound`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBoundedFrame?: (ctx: BoundedFrameContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.updateAssignment`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUpdateAssignment?: (ctx: UpdateAssignmentContext) => Result;
	/**
	 * Visit a parse tree produced by the `explainFormat`
	 * labeled alternative in `SqlBaseParser.explainOption`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExplainFormat?: (ctx: ExplainFormatContext) => Result;
	/**
	 * Visit a parse tree produced by the `explainType`
	 * labeled alternative in `SqlBaseParser.explainOption`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitExplainType?: (ctx: ExplainTypeContext) => Result;
	/**
	 * Visit a parse tree produced by the `isolationLevel`
	 * labeled alternative in `SqlBaseParser.transactionMode`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitIsolationLevel?: (ctx: IsolationLevelContext) => Result;
	/**
	 * Visit a parse tree produced by the `transactionAccessMode`
	 * labeled alternative in `SqlBaseParser.transactionMode`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTransactionAccessMode?: (ctx: TransactionAccessModeContext) => Result;
	/**
	 * Visit a parse tree produced by the `readUncommitted`
	 * labeled alternative in `SqlBaseParser.levelOfIsolation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitReadUncommitted?: (ctx: ReadUncommittedContext) => Result;
	/**
	 * Visit a parse tree produced by the `readCommitted`
	 * labeled alternative in `SqlBaseParser.levelOfIsolation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitReadCommitted?: (ctx: ReadCommittedContext) => Result;
	/**
	 * Visit a parse tree produced by the `repeatableRead`
	 * labeled alternative in `SqlBaseParser.levelOfIsolation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRepeatableRead?: (ctx: RepeatableReadContext) => Result;
	/**
	 * Visit a parse tree produced by the `serializable`
	 * labeled alternative in `SqlBaseParser.levelOfIsolation`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSerializable?: (ctx: SerializableContext) => Result;
	/**
	 * Visit a parse tree produced by the `positionalArgument`
	 * labeled alternative in `SqlBaseParser.callArgument`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitPositionalArgument?: (ctx: PositionalArgumentContext) => Result;
	/**
	 * Visit a parse tree produced by the `namedArgument`
	 * labeled alternative in `SqlBaseParser.callArgument`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNamedArgument?: (ctx: NamedArgumentContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.privilege`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitPrivilege?: (ctx: PrivilegeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.qualifiedName`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQualifiedName?: (ctx: QualifiedNameContext) => Result;
	/**
	 * Visit a parse tree produced by the `tableVersion`
	 * labeled alternative in `SqlBaseParser.tableVersionExpression`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTableVersion?: (ctx: TableVersionContext) => Result;
	/**
	 * Visit a parse tree produced by the `tableversionasof`
	 * labeled alternative in `SqlBaseParser.tableVersionState`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTableversionasof?: (ctx: TableversionasofContext) => Result;
	/**
	 * Visit a parse tree produced by the `tableversionbefore`
	 * labeled alternative in `SqlBaseParser.tableVersionState`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitTableversionbefore?: (ctx: TableversionbeforeContext) => Result;
	/**
	 * Visit a parse tree produced by the `currentUserGrantor`
	 * labeled alternative in `SqlBaseParser.grantor`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCurrentUserGrantor?: (ctx: CurrentUserGrantorContext) => Result;
	/**
	 * Visit a parse tree produced by the `currentRoleGrantor`
	 * labeled alternative in `SqlBaseParser.grantor`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitCurrentRoleGrantor?: (ctx: CurrentRoleGrantorContext) => Result;
	/**
	 * Visit a parse tree produced by the `specifiedPrincipal`
	 * labeled alternative in `SqlBaseParser.grantor`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitSpecifiedPrincipal?: (ctx: SpecifiedPrincipalContext) => Result;
	/**
	 * Visit a parse tree produced by the `userPrincipal`
	 * labeled alternative in `SqlBaseParser.principal`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUserPrincipal?: (ctx: UserPrincipalContext) => Result;
	/**
	 * Visit a parse tree produced by the `rolePrincipal`
	 * labeled alternative in `SqlBaseParser.principal`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRolePrincipal?: (ctx: RolePrincipalContext) => Result;
	/**
	 * Visit a parse tree produced by the `unspecifiedPrincipal`
	 * labeled alternative in `SqlBaseParser.principal`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnspecifiedPrincipal?: (ctx: UnspecifiedPrincipalContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.roles`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitRoles?: (ctx: RolesContext) => Result;
	/**
	 * Visit a parse tree produced by the `unquotedIdentifier`
	 * labeled alternative in `SqlBaseParser.identifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnquotedIdentifier?: (ctx: UnquotedIdentifierContext) => Result;
	/**
	 * Visit a parse tree produced by the `quotedIdentifier`
	 * labeled alternative in `SqlBaseParser.identifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitQuotedIdentifier?: (ctx: QuotedIdentifierContext) => Result;
	/**
	 * Visit a parse tree produced by the `backQuotedIdentifier`
	 * labeled alternative in `SqlBaseParser.identifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitBackQuotedIdentifier?: (ctx: BackQuotedIdentifierContext) => Result;
	/**
	 * Visit a parse tree produced by the `digitIdentifier`
	 * labeled alternative in `SqlBaseParser.identifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDigitIdentifier?: (ctx: DigitIdentifierContext) => Result;
	/**
	 * Visit a parse tree produced by the `decimalLiteral`
	 * labeled alternative in `SqlBaseParser.number`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDecimalLiteral?: (ctx: DecimalLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `doubleLiteral`
	 * labeled alternative in `SqlBaseParser.number`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitDoubleLiteral?: (ctx: DoubleLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by the `integerLiteral`
	 * labeled alternative in `SqlBaseParser.number`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitIntegerLiteral?: (ctx: IntegerLiteralContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintSpecification`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintSpecification?: (ctx: ConstraintSpecificationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.namedConstraintSpecification`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNamedConstraintSpecification?: (ctx: NamedConstraintSpecificationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.unnamedConstraintSpecification`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitUnnamedConstraintSpecification?: (ctx: UnnamedConstraintSpecificationContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintType`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintType?: (ctx: ConstraintTypeContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintQualifiers`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintQualifiers?: (ctx: ConstraintQualifiersContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintQualifier`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintQualifier?: (ctx: ConstraintQualifierContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintRely`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintRely?: (ctx: ConstraintRelyContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintEnabled`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintEnabled?: (ctx: ConstraintEnabledContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.constraintEnforced`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitConstraintEnforced?: (ctx: ConstraintEnforcedContext) => Result;
	/**
	 * Visit a parse tree produced by `SqlBaseParser.nonReserved`.
	 * @param ctx the parse tree
	 * @return the visitor result
	 */
	visitNonReserved?: (ctx: NonReservedContext) => Result;
}

