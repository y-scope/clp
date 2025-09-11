// @ts-nocheck
// Generated from SqlBase.g4 by ANTLR 4.13.2
// noinspection ES6UnusedImports,JSUnusedGlobalSymbols,JSUnusedLocalSymbols

import {
	ATN,
	ATNDeserializer, DecisionState, DFA, FailedPredicateException,
	RecognitionException, NoViableAltException, BailErrorStrategy,
	Parser, ParserATNSimulator,
	RuleContext, ParserRuleContext, PredictionMode, PredictionContextCache,
	TerminalNode, RuleNode,
	Token, TokenStream,
	Interval, IntervalSet
} from 'antlr4';
import SqlBaseVisitor from "./SqlBaseVisitor.js";

// for running tests with parameters, TODO: discuss strategy for typed parameters in CI
// eslint-disable-next-line no-unused-vars
type int = number;

export default class SqlBaseParser extends Parser {
	public static readonly T__0 = 1;
	public static readonly T__1 = 2;
	public static readonly T__2 = 3;
	public static readonly T__3 = 4;
	public static readonly T__4 = 5;
	public static readonly T__5 = 6;
	public static readonly T__6 = 7;
	public static readonly T__7 = 8;
	public static readonly T__8 = 9;
	public static readonly ADD = 10;
	public static readonly ADMIN = 11;
	public static readonly ALL = 12;
	public static readonly ALTER = 13;
	public static readonly ANALYZE = 14;
	public static readonly AND = 15;
	public static readonly ANY = 16;
	public static readonly ARRAY = 17;
	public static readonly AS = 18;
	public static readonly ASC = 19;
	public static readonly AT = 20;
	public static readonly BEFORE = 21;
	public static readonly BERNOULLI = 22;
	public static readonly BETWEEN = 23;
	public static readonly BY = 24;
	public static readonly CALL = 25;
	public static readonly CALLED = 26;
	public static readonly CASCADE = 27;
	public static readonly CASE = 28;
	public static readonly CAST = 29;
	public static readonly CATALOGS = 30;
	public static readonly COLUMN = 31;
	public static readonly COLUMNS = 32;
	public static readonly COMMENT = 33;
	public static readonly COMMIT = 34;
	public static readonly COMMITTED = 35;
	public static readonly CONSTRAINT = 36;
	public static readonly CREATE = 37;
	public static readonly CROSS = 38;
	public static readonly CUBE = 39;
	public static readonly CURRENT = 40;
	public static readonly CURRENT_DATE = 41;
	public static readonly CURRENT_ROLE = 42;
	public static readonly CURRENT_TIME = 43;
	public static readonly CURRENT_TIMESTAMP = 44;
	public static readonly CURRENT_USER = 45;
	public static readonly DATA = 46;
	public static readonly DATE = 47;
	public static readonly DAY = 48;
	public static readonly DEALLOCATE = 49;
	public static readonly DEFINER = 50;
	public static readonly DELETE = 51;
	public static readonly DESC = 52;
	public static readonly DESCRIBE = 53;
	public static readonly DETERMINISTIC = 54;
	public static readonly DISABLED = 55;
	public static readonly DISTINCT = 56;
	public static readonly DISTRIBUTED = 57;
	public static readonly DROP = 58;
	public static readonly ELSE = 59;
	public static readonly ENABLED = 60;
	public static readonly END = 61;
	public static readonly ENFORCED = 62;
	public static readonly ESCAPE = 63;
	public static readonly EXCEPT = 64;
	public static readonly EXCLUDING = 65;
	public static readonly EXECUTE = 66;
	public static readonly EXISTS = 67;
	public static readonly EXPLAIN = 68;
	public static readonly EXTRACT = 69;
	public static readonly EXTERNAL = 70;
	public static readonly FALSE = 71;
	public static readonly FETCH = 72;
	public static readonly FILTER = 73;
	public static readonly FIRST = 74;
	public static readonly FOLLOWING = 75;
	public static readonly FOR = 76;
	public static readonly FORMAT = 77;
	public static readonly FROM = 78;
	public static readonly FULL = 79;
	public static readonly FUNCTION = 80;
	public static readonly FUNCTIONS = 81;
	public static readonly GRANT = 82;
	public static readonly GRANTED = 83;
	public static readonly GRANTS = 84;
	public static readonly GRAPHVIZ = 85;
	public static readonly GROUP = 86;
	public static readonly GROUPING = 87;
	public static readonly GROUPS = 88;
	public static readonly HAVING = 89;
	public static readonly HOUR = 90;
	public static readonly IF = 91;
	public static readonly IGNORE = 92;
	public static readonly IN = 93;
	public static readonly INCLUDING = 94;
	public static readonly INNER = 95;
	public static readonly INPUT = 96;
	public static readonly INSERT = 97;
	public static readonly INTERSECT = 98;
	public static readonly INTERVAL = 99;
	public static readonly INTO = 100;
	public static readonly INVOKER = 101;
	public static readonly IO = 102;
	public static readonly IS = 103;
	public static readonly ISOLATION = 104;
	public static readonly JSON = 105;
	public static readonly JOIN = 106;
	public static readonly KEY = 107;
	public static readonly LANGUAGE = 108;
	public static readonly LAST = 109;
	public static readonly LATERAL = 110;
	public static readonly LEFT = 111;
	public static readonly LEVEL = 112;
	public static readonly LIKE = 113;
	public static readonly LIMIT = 114;
	public static readonly LOCALTIME = 115;
	public static readonly LOCALTIMESTAMP = 116;
	public static readonly LOGICAL = 117;
	public static readonly MAP = 118;
	public static readonly MATERIALIZED = 119;
	public static readonly MINUTE = 120;
	public static readonly MONTH = 121;
	public static readonly NAME = 122;
	public static readonly NATURAL = 123;
	public static readonly NFC = 124;
	public static readonly NFD = 125;
	public static readonly NFKC = 126;
	public static readonly NFKD = 127;
	public static readonly NO = 128;
	public static readonly NONE = 129;
	public static readonly NORMALIZE = 130;
	public static readonly NOT = 131;
	public static readonly NULL = 132;
	public static readonly NULLIF = 133;
	public static readonly NULLS = 134;
	public static readonly OF = 135;
	public static readonly OFFSET = 136;
	public static readonly ON = 137;
	public static readonly ONLY = 138;
	public static readonly OPTION = 139;
	public static readonly OR = 140;
	public static readonly ORDER = 141;
	public static readonly ORDINALITY = 142;
	public static readonly OUTER = 143;
	public static readonly OUTPUT = 144;
	public static readonly OVER = 145;
	public static readonly PARTITION = 146;
	public static readonly PARTITIONS = 147;
	public static readonly POSITION = 148;
	public static readonly PRECEDING = 149;
	public static readonly PREPARE = 150;
	public static readonly PRIMARY = 151;
	public static readonly PRIVILEGES = 152;
	public static readonly PROPERTIES = 153;
	public static readonly RANGE = 154;
	public static readonly READ = 155;
	public static readonly RECURSIVE = 156;
	public static readonly REFRESH = 157;
	public static readonly RELY = 158;
	public static readonly RENAME = 159;
	public static readonly REPEATABLE = 160;
	public static readonly REPLACE = 161;
	public static readonly RESET = 162;
	public static readonly RESPECT = 163;
	public static readonly RESTRICT = 164;
	public static readonly RETURN = 165;
	public static readonly RETURNS = 166;
	public static readonly REVOKE = 167;
	public static readonly RIGHT = 168;
	public static readonly ROLE = 169;
	public static readonly ROLES = 170;
	public static readonly ROLLBACK = 171;
	public static readonly ROLLUP = 172;
	public static readonly ROW = 173;
	public static readonly ROWS = 174;
	public static readonly SCHEMA = 175;
	public static readonly SCHEMAS = 176;
	public static readonly SECOND = 177;
	public static readonly SECURITY = 178;
	public static readonly SELECT = 179;
	public static readonly SERIALIZABLE = 180;
	public static readonly SESSION = 181;
	public static readonly SET = 182;
	public static readonly SETS = 183;
	public static readonly SHOW = 184;
	public static readonly SOME = 185;
	public static readonly SQL = 186;
	public static readonly START = 187;
	public static readonly STATS = 188;
	public static readonly SUBSTRING = 189;
	public static readonly SYSTEM = 190;
	public static readonly SYSTEM_TIME = 191;
	public static readonly SYSTEM_VERSION = 192;
	public static readonly TABLE = 193;
	public static readonly TABLES = 194;
	public static readonly TABLESAMPLE = 195;
	public static readonly TEMPORARY = 196;
	public static readonly TEXT = 197;
	public static readonly THEN = 198;
	public static readonly TIME = 199;
	public static readonly TIMESTAMP = 200;
	public static readonly TO = 201;
	public static readonly TRANSACTION = 202;
	public static readonly TRUE = 203;
	public static readonly TRUNCATE = 204;
	public static readonly TRY_CAST = 205;
	public static readonly TYPE = 206;
	public static readonly UESCAPE = 207;
	public static readonly UNBOUNDED = 208;
	public static readonly UNCOMMITTED = 209;
	public static readonly UNION = 210;
	public static readonly UNIQUE = 211;
	public static readonly UNNEST = 212;
	public static readonly UPDATE = 213;
	public static readonly USE = 214;
	public static readonly USER = 215;
	public static readonly USING = 216;
	public static readonly VALIDATE = 217;
	public static readonly VALUES = 218;
	public static readonly VERBOSE = 219;
	public static readonly VERSION = 220;
	public static readonly VIEW = 221;
	public static readonly WHEN = 222;
	public static readonly WHERE = 223;
	public static readonly WITH = 224;
	public static readonly WORK = 225;
	public static readonly WRITE = 226;
	public static readonly YEAR = 227;
	public static readonly ZONE = 228;
	public static readonly EQ = 229;
	public static readonly NEQ = 230;
	public static readonly LT = 231;
	public static readonly LTE = 232;
	public static readonly GT = 233;
	public static readonly GTE = 234;
	public static readonly PLUS = 235;
	public static readonly MINUS = 236;
	public static readonly ASTERISK = 237;
	public static readonly SLASH = 238;
	public static readonly PERCENT = 239;
	public static readonly CONCAT = 240;
	public static readonly STRING = 241;
	public static readonly UNICODE_STRING = 242;
	public static readonly BINARY_LITERAL = 243;
	public static readonly INTEGER_VALUE = 244;
	public static readonly DECIMAL_VALUE = 245;
	public static readonly DOUBLE_VALUE = 246;
	public static readonly IDENTIFIER = 247;
	public static readonly DIGIT_IDENTIFIER = 248;
	public static readonly QUOTED_IDENTIFIER = 249;
	public static readonly BACKQUOTED_IDENTIFIER = 250;
	public static readonly TIME_WITH_TIME_ZONE = 251;
	public static readonly TIMESTAMP_WITH_TIME_ZONE = 252;
	public static readonly DOUBLE_PRECISION = 253;
	public static readonly SIMPLE_COMMENT = 254;
	public static readonly BRACKETED_COMMENT = 255;
	public static readonly WS = 256;
	public static readonly UNRECOGNIZED = 257;
	public static readonly DELIMITER = 258;
	public static override readonly EOF = Token.EOF;
	public static readonly RULE_singleStatement = 0;
	public static readonly RULE_standaloneExpression = 1;
	public static readonly RULE_standaloneRoutineBody = 2;
	public static readonly RULE_standaloneSelectItemList = 3;
	public static readonly RULE_standaloneRelationList = 4;
	public static readonly RULE_standaloneBooleanExpression = 5;
	public static readonly RULE_standaloneSortItemList = 6;
	public static readonly RULE_standaloneIntegerValue = 7;
	public static readonly RULE_statement = 8;
	public static readonly RULE_query = 9;
	public static readonly RULE_with = 10;
	public static readonly RULE_tableElement = 11;
	public static readonly RULE_columnDefinition = 12;
	public static readonly RULE_likeClause = 13;
	public static readonly RULE_properties = 14;
	public static readonly RULE_property = 15;
	public static readonly RULE_sqlParameterDeclaration = 16;
	public static readonly RULE_routineCharacteristics = 17;
	public static readonly RULE_routineCharacteristic = 18;
	public static readonly RULE_alterRoutineCharacteristics = 19;
	public static readonly RULE_alterRoutineCharacteristic = 20;
	public static readonly RULE_routineBody = 21;
	public static readonly RULE_returnStatement = 22;
	public static readonly RULE_externalBodyReference = 23;
	public static readonly RULE_language = 24;
	public static readonly RULE_determinism = 25;
	public static readonly RULE_nullCallClause = 26;
	public static readonly RULE_externalRoutineName = 27;
	public static readonly RULE_queryNoWith = 28;
	public static readonly RULE_sortItemList = 29;
	public static readonly RULE_queryTerm = 30;
	public static readonly RULE_queryPrimary = 31;
	public static readonly RULE_sortItem = 32;
	public static readonly RULE_querySpecification = 33;
	public static readonly RULE_selectItemList = 34;
	public static readonly RULE_relationList = 35;
	public static readonly RULE_groupBy = 36;
	public static readonly RULE_groupingElement = 37;
	public static readonly RULE_groupingSet = 38;
	public static readonly RULE_namedQuery = 39;
	public static readonly RULE_setQuantifier = 40;
	public static readonly RULE_selectItem = 41;
	public static readonly RULE_relation = 42;
	public static readonly RULE_joinType = 43;
	public static readonly RULE_joinCriteria = 44;
	public static readonly RULE_sampledRelation = 45;
	public static readonly RULE_sampleType = 46;
	public static readonly RULE_aliasedRelation = 47;
	public static readonly RULE_columnAliases = 48;
	public static readonly RULE_relationPrimary = 49;
	public static readonly RULE_expression = 50;
	public static readonly RULE_booleanExpression = 51;
	public static readonly RULE_predicate = 52;
	public static readonly RULE_valueExpression = 53;
	public static readonly RULE_primaryExpression = 54;
	public static readonly RULE_string = 55;
	public static readonly RULE_nullTreatment = 56;
	public static readonly RULE_timeZoneSpecifier = 57;
	public static readonly RULE_comparisonOperator = 58;
	public static readonly RULE_comparisonQuantifier = 59;
	public static readonly RULE_booleanValue = 60;
	public static readonly RULE_interval = 61;
	public static readonly RULE_intervalField = 62;
	public static readonly RULE_normalForm = 63;
	public static readonly RULE_types = 64;
	public static readonly RULE_type = 65;
	public static readonly RULE_typeParameter = 66;
	public static readonly RULE_baseType = 67;
	public static readonly RULE_whenClause = 68;
	public static readonly RULE_filter = 69;
	public static readonly RULE_over = 70;
	public static readonly RULE_windowFrame = 71;
	public static readonly RULE_frameBound = 72;
	public static readonly RULE_updateAssignment = 73;
	public static readonly RULE_explainOption = 74;
	public static readonly RULE_transactionMode = 75;
	public static readonly RULE_levelOfIsolation = 76;
	public static readonly RULE_callArgument = 77;
	public static readonly RULE_privilege = 78;
	public static readonly RULE_qualifiedName = 79;
	public static readonly RULE_tableVersionExpression = 80;
	public static readonly RULE_tableVersionState = 81;
	public static readonly RULE_grantor = 82;
	public static readonly RULE_principal = 83;
	public static readonly RULE_roles = 84;
	public static readonly RULE_identifier = 85;
	public static readonly RULE_number = 86;
	public static readonly RULE_constraintSpecification = 87;
	public static readonly RULE_namedConstraintSpecification = 88;
	public static readonly RULE_unnamedConstraintSpecification = 89;
	public static readonly RULE_constraintType = 90;
	public static readonly RULE_constraintQualifiers = 91;
	public static readonly RULE_constraintQualifier = 92;
	public static readonly RULE_constraintRely = 93;
	public static readonly RULE_constraintEnabled = 94;
	public static readonly RULE_constraintEnforced = 95;
	public static readonly RULE_nonReserved = 96;
	public static readonly literalNames: (string | null)[] = [ null, "'.'", 
                                                            "'('", "')'", 
                                                            "','", "'?'", 
                                                            "'->'", "'['", 
                                                            "']'", "'=>'", 
                                                            "'ADD'", "'ADMIN'", 
                                                            "'ALL'", "'ALTER'", 
                                                            "'ANALYZE'", 
                                                            "'AND'", "'ANY'", 
                                                            "'ARRAY'", "'AS'", 
                                                            "'ASC'", "'AT'", 
                                                            "'BEFORE'", 
                                                            "'BERNOULLI'", 
                                                            "'BETWEEN'", 
                                                            "'BY'", "'CALL'", 
                                                            "'CALLED'", 
                                                            "'CASCADE'", 
                                                            "'CASE'", "'CAST'", 
                                                            "'CATALOGS'", 
                                                            "'COLUMN'", 
                                                            "'COLUMNS'", 
                                                            "'COMMENT'", 
                                                            "'COMMIT'", 
                                                            "'COMMITTED'", 
                                                            "'CONSTRAINT'", 
                                                            "'CREATE'", 
                                                            "'CROSS'", "'CUBE'", 
                                                            "'CURRENT'", 
                                                            "'CURRENT_DATE'", 
                                                            "'CURRENT_ROLE'", 
                                                            "'CURRENT_TIME'", 
                                                            "'CURRENT_TIMESTAMP'", 
                                                            "'CURRENT_USER'", 
                                                            "'DATA'", "'DATE'", 
                                                            "'DAY'", "'DEALLOCATE'", 
                                                            "'DEFINER'", 
                                                            "'DELETE'", 
                                                            "'DESC'", "'DESCRIBE'", 
                                                            "'DETERMINISTIC'", 
                                                            "'DISABLED'", 
                                                            "'DISTINCT'", 
                                                            "'DISTRIBUTED'", 
                                                            "'DROP'", "'ELSE'", 
                                                            "'ENABLED'", 
                                                            "'END'", "'ENFORCED'", 
                                                            "'ESCAPE'", 
                                                            "'EXCEPT'", 
                                                            "'EXCLUDING'", 
                                                            "'EXECUTE'", 
                                                            "'EXISTS'", 
                                                            "'EXPLAIN'", 
                                                            "'EXTRACT'", 
                                                            "'EXTERNAL'", 
                                                            "'FALSE'", "'FETCH'", 
                                                            "'FILTER'", 
                                                            "'FIRST'", "'FOLLOWING'", 
                                                            "'FOR'", "'FORMAT'", 
                                                            "'FROM'", "'FULL'", 
                                                            "'FUNCTION'", 
                                                            "'FUNCTIONS'", 
                                                            "'GRANT'", "'GRANTED'", 
                                                            "'GRANTS'", 
                                                            "'GRAPHVIZ'", 
                                                            "'GROUP'", "'GROUPING'", 
                                                            "'GROUPS'", 
                                                            "'HAVING'", 
                                                            "'HOUR'", "'IF'", 
                                                            "'IGNORE'", 
                                                            "'IN'", "'INCLUDING'", 
                                                            "'INNER'", "'INPUT'", 
                                                            "'INSERT'", 
                                                            "'INTERSECT'", 
                                                            "'INTERVAL'", 
                                                            "'INTO'", "'INVOKER'", 
                                                            "'IO'", "'IS'", 
                                                            "'ISOLATION'", 
                                                            "'JSON'", "'JOIN'", 
                                                            "'KEY'", "'LANGUAGE'", 
                                                            "'LAST'", "'LATERAL'", 
                                                            "'LEFT'", "'LEVEL'", 
                                                            "'LIKE'", "'LIMIT'", 
                                                            "'LOCALTIME'", 
                                                            "'LOCALTIMESTAMP'", 
                                                            "'LOGICAL'", 
                                                            "'MAP'", "'MATERIALIZED'", 
                                                            "'MINUTE'", 
                                                            "'MONTH'", "'NAME'", 
                                                            "'NATURAL'", 
                                                            "'NFC'", "'NFD'", 
                                                            "'NFKC'", "'NFKD'", 
                                                            "'NO'", "'NONE'", 
                                                            "'NORMALIZE'", 
                                                            "'NOT'", "'NULL'", 
                                                            "'NULLIF'", 
                                                            "'NULLS'", "'OF'", 
                                                            "'OFFSET'", 
                                                            "'ON'", "'ONLY'", 
                                                            "'OPTION'", 
                                                            "'OR'", "'ORDER'", 
                                                            "'ORDINALITY'", 
                                                            "'OUTER'", "'OUTPUT'", 
                                                            "'OVER'", "'PARTITION'", 
                                                            "'PARTITIONS'", 
                                                            "'POSITION'", 
                                                            "'PRECEDING'", 
                                                            "'PREPARE'", 
                                                            "'PRIMARY'", 
                                                            "'PRIVILEGES'", 
                                                            "'PROPERTIES'", 
                                                            "'RANGE'", "'READ'", 
                                                            "'RECURSIVE'", 
                                                            "'REFRESH'", 
                                                            "'RELY'", "'RENAME'", 
                                                            "'REPEATABLE'", 
                                                            "'REPLACE'", 
                                                            "'RESET'", "'RESPECT'", 
                                                            "'RESTRICT'", 
                                                            "'RETURN'", 
                                                            "'RETURNS'", 
                                                            "'REVOKE'", 
                                                            "'RIGHT'", "'ROLE'", 
                                                            "'ROLES'", "'ROLLBACK'", 
                                                            "'ROLLUP'", 
                                                            "'ROW'", "'ROWS'", 
                                                            "'SCHEMA'", 
                                                            "'SCHEMAS'", 
                                                            "'SECOND'", 
                                                            "'SECURITY'", 
                                                            "'SELECT'", 
                                                            "'SERIALIZABLE'", 
                                                            "'SESSION'", 
                                                            "'SET'", "'SETS'", 
                                                            "'SHOW'", "'SOME'", 
                                                            "'SQL'", "'START'", 
                                                            "'STATS'", "'SUBSTRING'", 
                                                            "'SYSTEM'", 
                                                            "'SYSTEM_TIME'", 
                                                            "'SYSTEM_VERSION'", 
                                                            "'TABLE'", "'TABLES'", 
                                                            "'TABLESAMPLE'", 
                                                            "'TEMPORARY'", 
                                                            "'TEXT'", "'THEN'", 
                                                            "'TIME'", "'TIMESTAMP'", 
                                                            "'TO'", "'TRANSACTION'", 
                                                            "'TRUE'", "'TRUNCATE'", 
                                                            "'TRY_CAST'", 
                                                            "'TYPE'", "'UESCAPE'", 
                                                            "'UNBOUNDED'", 
                                                            "'UNCOMMITTED'", 
                                                            "'UNION'", "'UNIQUE'", 
                                                            "'UNNEST'", 
                                                            "'UPDATE'", 
                                                            "'USE'", "'USER'", 
                                                            "'USING'", "'VALIDATE'", 
                                                            "'VALUES'", 
                                                            "'VERBOSE'", 
                                                            "'VERSION'", 
                                                            "'VIEW'", "'WHEN'", 
                                                            "'WHERE'", "'WITH'", 
                                                            "'WORK'", "'WRITE'", 
                                                            "'YEAR'", "'ZONE'", 
                                                            "'='", null, 
                                                            "'<'", "'<='", 
                                                            "'>'", "'>='", 
                                                            "'+'", "'-'", 
                                                            "'*'", "'/'", 
                                                            "'%'", "'||'" ];
	public static readonly symbolicNames: (string | null)[] = [ null, null, 
                                                             null, null, 
                                                             null, null, 
                                                             null, null, 
                                                             null, null, 
                                                             "ADD", "ADMIN", 
                                                             "ALL", "ALTER", 
                                                             "ANALYZE", 
                                                             "AND", "ANY", 
                                                             "ARRAY", "AS", 
                                                             "ASC", "AT", 
                                                             "BEFORE", "BERNOULLI", 
                                                             "BETWEEN", 
                                                             "BY", "CALL", 
                                                             "CALLED", "CASCADE", 
                                                             "CASE", "CAST", 
                                                             "CATALOGS", 
                                                             "COLUMN", "COLUMNS", 
                                                             "COMMENT", 
                                                             "COMMIT", "COMMITTED", 
                                                             "CONSTRAINT", 
                                                             "CREATE", "CROSS", 
                                                             "CUBE", "CURRENT", 
                                                             "CURRENT_DATE", 
                                                             "CURRENT_ROLE", 
                                                             "CURRENT_TIME", 
                                                             "CURRENT_TIMESTAMP", 
                                                             "CURRENT_USER", 
                                                             "DATA", "DATE", 
                                                             "DAY", "DEALLOCATE", 
                                                             "DEFINER", 
                                                             "DELETE", "DESC", 
                                                             "DESCRIBE", 
                                                             "DETERMINISTIC", 
                                                             "DISABLED", 
                                                             "DISTINCT", 
                                                             "DISTRIBUTED", 
                                                             "DROP", "ELSE", 
                                                             "ENABLED", 
                                                             "END", "ENFORCED", 
                                                             "ESCAPE", "EXCEPT", 
                                                             "EXCLUDING", 
                                                             "EXECUTE", 
                                                             "EXISTS", "EXPLAIN", 
                                                             "EXTRACT", 
                                                             "EXTERNAL", 
                                                             "FALSE", "FETCH", 
                                                             "FILTER", "FIRST", 
                                                             "FOLLOWING", 
                                                             "FOR", "FORMAT", 
                                                             "FROM", "FULL", 
                                                             "FUNCTION", 
                                                             "FUNCTIONS", 
                                                             "GRANT", "GRANTED", 
                                                             "GRANTS", "GRAPHVIZ", 
                                                             "GROUP", "GROUPING", 
                                                             "GROUPS", "HAVING", 
                                                             "HOUR", "IF", 
                                                             "IGNORE", "IN", 
                                                             "INCLUDING", 
                                                             "INNER", "INPUT", 
                                                             "INSERT", "INTERSECT", 
                                                             "INTERVAL", 
                                                             "INTO", "INVOKER", 
                                                             "IO", "IS", 
                                                             "ISOLATION", 
                                                             "JSON", "JOIN", 
                                                             "KEY", "LANGUAGE", 
                                                             "LAST", "LATERAL", 
                                                             "LEFT", "LEVEL", 
                                                             "LIKE", "LIMIT", 
                                                             "LOCALTIME", 
                                                             "LOCALTIMESTAMP", 
                                                             "LOGICAL", 
                                                             "MAP", "MATERIALIZED", 
                                                             "MINUTE", "MONTH", 
                                                             "NAME", "NATURAL", 
                                                             "NFC", "NFD", 
                                                             "NFKC", "NFKD", 
                                                             "NO", "NONE", 
                                                             "NORMALIZE", 
                                                             "NOT", "NULL", 
                                                             "NULLIF", "NULLS", 
                                                             "OF", "OFFSET", 
                                                             "ON", "ONLY", 
                                                             "OPTION", "OR", 
                                                             "ORDER", "ORDINALITY", 
                                                             "OUTER", "OUTPUT", 
                                                             "OVER", "PARTITION", 
                                                             "PARTITIONS", 
                                                             "POSITION", 
                                                             "PRECEDING", 
                                                             "PREPARE", 
                                                             "PRIMARY", 
                                                             "PRIVILEGES", 
                                                             "PROPERTIES", 
                                                             "RANGE", "READ", 
                                                             "RECURSIVE", 
                                                             "REFRESH", 
                                                             "RELY", "RENAME", 
                                                             "REPEATABLE", 
                                                             "REPLACE", 
                                                             "RESET", "RESPECT", 
                                                             "RESTRICT", 
                                                             "RETURN", "RETURNS", 
                                                             "REVOKE", "RIGHT", 
                                                             "ROLE", "ROLES", 
                                                             "ROLLBACK", 
                                                             "ROLLUP", "ROW", 
                                                             "ROWS", "SCHEMA", 
                                                             "SCHEMAS", 
                                                             "SECOND", "SECURITY", 
                                                             "SELECT", "SERIALIZABLE", 
                                                             "SESSION", 
                                                             "SET", "SETS", 
                                                             "SHOW", "SOME", 
                                                             "SQL", "START", 
                                                             "STATS", "SUBSTRING", 
                                                             "SYSTEM", "SYSTEM_TIME", 
                                                             "SYSTEM_VERSION", 
                                                             "TABLE", "TABLES", 
                                                             "TABLESAMPLE", 
                                                             "TEMPORARY", 
                                                             "TEXT", "THEN", 
                                                             "TIME", "TIMESTAMP", 
                                                             "TO", "TRANSACTION", 
                                                             "TRUE", "TRUNCATE", 
                                                             "TRY_CAST", 
                                                             "TYPE", "UESCAPE", 
                                                             "UNBOUNDED", 
                                                             "UNCOMMITTED", 
                                                             "UNION", "UNIQUE", 
                                                             "UNNEST", "UPDATE", 
                                                             "USE", "USER", 
                                                             "USING", "VALIDATE", 
                                                             "VALUES", "VERBOSE", 
                                                             "VERSION", 
                                                             "VIEW", "WHEN", 
                                                             "WHERE", "WITH", 
                                                             "WORK", "WRITE", 
                                                             "YEAR", "ZONE", 
                                                             "EQ", "NEQ", 
                                                             "LT", "LTE", 
                                                             "GT", "GTE", 
                                                             "PLUS", "MINUS", 
                                                             "ASTERISK", 
                                                             "SLASH", "PERCENT", 
                                                             "CONCAT", "STRING", 
                                                             "UNICODE_STRING", 
                                                             "BINARY_LITERAL", 
                                                             "INTEGER_VALUE", 
                                                             "DECIMAL_VALUE", 
                                                             "DOUBLE_VALUE", 
                                                             "IDENTIFIER", 
                                                             "DIGIT_IDENTIFIER", 
                                                             "QUOTED_IDENTIFIER", 
                                                             "BACKQUOTED_IDENTIFIER", 
                                                             "TIME_WITH_TIME_ZONE", 
                                                             "TIMESTAMP_WITH_TIME_ZONE", 
                                                             "DOUBLE_PRECISION", 
                                                             "SIMPLE_COMMENT", 
                                                             "BRACKETED_COMMENT", 
                                                             "WS", "UNRECOGNIZED", 
                                                             "DELIMITER" ];
	// tslint:disable:no-trailing-whitespace
	public static readonly ruleNames: string[] = [
		"singleStatement", "standaloneExpression", "standaloneRoutineBody", "standaloneSelectItemList", 
		"standaloneRelationList", "standaloneBooleanExpression", "standaloneSortItemList", 
		"standaloneIntegerValue", "statement", "query", "with", "tableElement", 
		"columnDefinition", "likeClause", "properties", "property", "sqlParameterDeclaration", 
		"routineCharacteristics", "routineCharacteristic", "alterRoutineCharacteristics", 
		"alterRoutineCharacteristic", "routineBody", "returnStatement", "externalBodyReference", 
		"language", "determinism", "nullCallClause", "externalRoutineName", "queryNoWith", 
		"sortItemList", "queryTerm", "queryPrimary", "sortItem", "querySpecification", 
		"selectItemList", "relationList", "groupBy", "groupingElement", "groupingSet", 
		"namedQuery", "setQuantifier", "selectItem", "relation", "joinType", "joinCriteria", 
		"sampledRelation", "sampleType", "aliasedRelation", "columnAliases", "relationPrimary", 
		"expression", "booleanExpression", "predicate", "valueExpression", "primaryExpression", 
		"string", "nullTreatment", "timeZoneSpecifier", "comparisonOperator", 
		"comparisonQuantifier", "booleanValue", "interval", "intervalField", "normalForm", 
		"types", "type", "typeParameter", "baseType", "whenClause", "filter", 
		"over", "windowFrame", "frameBound", "updateAssignment", "explainOption", 
		"transactionMode", "levelOfIsolation", "callArgument", "privilege", "qualifiedName", 
		"tableVersionExpression", "tableVersionState", "grantor", "principal", 
		"roles", "identifier", "number", "constraintSpecification", "namedConstraintSpecification", 
		"unnamedConstraintSpecification", "constraintType", "constraintQualifiers", 
		"constraintQualifier", "constraintRely", "constraintEnabled", "constraintEnforced", 
		"nonReserved",
	];
	public get grammarFileName(): string { return "SqlBase.g4"; }
	public get literalNames(): (string | null)[] { return SqlBaseParser.literalNames; }
	public get symbolicNames(): (string | null)[] { return SqlBaseParser.symbolicNames; }
	public get ruleNames(): string[] { return SqlBaseParser.ruleNames; }
	public get serializedATN(): number[] { return SqlBaseParser._serializedATN; }

	protected createFailedPredicateException(predicate?: string, message?: string): FailedPredicateException {
		return new FailedPredicateException(this, predicate, message);
	}

	constructor(input: TokenStream) {
		super(input);
		this._interp = new ParserATNSimulator(this, SqlBaseParser._ATN, SqlBaseParser.DecisionsToDFA, new PredictionContextCache());
	}
	// @RuleVersion(0)
	public singleStatement(): SingleStatementContext {
		let localctx: SingleStatementContext = new SingleStatementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 0, SqlBaseParser.RULE_singleStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 194;
			this.statement();
			this.state = 195;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneExpression(): StandaloneExpressionContext {
		let localctx: StandaloneExpressionContext = new StandaloneExpressionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 2, SqlBaseParser.RULE_standaloneExpression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 197;
			this.expression();
			this.state = 198;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneRoutineBody(): StandaloneRoutineBodyContext {
		let localctx: StandaloneRoutineBodyContext = new StandaloneRoutineBodyContext(this, this._ctx, this.state);
		this.enterRule(localctx, 4, SqlBaseParser.RULE_standaloneRoutineBody);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 200;
			this.routineBody();
			this.state = 201;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneSelectItemList(): StandaloneSelectItemListContext {
		let localctx: StandaloneSelectItemListContext = new StandaloneSelectItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 6, SqlBaseParser.RULE_standaloneSelectItemList);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 203;
			this.selectItemList();
			this.state = 204;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneRelationList(): StandaloneRelationListContext {
		let localctx: StandaloneRelationListContext = new StandaloneRelationListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 8, SqlBaseParser.RULE_standaloneRelationList);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 206;
			this.relationList();
			this.state = 207;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneBooleanExpression(): StandaloneBooleanExpressionContext {
		let localctx: StandaloneBooleanExpressionContext = new StandaloneBooleanExpressionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 10, SqlBaseParser.RULE_standaloneBooleanExpression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 209;
			this.booleanExpression(0);
			this.state = 210;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneSortItemList(): StandaloneSortItemListContext {
		let localctx: StandaloneSortItemListContext = new StandaloneSortItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 12, SqlBaseParser.RULE_standaloneSortItemList);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 212;
			this.sortItemList();
			this.state = 213;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public standaloneIntegerValue(): StandaloneIntegerValueContext {
		let localctx: StandaloneIntegerValueContext = new StandaloneIntegerValueContext(this, this._ctx, this.state);
		this.enterRule(localctx, 14, SqlBaseParser.RULE_standaloneIntegerValue);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 215;
			this.match(SqlBaseParser.INTEGER_VALUE);
			this.state = 216;
			this.match(SqlBaseParser.EOF);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public statement(): StatementContext {
		let localctx: StatementContext = new StatementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 16, SqlBaseParser.RULE_statement);
		let _la: number;
		try {
			this.state = 959;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 102, this._ctx) ) {
			case 1:
				localctx = new StatementDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 218;
				this.query();
				}
				break;
			case 2:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 219;
				this.match(SqlBaseParser.USE);
				this.state = 220;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 3:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 221;
				this.match(SqlBaseParser.USE);
				this.state = 222;
				(localctx as UseContext)._catalog = this.identifier();
				this.state = 223;
				this.match(SqlBaseParser.T__0);
				this.state = 224;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 4:
				localctx = new CreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 226;
				this.match(SqlBaseParser.CREATE);
				this.state = 227;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 231;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 0, this._ctx) ) {
				case 1:
					{
					this.state = 228;
					this.match(SqlBaseParser.IF);
					this.state = 229;
					this.match(SqlBaseParser.NOT);
					this.state = 230;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 233;
				this.qualifiedName();
				this.state = 236;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 234;
					this.match(SqlBaseParser.WITH);
					this.state = 235;
					this.properties();
					}
				}

				}
				break;
			case 5:
				localctx = new DropSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 238;
				this.match(SqlBaseParser.DROP);
				this.state = 239;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 242;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 2, this._ctx) ) {
				case 1:
					{
					this.state = 240;
					this.match(SqlBaseParser.IF);
					this.state = 241;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 244;
				this.qualifiedName();
				this.state = 246;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===27 || _la===164) {
					{
					this.state = 245;
					_la = this._input.LA(1);
					if(!(_la===27 || _la===164)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					}
				}

				}
				break;
			case 6:
				localctx = new RenameSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 248;
				this.match(SqlBaseParser.ALTER);
				this.state = 249;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 250;
				this.qualifiedName();
				this.state = 251;
				this.match(SqlBaseParser.RENAME);
				this.state = 252;
				this.match(SqlBaseParser.TO);
				this.state = 253;
				this.identifier();
				}
				break;
			case 7:
				localctx = new CreateTableAsSelectContext(this, localctx);
				this.enterOuterAlt(localctx, 7);
				{
				this.state = 255;
				this.match(SqlBaseParser.CREATE);
				this.state = 256;
				this.match(SqlBaseParser.TABLE);
				this.state = 260;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 4, this._ctx) ) {
				case 1:
					{
					this.state = 257;
					this.match(SqlBaseParser.IF);
					this.state = 258;
					this.match(SqlBaseParser.NOT);
					this.state = 259;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 262;
				this.qualifiedName();
				this.state = 264;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 263;
					this.columnAliases();
					}
				}

				this.state = 268;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 266;
					this.match(SqlBaseParser.COMMENT);
					this.state = 267;
					this.string_();
					}
				}

				this.state = 272;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 270;
					this.match(SqlBaseParser.WITH);
					this.state = 271;
					this.properties();
					}
				}

				this.state = 274;
				this.match(SqlBaseParser.AS);
				this.state = 280;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 8, this._ctx) ) {
				case 1:
					{
					this.state = 275;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 276;
					this.match(SqlBaseParser.T__1);
					this.state = 277;
					this.query();
					this.state = 278;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 287;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 282;
					this.match(SqlBaseParser.WITH);
					this.state = 284;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===128) {
						{
						this.state = 283;
						this.match(SqlBaseParser.NO);
						}
					}

					this.state = 286;
					this.match(SqlBaseParser.DATA);
					}
				}

				}
				break;
			case 8:
				localctx = new CreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 289;
				this.match(SqlBaseParser.CREATE);
				this.state = 290;
				this.match(SqlBaseParser.TABLE);
				this.state = 294;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 11, this._ctx) ) {
				case 1:
					{
					this.state = 291;
					this.match(SqlBaseParser.IF);
					this.state = 292;
					this.match(SqlBaseParser.NOT);
					this.state = 293;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 296;
				this.qualifiedName();
				this.state = 297;
				this.match(SqlBaseParser.T__1);
				this.state = 298;
				this.tableElement();
				this.state = 303;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 299;
					this.match(SqlBaseParser.T__3);
					this.state = 300;
					this.tableElement();
					}
					}
					this.state = 305;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 306;
				this.match(SqlBaseParser.T__2);
				this.state = 309;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 307;
					this.match(SqlBaseParser.COMMENT);
					this.state = 308;
					this.string_();
					}
				}

				this.state = 313;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 311;
					this.match(SqlBaseParser.WITH);
					this.state = 312;
					this.properties();
					}
				}

				}
				break;
			case 9:
				localctx = new DropTableContext(this, localctx);
				this.enterOuterAlt(localctx, 9);
				{
				this.state = 315;
				this.match(SqlBaseParser.DROP);
				this.state = 316;
				this.match(SqlBaseParser.TABLE);
				this.state = 319;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 15, this._ctx) ) {
				case 1:
					{
					this.state = 317;
					this.match(SqlBaseParser.IF);
					this.state = 318;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 321;
				this.qualifiedName();
				}
				break;
			case 10:
				localctx = new InsertIntoContext(this, localctx);
				this.enterOuterAlt(localctx, 10);
				{
				this.state = 322;
				this.match(SqlBaseParser.INSERT);
				this.state = 323;
				this.match(SqlBaseParser.INTO);
				this.state = 324;
				this.qualifiedName();
				this.state = 326;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 16, this._ctx) ) {
				case 1:
					{
					this.state = 325;
					this.columnAliases();
					}
					break;
				}
				this.state = 328;
				this.query();
				}
				break;
			case 11:
				localctx = new DeleteContext(this, localctx);
				this.enterOuterAlt(localctx, 11);
				{
				this.state = 330;
				this.match(SqlBaseParser.DELETE);
				this.state = 331;
				this.match(SqlBaseParser.FROM);
				this.state = 332;
				this.qualifiedName();
				this.state = 335;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 333;
					this.match(SqlBaseParser.WHERE);
					this.state = 334;
					this.booleanExpression(0);
					}
				}

				}
				break;
			case 12:
				localctx = new TruncateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 12);
				{
				this.state = 337;
				this.match(SqlBaseParser.TRUNCATE);
				this.state = 338;
				this.match(SqlBaseParser.TABLE);
				this.state = 339;
				this.qualifiedName();
				}
				break;
			case 13:
				localctx = new RenameTableContext(this, localctx);
				this.enterOuterAlt(localctx, 13);
				{
				this.state = 340;
				this.match(SqlBaseParser.ALTER);
				this.state = 341;
				this.match(SqlBaseParser.TABLE);
				this.state = 344;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 18, this._ctx) ) {
				case 1:
					{
					this.state = 342;
					this.match(SqlBaseParser.IF);
					this.state = 343;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 346;
				(localctx as RenameTableContext)._from_ = this.qualifiedName();
				this.state = 347;
				this.match(SqlBaseParser.RENAME);
				this.state = 348;
				this.match(SqlBaseParser.TO);
				this.state = 349;
				(localctx as RenameTableContext)._to = this.qualifiedName();
				}
				break;
			case 14:
				localctx = new RenameColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 14);
				{
				this.state = 351;
				this.match(SqlBaseParser.ALTER);
				this.state = 352;
				this.match(SqlBaseParser.TABLE);
				this.state = 355;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 19, this._ctx) ) {
				case 1:
					{
					this.state = 353;
					this.match(SqlBaseParser.IF);
					this.state = 354;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 357;
				(localctx as RenameColumnContext)._tableName = this.qualifiedName();
				this.state = 358;
				this.match(SqlBaseParser.RENAME);
				this.state = 359;
				this.match(SqlBaseParser.COLUMN);
				this.state = 362;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 20, this._ctx) ) {
				case 1:
					{
					this.state = 360;
					this.match(SqlBaseParser.IF);
					this.state = 361;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 364;
				(localctx as RenameColumnContext)._from_ = this.identifier();
				this.state = 365;
				this.match(SqlBaseParser.TO);
				this.state = 366;
				(localctx as RenameColumnContext)._to = this.identifier();
				}
				break;
			case 15:
				localctx = new DropColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 15);
				{
				this.state = 368;
				this.match(SqlBaseParser.ALTER);
				this.state = 369;
				this.match(SqlBaseParser.TABLE);
				this.state = 372;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 21, this._ctx) ) {
				case 1:
					{
					this.state = 370;
					this.match(SqlBaseParser.IF);
					this.state = 371;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 374;
				(localctx as DropColumnContext)._tableName = this.qualifiedName();
				this.state = 375;
				this.match(SqlBaseParser.DROP);
				this.state = 376;
				this.match(SqlBaseParser.COLUMN);
				this.state = 379;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 22, this._ctx) ) {
				case 1:
					{
					this.state = 377;
					this.match(SqlBaseParser.IF);
					this.state = 378;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 381;
				(localctx as DropColumnContext)._column = this.qualifiedName();
				}
				break;
			case 16:
				localctx = new AddColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 16);
				{
				this.state = 383;
				this.match(SqlBaseParser.ALTER);
				this.state = 384;
				this.match(SqlBaseParser.TABLE);
				this.state = 387;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 23, this._ctx) ) {
				case 1:
					{
					this.state = 385;
					this.match(SqlBaseParser.IF);
					this.state = 386;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 389;
				(localctx as AddColumnContext)._tableName = this.qualifiedName();
				this.state = 390;
				this.match(SqlBaseParser.ADD);
				this.state = 391;
				this.match(SqlBaseParser.COLUMN);
				this.state = 395;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 24, this._ctx) ) {
				case 1:
					{
					this.state = 392;
					this.match(SqlBaseParser.IF);
					this.state = 393;
					this.match(SqlBaseParser.NOT);
					this.state = 394;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 397;
				(localctx as AddColumnContext)._column = this.columnDefinition();
				}
				break;
			case 17:
				localctx = new AddConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 17);
				{
				this.state = 399;
				this.match(SqlBaseParser.ALTER);
				this.state = 400;
				this.match(SqlBaseParser.TABLE);
				this.state = 403;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 25, this._ctx) ) {
				case 1:
					{
					this.state = 401;
					this.match(SqlBaseParser.IF);
					this.state = 402;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 405;
				(localctx as AddConstraintContext)._tableName = this.qualifiedName();
				this.state = 406;
				this.match(SqlBaseParser.ADD);
				this.state = 407;
				this.constraintSpecification();
				}
				break;
			case 18:
				localctx = new DropConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 18);
				{
				this.state = 409;
				this.match(SqlBaseParser.ALTER);
				this.state = 410;
				this.match(SqlBaseParser.TABLE);
				this.state = 413;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 26, this._ctx) ) {
				case 1:
					{
					this.state = 411;
					this.match(SqlBaseParser.IF);
					this.state = 412;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 415;
				(localctx as DropConstraintContext)._tableName = this.qualifiedName();
				this.state = 416;
				this.match(SqlBaseParser.DROP);
				this.state = 417;
				this.match(SqlBaseParser.CONSTRAINT);
				this.state = 420;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 27, this._ctx) ) {
				case 1:
					{
					this.state = 418;
					this.match(SqlBaseParser.IF);
					this.state = 419;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 422;
				(localctx as DropConstraintContext)._name = this.identifier();
				}
				break;
			case 19:
				localctx = new AlterColumnSetNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 19);
				{
				this.state = 424;
				this.match(SqlBaseParser.ALTER);
				this.state = 425;
				this.match(SqlBaseParser.TABLE);
				this.state = 428;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 28, this._ctx) ) {
				case 1:
					{
					this.state = 426;
					this.match(SqlBaseParser.IF);
					this.state = 427;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 430;
				(localctx as AlterColumnSetNotNullContext)._tableName = this.qualifiedName();
				this.state = 431;
				this.match(SqlBaseParser.ALTER);
				this.state = 433;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 29, this._ctx) ) {
				case 1:
					{
					this.state = 432;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 435;
				(localctx as AlterColumnSetNotNullContext)._column = this.identifier();
				this.state = 436;
				this.match(SqlBaseParser.SET);
				this.state = 437;
				this.match(SqlBaseParser.NOT);
				this.state = 438;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 20:
				localctx = new AlterColumnDropNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 20);
				{
				this.state = 440;
				this.match(SqlBaseParser.ALTER);
				this.state = 441;
				this.match(SqlBaseParser.TABLE);
				this.state = 444;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 30, this._ctx) ) {
				case 1:
					{
					this.state = 442;
					this.match(SqlBaseParser.IF);
					this.state = 443;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 446;
				(localctx as AlterColumnDropNotNullContext)._tableName = this.qualifiedName();
				this.state = 447;
				this.match(SqlBaseParser.ALTER);
				this.state = 449;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 31, this._ctx) ) {
				case 1:
					{
					this.state = 448;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 451;
				(localctx as AlterColumnDropNotNullContext)._column = this.identifier();
				this.state = 452;
				this.match(SqlBaseParser.DROP);
				this.state = 453;
				this.match(SqlBaseParser.NOT);
				this.state = 454;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 21:
				localctx = new SetTablePropertiesContext(this, localctx);
				this.enterOuterAlt(localctx, 21);
				{
				this.state = 456;
				this.match(SqlBaseParser.ALTER);
				this.state = 457;
				this.match(SqlBaseParser.TABLE);
				this.state = 460;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 32, this._ctx) ) {
				case 1:
					{
					this.state = 458;
					this.match(SqlBaseParser.IF);
					this.state = 459;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 462;
				(localctx as SetTablePropertiesContext)._tableName = this.qualifiedName();
				this.state = 463;
				this.match(SqlBaseParser.SET);
				this.state = 464;
				this.match(SqlBaseParser.PROPERTIES);
				this.state = 465;
				this.properties();
				}
				break;
			case 22:
				localctx = new AnalyzeContext(this, localctx);
				this.enterOuterAlt(localctx, 22);
				{
				this.state = 467;
				this.match(SqlBaseParser.ANALYZE);
				this.state = 468;
				this.qualifiedName();
				this.state = 471;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 469;
					this.match(SqlBaseParser.WITH);
					this.state = 470;
					this.properties();
					}
				}

				}
				break;
			case 23:
				localctx = new CreateTypeContext(this, localctx);
				this.enterOuterAlt(localctx, 23);
				{
				this.state = 473;
				this.match(SqlBaseParser.CREATE);
				this.state = 474;
				this.match(SqlBaseParser.TYPE);
				this.state = 475;
				this.qualifiedName();
				this.state = 476;
				this.match(SqlBaseParser.AS);
				this.state = 489;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 2:
					{
					this.state = 477;
					this.match(SqlBaseParser.T__1);
					this.state = 478;
					this.sqlParameterDeclaration();
					this.state = 483;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 479;
						this.match(SqlBaseParser.T__3);
						this.state = 480;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 485;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 486;
					this.match(SqlBaseParser.T__2);
					}
					break;
				case 10:
				case 11:
				case 12:
				case 14:
				case 16:
				case 17:
				case 19:
				case 20:
				case 21:
				case 22:
				case 25:
				case 26:
				case 27:
				case 30:
				case 31:
				case 32:
				case 33:
				case 34:
				case 35:
				case 40:
				case 42:
				case 46:
				case 47:
				case 48:
				case 50:
				case 52:
				case 54:
				case 55:
				case 57:
				case 60:
				case 62:
				case 65:
				case 68:
				case 70:
				case 72:
				case 73:
				case 74:
				case 75:
				case 77:
				case 80:
				case 81:
				case 82:
				case 83:
				case 84:
				case 85:
				case 88:
				case 90:
				case 91:
				case 92:
				case 94:
				case 96:
				case 99:
				case 101:
				case 102:
				case 104:
				case 105:
				case 107:
				case 108:
				case 109:
				case 110:
				case 112:
				case 114:
				case 117:
				case 118:
				case 119:
				case 120:
				case 121:
				case 122:
				case 124:
				case 125:
				case 126:
				case 127:
				case 128:
				case 129:
				case 133:
				case 134:
				case 135:
				case 136:
				case 138:
				case 139:
				case 142:
				case 144:
				case 145:
				case 146:
				case 147:
				case 148:
				case 149:
				case 151:
				case 152:
				case 153:
				case 154:
				case 155:
				case 157:
				case 158:
				case 159:
				case 160:
				case 161:
				case 162:
				case 163:
				case 164:
				case 165:
				case 166:
				case 167:
				case 169:
				case 170:
				case 171:
				case 173:
				case 174:
				case 175:
				case 176:
				case 177:
				case 178:
				case 180:
				case 181:
				case 182:
				case 183:
				case 184:
				case 185:
				case 186:
				case 187:
				case 188:
				case 189:
				case 190:
				case 191:
				case 192:
				case 194:
				case 195:
				case 196:
				case 197:
				case 199:
				case 200:
				case 201:
				case 202:
				case 204:
				case 205:
				case 206:
				case 208:
				case 209:
				case 211:
				case 213:
				case 214:
				case 215:
				case 217:
				case 219:
				case 220:
				case 221:
				case 225:
				case 226:
				case 227:
				case 228:
				case 247:
				case 248:
				case 249:
				case 250:
				case 251:
				case 252:
				case 253:
					{
					this.state = 488;
					this.type_(0);
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				break;
			case 24:
				localctx = new CreateViewContext(this, localctx);
				this.enterOuterAlt(localctx, 24);
				{
				this.state = 491;
				this.match(SqlBaseParser.CREATE);
				this.state = 494;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 492;
					this.match(SqlBaseParser.OR);
					this.state = 493;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 496;
				this.match(SqlBaseParser.VIEW);
				this.state = 497;
				this.qualifiedName();
				this.state = 500;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===178) {
					{
					this.state = 498;
					this.match(SqlBaseParser.SECURITY);
					this.state = 499;
					_la = this._input.LA(1);
					if(!(_la===50 || _la===101)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					}
				}

				this.state = 502;
				this.match(SqlBaseParser.AS);
				this.state = 503;
				this.query();
				}
				break;
			case 25:
				localctx = new RenameViewContext(this, localctx);
				this.enterOuterAlt(localctx, 25);
				{
				this.state = 505;
				this.match(SqlBaseParser.ALTER);
				this.state = 506;
				this.match(SqlBaseParser.VIEW);
				this.state = 509;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 38, this._ctx) ) {
				case 1:
					{
					this.state = 507;
					this.match(SqlBaseParser.IF);
					this.state = 508;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 511;
				(localctx as RenameViewContext)._from_ = this.qualifiedName();
				this.state = 512;
				this.match(SqlBaseParser.RENAME);
				this.state = 513;
				this.match(SqlBaseParser.TO);
				this.state = 514;
				(localctx as RenameViewContext)._to = this.qualifiedName();
				}
				break;
			case 26:
				localctx = new DropViewContext(this, localctx);
				this.enterOuterAlt(localctx, 26);
				{
				this.state = 516;
				this.match(SqlBaseParser.DROP);
				this.state = 517;
				this.match(SqlBaseParser.VIEW);
				this.state = 520;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 39, this._ctx) ) {
				case 1:
					{
					this.state = 518;
					this.match(SqlBaseParser.IF);
					this.state = 519;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 522;
				this.qualifiedName();
				}
				break;
			case 27:
				localctx = new CreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 27);
				{
				this.state = 523;
				this.match(SqlBaseParser.CREATE);
				this.state = 524;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 525;
				this.match(SqlBaseParser.VIEW);
				this.state = 529;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 40, this._ctx) ) {
				case 1:
					{
					this.state = 526;
					this.match(SqlBaseParser.IF);
					this.state = 527;
					this.match(SqlBaseParser.NOT);
					this.state = 528;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 531;
				this.qualifiedName();
				this.state = 534;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 532;
					this.match(SqlBaseParser.COMMENT);
					this.state = 533;
					this.string_();
					}
				}

				this.state = 538;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 536;
					this.match(SqlBaseParser.WITH);
					this.state = 537;
					this.properties();
					}
				}

				this.state = 540;
				this.match(SqlBaseParser.AS);
				this.state = 546;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 43, this._ctx) ) {
				case 1:
					{
					this.state = 541;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 542;
					this.match(SqlBaseParser.T__1);
					this.state = 543;
					this.query();
					this.state = 544;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 28:
				localctx = new DropMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 28);
				{
				this.state = 548;
				this.match(SqlBaseParser.DROP);
				this.state = 549;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 550;
				this.match(SqlBaseParser.VIEW);
				this.state = 553;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 44, this._ctx) ) {
				case 1:
					{
					this.state = 551;
					this.match(SqlBaseParser.IF);
					this.state = 552;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 555;
				this.qualifiedName();
				}
				break;
			case 29:
				localctx = new RefreshMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 29);
				{
				this.state = 556;
				this.match(SqlBaseParser.REFRESH);
				this.state = 557;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 558;
				this.match(SqlBaseParser.VIEW);
				this.state = 559;
				this.qualifiedName();
				this.state = 560;
				this.match(SqlBaseParser.WHERE);
				this.state = 561;
				this.booleanExpression(0);
				}
				break;
			case 30:
				localctx = new CreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 30);
				{
				this.state = 563;
				this.match(SqlBaseParser.CREATE);
				this.state = 566;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 564;
					this.match(SqlBaseParser.OR);
					this.state = 565;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 569;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 568;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 571;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 572;
				(localctx as CreateFunctionContext)._functionName = this.qualifiedName();
				this.state = 573;
				this.match(SqlBaseParser.T__1);
				this.state = 582;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 574;
					this.sqlParameterDeclaration();
					this.state = 579;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 575;
						this.match(SqlBaseParser.T__3);
						this.state = 576;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 581;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 584;
				this.match(SqlBaseParser.T__2);
				this.state = 585;
				this.match(SqlBaseParser.RETURNS);
				this.state = 586;
				(localctx as CreateFunctionContext)._returnType = this.type_(0);
				this.state = 589;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 587;
					this.match(SqlBaseParser.COMMENT);
					this.state = 588;
					this.string_();
					}
				}

				this.state = 591;
				this.routineCharacteristics();
				this.state = 592;
				this.routineBody();
				}
				break;
			case 31:
				localctx = new AlterFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 31);
				{
				this.state = 594;
				this.match(SqlBaseParser.ALTER);
				this.state = 595;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 596;
				this.qualifiedName();
				this.state = 598;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 597;
					this.types();
					}
				}

				this.state = 600;
				this.alterRoutineCharacteristics();
				}
				break;
			case 32:
				localctx = new DropFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 32);
				{
				this.state = 602;
				this.match(SqlBaseParser.DROP);
				this.state = 604;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 603;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 606;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 609;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 52, this._ctx) ) {
				case 1:
					{
					this.state = 607;
					this.match(SqlBaseParser.IF);
					this.state = 608;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 611;
				this.qualifiedName();
				this.state = 613;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 612;
					this.types();
					}
				}

				}
				break;
			case 33:
				localctx = new CallContext(this, localctx);
				this.enterOuterAlt(localctx, 33);
				{
				this.state = 615;
				this.match(SqlBaseParser.CALL);
				this.state = 616;
				this.qualifiedName();
				this.state = 617;
				this.match(SqlBaseParser.T__1);
				this.state = 626;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 618;
					this.callArgument();
					this.state = 623;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 619;
						this.match(SqlBaseParser.T__3);
						this.state = 620;
						this.callArgument();
						}
						}
						this.state = 625;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 628;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				localctx = new CreateRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 34);
				{
				this.state = 630;
				this.match(SqlBaseParser.CREATE);
				this.state = 631;
				this.match(SqlBaseParser.ROLE);
				this.state = 632;
				(localctx as CreateRoleContext)._name = this.identifier();
				this.state = 636;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 633;
					this.match(SqlBaseParser.WITH);
					this.state = 634;
					this.match(SqlBaseParser.ADMIN);
					this.state = 635;
					this.grantor();
					}
				}

				}
				break;
			case 35:
				localctx = new DropRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 35);
				{
				this.state = 638;
				this.match(SqlBaseParser.DROP);
				this.state = 639;
				this.match(SqlBaseParser.ROLE);
				this.state = 640;
				(localctx as DropRoleContext)._name = this.identifier();
				}
				break;
			case 36:
				localctx = new GrantRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 36);
				{
				this.state = 641;
				this.match(SqlBaseParser.GRANT);
				this.state = 642;
				this.roles();
				this.state = 643;
				this.match(SqlBaseParser.TO);
				this.state = 644;
				this.principal();
				this.state = 649;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 645;
					this.match(SqlBaseParser.T__3);
					this.state = 646;
					this.principal();
					}
					}
					this.state = 651;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 655;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 652;
					this.match(SqlBaseParser.WITH);
					this.state = 653;
					this.match(SqlBaseParser.ADMIN);
					this.state = 654;
					this.match(SqlBaseParser.OPTION);
					}
				}

				this.state = 660;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 657;
					this.match(SqlBaseParser.GRANTED);
					this.state = 658;
					this.match(SqlBaseParser.BY);
					this.state = 659;
					this.grantor();
					}
				}

				}
				break;
			case 37:
				localctx = new RevokeRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 37);
				{
				this.state = 662;
				this.match(SqlBaseParser.REVOKE);
				this.state = 666;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 60, this._ctx) ) {
				case 1:
					{
					this.state = 663;
					this.match(SqlBaseParser.ADMIN);
					this.state = 664;
					this.match(SqlBaseParser.OPTION);
					this.state = 665;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 668;
				this.roles();
				this.state = 669;
				this.match(SqlBaseParser.FROM);
				this.state = 670;
				this.principal();
				this.state = 675;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 671;
					this.match(SqlBaseParser.T__3);
					this.state = 672;
					this.principal();
					}
					}
					this.state = 677;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 681;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 678;
					this.match(SqlBaseParser.GRANTED);
					this.state = 679;
					this.match(SqlBaseParser.BY);
					this.state = 680;
					this.grantor();
					}
				}

				}
				break;
			case 38:
				localctx = new SetRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 38);
				{
				this.state = 683;
				this.match(SqlBaseParser.SET);
				this.state = 684;
				this.match(SqlBaseParser.ROLE);
				this.state = 688;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 63, this._ctx) ) {
				case 1:
					{
					this.state = 685;
					this.match(SqlBaseParser.ALL);
					}
					break;
				case 2:
					{
					this.state = 686;
					this.match(SqlBaseParser.NONE);
					}
					break;
				case 3:
					{
					this.state = 687;
					(localctx as SetRoleContext)._role = this.identifier();
					}
					break;
				}
				}
				break;
			case 39:
				localctx = new GrantContext(this, localctx);
				this.enterOuterAlt(localctx, 39);
				{
				this.state = 690;
				this.match(SqlBaseParser.GRANT);
				this.state = 701;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 65, this._ctx) ) {
				case 1:
					{
					this.state = 691;
					this.privilege();
					this.state = 696;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 692;
						this.match(SqlBaseParser.T__3);
						this.state = 693;
						this.privilege();
						}
						}
						this.state = 698;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 699;
					this.match(SqlBaseParser.ALL);
					this.state = 700;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 703;
				this.match(SqlBaseParser.ON);
				this.state = 705;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 704;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 707;
				this.qualifiedName();
				this.state = 708;
				this.match(SqlBaseParser.TO);
				this.state = 709;
				(localctx as GrantContext)._grantee = this.principal();
				this.state = 713;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 710;
					this.match(SqlBaseParser.WITH);
					this.state = 711;
					this.match(SqlBaseParser.GRANT);
					this.state = 712;
					this.match(SqlBaseParser.OPTION);
					}
				}

				}
				break;
			case 40:
				localctx = new RevokeContext(this, localctx);
				this.enterOuterAlt(localctx, 40);
				{
				this.state = 715;
				this.match(SqlBaseParser.REVOKE);
				this.state = 719;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 68, this._ctx) ) {
				case 1:
					{
					this.state = 716;
					this.match(SqlBaseParser.GRANT);
					this.state = 717;
					this.match(SqlBaseParser.OPTION);
					this.state = 718;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 731;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 70, this._ctx) ) {
				case 1:
					{
					this.state = 721;
					this.privilege();
					this.state = 726;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 722;
						this.match(SqlBaseParser.T__3);
						this.state = 723;
						this.privilege();
						}
						}
						this.state = 728;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 729;
					this.match(SqlBaseParser.ALL);
					this.state = 730;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 733;
				this.match(SqlBaseParser.ON);
				this.state = 735;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 734;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 737;
				this.qualifiedName();
				this.state = 738;
				this.match(SqlBaseParser.FROM);
				this.state = 739;
				(localctx as RevokeContext)._grantee = this.principal();
				}
				break;
			case 41:
				localctx = new ShowGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 41);
				{
				this.state = 741;
				this.match(SqlBaseParser.SHOW);
				this.state = 742;
				this.match(SqlBaseParser.GRANTS);
				this.state = 748;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===137) {
					{
					this.state = 743;
					this.match(SqlBaseParser.ON);
					this.state = 745;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===193) {
						{
						this.state = 744;
						this.match(SqlBaseParser.TABLE);
						}
					}

					this.state = 747;
					this.qualifiedName();
					}
				}

				}
				break;
			case 42:
				localctx = new ExplainContext(this, localctx);
				this.enterOuterAlt(localctx, 42);
				{
				this.state = 750;
				this.match(SqlBaseParser.EXPLAIN);
				this.state = 752;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 74, this._ctx) ) {
				case 1:
					{
					this.state = 751;
					this.match(SqlBaseParser.ANALYZE);
					}
					break;
				}
				this.state = 755;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===219) {
					{
					this.state = 754;
					this.match(SqlBaseParser.VERBOSE);
					}
				}

				this.state = 768;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 77, this._ctx) ) {
				case 1:
					{
					this.state = 757;
					this.match(SqlBaseParser.T__1);
					this.state = 758;
					this.explainOption();
					this.state = 763;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 759;
						this.match(SqlBaseParser.T__3);
						this.state = 760;
						this.explainOption();
						}
						}
						this.state = 765;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 766;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 770;
				this.statement();
				}
				break;
			case 43:
				localctx = new ShowCreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 43);
				{
				this.state = 771;
				this.match(SqlBaseParser.SHOW);
				this.state = 772;
				this.match(SqlBaseParser.CREATE);
				this.state = 773;
				this.match(SqlBaseParser.TABLE);
				this.state = 774;
				this.qualifiedName();
				}
				break;
			case 44:
				localctx = new ShowCreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 44);
				{
				this.state = 775;
				this.match(SqlBaseParser.SHOW);
				this.state = 776;
				this.match(SqlBaseParser.CREATE);
				this.state = 777;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 778;
				this.qualifiedName();
				}
				break;
			case 45:
				localctx = new ShowCreateViewContext(this, localctx);
				this.enterOuterAlt(localctx, 45);
				{
				this.state = 779;
				this.match(SqlBaseParser.SHOW);
				this.state = 780;
				this.match(SqlBaseParser.CREATE);
				this.state = 781;
				this.match(SqlBaseParser.VIEW);
				this.state = 782;
				this.qualifiedName();
				}
				break;
			case 46:
				localctx = new ShowCreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 46);
				{
				this.state = 783;
				this.match(SqlBaseParser.SHOW);
				this.state = 784;
				this.match(SqlBaseParser.CREATE);
				this.state = 785;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 786;
				this.match(SqlBaseParser.VIEW);
				this.state = 787;
				this.qualifiedName();
				}
				break;
			case 47:
				localctx = new ShowCreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 47);
				{
				this.state = 788;
				this.match(SqlBaseParser.SHOW);
				this.state = 789;
				this.match(SqlBaseParser.CREATE);
				this.state = 790;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 791;
				this.qualifiedName();
				this.state = 793;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 792;
					this.types();
					}
				}

				}
				break;
			case 48:
				localctx = new ShowTablesContext(this, localctx);
				this.enterOuterAlt(localctx, 48);
				{
				this.state = 795;
				this.match(SqlBaseParser.SHOW);
				this.state = 796;
				this.match(SqlBaseParser.TABLES);
				this.state = 799;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 797;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 798;
					this.qualifiedName();
					}
				}

				this.state = 807;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 801;
					this.match(SqlBaseParser.LIKE);
					this.state = 802;
					(localctx as ShowTablesContext)._pattern = this.string_();
					this.state = 805;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 803;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 804;
						(localctx as ShowTablesContext)._escape = this.string_();
						}
					}

					}
				}

				}
				break;
			case 49:
				localctx = new ShowSchemasContext(this, localctx);
				this.enterOuterAlt(localctx, 49);
				{
				this.state = 809;
				this.match(SqlBaseParser.SHOW);
				this.state = 810;
				this.match(SqlBaseParser.SCHEMAS);
				this.state = 813;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 811;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 812;
					this.identifier();
					}
				}

				this.state = 821;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 815;
					this.match(SqlBaseParser.LIKE);
					this.state = 816;
					(localctx as ShowSchemasContext)._pattern = this.string_();
					this.state = 819;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 817;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 818;
						(localctx as ShowSchemasContext)._escape = this.string_();
						}
					}

					}
				}

				}
				break;
			case 50:
				localctx = new ShowCatalogsContext(this, localctx);
				this.enterOuterAlt(localctx, 50);
				{
				this.state = 823;
				this.match(SqlBaseParser.SHOW);
				this.state = 824;
				this.match(SqlBaseParser.CATALOGS);
				this.state = 831;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 825;
					this.match(SqlBaseParser.LIKE);
					this.state = 826;
					(localctx as ShowCatalogsContext)._pattern = this.string_();
					this.state = 829;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 827;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 828;
						(localctx as ShowCatalogsContext)._escape = this.string_();
						}
					}

					}
				}

				}
				break;
			case 51:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 51);
				{
				this.state = 833;
				this.match(SqlBaseParser.SHOW);
				this.state = 834;
				this.match(SqlBaseParser.COLUMNS);
				this.state = 835;
				_la = this._input.LA(1);
				if(!(_la===78 || _la===93)) {
				this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 836;
				this.qualifiedName();
				}
				break;
			case 52:
				localctx = new ShowStatsContext(this, localctx);
				this.enterOuterAlt(localctx, 52);
				{
				this.state = 837;
				this.match(SqlBaseParser.SHOW);
				this.state = 838;
				this.match(SqlBaseParser.STATS);
				this.state = 839;
				this.match(SqlBaseParser.FOR);
				this.state = 840;
				this.qualifiedName();
				}
				break;
			case 53:
				localctx = new ShowStatsForQueryContext(this, localctx);
				this.enterOuterAlt(localctx, 53);
				{
				this.state = 841;
				this.match(SqlBaseParser.SHOW);
				this.state = 842;
				this.match(SqlBaseParser.STATS);
				this.state = 843;
				this.match(SqlBaseParser.FOR);
				this.state = 844;
				this.match(SqlBaseParser.T__1);
				this.state = 845;
				this.querySpecification();
				this.state = 846;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 54:
				localctx = new ShowRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 54);
				{
				this.state = 848;
				this.match(SqlBaseParser.SHOW);
				this.state = 850;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===40) {
					{
					this.state = 849;
					this.match(SqlBaseParser.CURRENT);
					}
				}

				this.state = 852;
				this.match(SqlBaseParser.ROLES);
				this.state = 855;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 853;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 854;
					this.identifier();
					}
				}

				}
				break;
			case 55:
				localctx = new ShowRoleGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 55);
				{
				this.state = 857;
				this.match(SqlBaseParser.SHOW);
				this.state = 858;
				this.match(SqlBaseParser.ROLE);
				this.state = 859;
				this.match(SqlBaseParser.GRANTS);
				this.state = 862;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 860;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 861;
					this.identifier();
					}
				}

				}
				break;
			case 56:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 56);
				{
				this.state = 864;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 865;
				this.qualifiedName();
				}
				break;
			case 57:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 57);
				{
				this.state = 866;
				this.match(SqlBaseParser.DESC);
				this.state = 867;
				this.qualifiedName();
				}
				break;
			case 58:
				localctx = new ShowFunctionsContext(this, localctx);
				this.enterOuterAlt(localctx, 58);
				{
				this.state = 868;
				this.match(SqlBaseParser.SHOW);
				this.state = 869;
				this.match(SqlBaseParser.FUNCTIONS);
				this.state = 876;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 870;
					this.match(SqlBaseParser.LIKE);
					this.state = 871;
					(localctx as ShowFunctionsContext)._pattern = this.string_();
					this.state = 874;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 872;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 873;
						(localctx as ShowFunctionsContext)._escape = this.string_();
						}
					}

					}
				}

				}
				break;
			case 59:
				localctx = new ShowSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 59);
				{
				this.state = 878;
				this.match(SqlBaseParser.SHOW);
				this.state = 879;
				this.match(SqlBaseParser.SESSION);
				this.state = 886;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 880;
					this.match(SqlBaseParser.LIKE);
					this.state = 881;
					(localctx as ShowSessionContext)._pattern = this.string_();
					this.state = 884;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 882;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 883;
						(localctx as ShowSessionContext)._escape = this.string_();
						}
					}

					}
				}

				}
				break;
			case 60:
				localctx = new SetSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 60);
				{
				this.state = 888;
				this.match(SqlBaseParser.SET);
				this.state = 889;
				this.match(SqlBaseParser.SESSION);
				this.state = 890;
				this.qualifiedName();
				this.state = 891;
				this.match(SqlBaseParser.EQ);
				this.state = 892;
				this.expression();
				}
				break;
			case 61:
				localctx = new ResetSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 61);
				{
				this.state = 894;
				this.match(SqlBaseParser.RESET);
				this.state = 895;
				this.match(SqlBaseParser.SESSION);
				this.state = 896;
				this.qualifiedName();
				}
				break;
			case 62:
				localctx = new StartTransactionContext(this, localctx);
				this.enterOuterAlt(localctx, 62);
				{
				this.state = 897;
				this.match(SqlBaseParser.START);
				this.state = 898;
				this.match(SqlBaseParser.TRANSACTION);
				this.state = 907;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===104 || _la===155) {
					{
					this.state = 899;
					this.transactionMode();
					this.state = 904;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 900;
						this.match(SqlBaseParser.T__3);
						this.state = 901;
						this.transactionMode();
						}
						}
						this.state = 906;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				}
				break;
			case 63:
				localctx = new CommitContext(this, localctx);
				this.enterOuterAlt(localctx, 63);
				{
				this.state = 909;
				this.match(SqlBaseParser.COMMIT);
				this.state = 911;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 910;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 64:
				localctx = new RollbackContext(this, localctx);
				this.enterOuterAlt(localctx, 64);
				{
				this.state = 913;
				this.match(SqlBaseParser.ROLLBACK);
				this.state = 915;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 914;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 65:
				localctx = new PrepareContext(this, localctx);
				this.enterOuterAlt(localctx, 65);
				{
				this.state = 917;
				this.match(SqlBaseParser.PREPARE);
				this.state = 918;
				this.identifier();
				this.state = 919;
				this.match(SqlBaseParser.FROM);
				this.state = 920;
				this.statement();
				}
				break;
			case 66:
				localctx = new DeallocateContext(this, localctx);
				this.enterOuterAlt(localctx, 66);
				{
				this.state = 922;
				this.match(SqlBaseParser.DEALLOCATE);
				this.state = 923;
				this.match(SqlBaseParser.PREPARE);
				this.state = 924;
				this.identifier();
				}
				break;
			case 67:
				localctx = new ExecuteContext(this, localctx);
				this.enterOuterAlt(localctx, 67);
				{
				this.state = 925;
				this.match(SqlBaseParser.EXECUTE);
				this.state = 926;
				this.identifier();
				this.state = 936;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===216) {
					{
					this.state = 927;
					this.match(SqlBaseParser.USING);
					this.state = 928;
					this.expression();
					this.state = 933;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 929;
						this.match(SqlBaseParser.T__3);
						this.state = 930;
						this.expression();
						}
						}
						this.state = 935;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				}
				break;
			case 68:
				localctx = new DescribeInputContext(this, localctx);
				this.enterOuterAlt(localctx, 68);
				{
				this.state = 938;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 939;
				this.match(SqlBaseParser.INPUT);
				this.state = 940;
				this.identifier();
				}
				break;
			case 69:
				localctx = new DescribeOutputContext(this, localctx);
				this.enterOuterAlt(localctx, 69);
				{
				this.state = 941;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 942;
				this.match(SqlBaseParser.OUTPUT);
				this.state = 943;
				this.identifier();
				}
				break;
			case 70:
				localctx = new UpdateContext(this, localctx);
				this.enterOuterAlt(localctx, 70);
				{
				this.state = 944;
				this.match(SqlBaseParser.UPDATE);
				this.state = 945;
				this.qualifiedName();
				this.state = 946;
				this.match(SqlBaseParser.SET);
				this.state = 947;
				this.updateAssignment();
				this.state = 952;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 948;
					this.match(SqlBaseParser.T__3);
					this.state = 949;
					this.updateAssignment();
					}
					}
					this.state = 954;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 957;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 955;
					this.match(SqlBaseParser.WHERE);
					this.state = 956;
					(localctx as UpdateContext)._where = this.booleanExpression(0);
					}
				}

				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public query(): QueryContext {
		let localctx: QueryContext = new QueryContext(this, this._ctx, this.state);
		this.enterRule(localctx, 18, SqlBaseParser.RULE_query);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 962;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 961;
				this.with_();
				}
			}

			this.state = 964;
			this.queryNoWith();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public with_(): WithContext {
		let localctx: WithContext = new WithContext(this, this._ctx, this.state);
		this.enterRule(localctx, 20, SqlBaseParser.RULE_with);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 966;
			this.match(SqlBaseParser.WITH);
			this.state = 968;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===156) {
				{
				this.state = 967;
				this.match(SqlBaseParser.RECURSIVE);
				}
			}

			this.state = 970;
			this.namedQuery();
			this.state = 975;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 971;
				this.match(SqlBaseParser.T__3);
				this.state = 972;
				this.namedQuery();
				}
				}
				this.state = 977;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public tableElement(): TableElementContext {
		let localctx: TableElementContext = new TableElementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 22, SqlBaseParser.RULE_tableElement);
		try {
			this.state = 981;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 106, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 978;
				this.constraintSpecification();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 979;
				this.columnDefinition();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 980;
				this.likeClause();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public columnDefinition(): ColumnDefinitionContext {
		let localctx: ColumnDefinitionContext = new ColumnDefinitionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 24, SqlBaseParser.RULE_columnDefinition);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 983;
			this.identifier();
			this.state = 984;
			this.type_(0);
			this.state = 987;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===131) {
				{
				this.state = 985;
				this.match(SqlBaseParser.NOT);
				this.state = 986;
				this.match(SqlBaseParser.NULL);
				}
			}

			this.state = 991;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===33) {
				{
				this.state = 989;
				this.match(SqlBaseParser.COMMENT);
				this.state = 990;
				this.string_();
				}
			}

			this.state = 995;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 993;
				this.match(SqlBaseParser.WITH);
				this.state = 994;
				this.properties();
				}
			}

			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public likeClause(): LikeClauseContext {
		let localctx: LikeClauseContext = new LikeClauseContext(this, this._ctx, this.state);
		this.enterRule(localctx, 26, SqlBaseParser.RULE_likeClause);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 997;
			this.match(SqlBaseParser.LIKE);
			this.state = 998;
			this.qualifiedName();
			this.state = 1001;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===65 || _la===94) {
				{
				this.state = 999;
				localctx._optionType = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===65 || _la===94)) {
				    localctx._optionType = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1000;
				this.match(SqlBaseParser.PROPERTIES);
				}
			}

			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public properties(): PropertiesContext {
		let localctx: PropertiesContext = new PropertiesContext(this, this._ctx, this.state);
		this.enterRule(localctx, 28, SqlBaseParser.RULE_properties);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1003;
			this.match(SqlBaseParser.T__1);
			this.state = 1004;
			this.property();
			this.state = 1009;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 1005;
				this.match(SqlBaseParser.T__3);
				this.state = 1006;
				this.property();
				}
				}
				this.state = 1011;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 1012;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public property(): PropertyContext {
		let localctx: PropertyContext = new PropertyContext(this, this._ctx, this.state);
		this.enterRule(localctx, 30, SqlBaseParser.RULE_property);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1014;
			this.identifier();
			this.state = 1015;
			this.match(SqlBaseParser.EQ);
			this.state = 1016;
			this.expression();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public sqlParameterDeclaration(): SqlParameterDeclarationContext {
		let localctx: SqlParameterDeclarationContext = new SqlParameterDeclarationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 32, SqlBaseParser.RULE_sqlParameterDeclaration);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1018;
			this.identifier();
			this.state = 1019;
			this.type_(0);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public routineCharacteristics(): RoutineCharacteristicsContext {
		let localctx: RoutineCharacteristicsContext = new RoutineCharacteristicsContext(this, this._ctx, this.state);
		this.enterRule(localctx, 34, SqlBaseParser.RULE_routineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1024;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===54 || _la===108 || _la===131 || _la===166) {
				{
				{
				this.state = 1021;
				this.routineCharacteristic();
				}
				}
				this.state = 1026;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public routineCharacteristic(): RoutineCharacteristicContext {
		let localctx: RoutineCharacteristicContext = new RoutineCharacteristicContext(this, this._ctx, this.state);
		this.enterRule(localctx, 36, SqlBaseParser.RULE_routineCharacteristic);
		try {
			this.state = 1031;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 108:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1027;
				this.match(SqlBaseParser.LANGUAGE);
				this.state = 1028;
				this.language();
				}
				break;
			case 54:
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1029;
				this.determinism();
				}
				break;
			case 26:
			case 166:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1030;
				this.nullCallClause();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public alterRoutineCharacteristics(): AlterRoutineCharacteristicsContext {
		let localctx: AlterRoutineCharacteristicsContext = new AlterRoutineCharacteristicsContext(this, this._ctx, this.state);
		this.enterRule(localctx, 38, SqlBaseParser.RULE_alterRoutineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1036;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===166) {
				{
				{
				this.state = 1033;
				this.alterRoutineCharacteristic();
				}
				}
				this.state = 1038;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public alterRoutineCharacteristic(): AlterRoutineCharacteristicContext {
		let localctx: AlterRoutineCharacteristicContext = new AlterRoutineCharacteristicContext(this, this._ctx, this.state);
		this.enterRule(localctx, 40, SqlBaseParser.RULE_alterRoutineCharacteristic);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1039;
			this.nullCallClause();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public routineBody(): RoutineBodyContext {
		let localctx: RoutineBodyContext = new RoutineBodyContext(this, this._ctx, this.state);
		this.enterRule(localctx, 42, SqlBaseParser.RULE_routineBody);
		try {
			this.state = 1043;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 165:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1041;
				this.returnStatement();
				}
				break;
			case 70:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1042;
				this.externalBodyReference();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public returnStatement(): ReturnStatementContext {
		let localctx: ReturnStatementContext = new ReturnStatementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 44, SqlBaseParser.RULE_returnStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1045;
			this.match(SqlBaseParser.RETURN);
			this.state = 1046;
			this.expression();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public externalBodyReference(): ExternalBodyReferenceContext {
		let localctx: ExternalBodyReferenceContext = new ExternalBodyReferenceContext(this, this._ctx, this.state);
		this.enterRule(localctx, 46, SqlBaseParser.RULE_externalBodyReference);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1048;
			this.match(SqlBaseParser.EXTERNAL);
			this.state = 1051;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===122) {
				{
				this.state = 1049;
				this.match(SqlBaseParser.NAME);
				this.state = 1050;
				this.externalRoutineName();
				}
			}

			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public language(): LanguageContext {
		let localctx: LanguageContext = new LanguageContext(this, this._ctx, this.state);
		this.enterRule(localctx, 48, SqlBaseParser.RULE_language);
		try {
			this.state = 1055;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 117, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1053;
				this.match(SqlBaseParser.SQL);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1054;
				this.identifier();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public determinism(): DeterminismContext {
		let localctx: DeterminismContext = new DeterminismContext(this, this._ctx, this.state);
		this.enterRule(localctx, 50, SqlBaseParser.RULE_determinism);
		try {
			this.state = 1060;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 54:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1057;
				this.match(SqlBaseParser.DETERMINISTIC);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1058;
				this.match(SqlBaseParser.NOT);
				this.state = 1059;
				this.match(SqlBaseParser.DETERMINISTIC);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public nullCallClause(): NullCallClauseContext {
		let localctx: NullCallClauseContext = new NullCallClauseContext(this, this._ctx, this.state);
		this.enterRule(localctx, 52, SqlBaseParser.RULE_nullCallClause);
		try {
			this.state = 1071;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 166:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1062;
				this.match(SqlBaseParser.RETURNS);
				this.state = 1063;
				this.match(SqlBaseParser.NULL);
				this.state = 1064;
				this.match(SqlBaseParser.ON);
				this.state = 1065;
				this.match(SqlBaseParser.NULL);
				this.state = 1066;
				this.match(SqlBaseParser.INPUT);
				}
				break;
			case 26:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1067;
				this.match(SqlBaseParser.CALLED);
				this.state = 1068;
				this.match(SqlBaseParser.ON);
				this.state = 1069;
				this.match(SqlBaseParser.NULL);
				this.state = 1070;
				this.match(SqlBaseParser.INPUT);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public externalRoutineName(): ExternalRoutineNameContext {
		let localctx: ExternalRoutineNameContext = new ExternalRoutineNameContext(this, this._ctx, this.state);
		this.enterRule(localctx, 54, SqlBaseParser.RULE_externalRoutineName);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1073;
			this.identifier();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public queryNoWith(): QueryNoWithContext {
		let localctx: QueryNoWithContext = new QueryNoWithContext(this, this._ctx, this.state);
		this.enterRule(localctx, 56, SqlBaseParser.RULE_queryNoWith);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1075;
			this.queryTerm(0);
			this.state = 1079;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1076;
				this.match(SqlBaseParser.ORDER);
				this.state = 1077;
				this.match(SqlBaseParser.BY);
				this.state = 1078;
				this.sortItemList();
				}
			}

			this.state = 1086;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===136) {
				{
				this.state = 1081;
				this.match(SqlBaseParser.OFFSET);
				this.state = 1082;
				localctx._offset = this.match(SqlBaseParser.INTEGER_VALUE);
				this.state = 1084;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===173 || _la===174) {
					{
					this.state = 1083;
					_la = this._input.LA(1);
					if(!(_la===173 || _la===174)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					}
				}

				}
			}

			this.state = 1097;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===72 || _la===114) {
				{
				this.state = 1095;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 114:
					{
					this.state = 1088;
					this.match(SqlBaseParser.LIMIT);
					this.state = 1089;
					localctx._limit = this._input.LT(1);
					_la = this._input.LA(1);
					if(!(_la===12 || _la===244)) {
					    localctx._limit = this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					}
					break;
				case 72:
					{
					{
					this.state = 1090;
					this.match(SqlBaseParser.FETCH);
					this.state = 1091;
					this.match(SqlBaseParser.FIRST);
					this.state = 1092;
					localctx._fetchFirstNRows = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1093;
					this.match(SqlBaseParser.ROWS);
					this.state = 1094;
					this.match(SqlBaseParser.ONLY);
					}
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
			}

			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public sortItemList(): SortItemListContext {
		let localctx: SortItemListContext = new SortItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 58, SqlBaseParser.RULE_sortItemList);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1099;
			this.sortItem();
			this.state = 1104;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 1100;
				this.match(SqlBaseParser.T__3);
				this.state = 1101;
				this.sortItem();
				}
				}
				this.state = 1106;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public queryTerm(): QueryTermContext;
	public queryTerm(_p: number): QueryTermContext;
	// @RuleVersion(0)
	public queryTerm(_p?: number): QueryTermContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: QueryTermContext = new QueryTermContext(this, this._ctx, _parentState);
		let _prevctx: QueryTermContext = localctx;
		let _startState: number = 60;
		this.enterRecursionRule(localctx, 60, SqlBaseParser.RULE_queryTerm, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new QueryTermDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1108;
			this.queryPrimary();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1124;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 129, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1122;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 128, this._ctx) ) {
					case 1:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1110;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1111;
						(localctx as SetOperationContext)._operator = this.match(SqlBaseParser.INTERSECT);
						this.state = 1113;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1112;
							this.setQuantifier();
							}
						}

						this.state = 1115;
						(localctx as SetOperationContext)._right = this.queryTerm(3);
						}
						break;
					case 2:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1116;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1117;
						(localctx as SetOperationContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===64 || _la===210)) {
						    (localctx as SetOperationContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1119;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1118;
							this.setQuantifier();
							}
						}

						this.state = 1121;
						(localctx as SetOperationContext)._right = this.queryTerm(2);
						}
						break;
					}
					}
				}
				this.state = 1126;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 129, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}
	// @RuleVersion(0)
	public queryPrimary(): QueryPrimaryContext {
		let localctx: QueryPrimaryContext = new QueryPrimaryContext(this, this._ctx, this.state);
		this.enterRule(localctx, 62, SqlBaseParser.RULE_queryPrimary);
		try {
			let _alt: number;
			this.state = 1143;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				localctx = new QueryPrimaryDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1127;
				this.querySpecification();
				}
				break;
			case 193:
				localctx = new TableContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1128;
				this.match(SqlBaseParser.TABLE);
				this.state = 1129;
				this.qualifiedName();
				}
				break;
			case 218:
				localctx = new InlineTableContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1130;
				this.match(SqlBaseParser.VALUES);
				this.state = 1131;
				this.expression();
				this.state = 1136;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1132;
						this.match(SqlBaseParser.T__3);
						this.state = 1133;
						this.expression();
						}
						}
					}
					this.state = 1138;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				}
				}
				break;
			case 2:
				localctx = new SubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1139;
				this.match(SqlBaseParser.T__1);
				this.state = 1140;
				this.queryNoWith();
				this.state = 1141;
				this.match(SqlBaseParser.T__2);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public sortItem(): SortItemContext {
		let localctx: SortItemContext = new SortItemContext(this, this._ctx, this.state);
		this.enterRule(localctx, 64, SqlBaseParser.RULE_sortItem);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1145;
			this.expression();
			this.state = 1147;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===19 || _la===52) {
				{
				this.state = 1146;
				localctx._ordering = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===19 || _la===52)) {
				    localctx._ordering = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
			}

			this.state = 1151;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===134) {
				{
				this.state = 1149;
				this.match(SqlBaseParser.NULLS);
				this.state = 1150;
				localctx._nullOrdering = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===74 || _la===109)) {
				    localctx._nullOrdering = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
			}

			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public querySpecification(): QuerySpecificationContext {
		let localctx: QuerySpecificationContext = new QuerySpecificationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 66, SqlBaseParser.RULE_querySpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1153;
			this.match(SqlBaseParser.SELECT);
			this.state = 1155;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 134, this._ctx) ) {
			case 1:
				{
				this.state = 1154;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1157;
			this.selectItemList();
			this.state = 1160;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 135, this._ctx) ) {
			case 1:
				{
				this.state = 1158;
				this.match(SqlBaseParser.FROM);
				this.state = 1159;
				this.relationList();
				}
				break;
			}
			this.state = 1164;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 136, this._ctx) ) {
			case 1:
				{
				this.state = 1162;
				this.match(SqlBaseParser.WHERE);
				this.state = 1163;
				localctx._where = this.booleanExpression(0);
				}
				break;
			}
			this.state = 1169;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 137, this._ctx) ) {
			case 1:
				{
				this.state = 1166;
				this.match(SqlBaseParser.GROUP);
				this.state = 1167;
				this.match(SqlBaseParser.BY);
				this.state = 1168;
				this.groupBy();
				}
				break;
			}
			this.state = 1173;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 138, this._ctx) ) {
			case 1:
				{
				this.state = 1171;
				this.match(SqlBaseParser.HAVING);
				this.state = 1172;
				localctx._having = this.booleanExpression(0);
				}
				break;
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public selectItemList(): SelectItemListContext {
		let localctx: SelectItemListContext = new SelectItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 68, SqlBaseParser.RULE_selectItemList);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1175;
			this.selectItem();
			this.state = 1180;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 139, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1176;
					this.match(SqlBaseParser.T__3);
					this.state = 1177;
					this.selectItem();
					}
					}
				}
				this.state = 1182;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 139, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public relationList(): RelationListContext {
		let localctx: RelationListContext = new RelationListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 70, SqlBaseParser.RULE_relationList);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1183;
			this.relation(0);
			this.state = 1188;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 140, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1184;
					this.match(SqlBaseParser.T__3);
					this.state = 1185;
					this.relation(0);
					}
					}
				}
				this.state = 1190;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 140, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public groupBy(): GroupByContext {
		let localctx: GroupByContext = new GroupByContext(this, this._ctx, this.state);
		this.enterRule(localctx, 72, SqlBaseParser.RULE_groupBy);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1192;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 141, this._ctx) ) {
			case 1:
				{
				this.state = 1191;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1194;
			this.groupingElement();
			this.state = 1199;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 142, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1195;
					this.match(SqlBaseParser.T__3);
					this.state = 1196;
					this.groupingElement();
					}
					}
				}
				this.state = 1201;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 142, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public groupingElement(): GroupingElementContext {
		let localctx: GroupingElementContext = new GroupingElementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 74, SqlBaseParser.RULE_groupingElement);
		let _la: number;
		try {
			this.state = 1242;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 148, this._ctx) ) {
			case 1:
				localctx = new SingleGroupingSetContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1202;
				this.groupingSet();
				}
				break;
			case 2:
				localctx = new RollupContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1203;
				this.match(SqlBaseParser.ROLLUP);
				this.state = 1204;
				this.match(SqlBaseParser.T__1);
				this.state = 1213;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1205;
					this.expression();
					this.state = 1210;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1206;
						this.match(SqlBaseParser.T__3);
						this.state = 1207;
						this.expression();
						}
						}
						this.state = 1212;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1215;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new CubeContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1216;
				this.match(SqlBaseParser.CUBE);
				this.state = 1217;
				this.match(SqlBaseParser.T__1);
				this.state = 1226;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1218;
					this.expression();
					this.state = 1223;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1219;
						this.match(SqlBaseParser.T__3);
						this.state = 1220;
						this.expression();
						}
						}
						this.state = 1225;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1228;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				localctx = new MultipleGroupingSetsContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1229;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1230;
				this.match(SqlBaseParser.SETS);
				this.state = 1231;
				this.match(SqlBaseParser.T__1);
				this.state = 1232;
				this.groupingSet();
				this.state = 1237;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1233;
					this.match(SqlBaseParser.T__3);
					this.state = 1234;
					this.groupingSet();
					}
					}
					this.state = 1239;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1240;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public groupingSet(): GroupingSetContext {
		let localctx: GroupingSetContext = new GroupingSetContext(this, this._ctx, this.state);
		this.enterRule(localctx, 76, SqlBaseParser.RULE_groupingSet);
		let _la: number;
		try {
			this.state = 1257;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 151, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1244;
				this.match(SqlBaseParser.T__1);
				this.state = 1253;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1245;
					this.expression();
					this.state = 1250;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1246;
						this.match(SqlBaseParser.T__3);
						this.state = 1247;
						this.expression();
						}
						}
						this.state = 1252;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1255;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1256;
				this.expression();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public namedQuery(): NamedQueryContext {
		let localctx: NamedQueryContext = new NamedQueryContext(this, this._ctx, this.state);
		this.enterRule(localctx, 78, SqlBaseParser.RULE_namedQuery);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1259;
			localctx._name = this.identifier();
			this.state = 1261;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===2) {
				{
				this.state = 1260;
				this.columnAliases();
				}
			}

			this.state = 1263;
			this.match(SqlBaseParser.AS);
			this.state = 1264;
			this.match(SqlBaseParser.T__1);
			this.state = 1265;
			this.query();
			this.state = 1266;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public setQuantifier(): SetQuantifierContext {
		let localctx: SetQuantifierContext = new SetQuantifierContext(this, this._ctx, this.state);
		this.enterRule(localctx, 80, SqlBaseParser.RULE_setQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1268;
			_la = this._input.LA(1);
			if(!(_la===12 || _la===56)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public selectItem(): SelectItemContext {
		let localctx: SelectItemContext = new SelectItemContext(this, this._ctx, this.state);
		this.enterRule(localctx, 82, SqlBaseParser.RULE_selectItem);
		let _la: number;
		try {
			this.state = 1282;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 155, this._ctx) ) {
			case 1:
				localctx = new SelectSingleContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1270;
				this.expression();
				this.state = 1275;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 154, this._ctx) ) {
				case 1:
					{
					this.state = 1272;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===18) {
						{
						this.state = 1271;
						this.match(SqlBaseParser.AS);
						}
					}

					this.state = 1274;
					this.identifier();
					}
					break;
				}
				}
				break;
			case 2:
				localctx = new SelectAllContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1277;
				this.qualifiedName();
				this.state = 1278;
				this.match(SqlBaseParser.T__0);
				this.state = 1279;
				this.match(SqlBaseParser.ASTERISK);
				}
				break;
			case 3:
				localctx = new SelectAllContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1281;
				this.match(SqlBaseParser.ASTERISK);
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public relation(): RelationContext;
	public relation(_p: number): RelationContext;
	// @RuleVersion(0)
	public relation(_p?: number): RelationContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: RelationContext = new RelationContext(this, this._ctx, _parentState);
		let _prevctx: RelationContext = localctx;
		let _startState: number = 84;
		this.enterRecursionRule(localctx, 84, SqlBaseParser.RULE_relation, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new RelationDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1285;
			this.sampledRelation();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1305;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 157, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					{
					localctx = new JoinRelationContext(this, new RelationContext(this, _parentctx, _parentState));
					(localctx as JoinRelationContext)._left = _prevctx;
					this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_relation);
					this.state = 1287;
					if (!(this.precpred(this._ctx, 2))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
					}
					this.state = 1301;
					this._errHandler.sync(this);
					switch (this._input.LA(1)) {
					case 38:
						{
						this.state = 1288;
						this.match(SqlBaseParser.CROSS);
						this.state = 1289;
						this.match(SqlBaseParser.JOIN);
						this.state = 1290;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					case 79:
					case 95:
					case 106:
					case 111:
					case 168:
						{
						this.state = 1291;
						this.joinType();
						this.state = 1292;
						this.match(SqlBaseParser.JOIN);
						this.state = 1293;
						(localctx as JoinRelationContext)._rightRelation = this.relation(0);
						this.state = 1294;
						this.joinCriteria();
						}
						break;
					case 123:
						{
						this.state = 1296;
						this.match(SqlBaseParser.NATURAL);
						this.state = 1297;
						this.joinType();
						this.state = 1298;
						this.match(SqlBaseParser.JOIN);
						this.state = 1299;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					default:
						throw new NoViableAltException(this);
					}
					}
					}
				}
				this.state = 1307;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 157, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}
	// @RuleVersion(0)
	public joinType(): JoinTypeContext {
		let localctx: JoinTypeContext = new JoinTypeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 86, SqlBaseParser.RULE_joinType);
		let _la: number;
		try {
			this.state = 1323;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 95:
			case 106:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1309;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===95) {
					{
					this.state = 1308;
					this.match(SqlBaseParser.INNER);
					}
				}

				}
				break;
			case 111:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1311;
				this.match(SqlBaseParser.LEFT);
				this.state = 1313;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1312;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 168:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1315;
				this.match(SqlBaseParser.RIGHT);
				this.state = 1317;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1316;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 79:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1319;
				this.match(SqlBaseParser.FULL);
				this.state = 1321;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1320;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public joinCriteria(): JoinCriteriaContext {
		let localctx: JoinCriteriaContext = new JoinCriteriaContext(this, this._ctx, this.state);
		this.enterRule(localctx, 88, SqlBaseParser.RULE_joinCriteria);
		let _la: number;
		try {
			this.state = 1339;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 137:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1325;
				this.match(SqlBaseParser.ON);
				this.state = 1326;
				this.booleanExpression(0);
				}
				break;
			case 216:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1327;
				this.match(SqlBaseParser.USING);
				this.state = 1328;
				this.match(SqlBaseParser.T__1);
				this.state = 1329;
				this.identifier();
				this.state = 1334;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1330;
					this.match(SqlBaseParser.T__3);
					this.state = 1331;
					this.identifier();
					}
					}
					this.state = 1336;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1337;
				this.match(SqlBaseParser.T__2);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public sampledRelation(): SampledRelationContext {
		let localctx: SampledRelationContext = new SampledRelationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 90, SqlBaseParser.RULE_sampledRelation);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1341;
			this.aliasedRelation();
			this.state = 1348;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 165, this._ctx) ) {
			case 1:
				{
				this.state = 1342;
				this.match(SqlBaseParser.TABLESAMPLE);
				this.state = 1343;
				this.sampleType();
				this.state = 1344;
				this.match(SqlBaseParser.T__1);
				this.state = 1345;
				localctx._percentage = this.expression();
				this.state = 1346;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public sampleType(): SampleTypeContext {
		let localctx: SampleTypeContext = new SampleTypeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 92, SqlBaseParser.RULE_sampleType);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1350;
			_la = this._input.LA(1);
			if(!(_la===22 || _la===190)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public aliasedRelation(): AliasedRelationContext {
		let localctx: AliasedRelationContext = new AliasedRelationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 94, SqlBaseParser.RULE_aliasedRelation);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1352;
			this.relationPrimary();
			this.state = 1360;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 168, this._ctx) ) {
			case 1:
				{
				this.state = 1354;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===18) {
					{
					this.state = 1353;
					this.match(SqlBaseParser.AS);
					}
				}

				this.state = 1356;
				this.identifier();
				this.state = 1358;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 167, this._ctx) ) {
				case 1:
					{
					this.state = 1357;
					this.columnAliases();
					}
					break;
				}
				}
				break;
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public columnAliases(): ColumnAliasesContext {
		let localctx: ColumnAliasesContext = new ColumnAliasesContext(this, this._ctx, this.state);
		this.enterRule(localctx, 96, SqlBaseParser.RULE_columnAliases);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1362;
			this.match(SqlBaseParser.T__1);
			this.state = 1363;
			this.identifier();
			this.state = 1368;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 1364;
				this.match(SqlBaseParser.T__3);
				this.state = 1365;
				this.identifier();
				}
				}
				this.state = 1370;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 1371;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public relationPrimary(): RelationPrimaryContext {
		let localctx: RelationPrimaryContext = new RelationPrimaryContext(this, this._ctx, this.state);
		this.enterRule(localctx, 98, SqlBaseParser.RULE_relationPrimary);
		let _la: number;
		try {
			this.state = 1405;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 173, this._ctx) ) {
			case 1:
				localctx = new TableNameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1373;
				this.qualifiedName();
				this.state = 1375;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 170, this._ctx) ) {
				case 1:
					{
					this.state = 1374;
					this.tableVersionExpression();
					}
					break;
				}
				}
				break;
			case 2:
				localctx = new SubqueryRelationContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1377;
				this.match(SqlBaseParser.T__1);
				this.state = 1378;
				this.query();
				this.state = 1379;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new UnnestContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1381;
				this.match(SqlBaseParser.UNNEST);
				this.state = 1382;
				this.match(SqlBaseParser.T__1);
				this.state = 1383;
				this.expression();
				this.state = 1388;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1384;
					this.match(SqlBaseParser.T__3);
					this.state = 1385;
					this.expression();
					}
					}
					this.state = 1390;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1391;
				this.match(SqlBaseParser.T__2);
				this.state = 1394;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 172, this._ctx) ) {
				case 1:
					{
					this.state = 1392;
					this.match(SqlBaseParser.WITH);
					this.state = 1393;
					this.match(SqlBaseParser.ORDINALITY);
					}
					break;
				}
				}
				break;
			case 4:
				localctx = new LateralContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1396;
				this.match(SqlBaseParser.LATERAL);
				this.state = 1397;
				this.match(SqlBaseParser.T__1);
				this.state = 1398;
				this.query();
				this.state = 1399;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new ParenthesizedRelationContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1401;
				this.match(SqlBaseParser.T__1);
				this.state = 1402;
				this.relation(0);
				this.state = 1403;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public expression(): ExpressionContext {
		let localctx: ExpressionContext = new ExpressionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 100, SqlBaseParser.RULE_expression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1407;
			this.booleanExpression(0);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public booleanExpression(): BooleanExpressionContext;
	public booleanExpression(_p: number): BooleanExpressionContext;
	// @RuleVersion(0)
	public booleanExpression(_p?: number): BooleanExpressionContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: BooleanExpressionContext = new BooleanExpressionContext(this, this._ctx, _parentState);
		let _prevctx: BooleanExpressionContext = localctx;
		let _startState: number = 102;
		this.enterRecursionRule(localctx, 102, SqlBaseParser.RULE_booleanExpression, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1416;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 2:
			case 5:
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 67:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 87:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 130:
			case 132:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 203:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
			case 235:
			case 236:
			case 241:
			case 242:
			case 243:
			case 244:
			case 245:
			case 246:
			case 247:
			case 248:
			case 249:
			case 250:
			case 253:
				{
				localctx = new PredicatedContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1410;
				(localctx as PredicatedContext)._valueExpression = this.valueExpression(0);
				this.state = 1412;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 174, this._ctx) ) {
				case 1:
					{
					this.state = 1411;
					this.predicate((localctx as PredicatedContext)._valueExpression);
					}
					break;
				}
				}
				break;
			case 131:
				{
				localctx = new LogicalNotContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1414;
				this.match(SqlBaseParser.NOT);
				this.state = 1415;
				this.booleanExpression(3);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1426;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 177, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1424;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 176, this._ctx) ) {
					case 1:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1418;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1419;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.AND);
						this.state = 1420;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(3);
						}
						break;
					case 2:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1421;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1422;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.OR);
						this.state = 1423;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(2);
						}
						break;
					}
					}
				}
				this.state = 1428;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 177, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}
	// @RuleVersion(0)
	public predicate(value: ParserRuleContext): PredicateContext {
		let localctx: PredicateContext = new PredicateContext(this, this._ctx, this.state, value);
		this.enterRule(localctx, 104, SqlBaseParser.RULE_predicate);
		let _la: number;
		try {
			this.state = 1490;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 186, this._ctx) ) {
			case 1:
				localctx = new ComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1429;
				this.comparisonOperator();
				this.state = 1430;
				(localctx as ComparisonContext)._right = this.valueExpression(0);
				}
				break;
			case 2:
				localctx = new QuantifiedComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1432;
				this.comparisonOperator();
				this.state = 1433;
				this.comparisonQuantifier();
				this.state = 1434;
				this.match(SqlBaseParser.T__1);
				this.state = 1435;
				this.query();
				this.state = 1436;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new BetweenContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1439;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1438;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1441;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1442;
				(localctx as BetweenContext)._lower = this.valueExpression(0);
				this.state = 1443;
				this.match(SqlBaseParser.AND);
				this.state = 1444;
				(localctx as BetweenContext)._upper = this.valueExpression(0);
				}
				break;
			case 4:
				localctx = new InListContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1447;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1446;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1449;
				this.match(SqlBaseParser.IN);
				this.state = 1450;
				this.match(SqlBaseParser.T__1);
				this.state = 1451;
				this.expression();
				this.state = 1456;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1452;
					this.match(SqlBaseParser.T__3);
					this.state = 1453;
					this.expression();
					}
					}
					this.state = 1458;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1459;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new InSubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1462;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1461;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1464;
				this.match(SqlBaseParser.IN);
				this.state = 1465;
				this.match(SqlBaseParser.T__1);
				this.state = 1466;
				this.query();
				this.state = 1467;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 6:
				localctx = new LikeContext(this, localctx);
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1470;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1469;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1472;
				this.match(SqlBaseParser.LIKE);
				this.state = 1473;
				(localctx as LikeContext)._pattern = this.valueExpression(0);
				this.state = 1476;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 183, this._ctx) ) {
				case 1:
					{
					this.state = 1474;
					this.match(SqlBaseParser.ESCAPE);
					this.state = 1475;
					(localctx as LikeContext)._escape = this.valueExpression(0);
					}
					break;
				}
				}
				break;
			case 7:
				localctx = new NullPredicateContext(this, localctx);
				this.enterOuterAlt(localctx, 7);
				{
				this.state = 1478;
				this.match(SqlBaseParser.IS);
				this.state = 1480;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1479;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1482;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 8:
				localctx = new DistinctFromContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 1483;
				this.match(SqlBaseParser.IS);
				this.state = 1485;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1484;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1487;
				this.match(SqlBaseParser.DISTINCT);
				this.state = 1488;
				this.match(SqlBaseParser.FROM);
				this.state = 1489;
				(localctx as DistinctFromContext)._right = this.valueExpression(0);
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public valueExpression(): ValueExpressionContext;
	public valueExpression(_p: number): ValueExpressionContext;
	// @RuleVersion(0)
	public valueExpression(_p?: number): ValueExpressionContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: ValueExpressionContext = new ValueExpressionContext(this, this._ctx, _parentState);
		let _prevctx: ValueExpressionContext = localctx;
		let _startState: number = 106;
		this.enterRecursionRule(localctx, 106, SqlBaseParser.RULE_valueExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1496;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 2:
			case 5:
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 67:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 87:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 130:
			case 132:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 203:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
			case 241:
			case 242:
			case 243:
			case 244:
			case 245:
			case 246:
			case 247:
			case 248:
			case 249:
			case 250:
			case 253:
				{
				localctx = new ValueExpressionDefaultContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1493;
				this.primaryExpression(0);
				}
				break;
			case 235:
			case 236:
				{
				localctx = new ArithmeticUnaryContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1494;
				(localctx as ArithmeticUnaryContext)._operator = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===235 || _la===236)) {
				    (localctx as ArithmeticUnaryContext)._operator = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1495;
				this.valueExpression(4);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1512;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 189, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1510;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 188, this._ctx) ) {
					case 1:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1498;
						if (!(this.precpred(this._ctx, 3))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 3)");
						}
						this.state = 1499;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(((((_la - 237)) & ~0x1F) === 0 && ((1 << (_la - 237)) & 7) !== 0))) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1500;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(4);
						}
						break;
					case 2:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1501;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1502;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===235 || _la===236)) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1503;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(3);
						}
						break;
					case 3:
						{
						localctx = new ConcatenationContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ConcatenationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1504;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1505;
						this.match(SqlBaseParser.CONCAT);
						this.state = 1506;
						(localctx as ConcatenationContext)._right = this.valueExpression(2);
						}
						break;
					case 4:
						{
						localctx = new AtTimeZoneContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1507;
						if (!(this.precpred(this._ctx, 5))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 5)");
						}
						this.state = 1508;
						this.match(SqlBaseParser.AT);
						this.state = 1509;
						this.timeZoneSpecifier();
						}
						break;
					}
					}
				}
				this.state = 1514;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 189, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}

	public primaryExpression(): PrimaryExpressionContext;
	public primaryExpression(_p: number): PrimaryExpressionContext;
	// @RuleVersion(0)
	public primaryExpression(_p?: number): PrimaryExpressionContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: PrimaryExpressionContext = new PrimaryExpressionContext(this, this._ctx, _parentState);
		let _prevctx: PrimaryExpressionContext = localctx;
		let _startState: number = 108;
		this.enterRecursionRule(localctx, 108, SqlBaseParser.RULE_primaryExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1754;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 218, this._ctx) ) {
			case 1:
				{
				localctx = new NullLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1516;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 2:
				{
				localctx = new IntervalLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1517;
				this.interval();
				}
				break;
			case 3:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1518;
				this.identifier();
				this.state = 1519;
				this.string_();
				}
				break;
			case 4:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1521;
				this.match(SqlBaseParser.DOUBLE_PRECISION);
				this.state = 1522;
				this.string_();
				}
				break;
			case 5:
				{
				localctx = new NumericLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1523;
				this.number_();
				}
				break;
			case 6:
				{
				localctx = new BooleanLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1524;
				this.booleanValue();
				}
				break;
			case 7:
				{
				localctx = new StringLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1525;
				this.string_();
				}
				break;
			case 8:
				{
				localctx = new BinaryLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1526;
				this.match(SqlBaseParser.BINARY_LITERAL);
				}
				break;
			case 9:
				{
				localctx = new ParameterContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1527;
				this.match(SqlBaseParser.T__4);
				}
				break;
			case 10:
				{
				localctx = new PositionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1528;
				this.match(SqlBaseParser.POSITION);
				this.state = 1529;
				this.match(SqlBaseParser.T__1);
				this.state = 1530;
				this.valueExpression(0);
				this.state = 1531;
				this.match(SqlBaseParser.IN);
				this.state = 1532;
				this.valueExpression(0);
				this.state = 1533;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 11:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1535;
				this.match(SqlBaseParser.T__1);
				this.state = 1536;
				this.expression();
				this.state = 1539;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1537;
					this.match(SqlBaseParser.T__3);
					this.state = 1538;
					this.expression();
					}
					}
					this.state = 1541;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===4);
				this.state = 1543;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 12:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1545;
				this.match(SqlBaseParser.ROW);
				this.state = 1546;
				this.match(SqlBaseParser.T__1);
				this.state = 1547;
				this.expression();
				this.state = 1552;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1548;
					this.match(SqlBaseParser.T__3);
					this.state = 1549;
					this.expression();
					}
					}
					this.state = 1554;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1555;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 13:
				{
				localctx = new FunctionCallContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1557;
				this.qualifiedName();
				this.state = 1558;
				this.match(SqlBaseParser.T__1);
				this.state = 1559;
				this.match(SqlBaseParser.ASTERISK);
				this.state = 1560;
				this.match(SqlBaseParser.T__2);
				this.state = 1562;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 192, this._ctx) ) {
				case 1:
					{
					this.state = 1561;
					this.filter();
					}
					break;
				}
				this.state = 1565;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 193, this._ctx) ) {
				case 1:
					{
					this.state = 1564;
					this.over();
					}
					break;
				}
				}
				break;
			case 14:
				{
				localctx = new FunctionCallContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1567;
				this.qualifiedName();
				this.state = 1568;
				this.match(SqlBaseParser.T__1);
				this.state = 1580;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1406533391) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1570;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 194, this._ctx) ) {
					case 1:
						{
						this.state = 1569;
						this.setQuantifier();
						}
						break;
					}
					this.state = 1572;
					this.expression();
					this.state = 1577;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1573;
						this.match(SqlBaseParser.T__3);
						this.state = 1574;
						this.expression();
						}
						}
						this.state = 1579;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1592;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===141) {
					{
					this.state = 1582;
					this.match(SqlBaseParser.ORDER);
					this.state = 1583;
					this.match(SqlBaseParser.BY);
					this.state = 1584;
					this.sortItem();
					this.state = 1589;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1585;
						this.match(SqlBaseParser.T__3);
						this.state = 1586;
						this.sortItem();
						}
						}
						this.state = 1591;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1594;
				this.match(SqlBaseParser.T__2);
				this.state = 1596;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 199, this._ctx) ) {
				case 1:
					{
					this.state = 1595;
					this.filter();
					}
					break;
				}
				this.state = 1602;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 201, this._ctx) ) {
				case 1:
					{
					this.state = 1599;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===92 || _la===163) {
						{
						this.state = 1598;
						this.nullTreatment();
						}
					}

					this.state = 1601;
					this.over();
					}
					break;
				}
				}
				break;
			case 15:
				{
				localctx = new LambdaContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1604;
				this.identifier();
				this.state = 1605;
				this.match(SqlBaseParser.T__5);
				this.state = 1606;
				this.expression();
				}
				break;
			case 16:
				{
				localctx = new LambdaContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1608;
				this.match(SqlBaseParser.T__1);
				this.state = 1617;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1609;
					this.identifier();
					this.state = 1614;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1610;
						this.match(SqlBaseParser.T__3);
						this.state = 1611;
						this.identifier();
						}
						}
						this.state = 1616;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1619;
				this.match(SqlBaseParser.T__2);
				this.state = 1620;
				this.match(SqlBaseParser.T__5);
				this.state = 1621;
				this.expression();
				}
				break;
			case 17:
				{
				localctx = new SubqueryExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1622;
				this.match(SqlBaseParser.T__1);
				this.state = 1623;
				this.query();
				this.state = 1624;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 18:
				{
				localctx = new ExistsContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1626;
				this.match(SqlBaseParser.EXISTS);
				this.state = 1627;
				this.match(SqlBaseParser.T__1);
				this.state = 1628;
				this.query();
				this.state = 1629;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 19:
				{
				localctx = new SimpleCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1631;
				this.match(SqlBaseParser.CASE);
				this.state = 1632;
				this.valueExpression(0);
				this.state = 1634;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1633;
					this.whenClause();
					}
					}
					this.state = 1636;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1640;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1638;
					this.match(SqlBaseParser.ELSE);
					this.state = 1639;
					(localctx as SimpleCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1642;
				this.match(SqlBaseParser.END);
				}
				break;
			case 20:
				{
				localctx = new SearchedCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1644;
				this.match(SqlBaseParser.CASE);
				this.state = 1646;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1645;
					this.whenClause();
					}
					}
					this.state = 1648;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1652;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1650;
					this.match(SqlBaseParser.ELSE);
					this.state = 1651;
					(localctx as SearchedCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1654;
				this.match(SqlBaseParser.END);
				}
				break;
			case 21:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1656;
				this.match(SqlBaseParser.CAST);
				this.state = 1657;
				this.match(SqlBaseParser.T__1);
				this.state = 1658;
				this.expression();
				this.state = 1659;
				this.match(SqlBaseParser.AS);
				this.state = 1660;
				this.type_(0);
				this.state = 1661;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 22:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1663;
				this.match(SqlBaseParser.TRY_CAST);
				this.state = 1664;
				this.match(SqlBaseParser.T__1);
				this.state = 1665;
				this.expression();
				this.state = 1666;
				this.match(SqlBaseParser.AS);
				this.state = 1667;
				this.type_(0);
				this.state = 1668;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 23:
				{
				localctx = new ArrayConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1670;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1671;
				this.match(SqlBaseParser.T__6);
				this.state = 1680;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1672;
					this.expression();
					this.state = 1677;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1673;
						this.match(SqlBaseParser.T__3);
						this.state = 1674;
						this.expression();
						}
						}
						this.state = 1679;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1682;
				this.match(SqlBaseParser.T__7);
				}
				break;
			case 24:
				{
				localctx = new ColumnReferenceContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1683;
				this.identifier();
				}
				break;
			case 25:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1684;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_DATE);
				}
				break;
			case 26:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1685;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIME);
				this.state = 1689;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 210, this._ctx) ) {
				case 1:
					{
					this.state = 1686;
					this.match(SqlBaseParser.T__1);
					this.state = 1687;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1688;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 27:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1691;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIMESTAMP);
				this.state = 1695;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 211, this._ctx) ) {
				case 1:
					{
					this.state = 1692;
					this.match(SqlBaseParser.T__1);
					this.state = 1693;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1694;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 28:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1697;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIME);
				this.state = 1701;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 212, this._ctx) ) {
				case 1:
					{
					this.state = 1698;
					this.match(SqlBaseParser.T__1);
					this.state = 1699;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1700;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 29:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1703;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIMESTAMP);
				this.state = 1707;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 213, this._ctx) ) {
				case 1:
					{
					this.state = 1704;
					this.match(SqlBaseParser.T__1);
					this.state = 1705;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1706;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 30:
				{
				localctx = new CurrentUserContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1709;
				(localctx as CurrentUserContext)._name = this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 31:
				{
				localctx = new SubstringContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1710;
				this.match(SqlBaseParser.SUBSTRING);
				this.state = 1711;
				this.match(SqlBaseParser.T__1);
				this.state = 1712;
				this.valueExpression(0);
				this.state = 1713;
				this.match(SqlBaseParser.FROM);
				this.state = 1714;
				this.valueExpression(0);
				this.state = 1717;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===76) {
					{
					this.state = 1715;
					this.match(SqlBaseParser.FOR);
					this.state = 1716;
					this.valueExpression(0);
					}
				}

				this.state = 1719;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 32:
				{
				localctx = new NormalizeContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1721;
				this.match(SqlBaseParser.NORMALIZE);
				this.state = 1722;
				this.match(SqlBaseParser.T__1);
				this.state = 1723;
				this.valueExpression(0);
				this.state = 1726;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===4) {
					{
					this.state = 1724;
					this.match(SqlBaseParser.T__3);
					this.state = 1725;
					this.normalForm();
					}
				}

				this.state = 1728;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 33:
				{
				localctx = new ExtractContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1730;
				this.match(SqlBaseParser.EXTRACT);
				this.state = 1731;
				this.match(SqlBaseParser.T__1);
				this.state = 1732;
				this.identifier();
				this.state = 1733;
				this.match(SqlBaseParser.FROM);
				this.state = 1734;
				this.valueExpression(0);
				this.state = 1735;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				{
				localctx = new ParenthesizedExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1737;
				this.match(SqlBaseParser.T__1);
				this.state = 1738;
				this.expression();
				this.state = 1739;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 35:
				{
				localctx = new GroupingOperationContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1741;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1742;
				this.match(SqlBaseParser.T__1);
				this.state = 1751;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1743;
					this.qualifiedName();
					this.state = 1748;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1744;
						this.match(SqlBaseParser.T__3);
						this.state = 1745;
						this.qualifiedName();
						}
						}
						this.state = 1750;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1753;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1766;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 220, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1764;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 219, this._ctx) ) {
					case 1:
						{
						localctx = new SubscriptContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as SubscriptContext)._value = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1756;
						if (!(this.precpred(this._ctx, 14))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 14)");
						}
						this.state = 1757;
						this.match(SqlBaseParser.T__6);
						this.state = 1758;
						(localctx as SubscriptContext)._index = this.valueExpression(0);
						this.state = 1759;
						this.match(SqlBaseParser.T__7);
						}
						break;
					case 2:
						{
						localctx = new DereferenceContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as DereferenceContext)._base = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1761;
						if (!(this.precpred(this._ctx, 12))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 12)");
						}
						this.state = 1762;
						this.match(SqlBaseParser.T__0);
						this.state = 1763;
						(localctx as DereferenceContext)._fieldName = this.identifier();
						}
						break;
					}
					}
				}
				this.state = 1768;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 220, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}
	// @RuleVersion(0)
	public string_(): StringContext {
		let localctx: StringContext = new StringContext(this, this._ctx, this.state);
		this.enterRule(localctx, 110, SqlBaseParser.RULE_string);
		try {
			this.state = 1775;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 241:
				localctx = new BasicStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1769;
				this.match(SqlBaseParser.STRING);
				}
				break;
			case 242:
				localctx = new UnicodeStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1770;
				this.match(SqlBaseParser.UNICODE_STRING);
				this.state = 1773;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 221, this._ctx) ) {
				case 1:
					{
					this.state = 1771;
					this.match(SqlBaseParser.UESCAPE);
					this.state = 1772;
					this.match(SqlBaseParser.STRING);
					}
					break;
				}
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public nullTreatment(): NullTreatmentContext {
		let localctx: NullTreatmentContext = new NullTreatmentContext(this, this._ctx, this.state);
		this.enterRule(localctx, 112, SqlBaseParser.RULE_nullTreatment);
		try {
			this.state = 1781;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 92:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1777;
				this.match(SqlBaseParser.IGNORE);
				this.state = 1778;
				this.match(SqlBaseParser.NULLS);
				}
				break;
			case 163:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1779;
				this.match(SqlBaseParser.RESPECT);
				this.state = 1780;
				this.match(SqlBaseParser.NULLS);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public timeZoneSpecifier(): TimeZoneSpecifierContext {
		let localctx: TimeZoneSpecifierContext = new TimeZoneSpecifierContext(this, this._ctx, this.state);
		this.enterRule(localctx, 114, SqlBaseParser.RULE_timeZoneSpecifier);
		try {
			this.state = 1789;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 224, this._ctx) ) {
			case 1:
				localctx = new TimeZoneIntervalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1783;
				this.match(SqlBaseParser.TIME);
				this.state = 1784;
				this.match(SqlBaseParser.ZONE);
				this.state = 1785;
				this.interval();
				}
				break;
			case 2:
				localctx = new TimeZoneStringContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1786;
				this.match(SqlBaseParser.TIME);
				this.state = 1787;
				this.match(SqlBaseParser.ZONE);
				this.state = 1788;
				this.string_();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public comparisonOperator(): ComparisonOperatorContext {
		let localctx: ComparisonOperatorContext = new ComparisonOperatorContext(this, this._ctx, this.state);
		this.enterRule(localctx, 116, SqlBaseParser.RULE_comparisonOperator);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1791;
			_la = this._input.LA(1);
			if(!(((((_la - 229)) & ~0x1F) === 0 && ((1 << (_la - 229)) & 63) !== 0))) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public comparisonQuantifier(): ComparisonQuantifierContext {
		let localctx: ComparisonQuantifierContext = new ComparisonQuantifierContext(this, this._ctx, this.state);
		this.enterRule(localctx, 118, SqlBaseParser.RULE_comparisonQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1793;
			_la = this._input.LA(1);
			if(!(_la===12 || _la===16 || _la===185)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public booleanValue(): BooleanValueContext {
		let localctx: BooleanValueContext = new BooleanValueContext(this, this._ctx, this.state);
		this.enterRule(localctx, 120, SqlBaseParser.RULE_booleanValue);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1795;
			_la = this._input.LA(1);
			if(!(_la===71 || _la===203)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public interval(): IntervalContext {
		let localctx: IntervalContext = new IntervalContext(this, this._ctx, this.state);
		this.enterRule(localctx, 122, SqlBaseParser.RULE_interval);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1797;
			this.match(SqlBaseParser.INTERVAL);
			this.state = 1799;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===235 || _la===236) {
				{
				this.state = 1798;
				localctx._sign = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===235 || _la===236)) {
				    localctx._sign = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
			}

			this.state = 1801;
			this.string_();
			this.state = 1802;
			localctx._from_ = this.intervalField();
			this.state = 1805;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 226, this._ctx) ) {
			case 1:
				{
				this.state = 1803;
				this.match(SqlBaseParser.TO);
				this.state = 1804;
				localctx._to = this.intervalField();
				}
				break;
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public intervalField(): IntervalFieldContext {
		let localctx: IntervalFieldContext = new IntervalFieldContext(this, this._ctx, this.state);
		this.enterRule(localctx, 124, SqlBaseParser.RULE_intervalField);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1807;
			_la = this._input.LA(1);
			if(!(_la===48 || ((((_la - 90)) & ~0x1F) === 0 && ((1 << (_la - 90)) & 3221225473) !== 0) || _la===177 || _la===227)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public normalForm(): NormalFormContext {
		let localctx: NormalFormContext = new NormalFormContext(this, this._ctx, this.state);
		this.enterRule(localctx, 126, SqlBaseParser.RULE_normalForm);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1809;
			_la = this._input.LA(1);
			if(!(((((_la - 124)) & ~0x1F) === 0 && ((1 << (_la - 124)) & 15) !== 0))) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public types(): TypesContext {
		let localctx: TypesContext = new TypesContext(this, this._ctx, this.state);
		this.enterRule(localctx, 128, SqlBaseParser.RULE_types);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1811;
			this.match(SqlBaseParser.T__1);
			this.state = 1820;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 127) !== 0)) {
				{
				this.state = 1812;
				this.type_(0);
				this.state = 1817;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1813;
					this.match(SqlBaseParser.T__3);
					this.state = 1814;
					this.type_(0);
					}
					}
					this.state = 1819;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1822;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public type_(): TypeContext;
	public type_(_p: number): TypeContext;
	// @RuleVersion(0)
	public type_(_p?: number): TypeContext {
		if (_p === undefined) {
			_p = 0;
		}

		let _parentctx: ParserRuleContext = this._ctx;
		let _parentState: number = this.state;
		let localctx: TypeContext = new TypeContext(this, this._ctx, _parentState);
		let _prevctx: TypeContext = localctx;
		let _startState: number = 130;
		this.enterRecursionRule(localctx, 130, SqlBaseParser.RULE_type, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1871;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 232, this._ctx) ) {
			case 1:
				{
				this.state = 1825;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1826;
				this.match(SqlBaseParser.LT);
				this.state = 1827;
				this.type_(0);
				this.state = 1828;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 2:
				{
				this.state = 1830;
				this.match(SqlBaseParser.MAP);
				this.state = 1831;
				this.match(SqlBaseParser.LT);
				this.state = 1832;
				this.type_(0);
				this.state = 1833;
				this.match(SqlBaseParser.T__3);
				this.state = 1834;
				this.type_(0);
				this.state = 1835;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 3:
				{
				this.state = 1837;
				this.match(SqlBaseParser.ROW);
				this.state = 1838;
				this.match(SqlBaseParser.T__1);
				this.state = 1839;
				this.identifier();
				this.state = 1840;
				this.type_(0);
				this.state = 1847;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1841;
					this.match(SqlBaseParser.T__3);
					this.state = 1842;
					this.identifier();
					this.state = 1843;
					this.type_(0);
					}
					}
					this.state = 1849;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1850;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				{
				this.state = 1852;
				this.baseType();
				this.state = 1864;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 231, this._ctx) ) {
				case 1:
					{
					this.state = 1853;
					this.match(SqlBaseParser.T__1);
					this.state = 1854;
					this.typeParameter();
					this.state = 1859;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1855;
						this.match(SqlBaseParser.T__3);
						this.state = 1856;
						this.typeParameter();
						}
						}
						this.state = 1861;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 1862;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 5:
				{
				this.state = 1866;
				this.match(SqlBaseParser.INTERVAL);
				this.state = 1867;
				localctx._from_ = this.intervalField();
				this.state = 1868;
				this.match(SqlBaseParser.TO);
				this.state = 1869;
				localctx._to = this.intervalField();
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1877;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 233, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					{
					localctx = new TypeContext(this, _parentctx, _parentState);
					this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_type);
					this.state = 1873;
					if (!(this.precpred(this._ctx, 6))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 6)");
					}
					this.state = 1874;
					this.match(SqlBaseParser.ARRAY);
					}
					}
				}
				this.state = 1879;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 233, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.unrollRecursionContexts(_parentctx);
		}
		return localctx;
	}
	// @RuleVersion(0)
	public typeParameter(): TypeParameterContext {
		let localctx: TypeParameterContext = new TypeParameterContext(this, this._ctx, this.state);
		this.enterRule(localctx, 132, SqlBaseParser.RULE_typeParameter);
		try {
			this.state = 1882;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 244:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1880;
				this.match(SqlBaseParser.INTEGER_VALUE);
				}
				break;
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 42:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 68:
			case 70:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1881;
				this.type_(0);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public baseType(): BaseTypeContext {
		let localctx: BaseTypeContext = new BaseTypeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 134, SqlBaseParser.RULE_baseType);
		try {
			this.state = 1888;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 251:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1884;
				this.match(SqlBaseParser.TIME_WITH_TIME_ZONE);
				}
				break;
			case 252:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1885;
				this.match(SqlBaseParser.TIMESTAMP_WITH_TIME_ZONE);
				}
				break;
			case 253:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1886;
				this.match(SqlBaseParser.DOUBLE_PRECISION);
				}
				break;
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 42:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 68:
			case 70:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
			case 247:
			case 248:
			case 249:
			case 250:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1887;
				this.qualifiedName();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public whenClause(): WhenClauseContext {
		let localctx: WhenClauseContext = new WhenClauseContext(this, this._ctx, this.state);
		this.enterRule(localctx, 136, SqlBaseParser.RULE_whenClause);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1890;
			this.match(SqlBaseParser.WHEN);
			this.state = 1891;
			localctx._condition = this.expression();
			this.state = 1892;
			this.match(SqlBaseParser.THEN);
			this.state = 1893;
			localctx._result = this.expression();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public filter(): FilterContext {
		let localctx: FilterContext = new FilterContext(this, this._ctx, this.state);
		this.enterRule(localctx, 138, SqlBaseParser.RULE_filter);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1895;
			this.match(SqlBaseParser.FILTER);
			this.state = 1896;
			this.match(SqlBaseParser.T__1);
			this.state = 1897;
			this.match(SqlBaseParser.WHERE);
			this.state = 1898;
			this.booleanExpression(0);
			this.state = 1899;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public over(): OverContext {
		let localctx: OverContext = new OverContext(this, this._ctx, this.state);
		this.enterRule(localctx, 140, SqlBaseParser.RULE_over);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1901;
			this.match(SqlBaseParser.OVER);
			this.state = 1902;
			this.match(SqlBaseParser.T__1);
			this.state = 1913;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===146) {
				{
				this.state = 1903;
				this.match(SqlBaseParser.PARTITION);
				this.state = 1904;
				this.match(SqlBaseParser.BY);
				this.state = 1905;
				localctx._expression = this.expression();
				localctx._partition.push(localctx._expression);
				this.state = 1910;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1906;
					this.match(SqlBaseParser.T__3);
					this.state = 1907;
					localctx._expression = this.expression();
					localctx._partition.push(localctx._expression);
					}
					}
					this.state = 1912;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1925;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1915;
				this.match(SqlBaseParser.ORDER);
				this.state = 1916;
				this.match(SqlBaseParser.BY);
				this.state = 1917;
				this.sortItem();
				this.state = 1922;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1918;
					this.match(SqlBaseParser.T__3);
					this.state = 1919;
					this.sortItem();
					}
					}
					this.state = 1924;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1928;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===88 || _la===154 || _la===174) {
				{
				this.state = 1927;
				this.windowFrame();
				}
			}

			this.state = 1930;
			this.match(SqlBaseParser.T__2);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public windowFrame(): WindowFrameContext {
		let localctx: WindowFrameContext = new WindowFrameContext(this, this._ctx, this.state);
		this.enterRule(localctx, 142, SqlBaseParser.RULE_windowFrame);
		try {
			this.state = 1956;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 241, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1932;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1933;
				localctx._start = this.frameBound();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1934;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1935;
				localctx._start = this.frameBound();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1936;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1937;
				localctx._start = this.frameBound();
				}
				break;
			case 4:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1938;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1939;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1940;
				localctx._start = this.frameBound();
				this.state = 1941;
				this.match(SqlBaseParser.AND);
				this.state = 1942;
				localctx._end = this.frameBound();
				}
				break;
			case 5:
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1944;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1945;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1946;
				localctx._start = this.frameBound();
				this.state = 1947;
				this.match(SqlBaseParser.AND);
				this.state = 1948;
				localctx._end = this.frameBound();
				}
				break;
			case 6:
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1950;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1951;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1952;
				localctx._start = this.frameBound();
				this.state = 1953;
				this.match(SqlBaseParser.AND);
				this.state = 1954;
				localctx._end = this.frameBound();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public frameBound(): FrameBoundContext {
		let localctx: FrameBoundContext = new FrameBoundContext(this, this._ctx, this.state);
		this.enterRule(localctx, 144, SqlBaseParser.RULE_frameBound);
		let _la: number;
		try {
			this.state = 1967;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 242, this._ctx) ) {
			case 1:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1958;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1959;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.PRECEDING);
				}
				break;
			case 2:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1960;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1961;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.FOLLOWING);
				}
				break;
			case 3:
				localctx = new CurrentRowBoundContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1962;
				this.match(SqlBaseParser.CURRENT);
				this.state = 1963;
				this.match(SqlBaseParser.ROW);
				}
				break;
			case 4:
				localctx = new BoundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1964;
				this.expression();
				this.state = 1965;
				(localctx as BoundedFrameContext)._boundType = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===75 || _la===149)) {
				    (localctx as BoundedFrameContext)._boundType = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public updateAssignment(): UpdateAssignmentContext {
		let localctx: UpdateAssignmentContext = new UpdateAssignmentContext(this, this._ctx, this.state);
		this.enterRule(localctx, 146, SqlBaseParser.RULE_updateAssignment);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1969;
			this.identifier();
			this.state = 1970;
			this.match(SqlBaseParser.EQ);
			this.state = 1971;
			this.expression();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public explainOption(): ExplainOptionContext {
		let localctx: ExplainOptionContext = new ExplainOptionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 148, SqlBaseParser.RULE_explainOption);
		let _la: number;
		try {
			this.state = 1977;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 77:
				localctx = new ExplainFormatContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1973;
				this.match(SqlBaseParser.FORMAT);
				this.state = 1974;
				(localctx as ExplainFormatContext)._value = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===85 || _la===105 || _la===197)) {
				    (localctx as ExplainFormatContext)._value = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
				break;
			case 206:
				localctx = new ExplainTypeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1975;
				this.match(SqlBaseParser.TYPE);
				this.state = 1976;
				(localctx as ExplainTypeContext)._value = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===57 || _la===102 || _la===117 || _la===217)) {
				    (localctx as ExplainTypeContext)._value = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public transactionMode(): TransactionModeContext {
		let localctx: TransactionModeContext = new TransactionModeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 150, SqlBaseParser.RULE_transactionMode);
		let _la: number;
		try {
			this.state = 1984;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 104:
				localctx = new IsolationLevelContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1979;
				this.match(SqlBaseParser.ISOLATION);
				this.state = 1980;
				this.match(SqlBaseParser.LEVEL);
				this.state = 1981;
				this.levelOfIsolation();
				}
				break;
			case 155:
				localctx = new TransactionAccessModeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1982;
				this.match(SqlBaseParser.READ);
				this.state = 1983;
				(localctx as TransactionAccessModeContext)._accessMode = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===138 || _la===226)) {
				    (localctx as TransactionAccessModeContext)._accessMode = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public levelOfIsolation(): LevelOfIsolationContext {
		let localctx: LevelOfIsolationContext = new LevelOfIsolationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 152, SqlBaseParser.RULE_levelOfIsolation);
		try {
			this.state = 1993;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 245, this._ctx) ) {
			case 1:
				localctx = new ReadUncommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1986;
				this.match(SqlBaseParser.READ);
				this.state = 1987;
				this.match(SqlBaseParser.UNCOMMITTED);
				}
				break;
			case 2:
				localctx = new ReadCommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1988;
				this.match(SqlBaseParser.READ);
				this.state = 1989;
				this.match(SqlBaseParser.COMMITTED);
				}
				break;
			case 3:
				localctx = new RepeatableReadContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1990;
				this.match(SqlBaseParser.REPEATABLE);
				this.state = 1991;
				this.match(SqlBaseParser.READ);
				}
				break;
			case 4:
				localctx = new SerializableContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1992;
				this.match(SqlBaseParser.SERIALIZABLE);
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public callArgument(): CallArgumentContext {
		let localctx: CallArgumentContext = new CallArgumentContext(this, this._ctx, this.state);
		this.enterRule(localctx, 154, SqlBaseParser.RULE_callArgument);
		try {
			this.state = 2000;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 246, this._ctx) ) {
			case 1:
				localctx = new PositionalArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1995;
				this.expression();
				}
				break;
			case 2:
				localctx = new NamedArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1996;
				this.identifier();
				this.state = 1997;
				this.match(SqlBaseParser.T__8);
				this.state = 1998;
				this.expression();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public privilege(): PrivilegeContext {
		let localctx: PrivilegeContext = new PrivilegeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 156, SqlBaseParser.RULE_privilege);
		try {
			this.state = 2006;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2002;
				this.match(SqlBaseParser.SELECT);
				}
				break;
			case 51:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2003;
				this.match(SqlBaseParser.DELETE);
				}
				break;
			case 97:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2004;
				this.match(SqlBaseParser.INSERT);
				}
				break;
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 42:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 68:
			case 70:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
			case 247:
			case 248:
			case 249:
			case 250:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 2005;
				this.identifier();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public qualifiedName(): QualifiedNameContext {
		let localctx: QualifiedNameContext = new QualifiedNameContext(this, this._ctx, this.state);
		this.enterRule(localctx, 158, SqlBaseParser.RULE_qualifiedName);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2008;
			this.identifier();
			this.state = 2013;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 248, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 2009;
					this.match(SqlBaseParser.T__0);
					this.state = 2010;
					this.identifier();
					}
					}
				}
				this.state = 2015;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 248, this._ctx);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public tableVersionExpression(): TableVersionExpressionContext {
		let localctx: TableVersionExpressionContext = new TableVersionExpressionContext(this, this._ctx, this.state);
		this.enterRule(localctx, 160, SqlBaseParser.RULE_tableVersionExpression);
		let _la: number;
		try {
			localctx = new TableVersionContext(this, localctx);
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2016;
			this.match(SqlBaseParser.FOR);
			this.state = 2017;
			(localctx as TableVersionContext)._tableVersionType = this._input.LT(1);
			_la = this._input.LA(1);
			if(!(((((_la - 191)) & ~0x1F) === 0 && ((1 << (_la - 191)) & 536871427) !== 0))) {
			    (localctx as TableVersionContext)._tableVersionType = this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			this.state = 2018;
			this.tableVersionState();
			this.state = 2019;
			this.valueExpression(0);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public tableVersionState(): TableVersionStateContext {
		let localctx: TableVersionStateContext = new TableVersionStateContext(this, this._ctx, this.state);
		this.enterRule(localctx, 162, SqlBaseParser.RULE_tableVersionState);
		try {
			this.state = 2024;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 18:
				localctx = new TableversionasofContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2021;
				this.match(SqlBaseParser.AS);
				this.state = 2022;
				this.match(SqlBaseParser.OF);
				}
				break;
			case 21:
				localctx = new TableversionbeforeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2023;
				this.match(SqlBaseParser.BEFORE);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public grantor(): GrantorContext {
		let localctx: GrantorContext = new GrantorContext(this, this._ctx, this.state);
		this.enterRule(localctx, 164, SqlBaseParser.RULE_grantor);
		try {
			this.state = 2029;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 250, this._ctx) ) {
			case 1:
				localctx = new CurrentUserGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2026;
				this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 2:
				localctx = new CurrentRoleGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2027;
				this.match(SqlBaseParser.CURRENT_ROLE);
				}
				break;
			case 3:
				localctx = new SpecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2028;
				this.principal();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public principal(): PrincipalContext {
		let localctx: PrincipalContext = new PrincipalContext(this, this._ctx, this.state);
		this.enterRule(localctx, 166, SqlBaseParser.RULE_principal);
		try {
			this.state = 2036;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 251, this._ctx) ) {
			case 1:
				localctx = new UserPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2031;
				this.match(SqlBaseParser.USER);
				this.state = 2032;
				this.identifier();
				}
				break;
			case 2:
				localctx = new RolePrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2033;
				this.match(SqlBaseParser.ROLE);
				this.state = 2034;
				this.identifier();
				}
				break;
			case 3:
				localctx = new UnspecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2035;
				this.identifier();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public roles(): RolesContext {
		let localctx: RolesContext = new RolesContext(this, this._ctx, this.state);
		this.enterRule(localctx, 168, SqlBaseParser.RULE_roles);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2038;
			this.identifier();
			this.state = 2043;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 2039;
				this.match(SqlBaseParser.T__3);
				this.state = 2040;
				this.identifier();
				}
				}
				this.state = 2045;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public identifier(): IdentifierContext {
		let localctx: IdentifierContext = new IdentifierContext(this, this._ctx, this.state);
		this.enterRule(localctx, 170, SqlBaseParser.RULE_identifier);
		try {
			this.state = 2051;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 247:
				localctx = new UnquotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2046;
				this.match(SqlBaseParser.IDENTIFIER);
				}
				break;
			case 249:
				localctx = new QuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2047;
				this.match(SqlBaseParser.QUOTED_IDENTIFIER);
				}
				break;
			case 10:
			case 11:
			case 12:
			case 14:
			case 16:
			case 17:
			case 19:
			case 20:
			case 21:
			case 22:
			case 25:
			case 26:
			case 27:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 40:
			case 42:
			case 46:
			case 47:
			case 48:
			case 50:
			case 52:
			case 54:
			case 55:
			case 57:
			case 60:
			case 62:
			case 65:
			case 68:
			case 70:
			case 72:
			case 73:
			case 74:
			case 75:
			case 77:
			case 80:
			case 81:
			case 82:
			case 83:
			case 84:
			case 85:
			case 88:
			case 90:
			case 91:
			case 92:
			case 94:
			case 96:
			case 99:
			case 101:
			case 102:
			case 104:
			case 105:
			case 107:
			case 108:
			case 109:
			case 110:
			case 112:
			case 114:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 124:
			case 125:
			case 126:
			case 127:
			case 128:
			case 129:
			case 133:
			case 134:
			case 135:
			case 136:
			case 138:
			case 139:
			case 142:
			case 144:
			case 145:
			case 146:
			case 147:
			case 148:
			case 149:
			case 151:
			case 152:
			case 153:
			case 154:
			case 155:
			case 157:
			case 158:
			case 159:
			case 160:
			case 161:
			case 162:
			case 163:
			case 164:
			case 165:
			case 166:
			case 167:
			case 169:
			case 170:
			case 171:
			case 173:
			case 174:
			case 175:
			case 176:
			case 177:
			case 178:
			case 180:
			case 181:
			case 182:
			case 183:
			case 184:
			case 185:
			case 186:
			case 187:
			case 188:
			case 189:
			case 190:
			case 191:
			case 192:
			case 194:
			case 195:
			case 196:
			case 197:
			case 199:
			case 200:
			case 201:
			case 202:
			case 204:
			case 205:
			case 206:
			case 208:
			case 209:
			case 211:
			case 213:
			case 214:
			case 215:
			case 217:
			case 219:
			case 220:
			case 221:
			case 225:
			case 226:
			case 227:
			case 228:
				localctx = new UnquotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2048;
				this.nonReserved();
				}
				break;
			case 250:
				localctx = new BackQuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 2049;
				this.match(SqlBaseParser.BACKQUOTED_IDENTIFIER);
				}
				break;
			case 248:
				localctx = new DigitIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 2050;
				this.match(SqlBaseParser.DIGIT_IDENTIFIER);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public number_(): NumberContext {
		let localctx: NumberContext = new NumberContext(this, this._ctx, this.state);
		this.enterRule(localctx, 172, SqlBaseParser.RULE_number);
		try {
			this.state = 2056;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 245:
				localctx = new DecimalLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2053;
				this.match(SqlBaseParser.DECIMAL_VALUE);
				}
				break;
			case 246:
				localctx = new DoubleLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2054;
				this.match(SqlBaseParser.DOUBLE_VALUE);
				}
				break;
			case 244:
				localctx = new IntegerLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2055;
				this.match(SqlBaseParser.INTEGER_VALUE);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintSpecification(): ConstraintSpecificationContext {
		let localctx: ConstraintSpecificationContext = new ConstraintSpecificationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 174, SqlBaseParser.RULE_constraintSpecification);
		try {
			this.state = 2060;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 36:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2058;
				this.namedConstraintSpecification();
				}
				break;
			case 151:
			case 211:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2059;
				this.unnamedConstraintSpecification();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public namedConstraintSpecification(): NamedConstraintSpecificationContext {
		let localctx: NamedConstraintSpecificationContext = new NamedConstraintSpecificationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 176, SqlBaseParser.RULE_namedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2062;
			this.match(SqlBaseParser.CONSTRAINT);
			this.state = 2063;
			localctx._name = this.identifier();
			this.state = 2064;
			this.unnamedConstraintSpecification();
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public unnamedConstraintSpecification(): UnnamedConstraintSpecificationContext {
		let localctx: UnnamedConstraintSpecificationContext = new UnnamedConstraintSpecificationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 178, SqlBaseParser.RULE_unnamedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2066;
			this.constraintType();
			this.state = 2067;
			this.columnAliases();
			this.state = 2069;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 256, this._ctx) ) {
			case 1:
				{
				this.state = 2068;
				this.constraintQualifiers();
				}
				break;
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintType(): ConstraintTypeContext {
		let localctx: ConstraintTypeContext = new ConstraintTypeContext(this, this._ctx, this.state);
		this.enterRule(localctx, 180, SqlBaseParser.RULE_constraintType);
		try {
			this.state = 2074;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 211:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2071;
				this.match(SqlBaseParser.UNIQUE);
				}
				break;
			case 151:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2072;
				this.match(SqlBaseParser.PRIMARY);
				this.state = 2073;
				this.match(SqlBaseParser.KEY);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintQualifiers(): ConstraintQualifiersContext {
		let localctx: ConstraintQualifiersContext = new ConstraintQualifiersContext(this, this._ctx, this.state);
		this.enterRule(localctx, 182, SqlBaseParser.RULE_constraintQualifiers);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2079;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (((((_la - 55)) & ~0x1F) === 0 && ((1 << (_la - 55)) & 161) !== 0) || _la===131 || _la===158) {
				{
				{
				this.state = 2076;
				this.constraintQualifier();
				}
				}
				this.state = 2081;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintQualifier(): ConstraintQualifierContext {
		let localctx: ConstraintQualifierContext = new ConstraintQualifierContext(this, this._ctx, this.state);
		this.enterRule(localctx, 184, SqlBaseParser.RULE_constraintQualifier);
		try {
			this.state = 2085;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 259, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2082;
				this.constraintEnabled();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2083;
				this.constraintRely();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2084;
				this.constraintEnforced();
				}
				break;
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintRely(): ConstraintRelyContext {
		let localctx: ConstraintRelyContext = new ConstraintRelyContext(this, this._ctx, this.state);
		this.enterRule(localctx, 186, SqlBaseParser.RULE_constraintRely);
		try {
			this.state = 2090;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 158:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2087;
				this.match(SqlBaseParser.RELY);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2088;
				this.match(SqlBaseParser.NOT);
				this.state = 2089;
				this.match(SqlBaseParser.RELY);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintEnabled(): ConstraintEnabledContext {
		let localctx: ConstraintEnabledContext = new ConstraintEnabledContext(this, this._ctx, this.state);
		this.enterRule(localctx, 188, SqlBaseParser.RULE_constraintEnabled);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2092;
			_la = this._input.LA(1);
			if(!(_la===55 || _la===60)) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public constraintEnforced(): ConstraintEnforcedContext {
		let localctx: ConstraintEnforcedContext = new ConstraintEnforcedContext(this, this._ctx, this.state);
		this.enterRule(localctx, 190, SqlBaseParser.RULE_constraintEnforced);
		try {
			this.state = 2097;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 62:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2094;
				this.match(SqlBaseParser.ENFORCED);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2095;
				this.match(SqlBaseParser.NOT);
				this.state = 2096;
				this.match(SqlBaseParser.ENFORCED);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}
	// @RuleVersion(0)
	public nonReserved(): NonReservedContext {
		let localctx: NonReservedContext = new NonReservedContext(this, this._ctx, this.state);
		this.enterRule(localctx, 192, SqlBaseParser.RULE_nonReserved);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2099;
			_la = this._input.LA(1);
			if(!((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0))) {
			this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			}
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
			} else {
				throw re;
			}
		}
		finally {
			this.exitRule();
		}
		return localctx;
	}

	public sempred(localctx: RuleContext, ruleIndex: number, predIndex: number): boolean {
		switch (ruleIndex) {
		case 30:
			return this.queryTerm_sempred(localctx as QueryTermContext, predIndex);
		case 42:
			return this.relation_sempred(localctx as RelationContext, predIndex);
		case 51:
			return this.booleanExpression_sempred(localctx as BooleanExpressionContext, predIndex);
		case 53:
			return this.valueExpression_sempred(localctx as ValueExpressionContext, predIndex);
		case 54:
			return this.primaryExpression_sempred(localctx as PrimaryExpressionContext, predIndex);
		case 65:
			return this.type_sempred(localctx as TypeContext, predIndex);
		}
		return true;
	}
	private queryTerm_sempred(localctx: QueryTermContext, predIndex: number): boolean {
		switch (predIndex) {
		case 0:
			return this.precpred(this._ctx, 2);
		case 1:
			return this.precpred(this._ctx, 1);
		}
		return true;
	}
	private relation_sempred(localctx: RelationContext, predIndex: number): boolean {
		switch (predIndex) {
		case 2:
			return this.precpred(this._ctx, 2);
		}
		return true;
	}
	private booleanExpression_sempred(localctx: BooleanExpressionContext, predIndex: number): boolean {
		switch (predIndex) {
		case 3:
			return this.precpred(this._ctx, 2);
		case 4:
			return this.precpred(this._ctx, 1);
		}
		return true;
	}
	private valueExpression_sempred(localctx: ValueExpressionContext, predIndex: number): boolean {
		switch (predIndex) {
		case 5:
			return this.precpred(this._ctx, 3);
		case 6:
			return this.precpred(this._ctx, 2);
		case 7:
			return this.precpred(this._ctx, 1);
		case 8:
			return this.precpred(this._ctx, 5);
		}
		return true;
	}
	private primaryExpression_sempred(localctx: PrimaryExpressionContext, predIndex: number): boolean {
		switch (predIndex) {
		case 9:
			return this.precpred(this._ctx, 14);
		case 10:
			return this.precpred(this._ctx, 12);
		}
		return true;
	}
	private type_sempred(localctx: TypeContext, predIndex: number): boolean {
		switch (predIndex) {
		case 11:
			return this.precpred(this._ctx, 6);
		}
		return true;
	}

	public static readonly _serializedATN: number[] = [4,1,258,2102,2,0,7,0,
	2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,7,7,7,2,8,7,8,2,9,7,9,
	2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,14,2,15,7,15,2,16,7,16,2,
	17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,21,2,22,7,22,2,23,7,23,2,24,
	7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,28,2,29,7,29,2,30,7,30,2,31,7,
	31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,7,35,2,36,7,36,2,37,7,37,2,38,7,38,
	2,39,7,39,2,40,7,40,2,41,7,41,2,42,7,42,2,43,7,43,2,44,7,44,2,45,7,45,2,
	46,7,46,2,47,7,47,2,48,7,48,2,49,7,49,2,50,7,50,2,51,7,51,2,52,7,52,2,53,
	7,53,2,54,7,54,2,55,7,55,2,56,7,56,2,57,7,57,2,58,7,58,2,59,7,59,2,60,7,
	60,2,61,7,61,2,62,7,62,2,63,7,63,2,64,7,64,2,65,7,65,2,66,7,66,2,67,7,67,
	2,68,7,68,2,69,7,69,2,70,7,70,2,71,7,71,2,72,7,72,2,73,7,73,2,74,7,74,2,
	75,7,75,2,76,7,76,2,77,7,77,2,78,7,78,2,79,7,79,2,80,7,80,2,81,7,81,2,82,
	7,82,2,83,7,83,2,84,7,84,2,85,7,85,2,86,7,86,2,87,7,87,2,88,7,88,2,89,7,
	89,2,90,7,90,2,91,7,91,2,92,7,92,2,93,7,93,2,94,7,94,2,95,7,95,2,96,7,96,
	1,0,1,0,1,0,1,1,1,1,1,1,1,2,1,2,1,2,1,3,1,3,1,3,1,4,1,4,1,4,1,5,1,5,1,5,
	1,6,1,6,1,6,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,3,8,232,8,8,1,8,1,8,1,8,3,8,237,8,8,1,8,1,8,1,8,1,8,3,8,243,8,8,1,8,
	1,8,3,8,247,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,261,
	8,8,1,8,1,8,3,8,265,8,8,1,8,1,8,3,8,269,8,8,1,8,1,8,3,8,273,8,8,1,8,1,8,
	1,8,1,8,1,8,1,8,3,8,281,8,8,1,8,1,8,3,8,285,8,8,1,8,3,8,288,8,8,1,8,1,8,
	1,8,1,8,1,8,3,8,295,8,8,1,8,1,8,1,8,1,8,1,8,5,8,302,8,8,10,8,12,8,305,9,
	8,1,8,1,8,1,8,3,8,310,8,8,1,8,1,8,3,8,314,8,8,1,8,1,8,1,8,1,8,3,8,320,8,
	8,1,8,1,8,1,8,1,8,1,8,3,8,327,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,336,8,
	8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,345,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,3,8,356,8,8,1,8,1,8,1,8,1,8,1,8,3,8,363,8,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,1,8,3,8,373,8,8,1,8,1,8,1,8,1,8,1,8,3,8,380,8,8,1,8,1,8,1,8,1,8,1,
	8,1,8,3,8,388,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,396,8,8,1,8,1,8,1,8,1,8,1,
	8,1,8,3,8,404,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,414,8,8,1,8,1,8,1,
	8,1,8,1,8,3,8,421,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,429,8,8,1,8,1,8,1,8,3,
	8,434,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,445,8,8,1,8,1,8,1,8,3,
	8,450,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,461,8,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,1,8,1,8,3,8,472,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,482,
	8,8,10,8,12,8,485,9,8,1,8,1,8,1,8,3,8,490,8,8,1,8,1,8,1,8,3,8,495,8,8,1,
	8,1,8,1,8,1,8,3,8,501,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,510,8,8,1,8,1,
	8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,521,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,
	8,530,8,8,1,8,1,8,1,8,3,8,535,8,8,1,8,1,8,3,8,539,8,8,1,8,1,8,1,8,1,8,1,
	8,1,8,3,8,547,8,8,1,8,1,8,1,8,1,8,1,8,3,8,554,8,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,1,8,1,8,3,8,567,8,8,1,8,3,8,570,8,8,1,8,1,8,1,8,1,8,1,8,1,
	8,5,8,578,8,8,10,8,12,8,581,9,8,3,8,583,8,8,1,8,1,8,1,8,1,8,1,8,3,8,590,
	8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,599,8,8,1,8,1,8,1,8,1,8,3,8,605,8,8,
	1,8,1,8,1,8,3,8,610,8,8,1,8,1,8,3,8,614,8,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,
	622,8,8,10,8,12,8,625,9,8,3,8,627,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,
	8,637,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,648,8,8,10,8,12,8,651,
	9,8,1,8,1,8,1,8,3,8,656,8,8,1,8,1,8,1,8,3,8,661,8,8,1,8,1,8,1,8,1,8,3,8,
	667,8,8,1,8,1,8,1,8,1,8,1,8,5,8,674,8,8,10,8,12,8,677,9,8,1,8,1,8,1,8,3,
	8,682,8,8,1,8,1,8,1,8,1,8,1,8,3,8,689,8,8,1,8,1,8,1,8,1,8,5,8,695,8,8,10,
	8,12,8,698,9,8,1,8,1,8,3,8,702,8,8,1,8,1,8,3,8,706,8,8,1,8,1,8,1,8,1,8,
	1,8,1,8,3,8,714,8,8,1,8,1,8,1,8,1,8,3,8,720,8,8,1,8,1,8,1,8,5,8,725,8,8,
	10,8,12,8,728,9,8,1,8,1,8,3,8,732,8,8,1,8,1,8,3,8,736,8,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,1,8,3,8,746,8,8,1,8,3,8,749,8,8,1,8,1,8,3,8,753,8,8,1,8,3,
	8,756,8,8,1,8,1,8,1,8,1,8,5,8,762,8,8,10,8,12,8,765,9,8,1,8,1,8,3,8,769,
	8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,3,8,794,8,8,1,8,1,8,1,8,1,8,3,8,800,8,8,1,8,1,8,
	1,8,1,8,3,8,806,8,8,3,8,808,8,8,1,8,1,8,1,8,1,8,3,8,814,8,8,1,8,1,8,1,8,
	1,8,3,8,820,8,8,3,8,822,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,830,8,8,3,8,832,
	8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	3,8,851,8,8,1,8,1,8,1,8,3,8,856,8,8,1,8,1,8,1,8,1,8,1,8,3,8,863,8,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,875,8,8,3,8,877,8,8,1,8,1,8,1,8,
	1,8,1,8,1,8,3,8,885,8,8,3,8,887,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,5,8,903,8,8,10,8,12,8,906,9,8,3,8,908,8,8,1,8,1,8,3,
	8,912,8,8,1,8,1,8,3,8,916,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,5,8,932,8,8,10,8,12,8,935,9,8,3,8,937,8,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,951,8,8,10,8,12,8,954,9,8,1,8,1,8,3,
	8,958,8,8,3,8,960,8,8,1,9,3,9,963,8,9,1,9,1,9,1,10,1,10,3,10,969,8,10,1,
	10,1,10,1,10,5,10,974,8,10,10,10,12,10,977,9,10,1,11,1,11,1,11,3,11,982,
	8,11,1,12,1,12,1,12,1,12,3,12,988,8,12,1,12,1,12,3,12,992,8,12,1,12,1,12,
	3,12,996,8,12,1,13,1,13,1,13,1,13,3,13,1002,8,13,1,14,1,14,1,14,1,14,5,
	14,1008,8,14,10,14,12,14,1011,9,14,1,14,1,14,1,15,1,15,1,15,1,15,1,16,1,
	16,1,16,1,17,5,17,1023,8,17,10,17,12,17,1026,9,17,1,18,1,18,1,18,1,18,3,
	18,1032,8,18,1,19,5,19,1035,8,19,10,19,12,19,1038,9,19,1,20,1,20,1,21,1,
	21,3,21,1044,8,21,1,22,1,22,1,22,1,23,1,23,1,23,3,23,1052,8,23,1,24,1,24,
	3,24,1056,8,24,1,25,1,25,1,25,3,25,1061,8,25,1,26,1,26,1,26,1,26,1,26,1,
	26,1,26,1,26,1,26,3,26,1072,8,26,1,27,1,27,1,28,1,28,1,28,1,28,3,28,1080,
	8,28,1,28,1,28,1,28,3,28,1085,8,28,3,28,1087,8,28,1,28,1,28,1,28,1,28,1,
	28,1,28,1,28,3,28,1096,8,28,3,28,1098,8,28,1,29,1,29,1,29,5,29,1103,8,29,
	10,29,12,29,1106,9,29,1,30,1,30,1,30,1,30,1,30,1,30,3,30,1114,8,30,1,30,
	1,30,1,30,1,30,3,30,1120,8,30,1,30,5,30,1123,8,30,10,30,12,30,1126,9,30,
	1,31,1,31,1,31,1,31,1,31,1,31,1,31,5,31,1135,8,31,10,31,12,31,1138,9,31,
	1,31,1,31,1,31,1,31,3,31,1144,8,31,1,32,1,32,3,32,1148,8,32,1,32,1,32,3,
	32,1152,8,32,1,33,1,33,3,33,1156,8,33,1,33,1,33,1,33,3,33,1161,8,33,1,33,
	1,33,3,33,1165,8,33,1,33,1,33,1,33,3,33,1170,8,33,1,33,1,33,3,33,1174,8,
	33,1,34,1,34,1,34,5,34,1179,8,34,10,34,12,34,1182,9,34,1,35,1,35,1,35,5,
	35,1187,8,35,10,35,12,35,1190,9,35,1,36,3,36,1193,8,36,1,36,1,36,1,36,5,
	36,1198,8,36,10,36,12,36,1201,9,36,1,37,1,37,1,37,1,37,1,37,1,37,5,37,1209,
	8,37,10,37,12,37,1212,9,37,3,37,1214,8,37,1,37,1,37,1,37,1,37,1,37,1,37,
	5,37,1222,8,37,10,37,12,37,1225,9,37,3,37,1227,8,37,1,37,1,37,1,37,1,37,
	1,37,1,37,1,37,5,37,1236,8,37,10,37,12,37,1239,9,37,1,37,1,37,3,37,1243,
	8,37,1,38,1,38,1,38,1,38,5,38,1249,8,38,10,38,12,38,1252,9,38,3,38,1254,
	8,38,1,38,1,38,3,38,1258,8,38,1,39,1,39,3,39,1262,8,39,1,39,1,39,1,39,1,
	39,1,39,1,40,1,40,1,41,1,41,3,41,1273,8,41,1,41,3,41,1276,8,41,1,41,1,41,
	1,41,1,41,1,41,3,41,1283,8,41,1,42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,1,
	42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,3,42,1302,8,42,5,42,1304,8,42,
	10,42,12,42,1307,9,42,1,43,3,43,1310,8,43,1,43,1,43,3,43,1314,8,43,1,43,
	1,43,3,43,1318,8,43,1,43,1,43,3,43,1322,8,43,3,43,1324,8,43,1,44,1,44,1,
	44,1,44,1,44,1,44,1,44,5,44,1333,8,44,10,44,12,44,1336,9,44,1,44,1,44,3,
	44,1340,8,44,1,45,1,45,1,45,1,45,1,45,1,45,1,45,3,45,1349,8,45,1,46,1,46,
	1,47,1,47,3,47,1355,8,47,1,47,1,47,3,47,1359,8,47,3,47,1361,8,47,1,48,1,
	48,1,48,1,48,5,48,1367,8,48,10,48,12,48,1370,9,48,1,48,1,48,1,49,1,49,3,
	49,1376,8,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,5,49,1387,8,49,
	10,49,12,49,1390,9,49,1,49,1,49,1,49,3,49,1395,8,49,1,49,1,49,1,49,1,49,
	1,49,1,49,1,49,1,49,1,49,3,49,1406,8,49,1,50,1,50,1,51,1,51,1,51,3,51,1413,
	8,51,1,51,1,51,3,51,1417,8,51,1,51,1,51,1,51,1,51,1,51,1,51,5,51,1425,8,
	51,10,51,12,51,1428,9,51,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,
	52,3,52,1440,8,52,1,52,1,52,1,52,1,52,1,52,1,52,3,52,1448,8,52,1,52,1,52,
	1,52,1,52,1,52,5,52,1455,8,52,10,52,12,52,1458,9,52,1,52,1,52,1,52,3,52,
	1463,8,52,1,52,1,52,1,52,1,52,1,52,1,52,3,52,1471,8,52,1,52,1,52,1,52,1,
	52,3,52,1477,8,52,1,52,1,52,3,52,1481,8,52,1,52,1,52,1,52,3,52,1486,8,52,
	1,52,1,52,1,52,3,52,1491,8,52,1,53,1,53,1,53,1,53,3,53,1497,8,53,1,53,1,
	53,1,53,1,53,1,53,1,53,1,53,1,53,1,53,1,53,1,53,1,53,5,53,1511,8,53,10,
	53,12,53,1514,9,53,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,
	54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,4,54,
	1540,8,54,11,54,12,54,1541,1,54,1,54,1,54,1,54,1,54,1,54,1,54,5,54,1551,
	8,54,10,54,12,54,1554,9,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,3,54,1563,
	8,54,1,54,3,54,1566,8,54,1,54,1,54,1,54,3,54,1571,8,54,1,54,1,54,1,54,5,
	54,1576,8,54,10,54,12,54,1579,9,54,3,54,1581,8,54,1,54,1,54,1,54,1,54,1,
	54,5,54,1588,8,54,10,54,12,54,1591,9,54,3,54,1593,8,54,1,54,1,54,3,54,1597,
	8,54,1,54,3,54,1600,8,54,1,54,3,54,1603,8,54,1,54,1,54,1,54,1,54,1,54,1,
	54,1,54,1,54,5,54,1613,8,54,10,54,12,54,1616,9,54,3,54,1618,8,54,1,54,1,
	54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,4,54,
	1635,8,54,11,54,12,54,1636,1,54,1,54,3,54,1641,8,54,1,54,1,54,1,54,1,54,
	4,54,1647,8,54,11,54,12,54,1648,1,54,1,54,3,54,1653,8,54,1,54,1,54,1,54,
	1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,
	54,1,54,1,54,1,54,5,54,1676,8,54,10,54,12,54,1679,9,54,3,54,1681,8,54,1,
	54,1,54,1,54,1,54,1,54,1,54,1,54,3,54,1690,8,54,1,54,1,54,1,54,1,54,3,54,
	1696,8,54,1,54,1,54,1,54,1,54,3,54,1702,8,54,1,54,1,54,1,54,1,54,3,54,1708,
	8,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,3,54,1718,8,54,1,54,1,54,1,
	54,1,54,1,54,1,54,1,54,3,54,1727,8,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,
	1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,1,54,5,54,1747,8,54,10,
	54,12,54,1750,9,54,3,54,1752,8,54,1,54,3,54,1755,8,54,1,54,1,54,1,54,1,
	54,1,54,1,54,1,54,1,54,5,54,1765,8,54,10,54,12,54,1768,9,54,1,55,1,55,1,
	55,1,55,3,55,1774,8,55,3,55,1776,8,55,1,56,1,56,1,56,1,56,3,56,1782,8,56,
	1,57,1,57,1,57,1,57,1,57,1,57,3,57,1790,8,57,1,58,1,58,1,59,1,59,1,60,1,
	60,1,61,1,61,3,61,1800,8,61,1,61,1,61,1,61,1,61,3,61,1806,8,61,1,62,1,62,
	1,63,1,63,1,64,1,64,1,64,1,64,5,64,1816,8,64,10,64,12,64,1819,9,64,3,64,
	1821,8,64,1,64,1,64,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,
	65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,5,65,1846,8,65,10,
	65,12,65,1849,9,65,1,65,1,65,1,65,1,65,1,65,1,65,1,65,5,65,1858,8,65,10,
	65,12,65,1861,9,65,1,65,1,65,3,65,1865,8,65,1,65,1,65,1,65,1,65,1,65,3,
	65,1872,8,65,1,65,1,65,5,65,1876,8,65,10,65,12,65,1879,9,65,1,66,1,66,3,
	66,1883,8,66,1,67,1,67,1,67,1,67,3,67,1889,8,67,1,68,1,68,1,68,1,68,1,68,
	1,69,1,69,1,69,1,69,1,69,1,69,1,70,1,70,1,70,1,70,1,70,1,70,1,70,5,70,1909,
	8,70,10,70,12,70,1912,9,70,3,70,1914,8,70,1,70,1,70,1,70,1,70,1,70,5,70,
	1921,8,70,10,70,12,70,1924,9,70,3,70,1926,8,70,1,70,3,70,1929,8,70,1,70,
	1,70,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,
	71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,1,71,3,71,1957,8,71,1,72,
	1,72,1,72,1,72,1,72,1,72,1,72,1,72,1,72,3,72,1968,8,72,1,73,1,73,1,73,1,
	73,1,74,1,74,1,74,1,74,3,74,1978,8,74,1,75,1,75,1,75,1,75,1,75,3,75,1985,
	8,75,1,76,1,76,1,76,1,76,1,76,1,76,1,76,3,76,1994,8,76,1,77,1,77,1,77,1,
	77,1,77,3,77,2001,8,77,1,78,1,78,1,78,1,78,3,78,2007,8,78,1,79,1,79,1,79,
	5,79,2012,8,79,10,79,12,79,2015,9,79,1,80,1,80,1,80,1,80,1,80,1,81,1,81,
	1,81,3,81,2025,8,81,1,82,1,82,1,82,3,82,2030,8,82,1,83,1,83,1,83,1,83,1,
	83,3,83,2037,8,83,1,84,1,84,1,84,5,84,2042,8,84,10,84,12,84,2045,9,84,1,
	85,1,85,1,85,1,85,1,85,3,85,2052,8,85,1,86,1,86,1,86,3,86,2057,8,86,1,87,
	1,87,3,87,2061,8,87,1,88,1,88,1,88,1,88,1,89,1,89,1,89,3,89,2070,8,89,1,
	90,1,90,1,90,3,90,2075,8,90,1,91,5,91,2078,8,91,10,91,12,91,2081,9,91,1,
	92,1,92,1,92,3,92,2086,8,92,1,93,1,93,1,93,3,93,2091,8,93,1,94,1,94,1,95,
	1,95,1,95,3,95,2098,8,95,1,96,1,96,1,96,0,6,60,84,102,106,108,130,97,0,
	2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,
	52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,
	100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,
	136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,
	172,174,176,178,180,182,184,186,188,190,192,0,25,2,0,27,27,164,164,2,0,
	50,50,101,101,2,0,78,78,93,93,2,0,65,65,94,94,1,0,173,174,2,0,12,12,244,
	244,2,0,64,64,210,210,2,0,19,19,52,52,2,0,74,74,109,109,2,0,12,12,56,56,
	2,0,22,22,190,190,1,0,235,236,1,0,237,239,1,0,229,234,3,0,12,12,16,16,185,
	185,2,0,71,71,203,203,5,0,48,48,90,90,120,121,177,177,227,227,1,0,124,127,
	2,0,75,75,149,149,3,0,85,85,105,105,197,197,4,0,57,57,102,102,117,117,217,
	217,2,0,138,138,226,226,3,0,191,192,200,200,220,220,2,0,55,55,60,60,51,
	0,10,12,14,14,16,17,19,22,25,27,30,35,40,40,42,42,46,48,50,50,52,52,54,
	55,57,57,60,60,62,62,65,65,68,68,70,70,72,75,77,77,80,85,88,88,90,92,94,
	94,96,96,99,99,101,102,104,105,107,110,112,112,114,114,117,122,124,129,
	133,136,138,139,142,142,144,149,151,155,157,167,169,171,173,178,180,192,
	194,197,199,202,204,206,208,209,211,211,213,215,217,217,219,221,225,228,
	2411,0,194,1,0,0,0,2,197,1,0,0,0,4,200,1,0,0,0,6,203,1,0,0,0,8,206,1,0,
	0,0,10,209,1,0,0,0,12,212,1,0,0,0,14,215,1,0,0,0,16,959,1,0,0,0,18,962,
	1,0,0,0,20,966,1,0,0,0,22,981,1,0,0,0,24,983,1,0,0,0,26,997,1,0,0,0,28,
	1003,1,0,0,0,30,1014,1,0,0,0,32,1018,1,0,0,0,34,1024,1,0,0,0,36,1031,1,
	0,0,0,38,1036,1,0,0,0,40,1039,1,0,0,0,42,1043,1,0,0,0,44,1045,1,0,0,0,46,
	1048,1,0,0,0,48,1055,1,0,0,0,50,1060,1,0,0,0,52,1071,1,0,0,0,54,1073,1,
	0,0,0,56,1075,1,0,0,0,58,1099,1,0,0,0,60,1107,1,0,0,0,62,1143,1,0,0,0,64,
	1145,1,0,0,0,66,1153,1,0,0,0,68,1175,1,0,0,0,70,1183,1,0,0,0,72,1192,1,
	0,0,0,74,1242,1,0,0,0,76,1257,1,0,0,0,78,1259,1,0,0,0,80,1268,1,0,0,0,82,
	1282,1,0,0,0,84,1284,1,0,0,0,86,1323,1,0,0,0,88,1339,1,0,0,0,90,1341,1,
	0,0,0,92,1350,1,0,0,0,94,1352,1,0,0,0,96,1362,1,0,0,0,98,1405,1,0,0,0,100,
	1407,1,0,0,0,102,1416,1,0,0,0,104,1490,1,0,0,0,106,1496,1,0,0,0,108,1754,
	1,0,0,0,110,1775,1,0,0,0,112,1781,1,0,0,0,114,1789,1,0,0,0,116,1791,1,0,
	0,0,118,1793,1,0,0,0,120,1795,1,0,0,0,122,1797,1,0,0,0,124,1807,1,0,0,0,
	126,1809,1,0,0,0,128,1811,1,0,0,0,130,1871,1,0,0,0,132,1882,1,0,0,0,134,
	1888,1,0,0,0,136,1890,1,0,0,0,138,1895,1,0,0,0,140,1901,1,0,0,0,142,1956,
	1,0,0,0,144,1967,1,0,0,0,146,1969,1,0,0,0,148,1977,1,0,0,0,150,1984,1,0,
	0,0,152,1993,1,0,0,0,154,2000,1,0,0,0,156,2006,1,0,0,0,158,2008,1,0,0,0,
	160,2016,1,0,0,0,162,2024,1,0,0,0,164,2029,1,0,0,0,166,2036,1,0,0,0,168,
	2038,1,0,0,0,170,2051,1,0,0,0,172,2056,1,0,0,0,174,2060,1,0,0,0,176,2062,
	1,0,0,0,178,2066,1,0,0,0,180,2074,1,0,0,0,182,2079,1,0,0,0,184,2085,1,0,
	0,0,186,2090,1,0,0,0,188,2092,1,0,0,0,190,2097,1,0,0,0,192,2099,1,0,0,0,
	194,195,3,16,8,0,195,196,5,0,0,1,196,1,1,0,0,0,197,198,3,100,50,0,198,199,
	5,0,0,1,199,3,1,0,0,0,200,201,3,42,21,0,201,202,5,0,0,1,202,5,1,0,0,0,203,
	204,3,68,34,0,204,205,5,0,0,1,205,7,1,0,0,0,206,207,3,70,35,0,207,208,5,
	0,0,1,208,9,1,0,0,0,209,210,3,102,51,0,210,211,5,0,0,1,211,11,1,0,0,0,212,
	213,3,58,29,0,213,214,5,0,0,1,214,13,1,0,0,0,215,216,5,244,0,0,216,217,
	5,0,0,1,217,15,1,0,0,0,218,960,3,18,9,0,219,220,5,214,0,0,220,960,3,170,
	85,0,221,222,5,214,0,0,222,223,3,170,85,0,223,224,5,1,0,0,224,225,3,170,
	85,0,225,960,1,0,0,0,226,227,5,37,0,0,227,231,5,175,0,0,228,229,5,91,0,
	0,229,230,5,131,0,0,230,232,5,67,0,0,231,228,1,0,0,0,231,232,1,0,0,0,232,
	233,1,0,0,0,233,236,3,158,79,0,234,235,5,224,0,0,235,237,3,28,14,0,236,
	234,1,0,0,0,236,237,1,0,0,0,237,960,1,0,0,0,238,239,5,58,0,0,239,242,5,
	175,0,0,240,241,5,91,0,0,241,243,5,67,0,0,242,240,1,0,0,0,242,243,1,0,0,
	0,243,244,1,0,0,0,244,246,3,158,79,0,245,247,7,0,0,0,246,245,1,0,0,0,246,
	247,1,0,0,0,247,960,1,0,0,0,248,249,5,13,0,0,249,250,5,175,0,0,250,251,
	3,158,79,0,251,252,5,159,0,0,252,253,5,201,0,0,253,254,3,170,85,0,254,960,
	1,0,0,0,255,256,5,37,0,0,256,260,5,193,0,0,257,258,5,91,0,0,258,259,5,131,
	0,0,259,261,5,67,0,0,260,257,1,0,0,0,260,261,1,0,0,0,261,262,1,0,0,0,262,
	264,3,158,79,0,263,265,3,96,48,0,264,263,1,0,0,0,264,265,1,0,0,0,265,268,
	1,0,0,0,266,267,5,33,0,0,267,269,3,110,55,0,268,266,1,0,0,0,268,269,1,0,
	0,0,269,272,1,0,0,0,270,271,5,224,0,0,271,273,3,28,14,0,272,270,1,0,0,0,
	272,273,1,0,0,0,273,274,1,0,0,0,274,280,5,18,0,0,275,281,3,18,9,0,276,277,
	5,2,0,0,277,278,3,18,9,0,278,279,5,3,0,0,279,281,1,0,0,0,280,275,1,0,0,
	0,280,276,1,0,0,0,281,287,1,0,0,0,282,284,5,224,0,0,283,285,5,128,0,0,284,
	283,1,0,0,0,284,285,1,0,0,0,285,286,1,0,0,0,286,288,5,46,0,0,287,282,1,
	0,0,0,287,288,1,0,0,0,288,960,1,0,0,0,289,290,5,37,0,0,290,294,5,193,0,
	0,291,292,5,91,0,0,292,293,5,131,0,0,293,295,5,67,0,0,294,291,1,0,0,0,294,
	295,1,0,0,0,295,296,1,0,0,0,296,297,3,158,79,0,297,298,5,2,0,0,298,303,
	3,22,11,0,299,300,5,4,0,0,300,302,3,22,11,0,301,299,1,0,0,0,302,305,1,0,
	0,0,303,301,1,0,0,0,303,304,1,0,0,0,304,306,1,0,0,0,305,303,1,0,0,0,306,
	309,5,3,0,0,307,308,5,33,0,0,308,310,3,110,55,0,309,307,1,0,0,0,309,310,
	1,0,0,0,310,313,1,0,0,0,311,312,5,224,0,0,312,314,3,28,14,0,313,311,1,0,
	0,0,313,314,1,0,0,0,314,960,1,0,0,0,315,316,5,58,0,0,316,319,5,193,0,0,
	317,318,5,91,0,0,318,320,5,67,0,0,319,317,1,0,0,0,319,320,1,0,0,0,320,321,
	1,0,0,0,321,960,3,158,79,0,322,323,5,97,0,0,323,324,5,100,0,0,324,326,3,
	158,79,0,325,327,3,96,48,0,326,325,1,0,0,0,326,327,1,0,0,0,327,328,1,0,
	0,0,328,329,3,18,9,0,329,960,1,0,0,0,330,331,5,51,0,0,331,332,5,78,0,0,
	332,335,3,158,79,0,333,334,5,223,0,0,334,336,3,102,51,0,335,333,1,0,0,0,
	335,336,1,0,0,0,336,960,1,0,0,0,337,338,5,204,0,0,338,339,5,193,0,0,339,
	960,3,158,79,0,340,341,5,13,0,0,341,344,5,193,0,0,342,343,5,91,0,0,343,
	345,5,67,0,0,344,342,1,0,0,0,344,345,1,0,0,0,345,346,1,0,0,0,346,347,3,
	158,79,0,347,348,5,159,0,0,348,349,5,201,0,0,349,350,3,158,79,0,350,960,
	1,0,0,0,351,352,5,13,0,0,352,355,5,193,0,0,353,354,5,91,0,0,354,356,5,67,
	0,0,355,353,1,0,0,0,355,356,1,0,0,0,356,357,1,0,0,0,357,358,3,158,79,0,
	358,359,5,159,0,0,359,362,5,31,0,0,360,361,5,91,0,0,361,363,5,67,0,0,362,
	360,1,0,0,0,362,363,1,0,0,0,363,364,1,0,0,0,364,365,3,170,85,0,365,366,
	5,201,0,0,366,367,3,170,85,0,367,960,1,0,0,0,368,369,5,13,0,0,369,372,5,
	193,0,0,370,371,5,91,0,0,371,373,5,67,0,0,372,370,1,0,0,0,372,373,1,0,0,
	0,373,374,1,0,0,0,374,375,3,158,79,0,375,376,5,58,0,0,376,379,5,31,0,0,
	377,378,5,91,0,0,378,380,5,67,0,0,379,377,1,0,0,0,379,380,1,0,0,0,380,381,
	1,0,0,0,381,382,3,158,79,0,382,960,1,0,0,0,383,384,5,13,0,0,384,387,5,193,
	0,0,385,386,5,91,0,0,386,388,5,67,0,0,387,385,1,0,0,0,387,388,1,0,0,0,388,
	389,1,0,0,0,389,390,3,158,79,0,390,391,5,10,0,0,391,395,5,31,0,0,392,393,
	5,91,0,0,393,394,5,131,0,0,394,396,5,67,0,0,395,392,1,0,0,0,395,396,1,0,
	0,0,396,397,1,0,0,0,397,398,3,24,12,0,398,960,1,0,0,0,399,400,5,13,0,0,
	400,403,5,193,0,0,401,402,5,91,0,0,402,404,5,67,0,0,403,401,1,0,0,0,403,
	404,1,0,0,0,404,405,1,0,0,0,405,406,3,158,79,0,406,407,5,10,0,0,407,408,
	3,174,87,0,408,960,1,0,0,0,409,410,5,13,0,0,410,413,5,193,0,0,411,412,5,
	91,0,0,412,414,5,67,0,0,413,411,1,0,0,0,413,414,1,0,0,0,414,415,1,0,0,0,
	415,416,3,158,79,0,416,417,5,58,0,0,417,420,5,36,0,0,418,419,5,91,0,0,419,
	421,5,67,0,0,420,418,1,0,0,0,420,421,1,0,0,0,421,422,1,0,0,0,422,423,3,
	170,85,0,423,960,1,0,0,0,424,425,5,13,0,0,425,428,5,193,0,0,426,427,5,91,
	0,0,427,429,5,67,0,0,428,426,1,0,0,0,428,429,1,0,0,0,429,430,1,0,0,0,430,
	431,3,158,79,0,431,433,5,13,0,0,432,434,5,31,0,0,433,432,1,0,0,0,433,434,
	1,0,0,0,434,435,1,0,0,0,435,436,3,170,85,0,436,437,5,182,0,0,437,438,5,
	131,0,0,438,439,5,132,0,0,439,960,1,0,0,0,440,441,5,13,0,0,441,444,5,193,
	0,0,442,443,5,91,0,0,443,445,5,67,0,0,444,442,1,0,0,0,444,445,1,0,0,0,445,
	446,1,0,0,0,446,447,3,158,79,0,447,449,5,13,0,0,448,450,5,31,0,0,449,448,
	1,0,0,0,449,450,1,0,0,0,450,451,1,0,0,0,451,452,3,170,85,0,452,453,5,58,
	0,0,453,454,5,131,0,0,454,455,5,132,0,0,455,960,1,0,0,0,456,457,5,13,0,
	0,457,460,5,193,0,0,458,459,5,91,0,0,459,461,5,67,0,0,460,458,1,0,0,0,460,
	461,1,0,0,0,461,462,1,0,0,0,462,463,3,158,79,0,463,464,5,182,0,0,464,465,
	5,153,0,0,465,466,3,28,14,0,466,960,1,0,0,0,467,468,5,14,0,0,468,471,3,
	158,79,0,469,470,5,224,0,0,470,472,3,28,14,0,471,469,1,0,0,0,471,472,1,
	0,0,0,472,960,1,0,0,0,473,474,5,37,0,0,474,475,5,206,0,0,475,476,3,158,
	79,0,476,489,5,18,0,0,477,478,5,2,0,0,478,483,3,32,16,0,479,480,5,4,0,0,
	480,482,3,32,16,0,481,479,1,0,0,0,482,485,1,0,0,0,483,481,1,0,0,0,483,484,
	1,0,0,0,484,486,1,0,0,0,485,483,1,0,0,0,486,487,5,3,0,0,487,490,1,0,0,0,
	488,490,3,130,65,0,489,477,1,0,0,0,489,488,1,0,0,0,490,960,1,0,0,0,491,
	494,5,37,0,0,492,493,5,140,0,0,493,495,5,161,0,0,494,492,1,0,0,0,494,495,
	1,0,0,0,495,496,1,0,0,0,496,497,5,221,0,0,497,500,3,158,79,0,498,499,5,
	178,0,0,499,501,7,1,0,0,500,498,1,0,0,0,500,501,1,0,0,0,501,502,1,0,0,0,
	502,503,5,18,0,0,503,504,3,18,9,0,504,960,1,0,0,0,505,506,5,13,0,0,506,
	509,5,221,0,0,507,508,5,91,0,0,508,510,5,67,0,0,509,507,1,0,0,0,509,510,
	1,0,0,0,510,511,1,0,0,0,511,512,3,158,79,0,512,513,5,159,0,0,513,514,5,
	201,0,0,514,515,3,158,79,0,515,960,1,0,0,0,516,517,5,58,0,0,517,520,5,221,
	0,0,518,519,5,91,0,0,519,521,5,67,0,0,520,518,1,0,0,0,520,521,1,0,0,0,521,
	522,1,0,0,0,522,960,3,158,79,0,523,524,5,37,0,0,524,525,5,119,0,0,525,529,
	5,221,0,0,526,527,5,91,0,0,527,528,5,131,0,0,528,530,5,67,0,0,529,526,1,
	0,0,0,529,530,1,0,0,0,530,531,1,0,0,0,531,534,3,158,79,0,532,533,5,33,0,
	0,533,535,3,110,55,0,534,532,1,0,0,0,534,535,1,0,0,0,535,538,1,0,0,0,536,
	537,5,224,0,0,537,539,3,28,14,0,538,536,1,0,0,0,538,539,1,0,0,0,539,540,
	1,0,0,0,540,546,5,18,0,0,541,547,3,18,9,0,542,543,5,2,0,0,543,544,3,18,
	9,0,544,545,5,3,0,0,545,547,1,0,0,0,546,541,1,0,0,0,546,542,1,0,0,0,547,
	960,1,0,0,0,548,549,5,58,0,0,549,550,5,119,0,0,550,553,5,221,0,0,551,552,
	5,91,0,0,552,554,5,67,0,0,553,551,1,0,0,0,553,554,1,0,0,0,554,555,1,0,0,
	0,555,960,3,158,79,0,556,557,5,157,0,0,557,558,5,119,0,0,558,559,5,221,
	0,0,559,560,3,158,79,0,560,561,5,223,0,0,561,562,3,102,51,0,562,960,1,0,
	0,0,563,566,5,37,0,0,564,565,5,140,0,0,565,567,5,161,0,0,566,564,1,0,0,
	0,566,567,1,0,0,0,567,569,1,0,0,0,568,570,5,196,0,0,569,568,1,0,0,0,569,
	570,1,0,0,0,570,571,1,0,0,0,571,572,5,80,0,0,572,573,3,158,79,0,573,582,
	5,2,0,0,574,579,3,32,16,0,575,576,5,4,0,0,576,578,3,32,16,0,577,575,1,0,
	0,0,578,581,1,0,0,0,579,577,1,0,0,0,579,580,1,0,0,0,580,583,1,0,0,0,581,
	579,1,0,0,0,582,574,1,0,0,0,582,583,1,0,0,0,583,584,1,0,0,0,584,585,5,3,
	0,0,585,586,5,166,0,0,586,589,3,130,65,0,587,588,5,33,0,0,588,590,3,110,
	55,0,589,587,1,0,0,0,589,590,1,0,0,0,590,591,1,0,0,0,591,592,3,34,17,0,
	592,593,3,42,21,0,593,960,1,0,0,0,594,595,5,13,0,0,595,596,5,80,0,0,596,
	598,3,158,79,0,597,599,3,128,64,0,598,597,1,0,0,0,598,599,1,0,0,0,599,600,
	1,0,0,0,600,601,3,38,19,0,601,960,1,0,0,0,602,604,5,58,0,0,603,605,5,196,
	0,0,604,603,1,0,0,0,604,605,1,0,0,0,605,606,1,0,0,0,606,609,5,80,0,0,607,
	608,5,91,0,0,608,610,5,67,0,0,609,607,1,0,0,0,609,610,1,0,0,0,610,611,1,
	0,0,0,611,613,3,158,79,0,612,614,3,128,64,0,613,612,1,0,0,0,613,614,1,0,
	0,0,614,960,1,0,0,0,615,616,5,25,0,0,616,617,3,158,79,0,617,626,5,2,0,0,
	618,623,3,154,77,0,619,620,5,4,0,0,620,622,3,154,77,0,621,619,1,0,0,0,622,
	625,1,0,0,0,623,621,1,0,0,0,623,624,1,0,0,0,624,627,1,0,0,0,625,623,1,0,
	0,0,626,618,1,0,0,0,626,627,1,0,0,0,627,628,1,0,0,0,628,629,5,3,0,0,629,
	960,1,0,0,0,630,631,5,37,0,0,631,632,5,169,0,0,632,636,3,170,85,0,633,634,
	5,224,0,0,634,635,5,11,0,0,635,637,3,164,82,0,636,633,1,0,0,0,636,637,1,
	0,0,0,637,960,1,0,0,0,638,639,5,58,0,0,639,640,5,169,0,0,640,960,3,170,
	85,0,641,642,5,82,0,0,642,643,3,168,84,0,643,644,5,201,0,0,644,649,3,166,
	83,0,645,646,5,4,0,0,646,648,3,166,83,0,647,645,1,0,0,0,648,651,1,0,0,0,
	649,647,1,0,0,0,649,650,1,0,0,0,650,655,1,0,0,0,651,649,1,0,0,0,652,653,
	5,224,0,0,653,654,5,11,0,0,654,656,5,139,0,0,655,652,1,0,0,0,655,656,1,
	0,0,0,656,660,1,0,0,0,657,658,5,83,0,0,658,659,5,24,0,0,659,661,3,164,82,
	0,660,657,1,0,0,0,660,661,1,0,0,0,661,960,1,0,0,0,662,666,5,167,0,0,663,
	664,5,11,0,0,664,665,5,139,0,0,665,667,5,76,0,0,666,663,1,0,0,0,666,667,
	1,0,0,0,667,668,1,0,0,0,668,669,3,168,84,0,669,670,5,78,0,0,670,675,3,166,
	83,0,671,672,5,4,0,0,672,674,3,166,83,0,673,671,1,0,0,0,674,677,1,0,0,0,
	675,673,1,0,0,0,675,676,1,0,0,0,676,681,1,0,0,0,677,675,1,0,0,0,678,679,
	5,83,0,0,679,680,5,24,0,0,680,682,3,164,82,0,681,678,1,0,0,0,681,682,1,
	0,0,0,682,960,1,0,0,0,683,684,5,182,0,0,684,688,5,169,0,0,685,689,5,12,
	0,0,686,689,5,129,0,0,687,689,3,170,85,0,688,685,1,0,0,0,688,686,1,0,0,
	0,688,687,1,0,0,0,689,960,1,0,0,0,690,701,5,82,0,0,691,696,3,156,78,0,692,
	693,5,4,0,0,693,695,3,156,78,0,694,692,1,0,0,0,695,698,1,0,0,0,696,694,
	1,0,0,0,696,697,1,0,0,0,697,702,1,0,0,0,698,696,1,0,0,0,699,700,5,12,0,
	0,700,702,5,152,0,0,701,691,1,0,0,0,701,699,1,0,0,0,702,703,1,0,0,0,703,
	705,5,137,0,0,704,706,5,193,0,0,705,704,1,0,0,0,705,706,1,0,0,0,706,707,
	1,0,0,0,707,708,3,158,79,0,708,709,5,201,0,0,709,713,3,166,83,0,710,711,
	5,224,0,0,711,712,5,82,0,0,712,714,5,139,0,0,713,710,1,0,0,0,713,714,1,
	0,0,0,714,960,1,0,0,0,715,719,5,167,0,0,716,717,5,82,0,0,717,718,5,139,
	0,0,718,720,5,76,0,0,719,716,1,0,0,0,719,720,1,0,0,0,720,731,1,0,0,0,721,
	726,3,156,78,0,722,723,5,4,0,0,723,725,3,156,78,0,724,722,1,0,0,0,725,728,
	1,0,0,0,726,724,1,0,0,0,726,727,1,0,0,0,727,732,1,0,0,0,728,726,1,0,0,0,
	729,730,5,12,0,0,730,732,5,152,0,0,731,721,1,0,0,0,731,729,1,0,0,0,732,
	733,1,0,0,0,733,735,5,137,0,0,734,736,5,193,0,0,735,734,1,0,0,0,735,736,
	1,0,0,0,736,737,1,0,0,0,737,738,3,158,79,0,738,739,5,78,0,0,739,740,3,166,
	83,0,740,960,1,0,0,0,741,742,5,184,0,0,742,748,5,84,0,0,743,745,5,137,0,
	0,744,746,5,193,0,0,745,744,1,0,0,0,745,746,1,0,0,0,746,747,1,0,0,0,747,
	749,3,158,79,0,748,743,1,0,0,0,748,749,1,0,0,0,749,960,1,0,0,0,750,752,
	5,68,0,0,751,753,5,14,0,0,752,751,1,0,0,0,752,753,1,0,0,0,753,755,1,0,0,
	0,754,756,5,219,0,0,755,754,1,0,0,0,755,756,1,0,0,0,756,768,1,0,0,0,757,
	758,5,2,0,0,758,763,3,148,74,0,759,760,5,4,0,0,760,762,3,148,74,0,761,759,
	1,0,0,0,762,765,1,0,0,0,763,761,1,0,0,0,763,764,1,0,0,0,764,766,1,0,0,0,
	765,763,1,0,0,0,766,767,5,3,0,0,767,769,1,0,0,0,768,757,1,0,0,0,768,769,
	1,0,0,0,769,770,1,0,0,0,770,960,3,16,8,0,771,772,5,184,0,0,772,773,5,37,
	0,0,773,774,5,193,0,0,774,960,3,158,79,0,775,776,5,184,0,0,776,777,5,37,
	0,0,777,778,5,175,0,0,778,960,3,158,79,0,779,780,5,184,0,0,780,781,5,37,
	0,0,781,782,5,221,0,0,782,960,3,158,79,0,783,784,5,184,0,0,784,785,5,37,
	0,0,785,786,5,119,0,0,786,787,5,221,0,0,787,960,3,158,79,0,788,789,5,184,
	0,0,789,790,5,37,0,0,790,791,5,80,0,0,791,793,3,158,79,0,792,794,3,128,
	64,0,793,792,1,0,0,0,793,794,1,0,0,0,794,960,1,0,0,0,795,796,5,184,0,0,
	796,799,5,194,0,0,797,798,7,2,0,0,798,800,3,158,79,0,799,797,1,0,0,0,799,
	800,1,0,0,0,800,807,1,0,0,0,801,802,5,113,0,0,802,805,3,110,55,0,803,804,
	5,63,0,0,804,806,3,110,55,0,805,803,1,0,0,0,805,806,1,0,0,0,806,808,1,0,
	0,0,807,801,1,0,0,0,807,808,1,0,0,0,808,960,1,0,0,0,809,810,5,184,0,0,810,
	813,5,176,0,0,811,812,7,2,0,0,812,814,3,170,85,0,813,811,1,0,0,0,813,814,
	1,0,0,0,814,821,1,0,0,0,815,816,5,113,0,0,816,819,3,110,55,0,817,818,5,
	63,0,0,818,820,3,110,55,0,819,817,1,0,0,0,819,820,1,0,0,0,820,822,1,0,0,
	0,821,815,1,0,0,0,821,822,1,0,0,0,822,960,1,0,0,0,823,824,5,184,0,0,824,
	831,5,30,0,0,825,826,5,113,0,0,826,829,3,110,55,0,827,828,5,63,0,0,828,
	830,3,110,55,0,829,827,1,0,0,0,829,830,1,0,0,0,830,832,1,0,0,0,831,825,
	1,0,0,0,831,832,1,0,0,0,832,960,1,0,0,0,833,834,5,184,0,0,834,835,5,32,
	0,0,835,836,7,2,0,0,836,960,3,158,79,0,837,838,5,184,0,0,838,839,5,188,
	0,0,839,840,5,76,0,0,840,960,3,158,79,0,841,842,5,184,0,0,842,843,5,188,
	0,0,843,844,5,76,0,0,844,845,5,2,0,0,845,846,3,66,33,0,846,847,5,3,0,0,
	847,960,1,0,0,0,848,850,5,184,0,0,849,851,5,40,0,0,850,849,1,0,0,0,850,
	851,1,0,0,0,851,852,1,0,0,0,852,855,5,170,0,0,853,854,7,2,0,0,854,856,3,
	170,85,0,855,853,1,0,0,0,855,856,1,0,0,0,856,960,1,0,0,0,857,858,5,184,
	0,0,858,859,5,169,0,0,859,862,5,84,0,0,860,861,7,2,0,0,861,863,3,170,85,
	0,862,860,1,0,0,0,862,863,1,0,0,0,863,960,1,0,0,0,864,865,5,53,0,0,865,
	960,3,158,79,0,866,867,5,52,0,0,867,960,3,158,79,0,868,869,5,184,0,0,869,
	876,5,81,0,0,870,871,5,113,0,0,871,874,3,110,55,0,872,873,5,63,0,0,873,
	875,3,110,55,0,874,872,1,0,0,0,874,875,1,0,0,0,875,877,1,0,0,0,876,870,
	1,0,0,0,876,877,1,0,0,0,877,960,1,0,0,0,878,879,5,184,0,0,879,886,5,181,
	0,0,880,881,5,113,0,0,881,884,3,110,55,0,882,883,5,63,0,0,883,885,3,110,
	55,0,884,882,1,0,0,0,884,885,1,0,0,0,885,887,1,0,0,0,886,880,1,0,0,0,886,
	887,1,0,0,0,887,960,1,0,0,0,888,889,5,182,0,0,889,890,5,181,0,0,890,891,
	3,158,79,0,891,892,5,229,0,0,892,893,3,100,50,0,893,960,1,0,0,0,894,895,
	5,162,0,0,895,896,5,181,0,0,896,960,3,158,79,0,897,898,5,187,0,0,898,907,
	5,202,0,0,899,904,3,150,75,0,900,901,5,4,0,0,901,903,3,150,75,0,902,900,
	1,0,0,0,903,906,1,0,0,0,904,902,1,0,0,0,904,905,1,0,0,0,905,908,1,0,0,0,
	906,904,1,0,0,0,907,899,1,0,0,0,907,908,1,0,0,0,908,960,1,0,0,0,909,911,
	5,34,0,0,910,912,5,225,0,0,911,910,1,0,0,0,911,912,1,0,0,0,912,960,1,0,
	0,0,913,915,5,171,0,0,914,916,5,225,0,0,915,914,1,0,0,0,915,916,1,0,0,0,
	916,960,1,0,0,0,917,918,5,150,0,0,918,919,3,170,85,0,919,920,5,78,0,0,920,
	921,3,16,8,0,921,960,1,0,0,0,922,923,5,49,0,0,923,924,5,150,0,0,924,960,
	3,170,85,0,925,926,5,66,0,0,926,936,3,170,85,0,927,928,5,216,0,0,928,933,
	3,100,50,0,929,930,5,4,0,0,930,932,3,100,50,0,931,929,1,0,0,0,932,935,1,
	0,0,0,933,931,1,0,0,0,933,934,1,0,0,0,934,937,1,0,0,0,935,933,1,0,0,0,936,
	927,1,0,0,0,936,937,1,0,0,0,937,960,1,0,0,0,938,939,5,53,0,0,939,940,5,
	96,0,0,940,960,3,170,85,0,941,942,5,53,0,0,942,943,5,144,0,0,943,960,3,
	170,85,0,944,945,5,213,0,0,945,946,3,158,79,0,946,947,5,182,0,0,947,952,
	3,146,73,0,948,949,5,4,0,0,949,951,3,146,73,0,950,948,1,0,0,0,951,954,1,
	0,0,0,952,950,1,0,0,0,952,953,1,0,0,0,953,957,1,0,0,0,954,952,1,0,0,0,955,
	956,5,223,0,0,956,958,3,102,51,0,957,955,1,0,0,0,957,958,1,0,0,0,958,960,
	1,0,0,0,959,218,1,0,0,0,959,219,1,0,0,0,959,221,1,0,0,0,959,226,1,0,0,0,
	959,238,1,0,0,0,959,248,1,0,0,0,959,255,1,0,0,0,959,289,1,0,0,0,959,315,
	1,0,0,0,959,322,1,0,0,0,959,330,1,0,0,0,959,337,1,0,0,0,959,340,1,0,0,0,
	959,351,1,0,0,0,959,368,1,0,0,0,959,383,1,0,0,0,959,399,1,0,0,0,959,409,
	1,0,0,0,959,424,1,0,0,0,959,440,1,0,0,0,959,456,1,0,0,0,959,467,1,0,0,0,
	959,473,1,0,0,0,959,491,1,0,0,0,959,505,1,0,0,0,959,516,1,0,0,0,959,523,
	1,0,0,0,959,548,1,0,0,0,959,556,1,0,0,0,959,563,1,0,0,0,959,594,1,0,0,0,
	959,602,1,0,0,0,959,615,1,0,0,0,959,630,1,0,0,0,959,638,1,0,0,0,959,641,
	1,0,0,0,959,662,1,0,0,0,959,683,1,0,0,0,959,690,1,0,0,0,959,715,1,0,0,0,
	959,741,1,0,0,0,959,750,1,0,0,0,959,771,1,0,0,0,959,775,1,0,0,0,959,779,
	1,0,0,0,959,783,1,0,0,0,959,788,1,0,0,0,959,795,1,0,0,0,959,809,1,0,0,0,
	959,823,1,0,0,0,959,833,1,0,0,0,959,837,1,0,0,0,959,841,1,0,0,0,959,848,
	1,0,0,0,959,857,1,0,0,0,959,864,1,0,0,0,959,866,1,0,0,0,959,868,1,0,0,0,
	959,878,1,0,0,0,959,888,1,0,0,0,959,894,1,0,0,0,959,897,1,0,0,0,959,909,
	1,0,0,0,959,913,1,0,0,0,959,917,1,0,0,0,959,922,1,0,0,0,959,925,1,0,0,0,
	959,938,1,0,0,0,959,941,1,0,0,0,959,944,1,0,0,0,960,17,1,0,0,0,961,963,
	3,20,10,0,962,961,1,0,0,0,962,963,1,0,0,0,963,964,1,0,0,0,964,965,3,56,
	28,0,965,19,1,0,0,0,966,968,5,224,0,0,967,969,5,156,0,0,968,967,1,0,0,0,
	968,969,1,0,0,0,969,970,1,0,0,0,970,975,3,78,39,0,971,972,5,4,0,0,972,974,
	3,78,39,0,973,971,1,0,0,0,974,977,1,0,0,0,975,973,1,0,0,0,975,976,1,0,0,
	0,976,21,1,0,0,0,977,975,1,0,0,0,978,982,3,174,87,0,979,982,3,24,12,0,980,
	982,3,26,13,0,981,978,1,0,0,0,981,979,1,0,0,0,981,980,1,0,0,0,982,23,1,
	0,0,0,983,984,3,170,85,0,984,987,3,130,65,0,985,986,5,131,0,0,986,988,5,
	132,0,0,987,985,1,0,0,0,987,988,1,0,0,0,988,991,1,0,0,0,989,990,5,33,0,
	0,990,992,3,110,55,0,991,989,1,0,0,0,991,992,1,0,0,0,992,995,1,0,0,0,993,
	994,5,224,0,0,994,996,3,28,14,0,995,993,1,0,0,0,995,996,1,0,0,0,996,25,
	1,0,0,0,997,998,5,113,0,0,998,1001,3,158,79,0,999,1000,7,3,0,0,1000,1002,
	5,153,0,0,1001,999,1,0,0,0,1001,1002,1,0,0,0,1002,27,1,0,0,0,1003,1004,
	5,2,0,0,1004,1009,3,30,15,0,1005,1006,5,4,0,0,1006,1008,3,30,15,0,1007,
	1005,1,0,0,0,1008,1011,1,0,0,0,1009,1007,1,0,0,0,1009,1010,1,0,0,0,1010,
	1012,1,0,0,0,1011,1009,1,0,0,0,1012,1013,5,3,0,0,1013,29,1,0,0,0,1014,1015,
	3,170,85,0,1015,1016,5,229,0,0,1016,1017,3,100,50,0,1017,31,1,0,0,0,1018,
	1019,3,170,85,0,1019,1020,3,130,65,0,1020,33,1,0,0,0,1021,1023,3,36,18,
	0,1022,1021,1,0,0,0,1023,1026,1,0,0,0,1024,1022,1,0,0,0,1024,1025,1,0,0,
	0,1025,35,1,0,0,0,1026,1024,1,0,0,0,1027,1028,5,108,0,0,1028,1032,3,48,
	24,0,1029,1032,3,50,25,0,1030,1032,3,52,26,0,1031,1027,1,0,0,0,1031,1029,
	1,0,0,0,1031,1030,1,0,0,0,1032,37,1,0,0,0,1033,1035,3,40,20,0,1034,1033,
	1,0,0,0,1035,1038,1,0,0,0,1036,1034,1,0,0,0,1036,1037,1,0,0,0,1037,39,1,
	0,0,0,1038,1036,1,0,0,0,1039,1040,3,52,26,0,1040,41,1,0,0,0,1041,1044,3,
	44,22,0,1042,1044,3,46,23,0,1043,1041,1,0,0,0,1043,1042,1,0,0,0,1044,43,
	1,0,0,0,1045,1046,5,165,0,0,1046,1047,3,100,50,0,1047,45,1,0,0,0,1048,1051,
	5,70,0,0,1049,1050,5,122,0,0,1050,1052,3,54,27,0,1051,1049,1,0,0,0,1051,
	1052,1,0,0,0,1052,47,1,0,0,0,1053,1056,5,186,0,0,1054,1056,3,170,85,0,1055,
	1053,1,0,0,0,1055,1054,1,0,0,0,1056,49,1,0,0,0,1057,1061,5,54,0,0,1058,
	1059,5,131,0,0,1059,1061,5,54,0,0,1060,1057,1,0,0,0,1060,1058,1,0,0,0,1061,
	51,1,0,0,0,1062,1063,5,166,0,0,1063,1064,5,132,0,0,1064,1065,5,137,0,0,
	1065,1066,5,132,0,0,1066,1072,5,96,0,0,1067,1068,5,26,0,0,1068,1069,5,137,
	0,0,1069,1070,5,132,0,0,1070,1072,5,96,0,0,1071,1062,1,0,0,0,1071,1067,
	1,0,0,0,1072,53,1,0,0,0,1073,1074,3,170,85,0,1074,55,1,0,0,0,1075,1079,
	3,60,30,0,1076,1077,5,141,0,0,1077,1078,5,24,0,0,1078,1080,3,58,29,0,1079,
	1076,1,0,0,0,1079,1080,1,0,0,0,1080,1086,1,0,0,0,1081,1082,5,136,0,0,1082,
	1084,5,244,0,0,1083,1085,7,4,0,0,1084,1083,1,0,0,0,1084,1085,1,0,0,0,1085,
	1087,1,0,0,0,1086,1081,1,0,0,0,1086,1087,1,0,0,0,1087,1097,1,0,0,0,1088,
	1089,5,114,0,0,1089,1096,7,5,0,0,1090,1091,5,72,0,0,1091,1092,5,74,0,0,
	1092,1093,5,244,0,0,1093,1094,5,174,0,0,1094,1096,5,138,0,0,1095,1088,1,
	0,0,0,1095,1090,1,0,0,0,1096,1098,1,0,0,0,1097,1095,1,0,0,0,1097,1098,1,
	0,0,0,1098,57,1,0,0,0,1099,1104,3,64,32,0,1100,1101,5,4,0,0,1101,1103,3,
	64,32,0,1102,1100,1,0,0,0,1103,1106,1,0,0,0,1104,1102,1,0,0,0,1104,1105,
	1,0,0,0,1105,59,1,0,0,0,1106,1104,1,0,0,0,1107,1108,6,30,-1,0,1108,1109,
	3,62,31,0,1109,1124,1,0,0,0,1110,1111,10,2,0,0,1111,1113,5,98,0,0,1112,
	1114,3,80,40,0,1113,1112,1,0,0,0,1113,1114,1,0,0,0,1114,1115,1,0,0,0,1115,
	1123,3,60,30,3,1116,1117,10,1,0,0,1117,1119,7,6,0,0,1118,1120,3,80,40,0,
	1119,1118,1,0,0,0,1119,1120,1,0,0,0,1120,1121,1,0,0,0,1121,1123,3,60,30,
	2,1122,1110,1,0,0,0,1122,1116,1,0,0,0,1123,1126,1,0,0,0,1124,1122,1,0,0,
	0,1124,1125,1,0,0,0,1125,61,1,0,0,0,1126,1124,1,0,0,0,1127,1144,3,66,33,
	0,1128,1129,5,193,0,0,1129,1144,3,158,79,0,1130,1131,5,218,0,0,1131,1136,
	3,100,50,0,1132,1133,5,4,0,0,1133,1135,3,100,50,0,1134,1132,1,0,0,0,1135,
	1138,1,0,0,0,1136,1134,1,0,0,0,1136,1137,1,0,0,0,1137,1144,1,0,0,0,1138,
	1136,1,0,0,0,1139,1140,5,2,0,0,1140,1141,3,56,28,0,1141,1142,5,3,0,0,1142,
	1144,1,0,0,0,1143,1127,1,0,0,0,1143,1128,1,0,0,0,1143,1130,1,0,0,0,1143,
	1139,1,0,0,0,1144,63,1,0,0,0,1145,1147,3,100,50,0,1146,1148,7,7,0,0,1147,
	1146,1,0,0,0,1147,1148,1,0,0,0,1148,1151,1,0,0,0,1149,1150,5,134,0,0,1150,
	1152,7,8,0,0,1151,1149,1,0,0,0,1151,1152,1,0,0,0,1152,65,1,0,0,0,1153,1155,
	5,179,0,0,1154,1156,3,80,40,0,1155,1154,1,0,0,0,1155,1156,1,0,0,0,1156,
	1157,1,0,0,0,1157,1160,3,68,34,0,1158,1159,5,78,0,0,1159,1161,3,70,35,0,
	1160,1158,1,0,0,0,1160,1161,1,0,0,0,1161,1164,1,0,0,0,1162,1163,5,223,0,
	0,1163,1165,3,102,51,0,1164,1162,1,0,0,0,1164,1165,1,0,0,0,1165,1169,1,
	0,0,0,1166,1167,5,86,0,0,1167,1168,5,24,0,0,1168,1170,3,72,36,0,1169,1166,
	1,0,0,0,1169,1170,1,0,0,0,1170,1173,1,0,0,0,1171,1172,5,89,0,0,1172,1174,
	3,102,51,0,1173,1171,1,0,0,0,1173,1174,1,0,0,0,1174,67,1,0,0,0,1175,1180,
	3,82,41,0,1176,1177,5,4,0,0,1177,1179,3,82,41,0,1178,1176,1,0,0,0,1179,
	1182,1,0,0,0,1180,1178,1,0,0,0,1180,1181,1,0,0,0,1181,69,1,0,0,0,1182,1180,
	1,0,0,0,1183,1188,3,84,42,0,1184,1185,5,4,0,0,1185,1187,3,84,42,0,1186,
	1184,1,0,0,0,1187,1190,1,0,0,0,1188,1186,1,0,0,0,1188,1189,1,0,0,0,1189,
	71,1,0,0,0,1190,1188,1,0,0,0,1191,1193,3,80,40,0,1192,1191,1,0,0,0,1192,
	1193,1,0,0,0,1193,1194,1,0,0,0,1194,1199,3,74,37,0,1195,1196,5,4,0,0,1196,
	1198,3,74,37,0,1197,1195,1,0,0,0,1198,1201,1,0,0,0,1199,1197,1,0,0,0,1199,
	1200,1,0,0,0,1200,73,1,0,0,0,1201,1199,1,0,0,0,1202,1243,3,76,38,0,1203,
	1204,5,172,0,0,1204,1213,5,2,0,0,1205,1210,3,100,50,0,1206,1207,5,4,0,0,
	1207,1209,3,100,50,0,1208,1206,1,0,0,0,1209,1212,1,0,0,0,1210,1208,1,0,
	0,0,1210,1211,1,0,0,0,1211,1214,1,0,0,0,1212,1210,1,0,0,0,1213,1205,1,0,
	0,0,1213,1214,1,0,0,0,1214,1215,1,0,0,0,1215,1243,5,3,0,0,1216,1217,5,39,
	0,0,1217,1226,5,2,0,0,1218,1223,3,100,50,0,1219,1220,5,4,0,0,1220,1222,
	3,100,50,0,1221,1219,1,0,0,0,1222,1225,1,0,0,0,1223,1221,1,0,0,0,1223,1224,
	1,0,0,0,1224,1227,1,0,0,0,1225,1223,1,0,0,0,1226,1218,1,0,0,0,1226,1227,
	1,0,0,0,1227,1228,1,0,0,0,1228,1243,5,3,0,0,1229,1230,5,87,0,0,1230,1231,
	5,183,0,0,1231,1232,5,2,0,0,1232,1237,3,76,38,0,1233,1234,5,4,0,0,1234,
	1236,3,76,38,0,1235,1233,1,0,0,0,1236,1239,1,0,0,0,1237,1235,1,0,0,0,1237,
	1238,1,0,0,0,1238,1240,1,0,0,0,1239,1237,1,0,0,0,1240,1241,5,3,0,0,1241,
	1243,1,0,0,0,1242,1202,1,0,0,0,1242,1203,1,0,0,0,1242,1216,1,0,0,0,1242,
	1229,1,0,0,0,1243,75,1,0,0,0,1244,1253,5,2,0,0,1245,1250,3,100,50,0,1246,
	1247,5,4,0,0,1247,1249,3,100,50,0,1248,1246,1,0,0,0,1249,1252,1,0,0,0,1250,
	1248,1,0,0,0,1250,1251,1,0,0,0,1251,1254,1,0,0,0,1252,1250,1,0,0,0,1253,
	1245,1,0,0,0,1253,1254,1,0,0,0,1254,1255,1,0,0,0,1255,1258,5,3,0,0,1256,
	1258,3,100,50,0,1257,1244,1,0,0,0,1257,1256,1,0,0,0,1258,77,1,0,0,0,1259,
	1261,3,170,85,0,1260,1262,3,96,48,0,1261,1260,1,0,0,0,1261,1262,1,0,0,0,
	1262,1263,1,0,0,0,1263,1264,5,18,0,0,1264,1265,5,2,0,0,1265,1266,3,18,9,
	0,1266,1267,5,3,0,0,1267,79,1,0,0,0,1268,1269,7,9,0,0,1269,81,1,0,0,0,1270,
	1275,3,100,50,0,1271,1273,5,18,0,0,1272,1271,1,0,0,0,1272,1273,1,0,0,0,
	1273,1274,1,0,0,0,1274,1276,3,170,85,0,1275,1272,1,0,0,0,1275,1276,1,0,
	0,0,1276,1283,1,0,0,0,1277,1278,3,158,79,0,1278,1279,5,1,0,0,1279,1280,
	5,237,0,0,1280,1283,1,0,0,0,1281,1283,5,237,0,0,1282,1270,1,0,0,0,1282,
	1277,1,0,0,0,1282,1281,1,0,0,0,1283,83,1,0,0,0,1284,1285,6,42,-1,0,1285,
	1286,3,90,45,0,1286,1305,1,0,0,0,1287,1301,10,2,0,0,1288,1289,5,38,0,0,
	1289,1290,5,106,0,0,1290,1302,3,90,45,0,1291,1292,3,86,43,0,1292,1293,5,
	106,0,0,1293,1294,3,84,42,0,1294,1295,3,88,44,0,1295,1302,1,0,0,0,1296,
	1297,5,123,0,0,1297,1298,3,86,43,0,1298,1299,5,106,0,0,1299,1300,3,90,45,
	0,1300,1302,1,0,0,0,1301,1288,1,0,0,0,1301,1291,1,0,0,0,1301,1296,1,0,0,
	0,1302,1304,1,0,0,0,1303,1287,1,0,0,0,1304,1307,1,0,0,0,1305,1303,1,0,0,
	0,1305,1306,1,0,0,0,1306,85,1,0,0,0,1307,1305,1,0,0,0,1308,1310,5,95,0,
	0,1309,1308,1,0,0,0,1309,1310,1,0,0,0,1310,1324,1,0,0,0,1311,1313,5,111,
	0,0,1312,1314,5,143,0,0,1313,1312,1,0,0,0,1313,1314,1,0,0,0,1314,1324,1,
	0,0,0,1315,1317,5,168,0,0,1316,1318,5,143,0,0,1317,1316,1,0,0,0,1317,1318,
	1,0,0,0,1318,1324,1,0,0,0,1319,1321,5,79,0,0,1320,1322,5,143,0,0,1321,1320,
	1,0,0,0,1321,1322,1,0,0,0,1322,1324,1,0,0,0,1323,1309,1,0,0,0,1323,1311,
	1,0,0,0,1323,1315,1,0,0,0,1323,1319,1,0,0,0,1324,87,1,0,0,0,1325,1326,5,
	137,0,0,1326,1340,3,102,51,0,1327,1328,5,216,0,0,1328,1329,5,2,0,0,1329,
	1334,3,170,85,0,1330,1331,5,4,0,0,1331,1333,3,170,85,0,1332,1330,1,0,0,
	0,1333,1336,1,0,0,0,1334,1332,1,0,0,0,1334,1335,1,0,0,0,1335,1337,1,0,0,
	0,1336,1334,1,0,0,0,1337,1338,5,3,0,0,1338,1340,1,0,0,0,1339,1325,1,0,0,
	0,1339,1327,1,0,0,0,1340,89,1,0,0,0,1341,1348,3,94,47,0,1342,1343,5,195,
	0,0,1343,1344,3,92,46,0,1344,1345,5,2,0,0,1345,1346,3,100,50,0,1346,1347,
	5,3,0,0,1347,1349,1,0,0,0,1348,1342,1,0,0,0,1348,1349,1,0,0,0,1349,91,1,
	0,0,0,1350,1351,7,10,0,0,1351,93,1,0,0,0,1352,1360,3,98,49,0,1353,1355,
	5,18,0,0,1354,1353,1,0,0,0,1354,1355,1,0,0,0,1355,1356,1,0,0,0,1356,1358,
	3,170,85,0,1357,1359,3,96,48,0,1358,1357,1,0,0,0,1358,1359,1,0,0,0,1359,
	1361,1,0,0,0,1360,1354,1,0,0,0,1360,1361,1,0,0,0,1361,95,1,0,0,0,1362,1363,
	5,2,0,0,1363,1368,3,170,85,0,1364,1365,5,4,0,0,1365,1367,3,170,85,0,1366,
	1364,1,0,0,0,1367,1370,1,0,0,0,1368,1366,1,0,0,0,1368,1369,1,0,0,0,1369,
	1371,1,0,0,0,1370,1368,1,0,0,0,1371,1372,5,3,0,0,1372,97,1,0,0,0,1373,1375,
	3,158,79,0,1374,1376,3,160,80,0,1375,1374,1,0,0,0,1375,1376,1,0,0,0,1376,
	1406,1,0,0,0,1377,1378,5,2,0,0,1378,1379,3,18,9,0,1379,1380,5,3,0,0,1380,
	1406,1,0,0,0,1381,1382,5,212,0,0,1382,1383,5,2,0,0,1383,1388,3,100,50,0,
	1384,1385,5,4,0,0,1385,1387,3,100,50,0,1386,1384,1,0,0,0,1387,1390,1,0,
	0,0,1388,1386,1,0,0,0,1388,1389,1,0,0,0,1389,1391,1,0,0,0,1390,1388,1,0,
	0,0,1391,1394,5,3,0,0,1392,1393,5,224,0,0,1393,1395,5,142,0,0,1394,1392,
	1,0,0,0,1394,1395,1,0,0,0,1395,1406,1,0,0,0,1396,1397,5,110,0,0,1397,1398,
	5,2,0,0,1398,1399,3,18,9,0,1399,1400,5,3,0,0,1400,1406,1,0,0,0,1401,1402,
	5,2,0,0,1402,1403,3,84,42,0,1403,1404,5,3,0,0,1404,1406,1,0,0,0,1405,1373,
	1,0,0,0,1405,1377,1,0,0,0,1405,1381,1,0,0,0,1405,1396,1,0,0,0,1405,1401,
	1,0,0,0,1406,99,1,0,0,0,1407,1408,3,102,51,0,1408,101,1,0,0,0,1409,1410,
	6,51,-1,0,1410,1412,3,106,53,0,1411,1413,3,104,52,0,1412,1411,1,0,0,0,1412,
	1413,1,0,0,0,1413,1417,1,0,0,0,1414,1415,5,131,0,0,1415,1417,3,102,51,3,
	1416,1409,1,0,0,0,1416,1414,1,0,0,0,1417,1426,1,0,0,0,1418,1419,10,2,0,
	0,1419,1420,5,15,0,0,1420,1425,3,102,51,3,1421,1422,10,1,0,0,1422,1423,
	5,140,0,0,1423,1425,3,102,51,2,1424,1418,1,0,0,0,1424,1421,1,0,0,0,1425,
	1428,1,0,0,0,1426,1424,1,0,0,0,1426,1427,1,0,0,0,1427,103,1,0,0,0,1428,
	1426,1,0,0,0,1429,1430,3,116,58,0,1430,1431,3,106,53,0,1431,1491,1,0,0,
	0,1432,1433,3,116,58,0,1433,1434,3,118,59,0,1434,1435,5,2,0,0,1435,1436,
	3,18,9,0,1436,1437,5,3,0,0,1437,1491,1,0,0,0,1438,1440,5,131,0,0,1439,1438,
	1,0,0,0,1439,1440,1,0,0,0,1440,1441,1,0,0,0,1441,1442,5,23,0,0,1442,1443,
	3,106,53,0,1443,1444,5,15,0,0,1444,1445,3,106,53,0,1445,1491,1,0,0,0,1446,
	1448,5,131,0,0,1447,1446,1,0,0,0,1447,1448,1,0,0,0,1448,1449,1,0,0,0,1449,
	1450,5,93,0,0,1450,1451,5,2,0,0,1451,1456,3,100,50,0,1452,1453,5,4,0,0,
	1453,1455,3,100,50,0,1454,1452,1,0,0,0,1455,1458,1,0,0,0,1456,1454,1,0,
	0,0,1456,1457,1,0,0,0,1457,1459,1,0,0,0,1458,1456,1,0,0,0,1459,1460,5,3,
	0,0,1460,1491,1,0,0,0,1461,1463,5,131,0,0,1462,1461,1,0,0,0,1462,1463,1,
	0,0,0,1463,1464,1,0,0,0,1464,1465,5,93,0,0,1465,1466,5,2,0,0,1466,1467,
	3,18,9,0,1467,1468,5,3,0,0,1468,1491,1,0,0,0,1469,1471,5,131,0,0,1470,1469,
	1,0,0,0,1470,1471,1,0,0,0,1471,1472,1,0,0,0,1472,1473,5,113,0,0,1473,1476,
	3,106,53,0,1474,1475,5,63,0,0,1475,1477,3,106,53,0,1476,1474,1,0,0,0,1476,
	1477,1,0,0,0,1477,1491,1,0,0,0,1478,1480,5,103,0,0,1479,1481,5,131,0,0,
	1480,1479,1,0,0,0,1480,1481,1,0,0,0,1481,1482,1,0,0,0,1482,1491,5,132,0,
	0,1483,1485,5,103,0,0,1484,1486,5,131,0,0,1485,1484,1,0,0,0,1485,1486,1,
	0,0,0,1486,1487,1,0,0,0,1487,1488,5,56,0,0,1488,1489,5,78,0,0,1489,1491,
	3,106,53,0,1490,1429,1,0,0,0,1490,1432,1,0,0,0,1490,1439,1,0,0,0,1490,1447,
	1,0,0,0,1490,1462,1,0,0,0,1490,1470,1,0,0,0,1490,1478,1,0,0,0,1490,1483,
	1,0,0,0,1491,105,1,0,0,0,1492,1493,6,53,-1,0,1493,1497,3,108,54,0,1494,
	1495,7,11,0,0,1495,1497,3,106,53,4,1496,1492,1,0,0,0,1496,1494,1,0,0,0,
	1497,1512,1,0,0,0,1498,1499,10,3,0,0,1499,1500,7,12,0,0,1500,1511,3,106,
	53,4,1501,1502,10,2,0,0,1502,1503,7,11,0,0,1503,1511,3,106,53,3,1504,1505,
	10,1,0,0,1505,1506,5,240,0,0,1506,1511,3,106,53,2,1507,1508,10,5,0,0,1508,
	1509,5,20,0,0,1509,1511,3,114,57,0,1510,1498,1,0,0,0,1510,1501,1,0,0,0,
	1510,1504,1,0,0,0,1510,1507,1,0,0,0,1511,1514,1,0,0,0,1512,1510,1,0,0,0,
	1512,1513,1,0,0,0,1513,107,1,0,0,0,1514,1512,1,0,0,0,1515,1516,6,54,-1,
	0,1516,1755,5,132,0,0,1517,1755,3,122,61,0,1518,1519,3,170,85,0,1519,1520,
	3,110,55,0,1520,1755,1,0,0,0,1521,1522,5,253,0,0,1522,1755,3,110,55,0,1523,
	1755,3,172,86,0,1524,1755,3,120,60,0,1525,1755,3,110,55,0,1526,1755,5,243,
	0,0,1527,1755,5,5,0,0,1528,1529,5,148,0,0,1529,1530,5,2,0,0,1530,1531,3,
	106,53,0,1531,1532,5,93,0,0,1532,1533,3,106,53,0,1533,1534,5,3,0,0,1534,
	1755,1,0,0,0,1535,1536,5,2,0,0,1536,1539,3,100,50,0,1537,1538,5,4,0,0,1538,
	1540,3,100,50,0,1539,1537,1,0,0,0,1540,1541,1,0,0,0,1541,1539,1,0,0,0,1541,
	1542,1,0,0,0,1542,1543,1,0,0,0,1543,1544,5,3,0,0,1544,1755,1,0,0,0,1545,
	1546,5,173,0,0,1546,1547,5,2,0,0,1547,1552,3,100,50,0,1548,1549,5,4,0,0,
	1549,1551,3,100,50,0,1550,1548,1,0,0,0,1551,1554,1,0,0,0,1552,1550,1,0,
	0,0,1552,1553,1,0,0,0,1553,1555,1,0,0,0,1554,1552,1,0,0,0,1555,1556,5,3,
	0,0,1556,1755,1,0,0,0,1557,1558,3,158,79,0,1558,1559,5,2,0,0,1559,1560,
	5,237,0,0,1560,1562,5,3,0,0,1561,1563,3,138,69,0,1562,1561,1,0,0,0,1562,
	1563,1,0,0,0,1563,1565,1,0,0,0,1564,1566,3,140,70,0,1565,1564,1,0,0,0,1565,
	1566,1,0,0,0,1566,1755,1,0,0,0,1567,1568,3,158,79,0,1568,1580,5,2,0,0,1569,
	1571,3,80,40,0,1570,1569,1,0,0,0,1570,1571,1,0,0,0,1571,1572,1,0,0,0,1572,
	1577,3,100,50,0,1573,1574,5,4,0,0,1574,1576,3,100,50,0,1575,1573,1,0,0,
	0,1576,1579,1,0,0,0,1577,1575,1,0,0,0,1577,1578,1,0,0,0,1578,1581,1,0,0,
	0,1579,1577,1,0,0,0,1580,1570,1,0,0,0,1580,1581,1,0,0,0,1581,1592,1,0,0,
	0,1582,1583,5,141,0,0,1583,1584,5,24,0,0,1584,1589,3,64,32,0,1585,1586,
	5,4,0,0,1586,1588,3,64,32,0,1587,1585,1,0,0,0,1588,1591,1,0,0,0,1589,1587,
	1,0,0,0,1589,1590,1,0,0,0,1590,1593,1,0,0,0,1591,1589,1,0,0,0,1592,1582,
	1,0,0,0,1592,1593,1,0,0,0,1593,1594,1,0,0,0,1594,1596,5,3,0,0,1595,1597,
	3,138,69,0,1596,1595,1,0,0,0,1596,1597,1,0,0,0,1597,1602,1,0,0,0,1598,1600,
	3,112,56,0,1599,1598,1,0,0,0,1599,1600,1,0,0,0,1600,1601,1,0,0,0,1601,1603,
	3,140,70,0,1602,1599,1,0,0,0,1602,1603,1,0,0,0,1603,1755,1,0,0,0,1604,1605,
	3,170,85,0,1605,1606,5,6,0,0,1606,1607,3,100,50,0,1607,1755,1,0,0,0,1608,
	1617,5,2,0,0,1609,1614,3,170,85,0,1610,1611,5,4,0,0,1611,1613,3,170,85,
	0,1612,1610,1,0,0,0,1613,1616,1,0,0,0,1614,1612,1,0,0,0,1614,1615,1,0,0,
	0,1615,1618,1,0,0,0,1616,1614,1,0,0,0,1617,1609,1,0,0,0,1617,1618,1,0,0,
	0,1618,1619,1,0,0,0,1619,1620,5,3,0,0,1620,1621,5,6,0,0,1621,1755,3,100,
	50,0,1622,1623,5,2,0,0,1623,1624,3,18,9,0,1624,1625,5,3,0,0,1625,1755,1,
	0,0,0,1626,1627,5,67,0,0,1627,1628,5,2,0,0,1628,1629,3,18,9,0,1629,1630,
	5,3,0,0,1630,1755,1,0,0,0,1631,1632,5,28,0,0,1632,1634,3,106,53,0,1633,
	1635,3,136,68,0,1634,1633,1,0,0,0,1635,1636,1,0,0,0,1636,1634,1,0,0,0,1636,
	1637,1,0,0,0,1637,1640,1,0,0,0,1638,1639,5,59,0,0,1639,1641,3,100,50,0,
	1640,1638,1,0,0,0,1640,1641,1,0,0,0,1641,1642,1,0,0,0,1642,1643,5,61,0,
	0,1643,1755,1,0,0,0,1644,1646,5,28,0,0,1645,1647,3,136,68,0,1646,1645,1,
	0,0,0,1647,1648,1,0,0,0,1648,1646,1,0,0,0,1648,1649,1,0,0,0,1649,1652,1,
	0,0,0,1650,1651,5,59,0,0,1651,1653,3,100,50,0,1652,1650,1,0,0,0,1652,1653,
	1,0,0,0,1653,1654,1,0,0,0,1654,1655,5,61,0,0,1655,1755,1,0,0,0,1656,1657,
	5,29,0,0,1657,1658,5,2,0,0,1658,1659,3,100,50,0,1659,1660,5,18,0,0,1660,
	1661,3,130,65,0,1661,1662,5,3,0,0,1662,1755,1,0,0,0,1663,1664,5,205,0,0,
	1664,1665,5,2,0,0,1665,1666,3,100,50,0,1666,1667,5,18,0,0,1667,1668,3,130,
	65,0,1668,1669,5,3,0,0,1669,1755,1,0,0,0,1670,1671,5,17,0,0,1671,1680,5,
	7,0,0,1672,1677,3,100,50,0,1673,1674,5,4,0,0,1674,1676,3,100,50,0,1675,
	1673,1,0,0,0,1676,1679,1,0,0,0,1677,1675,1,0,0,0,1677,1678,1,0,0,0,1678,
	1681,1,0,0,0,1679,1677,1,0,0,0,1680,1672,1,0,0,0,1680,1681,1,0,0,0,1681,
	1682,1,0,0,0,1682,1755,5,8,0,0,1683,1755,3,170,85,0,1684,1755,5,41,0,0,
	1685,1689,5,43,0,0,1686,1687,5,2,0,0,1687,1688,5,244,0,0,1688,1690,5,3,
	0,0,1689,1686,1,0,0,0,1689,1690,1,0,0,0,1690,1755,1,0,0,0,1691,1695,5,44,
	0,0,1692,1693,5,2,0,0,1693,1694,5,244,0,0,1694,1696,5,3,0,0,1695,1692,1,
	0,0,0,1695,1696,1,0,0,0,1696,1755,1,0,0,0,1697,1701,5,115,0,0,1698,1699,
	5,2,0,0,1699,1700,5,244,0,0,1700,1702,5,3,0,0,1701,1698,1,0,0,0,1701,1702,
	1,0,0,0,1702,1755,1,0,0,0,1703,1707,5,116,0,0,1704,1705,5,2,0,0,1705,1706,
	5,244,0,0,1706,1708,5,3,0,0,1707,1704,1,0,0,0,1707,1708,1,0,0,0,1708,1755,
	1,0,0,0,1709,1755,5,45,0,0,1710,1711,5,189,0,0,1711,1712,5,2,0,0,1712,1713,
	3,106,53,0,1713,1714,5,78,0,0,1714,1717,3,106,53,0,1715,1716,5,76,0,0,1716,
	1718,3,106,53,0,1717,1715,1,0,0,0,1717,1718,1,0,0,0,1718,1719,1,0,0,0,1719,
	1720,5,3,0,0,1720,1755,1,0,0,0,1721,1722,5,130,0,0,1722,1723,5,2,0,0,1723,
	1726,3,106,53,0,1724,1725,5,4,0,0,1725,1727,3,126,63,0,1726,1724,1,0,0,
	0,1726,1727,1,0,0,0,1727,1728,1,0,0,0,1728,1729,5,3,0,0,1729,1755,1,0,0,
	0,1730,1731,5,69,0,0,1731,1732,5,2,0,0,1732,1733,3,170,85,0,1733,1734,5,
	78,0,0,1734,1735,3,106,53,0,1735,1736,5,3,0,0,1736,1755,1,0,0,0,1737,1738,
	5,2,0,0,1738,1739,3,100,50,0,1739,1740,5,3,0,0,1740,1755,1,0,0,0,1741,1742,
	5,87,0,0,1742,1751,5,2,0,0,1743,1748,3,158,79,0,1744,1745,5,4,0,0,1745,
	1747,3,158,79,0,1746,1744,1,0,0,0,1747,1750,1,0,0,0,1748,1746,1,0,0,0,1748,
	1749,1,0,0,0,1749,1752,1,0,0,0,1750,1748,1,0,0,0,1751,1743,1,0,0,0,1751,
	1752,1,0,0,0,1752,1753,1,0,0,0,1753,1755,5,3,0,0,1754,1515,1,0,0,0,1754,
	1517,1,0,0,0,1754,1518,1,0,0,0,1754,1521,1,0,0,0,1754,1523,1,0,0,0,1754,
	1524,1,0,0,0,1754,1525,1,0,0,0,1754,1526,1,0,0,0,1754,1527,1,0,0,0,1754,
	1528,1,0,0,0,1754,1535,1,0,0,0,1754,1545,1,0,0,0,1754,1557,1,0,0,0,1754,
	1567,1,0,0,0,1754,1604,1,0,0,0,1754,1608,1,0,0,0,1754,1622,1,0,0,0,1754,
	1626,1,0,0,0,1754,1631,1,0,0,0,1754,1644,1,0,0,0,1754,1656,1,0,0,0,1754,
	1663,1,0,0,0,1754,1670,1,0,0,0,1754,1683,1,0,0,0,1754,1684,1,0,0,0,1754,
	1685,1,0,0,0,1754,1691,1,0,0,0,1754,1697,1,0,0,0,1754,1703,1,0,0,0,1754,
	1709,1,0,0,0,1754,1710,1,0,0,0,1754,1721,1,0,0,0,1754,1730,1,0,0,0,1754,
	1737,1,0,0,0,1754,1741,1,0,0,0,1755,1766,1,0,0,0,1756,1757,10,14,0,0,1757,
	1758,5,7,0,0,1758,1759,3,106,53,0,1759,1760,5,8,0,0,1760,1765,1,0,0,0,1761,
	1762,10,12,0,0,1762,1763,5,1,0,0,1763,1765,3,170,85,0,1764,1756,1,0,0,0,
	1764,1761,1,0,0,0,1765,1768,1,0,0,0,1766,1764,1,0,0,0,1766,1767,1,0,0,0,
	1767,109,1,0,0,0,1768,1766,1,0,0,0,1769,1776,5,241,0,0,1770,1773,5,242,
	0,0,1771,1772,5,207,0,0,1772,1774,5,241,0,0,1773,1771,1,0,0,0,1773,1774,
	1,0,0,0,1774,1776,1,0,0,0,1775,1769,1,0,0,0,1775,1770,1,0,0,0,1776,111,
	1,0,0,0,1777,1778,5,92,0,0,1778,1782,5,134,0,0,1779,1780,5,163,0,0,1780,
	1782,5,134,0,0,1781,1777,1,0,0,0,1781,1779,1,0,0,0,1782,113,1,0,0,0,1783,
	1784,5,199,0,0,1784,1785,5,228,0,0,1785,1790,3,122,61,0,1786,1787,5,199,
	0,0,1787,1788,5,228,0,0,1788,1790,3,110,55,0,1789,1783,1,0,0,0,1789,1786,
	1,0,0,0,1790,115,1,0,0,0,1791,1792,7,13,0,0,1792,117,1,0,0,0,1793,1794,
	7,14,0,0,1794,119,1,0,0,0,1795,1796,7,15,0,0,1796,121,1,0,0,0,1797,1799,
	5,99,0,0,1798,1800,7,11,0,0,1799,1798,1,0,0,0,1799,1800,1,0,0,0,1800,1801,
	1,0,0,0,1801,1802,3,110,55,0,1802,1805,3,124,62,0,1803,1804,5,201,0,0,1804,
	1806,3,124,62,0,1805,1803,1,0,0,0,1805,1806,1,0,0,0,1806,123,1,0,0,0,1807,
	1808,7,16,0,0,1808,125,1,0,0,0,1809,1810,7,17,0,0,1810,127,1,0,0,0,1811,
	1820,5,2,0,0,1812,1817,3,130,65,0,1813,1814,5,4,0,0,1814,1816,3,130,65,
	0,1815,1813,1,0,0,0,1816,1819,1,0,0,0,1817,1815,1,0,0,0,1817,1818,1,0,0,
	0,1818,1821,1,0,0,0,1819,1817,1,0,0,0,1820,1812,1,0,0,0,1820,1821,1,0,0,
	0,1821,1822,1,0,0,0,1822,1823,5,3,0,0,1823,129,1,0,0,0,1824,1825,6,65,-1,
	0,1825,1826,5,17,0,0,1826,1827,5,231,0,0,1827,1828,3,130,65,0,1828,1829,
	5,233,0,0,1829,1872,1,0,0,0,1830,1831,5,118,0,0,1831,1832,5,231,0,0,1832,
	1833,3,130,65,0,1833,1834,5,4,0,0,1834,1835,3,130,65,0,1835,1836,5,233,
	0,0,1836,1872,1,0,0,0,1837,1838,5,173,0,0,1838,1839,5,2,0,0,1839,1840,3,
	170,85,0,1840,1847,3,130,65,0,1841,1842,5,4,0,0,1842,1843,3,170,85,0,1843,
	1844,3,130,65,0,1844,1846,1,0,0,0,1845,1841,1,0,0,0,1846,1849,1,0,0,0,1847,
	1845,1,0,0,0,1847,1848,1,0,0,0,1848,1850,1,0,0,0,1849,1847,1,0,0,0,1850,
	1851,5,3,0,0,1851,1872,1,0,0,0,1852,1864,3,134,67,0,1853,1854,5,2,0,0,1854,
	1859,3,132,66,0,1855,1856,5,4,0,0,1856,1858,3,132,66,0,1857,1855,1,0,0,
	0,1858,1861,1,0,0,0,1859,1857,1,0,0,0,1859,1860,1,0,0,0,1860,1862,1,0,0,
	0,1861,1859,1,0,0,0,1862,1863,5,3,0,0,1863,1865,1,0,0,0,1864,1853,1,0,0,
	0,1864,1865,1,0,0,0,1865,1872,1,0,0,0,1866,1867,5,99,0,0,1867,1868,3,124,
	62,0,1868,1869,5,201,0,0,1869,1870,3,124,62,0,1870,1872,1,0,0,0,1871,1824,
	1,0,0,0,1871,1830,1,0,0,0,1871,1837,1,0,0,0,1871,1852,1,0,0,0,1871,1866,
	1,0,0,0,1872,1877,1,0,0,0,1873,1874,10,6,0,0,1874,1876,5,17,0,0,1875,1873,
	1,0,0,0,1876,1879,1,0,0,0,1877,1875,1,0,0,0,1877,1878,1,0,0,0,1878,131,
	1,0,0,0,1879,1877,1,0,0,0,1880,1883,5,244,0,0,1881,1883,3,130,65,0,1882,
	1880,1,0,0,0,1882,1881,1,0,0,0,1883,133,1,0,0,0,1884,1889,5,251,0,0,1885,
	1889,5,252,0,0,1886,1889,5,253,0,0,1887,1889,3,158,79,0,1888,1884,1,0,0,
	0,1888,1885,1,0,0,0,1888,1886,1,0,0,0,1888,1887,1,0,0,0,1889,135,1,0,0,
	0,1890,1891,5,222,0,0,1891,1892,3,100,50,0,1892,1893,5,198,0,0,1893,1894,
	3,100,50,0,1894,137,1,0,0,0,1895,1896,5,73,0,0,1896,1897,5,2,0,0,1897,1898,
	5,223,0,0,1898,1899,3,102,51,0,1899,1900,5,3,0,0,1900,139,1,0,0,0,1901,
	1902,5,145,0,0,1902,1913,5,2,0,0,1903,1904,5,146,0,0,1904,1905,5,24,0,0,
	1905,1910,3,100,50,0,1906,1907,5,4,0,0,1907,1909,3,100,50,0,1908,1906,1,
	0,0,0,1909,1912,1,0,0,0,1910,1908,1,0,0,0,1910,1911,1,0,0,0,1911,1914,1,
	0,0,0,1912,1910,1,0,0,0,1913,1903,1,0,0,0,1913,1914,1,0,0,0,1914,1925,1,
	0,0,0,1915,1916,5,141,0,0,1916,1917,5,24,0,0,1917,1922,3,64,32,0,1918,1919,
	5,4,0,0,1919,1921,3,64,32,0,1920,1918,1,0,0,0,1921,1924,1,0,0,0,1922,1920,
	1,0,0,0,1922,1923,1,0,0,0,1923,1926,1,0,0,0,1924,1922,1,0,0,0,1925,1915,
	1,0,0,0,1925,1926,1,0,0,0,1926,1928,1,0,0,0,1927,1929,3,142,71,0,1928,1927,
	1,0,0,0,1928,1929,1,0,0,0,1929,1930,1,0,0,0,1930,1931,5,3,0,0,1931,141,
	1,0,0,0,1932,1933,5,154,0,0,1933,1957,3,144,72,0,1934,1935,5,174,0,0,1935,
	1957,3,144,72,0,1936,1937,5,88,0,0,1937,1957,3,144,72,0,1938,1939,5,154,
	0,0,1939,1940,5,23,0,0,1940,1941,3,144,72,0,1941,1942,5,15,0,0,1942,1943,
	3,144,72,0,1943,1957,1,0,0,0,1944,1945,5,174,0,0,1945,1946,5,23,0,0,1946,
	1947,3,144,72,0,1947,1948,5,15,0,0,1948,1949,3,144,72,0,1949,1957,1,0,0,
	0,1950,1951,5,88,0,0,1951,1952,5,23,0,0,1952,1953,3,144,72,0,1953,1954,
	5,15,0,0,1954,1955,3,144,72,0,1955,1957,1,0,0,0,1956,1932,1,0,0,0,1956,
	1934,1,0,0,0,1956,1936,1,0,0,0,1956,1938,1,0,0,0,1956,1944,1,0,0,0,1956,
	1950,1,0,0,0,1957,143,1,0,0,0,1958,1959,5,208,0,0,1959,1968,5,149,0,0,1960,
	1961,5,208,0,0,1961,1968,5,75,0,0,1962,1963,5,40,0,0,1963,1968,5,173,0,
	0,1964,1965,3,100,50,0,1965,1966,7,18,0,0,1966,1968,1,0,0,0,1967,1958,1,
	0,0,0,1967,1960,1,0,0,0,1967,1962,1,0,0,0,1967,1964,1,0,0,0,1968,145,1,
	0,0,0,1969,1970,3,170,85,0,1970,1971,5,229,0,0,1971,1972,3,100,50,0,1972,
	147,1,0,0,0,1973,1974,5,77,0,0,1974,1978,7,19,0,0,1975,1976,5,206,0,0,1976,
	1978,7,20,0,0,1977,1973,1,0,0,0,1977,1975,1,0,0,0,1978,149,1,0,0,0,1979,
	1980,5,104,0,0,1980,1981,5,112,0,0,1981,1985,3,152,76,0,1982,1983,5,155,
	0,0,1983,1985,7,21,0,0,1984,1979,1,0,0,0,1984,1982,1,0,0,0,1985,151,1,0,
	0,0,1986,1987,5,155,0,0,1987,1994,5,209,0,0,1988,1989,5,155,0,0,1989,1994,
	5,35,0,0,1990,1991,5,160,0,0,1991,1994,5,155,0,0,1992,1994,5,180,0,0,1993,
	1986,1,0,0,0,1993,1988,1,0,0,0,1993,1990,1,0,0,0,1993,1992,1,0,0,0,1994,
	153,1,0,0,0,1995,2001,3,100,50,0,1996,1997,3,170,85,0,1997,1998,5,9,0,0,
	1998,1999,3,100,50,0,1999,2001,1,0,0,0,2000,1995,1,0,0,0,2000,1996,1,0,
	0,0,2001,155,1,0,0,0,2002,2007,5,179,0,0,2003,2007,5,51,0,0,2004,2007,5,
	97,0,0,2005,2007,3,170,85,0,2006,2002,1,0,0,0,2006,2003,1,0,0,0,2006,2004,
	1,0,0,0,2006,2005,1,0,0,0,2007,157,1,0,0,0,2008,2013,3,170,85,0,2009,2010,
	5,1,0,0,2010,2012,3,170,85,0,2011,2009,1,0,0,0,2012,2015,1,0,0,0,2013,2011,
	1,0,0,0,2013,2014,1,0,0,0,2014,159,1,0,0,0,2015,2013,1,0,0,0,2016,2017,
	5,76,0,0,2017,2018,7,22,0,0,2018,2019,3,162,81,0,2019,2020,3,106,53,0,2020,
	161,1,0,0,0,2021,2022,5,18,0,0,2022,2025,5,135,0,0,2023,2025,5,21,0,0,2024,
	2021,1,0,0,0,2024,2023,1,0,0,0,2025,163,1,0,0,0,2026,2030,5,45,0,0,2027,
	2030,5,42,0,0,2028,2030,3,166,83,0,2029,2026,1,0,0,0,2029,2027,1,0,0,0,
	2029,2028,1,0,0,0,2030,165,1,0,0,0,2031,2032,5,215,0,0,2032,2037,3,170,
	85,0,2033,2034,5,169,0,0,2034,2037,3,170,85,0,2035,2037,3,170,85,0,2036,
	2031,1,0,0,0,2036,2033,1,0,0,0,2036,2035,1,0,0,0,2037,167,1,0,0,0,2038,
	2043,3,170,85,0,2039,2040,5,4,0,0,2040,2042,3,170,85,0,2041,2039,1,0,0,
	0,2042,2045,1,0,0,0,2043,2041,1,0,0,0,2043,2044,1,0,0,0,2044,169,1,0,0,
	0,2045,2043,1,0,0,0,2046,2052,5,247,0,0,2047,2052,5,249,0,0,2048,2052,3,
	192,96,0,2049,2052,5,250,0,0,2050,2052,5,248,0,0,2051,2046,1,0,0,0,2051,
	2047,1,0,0,0,2051,2048,1,0,0,0,2051,2049,1,0,0,0,2051,2050,1,0,0,0,2052,
	171,1,0,0,0,2053,2057,5,245,0,0,2054,2057,5,246,0,0,2055,2057,5,244,0,0,
	2056,2053,1,0,0,0,2056,2054,1,0,0,0,2056,2055,1,0,0,0,2057,173,1,0,0,0,
	2058,2061,3,176,88,0,2059,2061,3,178,89,0,2060,2058,1,0,0,0,2060,2059,1,
	0,0,0,2061,175,1,0,0,0,2062,2063,5,36,0,0,2063,2064,3,170,85,0,2064,2065,
	3,178,89,0,2065,177,1,0,0,0,2066,2067,3,180,90,0,2067,2069,3,96,48,0,2068,
	2070,3,182,91,0,2069,2068,1,0,0,0,2069,2070,1,0,0,0,2070,179,1,0,0,0,2071,
	2075,5,211,0,0,2072,2073,5,151,0,0,2073,2075,5,107,0,0,2074,2071,1,0,0,
	0,2074,2072,1,0,0,0,2075,181,1,0,0,0,2076,2078,3,184,92,0,2077,2076,1,0,
	0,0,2078,2081,1,0,0,0,2079,2077,1,0,0,0,2079,2080,1,0,0,0,2080,183,1,0,
	0,0,2081,2079,1,0,0,0,2082,2086,3,188,94,0,2083,2086,3,186,93,0,2084,2086,
	3,190,95,0,2085,2082,1,0,0,0,2085,2083,1,0,0,0,2085,2084,1,0,0,0,2086,185,
	1,0,0,0,2087,2091,5,158,0,0,2088,2089,5,131,0,0,2089,2091,5,158,0,0,2090,
	2087,1,0,0,0,2090,2088,1,0,0,0,2091,187,1,0,0,0,2092,2093,7,23,0,0,2093,
	189,1,0,0,0,2094,2098,5,62,0,0,2095,2096,5,131,0,0,2096,2098,5,62,0,0,2097,
	2094,1,0,0,0,2097,2095,1,0,0,0,2098,191,1,0,0,0,2099,2100,7,24,0,0,2100,
	193,1,0,0,0,262,231,236,242,246,260,264,268,272,280,284,287,294,303,309,
	313,319,326,335,344,355,362,372,379,387,395,403,413,420,428,433,444,449,
	460,471,483,489,494,500,509,520,529,534,538,546,553,566,569,579,582,589,
	598,604,609,613,623,626,636,649,655,660,666,675,681,688,696,701,705,713,
	719,726,731,735,745,748,752,755,763,768,793,799,805,807,813,819,821,829,
	831,850,855,862,874,876,884,886,904,907,911,915,933,936,952,957,959,962,
	968,975,981,987,991,995,1001,1009,1024,1031,1036,1043,1051,1055,1060,1071,
	1079,1084,1086,1095,1097,1104,1113,1119,1122,1124,1136,1143,1147,1151,1155,
	1160,1164,1169,1173,1180,1188,1192,1199,1210,1213,1223,1226,1237,1242,1250,
	1253,1257,1261,1272,1275,1282,1301,1305,1309,1313,1317,1321,1323,1334,1339,
	1348,1354,1358,1360,1368,1375,1388,1394,1405,1412,1416,1424,1426,1439,1447,
	1456,1462,1470,1476,1480,1485,1490,1496,1510,1512,1541,1552,1562,1565,1570,
	1577,1580,1589,1592,1596,1599,1602,1614,1617,1636,1640,1648,1652,1677,1680,
	1689,1695,1701,1707,1717,1726,1748,1751,1754,1764,1766,1773,1775,1781,1789,
	1799,1805,1817,1820,1847,1859,1864,1871,1877,1882,1888,1910,1913,1922,1925,
	1928,1956,1967,1977,1984,1993,2000,2006,2013,2024,2029,2036,2043,2051,2056,
	2060,2069,2074,2079,2085,2090,2097];

	private static __ATN: ATN;
	public static get _ATN(): ATN {
		if (!SqlBaseParser.__ATN) {
			SqlBaseParser.__ATN = new ATNDeserializer().deserialize(SqlBaseParser._serializedATN);
		}

		return SqlBaseParser.__ATN;
	}


	static DecisionsToDFA = SqlBaseParser._ATN.decisionToState.map( (ds: DecisionState, index: number) => new DFA(ds, index) );

}

export class SingleStatementContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_singleStatement;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSingleStatement) {
			return visitor.visitSingleStatement(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneExpression;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneExpression) {
			return visitor.visitStandaloneExpression(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneRoutineBodyContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public routineBody(): RoutineBodyContext {
		return this.getTypedRuleContext(RoutineBodyContext, 0) as RoutineBodyContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneRoutineBody;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneRoutineBody) {
			return visitor.visitStandaloneRoutineBody(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneSelectItemListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public selectItemList(): SelectItemListContext {
		return this.getTypedRuleContext(SelectItemListContext, 0) as SelectItemListContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneSelectItemList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneSelectItemList) {
			return visitor.visitStandaloneSelectItemList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneRelationListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public relationList(): RelationListContext {
		return this.getTypedRuleContext(RelationListContext, 0) as RelationListContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneRelationList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneRelationList) {
			return visitor.visitStandaloneRelationList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneBooleanExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneBooleanExpression;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneBooleanExpression) {
			return visitor.visitStandaloneBooleanExpression(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneSortItemListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public sortItemList(): SortItemListContext {
		return this.getTypedRuleContext(SortItemListContext, 0) as SortItemListContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneSortItemList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneSortItemList) {
			return visitor.visitStandaloneSortItemList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StandaloneIntegerValueContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.INTEGER_VALUE, 0);
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlBaseParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_standaloneIntegerValue;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStandaloneIntegerValue) {
			return visitor.visitStandaloneIntegerValue(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StatementContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_statement;
	}
	public override copyFrom(ctx: StatementContext): void {
		super.copyFrom(ctx);
	}
}
export class ExplainContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXPLAIN(): TerminalNode {
		return this.getToken(SqlBaseParser.EXPLAIN, 0);
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlBaseParser.ANALYZE, 0);
	}
	public VERBOSE(): TerminalNode {
		return this.getToken(SqlBaseParser.VERBOSE, 0);
	}
	public explainOption_list(): ExplainOptionContext[] {
		return this.getTypedRuleContexts(ExplainOptionContext) as ExplainOptionContext[];
	}
	public explainOption(i: number): ExplainOptionContext {
		return this.getTypedRuleContext(ExplainOptionContext, i) as ExplainOptionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExplain) {
			return visitor.visitExplain(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class PrepareContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public PREPARE(): TerminalNode {
		return this.getToken(SqlBaseParser.PREPARE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitPrepare) {
			return visitor.visitPrepare(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropMaterializedViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlBaseParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropMaterializedView) {
			return visitor.visitDropMaterializedView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UseContext extends StatementContext {
	public _schema!: IdentifierContext;
	public _catalog!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public USE(): TerminalNode {
		return this.getToken(SqlBaseParser.USE, 0);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUse) {
			return visitor.visitUse(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AddConstraintContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlBaseParser.ADD, 0);
	}
	public constraintSpecification(): ConstraintSpecificationContext {
		return this.getTypedRuleContext(ConstraintSpecificationContext, 0) as ConstraintSpecificationContext;
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAddConstraint) {
			return visitor.visitAddConstraint(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DeallocateContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DEALLOCATE(): TerminalNode {
		return this.getToken(SqlBaseParser.DEALLOCATE, 0);
	}
	public PREPARE(): TerminalNode {
		return this.getToken(SqlBaseParser.PREPARE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDeallocate) {
			return visitor.visitDeallocate(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RenameTableContext extends StatementContext {
	public _from_!: QualifiedNameContext;
	public _to!: QualifiedNameContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlBaseParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRenameTable) {
			return visitor.visitRenameTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CommitContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public COMMIT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMIT, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlBaseParser.WORK, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCommit) {
			return visitor.visitCommit(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateRoleContext extends StatementContext {
	public _name!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlBaseParser.ADMIN, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateRole) {
			return visitor.visitCreateRole(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCreateFunctionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public types(): TypesContext {
		return this.getTypedRuleContext(TypesContext, 0) as TypesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCreateFunction) {
			return visitor.visitShowCreateFunction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: QualifiedNameContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, i);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropColumn) {
			return visitor.visitDropColumn(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropView) {
			return visitor.visitDropView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowTablesContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public TABLES(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLES, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowTables) {
			return visitor.visitShowTables(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCatalogsContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CATALOGS(): TerminalNode {
		return this.getToken(SqlBaseParser.CATALOGS, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCatalogs) {
			return visitor.visitShowCatalogs(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowRolesContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public ROLES(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLES, 0);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowRoles) {
			return visitor.visitShowRoles(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RenameColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _from_!: IdentifierContext;
	public _to!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlBaseParser.RENAME, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, i);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRenameColumn) {
			return visitor.visitRenameColumn(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RevokeRolesContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlBaseParser.REVOKE, 0);
	}
	public roles(): RolesContext {
		return this.getTypedRuleContext(RolesContext, 0) as RolesContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public principal_list(): PrincipalContext[] {
		return this.getTypedRuleContexts(PrincipalContext) as PrincipalContext[];
	}
	public principal(i: number): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, i) as PrincipalContext;
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlBaseParser.ADMIN, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlBaseParser.OPTION, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTED, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlBaseParser.BY, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRevokeRoles) {
			return visitor.visitRevokeRoles(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCreateTableContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCreateTable) {
			return visitor.visitShowCreateTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowColumnsContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public COLUMNS(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMNS, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlBaseParser.DESCRIBE, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlBaseParser.DESC, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowColumns) {
			return visitor.visitShowColumns(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowRoleGrantsContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTS, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowRoleGrants) {
			return visitor.visitShowRoleGrants(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AddColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: ColumnDefinitionContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlBaseParser.ADD, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public columnDefinition(): ColumnDefinitionContext {
		return this.getTypedRuleContext(ColumnDefinitionContext, 0) as ColumnDefinitionContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, i);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAddColumn) {
			return visitor.visitAddColumn(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ResetSessionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public RESET(): TerminalNode {
		return this.getToken(SqlBaseParser.RESET, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SESSION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitResetSession) {
			return visitor.visitResetSession(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropConstraintContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _name!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public CONSTRAINT(): TerminalNode {
		return this.getToken(SqlBaseParser.CONSTRAINT, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, i);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropConstraint) {
			return visitor.visitDropConstraint(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class InsertIntoContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public INSERT(): TerminalNode {
		return this.getToken(SqlBaseParser.INSERT, 0);
	}
	public INTO(): TerminalNode {
		return this.getToken(SqlBaseParser.INTO, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitInsertInto) {
			return visitor.visitInsertInto(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowSessionContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SESSION, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowSession) {
			return visitor.visitShowSession(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateSchemaContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateSchema) {
			return visitor.visitCreateSchema(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ExecuteContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXECUTE(): TerminalNode {
		return this.getToken(SqlBaseParser.EXECUTE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public USING(): TerminalNode {
		return this.getToken(SqlBaseParser.USING, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExecute) {
			return visitor.visitExecute(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RenameSchemaContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlBaseParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRenameSchema) {
			return visitor.visitRenameSchema(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropRoleContext extends StatementContext {
	public _name!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropRole) {
			return visitor.visitDropRole(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AnalyzeContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlBaseParser.ANALYZE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAnalyze) {
			return visitor.visitAnalyze(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SetRoleContext extends StatementContext {
	public _role!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public NONE(): TerminalNode {
		return this.getToken(SqlBaseParser.NONE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSetRole) {
			return visitor.visitSetRole(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateFunctionContext extends StatementContext {
	public _functionName!: QualifiedNameContext;
	public _returnType!: TypeContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTION, 0);
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlBaseParser.RETURNS, 0);
	}
	public routineCharacteristics(): RoutineCharacteristicsContext {
		return this.getTypedRuleContext(RoutineCharacteristicsContext, 0) as RoutineCharacteristicsContext;
	}
	public routineBody(): RoutineBodyContext {
		return this.getTypedRuleContext(RoutineBodyContext, 0) as RoutineBodyContext;
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
	public OR(): TerminalNode {
		return this.getToken(SqlBaseParser.OR, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlBaseParser.REPLACE, 0);
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlBaseParser.TEMPORARY, 0);
	}
	public sqlParameterDeclaration_list(): SqlParameterDeclarationContext[] {
		return this.getTypedRuleContexts(SqlParameterDeclarationContext) as SqlParameterDeclarationContext[];
	}
	public sqlParameterDeclaration(i: number): SqlParameterDeclarationContext {
		return this.getTypedRuleContext(SqlParameterDeclarationContext, i) as SqlParameterDeclarationContext;
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateFunction) {
			return visitor.visitCreateFunction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowGrantsContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTS, 0);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlBaseParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowGrants) {
			return visitor.visitShowGrants(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropSchemaContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public CASCADE(): TerminalNode {
		return this.getToken(SqlBaseParser.CASCADE, 0);
	}
	public RESTRICT(): TerminalNode {
		return this.getToken(SqlBaseParser.RESTRICT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropSchema) {
			return visitor.visitDropSchema(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCreateViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCreateView) {
			return visitor.visitShowCreateView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateTableContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public tableElement_list(): TableElementContext[] {
		return this.getTypedRuleContexts(TableElementContext) as TableElementContext[];
	}
	public tableElement(i: number): TableElementContext {
		return this.getTypedRuleContext(TableElementContext, i) as TableElementContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateTable) {
			return visitor.visitCreateTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class StartTransactionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public START(): TerminalNode {
		return this.getToken(SqlBaseParser.START, 0);
	}
	public TRANSACTION(): TerminalNode {
		return this.getToken(SqlBaseParser.TRANSACTION, 0);
	}
	public transactionMode_list(): TransactionModeContext[] {
		return this.getTypedRuleContexts(TransactionModeContext) as TransactionModeContext[];
	}
	public transactionMode(i: number): TransactionModeContext {
		return this.getTypedRuleContext(TransactionModeContext, i) as TransactionModeContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStartTransaction) {
			return visitor.visitStartTransaction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateTableAsSelectContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.WITH);
	}
	public WITH(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, i);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	public DATA(): TerminalNode {
		return this.getToken(SqlBaseParser.DATA, 0);
	}
	public NO(): TerminalNode {
		return this.getToken(SqlBaseParser.NO, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateTableAsSelect) {
			return visitor.visitCreateTableAsSelect(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowStatsContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlBaseParser.STATS, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowStats) {
			return visitor.visitShowStats(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCreateSchemaContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCreateSchema) {
			return visitor.visitShowCreateSchema(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropFunctionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlBaseParser.TEMPORARY, 0);
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public types(): TypesContext {
		return this.getTypedRuleContext(TypesContext, 0) as TypesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropFunction) {
			return visitor.visitDropFunction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RevokeContext extends StatementContext {
	public _grantee!: PrincipalContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlBaseParser.REVOKE, 0);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlBaseParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public principal(): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, 0) as PrincipalContext;
	}
	public privilege_list(): PrivilegeContext[] {
		return this.getTypedRuleContexts(PrivilegeContext) as PrivilegeContext[];
	}
	public privilege(i: number): PrivilegeContext {
		return this.getTypedRuleContext(PrivilegeContext, i) as PrivilegeContext;
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlBaseParser.PRIVILEGES, 0);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANT, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlBaseParser.OPTION, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRevoke) {
			return visitor.visitRevoke(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UpdateContext extends StatementContext {
	public _where!: BooleanExpressionContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UPDATE(): TerminalNode {
		return this.getToken(SqlBaseParser.UPDATE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public updateAssignment_list(): UpdateAssignmentContext[] {
		return this.getTypedRuleContexts(UpdateAssignmentContext) as UpdateAssignmentContext[];
	}
	public updateAssignment(i: number): UpdateAssignmentContext {
		return this.getTypedRuleContext(UpdateAssignmentContext, i) as UpdateAssignmentContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlBaseParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUpdate) {
			return visitor.visitUpdate(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateTypeContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlBaseParser.TYPE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public sqlParameterDeclaration_list(): SqlParameterDeclarationContext[] {
		return this.getTypedRuleContexts(SqlParameterDeclarationContext) as SqlParameterDeclarationContext[];
	}
	public sqlParameterDeclaration(i: number): SqlParameterDeclarationContext {
		return this.getTypedRuleContext(SqlParameterDeclarationContext, i) as SqlParameterDeclarationContext;
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateType) {
			return visitor.visitCreateType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DeleteContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DELETE(): TerminalNode {
		return this.getToken(SqlBaseParser.DELETE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlBaseParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDelete) {
			return visitor.visitDelete(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DescribeInputContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlBaseParser.DESCRIBE, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlBaseParser.INPUT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDescribeInput) {
			return visitor.visitDescribeInput(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowStatsForQueryContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlBaseParser.STATS, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	public querySpecification(): QuerySpecificationContext {
		return this.getTypedRuleContext(QuerySpecificationContext, 0) as QuerySpecificationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowStatsForQuery) {
			return visitor.visitShowStatsForQuery(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class StatementDefaultContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStatementDefault) {
			return visitor.visitStatementDefault(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TruncateTableContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TRUNCATE(): TerminalNode {
		return this.getToken(SqlBaseParser.TRUNCATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTruncateTable) {
			return visitor.visitTruncateTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AlterColumnSetNotNullContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.ALTER);
	}
	public ALTER(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, i);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAlterColumnSetNotNull) {
			return visitor.visitAlterColumnSetNotNull(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateMaterializedViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlBaseParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateMaterializedView) {
			return visitor.visitCreateMaterializedView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AlterFunctionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public alterRoutineCharacteristics(): AlterRoutineCharacteristicsContext {
		return this.getTypedRuleContext(AlterRoutineCharacteristicsContext, 0) as AlterRoutineCharacteristicsContext;
	}
	public types(): TypesContext {
		return this.getTypedRuleContext(TypesContext, 0) as TypesContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAlterFunction) {
			return visitor.visitAlterFunction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SetSessionContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SESSION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlBaseParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSetSession) {
			return visitor.visitSetSession(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CreateViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public OR(): TerminalNode {
		return this.getToken(SqlBaseParser.OR, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlBaseParser.REPLACE, 0);
	}
	public SECURITY(): TerminalNode {
		return this.getToken(SqlBaseParser.SECURITY, 0);
	}
	public DEFINER(): TerminalNode {
		return this.getToken(SqlBaseParser.DEFINER, 0);
	}
	public INVOKER(): TerminalNode {
		return this.getToken(SqlBaseParser.INVOKER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCreateView) {
			return visitor.visitCreateView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowSchemasContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public SCHEMAS(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMAS, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowSchemas) {
			return visitor.visitShowSchemas(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DropTableContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDropTable) {
			return visitor.visitDropTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RollbackContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLLBACK(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLLBACK, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlBaseParser.WORK, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRollback) {
			return visitor.visitRollback(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RenameViewContext extends StatementContext {
	public _from_!: QualifiedNameContext;
	public _to!: QualifiedNameContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlBaseParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRenameView) {
			return visitor.visitRenameView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AlterColumnDropNotNullContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.ALTER);
	}
	public ALTER(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, i);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlBaseParser.DROP, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAlterColumnDropNotNull) {
			return visitor.visitAlterColumnDropNotNull(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class GrantRolesContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANT, 0);
	}
	public roles(): RolesContext {
		return this.getTypedRuleContext(RolesContext, 0) as RolesContext;
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public principal_list(): PrincipalContext[] {
		return this.getTypedRuleContexts(PrincipalContext) as PrincipalContext[];
	}
	public principal(i: number): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, i) as PrincipalContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlBaseParser.ADMIN, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlBaseParser.OPTION, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTED, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlBaseParser.BY, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitGrantRoles) {
			return visitor.visitGrantRoles(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CallContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CALL(): TerminalNode {
		return this.getToken(SqlBaseParser.CALL, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public callArgument_list(): CallArgumentContext[] {
		return this.getTypedRuleContexts(CallArgumentContext) as CallArgumentContext[];
	}
	public callArgument(i: number): CallArgumentContext {
		return this.getTypedRuleContext(CallArgumentContext, i) as CallArgumentContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCall) {
			return visitor.visitCall(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RefreshMaterializedViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REFRESH(): TerminalNode {
		return this.getToken(SqlBaseParser.REFRESH, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlBaseParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlBaseParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRefreshMaterializedView) {
			return visitor.visitRefreshMaterializedView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowCreateMaterializedViewContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CREATE, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlBaseParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowCreateMaterializedView) {
			return visitor.visitShowCreateMaterializedView(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ShowFunctionsContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public FUNCTIONS(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTIONS, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitShowFunctions) {
			return visitor.visitShowFunctions(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DescribeOutputContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlBaseParser.DESCRIBE, 0);
	}
	public OUTPUT(): TerminalNode {
		return this.getToken(SqlBaseParser.OUTPUT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDescribeOutput) {
			return visitor.visitDescribeOutput(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class GrantContext extends StatementContext {
	public _grantee!: PrincipalContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GRANT_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.GRANT);
	}
	public GRANT(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.GRANT, i);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlBaseParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public principal(): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, 0) as PrincipalContext;
	}
	public privilege_list(): PrivilegeContext[] {
		return this.getTypedRuleContexts(PrivilegeContext) as PrivilegeContext[];
	}
	public privilege(i: number): PrivilegeContext {
		return this.getTypedRuleContext(PrivilegeContext, i) as PrivilegeContext;
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlBaseParser.PRIVILEGES, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlBaseParser.OPTION, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitGrant) {
			return visitor.visitGrant(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SetTablePropertiesContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlBaseParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlBaseParser.PROPERTIES, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSetTableProperties) {
			return visitor.visitSetTableProperties(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QueryContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public queryNoWith(): QueryNoWithContext {
		return this.getTypedRuleContext(QueryNoWithContext, 0) as QueryNoWithContext;
	}
	public with_(): WithContext {
		return this.getTypedRuleContext(WithContext, 0) as WithContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_query;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQuery) {
			return visitor.visitQuery(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class WithContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public namedQuery_list(): NamedQueryContext[] {
		return this.getTypedRuleContexts(NamedQueryContext) as NamedQueryContext[];
	}
	public namedQuery(i: number): NamedQueryContext {
		return this.getTypedRuleContext(NamedQueryContext, i) as NamedQueryContext;
	}
	public RECURSIVE(): TerminalNode {
		return this.getToken(SqlBaseParser.RECURSIVE, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_with;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitWith) {
			return visitor.visitWith(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TableElementContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public constraintSpecification(): ConstraintSpecificationContext {
		return this.getTypedRuleContext(ConstraintSpecificationContext, 0) as ConstraintSpecificationContext;
	}
	public columnDefinition(): ColumnDefinitionContext {
		return this.getTypedRuleContext(ColumnDefinitionContext, 0) as ColumnDefinitionContext;
	}
	public likeClause(): LikeClauseContext {
		return this.getTypedRuleContext(LikeClauseContext, 0) as LikeClauseContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_tableElement;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTableElement) {
			return visitor.visitTableElement(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ColumnDefinitionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_columnDefinition;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitColumnDefinition) {
			return visitor.visitColumnDefinition(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class LikeClauseContext extends ParserRuleContext {
	public _optionType!: Token;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlBaseParser.PROPERTIES, 0);
	}
	public INCLUDING(): TerminalNode {
		return this.getToken(SqlBaseParser.INCLUDING, 0);
	}
	public EXCLUDING(): TerminalNode {
		return this.getToken(SqlBaseParser.EXCLUDING, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_likeClause;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLikeClause) {
			return visitor.visitLikeClause(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PropertiesContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public property_list(): PropertyContext[] {
		return this.getTypedRuleContexts(PropertyContext) as PropertyContext[];
	}
	public property(i: number): PropertyContext {
		return this.getTypedRuleContext(PropertyContext, i) as PropertyContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_properties;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitProperties) {
			return visitor.visitProperties(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PropertyContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlBaseParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_property;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitProperty) {
			return visitor.visitProperty(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SqlParameterDeclarationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_sqlParameterDeclaration;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSqlParameterDeclaration) {
			return visitor.visitSqlParameterDeclaration(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RoutineCharacteristicsContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public routineCharacteristic_list(): RoutineCharacteristicContext[] {
		return this.getTypedRuleContexts(RoutineCharacteristicContext) as RoutineCharacteristicContext[];
	}
	public routineCharacteristic(i: number): RoutineCharacteristicContext {
		return this.getTypedRuleContext(RoutineCharacteristicContext, i) as RoutineCharacteristicContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_routineCharacteristics;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRoutineCharacteristics) {
			return visitor.visitRoutineCharacteristics(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RoutineCharacteristicContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public LANGUAGE(): TerminalNode {
		return this.getToken(SqlBaseParser.LANGUAGE, 0);
	}
	public language(): LanguageContext {
		return this.getTypedRuleContext(LanguageContext, 0) as LanguageContext;
	}
	public determinism(): DeterminismContext {
		return this.getTypedRuleContext(DeterminismContext, 0) as DeterminismContext;
	}
	public nullCallClause(): NullCallClauseContext {
		return this.getTypedRuleContext(NullCallClauseContext, 0) as NullCallClauseContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_routineCharacteristic;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRoutineCharacteristic) {
			return visitor.visitRoutineCharacteristic(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class AlterRoutineCharacteristicsContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public alterRoutineCharacteristic_list(): AlterRoutineCharacteristicContext[] {
		return this.getTypedRuleContexts(AlterRoutineCharacteristicContext) as AlterRoutineCharacteristicContext[];
	}
	public alterRoutineCharacteristic(i: number): AlterRoutineCharacteristicContext {
		return this.getTypedRuleContext(AlterRoutineCharacteristicContext, i) as AlterRoutineCharacteristicContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_alterRoutineCharacteristics;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAlterRoutineCharacteristics) {
			return visitor.visitAlterRoutineCharacteristics(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class AlterRoutineCharacteristicContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public nullCallClause(): NullCallClauseContext {
		return this.getTypedRuleContext(NullCallClauseContext, 0) as NullCallClauseContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_alterRoutineCharacteristic;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAlterRoutineCharacteristic) {
			return visitor.visitAlterRoutineCharacteristic(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RoutineBodyContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public returnStatement(): ReturnStatementContext {
		return this.getTypedRuleContext(ReturnStatementContext, 0) as ReturnStatementContext;
	}
	public externalBodyReference(): ExternalBodyReferenceContext {
		return this.getTypedRuleContext(ExternalBodyReferenceContext, 0) as ExternalBodyReferenceContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_routineBody;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRoutineBody) {
			return visitor.visitRoutineBody(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ReturnStatementContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RETURN(): TerminalNode {
		return this.getToken(SqlBaseParser.RETURN, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_returnStatement;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitReturnStatement) {
			return visitor.visitReturnStatement(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ExternalBodyReferenceContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public EXTERNAL(): TerminalNode {
		return this.getToken(SqlBaseParser.EXTERNAL, 0);
	}
	public NAME(): TerminalNode {
		return this.getToken(SqlBaseParser.NAME, 0);
	}
	public externalRoutineName(): ExternalRoutineNameContext {
		return this.getTypedRuleContext(ExternalRoutineNameContext, 0) as ExternalRoutineNameContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_externalBodyReference;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExternalBodyReference) {
			return visitor.visitExternalBodyReference(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class LanguageContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SQL(): TerminalNode {
		return this.getToken(SqlBaseParser.SQL, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_language;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLanguage) {
			return visitor.visitLanguage(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class DeterminismContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public DETERMINISTIC(): TerminalNode {
		return this.getToken(SqlBaseParser.DETERMINISTIC, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_determinism;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDeterminism) {
			return visitor.visitDeterminism(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NullCallClauseContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlBaseParser.RETURNS, 0);
	}
	public NULL_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.NULL);
	}
	public NULL(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, i);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlBaseParser.ON, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlBaseParser.INPUT, 0);
	}
	public CALLED(): TerminalNode {
		return this.getToken(SqlBaseParser.CALLED, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_nullCallClause;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNullCallClause) {
			return visitor.visitNullCallClause(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ExternalRoutineNameContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_externalRoutineName;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExternalRoutineName) {
			return visitor.visitExternalRoutineName(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QueryNoWithContext extends ParserRuleContext {
	public _offset!: Token;
	public _limit!: Token;
	public _fetchFirstNRows!: Token;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public queryTerm(): QueryTermContext {
		return this.getTypedRuleContext(QueryTermContext, 0) as QueryTermContext;
	}
	public ORDER(): TerminalNode {
		return this.getToken(SqlBaseParser.ORDER, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlBaseParser.BY, 0);
	}
	public sortItemList(): SortItemListContext {
		return this.getTypedRuleContext(SortItemListContext, 0) as SortItemListContext;
	}
	public OFFSET(): TerminalNode {
		return this.getToken(SqlBaseParser.OFFSET, 0);
	}
	public INTEGER_VALUE_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.INTEGER_VALUE);
	}
	public INTEGER_VALUE(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.INTEGER_VALUE, i);
	}
	public LIMIT(): TerminalNode {
		return this.getToken(SqlBaseParser.LIMIT, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlBaseParser.ROW, 0);
	}
	public ROWS_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.ROWS);
	}
	public ROWS(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.ROWS, i);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public FETCH(): TerminalNode {
		return this.getToken(SqlBaseParser.FETCH, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlBaseParser.FIRST, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlBaseParser.ONLY, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_queryNoWith;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQueryNoWith) {
			return visitor.visitQueryNoWith(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SortItemListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public sortItem_list(): SortItemContext[] {
		return this.getTypedRuleContexts(SortItemContext) as SortItemContext[];
	}
	public sortItem(i: number): SortItemContext {
		return this.getTypedRuleContext(SortItemContext, i) as SortItemContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_sortItemList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSortItemList) {
			return visitor.visitSortItemList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QueryTermContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_queryTerm;
	}
	public override copyFrom(ctx: QueryTermContext): void {
		super.copyFrom(ctx);
	}
}
export class QueryTermDefaultContext extends QueryTermContext {
	constructor(parser: SqlBaseParser, ctx: QueryTermContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public queryPrimary(): QueryPrimaryContext {
		return this.getTypedRuleContext(QueryPrimaryContext, 0) as QueryPrimaryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQueryTermDefault) {
			return visitor.visitQueryTermDefault(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SetOperationContext extends QueryTermContext {
	public _left!: QueryTermContext;
	public _operator!: Token;
	public _right!: QueryTermContext;
	constructor(parser: SqlBaseParser, ctx: QueryTermContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public queryTerm_list(): QueryTermContext[] {
		return this.getTypedRuleContexts(QueryTermContext) as QueryTermContext[];
	}
	public queryTerm(i: number): QueryTermContext {
		return this.getTypedRuleContext(QueryTermContext, i) as QueryTermContext;
	}
	public INTERSECT(): TerminalNode {
		return this.getToken(SqlBaseParser.INTERSECT, 0);
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
	public UNION(): TerminalNode {
		return this.getToken(SqlBaseParser.UNION, 0);
	}
	public EXCEPT(): TerminalNode {
		return this.getToken(SqlBaseParser.EXCEPT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSetOperation) {
			return visitor.visitSetOperation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QueryPrimaryContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_queryPrimary;
	}
	public override copyFrom(ctx: QueryPrimaryContext): void {
		super.copyFrom(ctx);
	}
}
export class SubqueryContext extends QueryPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public queryNoWith(): QueryNoWithContext {
		return this.getTypedRuleContext(QueryNoWithContext, 0) as QueryNoWithContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSubquery) {
			return visitor.visitSubquery(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class QueryPrimaryDefaultContext extends QueryPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public querySpecification(): QuerySpecificationContext {
		return this.getTypedRuleContext(QuerySpecificationContext, 0) as QuerySpecificationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQueryPrimaryDefault) {
			return visitor.visitQueryPrimaryDefault(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TableContext extends QueryPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTable) {
			return visitor.visitTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class InlineTableContext extends QueryPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public VALUES(): TerminalNode {
		return this.getToken(SqlBaseParser.VALUES, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitInlineTable) {
			return visitor.visitInlineTable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SortItemContext extends ParserRuleContext {
	public _ordering!: Token;
	public _nullOrdering!: Token;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlBaseParser.NULLS, 0);
	}
	public ASC(): TerminalNode {
		return this.getToken(SqlBaseParser.ASC, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlBaseParser.DESC, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlBaseParser.FIRST, 0);
	}
	public LAST(): TerminalNode {
		return this.getToken(SqlBaseParser.LAST, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_sortItem;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSortItem) {
			return visitor.visitSortItem(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QuerySpecificationContext extends ParserRuleContext {
	public _where!: BooleanExpressionContext;
	public _having!: BooleanExpressionContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SELECT(): TerminalNode {
		return this.getToken(SqlBaseParser.SELECT, 0);
	}
	public selectItemList(): SelectItemListContext {
		return this.getTypedRuleContext(SelectItemListContext, 0) as SelectItemListContext;
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public relationList(): RelationListContext {
		return this.getTypedRuleContext(RelationListContext, 0) as RelationListContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlBaseParser.WHERE, 0);
	}
	public GROUP(): TerminalNode {
		return this.getToken(SqlBaseParser.GROUP, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlBaseParser.BY, 0);
	}
	public groupBy(): GroupByContext {
		return this.getTypedRuleContext(GroupByContext, 0) as GroupByContext;
	}
	public HAVING(): TerminalNode {
		return this.getToken(SqlBaseParser.HAVING, 0);
	}
	public booleanExpression_list(): BooleanExpressionContext[] {
		return this.getTypedRuleContexts(BooleanExpressionContext) as BooleanExpressionContext[];
	}
	public booleanExpression(i: number): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, i) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_querySpecification;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQuerySpecification) {
			return visitor.visitQuerySpecification(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SelectItemListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public selectItem_list(): SelectItemContext[] {
		return this.getTypedRuleContexts(SelectItemContext) as SelectItemContext[];
	}
	public selectItem(i: number): SelectItemContext {
		return this.getTypedRuleContext(SelectItemContext, i) as SelectItemContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_selectItemList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSelectItemList) {
			return visitor.visitSelectItemList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RelationListContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public relation_list(): RelationContext[] {
		return this.getTypedRuleContexts(RelationContext) as RelationContext[];
	}
	public relation(i: number): RelationContext {
		return this.getTypedRuleContext(RelationContext, i) as RelationContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_relationList;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRelationList) {
			return visitor.visitRelationList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class GroupByContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public groupingElement_list(): GroupingElementContext[] {
		return this.getTypedRuleContexts(GroupingElementContext) as GroupingElementContext[];
	}
	public groupingElement(i: number): GroupingElementContext {
		return this.getTypedRuleContext(GroupingElementContext, i) as GroupingElementContext;
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_groupBy;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitGroupBy) {
			return visitor.visitGroupBy(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class GroupingElementContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_groupingElement;
	}
	public override copyFrom(ctx: GroupingElementContext): void {
		super.copyFrom(ctx);
	}
}
export class MultipleGroupingSetsContext extends GroupingElementContext {
	constructor(parser: SqlBaseParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GROUPING(): TerminalNode {
		return this.getToken(SqlBaseParser.GROUPING, 0);
	}
	public SETS(): TerminalNode {
		return this.getToken(SqlBaseParser.SETS, 0);
	}
	public groupingSet_list(): GroupingSetContext[] {
		return this.getTypedRuleContexts(GroupingSetContext) as GroupingSetContext[];
	}
	public groupingSet(i: number): GroupingSetContext {
		return this.getTypedRuleContext(GroupingSetContext, i) as GroupingSetContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitMultipleGroupingSets) {
			return visitor.visitMultipleGroupingSets(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SingleGroupingSetContext extends GroupingElementContext {
	constructor(parser: SqlBaseParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public groupingSet(): GroupingSetContext {
		return this.getTypedRuleContext(GroupingSetContext, 0) as GroupingSetContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSingleGroupingSet) {
			return visitor.visitSingleGroupingSet(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CubeContext extends GroupingElementContext {
	constructor(parser: SqlBaseParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CUBE(): TerminalNode {
		return this.getToken(SqlBaseParser.CUBE, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCube) {
			return visitor.visitCube(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RollupContext extends GroupingElementContext {
	constructor(parser: SqlBaseParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLLUP(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLLUP, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRollup) {
			return visitor.visitRollup(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class GroupingSetContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_groupingSet;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitGroupingSet) {
			return visitor.visitGroupingSet(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NamedQueryContext extends ParserRuleContext {
	public _name!: IdentifierContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_namedQuery;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNamedQuery) {
			return visitor.visitNamedQuery(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SetQuantifierContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public DISTINCT(): TerminalNode {
		return this.getToken(SqlBaseParser.DISTINCT, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_setQuantifier;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSetQuantifier) {
			return visitor.visitSetQuantifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SelectItemContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_selectItem;
	}
	public override copyFrom(ctx: SelectItemContext): void {
		super.copyFrom(ctx);
	}
}
export class SelectAllContext extends SelectItemContext {
	constructor(parser: SqlBaseParser, ctx: SelectItemContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public ASTERISK(): TerminalNode {
		return this.getToken(SqlBaseParser.ASTERISK, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSelectAll) {
			return visitor.visitSelectAll(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SelectSingleContext extends SelectItemContext {
	constructor(parser: SqlBaseParser, ctx: SelectItemContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSelectSingle) {
			return visitor.visitSelectSingle(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RelationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_relation;
	}
	public override copyFrom(ctx: RelationContext): void {
		super.copyFrom(ctx);
	}
}
export class RelationDefaultContext extends RelationContext {
	constructor(parser: SqlBaseParser, ctx: RelationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public sampledRelation(): SampledRelationContext {
		return this.getTypedRuleContext(SampledRelationContext, 0) as SampledRelationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRelationDefault) {
			return visitor.visitRelationDefault(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class JoinRelationContext extends RelationContext {
	public _left!: RelationContext;
	public _right!: SampledRelationContext;
	public _rightRelation!: RelationContext;
	constructor(parser: SqlBaseParser, ctx: RelationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public relation_list(): RelationContext[] {
		return this.getTypedRuleContexts(RelationContext) as RelationContext[];
	}
	public relation(i: number): RelationContext {
		return this.getTypedRuleContext(RelationContext, i) as RelationContext;
	}
	public CROSS(): TerminalNode {
		return this.getToken(SqlBaseParser.CROSS, 0);
	}
	public JOIN(): TerminalNode {
		return this.getToken(SqlBaseParser.JOIN, 0);
	}
	public joinType(): JoinTypeContext {
		return this.getTypedRuleContext(JoinTypeContext, 0) as JoinTypeContext;
	}
	public joinCriteria(): JoinCriteriaContext {
		return this.getTypedRuleContext(JoinCriteriaContext, 0) as JoinCriteriaContext;
	}
	public NATURAL(): TerminalNode {
		return this.getToken(SqlBaseParser.NATURAL, 0);
	}
	public sampledRelation(): SampledRelationContext {
		return this.getTypedRuleContext(SampledRelationContext, 0) as SampledRelationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitJoinRelation) {
			return visitor.visitJoinRelation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class JoinTypeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INNER(): TerminalNode {
		return this.getToken(SqlBaseParser.INNER, 0);
	}
	public LEFT(): TerminalNode {
		return this.getToken(SqlBaseParser.LEFT, 0);
	}
	public OUTER(): TerminalNode {
		return this.getToken(SqlBaseParser.OUTER, 0);
	}
	public RIGHT(): TerminalNode {
		return this.getToken(SqlBaseParser.RIGHT, 0);
	}
	public FULL(): TerminalNode {
		return this.getToken(SqlBaseParser.FULL, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_joinType;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitJoinType) {
			return visitor.visitJoinType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class JoinCriteriaContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ON(): TerminalNode {
		return this.getToken(SqlBaseParser.ON, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	public USING(): TerminalNode {
		return this.getToken(SqlBaseParser.USING, 0);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_joinCriteria;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitJoinCriteria) {
			return visitor.visitJoinCriteria(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SampledRelationContext extends ParserRuleContext {
	public _percentage!: ExpressionContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public aliasedRelation(): AliasedRelationContext {
		return this.getTypedRuleContext(AliasedRelationContext, 0) as AliasedRelationContext;
	}
	public TABLESAMPLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLESAMPLE, 0);
	}
	public sampleType(): SampleTypeContext {
		return this.getTypedRuleContext(SampleTypeContext, 0) as SampleTypeContext;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_sampledRelation;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSampledRelation) {
			return visitor.visitSampledRelation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class SampleTypeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public BERNOULLI(): TerminalNode {
		return this.getToken(SqlBaseParser.BERNOULLI, 0);
	}
	public SYSTEM(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_sampleType;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSampleType) {
			return visitor.visitSampleType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class AliasedRelationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public relationPrimary(): RelationPrimaryContext {
		return this.getTypedRuleContext(RelationPrimaryContext, 0) as RelationPrimaryContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_aliasedRelation;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAliasedRelation) {
			return visitor.visitAliasedRelation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ColumnAliasesContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_columnAliases;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitColumnAliases) {
			return visitor.visitColumnAliases(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RelationPrimaryContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_relationPrimary;
	}
	public override copyFrom(ctx: RelationPrimaryContext): void {
		super.copyFrom(ctx);
	}
}
export class SubqueryRelationContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSubqueryRelation) {
			return visitor.visitSubqueryRelation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ParenthesizedRelationContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public relation(): RelationContext {
		return this.getTypedRuleContext(RelationContext, 0) as RelationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitParenthesizedRelation) {
			return visitor.visitParenthesizedRelation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UnnestContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNNEST(): TerminalNode {
		return this.getToken(SqlBaseParser.UNNEST, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlBaseParser.WITH, 0);
	}
	public ORDINALITY(): TerminalNode {
		return this.getToken(SqlBaseParser.ORDINALITY, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnnest) {
			return visitor.visitUnnest(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class LateralContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public LATERAL(): TerminalNode {
		return this.getToken(SqlBaseParser.LATERAL, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLateral) {
			return visitor.visitLateral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TableNameContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public tableVersionExpression(): TableVersionExpressionContext {
		return this.getTypedRuleContext(TableVersionExpressionContext, 0) as TableVersionExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTableName) {
			return visitor.visitTableName(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_expression;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExpression) {
			return visitor.visitExpression(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class BooleanExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_booleanExpression;
	}
	public override copyFrom(ctx: BooleanExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class LogicalNotContext extends BooleanExpressionContext {
	constructor(parser: SqlBaseParser, ctx: BooleanExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLogicalNot) {
			return visitor.visitLogicalNot(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class PredicatedContext extends BooleanExpressionContext {
	public _valueExpression!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: BooleanExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public predicate(): PredicateContext {
		return this.getTypedRuleContext(PredicateContext, 0) as PredicateContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitPredicated) {
			return visitor.visitPredicated(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class LogicalBinaryContext extends BooleanExpressionContext {
	public _left!: BooleanExpressionContext;
	public _operator!: Token;
	public _right!: BooleanExpressionContext;
	constructor(parser: SqlBaseParser, ctx: BooleanExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public booleanExpression_list(): BooleanExpressionContext[] {
		return this.getTypedRuleContexts(BooleanExpressionContext) as BooleanExpressionContext[];
	}
	public booleanExpression(i: number): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, i) as BooleanExpressionContext;
	}
	public AND(): TerminalNode {
		return this.getToken(SqlBaseParser.AND, 0);
	}
	public OR(): TerminalNode {
		return this.getToken(SqlBaseParser.OR, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLogicalBinary) {
			return visitor.visitLogicalBinary(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PredicateContext extends ParserRuleContext {
	public value: ParserRuleContext;
	constructor(parser: SqlBaseParser, parent: ParserRuleContext, invokingState: number, value: ParserRuleContext) {
		super(parent, invokingState);
    	this.parser = parser;
        this.value = value;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_predicate;
	}
	public override copyFrom(ctx: PredicateContext): void {
		super.copyFrom(ctx);
		this.value = ctx.value;
	}
}
export class ComparisonContext extends PredicateContext {
	public _right!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public comparisonOperator(): ComparisonOperatorContext {
		return this.getTypedRuleContext(ComparisonOperatorContext, 0) as ComparisonOperatorContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitComparison) {
			return visitor.visitComparison(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class LikeContext extends PredicateContext {
	public _pattern!: ValueExpressionContext;
	public _escape!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlBaseParser.LIKE, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.ESCAPE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLike) {
			return visitor.visitLike(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class InSubqueryContext extends PredicateContext {
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitInSubquery) {
			return visitor.visitInSubquery(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DistinctFromContext extends PredicateContext {
	public _right!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IS(): TerminalNode {
		return this.getToken(SqlBaseParser.IS, 0);
	}
	public DISTINCT(): TerminalNode {
		return this.getToken(SqlBaseParser.DISTINCT, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDistinctFrom) {
			return visitor.visitDistinctFrom(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class InListContext extends PredicateContext {
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitInList) {
			return visitor.visitInList(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class NullPredicateContext extends PredicateContext {
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IS(): TerminalNode {
		return this.getToken(SqlBaseParser.IS, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNullPredicate) {
			return visitor.visitNullPredicate(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class BetweenContext extends PredicateContext {
	public _lower!: ValueExpressionContext;
	public _upper!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public BETWEEN(): TerminalNode {
		return this.getToken(SqlBaseParser.BETWEEN, 0);
	}
	public AND(): TerminalNode {
		return this.getToken(SqlBaseParser.AND, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBetween) {
			return visitor.visitBetween(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class QuantifiedComparisonContext extends PredicateContext {
	constructor(parser: SqlBaseParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public comparisonOperator(): ComparisonOperatorContext {
		return this.getTypedRuleContext(ComparisonOperatorContext, 0) as ComparisonOperatorContext;
	}
	public comparisonQuantifier(): ComparisonQuantifierContext {
		return this.getTypedRuleContext(ComparisonQuantifierContext, 0) as ComparisonQuantifierContext;
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQuantifiedComparison) {
			return visitor.visitQuantifiedComparison(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ValueExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_valueExpression;
	}
	public override copyFrom(ctx: ValueExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class ValueExpressionDefaultContext extends ValueExpressionContext {
	constructor(parser: SqlBaseParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitValueExpressionDefault) {
			return visitor.visitValueExpressionDefault(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ConcatenationContext extends ValueExpressionContext {
	public _left!: ValueExpressionContext;
	public _right!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CONCAT(): TerminalNode {
		return this.getToken(SqlBaseParser.CONCAT, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConcatenation) {
			return visitor.visitConcatenation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ArithmeticBinaryContext extends ValueExpressionContext {
	public _left!: ValueExpressionContext;
	public _operator!: Token;
	public _right!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public ASTERISK(): TerminalNode {
		return this.getToken(SqlBaseParser.ASTERISK, 0);
	}
	public SLASH(): TerminalNode {
		return this.getToken(SqlBaseParser.SLASH, 0);
	}
	public PERCENT(): TerminalNode {
		return this.getToken(SqlBaseParser.PERCENT, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlBaseParser.PLUS, 0);
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlBaseParser.MINUS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitArithmeticBinary) {
			return visitor.visitArithmeticBinary(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ArithmeticUnaryContext extends ValueExpressionContext {
	public _operator!: Token;
	constructor(parser: SqlBaseParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlBaseParser.MINUS, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlBaseParser.PLUS, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitArithmeticUnary) {
			return visitor.visitArithmeticUnary(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class AtTimeZoneContext extends ValueExpressionContext {
	constructor(parser: SqlBaseParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public AT(): TerminalNode {
		return this.getToken(SqlBaseParser.AT, 0);
	}
	public timeZoneSpecifier(): TimeZoneSpecifierContext {
		return this.getTypedRuleContext(TimeZoneSpecifierContext, 0) as TimeZoneSpecifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitAtTimeZone) {
			return visitor.visitAtTimeZone(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PrimaryExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_primaryExpression;
	}
	public override copyFrom(ctx: PrimaryExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class DereferenceContext extends PrimaryExpressionContext {
	public _base!: PrimaryExpressionContext;
	public _fieldName!: IdentifierContext;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDereference) {
			return visitor.visitDereference(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TypeConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public DOUBLE_PRECISION(): TerminalNode {
		return this.getToken(SqlBaseParser.DOUBLE_PRECISION, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTypeConstructor) {
			return visitor.visitTypeConstructor(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SpecialDateTimeFunctionContext extends PrimaryExpressionContext {
	public _name!: Token;
	public _precision!: Token;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_DATE(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_DATE, 0);
	}
	public CURRENT_TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_TIME, 0);
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.INTEGER_VALUE, 0);
	}
	public CURRENT_TIMESTAMP(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_TIMESTAMP, 0);
	}
	public LOCALTIME(): TerminalNode {
		return this.getToken(SqlBaseParser.LOCALTIME, 0);
	}
	public LOCALTIMESTAMP(): TerminalNode {
		return this.getToken(SqlBaseParser.LOCALTIMESTAMP, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSpecialDateTimeFunction) {
			return visitor.visitSpecialDateTimeFunction(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SubstringContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SUBSTRING(): TerminalNode {
		return this.getToken(SqlBaseParser.SUBSTRING, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSubstring) {
			return visitor.visitSubstring(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CastContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CAST(): TerminalNode {
		return this.getToken(SqlBaseParser.CAST, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
	public TRY_CAST(): TerminalNode {
		return this.getToken(SqlBaseParser.TRY_CAST, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCast) {
			return visitor.visitCast(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class LambdaContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitLambda) {
			return visitor.visitLambda(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ParenthesizedExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitParenthesizedExpression) {
			return visitor.visitParenthesizedExpression(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ParameterContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitParameter) {
			return visitor.visitParameter(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class NormalizeContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NORMALIZE(): TerminalNode {
		return this.getToken(SqlBaseParser.NORMALIZE, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public normalForm(): NormalFormContext {
		return this.getTypedRuleContext(NormalFormContext, 0) as NormalFormContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNormalize) {
			return visitor.visitNormalize(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class IntervalLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public interval(): IntervalContext {
		return this.getTypedRuleContext(IntervalContext, 0) as IntervalContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitIntervalLiteral) {
			return visitor.visitIntervalLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class NumericLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public number_(): NumberContext {
		return this.getTypedRuleContext(NumberContext, 0) as NumberContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNumericLiteral) {
			return visitor.visitNumericLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class BooleanLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public booleanValue(): BooleanValueContext {
		return this.getTypedRuleContext(BooleanValueContext, 0) as BooleanValueContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBooleanLiteral) {
			return visitor.visitBooleanLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SimpleCaseContext extends PrimaryExpressionContext {
	public _elseExpression!: ExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CASE(): TerminalNode {
		return this.getToken(SqlBaseParser.CASE, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public END(): TerminalNode {
		return this.getToken(SqlBaseParser.END, 0);
	}
	public whenClause_list(): WhenClauseContext[] {
		return this.getTypedRuleContexts(WhenClauseContext) as WhenClauseContext[];
	}
	public whenClause(i: number): WhenClauseContext {
		return this.getTypedRuleContext(WhenClauseContext, i) as WhenClauseContext;
	}
	public ELSE(): TerminalNode {
		return this.getToken(SqlBaseParser.ELSE, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSimpleCase) {
			return visitor.visitSimpleCase(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ColumnReferenceContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitColumnReference) {
			return visitor.visitColumnReference(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class NullLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlBaseParser.NULL, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNullLiteral) {
			return visitor.visitNullLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RowConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlBaseParser.ROW, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRowConstructor) {
			return visitor.visitRowConstructor(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SubscriptContext extends PrimaryExpressionContext {
	public _value!: PrimaryExpressionContext;
	public _index!: ValueExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSubscript) {
			return visitor.visitSubscript(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SubqueryExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSubqueryExpression) {
			return visitor.visitSubqueryExpression(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class BinaryLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BINARY_LITERAL(): TerminalNode {
		return this.getToken(SqlBaseParser.BINARY_LITERAL, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBinaryLiteral) {
			return visitor.visitBinaryLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CurrentUserContext extends PrimaryExpressionContext {
	public _name!: Token;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_USER(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_USER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCurrentUser) {
			return visitor.visitCurrentUser(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ExtractContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXTRACT(): TerminalNode {
		return this.getToken(SqlBaseParser.EXTRACT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExtract) {
			return visitor.visitExtract(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class StringLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitStringLiteral) {
			return visitor.visitStringLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ArrayConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlBaseParser.ARRAY, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitArrayConstructor) {
			return visitor.visitArrayConstructor(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class FunctionCallContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public ASTERISK(): TerminalNode {
		return this.getToken(SqlBaseParser.ASTERISK, 0);
	}
	public filter(): FilterContext {
		return this.getTypedRuleContext(FilterContext, 0) as FilterContext;
	}
	public over(): OverContext {
		return this.getTypedRuleContext(OverContext, 0) as OverContext;
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public ORDER(): TerminalNode {
		return this.getToken(SqlBaseParser.ORDER, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlBaseParser.BY, 0);
	}
	public sortItem_list(): SortItemContext[] {
		return this.getTypedRuleContexts(SortItemContext) as SortItemContext[];
	}
	public sortItem(i: number): SortItemContext {
		return this.getTypedRuleContext(SortItemContext, i) as SortItemContext;
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
	public nullTreatment(): NullTreatmentContext {
		return this.getTypedRuleContext(NullTreatmentContext, 0) as NullTreatmentContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitFunctionCall) {
			return visitor.visitFunctionCall(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ExistsContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlBaseParser.EXISTS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExists) {
			return visitor.visitExists(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class PositionContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public POSITION(): TerminalNode {
		return this.getToken(SqlBaseParser.POSITION, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public IN(): TerminalNode {
		return this.getToken(SqlBaseParser.IN, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitPosition) {
			return visitor.visitPosition(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SearchedCaseContext extends PrimaryExpressionContext {
	public _elseExpression!: ExpressionContext;
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CASE(): TerminalNode {
		return this.getToken(SqlBaseParser.CASE, 0);
	}
	public END(): TerminalNode {
		return this.getToken(SqlBaseParser.END, 0);
	}
	public whenClause_list(): WhenClauseContext[] {
		return this.getTypedRuleContexts(WhenClauseContext) as WhenClauseContext[];
	}
	public whenClause(i: number): WhenClauseContext {
		return this.getTypedRuleContext(WhenClauseContext, i) as WhenClauseContext;
	}
	public ELSE(): TerminalNode {
		return this.getToken(SqlBaseParser.ELSE, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSearchedCase) {
			return visitor.visitSearchedCase(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class GroupingOperationContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GROUPING(): TerminalNode {
		return this.getToken(SqlBaseParser.GROUPING, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitGroupingOperation) {
			return visitor.visitGroupingOperation(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class StringContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_string;
	}
	public override copyFrom(ctx: StringContext): void {
		super.copyFrom(ctx);
	}
}
export class UnicodeStringLiteralContext extends StringContext {
	constructor(parser: SqlBaseParser, ctx: StringContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNICODE_STRING(): TerminalNode {
		return this.getToken(SqlBaseParser.UNICODE_STRING, 0);
	}
	public UESCAPE(): TerminalNode {
		return this.getToken(SqlBaseParser.UESCAPE, 0);
	}
	public STRING(): TerminalNode {
		return this.getToken(SqlBaseParser.STRING, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnicodeStringLiteral) {
			return visitor.visitUnicodeStringLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class BasicStringLiteralContext extends StringContext {
	constructor(parser: SqlBaseParser, ctx: StringContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public STRING(): TerminalNode {
		return this.getToken(SqlBaseParser.STRING, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBasicStringLiteral) {
			return visitor.visitBasicStringLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NullTreatmentContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public IGNORE(): TerminalNode {
		return this.getToken(SqlBaseParser.IGNORE, 0);
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlBaseParser.NULLS, 0);
	}
	public RESPECT(): TerminalNode {
		return this.getToken(SqlBaseParser.RESPECT, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_nullTreatment;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNullTreatment) {
			return visitor.visitNullTreatment(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TimeZoneSpecifierContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_timeZoneSpecifier;
	}
	public override copyFrom(ctx: TimeZoneSpecifierContext): void {
		super.copyFrom(ctx);
	}
}
export class TimeZoneIntervalContext extends TimeZoneSpecifierContext {
	constructor(parser: SqlBaseParser, ctx: TimeZoneSpecifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.TIME, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlBaseParser.ZONE, 0);
	}
	public interval(): IntervalContext {
		return this.getTypedRuleContext(IntervalContext, 0) as IntervalContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTimeZoneInterval) {
			return visitor.visitTimeZoneInterval(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TimeZoneStringContext extends TimeZoneSpecifierContext {
	constructor(parser: SqlBaseParser, ctx: TimeZoneSpecifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.TIME, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlBaseParser.ZONE, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTimeZoneString) {
			return visitor.visitTimeZoneString(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ComparisonOperatorContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlBaseParser.EQ, 0);
	}
	public NEQ(): TerminalNode {
		return this.getToken(SqlBaseParser.NEQ, 0);
	}
	public LT(): TerminalNode {
		return this.getToken(SqlBaseParser.LT, 0);
	}
	public LTE(): TerminalNode {
		return this.getToken(SqlBaseParser.LTE, 0);
	}
	public GT(): TerminalNode {
		return this.getToken(SqlBaseParser.GT, 0);
	}
	public GTE(): TerminalNode {
		return this.getToken(SqlBaseParser.GTE, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_comparisonOperator;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitComparisonOperator) {
			return visitor.visitComparisonOperator(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ComparisonQuantifierContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public SOME(): TerminalNode {
		return this.getToken(SqlBaseParser.SOME, 0);
	}
	public ANY(): TerminalNode {
		return this.getToken(SqlBaseParser.ANY, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_comparisonQuantifier;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitComparisonQuantifier) {
			return visitor.visitComparisonQuantifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class BooleanValueContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public TRUE(): TerminalNode {
		return this.getToken(SqlBaseParser.TRUE, 0);
	}
	public FALSE(): TerminalNode {
		return this.getToken(SqlBaseParser.FALSE, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_booleanValue;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBooleanValue) {
			return visitor.visitBooleanValue(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class IntervalContext extends ParserRuleContext {
	public _sign!: Token;
	public _from_!: IntervalFieldContext;
	public _to!: IntervalFieldContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INTERVAL(): TerminalNode {
		return this.getToken(SqlBaseParser.INTERVAL, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public intervalField_list(): IntervalFieldContext[] {
		return this.getTypedRuleContexts(IntervalFieldContext) as IntervalFieldContext[];
	}
	public intervalField(i: number): IntervalFieldContext {
		return this.getTypedRuleContext(IntervalFieldContext, i) as IntervalFieldContext;
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlBaseParser.PLUS, 0);
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlBaseParser.MINUS, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_interval;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitInterval) {
			return visitor.visitInterval(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class IntervalFieldContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public YEAR(): TerminalNode {
		return this.getToken(SqlBaseParser.YEAR, 0);
	}
	public MONTH(): TerminalNode {
		return this.getToken(SqlBaseParser.MONTH, 0);
	}
	public DAY(): TerminalNode {
		return this.getToken(SqlBaseParser.DAY, 0);
	}
	public HOUR(): TerminalNode {
		return this.getToken(SqlBaseParser.HOUR, 0);
	}
	public MINUTE(): TerminalNode {
		return this.getToken(SqlBaseParser.MINUTE, 0);
	}
	public SECOND(): TerminalNode {
		return this.getToken(SqlBaseParser.SECOND, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_intervalField;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitIntervalField) {
			return visitor.visitIntervalField(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NormalFormContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public NFD(): TerminalNode {
		return this.getToken(SqlBaseParser.NFD, 0);
	}
	public NFC(): TerminalNode {
		return this.getToken(SqlBaseParser.NFC, 0);
	}
	public NFKD(): TerminalNode {
		return this.getToken(SqlBaseParser.NFKD, 0);
	}
	public NFKC(): TerminalNode {
		return this.getToken(SqlBaseParser.NFKC, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_normalForm;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNormalForm) {
			return visitor.visitNormalForm(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TypesContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public type__list(): TypeContext[] {
		return this.getTypedRuleContexts(TypeContext) as TypeContext[];
	}
	public type_(i: number): TypeContext {
		return this.getTypedRuleContext(TypeContext, i) as TypeContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_types;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTypes) {
			return visitor.visitTypes(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TypeContext extends ParserRuleContext {
	public _from_!: IntervalFieldContext;
	public _to!: IntervalFieldContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlBaseParser.ARRAY, 0);
	}
	public LT(): TerminalNode {
		return this.getToken(SqlBaseParser.LT, 0);
	}
	public type__list(): TypeContext[] {
		return this.getTypedRuleContexts(TypeContext) as TypeContext[];
	}
	public type_(i: number): TypeContext {
		return this.getTypedRuleContext(TypeContext, i) as TypeContext;
	}
	public GT(): TerminalNode {
		return this.getToken(SqlBaseParser.GT, 0);
	}
	public MAP(): TerminalNode {
		return this.getToken(SqlBaseParser.MAP, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlBaseParser.ROW, 0);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
	public baseType(): BaseTypeContext {
		return this.getTypedRuleContext(BaseTypeContext, 0) as BaseTypeContext;
	}
	public typeParameter_list(): TypeParameterContext[] {
		return this.getTypedRuleContexts(TypeParameterContext) as TypeParameterContext[];
	}
	public typeParameter(i: number): TypeParameterContext {
		return this.getTypedRuleContext(TypeParameterContext, i) as TypeParameterContext;
	}
	public INTERVAL(): TerminalNode {
		return this.getToken(SqlBaseParser.INTERVAL, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public intervalField_list(): IntervalFieldContext[] {
		return this.getTypedRuleContexts(IntervalFieldContext) as IntervalFieldContext[];
	}
	public intervalField(i: number): IntervalFieldContext {
		return this.getTypedRuleContext(IntervalFieldContext, i) as IntervalFieldContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_type;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitType) {
			return visitor.visitType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TypeParameterContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.INTEGER_VALUE, 0);
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_typeParameter;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTypeParameter) {
			return visitor.visitTypeParameter(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class BaseTypeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public TIME_WITH_TIME_ZONE(): TerminalNode {
		return this.getToken(SqlBaseParser.TIME_WITH_TIME_ZONE, 0);
	}
	public TIMESTAMP_WITH_TIME_ZONE(): TerminalNode {
		return this.getToken(SqlBaseParser.TIMESTAMP_WITH_TIME_ZONE, 0);
	}
	public DOUBLE_PRECISION(): TerminalNode {
		return this.getToken(SqlBaseParser.DOUBLE_PRECISION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_baseType;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBaseType) {
			return visitor.visitBaseType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class WhenClauseContext extends ParserRuleContext {
	public _condition!: ExpressionContext;
	public _result!: ExpressionContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public WHEN(): TerminalNode {
		return this.getToken(SqlBaseParser.WHEN, 0);
	}
	public THEN(): TerminalNode {
		return this.getToken(SqlBaseParser.THEN, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_whenClause;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitWhenClause) {
			return visitor.visitWhenClause(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class FilterContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public FILTER(): TerminalNode {
		return this.getToken(SqlBaseParser.FILTER, 0);
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlBaseParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_filter;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitFilter) {
			return visitor.visitFilter(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class OverContext extends ParserRuleContext {
	public _expression!: ExpressionContext;
	public _partition: ExpressionContext[] = [];
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public OVER(): TerminalNode {
		return this.getToken(SqlBaseParser.OVER, 0);
	}
	public PARTITION(): TerminalNode {
		return this.getToken(SqlBaseParser.PARTITION, 0);
	}
	public BY_list(): TerminalNode[] {
	    	return this.getTokens(SqlBaseParser.BY);
	}
	public BY(i: number): TerminalNode {
		return this.getToken(SqlBaseParser.BY, i);
	}
	public ORDER(): TerminalNode {
		return this.getToken(SqlBaseParser.ORDER, 0);
	}
	public sortItem_list(): SortItemContext[] {
		return this.getTypedRuleContexts(SortItemContext) as SortItemContext[];
	}
	public sortItem(i: number): SortItemContext {
		return this.getTypedRuleContext(SortItemContext, i) as SortItemContext;
	}
	public windowFrame(): WindowFrameContext {
		return this.getTypedRuleContext(WindowFrameContext, 0) as WindowFrameContext;
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_over;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitOver) {
			return visitor.visitOver(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class WindowFrameContext extends ParserRuleContext {
	public _frameType!: Token;
	public _start!: FrameBoundContext;
	public _end!: FrameBoundContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RANGE(): TerminalNode {
		return this.getToken(SqlBaseParser.RANGE, 0);
	}
	public frameBound_list(): FrameBoundContext[] {
		return this.getTypedRuleContexts(FrameBoundContext) as FrameBoundContext[];
	}
	public frameBound(i: number): FrameBoundContext {
		return this.getTypedRuleContext(FrameBoundContext, i) as FrameBoundContext;
	}
	public ROWS(): TerminalNode {
		return this.getToken(SqlBaseParser.ROWS, 0);
	}
	public GROUPS(): TerminalNode {
		return this.getToken(SqlBaseParser.GROUPS, 0);
	}
	public BETWEEN(): TerminalNode {
		return this.getToken(SqlBaseParser.BETWEEN, 0);
	}
	public AND(): TerminalNode {
		return this.getToken(SqlBaseParser.AND, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_windowFrame;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitWindowFrame) {
			return visitor.visitWindowFrame(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class FrameBoundContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_frameBound;
	}
	public override copyFrom(ctx: FrameBoundContext): void {
		super.copyFrom(ctx);
	}
}
export class BoundedFrameContext extends FrameBoundContext {
	public _boundType!: Token;
	constructor(parser: SqlBaseParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlBaseParser.PRECEDING, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlBaseParser.FOLLOWING, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBoundedFrame) {
			return visitor.visitBoundedFrame(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UnboundedFrameContext extends FrameBoundContext {
	public _boundType!: Token;
	constructor(parser: SqlBaseParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNBOUNDED(): TerminalNode {
		return this.getToken(SqlBaseParser.UNBOUNDED, 0);
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlBaseParser.PRECEDING, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlBaseParser.FOLLOWING, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnboundedFrame) {
			return visitor.visitUnboundedFrame(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CurrentRowBoundContext extends FrameBoundContext {
	constructor(parser: SqlBaseParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlBaseParser.ROW, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCurrentRowBound) {
			return visitor.visitCurrentRowBound(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class UpdateAssignmentContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlBaseParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_updateAssignment;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUpdateAssignment) {
			return visitor.visitUpdateAssignment(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ExplainOptionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_explainOption;
	}
	public override copyFrom(ctx: ExplainOptionContext): void {
		super.copyFrom(ctx);
	}
}
export class ExplainFormatContext extends ExplainOptionContext {
	public _value!: Token;
	constructor(parser: SqlBaseParser, ctx: ExplainOptionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public FORMAT(): TerminalNode {
		return this.getToken(SqlBaseParser.FORMAT, 0);
	}
	public TEXT(): TerminalNode {
		return this.getToken(SqlBaseParser.TEXT, 0);
	}
	public GRAPHVIZ(): TerminalNode {
		return this.getToken(SqlBaseParser.GRAPHVIZ, 0);
	}
	public JSON(): TerminalNode {
		return this.getToken(SqlBaseParser.JSON, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExplainFormat) {
			return visitor.visitExplainFormat(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ExplainTypeContext extends ExplainOptionContext {
	public _value!: Token;
	constructor(parser: SqlBaseParser, ctx: ExplainOptionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlBaseParser.TYPE, 0);
	}
	public LOGICAL(): TerminalNode {
		return this.getToken(SqlBaseParser.LOGICAL, 0);
	}
	public DISTRIBUTED(): TerminalNode {
		return this.getToken(SqlBaseParser.DISTRIBUTED, 0);
	}
	public VALIDATE(): TerminalNode {
		return this.getToken(SqlBaseParser.VALIDATE, 0);
	}
	public IO(): TerminalNode {
		return this.getToken(SqlBaseParser.IO, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitExplainType) {
			return visitor.visitExplainType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TransactionModeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_transactionMode;
	}
	public override copyFrom(ctx: TransactionModeContext): void {
		super.copyFrom(ctx);
	}
}
export class TransactionAccessModeContext extends TransactionModeContext {
	public _accessMode!: Token;
	constructor(parser: SqlBaseParser, ctx: TransactionModeContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlBaseParser.READ, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlBaseParser.ONLY, 0);
	}
	public WRITE(): TerminalNode {
		return this.getToken(SqlBaseParser.WRITE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTransactionAccessMode) {
			return visitor.visitTransactionAccessMode(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class IsolationLevelContext extends TransactionModeContext {
	constructor(parser: SqlBaseParser, ctx: TransactionModeContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ISOLATION(): TerminalNode {
		return this.getToken(SqlBaseParser.ISOLATION, 0);
	}
	public LEVEL(): TerminalNode {
		return this.getToken(SqlBaseParser.LEVEL, 0);
	}
	public levelOfIsolation(): LevelOfIsolationContext {
		return this.getTypedRuleContext(LevelOfIsolationContext, 0) as LevelOfIsolationContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitIsolationLevel) {
			return visitor.visitIsolationLevel(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class LevelOfIsolationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_levelOfIsolation;
	}
	public override copyFrom(ctx: LevelOfIsolationContext): void {
		super.copyFrom(ctx);
	}
}
export class ReadUncommittedContext extends LevelOfIsolationContext {
	constructor(parser: SqlBaseParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlBaseParser.READ, 0);
	}
	public UNCOMMITTED(): TerminalNode {
		return this.getToken(SqlBaseParser.UNCOMMITTED, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitReadUncommitted) {
			return visitor.visitReadUncommitted(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SerializableContext extends LevelOfIsolationContext {
	constructor(parser: SqlBaseParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SERIALIZABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.SERIALIZABLE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSerializable) {
			return visitor.visitSerializable(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class ReadCommittedContext extends LevelOfIsolationContext {
	constructor(parser: SqlBaseParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlBaseParser.READ, 0);
	}
	public COMMITTED(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMITTED, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitReadCommitted) {
			return visitor.visitReadCommitted(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RepeatableReadContext extends LevelOfIsolationContext {
	constructor(parser: SqlBaseParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REPEATABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.REPEATABLE, 0);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlBaseParser.READ, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRepeatableRead) {
			return visitor.visitRepeatableRead(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class CallArgumentContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_callArgument;
	}
	public override copyFrom(ctx: CallArgumentContext): void {
		super.copyFrom(ctx);
	}
}
export class PositionalArgumentContext extends CallArgumentContext {
	constructor(parser: SqlBaseParser, ctx: CallArgumentContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitPositionalArgument) {
			return visitor.visitPositionalArgument(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class NamedArgumentContext extends CallArgumentContext {
	constructor(parser: SqlBaseParser, ctx: CallArgumentContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNamedArgument) {
			return visitor.visitNamedArgument(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PrivilegeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SELECT(): TerminalNode {
		return this.getToken(SqlBaseParser.SELECT, 0);
	}
	public DELETE(): TerminalNode {
		return this.getToken(SqlBaseParser.DELETE, 0);
	}
	public INSERT(): TerminalNode {
		return this.getToken(SqlBaseParser.INSERT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_privilege;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitPrivilege) {
			return visitor.visitPrivilege(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class QualifiedNameContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_qualifiedName;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQualifiedName) {
			return visitor.visitQualifiedName(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TableVersionExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_tableVersionExpression;
	}
	public override copyFrom(ctx: TableVersionExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class TableVersionContext extends TableVersionExpressionContext {
	public _tableVersionType!: Token;
	constructor(parser: SqlBaseParser, ctx: TableVersionExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlBaseParser.FOR, 0);
	}
	public tableVersionState(): TableVersionStateContext {
		return this.getTypedRuleContext(TableVersionStateContext, 0) as TableVersionStateContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public SYSTEM_TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM_TIME, 0);
	}
	public SYSTEM_VERSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM_VERSION, 0);
	}
	public TIMESTAMP(): TerminalNode {
		return this.getToken(SqlBaseParser.TIMESTAMP, 0);
	}
	public VERSION(): TerminalNode {
		return this.getToken(SqlBaseParser.VERSION, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTableVersion) {
			return visitor.visitTableVersion(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class TableVersionStateContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_tableVersionState;
	}
	public override copyFrom(ctx: TableVersionStateContext): void {
		super.copyFrom(ctx);
	}
}
export class TableversionbeforeContext extends TableVersionStateContext {
	constructor(parser: SqlBaseParser, ctx: TableVersionStateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BEFORE(): TerminalNode {
		return this.getToken(SqlBaseParser.BEFORE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTableversionbefore) {
			return visitor.visitTableversionbefore(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class TableversionasofContext extends TableVersionStateContext {
	constructor(parser: SqlBaseParser, ctx: TableVersionStateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public AS(): TerminalNode {
		return this.getToken(SqlBaseParser.AS, 0);
	}
	public OF(): TerminalNode {
		return this.getToken(SqlBaseParser.OF, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitTableversionasof) {
			return visitor.visitTableversionasof(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class GrantorContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_grantor;
	}
	public override copyFrom(ctx: GrantorContext): void {
		super.copyFrom(ctx);
	}
}
export class CurrentUserGrantorContext extends GrantorContext {
	constructor(parser: SqlBaseParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_USER(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_USER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCurrentUserGrantor) {
			return visitor.visitCurrentUserGrantor(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class SpecifiedPrincipalContext extends GrantorContext {
	constructor(parser: SqlBaseParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public principal(): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, 0) as PrincipalContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitSpecifiedPrincipal) {
			return visitor.visitSpecifiedPrincipal(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class CurrentRoleGrantorContext extends GrantorContext {
	constructor(parser: SqlBaseParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_ROLE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitCurrentRoleGrantor) {
			return visitor.visitCurrentRoleGrantor(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class PrincipalContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_principal;
	}
	public override copyFrom(ctx: PrincipalContext): void {
		super.copyFrom(ctx);
	}
}
export class UnspecifiedPrincipalContext extends PrincipalContext {
	constructor(parser: SqlBaseParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnspecifiedPrincipal) {
			return visitor.visitUnspecifiedPrincipal(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UserPrincipalContext extends PrincipalContext {
	constructor(parser: SqlBaseParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public USER(): TerminalNode {
		return this.getToken(SqlBaseParser.USER, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUserPrincipal) {
			return visitor.visitUserPrincipal(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class RolePrincipalContext extends PrincipalContext {
	constructor(parser: SqlBaseParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRolePrincipal) {
			return visitor.visitRolePrincipal(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class RolesContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_roles;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitRoles) {
			return visitor.visitRoles(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class IdentifierContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_identifier;
	}
	public override copyFrom(ctx: IdentifierContext): void {
		super.copyFrom(ctx);
	}
}
export class BackQuotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlBaseParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BACKQUOTED_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlBaseParser.BACKQUOTED_IDENTIFIER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitBackQuotedIdentifier) {
			return visitor.visitBackQuotedIdentifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class QuotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlBaseParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public QUOTED_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlBaseParser.QUOTED_IDENTIFIER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitQuotedIdentifier) {
			return visitor.visitQuotedIdentifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DigitIdentifierContext extends IdentifierContext {
	constructor(parser: SqlBaseParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DIGIT_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlBaseParser.DIGIT_IDENTIFIER, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDigitIdentifier) {
			return visitor.visitDigitIdentifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class UnquotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlBaseParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public IDENTIFIER(): TerminalNode {
		return this.getToken(SqlBaseParser.IDENTIFIER, 0);
	}
	public nonReserved(): NonReservedContext {
		return this.getTypedRuleContext(NonReservedContext, 0) as NonReservedContext;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnquotedIdentifier) {
			return visitor.visitUnquotedIdentifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NumberContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_number;
	}
	public override copyFrom(ctx: NumberContext): void {
		super.copyFrom(ctx);
	}
}
export class DecimalLiteralContext extends NumberContext {
	constructor(parser: SqlBaseParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DECIMAL_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.DECIMAL_VALUE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDecimalLiteral) {
			return visitor.visitDecimalLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class DoubleLiteralContext extends NumberContext {
	constructor(parser: SqlBaseParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DOUBLE_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.DOUBLE_VALUE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitDoubleLiteral) {
			return visitor.visitDoubleLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
export class IntegerLiteralContext extends NumberContext {
	constructor(parser: SqlBaseParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.INTEGER_VALUE, 0);
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitIntegerLiteral) {
			return visitor.visitIntegerLiteral(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintSpecificationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public namedConstraintSpecification(): NamedConstraintSpecificationContext {
		return this.getTypedRuleContext(NamedConstraintSpecificationContext, 0) as NamedConstraintSpecificationContext;
	}
	public unnamedConstraintSpecification(): UnnamedConstraintSpecificationContext {
		return this.getTypedRuleContext(UnnamedConstraintSpecificationContext, 0) as UnnamedConstraintSpecificationContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintSpecification;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintSpecification) {
			return visitor.visitConstraintSpecification(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NamedConstraintSpecificationContext extends ParserRuleContext {
	public _name!: IdentifierContext;
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public CONSTRAINT(): TerminalNode {
		return this.getToken(SqlBaseParser.CONSTRAINT, 0);
	}
	public unnamedConstraintSpecification(): UnnamedConstraintSpecificationContext {
		return this.getTypedRuleContext(UnnamedConstraintSpecificationContext, 0) as UnnamedConstraintSpecificationContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_namedConstraintSpecification;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNamedConstraintSpecification) {
			return visitor.visitNamedConstraintSpecification(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class UnnamedConstraintSpecificationContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public constraintType(): ConstraintTypeContext {
		return this.getTypedRuleContext(ConstraintTypeContext, 0) as ConstraintTypeContext;
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
	public constraintQualifiers(): ConstraintQualifiersContext {
		return this.getTypedRuleContext(ConstraintQualifiersContext, 0) as ConstraintQualifiersContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_unnamedConstraintSpecification;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitUnnamedConstraintSpecification) {
			return visitor.visitUnnamedConstraintSpecification(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintTypeContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public UNIQUE(): TerminalNode {
		return this.getToken(SqlBaseParser.UNIQUE, 0);
	}
	public PRIMARY(): TerminalNode {
		return this.getToken(SqlBaseParser.PRIMARY, 0);
	}
	public KEY(): TerminalNode {
		return this.getToken(SqlBaseParser.KEY, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintType;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintType) {
			return visitor.visitConstraintType(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintQualifiersContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public constraintQualifier_list(): ConstraintQualifierContext[] {
		return this.getTypedRuleContexts(ConstraintQualifierContext) as ConstraintQualifierContext[];
	}
	public constraintQualifier(i: number): ConstraintQualifierContext {
		return this.getTypedRuleContext(ConstraintQualifierContext, i) as ConstraintQualifierContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintQualifiers;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintQualifiers) {
			return visitor.visitConstraintQualifiers(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintQualifierContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public constraintEnabled(): ConstraintEnabledContext {
		return this.getTypedRuleContext(ConstraintEnabledContext, 0) as ConstraintEnabledContext;
	}
	public constraintRely(): ConstraintRelyContext {
		return this.getTypedRuleContext(ConstraintRelyContext, 0) as ConstraintRelyContext;
	}
	public constraintEnforced(): ConstraintEnforcedContext {
		return this.getTypedRuleContext(ConstraintEnforcedContext, 0) as ConstraintEnforcedContext;
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintQualifier;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintQualifier) {
			return visitor.visitConstraintQualifier(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintRelyContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RELY(): TerminalNode {
		return this.getToken(SqlBaseParser.RELY, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintRely;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintRely) {
			return visitor.visitConstraintRely(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintEnabledContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ENABLED(): TerminalNode {
		return this.getToken(SqlBaseParser.ENABLED, 0);
	}
	public DISABLED(): TerminalNode {
		return this.getToken(SqlBaseParser.DISABLED, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintEnabled;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintEnabled) {
			return visitor.visitConstraintEnabled(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class ConstraintEnforcedContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ENFORCED(): TerminalNode {
		return this.getToken(SqlBaseParser.ENFORCED, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlBaseParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_constraintEnforced;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitConstraintEnforced) {
			return visitor.visitConstraintEnforced(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}


export class NonReservedContext extends ParserRuleContext {
	constructor(parser?: SqlBaseParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlBaseParser.ADD, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlBaseParser.ADMIN, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlBaseParser.ALL, 0);
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlBaseParser.ANALYZE, 0);
	}
	public ANY(): TerminalNode {
		return this.getToken(SqlBaseParser.ANY, 0);
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlBaseParser.ARRAY, 0);
	}
	public ASC(): TerminalNode {
		return this.getToken(SqlBaseParser.ASC, 0);
	}
	public AT(): TerminalNode {
		return this.getToken(SqlBaseParser.AT, 0);
	}
	public BEFORE(): TerminalNode {
		return this.getToken(SqlBaseParser.BEFORE, 0);
	}
	public BERNOULLI(): TerminalNode {
		return this.getToken(SqlBaseParser.BERNOULLI, 0);
	}
	public CALL(): TerminalNode {
		return this.getToken(SqlBaseParser.CALL, 0);
	}
	public CALLED(): TerminalNode {
		return this.getToken(SqlBaseParser.CALLED, 0);
	}
	public CASCADE(): TerminalNode {
		return this.getToken(SqlBaseParser.CASCADE, 0);
	}
	public CATALOGS(): TerminalNode {
		return this.getToken(SqlBaseParser.CATALOGS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMN, 0);
	}
	public COLUMNS(): TerminalNode {
		return this.getToken(SqlBaseParser.COLUMNS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMENT, 0);
	}
	public COMMIT(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMIT, 0);
	}
	public COMMITTED(): TerminalNode {
		return this.getToken(SqlBaseParser.COMMITTED, 0);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT, 0);
	}
	public CURRENT_ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.CURRENT_ROLE, 0);
	}
	public DATA(): TerminalNode {
		return this.getToken(SqlBaseParser.DATA, 0);
	}
	public DATE(): TerminalNode {
		return this.getToken(SqlBaseParser.DATE, 0);
	}
	public DAY(): TerminalNode {
		return this.getToken(SqlBaseParser.DAY, 0);
	}
	public DEFINER(): TerminalNode {
		return this.getToken(SqlBaseParser.DEFINER, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlBaseParser.DESC, 0);
	}
	public DETERMINISTIC(): TerminalNode {
		return this.getToken(SqlBaseParser.DETERMINISTIC, 0);
	}
	public DISABLED(): TerminalNode {
		return this.getToken(SqlBaseParser.DISABLED, 0);
	}
	public DISTRIBUTED(): TerminalNode {
		return this.getToken(SqlBaseParser.DISTRIBUTED, 0);
	}
	public ENABLED(): TerminalNode {
		return this.getToken(SqlBaseParser.ENABLED, 0);
	}
	public ENFORCED(): TerminalNode {
		return this.getToken(SqlBaseParser.ENFORCED, 0);
	}
	public EXCLUDING(): TerminalNode {
		return this.getToken(SqlBaseParser.EXCLUDING, 0);
	}
	public EXPLAIN(): TerminalNode {
		return this.getToken(SqlBaseParser.EXPLAIN, 0);
	}
	public EXTERNAL(): TerminalNode {
		return this.getToken(SqlBaseParser.EXTERNAL, 0);
	}
	public FETCH(): TerminalNode {
		return this.getToken(SqlBaseParser.FETCH, 0);
	}
	public FILTER(): TerminalNode {
		return this.getToken(SqlBaseParser.FILTER, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlBaseParser.FIRST, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlBaseParser.FOLLOWING, 0);
	}
	public FORMAT(): TerminalNode {
		return this.getToken(SqlBaseParser.FORMAT, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTION, 0);
	}
	public FUNCTIONS(): TerminalNode {
		return this.getToken(SqlBaseParser.FUNCTIONS, 0);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANT, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTED, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlBaseParser.GRANTS, 0);
	}
	public GRAPHVIZ(): TerminalNode {
		return this.getToken(SqlBaseParser.GRAPHVIZ, 0);
	}
	public GROUPS(): TerminalNode {
		return this.getToken(SqlBaseParser.GROUPS, 0);
	}
	public HOUR(): TerminalNode {
		return this.getToken(SqlBaseParser.HOUR, 0);
	}
	public IF(): TerminalNode {
		return this.getToken(SqlBaseParser.IF, 0);
	}
	public IGNORE(): TerminalNode {
		return this.getToken(SqlBaseParser.IGNORE, 0);
	}
	public INCLUDING(): TerminalNode {
		return this.getToken(SqlBaseParser.INCLUDING, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlBaseParser.INPUT, 0);
	}
	public INTERVAL(): TerminalNode {
		return this.getToken(SqlBaseParser.INTERVAL, 0);
	}
	public INVOKER(): TerminalNode {
		return this.getToken(SqlBaseParser.INVOKER, 0);
	}
	public IO(): TerminalNode {
		return this.getToken(SqlBaseParser.IO, 0);
	}
	public ISOLATION(): TerminalNode {
		return this.getToken(SqlBaseParser.ISOLATION, 0);
	}
	public JSON(): TerminalNode {
		return this.getToken(SqlBaseParser.JSON, 0);
	}
	public KEY(): TerminalNode {
		return this.getToken(SqlBaseParser.KEY, 0);
	}
	public LANGUAGE(): TerminalNode {
		return this.getToken(SqlBaseParser.LANGUAGE, 0);
	}
	public LAST(): TerminalNode {
		return this.getToken(SqlBaseParser.LAST, 0);
	}
	public LATERAL(): TerminalNode {
		return this.getToken(SqlBaseParser.LATERAL, 0);
	}
	public LEVEL(): TerminalNode {
		return this.getToken(SqlBaseParser.LEVEL, 0);
	}
	public LIMIT(): TerminalNode {
		return this.getToken(SqlBaseParser.LIMIT, 0);
	}
	public LOGICAL(): TerminalNode {
		return this.getToken(SqlBaseParser.LOGICAL, 0);
	}
	public MAP(): TerminalNode {
		return this.getToken(SqlBaseParser.MAP, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlBaseParser.MATERIALIZED, 0);
	}
	public MINUTE(): TerminalNode {
		return this.getToken(SqlBaseParser.MINUTE, 0);
	}
	public MONTH(): TerminalNode {
		return this.getToken(SqlBaseParser.MONTH, 0);
	}
	public NAME(): TerminalNode {
		return this.getToken(SqlBaseParser.NAME, 0);
	}
	public NFC(): TerminalNode {
		return this.getToken(SqlBaseParser.NFC, 0);
	}
	public NFD(): TerminalNode {
		return this.getToken(SqlBaseParser.NFD, 0);
	}
	public NFKC(): TerminalNode {
		return this.getToken(SqlBaseParser.NFKC, 0);
	}
	public NFKD(): TerminalNode {
		return this.getToken(SqlBaseParser.NFKD, 0);
	}
	public NO(): TerminalNode {
		return this.getToken(SqlBaseParser.NO, 0);
	}
	public NONE(): TerminalNode {
		return this.getToken(SqlBaseParser.NONE, 0);
	}
	public NULLIF(): TerminalNode {
		return this.getToken(SqlBaseParser.NULLIF, 0);
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlBaseParser.NULLS, 0);
	}
	public OF(): TerminalNode {
		return this.getToken(SqlBaseParser.OF, 0);
	}
	public OFFSET(): TerminalNode {
		return this.getToken(SqlBaseParser.OFFSET, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlBaseParser.ONLY, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlBaseParser.OPTION, 0);
	}
	public ORDINALITY(): TerminalNode {
		return this.getToken(SqlBaseParser.ORDINALITY, 0);
	}
	public OUTPUT(): TerminalNode {
		return this.getToken(SqlBaseParser.OUTPUT, 0);
	}
	public OVER(): TerminalNode {
		return this.getToken(SqlBaseParser.OVER, 0);
	}
	public PARTITION(): TerminalNode {
		return this.getToken(SqlBaseParser.PARTITION, 0);
	}
	public PARTITIONS(): TerminalNode {
		return this.getToken(SqlBaseParser.PARTITIONS, 0);
	}
	public POSITION(): TerminalNode {
		return this.getToken(SqlBaseParser.POSITION, 0);
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlBaseParser.PRECEDING, 0);
	}
	public PRIMARY(): TerminalNode {
		return this.getToken(SqlBaseParser.PRIMARY, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlBaseParser.PRIVILEGES, 0);
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlBaseParser.PROPERTIES, 0);
	}
	public RANGE(): TerminalNode {
		return this.getToken(SqlBaseParser.RANGE, 0);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlBaseParser.READ, 0);
	}
	public REFRESH(): TerminalNode {
		return this.getToken(SqlBaseParser.REFRESH, 0);
	}
	public RELY(): TerminalNode {
		return this.getToken(SqlBaseParser.RELY, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlBaseParser.RENAME, 0);
	}
	public REPEATABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.REPEATABLE, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlBaseParser.REPLACE, 0);
	}
	public RESET(): TerminalNode {
		return this.getToken(SqlBaseParser.RESET, 0);
	}
	public RESPECT(): TerminalNode {
		return this.getToken(SqlBaseParser.RESPECT, 0);
	}
	public RESTRICT(): TerminalNode {
		return this.getToken(SqlBaseParser.RESTRICT, 0);
	}
	public RETURN(): TerminalNode {
		return this.getToken(SqlBaseParser.RETURN, 0);
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlBaseParser.RETURNS, 0);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlBaseParser.REVOKE, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLE, 0);
	}
	public ROLES(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLES, 0);
	}
	public ROLLBACK(): TerminalNode {
		return this.getToken(SqlBaseParser.ROLLBACK, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlBaseParser.ROW, 0);
	}
	public ROWS(): TerminalNode {
		return this.getToken(SqlBaseParser.ROWS, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMA, 0);
	}
	public SCHEMAS(): TerminalNode {
		return this.getToken(SqlBaseParser.SCHEMAS, 0);
	}
	public SECOND(): TerminalNode {
		return this.getToken(SqlBaseParser.SECOND, 0);
	}
	public SECURITY(): TerminalNode {
		return this.getToken(SqlBaseParser.SECURITY, 0);
	}
	public SERIALIZABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.SERIALIZABLE, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SESSION, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlBaseParser.SET, 0);
	}
	public SETS(): TerminalNode {
		return this.getToken(SqlBaseParser.SETS, 0);
	}
	public SQL(): TerminalNode {
		return this.getToken(SqlBaseParser.SQL, 0);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlBaseParser.SHOW, 0);
	}
	public SOME(): TerminalNode {
		return this.getToken(SqlBaseParser.SOME, 0);
	}
	public START(): TerminalNode {
		return this.getToken(SqlBaseParser.START, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlBaseParser.STATS, 0);
	}
	public SUBSTRING(): TerminalNode {
		return this.getToken(SqlBaseParser.SUBSTRING, 0);
	}
	public SYSTEM(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM, 0);
	}
	public SYSTEM_TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM_TIME, 0);
	}
	public SYSTEM_VERSION(): TerminalNode {
		return this.getToken(SqlBaseParser.SYSTEM_VERSION, 0);
	}
	public TABLES(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLES, 0);
	}
	public TABLESAMPLE(): TerminalNode {
		return this.getToken(SqlBaseParser.TABLESAMPLE, 0);
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlBaseParser.TEMPORARY, 0);
	}
	public TEXT(): TerminalNode {
		return this.getToken(SqlBaseParser.TEXT, 0);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlBaseParser.TIME, 0);
	}
	public TIMESTAMP(): TerminalNode {
		return this.getToken(SqlBaseParser.TIMESTAMP, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlBaseParser.TO, 0);
	}
	public TRANSACTION(): TerminalNode {
		return this.getToken(SqlBaseParser.TRANSACTION, 0);
	}
	public TRUNCATE(): TerminalNode {
		return this.getToken(SqlBaseParser.TRUNCATE, 0);
	}
	public TRY_CAST(): TerminalNode {
		return this.getToken(SqlBaseParser.TRY_CAST, 0);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlBaseParser.TYPE, 0);
	}
	public UNBOUNDED(): TerminalNode {
		return this.getToken(SqlBaseParser.UNBOUNDED, 0);
	}
	public UNCOMMITTED(): TerminalNode {
		return this.getToken(SqlBaseParser.UNCOMMITTED, 0);
	}
	public UNIQUE(): TerminalNode {
		return this.getToken(SqlBaseParser.UNIQUE, 0);
	}
	public UPDATE(): TerminalNode {
		return this.getToken(SqlBaseParser.UPDATE, 0);
	}
	public USE(): TerminalNode {
		return this.getToken(SqlBaseParser.USE, 0);
	}
	public USER(): TerminalNode {
		return this.getToken(SqlBaseParser.USER, 0);
	}
	public VALIDATE(): TerminalNode {
		return this.getToken(SqlBaseParser.VALIDATE, 0);
	}
	public VERBOSE(): TerminalNode {
		return this.getToken(SqlBaseParser.VERBOSE, 0);
	}
	public VERSION(): TerminalNode {
		return this.getToken(SqlBaseParser.VERSION, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlBaseParser.VIEW, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlBaseParser.WORK, 0);
	}
	public WRITE(): TerminalNode {
		return this.getToken(SqlBaseParser.WRITE, 0);
	}
	public YEAR(): TerminalNode {
		return this.getToken(SqlBaseParser.YEAR, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlBaseParser.ZONE, 0);
	}
    public get ruleIndex(): number {
    	return SqlBaseParser.RULE_nonReserved;
	}
	// @Override
	public accept<Result>(visitor: SqlBaseVisitor<Result>): Result {
		if (visitor.visitNonReserved) {
			return visitor.visitNonReserved(this);
		} else {
			return visitor.visitChildren(this);
		}
	}
}
