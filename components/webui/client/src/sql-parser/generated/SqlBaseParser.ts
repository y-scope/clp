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
	public static readonly RULE_standaloneBooleanExpression = 3;
	public static readonly RULE_statement = 4;
	public static readonly RULE_query = 5;
	public static readonly RULE_with = 6;
	public static readonly RULE_tableElement = 7;
	public static readonly RULE_columnDefinition = 8;
	public static readonly RULE_likeClause = 9;
	public static readonly RULE_properties = 10;
	public static readonly RULE_property = 11;
	public static readonly RULE_sqlParameterDeclaration = 12;
	public static readonly RULE_routineCharacteristics = 13;
	public static readonly RULE_routineCharacteristic = 14;
	public static readonly RULE_alterRoutineCharacteristics = 15;
	public static readonly RULE_alterRoutineCharacteristic = 16;
	public static readonly RULE_routineBody = 17;
	public static readonly RULE_returnStatement = 18;
	public static readonly RULE_externalBodyReference = 19;
	public static readonly RULE_language = 20;
	public static readonly RULE_determinism = 21;
	public static readonly RULE_nullCallClause = 22;
	public static readonly RULE_externalRoutineName = 23;
	public static readonly RULE_queryNoWith = 24;
	public static readonly RULE_queryTerm = 25;
	public static readonly RULE_queryPrimary = 26;
	public static readonly RULE_sortItem = 27;
	public static readonly RULE_querySpecification = 28;
	public static readonly RULE_groupBy = 29;
	public static readonly RULE_groupingElement = 30;
	public static readonly RULE_groupingSet = 31;
	public static readonly RULE_namedQuery = 32;
	public static readonly RULE_setQuantifier = 33;
	public static readonly RULE_selectItem = 34;
	public static readonly RULE_relation = 35;
	public static readonly RULE_joinType = 36;
	public static readonly RULE_joinCriteria = 37;
	public static readonly RULE_sampledRelation = 38;
	public static readonly RULE_sampleType = 39;
	public static readonly RULE_aliasedRelation = 40;
	public static readonly RULE_columnAliases = 41;
	public static readonly RULE_relationPrimary = 42;
	public static readonly RULE_expression = 43;
	public static readonly RULE_booleanExpression = 44;
	public static readonly RULE_predicate = 45;
	public static readonly RULE_valueExpression = 46;
	public static readonly RULE_primaryExpression = 47;
	public static readonly RULE_string = 48;
	public static readonly RULE_nullTreatment = 49;
	public static readonly RULE_timeZoneSpecifier = 50;
	public static readonly RULE_comparisonOperator = 51;
	public static readonly RULE_comparisonQuantifier = 52;
	public static readonly RULE_booleanValue = 53;
	public static readonly RULE_interval = 54;
	public static readonly RULE_intervalField = 55;
	public static readonly RULE_normalForm = 56;
	public static readonly RULE_types = 57;
	public static readonly RULE_type = 58;
	public static readonly RULE_typeParameter = 59;
	public static readonly RULE_baseType = 60;
	public static readonly RULE_whenClause = 61;
	public static readonly RULE_filter = 62;
	public static readonly RULE_over = 63;
	public static readonly RULE_windowFrame = 64;
	public static readonly RULE_frameBound = 65;
	public static readonly RULE_updateAssignment = 66;
	public static readonly RULE_explainOption = 67;
	public static readonly RULE_transactionMode = 68;
	public static readonly RULE_levelOfIsolation = 69;
	public static readonly RULE_callArgument = 70;
	public static readonly RULE_privilege = 71;
	public static readonly RULE_qualifiedName = 72;
	public static readonly RULE_tableVersionExpression = 73;
	public static readonly RULE_tableVersionState = 74;
	public static readonly RULE_grantor = 75;
	public static readonly RULE_principal = 76;
	public static readonly RULE_roles = 77;
	public static readonly RULE_identifier = 78;
	public static readonly RULE_number = 79;
	public static readonly RULE_constraintSpecification = 80;
	public static readonly RULE_namedConstraintSpecification = 81;
	public static readonly RULE_unnamedConstraintSpecification = 82;
	public static readonly RULE_constraintType = 83;
	public static readonly RULE_constraintQualifiers = 84;
	public static readonly RULE_constraintQualifier = 85;
	public static readonly RULE_constraintRely = 86;
	public static readonly RULE_constraintEnabled = 87;
	public static readonly RULE_constraintEnforced = 88;
	public static readonly RULE_nonReserved = 89;
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
		"singleStatement", "standaloneExpression", "standaloneRoutineBody", "standaloneBooleanExpression", 
		"statement", "query", "with", "tableElement", "columnDefinition", "likeClause", 
		"properties", "property", "sqlParameterDeclaration", "routineCharacteristics", 
		"routineCharacteristic", "alterRoutineCharacteristics", "alterRoutineCharacteristic", 
		"routineBody", "returnStatement", "externalBodyReference", "language", 
		"determinism", "nullCallClause", "externalRoutineName", "queryNoWith", 
		"queryTerm", "queryPrimary", "sortItem", "querySpecification", "groupBy", 
		"groupingElement", "groupingSet", "namedQuery", "setQuantifier", "selectItem", 
		"relation", "joinType", "joinCriteria", "sampledRelation", "sampleType", 
		"aliasedRelation", "columnAliases", "relationPrimary", "expression", "booleanExpression", 
		"predicate", "valueExpression", "primaryExpression", "string", "nullTreatment", 
		"timeZoneSpecifier", "comparisonOperator", "comparisonQuantifier", "booleanValue", 
		"interval", "intervalField", "normalForm", "types", "type", "typeParameter", 
		"baseType", "whenClause", "filter", "over", "windowFrame", "frameBound", 
		"updateAssignment", "explainOption", "transactionMode", "levelOfIsolation", 
		"callArgument", "privilege", "qualifiedName", "tableVersionExpression", 
		"tableVersionState", "grantor", "principal", "roles", "identifier", "number", 
		"constraintSpecification", "namedConstraintSpecification", "unnamedConstraintSpecification", 
		"constraintType", "constraintQualifiers", "constraintQualifier", "constraintRely", 
		"constraintEnabled", "constraintEnforced", "nonReserved",
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
			this.state = 180;
			this.statement();
			this.state = 181;
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
			this.state = 183;
			this.expression();
			this.state = 184;
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
			this.state = 186;
			this.routineBody();
			this.state = 187;
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
		this.enterRule(localctx, 6, SqlBaseParser.RULE_standaloneBooleanExpression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 189;
			this.booleanExpression(0);
			this.state = 190;
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
		this.enterRule(localctx, 8, SqlBaseParser.RULE_statement);
		let _la: number;
		try {
			this.state = 933;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 102, this._ctx) ) {
			case 1:
				localctx = new StatementDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 192;
				this.query();
				}
				break;
			case 2:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 193;
				this.match(SqlBaseParser.USE);
				this.state = 194;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 3:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 195;
				this.match(SqlBaseParser.USE);
				this.state = 196;
				(localctx as UseContext)._catalog = this.identifier();
				this.state = 197;
				this.match(SqlBaseParser.T__0);
				this.state = 198;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 4:
				localctx = new CreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 200;
				this.match(SqlBaseParser.CREATE);
				this.state = 201;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 205;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 0, this._ctx) ) {
				case 1:
					{
					this.state = 202;
					this.match(SqlBaseParser.IF);
					this.state = 203;
					this.match(SqlBaseParser.NOT);
					this.state = 204;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 207;
				this.qualifiedName();
				this.state = 210;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 208;
					this.match(SqlBaseParser.WITH);
					this.state = 209;
					this.properties();
					}
				}

				}
				break;
			case 5:
				localctx = new DropSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 212;
				this.match(SqlBaseParser.DROP);
				this.state = 213;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 216;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 2, this._ctx) ) {
				case 1:
					{
					this.state = 214;
					this.match(SqlBaseParser.IF);
					this.state = 215;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 218;
				this.qualifiedName();
				this.state = 220;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===27 || _la===164) {
					{
					this.state = 219;
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
				this.state = 222;
				this.match(SqlBaseParser.ALTER);
				this.state = 223;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 224;
				this.qualifiedName();
				this.state = 225;
				this.match(SqlBaseParser.RENAME);
				this.state = 226;
				this.match(SqlBaseParser.TO);
				this.state = 227;
				this.identifier();
				}
				break;
			case 7:
				localctx = new CreateTableAsSelectContext(this, localctx);
				this.enterOuterAlt(localctx, 7);
				{
				this.state = 229;
				this.match(SqlBaseParser.CREATE);
				this.state = 230;
				this.match(SqlBaseParser.TABLE);
				this.state = 234;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 4, this._ctx) ) {
				case 1:
					{
					this.state = 231;
					this.match(SqlBaseParser.IF);
					this.state = 232;
					this.match(SqlBaseParser.NOT);
					this.state = 233;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 236;
				this.qualifiedName();
				this.state = 238;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 237;
					this.columnAliases();
					}
				}

				this.state = 242;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 240;
					this.match(SqlBaseParser.COMMENT);
					this.state = 241;
					this.string_();
					}
				}

				this.state = 246;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 244;
					this.match(SqlBaseParser.WITH);
					this.state = 245;
					this.properties();
					}
				}

				this.state = 248;
				this.match(SqlBaseParser.AS);
				this.state = 254;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 8, this._ctx) ) {
				case 1:
					{
					this.state = 249;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 250;
					this.match(SqlBaseParser.T__1);
					this.state = 251;
					this.query();
					this.state = 252;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 261;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 256;
					this.match(SqlBaseParser.WITH);
					this.state = 258;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===128) {
						{
						this.state = 257;
						this.match(SqlBaseParser.NO);
						}
					}

					this.state = 260;
					this.match(SqlBaseParser.DATA);
					}
				}

				}
				break;
			case 8:
				localctx = new CreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 263;
				this.match(SqlBaseParser.CREATE);
				this.state = 264;
				this.match(SqlBaseParser.TABLE);
				this.state = 268;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 11, this._ctx) ) {
				case 1:
					{
					this.state = 265;
					this.match(SqlBaseParser.IF);
					this.state = 266;
					this.match(SqlBaseParser.NOT);
					this.state = 267;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 270;
				this.qualifiedName();
				this.state = 271;
				this.match(SqlBaseParser.T__1);
				this.state = 272;
				this.tableElement();
				this.state = 277;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 273;
					this.match(SqlBaseParser.T__3);
					this.state = 274;
					this.tableElement();
					}
					}
					this.state = 279;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 280;
				this.match(SqlBaseParser.T__2);
				this.state = 283;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 281;
					this.match(SqlBaseParser.COMMENT);
					this.state = 282;
					this.string_();
					}
				}

				this.state = 287;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 285;
					this.match(SqlBaseParser.WITH);
					this.state = 286;
					this.properties();
					}
				}

				}
				break;
			case 9:
				localctx = new DropTableContext(this, localctx);
				this.enterOuterAlt(localctx, 9);
				{
				this.state = 289;
				this.match(SqlBaseParser.DROP);
				this.state = 290;
				this.match(SqlBaseParser.TABLE);
				this.state = 293;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 15, this._ctx) ) {
				case 1:
					{
					this.state = 291;
					this.match(SqlBaseParser.IF);
					this.state = 292;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 295;
				this.qualifiedName();
				}
				break;
			case 10:
				localctx = new InsertIntoContext(this, localctx);
				this.enterOuterAlt(localctx, 10);
				{
				this.state = 296;
				this.match(SqlBaseParser.INSERT);
				this.state = 297;
				this.match(SqlBaseParser.INTO);
				this.state = 298;
				this.qualifiedName();
				this.state = 300;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 16, this._ctx) ) {
				case 1:
					{
					this.state = 299;
					this.columnAliases();
					}
					break;
				}
				this.state = 302;
				this.query();
				}
				break;
			case 11:
				localctx = new DeleteContext(this, localctx);
				this.enterOuterAlt(localctx, 11);
				{
				this.state = 304;
				this.match(SqlBaseParser.DELETE);
				this.state = 305;
				this.match(SqlBaseParser.FROM);
				this.state = 306;
				this.qualifiedName();
				this.state = 309;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 307;
					this.match(SqlBaseParser.WHERE);
					this.state = 308;
					this.booleanExpression(0);
					}
				}

				}
				break;
			case 12:
				localctx = new TruncateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 12);
				{
				this.state = 311;
				this.match(SqlBaseParser.TRUNCATE);
				this.state = 312;
				this.match(SqlBaseParser.TABLE);
				this.state = 313;
				this.qualifiedName();
				}
				break;
			case 13:
				localctx = new RenameTableContext(this, localctx);
				this.enterOuterAlt(localctx, 13);
				{
				this.state = 314;
				this.match(SqlBaseParser.ALTER);
				this.state = 315;
				this.match(SqlBaseParser.TABLE);
				this.state = 318;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 18, this._ctx) ) {
				case 1:
					{
					this.state = 316;
					this.match(SqlBaseParser.IF);
					this.state = 317;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 320;
				(localctx as RenameTableContext)._from_ = this.qualifiedName();
				this.state = 321;
				this.match(SqlBaseParser.RENAME);
				this.state = 322;
				this.match(SqlBaseParser.TO);
				this.state = 323;
				(localctx as RenameTableContext)._to = this.qualifiedName();
				}
				break;
			case 14:
				localctx = new RenameColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 14);
				{
				this.state = 325;
				this.match(SqlBaseParser.ALTER);
				this.state = 326;
				this.match(SqlBaseParser.TABLE);
				this.state = 329;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 19, this._ctx) ) {
				case 1:
					{
					this.state = 327;
					this.match(SqlBaseParser.IF);
					this.state = 328;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 331;
				(localctx as RenameColumnContext)._tableName = this.qualifiedName();
				this.state = 332;
				this.match(SqlBaseParser.RENAME);
				this.state = 333;
				this.match(SqlBaseParser.COLUMN);
				this.state = 336;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 20, this._ctx) ) {
				case 1:
					{
					this.state = 334;
					this.match(SqlBaseParser.IF);
					this.state = 335;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 338;
				(localctx as RenameColumnContext)._from_ = this.identifier();
				this.state = 339;
				this.match(SqlBaseParser.TO);
				this.state = 340;
				(localctx as RenameColumnContext)._to = this.identifier();
				}
				break;
			case 15:
				localctx = new DropColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 15);
				{
				this.state = 342;
				this.match(SqlBaseParser.ALTER);
				this.state = 343;
				this.match(SqlBaseParser.TABLE);
				this.state = 346;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 21, this._ctx) ) {
				case 1:
					{
					this.state = 344;
					this.match(SqlBaseParser.IF);
					this.state = 345;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 348;
				(localctx as DropColumnContext)._tableName = this.qualifiedName();
				this.state = 349;
				this.match(SqlBaseParser.DROP);
				this.state = 350;
				this.match(SqlBaseParser.COLUMN);
				this.state = 353;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 22, this._ctx) ) {
				case 1:
					{
					this.state = 351;
					this.match(SqlBaseParser.IF);
					this.state = 352;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 355;
				(localctx as DropColumnContext)._column = this.qualifiedName();
				}
				break;
			case 16:
				localctx = new AddColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 16);
				{
				this.state = 357;
				this.match(SqlBaseParser.ALTER);
				this.state = 358;
				this.match(SqlBaseParser.TABLE);
				this.state = 361;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 23, this._ctx) ) {
				case 1:
					{
					this.state = 359;
					this.match(SqlBaseParser.IF);
					this.state = 360;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 363;
				(localctx as AddColumnContext)._tableName = this.qualifiedName();
				this.state = 364;
				this.match(SqlBaseParser.ADD);
				this.state = 365;
				this.match(SqlBaseParser.COLUMN);
				this.state = 369;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 24, this._ctx) ) {
				case 1:
					{
					this.state = 366;
					this.match(SqlBaseParser.IF);
					this.state = 367;
					this.match(SqlBaseParser.NOT);
					this.state = 368;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 371;
				(localctx as AddColumnContext)._column = this.columnDefinition();
				}
				break;
			case 17:
				localctx = new AddConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 17);
				{
				this.state = 373;
				this.match(SqlBaseParser.ALTER);
				this.state = 374;
				this.match(SqlBaseParser.TABLE);
				this.state = 377;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 25, this._ctx) ) {
				case 1:
					{
					this.state = 375;
					this.match(SqlBaseParser.IF);
					this.state = 376;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 379;
				(localctx as AddConstraintContext)._tableName = this.qualifiedName();
				this.state = 380;
				this.match(SqlBaseParser.ADD);
				this.state = 381;
				this.constraintSpecification();
				}
				break;
			case 18:
				localctx = new DropConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 18);
				{
				this.state = 383;
				this.match(SqlBaseParser.ALTER);
				this.state = 384;
				this.match(SqlBaseParser.TABLE);
				this.state = 387;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 26, this._ctx) ) {
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
				(localctx as DropConstraintContext)._tableName = this.qualifiedName();
				this.state = 390;
				this.match(SqlBaseParser.DROP);
				this.state = 391;
				this.match(SqlBaseParser.CONSTRAINT);
				this.state = 394;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 27, this._ctx) ) {
				case 1:
					{
					this.state = 392;
					this.match(SqlBaseParser.IF);
					this.state = 393;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 396;
				(localctx as DropConstraintContext)._name = this.identifier();
				}
				break;
			case 19:
				localctx = new AlterColumnSetNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 19);
				{
				this.state = 398;
				this.match(SqlBaseParser.ALTER);
				this.state = 399;
				this.match(SqlBaseParser.TABLE);
				this.state = 402;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 28, this._ctx) ) {
				case 1:
					{
					this.state = 400;
					this.match(SqlBaseParser.IF);
					this.state = 401;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 404;
				(localctx as AlterColumnSetNotNullContext)._tableName = this.qualifiedName();
				this.state = 405;
				this.match(SqlBaseParser.ALTER);
				this.state = 407;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 29, this._ctx) ) {
				case 1:
					{
					this.state = 406;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 409;
				(localctx as AlterColumnSetNotNullContext)._column = this.identifier();
				this.state = 410;
				this.match(SqlBaseParser.SET);
				this.state = 411;
				this.match(SqlBaseParser.NOT);
				this.state = 412;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 20:
				localctx = new AlterColumnDropNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 20);
				{
				this.state = 414;
				this.match(SqlBaseParser.ALTER);
				this.state = 415;
				this.match(SqlBaseParser.TABLE);
				this.state = 418;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 30, this._ctx) ) {
				case 1:
					{
					this.state = 416;
					this.match(SqlBaseParser.IF);
					this.state = 417;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 420;
				(localctx as AlterColumnDropNotNullContext)._tableName = this.qualifiedName();
				this.state = 421;
				this.match(SqlBaseParser.ALTER);
				this.state = 423;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 31, this._ctx) ) {
				case 1:
					{
					this.state = 422;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 425;
				(localctx as AlterColumnDropNotNullContext)._column = this.identifier();
				this.state = 426;
				this.match(SqlBaseParser.DROP);
				this.state = 427;
				this.match(SqlBaseParser.NOT);
				this.state = 428;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 21:
				localctx = new SetTablePropertiesContext(this, localctx);
				this.enterOuterAlt(localctx, 21);
				{
				this.state = 430;
				this.match(SqlBaseParser.ALTER);
				this.state = 431;
				this.match(SqlBaseParser.TABLE);
				this.state = 434;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 32, this._ctx) ) {
				case 1:
					{
					this.state = 432;
					this.match(SqlBaseParser.IF);
					this.state = 433;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 436;
				(localctx as SetTablePropertiesContext)._tableName = this.qualifiedName();
				this.state = 437;
				this.match(SqlBaseParser.SET);
				this.state = 438;
				this.match(SqlBaseParser.PROPERTIES);
				this.state = 439;
				this.properties();
				}
				break;
			case 22:
				localctx = new AnalyzeContext(this, localctx);
				this.enterOuterAlt(localctx, 22);
				{
				this.state = 441;
				this.match(SqlBaseParser.ANALYZE);
				this.state = 442;
				this.qualifiedName();
				this.state = 445;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 443;
					this.match(SqlBaseParser.WITH);
					this.state = 444;
					this.properties();
					}
				}

				}
				break;
			case 23:
				localctx = new CreateTypeContext(this, localctx);
				this.enterOuterAlt(localctx, 23);
				{
				this.state = 447;
				this.match(SqlBaseParser.CREATE);
				this.state = 448;
				this.match(SqlBaseParser.TYPE);
				this.state = 449;
				this.qualifiedName();
				this.state = 450;
				this.match(SqlBaseParser.AS);
				this.state = 463;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 2:
					{
					this.state = 451;
					this.match(SqlBaseParser.T__1);
					this.state = 452;
					this.sqlParameterDeclaration();
					this.state = 457;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 453;
						this.match(SqlBaseParser.T__3);
						this.state = 454;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 459;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 460;
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
					this.state = 462;
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
				this.state = 465;
				this.match(SqlBaseParser.CREATE);
				this.state = 468;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 466;
					this.match(SqlBaseParser.OR);
					this.state = 467;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 470;
				this.match(SqlBaseParser.VIEW);
				this.state = 471;
				this.qualifiedName();
				this.state = 474;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===178) {
					{
					this.state = 472;
					this.match(SqlBaseParser.SECURITY);
					this.state = 473;
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

				this.state = 476;
				this.match(SqlBaseParser.AS);
				this.state = 477;
				this.query();
				}
				break;
			case 25:
				localctx = new RenameViewContext(this, localctx);
				this.enterOuterAlt(localctx, 25);
				{
				this.state = 479;
				this.match(SqlBaseParser.ALTER);
				this.state = 480;
				this.match(SqlBaseParser.VIEW);
				this.state = 483;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 38, this._ctx) ) {
				case 1:
					{
					this.state = 481;
					this.match(SqlBaseParser.IF);
					this.state = 482;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 485;
				(localctx as RenameViewContext)._from_ = this.qualifiedName();
				this.state = 486;
				this.match(SqlBaseParser.RENAME);
				this.state = 487;
				this.match(SqlBaseParser.TO);
				this.state = 488;
				(localctx as RenameViewContext)._to = this.qualifiedName();
				}
				break;
			case 26:
				localctx = new DropViewContext(this, localctx);
				this.enterOuterAlt(localctx, 26);
				{
				this.state = 490;
				this.match(SqlBaseParser.DROP);
				this.state = 491;
				this.match(SqlBaseParser.VIEW);
				this.state = 494;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 39, this._ctx) ) {
				case 1:
					{
					this.state = 492;
					this.match(SqlBaseParser.IF);
					this.state = 493;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 496;
				this.qualifiedName();
				}
				break;
			case 27:
				localctx = new CreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 27);
				{
				this.state = 497;
				this.match(SqlBaseParser.CREATE);
				this.state = 498;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 499;
				this.match(SqlBaseParser.VIEW);
				this.state = 503;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 40, this._ctx) ) {
				case 1:
					{
					this.state = 500;
					this.match(SqlBaseParser.IF);
					this.state = 501;
					this.match(SqlBaseParser.NOT);
					this.state = 502;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 505;
				this.qualifiedName();
				this.state = 508;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 506;
					this.match(SqlBaseParser.COMMENT);
					this.state = 507;
					this.string_();
					}
				}

				this.state = 512;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 510;
					this.match(SqlBaseParser.WITH);
					this.state = 511;
					this.properties();
					}
				}

				this.state = 514;
				this.match(SqlBaseParser.AS);
				this.state = 520;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 43, this._ctx) ) {
				case 1:
					{
					this.state = 515;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 516;
					this.match(SqlBaseParser.T__1);
					this.state = 517;
					this.query();
					this.state = 518;
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
				this.state = 522;
				this.match(SqlBaseParser.DROP);
				this.state = 523;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 524;
				this.match(SqlBaseParser.VIEW);
				this.state = 527;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 44, this._ctx) ) {
				case 1:
					{
					this.state = 525;
					this.match(SqlBaseParser.IF);
					this.state = 526;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 529;
				this.qualifiedName();
				}
				break;
			case 29:
				localctx = new RefreshMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 29);
				{
				this.state = 530;
				this.match(SqlBaseParser.REFRESH);
				this.state = 531;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 532;
				this.match(SqlBaseParser.VIEW);
				this.state = 533;
				this.qualifiedName();
				this.state = 534;
				this.match(SqlBaseParser.WHERE);
				this.state = 535;
				this.booleanExpression(0);
				}
				break;
			case 30:
				localctx = new CreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 30);
				{
				this.state = 537;
				this.match(SqlBaseParser.CREATE);
				this.state = 540;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 538;
					this.match(SqlBaseParser.OR);
					this.state = 539;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 543;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 542;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 545;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 546;
				(localctx as CreateFunctionContext)._functionName = this.qualifiedName();
				this.state = 547;
				this.match(SqlBaseParser.T__1);
				this.state = 556;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 548;
					this.sqlParameterDeclaration();
					this.state = 553;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 549;
						this.match(SqlBaseParser.T__3);
						this.state = 550;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 555;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 558;
				this.match(SqlBaseParser.T__2);
				this.state = 559;
				this.match(SqlBaseParser.RETURNS);
				this.state = 560;
				(localctx as CreateFunctionContext)._returnType = this.type_(0);
				this.state = 563;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 561;
					this.match(SqlBaseParser.COMMENT);
					this.state = 562;
					this.string_();
					}
				}

				this.state = 565;
				this.routineCharacteristics();
				this.state = 566;
				this.routineBody();
				}
				break;
			case 31:
				localctx = new AlterFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 31);
				{
				this.state = 568;
				this.match(SqlBaseParser.ALTER);
				this.state = 569;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 570;
				this.qualifiedName();
				this.state = 572;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 571;
					this.types();
					}
				}

				this.state = 574;
				this.alterRoutineCharacteristics();
				}
				break;
			case 32:
				localctx = new DropFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 32);
				{
				this.state = 576;
				this.match(SqlBaseParser.DROP);
				this.state = 578;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 577;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 580;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 583;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 52, this._ctx) ) {
				case 1:
					{
					this.state = 581;
					this.match(SqlBaseParser.IF);
					this.state = 582;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 585;
				this.qualifiedName();
				this.state = 587;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 586;
					this.types();
					}
				}

				}
				break;
			case 33:
				localctx = new CallContext(this, localctx);
				this.enterOuterAlt(localctx, 33);
				{
				this.state = 589;
				this.match(SqlBaseParser.CALL);
				this.state = 590;
				this.qualifiedName();
				this.state = 591;
				this.match(SqlBaseParser.T__1);
				this.state = 600;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 592;
					this.callArgument();
					this.state = 597;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 593;
						this.match(SqlBaseParser.T__3);
						this.state = 594;
						this.callArgument();
						}
						}
						this.state = 599;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 602;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				localctx = new CreateRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 34);
				{
				this.state = 604;
				this.match(SqlBaseParser.CREATE);
				this.state = 605;
				this.match(SqlBaseParser.ROLE);
				this.state = 606;
				(localctx as CreateRoleContext)._name = this.identifier();
				this.state = 610;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 607;
					this.match(SqlBaseParser.WITH);
					this.state = 608;
					this.match(SqlBaseParser.ADMIN);
					this.state = 609;
					this.grantor();
					}
				}

				}
				break;
			case 35:
				localctx = new DropRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 35);
				{
				this.state = 612;
				this.match(SqlBaseParser.DROP);
				this.state = 613;
				this.match(SqlBaseParser.ROLE);
				this.state = 614;
				(localctx as DropRoleContext)._name = this.identifier();
				}
				break;
			case 36:
				localctx = new GrantRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 36);
				{
				this.state = 615;
				this.match(SqlBaseParser.GRANT);
				this.state = 616;
				this.roles();
				this.state = 617;
				this.match(SqlBaseParser.TO);
				this.state = 618;
				this.principal();
				this.state = 623;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 619;
					this.match(SqlBaseParser.T__3);
					this.state = 620;
					this.principal();
					}
					}
					this.state = 625;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 629;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 626;
					this.match(SqlBaseParser.WITH);
					this.state = 627;
					this.match(SqlBaseParser.ADMIN);
					this.state = 628;
					this.match(SqlBaseParser.OPTION);
					}
				}

				this.state = 634;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 631;
					this.match(SqlBaseParser.GRANTED);
					this.state = 632;
					this.match(SqlBaseParser.BY);
					this.state = 633;
					this.grantor();
					}
				}

				}
				break;
			case 37:
				localctx = new RevokeRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 37);
				{
				this.state = 636;
				this.match(SqlBaseParser.REVOKE);
				this.state = 640;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 60, this._ctx) ) {
				case 1:
					{
					this.state = 637;
					this.match(SqlBaseParser.ADMIN);
					this.state = 638;
					this.match(SqlBaseParser.OPTION);
					this.state = 639;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 642;
				this.roles();
				this.state = 643;
				this.match(SqlBaseParser.FROM);
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
				if (_la===83) {
					{
					this.state = 652;
					this.match(SqlBaseParser.GRANTED);
					this.state = 653;
					this.match(SqlBaseParser.BY);
					this.state = 654;
					this.grantor();
					}
				}

				}
				break;
			case 38:
				localctx = new SetRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 38);
				{
				this.state = 657;
				this.match(SqlBaseParser.SET);
				this.state = 658;
				this.match(SqlBaseParser.ROLE);
				this.state = 662;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 63, this._ctx) ) {
				case 1:
					{
					this.state = 659;
					this.match(SqlBaseParser.ALL);
					}
					break;
				case 2:
					{
					this.state = 660;
					this.match(SqlBaseParser.NONE);
					}
					break;
				case 3:
					{
					this.state = 661;
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
				this.state = 664;
				this.match(SqlBaseParser.GRANT);
				this.state = 675;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 65, this._ctx) ) {
				case 1:
					{
					this.state = 665;
					this.privilege();
					this.state = 670;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 666;
						this.match(SqlBaseParser.T__3);
						this.state = 667;
						this.privilege();
						}
						}
						this.state = 672;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 673;
					this.match(SqlBaseParser.ALL);
					this.state = 674;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 677;
				this.match(SqlBaseParser.ON);
				this.state = 679;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 678;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 681;
				this.qualifiedName();
				this.state = 682;
				this.match(SqlBaseParser.TO);
				this.state = 683;
				(localctx as GrantContext)._grantee = this.principal();
				this.state = 687;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 684;
					this.match(SqlBaseParser.WITH);
					this.state = 685;
					this.match(SqlBaseParser.GRANT);
					this.state = 686;
					this.match(SqlBaseParser.OPTION);
					}
				}

				}
				break;
			case 40:
				localctx = new RevokeContext(this, localctx);
				this.enterOuterAlt(localctx, 40);
				{
				this.state = 689;
				this.match(SqlBaseParser.REVOKE);
				this.state = 693;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 68, this._ctx) ) {
				case 1:
					{
					this.state = 690;
					this.match(SqlBaseParser.GRANT);
					this.state = 691;
					this.match(SqlBaseParser.OPTION);
					this.state = 692;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 705;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 70, this._ctx) ) {
				case 1:
					{
					this.state = 695;
					this.privilege();
					this.state = 700;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 696;
						this.match(SqlBaseParser.T__3);
						this.state = 697;
						this.privilege();
						}
						}
						this.state = 702;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 703;
					this.match(SqlBaseParser.ALL);
					this.state = 704;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 707;
				this.match(SqlBaseParser.ON);
				this.state = 709;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 708;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 711;
				this.qualifiedName();
				this.state = 712;
				this.match(SqlBaseParser.FROM);
				this.state = 713;
				(localctx as RevokeContext)._grantee = this.principal();
				}
				break;
			case 41:
				localctx = new ShowGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 41);
				{
				this.state = 715;
				this.match(SqlBaseParser.SHOW);
				this.state = 716;
				this.match(SqlBaseParser.GRANTS);
				this.state = 722;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===137) {
					{
					this.state = 717;
					this.match(SqlBaseParser.ON);
					this.state = 719;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===193) {
						{
						this.state = 718;
						this.match(SqlBaseParser.TABLE);
						}
					}

					this.state = 721;
					this.qualifiedName();
					}
				}

				}
				break;
			case 42:
				localctx = new ExplainContext(this, localctx);
				this.enterOuterAlt(localctx, 42);
				{
				this.state = 724;
				this.match(SqlBaseParser.EXPLAIN);
				this.state = 726;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 74, this._ctx) ) {
				case 1:
					{
					this.state = 725;
					this.match(SqlBaseParser.ANALYZE);
					}
					break;
				}
				this.state = 729;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===219) {
					{
					this.state = 728;
					this.match(SqlBaseParser.VERBOSE);
					}
				}

				this.state = 742;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 77, this._ctx) ) {
				case 1:
					{
					this.state = 731;
					this.match(SqlBaseParser.T__1);
					this.state = 732;
					this.explainOption();
					this.state = 737;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 733;
						this.match(SqlBaseParser.T__3);
						this.state = 734;
						this.explainOption();
						}
						}
						this.state = 739;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 740;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 744;
				this.statement();
				}
				break;
			case 43:
				localctx = new ShowCreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 43);
				{
				this.state = 745;
				this.match(SqlBaseParser.SHOW);
				this.state = 746;
				this.match(SqlBaseParser.CREATE);
				this.state = 747;
				this.match(SqlBaseParser.TABLE);
				this.state = 748;
				this.qualifiedName();
				}
				break;
			case 44:
				localctx = new ShowCreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 44);
				{
				this.state = 749;
				this.match(SqlBaseParser.SHOW);
				this.state = 750;
				this.match(SqlBaseParser.CREATE);
				this.state = 751;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 752;
				this.qualifiedName();
				}
				break;
			case 45:
				localctx = new ShowCreateViewContext(this, localctx);
				this.enterOuterAlt(localctx, 45);
				{
				this.state = 753;
				this.match(SqlBaseParser.SHOW);
				this.state = 754;
				this.match(SqlBaseParser.CREATE);
				this.state = 755;
				this.match(SqlBaseParser.VIEW);
				this.state = 756;
				this.qualifiedName();
				}
				break;
			case 46:
				localctx = new ShowCreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 46);
				{
				this.state = 757;
				this.match(SqlBaseParser.SHOW);
				this.state = 758;
				this.match(SqlBaseParser.CREATE);
				this.state = 759;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 760;
				this.match(SqlBaseParser.VIEW);
				this.state = 761;
				this.qualifiedName();
				}
				break;
			case 47:
				localctx = new ShowCreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 47);
				{
				this.state = 762;
				this.match(SqlBaseParser.SHOW);
				this.state = 763;
				this.match(SqlBaseParser.CREATE);
				this.state = 764;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 765;
				this.qualifiedName();
				this.state = 767;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 766;
					this.types();
					}
				}

				}
				break;
			case 48:
				localctx = new ShowTablesContext(this, localctx);
				this.enterOuterAlt(localctx, 48);
				{
				this.state = 769;
				this.match(SqlBaseParser.SHOW);
				this.state = 770;
				this.match(SqlBaseParser.TABLES);
				this.state = 773;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 771;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 772;
					this.qualifiedName();
					}
				}

				this.state = 781;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 775;
					this.match(SqlBaseParser.LIKE);
					this.state = 776;
					(localctx as ShowTablesContext)._pattern = this.string_();
					this.state = 779;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 777;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 778;
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
				this.state = 783;
				this.match(SqlBaseParser.SHOW);
				this.state = 784;
				this.match(SqlBaseParser.SCHEMAS);
				this.state = 787;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 785;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 786;
					this.identifier();
					}
				}

				this.state = 795;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 789;
					this.match(SqlBaseParser.LIKE);
					this.state = 790;
					(localctx as ShowSchemasContext)._pattern = this.string_();
					this.state = 793;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 791;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 792;
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
				this.state = 797;
				this.match(SqlBaseParser.SHOW);
				this.state = 798;
				this.match(SqlBaseParser.CATALOGS);
				this.state = 805;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 799;
					this.match(SqlBaseParser.LIKE);
					this.state = 800;
					(localctx as ShowCatalogsContext)._pattern = this.string_();
					this.state = 803;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 801;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 802;
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
				this.state = 807;
				this.match(SqlBaseParser.SHOW);
				this.state = 808;
				this.match(SqlBaseParser.COLUMNS);
				this.state = 809;
				_la = this._input.LA(1);
				if(!(_la===78 || _la===93)) {
				this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 810;
				this.qualifiedName();
				}
				break;
			case 52:
				localctx = new ShowStatsContext(this, localctx);
				this.enterOuterAlt(localctx, 52);
				{
				this.state = 811;
				this.match(SqlBaseParser.SHOW);
				this.state = 812;
				this.match(SqlBaseParser.STATS);
				this.state = 813;
				this.match(SqlBaseParser.FOR);
				this.state = 814;
				this.qualifiedName();
				}
				break;
			case 53:
				localctx = new ShowStatsForQueryContext(this, localctx);
				this.enterOuterAlt(localctx, 53);
				{
				this.state = 815;
				this.match(SqlBaseParser.SHOW);
				this.state = 816;
				this.match(SqlBaseParser.STATS);
				this.state = 817;
				this.match(SqlBaseParser.FOR);
				this.state = 818;
				this.match(SqlBaseParser.T__1);
				this.state = 819;
				this.querySpecification();
				this.state = 820;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 54:
				localctx = new ShowRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 54);
				{
				this.state = 822;
				this.match(SqlBaseParser.SHOW);
				this.state = 824;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===40) {
					{
					this.state = 823;
					this.match(SqlBaseParser.CURRENT);
					}
				}

				this.state = 826;
				this.match(SqlBaseParser.ROLES);
				this.state = 829;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 827;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 828;
					this.identifier();
					}
				}

				}
				break;
			case 55:
				localctx = new ShowRoleGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 55);
				{
				this.state = 831;
				this.match(SqlBaseParser.SHOW);
				this.state = 832;
				this.match(SqlBaseParser.ROLE);
				this.state = 833;
				this.match(SqlBaseParser.GRANTS);
				this.state = 836;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 834;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 835;
					this.identifier();
					}
				}

				}
				break;
			case 56:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 56);
				{
				this.state = 838;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 839;
				this.qualifiedName();
				}
				break;
			case 57:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 57);
				{
				this.state = 840;
				this.match(SqlBaseParser.DESC);
				this.state = 841;
				this.qualifiedName();
				}
				break;
			case 58:
				localctx = new ShowFunctionsContext(this, localctx);
				this.enterOuterAlt(localctx, 58);
				{
				this.state = 842;
				this.match(SqlBaseParser.SHOW);
				this.state = 843;
				this.match(SqlBaseParser.FUNCTIONS);
				this.state = 850;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 844;
					this.match(SqlBaseParser.LIKE);
					this.state = 845;
					(localctx as ShowFunctionsContext)._pattern = this.string_();
					this.state = 848;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 846;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 847;
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
				this.state = 852;
				this.match(SqlBaseParser.SHOW);
				this.state = 853;
				this.match(SqlBaseParser.SESSION);
				this.state = 860;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 854;
					this.match(SqlBaseParser.LIKE);
					this.state = 855;
					(localctx as ShowSessionContext)._pattern = this.string_();
					this.state = 858;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 856;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 857;
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
				this.state = 862;
				this.match(SqlBaseParser.SET);
				this.state = 863;
				this.match(SqlBaseParser.SESSION);
				this.state = 864;
				this.qualifiedName();
				this.state = 865;
				this.match(SqlBaseParser.EQ);
				this.state = 866;
				this.expression();
				}
				break;
			case 61:
				localctx = new ResetSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 61);
				{
				this.state = 868;
				this.match(SqlBaseParser.RESET);
				this.state = 869;
				this.match(SqlBaseParser.SESSION);
				this.state = 870;
				this.qualifiedName();
				}
				break;
			case 62:
				localctx = new StartTransactionContext(this, localctx);
				this.enterOuterAlt(localctx, 62);
				{
				this.state = 871;
				this.match(SqlBaseParser.START);
				this.state = 872;
				this.match(SqlBaseParser.TRANSACTION);
				this.state = 881;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===104 || _la===155) {
					{
					this.state = 873;
					this.transactionMode();
					this.state = 878;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 874;
						this.match(SqlBaseParser.T__3);
						this.state = 875;
						this.transactionMode();
						}
						}
						this.state = 880;
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
				this.state = 883;
				this.match(SqlBaseParser.COMMIT);
				this.state = 885;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 884;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 64:
				localctx = new RollbackContext(this, localctx);
				this.enterOuterAlt(localctx, 64);
				{
				this.state = 887;
				this.match(SqlBaseParser.ROLLBACK);
				this.state = 889;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 888;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 65:
				localctx = new PrepareContext(this, localctx);
				this.enterOuterAlt(localctx, 65);
				{
				this.state = 891;
				this.match(SqlBaseParser.PREPARE);
				this.state = 892;
				this.identifier();
				this.state = 893;
				this.match(SqlBaseParser.FROM);
				this.state = 894;
				this.statement();
				}
				break;
			case 66:
				localctx = new DeallocateContext(this, localctx);
				this.enterOuterAlt(localctx, 66);
				{
				this.state = 896;
				this.match(SqlBaseParser.DEALLOCATE);
				this.state = 897;
				this.match(SqlBaseParser.PREPARE);
				this.state = 898;
				this.identifier();
				}
				break;
			case 67:
				localctx = new ExecuteContext(this, localctx);
				this.enterOuterAlt(localctx, 67);
				{
				this.state = 899;
				this.match(SqlBaseParser.EXECUTE);
				this.state = 900;
				this.identifier();
				this.state = 910;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===216) {
					{
					this.state = 901;
					this.match(SqlBaseParser.USING);
					this.state = 902;
					this.expression();
					this.state = 907;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 903;
						this.match(SqlBaseParser.T__3);
						this.state = 904;
						this.expression();
						}
						}
						this.state = 909;
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
				this.state = 912;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 913;
				this.match(SqlBaseParser.INPUT);
				this.state = 914;
				this.identifier();
				}
				break;
			case 69:
				localctx = new DescribeOutputContext(this, localctx);
				this.enterOuterAlt(localctx, 69);
				{
				this.state = 915;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 916;
				this.match(SqlBaseParser.OUTPUT);
				this.state = 917;
				this.identifier();
				}
				break;
			case 70:
				localctx = new UpdateContext(this, localctx);
				this.enterOuterAlt(localctx, 70);
				{
				this.state = 918;
				this.match(SqlBaseParser.UPDATE);
				this.state = 919;
				this.qualifiedName();
				this.state = 920;
				this.match(SqlBaseParser.SET);
				this.state = 921;
				this.updateAssignment();
				this.state = 926;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 922;
					this.match(SqlBaseParser.T__3);
					this.state = 923;
					this.updateAssignment();
					}
					}
					this.state = 928;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 931;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 929;
					this.match(SqlBaseParser.WHERE);
					this.state = 930;
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
		this.enterRule(localctx, 10, SqlBaseParser.RULE_query);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 936;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 935;
				this.with_();
				}
			}

			this.state = 938;
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
		this.enterRule(localctx, 12, SqlBaseParser.RULE_with);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 940;
			this.match(SqlBaseParser.WITH);
			this.state = 942;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===156) {
				{
				this.state = 941;
				this.match(SqlBaseParser.RECURSIVE);
				}
			}

			this.state = 944;
			this.namedQuery();
			this.state = 949;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 945;
				this.match(SqlBaseParser.T__3);
				this.state = 946;
				this.namedQuery();
				}
				}
				this.state = 951;
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
		this.enterRule(localctx, 14, SqlBaseParser.RULE_tableElement);
		try {
			this.state = 955;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 106, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 952;
				this.constraintSpecification();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 953;
				this.columnDefinition();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 954;
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
		this.enterRule(localctx, 16, SqlBaseParser.RULE_columnDefinition);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 957;
			this.identifier();
			this.state = 958;
			this.type_(0);
			this.state = 961;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===131) {
				{
				this.state = 959;
				this.match(SqlBaseParser.NOT);
				this.state = 960;
				this.match(SqlBaseParser.NULL);
				}
			}

			this.state = 965;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===33) {
				{
				this.state = 963;
				this.match(SqlBaseParser.COMMENT);
				this.state = 964;
				this.string_();
				}
			}

			this.state = 969;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 967;
				this.match(SqlBaseParser.WITH);
				this.state = 968;
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
		this.enterRule(localctx, 18, SqlBaseParser.RULE_likeClause);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 971;
			this.match(SqlBaseParser.LIKE);
			this.state = 972;
			this.qualifiedName();
			this.state = 975;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===65 || _la===94) {
				{
				this.state = 973;
				localctx._optionType = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===65 || _la===94)) {
				    localctx._optionType = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 974;
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
		this.enterRule(localctx, 20, SqlBaseParser.RULE_properties);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 977;
			this.match(SqlBaseParser.T__1);
			this.state = 978;
			this.property();
			this.state = 983;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 979;
				this.match(SqlBaseParser.T__3);
				this.state = 980;
				this.property();
				}
				}
				this.state = 985;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 986;
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
		this.enterRule(localctx, 22, SqlBaseParser.RULE_property);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 988;
			this.identifier();
			this.state = 989;
			this.match(SqlBaseParser.EQ);
			this.state = 990;
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
		this.enterRule(localctx, 24, SqlBaseParser.RULE_sqlParameterDeclaration);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 992;
			this.identifier();
			this.state = 993;
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
		this.enterRule(localctx, 26, SqlBaseParser.RULE_routineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 998;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===54 || _la===108 || _la===131 || _la===166) {
				{
				{
				this.state = 995;
				this.routineCharacteristic();
				}
				}
				this.state = 1000;
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
		this.enterRule(localctx, 28, SqlBaseParser.RULE_routineCharacteristic);
		try {
			this.state = 1005;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 108:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1001;
				this.match(SqlBaseParser.LANGUAGE);
				this.state = 1002;
				this.language();
				}
				break;
			case 54:
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1003;
				this.determinism();
				}
				break;
			case 26:
			case 166:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1004;
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
		this.enterRule(localctx, 30, SqlBaseParser.RULE_alterRoutineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1010;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===166) {
				{
				{
				this.state = 1007;
				this.alterRoutineCharacteristic();
				}
				}
				this.state = 1012;
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
		this.enterRule(localctx, 32, SqlBaseParser.RULE_alterRoutineCharacteristic);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1013;
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
		this.enterRule(localctx, 34, SqlBaseParser.RULE_routineBody);
		try {
			this.state = 1017;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 165:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1015;
				this.returnStatement();
				}
				break;
			case 70:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1016;
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
		this.enterRule(localctx, 36, SqlBaseParser.RULE_returnStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1019;
			this.match(SqlBaseParser.RETURN);
			this.state = 1020;
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
		this.enterRule(localctx, 38, SqlBaseParser.RULE_externalBodyReference);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1022;
			this.match(SqlBaseParser.EXTERNAL);
			this.state = 1025;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===122) {
				{
				this.state = 1023;
				this.match(SqlBaseParser.NAME);
				this.state = 1024;
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
		this.enterRule(localctx, 40, SqlBaseParser.RULE_language);
		try {
			this.state = 1029;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 117, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1027;
				this.match(SqlBaseParser.SQL);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1028;
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
		this.enterRule(localctx, 42, SqlBaseParser.RULE_determinism);
		try {
			this.state = 1034;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 54:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1031;
				this.match(SqlBaseParser.DETERMINISTIC);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1032;
				this.match(SqlBaseParser.NOT);
				this.state = 1033;
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
		this.enterRule(localctx, 44, SqlBaseParser.RULE_nullCallClause);
		try {
			this.state = 1045;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 166:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1036;
				this.match(SqlBaseParser.RETURNS);
				this.state = 1037;
				this.match(SqlBaseParser.NULL);
				this.state = 1038;
				this.match(SqlBaseParser.ON);
				this.state = 1039;
				this.match(SqlBaseParser.NULL);
				this.state = 1040;
				this.match(SqlBaseParser.INPUT);
				}
				break;
			case 26:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1041;
				this.match(SqlBaseParser.CALLED);
				this.state = 1042;
				this.match(SqlBaseParser.ON);
				this.state = 1043;
				this.match(SqlBaseParser.NULL);
				this.state = 1044;
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
		this.enterRule(localctx, 46, SqlBaseParser.RULE_externalRoutineName);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1047;
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
		this.enterRule(localctx, 48, SqlBaseParser.RULE_queryNoWith);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1049;
			this.queryTerm(0);
			this.state = 1060;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1050;
				this.match(SqlBaseParser.ORDER);
				this.state = 1051;
				this.match(SqlBaseParser.BY);
				this.state = 1052;
				this.sortItem();
				this.state = 1057;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1053;
					this.match(SqlBaseParser.T__3);
					this.state = 1054;
					this.sortItem();
					}
					}
					this.state = 1059;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1067;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===136) {
				{
				this.state = 1062;
				this.match(SqlBaseParser.OFFSET);
				this.state = 1063;
				localctx._offset = this.match(SqlBaseParser.INTEGER_VALUE);
				this.state = 1065;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===173 || _la===174) {
					{
					this.state = 1064;
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

			this.state = 1078;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===72 || _la===114) {
				{
				this.state = 1076;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 114:
					{
					this.state = 1069;
					this.match(SqlBaseParser.LIMIT);
					this.state = 1070;
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
					this.state = 1071;
					this.match(SqlBaseParser.FETCH);
					this.state = 1072;
					this.match(SqlBaseParser.FIRST);
					this.state = 1073;
					localctx._fetchFirstNRows = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1074;
					this.match(SqlBaseParser.ROWS);
					this.state = 1075;
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
		let _startState: number = 50;
		this.enterRecursionRule(localctx, 50, SqlBaseParser.RULE_queryTerm, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new QueryTermDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1081;
			this.queryPrimary();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1097;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 129, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1095;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 128, this._ctx) ) {
					case 1:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1083;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1084;
						(localctx as SetOperationContext)._operator = this.match(SqlBaseParser.INTERSECT);
						this.state = 1086;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1085;
							this.setQuantifier();
							}
						}

						this.state = 1088;
						(localctx as SetOperationContext)._right = this.queryTerm(3);
						}
						break;
					case 2:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1089;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1090;
						(localctx as SetOperationContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===64 || _la===210)) {
						    (localctx as SetOperationContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1092;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1091;
							this.setQuantifier();
							}
						}

						this.state = 1094;
						(localctx as SetOperationContext)._right = this.queryTerm(2);
						}
						break;
					}
					}
				}
				this.state = 1099;
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
		this.enterRule(localctx, 52, SqlBaseParser.RULE_queryPrimary);
		try {
			let _alt: number;
			this.state = 1116;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				localctx = new QueryPrimaryDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1100;
				this.querySpecification();
				}
				break;
			case 193:
				localctx = new TableContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1101;
				this.match(SqlBaseParser.TABLE);
				this.state = 1102;
				this.qualifiedName();
				}
				break;
			case 218:
				localctx = new InlineTableContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1103;
				this.match(SqlBaseParser.VALUES);
				this.state = 1104;
				this.expression();
				this.state = 1109;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1105;
						this.match(SqlBaseParser.T__3);
						this.state = 1106;
						this.expression();
						}
						}
					}
					this.state = 1111;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				}
				}
				break;
			case 2:
				localctx = new SubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1112;
				this.match(SqlBaseParser.T__1);
				this.state = 1113;
				this.queryNoWith();
				this.state = 1114;
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
		this.enterRule(localctx, 54, SqlBaseParser.RULE_sortItem);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1118;
			this.expression();
			this.state = 1120;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===19 || _la===52) {
				{
				this.state = 1119;
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

			this.state = 1124;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===134) {
				{
				this.state = 1122;
				this.match(SqlBaseParser.NULLS);
				this.state = 1123;
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
		this.enterRule(localctx, 56, SqlBaseParser.RULE_querySpecification);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1126;
			this.match(SqlBaseParser.SELECT);
			this.state = 1128;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 134, this._ctx) ) {
			case 1:
				{
				this.state = 1127;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1130;
			this.selectItem();
			this.state = 1135;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 135, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1131;
					this.match(SqlBaseParser.T__3);
					this.state = 1132;
					this.selectItem();
					}
					}
				}
				this.state = 1137;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 135, this._ctx);
			}
			this.state = 1147;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 137, this._ctx) ) {
			case 1:
				{
				this.state = 1138;
				this.match(SqlBaseParser.FROM);
				this.state = 1139;
				this.relation(0);
				this.state = 1144;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 136, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1140;
						this.match(SqlBaseParser.T__3);
						this.state = 1141;
						this.relation(0);
						}
						}
					}
					this.state = 1146;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 136, this._ctx);
				}
				}
				break;
			}
			this.state = 1151;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 138, this._ctx) ) {
			case 1:
				{
				this.state = 1149;
				this.match(SqlBaseParser.WHERE);
				this.state = 1150;
				localctx._where = this.booleanExpression(0);
				}
				break;
			}
			this.state = 1156;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 139, this._ctx) ) {
			case 1:
				{
				this.state = 1153;
				this.match(SqlBaseParser.GROUP);
				this.state = 1154;
				this.match(SqlBaseParser.BY);
				this.state = 1155;
				this.groupBy();
				}
				break;
			}
			this.state = 1160;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 140, this._ctx) ) {
			case 1:
				{
				this.state = 1158;
				this.match(SqlBaseParser.HAVING);
				this.state = 1159;
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
	public groupBy(): GroupByContext {
		let localctx: GroupByContext = new GroupByContext(this, this._ctx, this.state);
		this.enterRule(localctx, 58, SqlBaseParser.RULE_groupBy);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1163;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 141, this._ctx) ) {
			case 1:
				{
				this.state = 1162;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1165;
			this.groupingElement();
			this.state = 1170;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 142, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1166;
					this.match(SqlBaseParser.T__3);
					this.state = 1167;
					this.groupingElement();
					}
					}
				}
				this.state = 1172;
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
		this.enterRule(localctx, 60, SqlBaseParser.RULE_groupingElement);
		let _la: number;
		try {
			this.state = 1213;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 148, this._ctx) ) {
			case 1:
				localctx = new SingleGroupingSetContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1173;
				this.groupingSet();
				}
				break;
			case 2:
				localctx = new RollupContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1174;
				this.match(SqlBaseParser.ROLLUP);
				this.state = 1175;
				this.match(SqlBaseParser.T__1);
				this.state = 1184;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1176;
					this.expression();
					this.state = 1181;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1177;
						this.match(SqlBaseParser.T__3);
						this.state = 1178;
						this.expression();
						}
						}
						this.state = 1183;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1186;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new CubeContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1187;
				this.match(SqlBaseParser.CUBE);
				this.state = 1188;
				this.match(SqlBaseParser.T__1);
				this.state = 1197;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1189;
					this.expression();
					this.state = 1194;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1190;
						this.match(SqlBaseParser.T__3);
						this.state = 1191;
						this.expression();
						}
						}
						this.state = 1196;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1199;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				localctx = new MultipleGroupingSetsContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1200;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1201;
				this.match(SqlBaseParser.SETS);
				this.state = 1202;
				this.match(SqlBaseParser.T__1);
				this.state = 1203;
				this.groupingSet();
				this.state = 1208;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1204;
					this.match(SqlBaseParser.T__3);
					this.state = 1205;
					this.groupingSet();
					}
					}
					this.state = 1210;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1211;
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
		this.enterRule(localctx, 62, SqlBaseParser.RULE_groupingSet);
		let _la: number;
		try {
			this.state = 1228;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 151, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1215;
				this.match(SqlBaseParser.T__1);
				this.state = 1224;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1216;
					this.expression();
					this.state = 1221;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1217;
						this.match(SqlBaseParser.T__3);
						this.state = 1218;
						this.expression();
						}
						}
						this.state = 1223;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1226;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1227;
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
		this.enterRule(localctx, 64, SqlBaseParser.RULE_namedQuery);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1230;
			localctx._name = this.identifier();
			this.state = 1232;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===2) {
				{
				this.state = 1231;
				this.columnAliases();
				}
			}

			this.state = 1234;
			this.match(SqlBaseParser.AS);
			this.state = 1235;
			this.match(SqlBaseParser.T__1);
			this.state = 1236;
			this.query();
			this.state = 1237;
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
		this.enterRule(localctx, 66, SqlBaseParser.RULE_setQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1239;
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
		this.enterRule(localctx, 68, SqlBaseParser.RULE_selectItem);
		let _la: number;
		try {
			this.state = 1253;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 155, this._ctx) ) {
			case 1:
				localctx = new SelectSingleContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1241;
				this.expression();
				this.state = 1246;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 154, this._ctx) ) {
				case 1:
					{
					this.state = 1243;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===18) {
						{
						this.state = 1242;
						this.match(SqlBaseParser.AS);
						}
					}

					this.state = 1245;
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
				this.state = 1248;
				this.qualifiedName();
				this.state = 1249;
				this.match(SqlBaseParser.T__0);
				this.state = 1250;
				this.match(SqlBaseParser.ASTERISK);
				}
				break;
			case 3:
				localctx = new SelectAllContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1252;
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
		let _startState: number = 70;
		this.enterRecursionRule(localctx, 70, SqlBaseParser.RULE_relation, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new RelationDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1256;
			this.sampledRelation();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1276;
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
					this.state = 1258;
					if (!(this.precpred(this._ctx, 2))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
					}
					this.state = 1272;
					this._errHandler.sync(this);
					switch (this._input.LA(1)) {
					case 38:
						{
						this.state = 1259;
						this.match(SqlBaseParser.CROSS);
						this.state = 1260;
						this.match(SqlBaseParser.JOIN);
						this.state = 1261;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					case 79:
					case 95:
					case 106:
					case 111:
					case 168:
						{
						this.state = 1262;
						this.joinType();
						this.state = 1263;
						this.match(SqlBaseParser.JOIN);
						this.state = 1264;
						(localctx as JoinRelationContext)._rightRelation = this.relation(0);
						this.state = 1265;
						this.joinCriteria();
						}
						break;
					case 123:
						{
						this.state = 1267;
						this.match(SqlBaseParser.NATURAL);
						this.state = 1268;
						this.joinType();
						this.state = 1269;
						this.match(SqlBaseParser.JOIN);
						this.state = 1270;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					default:
						throw new NoViableAltException(this);
					}
					}
					}
				}
				this.state = 1278;
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
		this.enterRule(localctx, 72, SqlBaseParser.RULE_joinType);
		let _la: number;
		try {
			this.state = 1294;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 95:
			case 106:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1280;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===95) {
					{
					this.state = 1279;
					this.match(SqlBaseParser.INNER);
					}
				}

				}
				break;
			case 111:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1282;
				this.match(SqlBaseParser.LEFT);
				this.state = 1284;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1283;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 168:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1286;
				this.match(SqlBaseParser.RIGHT);
				this.state = 1288;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1287;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 79:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1290;
				this.match(SqlBaseParser.FULL);
				this.state = 1292;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1291;
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
		this.enterRule(localctx, 74, SqlBaseParser.RULE_joinCriteria);
		let _la: number;
		try {
			this.state = 1310;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 137:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1296;
				this.match(SqlBaseParser.ON);
				this.state = 1297;
				this.booleanExpression(0);
				}
				break;
			case 216:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1298;
				this.match(SqlBaseParser.USING);
				this.state = 1299;
				this.match(SqlBaseParser.T__1);
				this.state = 1300;
				this.identifier();
				this.state = 1305;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1301;
					this.match(SqlBaseParser.T__3);
					this.state = 1302;
					this.identifier();
					}
					}
					this.state = 1307;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1308;
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
		this.enterRule(localctx, 76, SqlBaseParser.RULE_sampledRelation);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1312;
			this.aliasedRelation();
			this.state = 1319;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 165, this._ctx) ) {
			case 1:
				{
				this.state = 1313;
				this.match(SqlBaseParser.TABLESAMPLE);
				this.state = 1314;
				this.sampleType();
				this.state = 1315;
				this.match(SqlBaseParser.T__1);
				this.state = 1316;
				localctx._percentage = this.expression();
				this.state = 1317;
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
		this.enterRule(localctx, 78, SqlBaseParser.RULE_sampleType);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1321;
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
		this.enterRule(localctx, 80, SqlBaseParser.RULE_aliasedRelation);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1323;
			this.relationPrimary();
			this.state = 1331;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 168, this._ctx) ) {
			case 1:
				{
				this.state = 1325;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===18) {
					{
					this.state = 1324;
					this.match(SqlBaseParser.AS);
					}
				}

				this.state = 1327;
				this.identifier();
				this.state = 1329;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 167, this._ctx) ) {
				case 1:
					{
					this.state = 1328;
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
		this.enterRule(localctx, 82, SqlBaseParser.RULE_columnAliases);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1333;
			this.match(SqlBaseParser.T__1);
			this.state = 1334;
			this.identifier();
			this.state = 1339;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 1335;
				this.match(SqlBaseParser.T__3);
				this.state = 1336;
				this.identifier();
				}
				}
				this.state = 1341;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 1342;
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
		this.enterRule(localctx, 84, SqlBaseParser.RULE_relationPrimary);
		let _la: number;
		try {
			this.state = 1376;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 173, this._ctx) ) {
			case 1:
				localctx = new TableNameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1344;
				this.qualifiedName();
				this.state = 1346;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 170, this._ctx) ) {
				case 1:
					{
					this.state = 1345;
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
				this.state = 1348;
				this.match(SqlBaseParser.T__1);
				this.state = 1349;
				this.query();
				this.state = 1350;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new UnnestContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1352;
				this.match(SqlBaseParser.UNNEST);
				this.state = 1353;
				this.match(SqlBaseParser.T__1);
				this.state = 1354;
				this.expression();
				this.state = 1359;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1355;
					this.match(SqlBaseParser.T__3);
					this.state = 1356;
					this.expression();
					}
					}
					this.state = 1361;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1362;
				this.match(SqlBaseParser.T__2);
				this.state = 1365;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 172, this._ctx) ) {
				case 1:
					{
					this.state = 1363;
					this.match(SqlBaseParser.WITH);
					this.state = 1364;
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
				this.state = 1367;
				this.match(SqlBaseParser.LATERAL);
				this.state = 1368;
				this.match(SqlBaseParser.T__1);
				this.state = 1369;
				this.query();
				this.state = 1370;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new ParenthesizedRelationContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1372;
				this.match(SqlBaseParser.T__1);
				this.state = 1373;
				this.relation(0);
				this.state = 1374;
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
		this.enterRule(localctx, 86, SqlBaseParser.RULE_expression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1378;
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
		let _startState: number = 88;
		this.enterRecursionRule(localctx, 88, SqlBaseParser.RULE_booleanExpression, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1387;
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

				this.state = 1381;
				(localctx as PredicatedContext)._valueExpression = this.valueExpression(0);
				this.state = 1383;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 174, this._ctx) ) {
				case 1:
					{
					this.state = 1382;
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
				this.state = 1385;
				this.match(SqlBaseParser.NOT);
				this.state = 1386;
				this.booleanExpression(3);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1397;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 177, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1395;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 176, this._ctx) ) {
					case 1:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1389;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1390;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.AND);
						this.state = 1391;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(3);
						}
						break;
					case 2:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1392;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1393;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.OR);
						this.state = 1394;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(2);
						}
						break;
					}
					}
				}
				this.state = 1399;
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
		this.enterRule(localctx, 90, SqlBaseParser.RULE_predicate);
		let _la: number;
		try {
			this.state = 1461;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 186, this._ctx) ) {
			case 1:
				localctx = new ComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1400;
				this.comparisonOperator();
				this.state = 1401;
				(localctx as ComparisonContext)._right = this.valueExpression(0);
				}
				break;
			case 2:
				localctx = new QuantifiedComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1403;
				this.comparisonOperator();
				this.state = 1404;
				this.comparisonQuantifier();
				this.state = 1405;
				this.match(SqlBaseParser.T__1);
				this.state = 1406;
				this.query();
				this.state = 1407;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new BetweenContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1410;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1409;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1412;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1413;
				(localctx as BetweenContext)._lower = this.valueExpression(0);
				this.state = 1414;
				this.match(SqlBaseParser.AND);
				this.state = 1415;
				(localctx as BetweenContext)._upper = this.valueExpression(0);
				}
				break;
			case 4:
				localctx = new InListContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1418;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1417;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1420;
				this.match(SqlBaseParser.IN);
				this.state = 1421;
				this.match(SqlBaseParser.T__1);
				this.state = 1422;
				this.expression();
				this.state = 1427;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1423;
					this.match(SqlBaseParser.T__3);
					this.state = 1424;
					this.expression();
					}
					}
					this.state = 1429;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1430;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new InSubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1433;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1432;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1435;
				this.match(SqlBaseParser.IN);
				this.state = 1436;
				this.match(SqlBaseParser.T__1);
				this.state = 1437;
				this.query();
				this.state = 1438;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 6:
				localctx = new LikeContext(this, localctx);
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1441;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1440;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1443;
				this.match(SqlBaseParser.LIKE);
				this.state = 1444;
				(localctx as LikeContext)._pattern = this.valueExpression(0);
				this.state = 1447;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 183, this._ctx) ) {
				case 1:
					{
					this.state = 1445;
					this.match(SqlBaseParser.ESCAPE);
					this.state = 1446;
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
				this.state = 1449;
				this.match(SqlBaseParser.IS);
				this.state = 1451;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1450;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1453;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 8:
				localctx = new DistinctFromContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 1454;
				this.match(SqlBaseParser.IS);
				this.state = 1456;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1455;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1458;
				this.match(SqlBaseParser.DISTINCT);
				this.state = 1459;
				this.match(SqlBaseParser.FROM);
				this.state = 1460;
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
		let _startState: number = 92;
		this.enterRecursionRule(localctx, 92, SqlBaseParser.RULE_valueExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1467;
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

				this.state = 1464;
				this.primaryExpression(0);
				}
				break;
			case 235:
			case 236:
				{
				localctx = new ArithmeticUnaryContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1465;
				(localctx as ArithmeticUnaryContext)._operator = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===235 || _la===236)) {
				    (localctx as ArithmeticUnaryContext)._operator = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1466;
				this.valueExpression(4);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1483;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 189, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1481;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 188, this._ctx) ) {
					case 1:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1469;
						if (!(this.precpred(this._ctx, 3))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 3)");
						}
						this.state = 1470;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(((((_la - 237)) & ~0x1F) === 0 && ((1 << (_la - 237)) & 7) !== 0))) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1471;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(4);
						}
						break;
					case 2:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1472;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1473;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===235 || _la===236)) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1474;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(3);
						}
						break;
					case 3:
						{
						localctx = new ConcatenationContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ConcatenationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1475;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1476;
						this.match(SqlBaseParser.CONCAT);
						this.state = 1477;
						(localctx as ConcatenationContext)._right = this.valueExpression(2);
						}
						break;
					case 4:
						{
						localctx = new AtTimeZoneContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1478;
						if (!(this.precpred(this._ctx, 5))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 5)");
						}
						this.state = 1479;
						this.match(SqlBaseParser.AT);
						this.state = 1480;
						this.timeZoneSpecifier();
						}
						break;
					}
					}
				}
				this.state = 1485;
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
		let _startState: number = 94;
		this.enterRecursionRule(localctx, 94, SqlBaseParser.RULE_primaryExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1725;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 218, this._ctx) ) {
			case 1:
				{
				localctx = new NullLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1487;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 2:
				{
				localctx = new IntervalLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1488;
				this.interval();
				}
				break;
			case 3:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1489;
				this.identifier();
				this.state = 1490;
				this.string_();
				}
				break;
			case 4:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1492;
				this.match(SqlBaseParser.DOUBLE_PRECISION);
				this.state = 1493;
				this.string_();
				}
				break;
			case 5:
				{
				localctx = new NumericLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1494;
				this.number_();
				}
				break;
			case 6:
				{
				localctx = new BooleanLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1495;
				this.booleanValue();
				}
				break;
			case 7:
				{
				localctx = new StringLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1496;
				this.string_();
				}
				break;
			case 8:
				{
				localctx = new BinaryLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1497;
				this.match(SqlBaseParser.BINARY_LITERAL);
				}
				break;
			case 9:
				{
				localctx = new ParameterContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1498;
				this.match(SqlBaseParser.T__4);
				}
				break;
			case 10:
				{
				localctx = new PositionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1499;
				this.match(SqlBaseParser.POSITION);
				this.state = 1500;
				this.match(SqlBaseParser.T__1);
				this.state = 1501;
				this.valueExpression(0);
				this.state = 1502;
				this.match(SqlBaseParser.IN);
				this.state = 1503;
				this.valueExpression(0);
				this.state = 1504;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 11:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1506;
				this.match(SqlBaseParser.T__1);
				this.state = 1507;
				this.expression();
				this.state = 1510;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1508;
					this.match(SqlBaseParser.T__3);
					this.state = 1509;
					this.expression();
					}
					}
					this.state = 1512;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===4);
				this.state = 1514;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 12:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1516;
				this.match(SqlBaseParser.ROW);
				this.state = 1517;
				this.match(SqlBaseParser.T__1);
				this.state = 1518;
				this.expression();
				this.state = 1523;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1519;
					this.match(SqlBaseParser.T__3);
					this.state = 1520;
					this.expression();
					}
					}
					this.state = 1525;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1526;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 13:
				{
				localctx = new FunctionCallContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1528;
				this.qualifiedName();
				this.state = 1529;
				this.match(SqlBaseParser.T__1);
				this.state = 1530;
				this.match(SqlBaseParser.ASTERISK);
				this.state = 1531;
				this.match(SqlBaseParser.T__2);
				this.state = 1533;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 192, this._ctx) ) {
				case 1:
					{
					this.state = 1532;
					this.filter();
					}
					break;
				}
				this.state = 1536;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 193, this._ctx) ) {
				case 1:
					{
					this.state = 1535;
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
				this.state = 1538;
				this.qualifiedName();
				this.state = 1539;
				this.match(SqlBaseParser.T__1);
				this.state = 1551;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1406533391) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1541;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 194, this._ctx) ) {
					case 1:
						{
						this.state = 1540;
						this.setQuantifier();
						}
						break;
					}
					this.state = 1543;
					this.expression();
					this.state = 1548;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1544;
						this.match(SqlBaseParser.T__3);
						this.state = 1545;
						this.expression();
						}
						}
						this.state = 1550;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1563;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===141) {
					{
					this.state = 1553;
					this.match(SqlBaseParser.ORDER);
					this.state = 1554;
					this.match(SqlBaseParser.BY);
					this.state = 1555;
					this.sortItem();
					this.state = 1560;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1556;
						this.match(SqlBaseParser.T__3);
						this.state = 1557;
						this.sortItem();
						}
						}
						this.state = 1562;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1565;
				this.match(SqlBaseParser.T__2);
				this.state = 1567;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 199, this._ctx) ) {
				case 1:
					{
					this.state = 1566;
					this.filter();
					}
					break;
				}
				this.state = 1573;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 201, this._ctx) ) {
				case 1:
					{
					this.state = 1570;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===92 || _la===163) {
						{
						this.state = 1569;
						this.nullTreatment();
						}
					}

					this.state = 1572;
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
				this.state = 1575;
				this.identifier();
				this.state = 1576;
				this.match(SqlBaseParser.T__5);
				this.state = 1577;
				this.expression();
				}
				break;
			case 16:
				{
				localctx = new LambdaContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1579;
				this.match(SqlBaseParser.T__1);
				this.state = 1588;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1580;
					this.identifier();
					this.state = 1585;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1581;
						this.match(SqlBaseParser.T__3);
						this.state = 1582;
						this.identifier();
						}
						}
						this.state = 1587;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1590;
				this.match(SqlBaseParser.T__2);
				this.state = 1591;
				this.match(SqlBaseParser.T__5);
				this.state = 1592;
				this.expression();
				}
				break;
			case 17:
				{
				localctx = new SubqueryExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1593;
				this.match(SqlBaseParser.T__1);
				this.state = 1594;
				this.query();
				this.state = 1595;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 18:
				{
				localctx = new ExistsContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1597;
				this.match(SqlBaseParser.EXISTS);
				this.state = 1598;
				this.match(SqlBaseParser.T__1);
				this.state = 1599;
				this.query();
				this.state = 1600;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 19:
				{
				localctx = new SimpleCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1602;
				this.match(SqlBaseParser.CASE);
				this.state = 1603;
				this.valueExpression(0);
				this.state = 1605;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1604;
					this.whenClause();
					}
					}
					this.state = 1607;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1611;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1609;
					this.match(SqlBaseParser.ELSE);
					this.state = 1610;
					(localctx as SimpleCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1613;
				this.match(SqlBaseParser.END);
				}
				break;
			case 20:
				{
				localctx = new SearchedCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1615;
				this.match(SqlBaseParser.CASE);
				this.state = 1617;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1616;
					this.whenClause();
					}
					}
					this.state = 1619;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1623;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1621;
					this.match(SqlBaseParser.ELSE);
					this.state = 1622;
					(localctx as SearchedCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1625;
				this.match(SqlBaseParser.END);
				}
				break;
			case 21:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1627;
				this.match(SqlBaseParser.CAST);
				this.state = 1628;
				this.match(SqlBaseParser.T__1);
				this.state = 1629;
				this.expression();
				this.state = 1630;
				this.match(SqlBaseParser.AS);
				this.state = 1631;
				this.type_(0);
				this.state = 1632;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 22:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1634;
				this.match(SqlBaseParser.TRY_CAST);
				this.state = 1635;
				this.match(SqlBaseParser.T__1);
				this.state = 1636;
				this.expression();
				this.state = 1637;
				this.match(SqlBaseParser.AS);
				this.state = 1638;
				this.type_(0);
				this.state = 1639;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 23:
				{
				localctx = new ArrayConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1641;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1642;
				this.match(SqlBaseParser.T__6);
				this.state = 1651;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1643;
					this.expression();
					this.state = 1648;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1644;
						this.match(SqlBaseParser.T__3);
						this.state = 1645;
						this.expression();
						}
						}
						this.state = 1650;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1653;
				this.match(SqlBaseParser.T__7);
				}
				break;
			case 24:
				{
				localctx = new ColumnReferenceContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1654;
				this.identifier();
				}
				break;
			case 25:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1655;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_DATE);
				}
				break;
			case 26:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1656;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIME);
				this.state = 1660;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 210, this._ctx) ) {
				case 1:
					{
					this.state = 1657;
					this.match(SqlBaseParser.T__1);
					this.state = 1658;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1659;
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
				this.state = 1662;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIMESTAMP);
				this.state = 1666;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 211, this._ctx) ) {
				case 1:
					{
					this.state = 1663;
					this.match(SqlBaseParser.T__1);
					this.state = 1664;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1665;
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
				this.state = 1668;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIME);
				this.state = 1672;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 212, this._ctx) ) {
				case 1:
					{
					this.state = 1669;
					this.match(SqlBaseParser.T__1);
					this.state = 1670;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1671;
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
				this.state = 1674;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIMESTAMP);
				this.state = 1678;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 213, this._ctx) ) {
				case 1:
					{
					this.state = 1675;
					this.match(SqlBaseParser.T__1);
					this.state = 1676;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1677;
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
				this.state = 1680;
				(localctx as CurrentUserContext)._name = this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 31:
				{
				localctx = new SubstringContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1681;
				this.match(SqlBaseParser.SUBSTRING);
				this.state = 1682;
				this.match(SqlBaseParser.T__1);
				this.state = 1683;
				this.valueExpression(0);
				this.state = 1684;
				this.match(SqlBaseParser.FROM);
				this.state = 1685;
				this.valueExpression(0);
				this.state = 1688;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===76) {
					{
					this.state = 1686;
					this.match(SqlBaseParser.FOR);
					this.state = 1687;
					this.valueExpression(0);
					}
				}

				this.state = 1690;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 32:
				{
				localctx = new NormalizeContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1692;
				this.match(SqlBaseParser.NORMALIZE);
				this.state = 1693;
				this.match(SqlBaseParser.T__1);
				this.state = 1694;
				this.valueExpression(0);
				this.state = 1697;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===4) {
					{
					this.state = 1695;
					this.match(SqlBaseParser.T__3);
					this.state = 1696;
					this.normalForm();
					}
				}

				this.state = 1699;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 33:
				{
				localctx = new ExtractContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1701;
				this.match(SqlBaseParser.EXTRACT);
				this.state = 1702;
				this.match(SqlBaseParser.T__1);
				this.state = 1703;
				this.identifier();
				this.state = 1704;
				this.match(SqlBaseParser.FROM);
				this.state = 1705;
				this.valueExpression(0);
				this.state = 1706;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				{
				localctx = new ParenthesizedExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1708;
				this.match(SqlBaseParser.T__1);
				this.state = 1709;
				this.expression();
				this.state = 1710;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 35:
				{
				localctx = new GroupingOperationContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1712;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1713;
				this.match(SqlBaseParser.T__1);
				this.state = 1722;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1714;
					this.qualifiedName();
					this.state = 1719;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1715;
						this.match(SqlBaseParser.T__3);
						this.state = 1716;
						this.qualifiedName();
						}
						}
						this.state = 1721;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1724;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1737;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 220, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1735;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 219, this._ctx) ) {
					case 1:
						{
						localctx = new SubscriptContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as SubscriptContext)._value = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1727;
						if (!(this.precpred(this._ctx, 14))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 14)");
						}
						this.state = 1728;
						this.match(SqlBaseParser.T__6);
						this.state = 1729;
						(localctx as SubscriptContext)._index = this.valueExpression(0);
						this.state = 1730;
						this.match(SqlBaseParser.T__7);
						}
						break;
					case 2:
						{
						localctx = new DereferenceContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as DereferenceContext)._base = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1732;
						if (!(this.precpred(this._ctx, 12))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 12)");
						}
						this.state = 1733;
						this.match(SqlBaseParser.T__0);
						this.state = 1734;
						(localctx as DereferenceContext)._fieldName = this.identifier();
						}
						break;
					}
					}
				}
				this.state = 1739;
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
		this.enterRule(localctx, 96, SqlBaseParser.RULE_string);
		try {
			this.state = 1746;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 241:
				localctx = new BasicStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1740;
				this.match(SqlBaseParser.STRING);
				}
				break;
			case 242:
				localctx = new UnicodeStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1741;
				this.match(SqlBaseParser.UNICODE_STRING);
				this.state = 1744;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 221, this._ctx) ) {
				case 1:
					{
					this.state = 1742;
					this.match(SqlBaseParser.UESCAPE);
					this.state = 1743;
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
		this.enterRule(localctx, 98, SqlBaseParser.RULE_nullTreatment);
		try {
			this.state = 1752;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 92:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1748;
				this.match(SqlBaseParser.IGNORE);
				this.state = 1749;
				this.match(SqlBaseParser.NULLS);
				}
				break;
			case 163:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1750;
				this.match(SqlBaseParser.RESPECT);
				this.state = 1751;
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
		this.enterRule(localctx, 100, SqlBaseParser.RULE_timeZoneSpecifier);
		try {
			this.state = 1760;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 224, this._ctx) ) {
			case 1:
				localctx = new TimeZoneIntervalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1754;
				this.match(SqlBaseParser.TIME);
				this.state = 1755;
				this.match(SqlBaseParser.ZONE);
				this.state = 1756;
				this.interval();
				}
				break;
			case 2:
				localctx = new TimeZoneStringContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1757;
				this.match(SqlBaseParser.TIME);
				this.state = 1758;
				this.match(SqlBaseParser.ZONE);
				this.state = 1759;
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
		this.enterRule(localctx, 102, SqlBaseParser.RULE_comparisonOperator);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1762;
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
		this.enterRule(localctx, 104, SqlBaseParser.RULE_comparisonQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1764;
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
		this.enterRule(localctx, 106, SqlBaseParser.RULE_booleanValue);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1766;
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
		this.enterRule(localctx, 108, SqlBaseParser.RULE_interval);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1768;
			this.match(SqlBaseParser.INTERVAL);
			this.state = 1770;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===235 || _la===236) {
				{
				this.state = 1769;
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

			this.state = 1772;
			this.string_();
			this.state = 1773;
			localctx._from_ = this.intervalField();
			this.state = 1776;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 226, this._ctx) ) {
			case 1:
				{
				this.state = 1774;
				this.match(SqlBaseParser.TO);
				this.state = 1775;
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
		this.enterRule(localctx, 110, SqlBaseParser.RULE_intervalField);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1778;
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
		this.enterRule(localctx, 112, SqlBaseParser.RULE_normalForm);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1780;
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
		this.enterRule(localctx, 114, SqlBaseParser.RULE_types);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1782;
			this.match(SqlBaseParser.T__1);
			this.state = 1791;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 127) !== 0)) {
				{
				this.state = 1783;
				this.type_(0);
				this.state = 1788;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1784;
					this.match(SqlBaseParser.T__3);
					this.state = 1785;
					this.type_(0);
					}
					}
					this.state = 1790;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1793;
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
		let _startState: number = 116;
		this.enterRecursionRule(localctx, 116, SqlBaseParser.RULE_type, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1842;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 232, this._ctx) ) {
			case 1:
				{
				this.state = 1796;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1797;
				this.match(SqlBaseParser.LT);
				this.state = 1798;
				this.type_(0);
				this.state = 1799;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 2:
				{
				this.state = 1801;
				this.match(SqlBaseParser.MAP);
				this.state = 1802;
				this.match(SqlBaseParser.LT);
				this.state = 1803;
				this.type_(0);
				this.state = 1804;
				this.match(SqlBaseParser.T__3);
				this.state = 1805;
				this.type_(0);
				this.state = 1806;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 3:
				{
				this.state = 1808;
				this.match(SqlBaseParser.ROW);
				this.state = 1809;
				this.match(SqlBaseParser.T__1);
				this.state = 1810;
				this.identifier();
				this.state = 1811;
				this.type_(0);
				this.state = 1818;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1812;
					this.match(SqlBaseParser.T__3);
					this.state = 1813;
					this.identifier();
					this.state = 1814;
					this.type_(0);
					}
					}
					this.state = 1820;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1821;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				{
				this.state = 1823;
				this.baseType();
				this.state = 1835;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 231, this._ctx) ) {
				case 1:
					{
					this.state = 1824;
					this.match(SqlBaseParser.T__1);
					this.state = 1825;
					this.typeParameter();
					this.state = 1830;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1826;
						this.match(SqlBaseParser.T__3);
						this.state = 1827;
						this.typeParameter();
						}
						}
						this.state = 1832;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 1833;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 5:
				{
				this.state = 1837;
				this.match(SqlBaseParser.INTERVAL);
				this.state = 1838;
				localctx._from_ = this.intervalField();
				this.state = 1839;
				this.match(SqlBaseParser.TO);
				this.state = 1840;
				localctx._to = this.intervalField();
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1848;
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
					this.state = 1844;
					if (!(this.precpred(this._ctx, 6))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 6)");
					}
					this.state = 1845;
					this.match(SqlBaseParser.ARRAY);
					}
					}
				}
				this.state = 1850;
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
		this.enterRule(localctx, 118, SqlBaseParser.RULE_typeParameter);
		try {
			this.state = 1853;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 244:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1851;
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
				this.state = 1852;
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
		this.enterRule(localctx, 120, SqlBaseParser.RULE_baseType);
		try {
			this.state = 1859;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 251:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1855;
				this.match(SqlBaseParser.TIME_WITH_TIME_ZONE);
				}
				break;
			case 252:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1856;
				this.match(SqlBaseParser.TIMESTAMP_WITH_TIME_ZONE);
				}
				break;
			case 253:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1857;
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
				this.state = 1858;
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
		this.enterRule(localctx, 122, SqlBaseParser.RULE_whenClause);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1861;
			this.match(SqlBaseParser.WHEN);
			this.state = 1862;
			localctx._condition = this.expression();
			this.state = 1863;
			this.match(SqlBaseParser.THEN);
			this.state = 1864;
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
		this.enterRule(localctx, 124, SqlBaseParser.RULE_filter);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1866;
			this.match(SqlBaseParser.FILTER);
			this.state = 1867;
			this.match(SqlBaseParser.T__1);
			this.state = 1868;
			this.match(SqlBaseParser.WHERE);
			this.state = 1869;
			this.booleanExpression(0);
			this.state = 1870;
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
		this.enterRule(localctx, 126, SqlBaseParser.RULE_over);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1872;
			this.match(SqlBaseParser.OVER);
			this.state = 1873;
			this.match(SqlBaseParser.T__1);
			this.state = 1884;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===146) {
				{
				this.state = 1874;
				this.match(SqlBaseParser.PARTITION);
				this.state = 1875;
				this.match(SqlBaseParser.BY);
				this.state = 1876;
				localctx._expression = this.expression();
				localctx._partition.push(localctx._expression);
				this.state = 1881;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1877;
					this.match(SqlBaseParser.T__3);
					this.state = 1878;
					localctx._expression = this.expression();
					localctx._partition.push(localctx._expression);
					}
					}
					this.state = 1883;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1896;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1886;
				this.match(SqlBaseParser.ORDER);
				this.state = 1887;
				this.match(SqlBaseParser.BY);
				this.state = 1888;
				this.sortItem();
				this.state = 1893;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1889;
					this.match(SqlBaseParser.T__3);
					this.state = 1890;
					this.sortItem();
					}
					}
					this.state = 1895;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1899;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===88 || _la===154 || _la===174) {
				{
				this.state = 1898;
				this.windowFrame();
				}
			}

			this.state = 1901;
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
		this.enterRule(localctx, 128, SqlBaseParser.RULE_windowFrame);
		try {
			this.state = 1927;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 241, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1903;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1904;
				localctx._start = this.frameBound();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1905;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1906;
				localctx._start = this.frameBound();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1907;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1908;
				localctx._start = this.frameBound();
				}
				break;
			case 4:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1909;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1910;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1911;
				localctx._start = this.frameBound();
				this.state = 1912;
				this.match(SqlBaseParser.AND);
				this.state = 1913;
				localctx._end = this.frameBound();
				}
				break;
			case 5:
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1915;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1916;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1917;
				localctx._start = this.frameBound();
				this.state = 1918;
				this.match(SqlBaseParser.AND);
				this.state = 1919;
				localctx._end = this.frameBound();
				}
				break;
			case 6:
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1921;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1922;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1923;
				localctx._start = this.frameBound();
				this.state = 1924;
				this.match(SqlBaseParser.AND);
				this.state = 1925;
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
		this.enterRule(localctx, 130, SqlBaseParser.RULE_frameBound);
		let _la: number;
		try {
			this.state = 1938;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 242, this._ctx) ) {
			case 1:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1929;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1930;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.PRECEDING);
				}
				break;
			case 2:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1931;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1932;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.FOLLOWING);
				}
				break;
			case 3:
				localctx = new CurrentRowBoundContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1933;
				this.match(SqlBaseParser.CURRENT);
				this.state = 1934;
				this.match(SqlBaseParser.ROW);
				}
				break;
			case 4:
				localctx = new BoundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1935;
				this.expression();
				this.state = 1936;
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
		this.enterRule(localctx, 132, SqlBaseParser.RULE_updateAssignment);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1940;
			this.identifier();
			this.state = 1941;
			this.match(SqlBaseParser.EQ);
			this.state = 1942;
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
		this.enterRule(localctx, 134, SqlBaseParser.RULE_explainOption);
		let _la: number;
		try {
			this.state = 1948;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 77:
				localctx = new ExplainFormatContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1944;
				this.match(SqlBaseParser.FORMAT);
				this.state = 1945;
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
				this.state = 1946;
				this.match(SqlBaseParser.TYPE);
				this.state = 1947;
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
		this.enterRule(localctx, 136, SqlBaseParser.RULE_transactionMode);
		let _la: number;
		try {
			this.state = 1955;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 104:
				localctx = new IsolationLevelContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1950;
				this.match(SqlBaseParser.ISOLATION);
				this.state = 1951;
				this.match(SqlBaseParser.LEVEL);
				this.state = 1952;
				this.levelOfIsolation();
				}
				break;
			case 155:
				localctx = new TransactionAccessModeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1953;
				this.match(SqlBaseParser.READ);
				this.state = 1954;
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
		this.enterRule(localctx, 138, SqlBaseParser.RULE_levelOfIsolation);
		try {
			this.state = 1964;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 245, this._ctx) ) {
			case 1:
				localctx = new ReadUncommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1957;
				this.match(SqlBaseParser.READ);
				this.state = 1958;
				this.match(SqlBaseParser.UNCOMMITTED);
				}
				break;
			case 2:
				localctx = new ReadCommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1959;
				this.match(SqlBaseParser.READ);
				this.state = 1960;
				this.match(SqlBaseParser.COMMITTED);
				}
				break;
			case 3:
				localctx = new RepeatableReadContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1961;
				this.match(SqlBaseParser.REPEATABLE);
				this.state = 1962;
				this.match(SqlBaseParser.READ);
				}
				break;
			case 4:
				localctx = new SerializableContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1963;
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
		this.enterRule(localctx, 140, SqlBaseParser.RULE_callArgument);
		try {
			this.state = 1971;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 246, this._ctx) ) {
			case 1:
				localctx = new PositionalArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1966;
				this.expression();
				}
				break;
			case 2:
				localctx = new NamedArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1967;
				this.identifier();
				this.state = 1968;
				this.match(SqlBaseParser.T__8);
				this.state = 1969;
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
		this.enterRule(localctx, 142, SqlBaseParser.RULE_privilege);
		try {
			this.state = 1977;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1973;
				this.match(SqlBaseParser.SELECT);
				}
				break;
			case 51:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1974;
				this.match(SqlBaseParser.DELETE);
				}
				break;
			case 97:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1975;
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
				this.state = 1976;
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
		this.enterRule(localctx, 144, SqlBaseParser.RULE_qualifiedName);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1979;
			this.identifier();
			this.state = 1984;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 248, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1980;
					this.match(SqlBaseParser.T__0);
					this.state = 1981;
					this.identifier();
					}
					}
				}
				this.state = 1986;
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
		this.enterRule(localctx, 146, SqlBaseParser.RULE_tableVersionExpression);
		let _la: number;
		try {
			localctx = new TableVersionContext(this, localctx);
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1987;
			this.match(SqlBaseParser.FOR);
			this.state = 1988;
			(localctx as TableVersionContext)._tableVersionType = this._input.LT(1);
			_la = this._input.LA(1);
			if(!(((((_la - 191)) & ~0x1F) === 0 && ((1 << (_la - 191)) & 536871427) !== 0))) {
			    (localctx as TableVersionContext)._tableVersionType = this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			this.state = 1989;
			this.tableVersionState();
			this.state = 1990;
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
		this.enterRule(localctx, 148, SqlBaseParser.RULE_tableVersionState);
		try {
			this.state = 1995;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 18:
				localctx = new TableversionasofContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1992;
				this.match(SqlBaseParser.AS);
				this.state = 1993;
				this.match(SqlBaseParser.OF);
				}
				break;
			case 21:
				localctx = new TableversionbeforeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1994;
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
		this.enterRule(localctx, 150, SqlBaseParser.RULE_grantor);
		try {
			this.state = 2000;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 250, this._ctx) ) {
			case 1:
				localctx = new CurrentUserGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1997;
				this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 2:
				localctx = new CurrentRoleGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1998;
				this.match(SqlBaseParser.CURRENT_ROLE);
				}
				break;
			case 3:
				localctx = new SpecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1999;
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
		this.enterRule(localctx, 152, SqlBaseParser.RULE_principal);
		try {
			this.state = 2007;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 251, this._ctx) ) {
			case 1:
				localctx = new UserPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2002;
				this.match(SqlBaseParser.USER);
				this.state = 2003;
				this.identifier();
				}
				break;
			case 2:
				localctx = new RolePrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2004;
				this.match(SqlBaseParser.ROLE);
				this.state = 2005;
				this.identifier();
				}
				break;
			case 3:
				localctx = new UnspecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2006;
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
		this.enterRule(localctx, 154, SqlBaseParser.RULE_roles);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2009;
			this.identifier();
			this.state = 2014;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 2010;
				this.match(SqlBaseParser.T__3);
				this.state = 2011;
				this.identifier();
				}
				}
				this.state = 2016;
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
		this.enterRule(localctx, 156, SqlBaseParser.RULE_identifier);
		try {
			this.state = 2022;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 247:
				localctx = new UnquotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2017;
				this.match(SqlBaseParser.IDENTIFIER);
				}
				break;
			case 249:
				localctx = new QuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2018;
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
				this.state = 2019;
				this.nonReserved();
				}
				break;
			case 250:
				localctx = new BackQuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 2020;
				this.match(SqlBaseParser.BACKQUOTED_IDENTIFIER);
				}
				break;
			case 248:
				localctx = new DigitIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 2021;
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
		this.enterRule(localctx, 158, SqlBaseParser.RULE_number);
		try {
			this.state = 2027;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 245:
				localctx = new DecimalLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2024;
				this.match(SqlBaseParser.DECIMAL_VALUE);
				}
				break;
			case 246:
				localctx = new DoubleLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2025;
				this.match(SqlBaseParser.DOUBLE_VALUE);
				}
				break;
			case 244:
				localctx = new IntegerLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2026;
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
		this.enterRule(localctx, 160, SqlBaseParser.RULE_constraintSpecification);
		try {
			this.state = 2031;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 36:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2029;
				this.namedConstraintSpecification();
				}
				break;
			case 151:
			case 211:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2030;
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
		this.enterRule(localctx, 162, SqlBaseParser.RULE_namedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2033;
			this.match(SqlBaseParser.CONSTRAINT);
			this.state = 2034;
			localctx._name = this.identifier();
			this.state = 2035;
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
		this.enterRule(localctx, 164, SqlBaseParser.RULE_unnamedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2037;
			this.constraintType();
			this.state = 2038;
			this.columnAliases();
			this.state = 2040;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 256, this._ctx) ) {
			case 1:
				{
				this.state = 2039;
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
		this.enterRule(localctx, 166, SqlBaseParser.RULE_constraintType);
		try {
			this.state = 2045;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 211:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2042;
				this.match(SqlBaseParser.UNIQUE);
				}
				break;
			case 151:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2043;
				this.match(SqlBaseParser.PRIMARY);
				this.state = 2044;
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
		this.enterRule(localctx, 168, SqlBaseParser.RULE_constraintQualifiers);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2050;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (((((_la - 55)) & ~0x1F) === 0 && ((1 << (_la - 55)) & 161) !== 0) || _la===131 || _la===158) {
				{
				{
				this.state = 2047;
				this.constraintQualifier();
				}
				}
				this.state = 2052;
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
		this.enterRule(localctx, 170, SqlBaseParser.RULE_constraintQualifier);
		try {
			this.state = 2056;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 259, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2053;
				this.constraintEnabled();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2054;
				this.constraintRely();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2055;
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
		this.enterRule(localctx, 172, SqlBaseParser.RULE_constraintRely);
		try {
			this.state = 2061;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 158:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2058;
				this.match(SqlBaseParser.RELY);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2059;
				this.match(SqlBaseParser.NOT);
				this.state = 2060;
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
		this.enterRule(localctx, 174, SqlBaseParser.RULE_constraintEnabled);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2063;
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
		this.enterRule(localctx, 176, SqlBaseParser.RULE_constraintEnforced);
		try {
			this.state = 2068;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 62:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2065;
				this.match(SqlBaseParser.ENFORCED);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2066;
				this.match(SqlBaseParser.NOT);
				this.state = 2067;
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
		this.enterRule(localctx, 178, SqlBaseParser.RULE_nonReserved);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2070;
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
		case 25:
			return this.queryTerm_sempred(localctx as QueryTermContext, predIndex);
		case 35:
			return this.relation_sempred(localctx as RelationContext, predIndex);
		case 44:
			return this.booleanExpression_sempred(localctx as BooleanExpressionContext, predIndex);
		case 46:
			return this.valueExpression_sempred(localctx as ValueExpressionContext, predIndex);
		case 47:
			return this.primaryExpression_sempred(localctx as PrimaryExpressionContext, predIndex);
		case 58:
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

	public static readonly _serializedATN: number[] = [4,1,258,2073,2,0,7,0,
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
	89,1,0,1,0,1,0,1,1,1,1,1,1,1,2,1,2,1,2,1,3,1,3,1,3,1,4,1,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,206,8,4,1,4,1,4,1,4,3,4,211,8,4,1,4,
	1,4,1,4,1,4,3,4,217,8,4,1,4,1,4,3,4,221,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,3,4,235,8,4,1,4,1,4,3,4,239,8,4,1,4,1,4,3,4,243,8,4,
	1,4,1,4,3,4,247,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,255,8,4,1,4,1,4,3,4,259,
	8,4,1,4,3,4,262,8,4,1,4,1,4,1,4,1,4,1,4,3,4,269,8,4,1,4,1,4,1,4,1,4,1,4,
	5,4,276,8,4,10,4,12,4,279,9,4,1,4,1,4,1,4,3,4,284,8,4,1,4,1,4,3,4,288,8,
	4,1,4,1,4,1,4,1,4,3,4,294,8,4,1,4,1,4,1,4,1,4,1,4,3,4,301,8,4,1,4,1,4,1,
	4,1,4,1,4,1,4,1,4,3,4,310,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,319,8,4,1,
	4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,330,8,4,1,4,1,4,1,4,1,4,1,4,3,4,337,
	8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,347,8,4,1,4,1,4,1,4,1,4,1,4,3,4,
	354,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,362,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,
	370,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,378,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	1,4,3,4,388,8,4,1,4,1,4,1,4,1,4,1,4,3,4,395,8,4,1,4,1,4,1,4,1,4,1,4,1,4,
	3,4,403,8,4,1,4,1,4,1,4,3,4,408,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	3,4,419,8,4,1,4,1,4,1,4,3,4,424,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	3,4,435,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,446,8,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,5,4,456,8,4,10,4,12,4,459,9,4,1,4,1,4,1,4,3,4,464,8,
	4,1,4,1,4,1,4,3,4,469,8,4,1,4,1,4,1,4,1,4,3,4,475,8,4,1,4,1,4,1,4,1,4,1,
	4,1,4,1,4,3,4,484,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,495,8,4,1,
	4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,504,8,4,1,4,1,4,1,4,3,4,509,8,4,1,4,1,4,3,
	4,513,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,521,8,4,1,4,1,4,1,4,1,4,1,4,3,4,528,
	8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,541,8,4,1,4,3,4,544,
	8,4,1,4,1,4,1,4,1,4,1,4,1,4,5,4,552,8,4,10,4,12,4,555,9,4,3,4,557,8,4,1,
	4,1,4,1,4,1,4,1,4,3,4,564,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,573,8,4,1,
	4,1,4,1,4,1,4,3,4,579,8,4,1,4,1,4,1,4,3,4,584,8,4,1,4,1,4,3,4,588,8,4,1,
	4,1,4,1,4,1,4,1,4,1,4,5,4,596,8,4,10,4,12,4,599,9,4,3,4,601,8,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,1,4,3,4,611,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	5,4,622,8,4,10,4,12,4,625,9,4,1,4,1,4,1,4,3,4,630,8,4,1,4,1,4,1,4,3,4,635,
	8,4,1,4,1,4,1,4,1,4,3,4,641,8,4,1,4,1,4,1,4,1,4,1,4,5,4,648,8,4,10,4,12,
	4,651,9,4,1,4,1,4,1,4,3,4,656,8,4,1,4,1,4,1,4,1,4,1,4,3,4,663,8,4,1,4,1,
	4,1,4,1,4,5,4,669,8,4,10,4,12,4,672,9,4,1,4,1,4,3,4,676,8,4,1,4,1,4,3,4,
	680,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,688,8,4,1,4,1,4,1,4,1,4,3,4,694,8,4,
	1,4,1,4,1,4,5,4,699,8,4,10,4,12,4,702,9,4,1,4,1,4,3,4,706,8,4,1,4,1,4,3,
	4,710,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,720,8,4,1,4,3,4,723,8,4,1,
	4,1,4,3,4,727,8,4,1,4,3,4,730,8,4,1,4,1,4,1,4,1,4,5,4,736,8,4,10,4,12,4,
	739,9,4,1,4,1,4,3,4,743,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,768,8,4,1,4,1,4,1,4,
	1,4,3,4,774,8,4,1,4,1,4,1,4,1,4,3,4,780,8,4,3,4,782,8,4,1,4,1,4,1,4,1,4,
	3,4,788,8,4,1,4,1,4,1,4,1,4,3,4,794,8,4,3,4,796,8,4,1,4,1,4,1,4,1,4,1,4,
	1,4,3,4,804,8,4,3,4,806,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,1,4,3,4,825,8,4,1,4,1,4,1,4,3,4,830,8,4,1,4,1,4,1,4,
	1,4,1,4,3,4,837,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,849,8,4,
	3,4,851,8,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,859,8,4,3,4,861,8,4,1,4,1,4,1,4,
	1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,5,4,877,8,4,10,4,12,4,880,9,
	4,3,4,882,8,4,1,4,1,4,3,4,886,8,4,1,4,1,4,3,4,890,8,4,1,4,1,4,1,4,1,4,1,
	4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,5,4,906,8,4,10,4,12,4,909,9,4,3,4,
	911,8,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,5,4,925,8,4,10,
	4,12,4,928,9,4,1,4,1,4,3,4,932,8,4,3,4,934,8,4,1,5,3,5,937,8,5,1,5,1,5,
	1,6,1,6,3,6,943,8,6,1,6,1,6,1,6,5,6,948,8,6,10,6,12,6,951,9,6,1,7,1,7,1,
	7,3,7,956,8,7,1,8,1,8,1,8,1,8,3,8,962,8,8,1,8,1,8,3,8,966,8,8,1,8,1,8,3,
	8,970,8,8,1,9,1,9,1,9,1,9,3,9,976,8,9,1,10,1,10,1,10,1,10,5,10,982,8,10,
	10,10,12,10,985,9,10,1,10,1,10,1,11,1,11,1,11,1,11,1,12,1,12,1,12,1,13,
	5,13,997,8,13,10,13,12,13,1000,9,13,1,14,1,14,1,14,1,14,3,14,1006,8,14,
	1,15,5,15,1009,8,15,10,15,12,15,1012,9,15,1,16,1,16,1,17,1,17,3,17,1018,
	8,17,1,18,1,18,1,18,1,19,1,19,1,19,3,19,1026,8,19,1,20,1,20,3,20,1030,8,
	20,1,21,1,21,1,21,3,21,1035,8,21,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,
	1,22,3,22,1046,8,22,1,23,1,23,1,24,1,24,1,24,1,24,1,24,1,24,5,24,1056,8,
	24,10,24,12,24,1059,9,24,3,24,1061,8,24,1,24,1,24,1,24,3,24,1066,8,24,3,
	24,1068,8,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,3,24,1077,8,24,3,24,1079,
	8,24,1,25,1,25,1,25,1,25,1,25,1,25,3,25,1087,8,25,1,25,1,25,1,25,1,25,3,
	25,1093,8,25,1,25,5,25,1096,8,25,10,25,12,25,1099,9,25,1,26,1,26,1,26,1,
	26,1,26,1,26,1,26,5,26,1108,8,26,10,26,12,26,1111,9,26,1,26,1,26,1,26,1,
	26,3,26,1117,8,26,1,27,1,27,3,27,1121,8,27,1,27,1,27,3,27,1125,8,27,1,28,
	1,28,3,28,1129,8,28,1,28,1,28,1,28,5,28,1134,8,28,10,28,12,28,1137,9,28,
	1,28,1,28,1,28,1,28,5,28,1143,8,28,10,28,12,28,1146,9,28,3,28,1148,8,28,
	1,28,1,28,3,28,1152,8,28,1,28,1,28,1,28,3,28,1157,8,28,1,28,1,28,3,28,1161,
	8,28,1,29,3,29,1164,8,29,1,29,1,29,1,29,5,29,1169,8,29,10,29,12,29,1172,
	9,29,1,30,1,30,1,30,1,30,1,30,1,30,5,30,1180,8,30,10,30,12,30,1183,9,30,
	3,30,1185,8,30,1,30,1,30,1,30,1,30,1,30,1,30,5,30,1193,8,30,10,30,12,30,
	1196,9,30,3,30,1198,8,30,1,30,1,30,1,30,1,30,1,30,1,30,1,30,5,30,1207,8,
	30,10,30,12,30,1210,9,30,1,30,1,30,3,30,1214,8,30,1,31,1,31,1,31,1,31,5,
	31,1220,8,31,10,31,12,31,1223,9,31,3,31,1225,8,31,1,31,1,31,3,31,1229,8,
	31,1,32,1,32,3,32,1233,8,32,1,32,1,32,1,32,1,32,1,32,1,33,1,33,1,34,1,34,
	3,34,1244,8,34,1,34,3,34,1247,8,34,1,34,1,34,1,34,1,34,1,34,3,34,1254,8,
	34,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,
	1,35,1,35,1,35,3,35,1273,8,35,5,35,1275,8,35,10,35,12,35,1278,9,35,1,36,
	3,36,1281,8,36,1,36,1,36,3,36,1285,8,36,1,36,1,36,3,36,1289,8,36,1,36,1,
	36,3,36,1293,8,36,3,36,1295,8,36,1,37,1,37,1,37,1,37,1,37,1,37,1,37,5,37,
	1304,8,37,10,37,12,37,1307,9,37,1,37,1,37,3,37,1311,8,37,1,38,1,38,1,38,
	1,38,1,38,1,38,1,38,3,38,1320,8,38,1,39,1,39,1,40,1,40,3,40,1326,8,40,1,
	40,1,40,3,40,1330,8,40,3,40,1332,8,40,1,41,1,41,1,41,1,41,5,41,1338,8,41,
	10,41,12,41,1341,9,41,1,41,1,41,1,42,1,42,3,42,1347,8,42,1,42,1,42,1,42,
	1,42,1,42,1,42,1,42,1,42,1,42,5,42,1358,8,42,10,42,12,42,1361,9,42,1,42,
	1,42,1,42,3,42,1366,8,42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,1,42,3,
	42,1377,8,42,1,43,1,43,1,44,1,44,1,44,3,44,1384,8,44,1,44,1,44,3,44,1388,
	8,44,1,44,1,44,1,44,1,44,1,44,1,44,5,44,1396,8,44,10,44,12,44,1399,9,44,
	1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,3,45,1411,8,45,1,45,1,
	45,1,45,1,45,1,45,1,45,3,45,1419,8,45,1,45,1,45,1,45,1,45,1,45,5,45,1426,
	8,45,10,45,12,45,1429,9,45,1,45,1,45,1,45,3,45,1434,8,45,1,45,1,45,1,45,
	1,45,1,45,1,45,3,45,1442,8,45,1,45,1,45,1,45,1,45,3,45,1448,8,45,1,45,1,
	45,3,45,1452,8,45,1,45,1,45,1,45,3,45,1457,8,45,1,45,1,45,1,45,3,45,1462,
	8,45,1,46,1,46,1,46,1,46,3,46,1468,8,46,1,46,1,46,1,46,1,46,1,46,1,46,1,
	46,1,46,1,46,1,46,1,46,1,46,5,46,1482,8,46,10,46,12,46,1485,9,46,1,47,1,
	47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,
	1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,4,47,1511,8,47,11,47,12,47,1512,
	1,47,1,47,1,47,1,47,1,47,1,47,1,47,5,47,1522,8,47,10,47,12,47,1525,9,47,
	1,47,1,47,1,47,1,47,1,47,1,47,1,47,3,47,1534,8,47,1,47,3,47,1537,8,47,1,
	47,1,47,1,47,3,47,1542,8,47,1,47,1,47,1,47,5,47,1547,8,47,10,47,12,47,1550,
	9,47,3,47,1552,8,47,1,47,1,47,1,47,1,47,1,47,5,47,1559,8,47,10,47,12,47,
	1562,9,47,3,47,1564,8,47,1,47,1,47,3,47,1568,8,47,1,47,3,47,1571,8,47,1,
	47,3,47,1574,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,5,47,1584,8,47,
	10,47,12,47,1587,9,47,3,47,1589,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,
	1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,4,47,1606,8,47,11,47,12,47,1607,
	1,47,1,47,3,47,1612,8,47,1,47,1,47,1,47,1,47,4,47,1618,8,47,11,47,12,47,
	1619,1,47,1,47,3,47,1624,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,
	47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,5,47,1647,
	8,47,10,47,12,47,1650,9,47,3,47,1652,8,47,1,47,1,47,1,47,1,47,1,47,1,47,
	1,47,3,47,1661,8,47,1,47,1,47,1,47,1,47,3,47,1667,8,47,1,47,1,47,1,47,1,
	47,3,47,1673,8,47,1,47,1,47,1,47,1,47,3,47,1679,8,47,1,47,1,47,1,47,1,47,
	1,47,1,47,1,47,1,47,3,47,1689,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,3,
	47,1698,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,
	1,47,1,47,1,47,1,47,1,47,1,47,5,47,1718,8,47,10,47,12,47,1721,9,47,3,47,
	1723,8,47,1,47,3,47,1726,8,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,1,47,5,
	47,1736,8,47,10,47,12,47,1739,9,47,1,48,1,48,1,48,1,48,3,48,1745,8,48,3,
	48,1747,8,48,1,49,1,49,1,49,1,49,3,49,1753,8,49,1,50,1,50,1,50,1,50,1,50,
	1,50,3,50,1761,8,50,1,51,1,51,1,52,1,52,1,53,1,53,1,54,1,54,3,54,1771,8,
	54,1,54,1,54,1,54,1,54,3,54,1777,8,54,1,55,1,55,1,56,1,56,1,57,1,57,1,57,
	1,57,5,57,1787,8,57,10,57,12,57,1790,9,57,3,57,1792,8,57,1,57,1,57,1,58,
	1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,
	58,1,58,1,58,1,58,1,58,1,58,5,58,1817,8,58,10,58,12,58,1820,9,58,1,58,1,
	58,1,58,1,58,1,58,1,58,1,58,5,58,1829,8,58,10,58,12,58,1832,9,58,1,58,1,
	58,3,58,1836,8,58,1,58,1,58,1,58,1,58,1,58,3,58,1843,8,58,1,58,1,58,5,58,
	1847,8,58,10,58,12,58,1850,9,58,1,59,1,59,3,59,1854,8,59,1,60,1,60,1,60,
	1,60,3,60,1860,8,60,1,61,1,61,1,61,1,61,1,61,1,62,1,62,1,62,1,62,1,62,1,
	62,1,63,1,63,1,63,1,63,1,63,1,63,1,63,5,63,1880,8,63,10,63,12,63,1883,9,
	63,3,63,1885,8,63,1,63,1,63,1,63,1,63,1,63,5,63,1892,8,63,10,63,12,63,1895,
	9,63,3,63,1897,8,63,1,63,3,63,1900,8,63,1,63,1,63,1,64,1,64,1,64,1,64,1,
	64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,
	1,64,1,64,1,64,1,64,1,64,3,64,1928,8,64,1,65,1,65,1,65,1,65,1,65,1,65,1,
	65,1,65,1,65,3,65,1939,8,65,1,66,1,66,1,66,1,66,1,67,1,67,1,67,1,67,3,67,
	1949,8,67,1,68,1,68,1,68,1,68,1,68,3,68,1956,8,68,1,69,1,69,1,69,1,69,1,
	69,1,69,1,69,3,69,1965,8,69,1,70,1,70,1,70,1,70,1,70,3,70,1972,8,70,1,71,
	1,71,1,71,1,71,3,71,1978,8,71,1,72,1,72,1,72,5,72,1983,8,72,10,72,12,72,
	1986,9,72,1,73,1,73,1,73,1,73,1,73,1,74,1,74,1,74,3,74,1996,8,74,1,75,1,
	75,1,75,3,75,2001,8,75,1,76,1,76,1,76,1,76,1,76,3,76,2008,8,76,1,77,1,77,
	1,77,5,77,2013,8,77,10,77,12,77,2016,9,77,1,78,1,78,1,78,1,78,1,78,3,78,
	2023,8,78,1,79,1,79,1,79,3,79,2028,8,79,1,80,1,80,3,80,2032,8,80,1,81,1,
	81,1,81,1,81,1,82,1,82,1,82,3,82,2041,8,82,1,83,1,83,1,83,3,83,2046,8,83,
	1,84,5,84,2049,8,84,10,84,12,84,2052,9,84,1,85,1,85,1,85,3,85,2057,8,85,
	1,86,1,86,1,86,3,86,2062,8,86,1,87,1,87,1,88,1,88,1,88,3,88,2069,8,88,1,
	89,1,89,1,89,0,6,50,70,88,92,94,116,90,0,2,4,6,8,10,12,14,16,18,20,22,24,
	26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,
	74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,
	116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,
	152,154,156,158,160,162,164,166,168,170,172,174,176,178,0,25,2,0,27,27,
	164,164,2,0,50,50,101,101,2,0,78,78,93,93,2,0,65,65,94,94,1,0,173,174,2,
	0,12,12,244,244,2,0,64,64,210,210,2,0,19,19,52,52,2,0,74,74,109,109,2,0,
	12,12,56,56,2,0,22,22,190,190,1,0,235,236,1,0,237,239,1,0,229,234,3,0,12,
	12,16,16,185,185,2,0,71,71,203,203,5,0,48,48,90,90,120,121,177,177,227,
	227,1,0,124,127,2,0,75,75,149,149,3,0,85,85,105,105,197,197,4,0,57,57,102,
	102,117,117,217,217,2,0,138,138,226,226,3,0,191,192,200,200,220,220,2,0,
	55,55,60,60,51,0,10,12,14,14,16,17,19,22,25,27,30,35,40,40,42,42,46,48,
	50,50,52,52,54,55,57,57,60,60,62,62,65,65,68,68,70,70,72,75,77,77,80,85,
	88,88,90,92,94,94,96,96,99,99,101,102,104,105,107,110,112,112,114,114,117,
	122,124,129,133,136,138,139,142,142,144,149,151,155,157,167,169,171,173,
	178,180,192,194,197,199,202,204,206,208,209,211,211,213,215,217,217,219,
	221,225,228,2389,0,180,1,0,0,0,2,183,1,0,0,0,4,186,1,0,0,0,6,189,1,0,0,
	0,8,933,1,0,0,0,10,936,1,0,0,0,12,940,1,0,0,0,14,955,1,0,0,0,16,957,1,0,
	0,0,18,971,1,0,0,0,20,977,1,0,0,0,22,988,1,0,0,0,24,992,1,0,0,0,26,998,
	1,0,0,0,28,1005,1,0,0,0,30,1010,1,0,0,0,32,1013,1,0,0,0,34,1017,1,0,0,0,
	36,1019,1,0,0,0,38,1022,1,0,0,0,40,1029,1,0,0,0,42,1034,1,0,0,0,44,1045,
	1,0,0,0,46,1047,1,0,0,0,48,1049,1,0,0,0,50,1080,1,0,0,0,52,1116,1,0,0,0,
	54,1118,1,0,0,0,56,1126,1,0,0,0,58,1163,1,0,0,0,60,1213,1,0,0,0,62,1228,
	1,0,0,0,64,1230,1,0,0,0,66,1239,1,0,0,0,68,1253,1,0,0,0,70,1255,1,0,0,0,
	72,1294,1,0,0,0,74,1310,1,0,0,0,76,1312,1,0,0,0,78,1321,1,0,0,0,80,1323,
	1,0,0,0,82,1333,1,0,0,0,84,1376,1,0,0,0,86,1378,1,0,0,0,88,1387,1,0,0,0,
	90,1461,1,0,0,0,92,1467,1,0,0,0,94,1725,1,0,0,0,96,1746,1,0,0,0,98,1752,
	1,0,0,0,100,1760,1,0,0,0,102,1762,1,0,0,0,104,1764,1,0,0,0,106,1766,1,0,
	0,0,108,1768,1,0,0,0,110,1778,1,0,0,0,112,1780,1,0,0,0,114,1782,1,0,0,0,
	116,1842,1,0,0,0,118,1853,1,0,0,0,120,1859,1,0,0,0,122,1861,1,0,0,0,124,
	1866,1,0,0,0,126,1872,1,0,0,0,128,1927,1,0,0,0,130,1938,1,0,0,0,132,1940,
	1,0,0,0,134,1948,1,0,0,0,136,1955,1,0,0,0,138,1964,1,0,0,0,140,1971,1,0,
	0,0,142,1977,1,0,0,0,144,1979,1,0,0,0,146,1987,1,0,0,0,148,1995,1,0,0,0,
	150,2000,1,0,0,0,152,2007,1,0,0,0,154,2009,1,0,0,0,156,2022,1,0,0,0,158,
	2027,1,0,0,0,160,2031,1,0,0,0,162,2033,1,0,0,0,164,2037,1,0,0,0,166,2045,
	1,0,0,0,168,2050,1,0,0,0,170,2056,1,0,0,0,172,2061,1,0,0,0,174,2063,1,0,
	0,0,176,2068,1,0,0,0,178,2070,1,0,0,0,180,181,3,8,4,0,181,182,5,0,0,1,182,
	1,1,0,0,0,183,184,3,86,43,0,184,185,5,0,0,1,185,3,1,0,0,0,186,187,3,34,
	17,0,187,188,5,0,0,1,188,5,1,0,0,0,189,190,3,88,44,0,190,191,5,0,0,1,191,
	7,1,0,0,0,192,934,3,10,5,0,193,194,5,214,0,0,194,934,3,156,78,0,195,196,
	5,214,0,0,196,197,3,156,78,0,197,198,5,1,0,0,198,199,3,156,78,0,199,934,
	1,0,0,0,200,201,5,37,0,0,201,205,5,175,0,0,202,203,5,91,0,0,203,204,5,131,
	0,0,204,206,5,67,0,0,205,202,1,0,0,0,205,206,1,0,0,0,206,207,1,0,0,0,207,
	210,3,144,72,0,208,209,5,224,0,0,209,211,3,20,10,0,210,208,1,0,0,0,210,
	211,1,0,0,0,211,934,1,0,0,0,212,213,5,58,0,0,213,216,5,175,0,0,214,215,
	5,91,0,0,215,217,5,67,0,0,216,214,1,0,0,0,216,217,1,0,0,0,217,218,1,0,0,
	0,218,220,3,144,72,0,219,221,7,0,0,0,220,219,1,0,0,0,220,221,1,0,0,0,221,
	934,1,0,0,0,222,223,5,13,0,0,223,224,5,175,0,0,224,225,3,144,72,0,225,226,
	5,159,0,0,226,227,5,201,0,0,227,228,3,156,78,0,228,934,1,0,0,0,229,230,
	5,37,0,0,230,234,5,193,0,0,231,232,5,91,0,0,232,233,5,131,0,0,233,235,5,
	67,0,0,234,231,1,0,0,0,234,235,1,0,0,0,235,236,1,0,0,0,236,238,3,144,72,
	0,237,239,3,82,41,0,238,237,1,0,0,0,238,239,1,0,0,0,239,242,1,0,0,0,240,
	241,5,33,0,0,241,243,3,96,48,0,242,240,1,0,0,0,242,243,1,0,0,0,243,246,
	1,0,0,0,244,245,5,224,0,0,245,247,3,20,10,0,246,244,1,0,0,0,246,247,1,0,
	0,0,247,248,1,0,0,0,248,254,5,18,0,0,249,255,3,10,5,0,250,251,5,2,0,0,251,
	252,3,10,5,0,252,253,5,3,0,0,253,255,1,0,0,0,254,249,1,0,0,0,254,250,1,
	0,0,0,255,261,1,0,0,0,256,258,5,224,0,0,257,259,5,128,0,0,258,257,1,0,0,
	0,258,259,1,0,0,0,259,260,1,0,0,0,260,262,5,46,0,0,261,256,1,0,0,0,261,
	262,1,0,0,0,262,934,1,0,0,0,263,264,5,37,0,0,264,268,5,193,0,0,265,266,
	5,91,0,0,266,267,5,131,0,0,267,269,5,67,0,0,268,265,1,0,0,0,268,269,1,0,
	0,0,269,270,1,0,0,0,270,271,3,144,72,0,271,272,5,2,0,0,272,277,3,14,7,0,
	273,274,5,4,0,0,274,276,3,14,7,0,275,273,1,0,0,0,276,279,1,0,0,0,277,275,
	1,0,0,0,277,278,1,0,0,0,278,280,1,0,0,0,279,277,1,0,0,0,280,283,5,3,0,0,
	281,282,5,33,0,0,282,284,3,96,48,0,283,281,1,0,0,0,283,284,1,0,0,0,284,
	287,1,0,0,0,285,286,5,224,0,0,286,288,3,20,10,0,287,285,1,0,0,0,287,288,
	1,0,0,0,288,934,1,0,0,0,289,290,5,58,0,0,290,293,5,193,0,0,291,292,5,91,
	0,0,292,294,5,67,0,0,293,291,1,0,0,0,293,294,1,0,0,0,294,295,1,0,0,0,295,
	934,3,144,72,0,296,297,5,97,0,0,297,298,5,100,0,0,298,300,3,144,72,0,299,
	301,3,82,41,0,300,299,1,0,0,0,300,301,1,0,0,0,301,302,1,0,0,0,302,303,3,
	10,5,0,303,934,1,0,0,0,304,305,5,51,0,0,305,306,5,78,0,0,306,309,3,144,
	72,0,307,308,5,223,0,0,308,310,3,88,44,0,309,307,1,0,0,0,309,310,1,0,0,
	0,310,934,1,0,0,0,311,312,5,204,0,0,312,313,5,193,0,0,313,934,3,144,72,
	0,314,315,5,13,0,0,315,318,5,193,0,0,316,317,5,91,0,0,317,319,5,67,0,0,
	318,316,1,0,0,0,318,319,1,0,0,0,319,320,1,0,0,0,320,321,3,144,72,0,321,
	322,5,159,0,0,322,323,5,201,0,0,323,324,3,144,72,0,324,934,1,0,0,0,325,
	326,5,13,0,0,326,329,5,193,0,0,327,328,5,91,0,0,328,330,5,67,0,0,329,327,
	1,0,0,0,329,330,1,0,0,0,330,331,1,0,0,0,331,332,3,144,72,0,332,333,5,159,
	0,0,333,336,5,31,0,0,334,335,5,91,0,0,335,337,5,67,0,0,336,334,1,0,0,0,
	336,337,1,0,0,0,337,338,1,0,0,0,338,339,3,156,78,0,339,340,5,201,0,0,340,
	341,3,156,78,0,341,934,1,0,0,0,342,343,5,13,0,0,343,346,5,193,0,0,344,345,
	5,91,0,0,345,347,5,67,0,0,346,344,1,0,0,0,346,347,1,0,0,0,347,348,1,0,0,
	0,348,349,3,144,72,0,349,350,5,58,0,0,350,353,5,31,0,0,351,352,5,91,0,0,
	352,354,5,67,0,0,353,351,1,0,0,0,353,354,1,0,0,0,354,355,1,0,0,0,355,356,
	3,144,72,0,356,934,1,0,0,0,357,358,5,13,0,0,358,361,5,193,0,0,359,360,5,
	91,0,0,360,362,5,67,0,0,361,359,1,0,0,0,361,362,1,0,0,0,362,363,1,0,0,0,
	363,364,3,144,72,0,364,365,5,10,0,0,365,369,5,31,0,0,366,367,5,91,0,0,367,
	368,5,131,0,0,368,370,5,67,0,0,369,366,1,0,0,0,369,370,1,0,0,0,370,371,
	1,0,0,0,371,372,3,16,8,0,372,934,1,0,0,0,373,374,5,13,0,0,374,377,5,193,
	0,0,375,376,5,91,0,0,376,378,5,67,0,0,377,375,1,0,0,0,377,378,1,0,0,0,378,
	379,1,0,0,0,379,380,3,144,72,0,380,381,5,10,0,0,381,382,3,160,80,0,382,
	934,1,0,0,0,383,384,5,13,0,0,384,387,5,193,0,0,385,386,5,91,0,0,386,388,
	5,67,0,0,387,385,1,0,0,0,387,388,1,0,0,0,388,389,1,0,0,0,389,390,3,144,
	72,0,390,391,5,58,0,0,391,394,5,36,0,0,392,393,5,91,0,0,393,395,5,67,0,
	0,394,392,1,0,0,0,394,395,1,0,0,0,395,396,1,0,0,0,396,397,3,156,78,0,397,
	934,1,0,0,0,398,399,5,13,0,0,399,402,5,193,0,0,400,401,5,91,0,0,401,403,
	5,67,0,0,402,400,1,0,0,0,402,403,1,0,0,0,403,404,1,0,0,0,404,405,3,144,
	72,0,405,407,5,13,0,0,406,408,5,31,0,0,407,406,1,0,0,0,407,408,1,0,0,0,
	408,409,1,0,0,0,409,410,3,156,78,0,410,411,5,182,0,0,411,412,5,131,0,0,
	412,413,5,132,0,0,413,934,1,0,0,0,414,415,5,13,0,0,415,418,5,193,0,0,416,
	417,5,91,0,0,417,419,5,67,0,0,418,416,1,0,0,0,418,419,1,0,0,0,419,420,1,
	0,0,0,420,421,3,144,72,0,421,423,5,13,0,0,422,424,5,31,0,0,423,422,1,0,
	0,0,423,424,1,0,0,0,424,425,1,0,0,0,425,426,3,156,78,0,426,427,5,58,0,0,
	427,428,5,131,0,0,428,429,5,132,0,0,429,934,1,0,0,0,430,431,5,13,0,0,431,
	434,5,193,0,0,432,433,5,91,0,0,433,435,5,67,0,0,434,432,1,0,0,0,434,435,
	1,0,0,0,435,436,1,0,0,0,436,437,3,144,72,0,437,438,5,182,0,0,438,439,5,
	153,0,0,439,440,3,20,10,0,440,934,1,0,0,0,441,442,5,14,0,0,442,445,3,144,
	72,0,443,444,5,224,0,0,444,446,3,20,10,0,445,443,1,0,0,0,445,446,1,0,0,
	0,446,934,1,0,0,0,447,448,5,37,0,0,448,449,5,206,0,0,449,450,3,144,72,0,
	450,463,5,18,0,0,451,452,5,2,0,0,452,457,3,24,12,0,453,454,5,4,0,0,454,
	456,3,24,12,0,455,453,1,0,0,0,456,459,1,0,0,0,457,455,1,0,0,0,457,458,1,
	0,0,0,458,460,1,0,0,0,459,457,1,0,0,0,460,461,5,3,0,0,461,464,1,0,0,0,462,
	464,3,116,58,0,463,451,1,0,0,0,463,462,1,0,0,0,464,934,1,0,0,0,465,468,
	5,37,0,0,466,467,5,140,0,0,467,469,5,161,0,0,468,466,1,0,0,0,468,469,1,
	0,0,0,469,470,1,0,0,0,470,471,5,221,0,0,471,474,3,144,72,0,472,473,5,178,
	0,0,473,475,7,1,0,0,474,472,1,0,0,0,474,475,1,0,0,0,475,476,1,0,0,0,476,
	477,5,18,0,0,477,478,3,10,5,0,478,934,1,0,0,0,479,480,5,13,0,0,480,483,
	5,221,0,0,481,482,5,91,0,0,482,484,5,67,0,0,483,481,1,0,0,0,483,484,1,0,
	0,0,484,485,1,0,0,0,485,486,3,144,72,0,486,487,5,159,0,0,487,488,5,201,
	0,0,488,489,3,144,72,0,489,934,1,0,0,0,490,491,5,58,0,0,491,494,5,221,0,
	0,492,493,5,91,0,0,493,495,5,67,0,0,494,492,1,0,0,0,494,495,1,0,0,0,495,
	496,1,0,0,0,496,934,3,144,72,0,497,498,5,37,0,0,498,499,5,119,0,0,499,503,
	5,221,0,0,500,501,5,91,0,0,501,502,5,131,0,0,502,504,5,67,0,0,503,500,1,
	0,0,0,503,504,1,0,0,0,504,505,1,0,0,0,505,508,3,144,72,0,506,507,5,33,0,
	0,507,509,3,96,48,0,508,506,1,0,0,0,508,509,1,0,0,0,509,512,1,0,0,0,510,
	511,5,224,0,0,511,513,3,20,10,0,512,510,1,0,0,0,512,513,1,0,0,0,513,514,
	1,0,0,0,514,520,5,18,0,0,515,521,3,10,5,0,516,517,5,2,0,0,517,518,3,10,
	5,0,518,519,5,3,0,0,519,521,1,0,0,0,520,515,1,0,0,0,520,516,1,0,0,0,521,
	934,1,0,0,0,522,523,5,58,0,0,523,524,5,119,0,0,524,527,5,221,0,0,525,526,
	5,91,0,0,526,528,5,67,0,0,527,525,1,0,0,0,527,528,1,0,0,0,528,529,1,0,0,
	0,529,934,3,144,72,0,530,531,5,157,0,0,531,532,5,119,0,0,532,533,5,221,
	0,0,533,534,3,144,72,0,534,535,5,223,0,0,535,536,3,88,44,0,536,934,1,0,
	0,0,537,540,5,37,0,0,538,539,5,140,0,0,539,541,5,161,0,0,540,538,1,0,0,
	0,540,541,1,0,0,0,541,543,1,0,0,0,542,544,5,196,0,0,543,542,1,0,0,0,543,
	544,1,0,0,0,544,545,1,0,0,0,545,546,5,80,0,0,546,547,3,144,72,0,547,556,
	5,2,0,0,548,553,3,24,12,0,549,550,5,4,0,0,550,552,3,24,12,0,551,549,1,0,
	0,0,552,555,1,0,0,0,553,551,1,0,0,0,553,554,1,0,0,0,554,557,1,0,0,0,555,
	553,1,0,0,0,556,548,1,0,0,0,556,557,1,0,0,0,557,558,1,0,0,0,558,559,5,3,
	0,0,559,560,5,166,0,0,560,563,3,116,58,0,561,562,5,33,0,0,562,564,3,96,
	48,0,563,561,1,0,0,0,563,564,1,0,0,0,564,565,1,0,0,0,565,566,3,26,13,0,
	566,567,3,34,17,0,567,934,1,0,0,0,568,569,5,13,0,0,569,570,5,80,0,0,570,
	572,3,144,72,0,571,573,3,114,57,0,572,571,1,0,0,0,572,573,1,0,0,0,573,574,
	1,0,0,0,574,575,3,30,15,0,575,934,1,0,0,0,576,578,5,58,0,0,577,579,5,196,
	0,0,578,577,1,0,0,0,578,579,1,0,0,0,579,580,1,0,0,0,580,583,5,80,0,0,581,
	582,5,91,0,0,582,584,5,67,0,0,583,581,1,0,0,0,583,584,1,0,0,0,584,585,1,
	0,0,0,585,587,3,144,72,0,586,588,3,114,57,0,587,586,1,0,0,0,587,588,1,0,
	0,0,588,934,1,0,0,0,589,590,5,25,0,0,590,591,3,144,72,0,591,600,5,2,0,0,
	592,597,3,140,70,0,593,594,5,4,0,0,594,596,3,140,70,0,595,593,1,0,0,0,596,
	599,1,0,0,0,597,595,1,0,0,0,597,598,1,0,0,0,598,601,1,0,0,0,599,597,1,0,
	0,0,600,592,1,0,0,0,600,601,1,0,0,0,601,602,1,0,0,0,602,603,5,3,0,0,603,
	934,1,0,0,0,604,605,5,37,0,0,605,606,5,169,0,0,606,610,3,156,78,0,607,608,
	5,224,0,0,608,609,5,11,0,0,609,611,3,150,75,0,610,607,1,0,0,0,610,611,1,
	0,0,0,611,934,1,0,0,0,612,613,5,58,0,0,613,614,5,169,0,0,614,934,3,156,
	78,0,615,616,5,82,0,0,616,617,3,154,77,0,617,618,5,201,0,0,618,623,3,152,
	76,0,619,620,5,4,0,0,620,622,3,152,76,0,621,619,1,0,0,0,622,625,1,0,0,0,
	623,621,1,0,0,0,623,624,1,0,0,0,624,629,1,0,0,0,625,623,1,0,0,0,626,627,
	5,224,0,0,627,628,5,11,0,0,628,630,5,139,0,0,629,626,1,0,0,0,629,630,1,
	0,0,0,630,634,1,0,0,0,631,632,5,83,0,0,632,633,5,24,0,0,633,635,3,150,75,
	0,634,631,1,0,0,0,634,635,1,0,0,0,635,934,1,0,0,0,636,640,5,167,0,0,637,
	638,5,11,0,0,638,639,5,139,0,0,639,641,5,76,0,0,640,637,1,0,0,0,640,641,
	1,0,0,0,641,642,1,0,0,0,642,643,3,154,77,0,643,644,5,78,0,0,644,649,3,152,
	76,0,645,646,5,4,0,0,646,648,3,152,76,0,647,645,1,0,0,0,648,651,1,0,0,0,
	649,647,1,0,0,0,649,650,1,0,0,0,650,655,1,0,0,0,651,649,1,0,0,0,652,653,
	5,83,0,0,653,654,5,24,0,0,654,656,3,150,75,0,655,652,1,0,0,0,655,656,1,
	0,0,0,656,934,1,0,0,0,657,658,5,182,0,0,658,662,5,169,0,0,659,663,5,12,
	0,0,660,663,5,129,0,0,661,663,3,156,78,0,662,659,1,0,0,0,662,660,1,0,0,
	0,662,661,1,0,0,0,663,934,1,0,0,0,664,675,5,82,0,0,665,670,3,142,71,0,666,
	667,5,4,0,0,667,669,3,142,71,0,668,666,1,0,0,0,669,672,1,0,0,0,670,668,
	1,0,0,0,670,671,1,0,0,0,671,676,1,0,0,0,672,670,1,0,0,0,673,674,5,12,0,
	0,674,676,5,152,0,0,675,665,1,0,0,0,675,673,1,0,0,0,676,677,1,0,0,0,677,
	679,5,137,0,0,678,680,5,193,0,0,679,678,1,0,0,0,679,680,1,0,0,0,680,681,
	1,0,0,0,681,682,3,144,72,0,682,683,5,201,0,0,683,687,3,152,76,0,684,685,
	5,224,0,0,685,686,5,82,0,0,686,688,5,139,0,0,687,684,1,0,0,0,687,688,1,
	0,0,0,688,934,1,0,0,0,689,693,5,167,0,0,690,691,5,82,0,0,691,692,5,139,
	0,0,692,694,5,76,0,0,693,690,1,0,0,0,693,694,1,0,0,0,694,705,1,0,0,0,695,
	700,3,142,71,0,696,697,5,4,0,0,697,699,3,142,71,0,698,696,1,0,0,0,699,702,
	1,0,0,0,700,698,1,0,0,0,700,701,1,0,0,0,701,706,1,0,0,0,702,700,1,0,0,0,
	703,704,5,12,0,0,704,706,5,152,0,0,705,695,1,0,0,0,705,703,1,0,0,0,706,
	707,1,0,0,0,707,709,5,137,0,0,708,710,5,193,0,0,709,708,1,0,0,0,709,710,
	1,0,0,0,710,711,1,0,0,0,711,712,3,144,72,0,712,713,5,78,0,0,713,714,3,152,
	76,0,714,934,1,0,0,0,715,716,5,184,0,0,716,722,5,84,0,0,717,719,5,137,0,
	0,718,720,5,193,0,0,719,718,1,0,0,0,719,720,1,0,0,0,720,721,1,0,0,0,721,
	723,3,144,72,0,722,717,1,0,0,0,722,723,1,0,0,0,723,934,1,0,0,0,724,726,
	5,68,0,0,725,727,5,14,0,0,726,725,1,0,0,0,726,727,1,0,0,0,727,729,1,0,0,
	0,728,730,5,219,0,0,729,728,1,0,0,0,729,730,1,0,0,0,730,742,1,0,0,0,731,
	732,5,2,0,0,732,737,3,134,67,0,733,734,5,4,0,0,734,736,3,134,67,0,735,733,
	1,0,0,0,736,739,1,0,0,0,737,735,1,0,0,0,737,738,1,0,0,0,738,740,1,0,0,0,
	739,737,1,0,0,0,740,741,5,3,0,0,741,743,1,0,0,0,742,731,1,0,0,0,742,743,
	1,0,0,0,743,744,1,0,0,0,744,934,3,8,4,0,745,746,5,184,0,0,746,747,5,37,
	0,0,747,748,5,193,0,0,748,934,3,144,72,0,749,750,5,184,0,0,750,751,5,37,
	0,0,751,752,5,175,0,0,752,934,3,144,72,0,753,754,5,184,0,0,754,755,5,37,
	0,0,755,756,5,221,0,0,756,934,3,144,72,0,757,758,5,184,0,0,758,759,5,37,
	0,0,759,760,5,119,0,0,760,761,5,221,0,0,761,934,3,144,72,0,762,763,5,184,
	0,0,763,764,5,37,0,0,764,765,5,80,0,0,765,767,3,144,72,0,766,768,3,114,
	57,0,767,766,1,0,0,0,767,768,1,0,0,0,768,934,1,0,0,0,769,770,5,184,0,0,
	770,773,5,194,0,0,771,772,7,2,0,0,772,774,3,144,72,0,773,771,1,0,0,0,773,
	774,1,0,0,0,774,781,1,0,0,0,775,776,5,113,0,0,776,779,3,96,48,0,777,778,
	5,63,0,0,778,780,3,96,48,0,779,777,1,0,0,0,779,780,1,0,0,0,780,782,1,0,
	0,0,781,775,1,0,0,0,781,782,1,0,0,0,782,934,1,0,0,0,783,784,5,184,0,0,784,
	787,5,176,0,0,785,786,7,2,0,0,786,788,3,156,78,0,787,785,1,0,0,0,787,788,
	1,0,0,0,788,795,1,0,0,0,789,790,5,113,0,0,790,793,3,96,48,0,791,792,5,63,
	0,0,792,794,3,96,48,0,793,791,1,0,0,0,793,794,1,0,0,0,794,796,1,0,0,0,795,
	789,1,0,0,0,795,796,1,0,0,0,796,934,1,0,0,0,797,798,5,184,0,0,798,805,5,
	30,0,0,799,800,5,113,0,0,800,803,3,96,48,0,801,802,5,63,0,0,802,804,3,96,
	48,0,803,801,1,0,0,0,803,804,1,0,0,0,804,806,1,0,0,0,805,799,1,0,0,0,805,
	806,1,0,0,0,806,934,1,0,0,0,807,808,5,184,0,0,808,809,5,32,0,0,809,810,
	7,2,0,0,810,934,3,144,72,0,811,812,5,184,0,0,812,813,5,188,0,0,813,814,
	5,76,0,0,814,934,3,144,72,0,815,816,5,184,0,0,816,817,5,188,0,0,817,818,
	5,76,0,0,818,819,5,2,0,0,819,820,3,56,28,0,820,821,5,3,0,0,821,934,1,0,
	0,0,822,824,5,184,0,0,823,825,5,40,0,0,824,823,1,0,0,0,824,825,1,0,0,0,
	825,826,1,0,0,0,826,829,5,170,0,0,827,828,7,2,0,0,828,830,3,156,78,0,829,
	827,1,0,0,0,829,830,1,0,0,0,830,934,1,0,0,0,831,832,5,184,0,0,832,833,5,
	169,0,0,833,836,5,84,0,0,834,835,7,2,0,0,835,837,3,156,78,0,836,834,1,0,
	0,0,836,837,1,0,0,0,837,934,1,0,0,0,838,839,5,53,0,0,839,934,3,144,72,0,
	840,841,5,52,0,0,841,934,3,144,72,0,842,843,5,184,0,0,843,850,5,81,0,0,
	844,845,5,113,0,0,845,848,3,96,48,0,846,847,5,63,0,0,847,849,3,96,48,0,
	848,846,1,0,0,0,848,849,1,0,0,0,849,851,1,0,0,0,850,844,1,0,0,0,850,851,
	1,0,0,0,851,934,1,0,0,0,852,853,5,184,0,0,853,860,5,181,0,0,854,855,5,113,
	0,0,855,858,3,96,48,0,856,857,5,63,0,0,857,859,3,96,48,0,858,856,1,0,0,
	0,858,859,1,0,0,0,859,861,1,0,0,0,860,854,1,0,0,0,860,861,1,0,0,0,861,934,
	1,0,0,0,862,863,5,182,0,0,863,864,5,181,0,0,864,865,3,144,72,0,865,866,
	5,229,0,0,866,867,3,86,43,0,867,934,1,0,0,0,868,869,5,162,0,0,869,870,5,
	181,0,0,870,934,3,144,72,0,871,872,5,187,0,0,872,881,5,202,0,0,873,878,
	3,136,68,0,874,875,5,4,0,0,875,877,3,136,68,0,876,874,1,0,0,0,877,880,1,
	0,0,0,878,876,1,0,0,0,878,879,1,0,0,0,879,882,1,0,0,0,880,878,1,0,0,0,881,
	873,1,0,0,0,881,882,1,0,0,0,882,934,1,0,0,0,883,885,5,34,0,0,884,886,5,
	225,0,0,885,884,1,0,0,0,885,886,1,0,0,0,886,934,1,0,0,0,887,889,5,171,0,
	0,888,890,5,225,0,0,889,888,1,0,0,0,889,890,1,0,0,0,890,934,1,0,0,0,891,
	892,5,150,0,0,892,893,3,156,78,0,893,894,5,78,0,0,894,895,3,8,4,0,895,934,
	1,0,0,0,896,897,5,49,0,0,897,898,5,150,0,0,898,934,3,156,78,0,899,900,5,
	66,0,0,900,910,3,156,78,0,901,902,5,216,0,0,902,907,3,86,43,0,903,904,5,
	4,0,0,904,906,3,86,43,0,905,903,1,0,0,0,906,909,1,0,0,0,907,905,1,0,0,0,
	907,908,1,0,0,0,908,911,1,0,0,0,909,907,1,0,0,0,910,901,1,0,0,0,910,911,
	1,0,0,0,911,934,1,0,0,0,912,913,5,53,0,0,913,914,5,96,0,0,914,934,3,156,
	78,0,915,916,5,53,0,0,916,917,5,144,0,0,917,934,3,156,78,0,918,919,5,213,
	0,0,919,920,3,144,72,0,920,921,5,182,0,0,921,926,3,132,66,0,922,923,5,4,
	0,0,923,925,3,132,66,0,924,922,1,0,0,0,925,928,1,0,0,0,926,924,1,0,0,0,
	926,927,1,0,0,0,927,931,1,0,0,0,928,926,1,0,0,0,929,930,5,223,0,0,930,932,
	3,88,44,0,931,929,1,0,0,0,931,932,1,0,0,0,932,934,1,0,0,0,933,192,1,0,0,
	0,933,193,1,0,0,0,933,195,1,0,0,0,933,200,1,0,0,0,933,212,1,0,0,0,933,222,
	1,0,0,0,933,229,1,0,0,0,933,263,1,0,0,0,933,289,1,0,0,0,933,296,1,0,0,0,
	933,304,1,0,0,0,933,311,1,0,0,0,933,314,1,0,0,0,933,325,1,0,0,0,933,342,
	1,0,0,0,933,357,1,0,0,0,933,373,1,0,0,0,933,383,1,0,0,0,933,398,1,0,0,0,
	933,414,1,0,0,0,933,430,1,0,0,0,933,441,1,0,0,0,933,447,1,0,0,0,933,465,
	1,0,0,0,933,479,1,0,0,0,933,490,1,0,0,0,933,497,1,0,0,0,933,522,1,0,0,0,
	933,530,1,0,0,0,933,537,1,0,0,0,933,568,1,0,0,0,933,576,1,0,0,0,933,589,
	1,0,0,0,933,604,1,0,0,0,933,612,1,0,0,0,933,615,1,0,0,0,933,636,1,0,0,0,
	933,657,1,0,0,0,933,664,1,0,0,0,933,689,1,0,0,0,933,715,1,0,0,0,933,724,
	1,0,0,0,933,745,1,0,0,0,933,749,1,0,0,0,933,753,1,0,0,0,933,757,1,0,0,0,
	933,762,1,0,0,0,933,769,1,0,0,0,933,783,1,0,0,0,933,797,1,0,0,0,933,807,
	1,0,0,0,933,811,1,0,0,0,933,815,1,0,0,0,933,822,1,0,0,0,933,831,1,0,0,0,
	933,838,1,0,0,0,933,840,1,0,0,0,933,842,1,0,0,0,933,852,1,0,0,0,933,862,
	1,0,0,0,933,868,1,0,0,0,933,871,1,0,0,0,933,883,1,0,0,0,933,887,1,0,0,0,
	933,891,1,0,0,0,933,896,1,0,0,0,933,899,1,0,0,0,933,912,1,0,0,0,933,915,
	1,0,0,0,933,918,1,0,0,0,934,9,1,0,0,0,935,937,3,12,6,0,936,935,1,0,0,0,
	936,937,1,0,0,0,937,938,1,0,0,0,938,939,3,48,24,0,939,11,1,0,0,0,940,942,
	5,224,0,0,941,943,5,156,0,0,942,941,1,0,0,0,942,943,1,0,0,0,943,944,1,0,
	0,0,944,949,3,64,32,0,945,946,5,4,0,0,946,948,3,64,32,0,947,945,1,0,0,0,
	948,951,1,0,0,0,949,947,1,0,0,0,949,950,1,0,0,0,950,13,1,0,0,0,951,949,
	1,0,0,0,952,956,3,160,80,0,953,956,3,16,8,0,954,956,3,18,9,0,955,952,1,
	0,0,0,955,953,1,0,0,0,955,954,1,0,0,0,956,15,1,0,0,0,957,958,3,156,78,0,
	958,961,3,116,58,0,959,960,5,131,0,0,960,962,5,132,0,0,961,959,1,0,0,0,
	961,962,1,0,0,0,962,965,1,0,0,0,963,964,5,33,0,0,964,966,3,96,48,0,965,
	963,1,0,0,0,965,966,1,0,0,0,966,969,1,0,0,0,967,968,5,224,0,0,968,970,3,
	20,10,0,969,967,1,0,0,0,969,970,1,0,0,0,970,17,1,0,0,0,971,972,5,113,0,
	0,972,975,3,144,72,0,973,974,7,3,0,0,974,976,5,153,0,0,975,973,1,0,0,0,
	975,976,1,0,0,0,976,19,1,0,0,0,977,978,5,2,0,0,978,983,3,22,11,0,979,980,
	5,4,0,0,980,982,3,22,11,0,981,979,1,0,0,0,982,985,1,0,0,0,983,981,1,0,0,
	0,983,984,1,0,0,0,984,986,1,0,0,0,985,983,1,0,0,0,986,987,5,3,0,0,987,21,
	1,0,0,0,988,989,3,156,78,0,989,990,5,229,0,0,990,991,3,86,43,0,991,23,1,
	0,0,0,992,993,3,156,78,0,993,994,3,116,58,0,994,25,1,0,0,0,995,997,3,28,
	14,0,996,995,1,0,0,0,997,1000,1,0,0,0,998,996,1,0,0,0,998,999,1,0,0,0,999,
	27,1,0,0,0,1000,998,1,0,0,0,1001,1002,5,108,0,0,1002,1006,3,40,20,0,1003,
	1006,3,42,21,0,1004,1006,3,44,22,0,1005,1001,1,0,0,0,1005,1003,1,0,0,0,
	1005,1004,1,0,0,0,1006,29,1,0,0,0,1007,1009,3,32,16,0,1008,1007,1,0,0,0,
	1009,1012,1,0,0,0,1010,1008,1,0,0,0,1010,1011,1,0,0,0,1011,31,1,0,0,0,1012,
	1010,1,0,0,0,1013,1014,3,44,22,0,1014,33,1,0,0,0,1015,1018,3,36,18,0,1016,
	1018,3,38,19,0,1017,1015,1,0,0,0,1017,1016,1,0,0,0,1018,35,1,0,0,0,1019,
	1020,5,165,0,0,1020,1021,3,86,43,0,1021,37,1,0,0,0,1022,1025,5,70,0,0,1023,
	1024,5,122,0,0,1024,1026,3,46,23,0,1025,1023,1,0,0,0,1025,1026,1,0,0,0,
	1026,39,1,0,0,0,1027,1030,5,186,0,0,1028,1030,3,156,78,0,1029,1027,1,0,
	0,0,1029,1028,1,0,0,0,1030,41,1,0,0,0,1031,1035,5,54,0,0,1032,1033,5,131,
	0,0,1033,1035,5,54,0,0,1034,1031,1,0,0,0,1034,1032,1,0,0,0,1035,43,1,0,
	0,0,1036,1037,5,166,0,0,1037,1038,5,132,0,0,1038,1039,5,137,0,0,1039,1040,
	5,132,0,0,1040,1046,5,96,0,0,1041,1042,5,26,0,0,1042,1043,5,137,0,0,1043,
	1044,5,132,0,0,1044,1046,5,96,0,0,1045,1036,1,0,0,0,1045,1041,1,0,0,0,1046,
	45,1,0,0,0,1047,1048,3,156,78,0,1048,47,1,0,0,0,1049,1060,3,50,25,0,1050,
	1051,5,141,0,0,1051,1052,5,24,0,0,1052,1057,3,54,27,0,1053,1054,5,4,0,0,
	1054,1056,3,54,27,0,1055,1053,1,0,0,0,1056,1059,1,0,0,0,1057,1055,1,0,0,
	0,1057,1058,1,0,0,0,1058,1061,1,0,0,0,1059,1057,1,0,0,0,1060,1050,1,0,0,
	0,1060,1061,1,0,0,0,1061,1067,1,0,0,0,1062,1063,5,136,0,0,1063,1065,5,244,
	0,0,1064,1066,7,4,0,0,1065,1064,1,0,0,0,1065,1066,1,0,0,0,1066,1068,1,0,
	0,0,1067,1062,1,0,0,0,1067,1068,1,0,0,0,1068,1078,1,0,0,0,1069,1070,5,114,
	0,0,1070,1077,7,5,0,0,1071,1072,5,72,0,0,1072,1073,5,74,0,0,1073,1074,5,
	244,0,0,1074,1075,5,174,0,0,1075,1077,5,138,0,0,1076,1069,1,0,0,0,1076,
	1071,1,0,0,0,1077,1079,1,0,0,0,1078,1076,1,0,0,0,1078,1079,1,0,0,0,1079,
	49,1,0,0,0,1080,1081,6,25,-1,0,1081,1082,3,52,26,0,1082,1097,1,0,0,0,1083,
	1084,10,2,0,0,1084,1086,5,98,0,0,1085,1087,3,66,33,0,1086,1085,1,0,0,0,
	1086,1087,1,0,0,0,1087,1088,1,0,0,0,1088,1096,3,50,25,3,1089,1090,10,1,
	0,0,1090,1092,7,6,0,0,1091,1093,3,66,33,0,1092,1091,1,0,0,0,1092,1093,1,
	0,0,0,1093,1094,1,0,0,0,1094,1096,3,50,25,2,1095,1083,1,0,0,0,1095,1089,
	1,0,0,0,1096,1099,1,0,0,0,1097,1095,1,0,0,0,1097,1098,1,0,0,0,1098,51,1,
	0,0,0,1099,1097,1,0,0,0,1100,1117,3,56,28,0,1101,1102,5,193,0,0,1102,1117,
	3,144,72,0,1103,1104,5,218,0,0,1104,1109,3,86,43,0,1105,1106,5,4,0,0,1106,
	1108,3,86,43,0,1107,1105,1,0,0,0,1108,1111,1,0,0,0,1109,1107,1,0,0,0,1109,
	1110,1,0,0,0,1110,1117,1,0,0,0,1111,1109,1,0,0,0,1112,1113,5,2,0,0,1113,
	1114,3,48,24,0,1114,1115,5,3,0,0,1115,1117,1,0,0,0,1116,1100,1,0,0,0,1116,
	1101,1,0,0,0,1116,1103,1,0,0,0,1116,1112,1,0,0,0,1117,53,1,0,0,0,1118,1120,
	3,86,43,0,1119,1121,7,7,0,0,1120,1119,1,0,0,0,1120,1121,1,0,0,0,1121,1124,
	1,0,0,0,1122,1123,5,134,0,0,1123,1125,7,8,0,0,1124,1122,1,0,0,0,1124,1125,
	1,0,0,0,1125,55,1,0,0,0,1126,1128,5,179,0,0,1127,1129,3,66,33,0,1128,1127,
	1,0,0,0,1128,1129,1,0,0,0,1129,1130,1,0,0,0,1130,1135,3,68,34,0,1131,1132,
	5,4,0,0,1132,1134,3,68,34,0,1133,1131,1,0,0,0,1134,1137,1,0,0,0,1135,1133,
	1,0,0,0,1135,1136,1,0,0,0,1136,1147,1,0,0,0,1137,1135,1,0,0,0,1138,1139,
	5,78,0,0,1139,1144,3,70,35,0,1140,1141,5,4,0,0,1141,1143,3,70,35,0,1142,
	1140,1,0,0,0,1143,1146,1,0,0,0,1144,1142,1,0,0,0,1144,1145,1,0,0,0,1145,
	1148,1,0,0,0,1146,1144,1,0,0,0,1147,1138,1,0,0,0,1147,1148,1,0,0,0,1148,
	1151,1,0,0,0,1149,1150,5,223,0,0,1150,1152,3,88,44,0,1151,1149,1,0,0,0,
	1151,1152,1,0,0,0,1152,1156,1,0,0,0,1153,1154,5,86,0,0,1154,1155,5,24,0,
	0,1155,1157,3,58,29,0,1156,1153,1,0,0,0,1156,1157,1,0,0,0,1157,1160,1,0,
	0,0,1158,1159,5,89,0,0,1159,1161,3,88,44,0,1160,1158,1,0,0,0,1160,1161,
	1,0,0,0,1161,57,1,0,0,0,1162,1164,3,66,33,0,1163,1162,1,0,0,0,1163,1164,
	1,0,0,0,1164,1165,1,0,0,0,1165,1170,3,60,30,0,1166,1167,5,4,0,0,1167,1169,
	3,60,30,0,1168,1166,1,0,0,0,1169,1172,1,0,0,0,1170,1168,1,0,0,0,1170,1171,
	1,0,0,0,1171,59,1,0,0,0,1172,1170,1,0,0,0,1173,1214,3,62,31,0,1174,1175,
	5,172,0,0,1175,1184,5,2,0,0,1176,1181,3,86,43,0,1177,1178,5,4,0,0,1178,
	1180,3,86,43,0,1179,1177,1,0,0,0,1180,1183,1,0,0,0,1181,1179,1,0,0,0,1181,
	1182,1,0,0,0,1182,1185,1,0,0,0,1183,1181,1,0,0,0,1184,1176,1,0,0,0,1184,
	1185,1,0,0,0,1185,1186,1,0,0,0,1186,1214,5,3,0,0,1187,1188,5,39,0,0,1188,
	1197,5,2,0,0,1189,1194,3,86,43,0,1190,1191,5,4,0,0,1191,1193,3,86,43,0,
	1192,1190,1,0,0,0,1193,1196,1,0,0,0,1194,1192,1,0,0,0,1194,1195,1,0,0,0,
	1195,1198,1,0,0,0,1196,1194,1,0,0,0,1197,1189,1,0,0,0,1197,1198,1,0,0,0,
	1198,1199,1,0,0,0,1199,1214,5,3,0,0,1200,1201,5,87,0,0,1201,1202,5,183,
	0,0,1202,1203,5,2,0,0,1203,1208,3,62,31,0,1204,1205,5,4,0,0,1205,1207,3,
	62,31,0,1206,1204,1,0,0,0,1207,1210,1,0,0,0,1208,1206,1,0,0,0,1208,1209,
	1,0,0,0,1209,1211,1,0,0,0,1210,1208,1,0,0,0,1211,1212,5,3,0,0,1212,1214,
	1,0,0,0,1213,1173,1,0,0,0,1213,1174,1,0,0,0,1213,1187,1,0,0,0,1213,1200,
	1,0,0,0,1214,61,1,0,0,0,1215,1224,5,2,0,0,1216,1221,3,86,43,0,1217,1218,
	5,4,0,0,1218,1220,3,86,43,0,1219,1217,1,0,0,0,1220,1223,1,0,0,0,1221,1219,
	1,0,0,0,1221,1222,1,0,0,0,1222,1225,1,0,0,0,1223,1221,1,0,0,0,1224,1216,
	1,0,0,0,1224,1225,1,0,0,0,1225,1226,1,0,0,0,1226,1229,5,3,0,0,1227,1229,
	3,86,43,0,1228,1215,1,0,0,0,1228,1227,1,0,0,0,1229,63,1,0,0,0,1230,1232,
	3,156,78,0,1231,1233,3,82,41,0,1232,1231,1,0,0,0,1232,1233,1,0,0,0,1233,
	1234,1,0,0,0,1234,1235,5,18,0,0,1235,1236,5,2,0,0,1236,1237,3,10,5,0,1237,
	1238,5,3,0,0,1238,65,1,0,0,0,1239,1240,7,9,0,0,1240,67,1,0,0,0,1241,1246,
	3,86,43,0,1242,1244,5,18,0,0,1243,1242,1,0,0,0,1243,1244,1,0,0,0,1244,1245,
	1,0,0,0,1245,1247,3,156,78,0,1246,1243,1,0,0,0,1246,1247,1,0,0,0,1247,1254,
	1,0,0,0,1248,1249,3,144,72,0,1249,1250,5,1,0,0,1250,1251,5,237,0,0,1251,
	1254,1,0,0,0,1252,1254,5,237,0,0,1253,1241,1,0,0,0,1253,1248,1,0,0,0,1253,
	1252,1,0,0,0,1254,69,1,0,0,0,1255,1256,6,35,-1,0,1256,1257,3,76,38,0,1257,
	1276,1,0,0,0,1258,1272,10,2,0,0,1259,1260,5,38,0,0,1260,1261,5,106,0,0,
	1261,1273,3,76,38,0,1262,1263,3,72,36,0,1263,1264,5,106,0,0,1264,1265,3,
	70,35,0,1265,1266,3,74,37,0,1266,1273,1,0,0,0,1267,1268,5,123,0,0,1268,
	1269,3,72,36,0,1269,1270,5,106,0,0,1270,1271,3,76,38,0,1271,1273,1,0,0,
	0,1272,1259,1,0,0,0,1272,1262,1,0,0,0,1272,1267,1,0,0,0,1273,1275,1,0,0,
	0,1274,1258,1,0,0,0,1275,1278,1,0,0,0,1276,1274,1,0,0,0,1276,1277,1,0,0,
	0,1277,71,1,0,0,0,1278,1276,1,0,0,0,1279,1281,5,95,0,0,1280,1279,1,0,0,
	0,1280,1281,1,0,0,0,1281,1295,1,0,0,0,1282,1284,5,111,0,0,1283,1285,5,143,
	0,0,1284,1283,1,0,0,0,1284,1285,1,0,0,0,1285,1295,1,0,0,0,1286,1288,5,168,
	0,0,1287,1289,5,143,0,0,1288,1287,1,0,0,0,1288,1289,1,0,0,0,1289,1295,1,
	0,0,0,1290,1292,5,79,0,0,1291,1293,5,143,0,0,1292,1291,1,0,0,0,1292,1293,
	1,0,0,0,1293,1295,1,0,0,0,1294,1280,1,0,0,0,1294,1282,1,0,0,0,1294,1286,
	1,0,0,0,1294,1290,1,0,0,0,1295,73,1,0,0,0,1296,1297,5,137,0,0,1297,1311,
	3,88,44,0,1298,1299,5,216,0,0,1299,1300,5,2,0,0,1300,1305,3,156,78,0,1301,
	1302,5,4,0,0,1302,1304,3,156,78,0,1303,1301,1,0,0,0,1304,1307,1,0,0,0,1305,
	1303,1,0,0,0,1305,1306,1,0,0,0,1306,1308,1,0,0,0,1307,1305,1,0,0,0,1308,
	1309,5,3,0,0,1309,1311,1,0,0,0,1310,1296,1,0,0,0,1310,1298,1,0,0,0,1311,
	75,1,0,0,0,1312,1319,3,80,40,0,1313,1314,5,195,0,0,1314,1315,3,78,39,0,
	1315,1316,5,2,0,0,1316,1317,3,86,43,0,1317,1318,5,3,0,0,1318,1320,1,0,0,
	0,1319,1313,1,0,0,0,1319,1320,1,0,0,0,1320,77,1,0,0,0,1321,1322,7,10,0,
	0,1322,79,1,0,0,0,1323,1331,3,84,42,0,1324,1326,5,18,0,0,1325,1324,1,0,
	0,0,1325,1326,1,0,0,0,1326,1327,1,0,0,0,1327,1329,3,156,78,0,1328,1330,
	3,82,41,0,1329,1328,1,0,0,0,1329,1330,1,0,0,0,1330,1332,1,0,0,0,1331,1325,
	1,0,0,0,1331,1332,1,0,0,0,1332,81,1,0,0,0,1333,1334,5,2,0,0,1334,1339,3,
	156,78,0,1335,1336,5,4,0,0,1336,1338,3,156,78,0,1337,1335,1,0,0,0,1338,
	1341,1,0,0,0,1339,1337,1,0,0,0,1339,1340,1,0,0,0,1340,1342,1,0,0,0,1341,
	1339,1,0,0,0,1342,1343,5,3,0,0,1343,83,1,0,0,0,1344,1346,3,144,72,0,1345,
	1347,3,146,73,0,1346,1345,1,0,0,0,1346,1347,1,0,0,0,1347,1377,1,0,0,0,1348,
	1349,5,2,0,0,1349,1350,3,10,5,0,1350,1351,5,3,0,0,1351,1377,1,0,0,0,1352,
	1353,5,212,0,0,1353,1354,5,2,0,0,1354,1359,3,86,43,0,1355,1356,5,4,0,0,
	1356,1358,3,86,43,0,1357,1355,1,0,0,0,1358,1361,1,0,0,0,1359,1357,1,0,0,
	0,1359,1360,1,0,0,0,1360,1362,1,0,0,0,1361,1359,1,0,0,0,1362,1365,5,3,0,
	0,1363,1364,5,224,0,0,1364,1366,5,142,0,0,1365,1363,1,0,0,0,1365,1366,1,
	0,0,0,1366,1377,1,0,0,0,1367,1368,5,110,0,0,1368,1369,5,2,0,0,1369,1370,
	3,10,5,0,1370,1371,5,3,0,0,1371,1377,1,0,0,0,1372,1373,5,2,0,0,1373,1374,
	3,70,35,0,1374,1375,5,3,0,0,1375,1377,1,0,0,0,1376,1344,1,0,0,0,1376,1348,
	1,0,0,0,1376,1352,1,0,0,0,1376,1367,1,0,0,0,1376,1372,1,0,0,0,1377,85,1,
	0,0,0,1378,1379,3,88,44,0,1379,87,1,0,0,0,1380,1381,6,44,-1,0,1381,1383,
	3,92,46,0,1382,1384,3,90,45,0,1383,1382,1,0,0,0,1383,1384,1,0,0,0,1384,
	1388,1,0,0,0,1385,1386,5,131,0,0,1386,1388,3,88,44,3,1387,1380,1,0,0,0,
	1387,1385,1,0,0,0,1388,1397,1,0,0,0,1389,1390,10,2,0,0,1390,1391,5,15,0,
	0,1391,1396,3,88,44,3,1392,1393,10,1,0,0,1393,1394,5,140,0,0,1394,1396,
	3,88,44,2,1395,1389,1,0,0,0,1395,1392,1,0,0,0,1396,1399,1,0,0,0,1397,1395,
	1,0,0,0,1397,1398,1,0,0,0,1398,89,1,0,0,0,1399,1397,1,0,0,0,1400,1401,3,
	102,51,0,1401,1402,3,92,46,0,1402,1462,1,0,0,0,1403,1404,3,102,51,0,1404,
	1405,3,104,52,0,1405,1406,5,2,0,0,1406,1407,3,10,5,0,1407,1408,5,3,0,0,
	1408,1462,1,0,0,0,1409,1411,5,131,0,0,1410,1409,1,0,0,0,1410,1411,1,0,0,
	0,1411,1412,1,0,0,0,1412,1413,5,23,0,0,1413,1414,3,92,46,0,1414,1415,5,
	15,0,0,1415,1416,3,92,46,0,1416,1462,1,0,0,0,1417,1419,5,131,0,0,1418,1417,
	1,0,0,0,1418,1419,1,0,0,0,1419,1420,1,0,0,0,1420,1421,5,93,0,0,1421,1422,
	5,2,0,0,1422,1427,3,86,43,0,1423,1424,5,4,0,0,1424,1426,3,86,43,0,1425,
	1423,1,0,0,0,1426,1429,1,0,0,0,1427,1425,1,0,0,0,1427,1428,1,0,0,0,1428,
	1430,1,0,0,0,1429,1427,1,0,0,0,1430,1431,5,3,0,0,1431,1462,1,0,0,0,1432,
	1434,5,131,0,0,1433,1432,1,0,0,0,1433,1434,1,0,0,0,1434,1435,1,0,0,0,1435,
	1436,5,93,0,0,1436,1437,5,2,0,0,1437,1438,3,10,5,0,1438,1439,5,3,0,0,1439,
	1462,1,0,0,0,1440,1442,5,131,0,0,1441,1440,1,0,0,0,1441,1442,1,0,0,0,1442,
	1443,1,0,0,0,1443,1444,5,113,0,0,1444,1447,3,92,46,0,1445,1446,5,63,0,0,
	1446,1448,3,92,46,0,1447,1445,1,0,0,0,1447,1448,1,0,0,0,1448,1462,1,0,0,
	0,1449,1451,5,103,0,0,1450,1452,5,131,0,0,1451,1450,1,0,0,0,1451,1452,1,
	0,0,0,1452,1453,1,0,0,0,1453,1462,5,132,0,0,1454,1456,5,103,0,0,1455,1457,
	5,131,0,0,1456,1455,1,0,0,0,1456,1457,1,0,0,0,1457,1458,1,0,0,0,1458,1459,
	5,56,0,0,1459,1460,5,78,0,0,1460,1462,3,92,46,0,1461,1400,1,0,0,0,1461,
	1403,1,0,0,0,1461,1410,1,0,0,0,1461,1418,1,0,0,0,1461,1433,1,0,0,0,1461,
	1441,1,0,0,0,1461,1449,1,0,0,0,1461,1454,1,0,0,0,1462,91,1,0,0,0,1463,1464,
	6,46,-1,0,1464,1468,3,94,47,0,1465,1466,7,11,0,0,1466,1468,3,92,46,4,1467,
	1463,1,0,0,0,1467,1465,1,0,0,0,1468,1483,1,0,0,0,1469,1470,10,3,0,0,1470,
	1471,7,12,0,0,1471,1482,3,92,46,4,1472,1473,10,2,0,0,1473,1474,7,11,0,0,
	1474,1482,3,92,46,3,1475,1476,10,1,0,0,1476,1477,5,240,0,0,1477,1482,3,
	92,46,2,1478,1479,10,5,0,0,1479,1480,5,20,0,0,1480,1482,3,100,50,0,1481,
	1469,1,0,0,0,1481,1472,1,0,0,0,1481,1475,1,0,0,0,1481,1478,1,0,0,0,1482,
	1485,1,0,0,0,1483,1481,1,0,0,0,1483,1484,1,0,0,0,1484,93,1,0,0,0,1485,1483,
	1,0,0,0,1486,1487,6,47,-1,0,1487,1726,5,132,0,0,1488,1726,3,108,54,0,1489,
	1490,3,156,78,0,1490,1491,3,96,48,0,1491,1726,1,0,0,0,1492,1493,5,253,0,
	0,1493,1726,3,96,48,0,1494,1726,3,158,79,0,1495,1726,3,106,53,0,1496,1726,
	3,96,48,0,1497,1726,5,243,0,0,1498,1726,5,5,0,0,1499,1500,5,148,0,0,1500,
	1501,5,2,0,0,1501,1502,3,92,46,0,1502,1503,5,93,0,0,1503,1504,3,92,46,0,
	1504,1505,5,3,0,0,1505,1726,1,0,0,0,1506,1507,5,2,0,0,1507,1510,3,86,43,
	0,1508,1509,5,4,0,0,1509,1511,3,86,43,0,1510,1508,1,0,0,0,1511,1512,1,0,
	0,0,1512,1510,1,0,0,0,1512,1513,1,0,0,0,1513,1514,1,0,0,0,1514,1515,5,3,
	0,0,1515,1726,1,0,0,0,1516,1517,5,173,0,0,1517,1518,5,2,0,0,1518,1523,3,
	86,43,0,1519,1520,5,4,0,0,1520,1522,3,86,43,0,1521,1519,1,0,0,0,1522,1525,
	1,0,0,0,1523,1521,1,0,0,0,1523,1524,1,0,0,0,1524,1526,1,0,0,0,1525,1523,
	1,0,0,0,1526,1527,5,3,0,0,1527,1726,1,0,0,0,1528,1529,3,144,72,0,1529,1530,
	5,2,0,0,1530,1531,5,237,0,0,1531,1533,5,3,0,0,1532,1534,3,124,62,0,1533,
	1532,1,0,0,0,1533,1534,1,0,0,0,1534,1536,1,0,0,0,1535,1537,3,126,63,0,1536,
	1535,1,0,0,0,1536,1537,1,0,0,0,1537,1726,1,0,0,0,1538,1539,3,144,72,0,1539,
	1551,5,2,0,0,1540,1542,3,66,33,0,1541,1540,1,0,0,0,1541,1542,1,0,0,0,1542,
	1543,1,0,0,0,1543,1548,3,86,43,0,1544,1545,5,4,0,0,1545,1547,3,86,43,0,
	1546,1544,1,0,0,0,1547,1550,1,0,0,0,1548,1546,1,0,0,0,1548,1549,1,0,0,0,
	1549,1552,1,0,0,0,1550,1548,1,0,0,0,1551,1541,1,0,0,0,1551,1552,1,0,0,0,
	1552,1563,1,0,0,0,1553,1554,5,141,0,0,1554,1555,5,24,0,0,1555,1560,3,54,
	27,0,1556,1557,5,4,0,0,1557,1559,3,54,27,0,1558,1556,1,0,0,0,1559,1562,
	1,0,0,0,1560,1558,1,0,0,0,1560,1561,1,0,0,0,1561,1564,1,0,0,0,1562,1560,
	1,0,0,0,1563,1553,1,0,0,0,1563,1564,1,0,0,0,1564,1565,1,0,0,0,1565,1567,
	5,3,0,0,1566,1568,3,124,62,0,1567,1566,1,0,0,0,1567,1568,1,0,0,0,1568,1573,
	1,0,0,0,1569,1571,3,98,49,0,1570,1569,1,0,0,0,1570,1571,1,0,0,0,1571,1572,
	1,0,0,0,1572,1574,3,126,63,0,1573,1570,1,0,0,0,1573,1574,1,0,0,0,1574,1726,
	1,0,0,0,1575,1576,3,156,78,0,1576,1577,5,6,0,0,1577,1578,3,86,43,0,1578,
	1726,1,0,0,0,1579,1588,5,2,0,0,1580,1585,3,156,78,0,1581,1582,5,4,0,0,1582,
	1584,3,156,78,0,1583,1581,1,0,0,0,1584,1587,1,0,0,0,1585,1583,1,0,0,0,1585,
	1586,1,0,0,0,1586,1589,1,0,0,0,1587,1585,1,0,0,0,1588,1580,1,0,0,0,1588,
	1589,1,0,0,0,1589,1590,1,0,0,0,1590,1591,5,3,0,0,1591,1592,5,6,0,0,1592,
	1726,3,86,43,0,1593,1594,5,2,0,0,1594,1595,3,10,5,0,1595,1596,5,3,0,0,1596,
	1726,1,0,0,0,1597,1598,5,67,0,0,1598,1599,5,2,0,0,1599,1600,3,10,5,0,1600,
	1601,5,3,0,0,1601,1726,1,0,0,0,1602,1603,5,28,0,0,1603,1605,3,92,46,0,1604,
	1606,3,122,61,0,1605,1604,1,0,0,0,1606,1607,1,0,0,0,1607,1605,1,0,0,0,1607,
	1608,1,0,0,0,1608,1611,1,0,0,0,1609,1610,5,59,0,0,1610,1612,3,86,43,0,1611,
	1609,1,0,0,0,1611,1612,1,0,0,0,1612,1613,1,0,0,0,1613,1614,5,61,0,0,1614,
	1726,1,0,0,0,1615,1617,5,28,0,0,1616,1618,3,122,61,0,1617,1616,1,0,0,0,
	1618,1619,1,0,0,0,1619,1617,1,0,0,0,1619,1620,1,0,0,0,1620,1623,1,0,0,0,
	1621,1622,5,59,0,0,1622,1624,3,86,43,0,1623,1621,1,0,0,0,1623,1624,1,0,
	0,0,1624,1625,1,0,0,0,1625,1626,5,61,0,0,1626,1726,1,0,0,0,1627,1628,5,
	29,0,0,1628,1629,5,2,0,0,1629,1630,3,86,43,0,1630,1631,5,18,0,0,1631,1632,
	3,116,58,0,1632,1633,5,3,0,0,1633,1726,1,0,0,0,1634,1635,5,205,0,0,1635,
	1636,5,2,0,0,1636,1637,3,86,43,0,1637,1638,5,18,0,0,1638,1639,3,116,58,
	0,1639,1640,5,3,0,0,1640,1726,1,0,0,0,1641,1642,5,17,0,0,1642,1651,5,7,
	0,0,1643,1648,3,86,43,0,1644,1645,5,4,0,0,1645,1647,3,86,43,0,1646,1644,
	1,0,0,0,1647,1650,1,0,0,0,1648,1646,1,0,0,0,1648,1649,1,0,0,0,1649,1652,
	1,0,0,0,1650,1648,1,0,0,0,1651,1643,1,0,0,0,1651,1652,1,0,0,0,1652,1653,
	1,0,0,0,1653,1726,5,8,0,0,1654,1726,3,156,78,0,1655,1726,5,41,0,0,1656,
	1660,5,43,0,0,1657,1658,5,2,0,0,1658,1659,5,244,0,0,1659,1661,5,3,0,0,1660,
	1657,1,0,0,0,1660,1661,1,0,0,0,1661,1726,1,0,0,0,1662,1666,5,44,0,0,1663,
	1664,5,2,0,0,1664,1665,5,244,0,0,1665,1667,5,3,0,0,1666,1663,1,0,0,0,1666,
	1667,1,0,0,0,1667,1726,1,0,0,0,1668,1672,5,115,0,0,1669,1670,5,2,0,0,1670,
	1671,5,244,0,0,1671,1673,5,3,0,0,1672,1669,1,0,0,0,1672,1673,1,0,0,0,1673,
	1726,1,0,0,0,1674,1678,5,116,0,0,1675,1676,5,2,0,0,1676,1677,5,244,0,0,
	1677,1679,5,3,0,0,1678,1675,1,0,0,0,1678,1679,1,0,0,0,1679,1726,1,0,0,0,
	1680,1726,5,45,0,0,1681,1682,5,189,0,0,1682,1683,5,2,0,0,1683,1684,3,92,
	46,0,1684,1685,5,78,0,0,1685,1688,3,92,46,0,1686,1687,5,76,0,0,1687,1689,
	3,92,46,0,1688,1686,1,0,0,0,1688,1689,1,0,0,0,1689,1690,1,0,0,0,1690,1691,
	5,3,0,0,1691,1726,1,0,0,0,1692,1693,5,130,0,0,1693,1694,5,2,0,0,1694,1697,
	3,92,46,0,1695,1696,5,4,0,0,1696,1698,3,112,56,0,1697,1695,1,0,0,0,1697,
	1698,1,0,0,0,1698,1699,1,0,0,0,1699,1700,5,3,0,0,1700,1726,1,0,0,0,1701,
	1702,5,69,0,0,1702,1703,5,2,0,0,1703,1704,3,156,78,0,1704,1705,5,78,0,0,
	1705,1706,3,92,46,0,1706,1707,5,3,0,0,1707,1726,1,0,0,0,1708,1709,5,2,0,
	0,1709,1710,3,86,43,0,1710,1711,5,3,0,0,1711,1726,1,0,0,0,1712,1713,5,87,
	0,0,1713,1722,5,2,0,0,1714,1719,3,144,72,0,1715,1716,5,4,0,0,1716,1718,
	3,144,72,0,1717,1715,1,0,0,0,1718,1721,1,0,0,0,1719,1717,1,0,0,0,1719,1720,
	1,0,0,0,1720,1723,1,0,0,0,1721,1719,1,0,0,0,1722,1714,1,0,0,0,1722,1723,
	1,0,0,0,1723,1724,1,0,0,0,1724,1726,5,3,0,0,1725,1486,1,0,0,0,1725,1488,
	1,0,0,0,1725,1489,1,0,0,0,1725,1492,1,0,0,0,1725,1494,1,0,0,0,1725,1495,
	1,0,0,0,1725,1496,1,0,0,0,1725,1497,1,0,0,0,1725,1498,1,0,0,0,1725,1499,
	1,0,0,0,1725,1506,1,0,0,0,1725,1516,1,0,0,0,1725,1528,1,0,0,0,1725,1538,
	1,0,0,0,1725,1575,1,0,0,0,1725,1579,1,0,0,0,1725,1593,1,0,0,0,1725,1597,
	1,0,0,0,1725,1602,1,0,0,0,1725,1615,1,0,0,0,1725,1627,1,0,0,0,1725,1634,
	1,0,0,0,1725,1641,1,0,0,0,1725,1654,1,0,0,0,1725,1655,1,0,0,0,1725,1656,
	1,0,0,0,1725,1662,1,0,0,0,1725,1668,1,0,0,0,1725,1674,1,0,0,0,1725,1680,
	1,0,0,0,1725,1681,1,0,0,0,1725,1692,1,0,0,0,1725,1701,1,0,0,0,1725,1708,
	1,0,0,0,1725,1712,1,0,0,0,1726,1737,1,0,0,0,1727,1728,10,14,0,0,1728,1729,
	5,7,0,0,1729,1730,3,92,46,0,1730,1731,5,8,0,0,1731,1736,1,0,0,0,1732,1733,
	10,12,0,0,1733,1734,5,1,0,0,1734,1736,3,156,78,0,1735,1727,1,0,0,0,1735,
	1732,1,0,0,0,1736,1739,1,0,0,0,1737,1735,1,0,0,0,1737,1738,1,0,0,0,1738,
	95,1,0,0,0,1739,1737,1,0,0,0,1740,1747,5,241,0,0,1741,1744,5,242,0,0,1742,
	1743,5,207,0,0,1743,1745,5,241,0,0,1744,1742,1,0,0,0,1744,1745,1,0,0,0,
	1745,1747,1,0,0,0,1746,1740,1,0,0,0,1746,1741,1,0,0,0,1747,97,1,0,0,0,1748,
	1749,5,92,0,0,1749,1753,5,134,0,0,1750,1751,5,163,0,0,1751,1753,5,134,0,
	0,1752,1748,1,0,0,0,1752,1750,1,0,0,0,1753,99,1,0,0,0,1754,1755,5,199,0,
	0,1755,1756,5,228,0,0,1756,1761,3,108,54,0,1757,1758,5,199,0,0,1758,1759,
	5,228,0,0,1759,1761,3,96,48,0,1760,1754,1,0,0,0,1760,1757,1,0,0,0,1761,
	101,1,0,0,0,1762,1763,7,13,0,0,1763,103,1,0,0,0,1764,1765,7,14,0,0,1765,
	105,1,0,0,0,1766,1767,7,15,0,0,1767,107,1,0,0,0,1768,1770,5,99,0,0,1769,
	1771,7,11,0,0,1770,1769,1,0,0,0,1770,1771,1,0,0,0,1771,1772,1,0,0,0,1772,
	1773,3,96,48,0,1773,1776,3,110,55,0,1774,1775,5,201,0,0,1775,1777,3,110,
	55,0,1776,1774,1,0,0,0,1776,1777,1,0,0,0,1777,109,1,0,0,0,1778,1779,7,16,
	0,0,1779,111,1,0,0,0,1780,1781,7,17,0,0,1781,113,1,0,0,0,1782,1791,5,2,
	0,0,1783,1788,3,116,58,0,1784,1785,5,4,0,0,1785,1787,3,116,58,0,1786,1784,
	1,0,0,0,1787,1790,1,0,0,0,1788,1786,1,0,0,0,1788,1789,1,0,0,0,1789,1792,
	1,0,0,0,1790,1788,1,0,0,0,1791,1783,1,0,0,0,1791,1792,1,0,0,0,1792,1793,
	1,0,0,0,1793,1794,5,3,0,0,1794,115,1,0,0,0,1795,1796,6,58,-1,0,1796,1797,
	5,17,0,0,1797,1798,5,231,0,0,1798,1799,3,116,58,0,1799,1800,5,233,0,0,1800,
	1843,1,0,0,0,1801,1802,5,118,0,0,1802,1803,5,231,0,0,1803,1804,3,116,58,
	0,1804,1805,5,4,0,0,1805,1806,3,116,58,0,1806,1807,5,233,0,0,1807,1843,
	1,0,0,0,1808,1809,5,173,0,0,1809,1810,5,2,0,0,1810,1811,3,156,78,0,1811,
	1818,3,116,58,0,1812,1813,5,4,0,0,1813,1814,3,156,78,0,1814,1815,3,116,
	58,0,1815,1817,1,0,0,0,1816,1812,1,0,0,0,1817,1820,1,0,0,0,1818,1816,1,
	0,0,0,1818,1819,1,0,0,0,1819,1821,1,0,0,0,1820,1818,1,0,0,0,1821,1822,5,
	3,0,0,1822,1843,1,0,0,0,1823,1835,3,120,60,0,1824,1825,5,2,0,0,1825,1830,
	3,118,59,0,1826,1827,5,4,0,0,1827,1829,3,118,59,0,1828,1826,1,0,0,0,1829,
	1832,1,0,0,0,1830,1828,1,0,0,0,1830,1831,1,0,0,0,1831,1833,1,0,0,0,1832,
	1830,1,0,0,0,1833,1834,5,3,0,0,1834,1836,1,0,0,0,1835,1824,1,0,0,0,1835,
	1836,1,0,0,0,1836,1843,1,0,0,0,1837,1838,5,99,0,0,1838,1839,3,110,55,0,
	1839,1840,5,201,0,0,1840,1841,3,110,55,0,1841,1843,1,0,0,0,1842,1795,1,
	0,0,0,1842,1801,1,0,0,0,1842,1808,1,0,0,0,1842,1823,1,0,0,0,1842,1837,1,
	0,0,0,1843,1848,1,0,0,0,1844,1845,10,6,0,0,1845,1847,5,17,0,0,1846,1844,
	1,0,0,0,1847,1850,1,0,0,0,1848,1846,1,0,0,0,1848,1849,1,0,0,0,1849,117,
	1,0,0,0,1850,1848,1,0,0,0,1851,1854,5,244,0,0,1852,1854,3,116,58,0,1853,
	1851,1,0,0,0,1853,1852,1,0,0,0,1854,119,1,0,0,0,1855,1860,5,251,0,0,1856,
	1860,5,252,0,0,1857,1860,5,253,0,0,1858,1860,3,144,72,0,1859,1855,1,0,0,
	0,1859,1856,1,0,0,0,1859,1857,1,0,0,0,1859,1858,1,0,0,0,1860,121,1,0,0,
	0,1861,1862,5,222,0,0,1862,1863,3,86,43,0,1863,1864,5,198,0,0,1864,1865,
	3,86,43,0,1865,123,1,0,0,0,1866,1867,5,73,0,0,1867,1868,5,2,0,0,1868,1869,
	5,223,0,0,1869,1870,3,88,44,0,1870,1871,5,3,0,0,1871,125,1,0,0,0,1872,1873,
	5,145,0,0,1873,1884,5,2,0,0,1874,1875,5,146,0,0,1875,1876,5,24,0,0,1876,
	1881,3,86,43,0,1877,1878,5,4,0,0,1878,1880,3,86,43,0,1879,1877,1,0,0,0,
	1880,1883,1,0,0,0,1881,1879,1,0,0,0,1881,1882,1,0,0,0,1882,1885,1,0,0,0,
	1883,1881,1,0,0,0,1884,1874,1,0,0,0,1884,1885,1,0,0,0,1885,1896,1,0,0,0,
	1886,1887,5,141,0,0,1887,1888,5,24,0,0,1888,1893,3,54,27,0,1889,1890,5,
	4,0,0,1890,1892,3,54,27,0,1891,1889,1,0,0,0,1892,1895,1,0,0,0,1893,1891,
	1,0,0,0,1893,1894,1,0,0,0,1894,1897,1,0,0,0,1895,1893,1,0,0,0,1896,1886,
	1,0,0,0,1896,1897,1,0,0,0,1897,1899,1,0,0,0,1898,1900,3,128,64,0,1899,1898,
	1,0,0,0,1899,1900,1,0,0,0,1900,1901,1,0,0,0,1901,1902,5,3,0,0,1902,127,
	1,0,0,0,1903,1904,5,154,0,0,1904,1928,3,130,65,0,1905,1906,5,174,0,0,1906,
	1928,3,130,65,0,1907,1908,5,88,0,0,1908,1928,3,130,65,0,1909,1910,5,154,
	0,0,1910,1911,5,23,0,0,1911,1912,3,130,65,0,1912,1913,5,15,0,0,1913,1914,
	3,130,65,0,1914,1928,1,0,0,0,1915,1916,5,174,0,0,1916,1917,5,23,0,0,1917,
	1918,3,130,65,0,1918,1919,5,15,0,0,1919,1920,3,130,65,0,1920,1928,1,0,0,
	0,1921,1922,5,88,0,0,1922,1923,5,23,0,0,1923,1924,3,130,65,0,1924,1925,
	5,15,0,0,1925,1926,3,130,65,0,1926,1928,1,0,0,0,1927,1903,1,0,0,0,1927,
	1905,1,0,0,0,1927,1907,1,0,0,0,1927,1909,1,0,0,0,1927,1915,1,0,0,0,1927,
	1921,1,0,0,0,1928,129,1,0,0,0,1929,1930,5,208,0,0,1930,1939,5,149,0,0,1931,
	1932,5,208,0,0,1932,1939,5,75,0,0,1933,1934,5,40,0,0,1934,1939,5,173,0,
	0,1935,1936,3,86,43,0,1936,1937,7,18,0,0,1937,1939,1,0,0,0,1938,1929,1,
	0,0,0,1938,1931,1,0,0,0,1938,1933,1,0,0,0,1938,1935,1,0,0,0,1939,131,1,
	0,0,0,1940,1941,3,156,78,0,1941,1942,5,229,0,0,1942,1943,3,86,43,0,1943,
	133,1,0,0,0,1944,1945,5,77,0,0,1945,1949,7,19,0,0,1946,1947,5,206,0,0,1947,
	1949,7,20,0,0,1948,1944,1,0,0,0,1948,1946,1,0,0,0,1949,135,1,0,0,0,1950,
	1951,5,104,0,0,1951,1952,5,112,0,0,1952,1956,3,138,69,0,1953,1954,5,155,
	0,0,1954,1956,7,21,0,0,1955,1950,1,0,0,0,1955,1953,1,0,0,0,1956,137,1,0,
	0,0,1957,1958,5,155,0,0,1958,1965,5,209,0,0,1959,1960,5,155,0,0,1960,1965,
	5,35,0,0,1961,1962,5,160,0,0,1962,1965,5,155,0,0,1963,1965,5,180,0,0,1964,
	1957,1,0,0,0,1964,1959,1,0,0,0,1964,1961,1,0,0,0,1964,1963,1,0,0,0,1965,
	139,1,0,0,0,1966,1972,3,86,43,0,1967,1968,3,156,78,0,1968,1969,5,9,0,0,
	1969,1970,3,86,43,0,1970,1972,1,0,0,0,1971,1966,1,0,0,0,1971,1967,1,0,0,
	0,1972,141,1,0,0,0,1973,1978,5,179,0,0,1974,1978,5,51,0,0,1975,1978,5,97,
	0,0,1976,1978,3,156,78,0,1977,1973,1,0,0,0,1977,1974,1,0,0,0,1977,1975,
	1,0,0,0,1977,1976,1,0,0,0,1978,143,1,0,0,0,1979,1984,3,156,78,0,1980,1981,
	5,1,0,0,1981,1983,3,156,78,0,1982,1980,1,0,0,0,1983,1986,1,0,0,0,1984,1982,
	1,0,0,0,1984,1985,1,0,0,0,1985,145,1,0,0,0,1986,1984,1,0,0,0,1987,1988,
	5,76,0,0,1988,1989,7,22,0,0,1989,1990,3,148,74,0,1990,1991,3,92,46,0,1991,
	147,1,0,0,0,1992,1993,5,18,0,0,1993,1996,5,135,0,0,1994,1996,5,21,0,0,1995,
	1992,1,0,0,0,1995,1994,1,0,0,0,1996,149,1,0,0,0,1997,2001,5,45,0,0,1998,
	2001,5,42,0,0,1999,2001,3,152,76,0,2000,1997,1,0,0,0,2000,1998,1,0,0,0,
	2000,1999,1,0,0,0,2001,151,1,0,0,0,2002,2003,5,215,0,0,2003,2008,3,156,
	78,0,2004,2005,5,169,0,0,2005,2008,3,156,78,0,2006,2008,3,156,78,0,2007,
	2002,1,0,0,0,2007,2004,1,0,0,0,2007,2006,1,0,0,0,2008,153,1,0,0,0,2009,
	2014,3,156,78,0,2010,2011,5,4,0,0,2011,2013,3,156,78,0,2012,2010,1,0,0,
	0,2013,2016,1,0,0,0,2014,2012,1,0,0,0,2014,2015,1,0,0,0,2015,155,1,0,0,
	0,2016,2014,1,0,0,0,2017,2023,5,247,0,0,2018,2023,5,249,0,0,2019,2023,3,
	178,89,0,2020,2023,5,250,0,0,2021,2023,5,248,0,0,2022,2017,1,0,0,0,2022,
	2018,1,0,0,0,2022,2019,1,0,0,0,2022,2020,1,0,0,0,2022,2021,1,0,0,0,2023,
	157,1,0,0,0,2024,2028,5,245,0,0,2025,2028,5,246,0,0,2026,2028,5,244,0,0,
	2027,2024,1,0,0,0,2027,2025,1,0,0,0,2027,2026,1,0,0,0,2028,159,1,0,0,0,
	2029,2032,3,162,81,0,2030,2032,3,164,82,0,2031,2029,1,0,0,0,2031,2030,1,
	0,0,0,2032,161,1,0,0,0,2033,2034,5,36,0,0,2034,2035,3,156,78,0,2035,2036,
	3,164,82,0,2036,163,1,0,0,0,2037,2038,3,166,83,0,2038,2040,3,82,41,0,2039,
	2041,3,168,84,0,2040,2039,1,0,0,0,2040,2041,1,0,0,0,2041,165,1,0,0,0,2042,
	2046,5,211,0,0,2043,2044,5,151,0,0,2044,2046,5,107,0,0,2045,2042,1,0,0,
	0,2045,2043,1,0,0,0,2046,167,1,0,0,0,2047,2049,3,170,85,0,2048,2047,1,0,
	0,0,2049,2052,1,0,0,0,2050,2048,1,0,0,0,2050,2051,1,0,0,0,2051,169,1,0,
	0,0,2052,2050,1,0,0,0,2053,2057,3,174,87,0,2054,2057,3,172,86,0,2055,2057,
	3,176,88,0,2056,2053,1,0,0,0,2056,2054,1,0,0,0,2056,2055,1,0,0,0,2057,171,
	1,0,0,0,2058,2062,5,158,0,0,2059,2060,5,131,0,0,2060,2062,5,158,0,0,2061,
	2058,1,0,0,0,2061,2059,1,0,0,0,2062,173,1,0,0,0,2063,2064,7,23,0,0,2064,
	175,1,0,0,0,2065,2069,5,62,0,0,2066,2067,5,131,0,0,2067,2069,5,62,0,0,2068,
	2065,1,0,0,0,2068,2066,1,0,0,0,2069,177,1,0,0,0,2070,2071,7,24,0,0,2071,
	179,1,0,0,0,262,205,210,216,220,234,238,242,246,254,258,261,268,277,283,
	287,293,300,309,318,329,336,346,353,361,369,377,387,394,402,407,418,423,
	434,445,457,463,468,474,483,494,503,508,512,520,527,540,543,553,556,563,
	572,578,583,587,597,600,610,623,629,634,640,649,655,662,670,675,679,687,
	693,700,705,709,719,722,726,729,737,742,767,773,779,781,787,793,795,803,
	805,824,829,836,848,850,858,860,878,881,885,889,907,910,926,931,933,936,
	942,949,955,961,965,969,975,983,998,1005,1010,1017,1025,1029,1034,1045,
	1057,1060,1065,1067,1076,1078,1086,1092,1095,1097,1109,1116,1120,1124,1128,
	1135,1144,1147,1151,1156,1160,1163,1170,1181,1184,1194,1197,1208,1213,1221,
	1224,1228,1232,1243,1246,1253,1272,1276,1280,1284,1288,1292,1294,1305,1310,
	1319,1325,1329,1331,1339,1346,1359,1365,1376,1383,1387,1395,1397,1410,1418,
	1427,1433,1441,1447,1451,1456,1461,1467,1481,1483,1512,1523,1533,1536,1541,
	1548,1551,1560,1563,1567,1570,1573,1585,1588,1607,1611,1619,1623,1648,1651,
	1660,1666,1672,1678,1688,1697,1719,1722,1725,1735,1737,1744,1746,1752,1760,
	1770,1776,1788,1791,1818,1830,1835,1842,1848,1853,1859,1881,1884,1893,1896,
	1899,1927,1938,1948,1955,1964,1971,1977,1984,1995,2000,2007,2014,2022,2027,
	2031,2040,2045,2050,2056,2061,2068];

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
	public sortItem_list(): SortItemContext[] {
		return this.getTypedRuleContexts(SortItemContext) as SortItemContext[];
	}
	public sortItem(i: number): SortItemContext {
		return this.getTypedRuleContext(SortItemContext, i) as SortItemContext;
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
	public selectItem_list(): SelectItemContext[] {
		return this.getTypedRuleContexts(SelectItemContext) as SelectItemContext[];
	}
	public selectItem(i: number): SelectItemContext {
		return this.getTypedRuleContext(SelectItemContext, i) as SelectItemContext;
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlBaseParser.FROM, 0);
	}
	public relation_list(): RelationContext[] {
		return this.getTypedRuleContexts(RelationContext) as RelationContext[];
	}
	public relation(i: number): RelationContext {
		return this.getTypedRuleContext(RelationContext, i) as RelationContext;
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
