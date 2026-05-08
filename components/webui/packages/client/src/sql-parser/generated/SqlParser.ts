// @ts-nocheck
// Generated from Sql.g4 by ANTLR 4.13.2
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
// for running tests with parameters, TODO: discuss strategy for typed parameters in CI
// eslint-disable-next-line no-unused-vars
type int = number;

export default class SqlParser extends Parser {
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
	public static readonly RULE_selectItemList = 0;
	public static readonly RULE_standaloneSelectItemList = 1;
	public static readonly RULE_standaloneBooleanExpression = 2;
	public static readonly RULE_sortItemList = 3;
	public static readonly RULE_standaloneSortItemList = 4;
	public static readonly RULE_singleStatement = 5;
	public static readonly RULE_standaloneExpression = 6;
	public static readonly RULE_standaloneRoutineBody = 7;
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
	public static readonly RULE_queryTerm = 29;
	public static readonly RULE_queryPrimary = 30;
	public static readonly RULE_sortItem = 31;
	public static readonly RULE_querySpecification = 32;
	public static readonly RULE_groupBy = 33;
	public static readonly RULE_groupingElement = 34;
	public static readonly RULE_groupingSet = 35;
	public static readonly RULE_namedQuery = 36;
	public static readonly RULE_setQuantifier = 37;
	public static readonly RULE_selectItem = 38;
	public static readonly RULE_relation = 39;
	public static readonly RULE_joinType = 40;
	public static readonly RULE_joinCriteria = 41;
	public static readonly RULE_sampledRelation = 42;
	public static readonly RULE_sampleType = 43;
	public static readonly RULE_aliasedRelation = 44;
	public static readonly RULE_columnAliases = 45;
	public static readonly RULE_relationPrimary = 46;
	public static readonly RULE_expression = 47;
	public static readonly RULE_booleanExpression = 48;
	public static readonly RULE_predicate = 49;
	public static readonly RULE_valueExpression = 50;
	public static readonly RULE_primaryExpression = 51;
	public static readonly RULE_string = 52;
	public static readonly RULE_nullTreatment = 53;
	public static readonly RULE_timeZoneSpecifier = 54;
	public static readonly RULE_comparisonOperator = 55;
	public static readonly RULE_comparisonQuantifier = 56;
	public static readonly RULE_booleanValue = 57;
	public static readonly RULE_interval = 58;
	public static readonly RULE_intervalField = 59;
	public static readonly RULE_normalForm = 60;
	public static readonly RULE_types = 61;
	public static readonly RULE_type = 62;
	public static readonly RULE_typeParameter = 63;
	public static readonly RULE_baseType = 64;
	public static readonly RULE_whenClause = 65;
	public static readonly RULE_filter = 66;
	public static readonly RULE_over = 67;
	public static readonly RULE_windowFrame = 68;
	public static readonly RULE_frameBound = 69;
	public static readonly RULE_updateAssignment = 70;
	public static readonly RULE_explainOption = 71;
	public static readonly RULE_transactionMode = 72;
	public static readonly RULE_levelOfIsolation = 73;
	public static readonly RULE_callArgument = 74;
	public static readonly RULE_privilege = 75;
	public static readonly RULE_qualifiedName = 76;
	public static readonly RULE_tableVersionExpression = 77;
	public static readonly RULE_tableVersionState = 78;
	public static readonly RULE_grantor = 79;
	public static readonly RULE_principal = 80;
	public static readonly RULE_roles = 81;
	public static readonly RULE_identifier = 82;
	public static readonly RULE_number = 83;
	public static readonly RULE_constraintSpecification = 84;
	public static readonly RULE_namedConstraintSpecification = 85;
	public static readonly RULE_unnamedConstraintSpecification = 86;
	public static readonly RULE_constraintType = 87;
	public static readonly RULE_constraintQualifiers = 88;
	public static readonly RULE_constraintQualifier = 89;
	public static readonly RULE_constraintRely = 90;
	public static readonly RULE_constraintEnabled = 91;
	public static readonly RULE_constraintEnforced = 92;
	public static readonly RULE_nonReserved = 93;
	public static readonly literalNames: (string | null)[] = [ null, "','", 
                                                            "'.'", "'('", 
                                                            "')'", "'?'", 
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
		"selectItemList", "standaloneSelectItemList", "standaloneBooleanExpression", 
		"sortItemList", "standaloneSortItemList", "singleStatement", "standaloneExpression", 
		"standaloneRoutineBody", "statement", "query", "with", "tableElement", 
		"columnDefinition", "likeClause", "properties", "property", "sqlParameterDeclaration", 
		"routineCharacteristics", "routineCharacteristic", "alterRoutineCharacteristics", 
		"alterRoutineCharacteristic", "routineBody", "returnStatement", "externalBodyReference", 
		"language", "determinism", "nullCallClause", "externalRoutineName", "queryNoWith", 
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
	public get grammarFileName(): string { return "Sql.g4"; }
	public get literalNames(): (string | null)[] { return SqlParser.literalNames; }
	public get symbolicNames(): (string | null)[] { return SqlParser.symbolicNames; }
	public get ruleNames(): string[] { return SqlParser.ruleNames; }
	public get serializedATN(): number[] { return SqlParser._serializedATN; }

	protected createFailedPredicateException(predicate?: string, message?: string): FailedPredicateException {
		return new FailedPredicateException(this, predicate, message);
	}

	constructor(input: TokenStream) {
		super(input);
		this._interp = new ParserATNSimulator(this, SqlParser._ATN, SqlParser.DecisionsToDFA, new PredictionContextCache());
	}
	// @RuleVersion(0)
	public selectItemList(): SelectItemListContext {
		let localctx: SelectItemListContext = new SelectItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 0, SqlParser.RULE_selectItemList);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 188;
			this.selectItem();
			this.state = 193;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 189;
				this.match(SqlParser.T__0);
				this.state = 190;
				this.selectItem();
				}
				}
				this.state = 195;
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
	public standaloneSelectItemList(): StandaloneSelectItemListContext {
		let localctx: StandaloneSelectItemListContext = new StandaloneSelectItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 2, SqlParser.RULE_standaloneSelectItemList);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 196;
			this.selectItemList();
			this.state = 197;
			this.match(SqlParser.EOF);
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
		this.enterRule(localctx, 4, SqlParser.RULE_standaloneBooleanExpression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 199;
			this.booleanExpression(0);
			this.state = 200;
			this.match(SqlParser.EOF);
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
		this.enterRule(localctx, 6, SqlParser.RULE_sortItemList);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 202;
			this.sortItem();
			this.state = 207;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 203;
				this.match(SqlParser.T__0);
				this.state = 204;
				this.sortItem();
				}
				}
				this.state = 209;
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
	public standaloneSortItemList(): StandaloneSortItemListContext {
		let localctx: StandaloneSortItemListContext = new StandaloneSortItemListContext(this, this._ctx, this.state);
		this.enterRule(localctx, 8, SqlParser.RULE_standaloneSortItemList);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 210;
			this.sortItemList();
			this.state = 211;
			this.match(SqlParser.EOF);
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
	public singleStatement(): SingleStatementContext {
		let localctx: SingleStatementContext = new SingleStatementContext(this, this._ctx, this.state);
		this.enterRule(localctx, 10, SqlParser.RULE_singleStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 213;
			this.statement();
			this.state = 214;
			this.match(SqlParser.EOF);
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
		this.enterRule(localctx, 12, SqlParser.RULE_standaloneExpression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 216;
			this.expression();
			this.state = 217;
			this.match(SqlParser.EOF);
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
		this.enterRule(localctx, 14, SqlParser.RULE_standaloneRoutineBody);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 219;
			this.routineBody();
			this.state = 220;
			this.match(SqlParser.EOF);
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
		this.enterRule(localctx, 16, SqlParser.RULE_statement);
		let _la: number;
		try {
			this.state = 963;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 104, this._ctx) ) {
			case 1:
				localctx = new StatementDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 222;
				this.query();
				}
				break;
			case 2:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 223;
				this.match(SqlParser.USE);
				this.state = 224;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 3:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 225;
				this.match(SqlParser.USE);
				this.state = 226;
				(localctx as UseContext)._catalog = this.identifier();
				this.state = 227;
				this.match(SqlParser.T__1);
				this.state = 228;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 4:
				localctx = new CreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 230;
				this.match(SqlParser.CREATE);
				this.state = 231;
				this.match(SqlParser.SCHEMA);
				this.state = 235;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 2, this._ctx) ) {
				case 1:
					{
					this.state = 232;
					this.match(SqlParser.IF);
					this.state = 233;
					this.match(SqlParser.NOT);
					this.state = 234;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 237;
				this.qualifiedName();
				this.state = 240;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 238;
					this.match(SqlParser.WITH);
					this.state = 239;
					this.properties();
					}
				}

				}
				break;
			case 5:
				localctx = new DropSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 242;
				this.match(SqlParser.DROP);
				this.state = 243;
				this.match(SqlParser.SCHEMA);
				this.state = 246;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 4, this._ctx) ) {
				case 1:
					{
					this.state = 244;
					this.match(SqlParser.IF);
					this.state = 245;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 248;
				this.qualifiedName();
				this.state = 250;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===27 || _la===164) {
					{
					this.state = 249;
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
				this.state = 252;
				this.match(SqlParser.ALTER);
				this.state = 253;
				this.match(SqlParser.SCHEMA);
				this.state = 254;
				this.qualifiedName();
				this.state = 255;
				this.match(SqlParser.RENAME);
				this.state = 256;
				this.match(SqlParser.TO);
				this.state = 257;
				this.identifier();
				}
				break;
			case 7:
				localctx = new CreateTableAsSelectContext(this, localctx);
				this.enterOuterAlt(localctx, 7);
				{
				this.state = 259;
				this.match(SqlParser.CREATE);
				this.state = 260;
				this.match(SqlParser.TABLE);
				this.state = 264;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 6, this._ctx) ) {
				case 1:
					{
					this.state = 261;
					this.match(SqlParser.IF);
					this.state = 262;
					this.match(SqlParser.NOT);
					this.state = 263;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 266;
				this.qualifiedName();
				this.state = 268;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===3) {
					{
					this.state = 267;
					this.columnAliases();
					}
				}

				this.state = 272;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 270;
					this.match(SqlParser.COMMENT);
					this.state = 271;
					this.string_();
					}
				}

				this.state = 276;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 274;
					this.match(SqlParser.WITH);
					this.state = 275;
					this.properties();
					}
				}

				this.state = 278;
				this.match(SqlParser.AS);
				this.state = 284;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 10, this._ctx) ) {
				case 1:
					{
					this.state = 279;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 280;
					this.match(SqlParser.T__2);
					this.state = 281;
					this.query();
					this.state = 282;
					this.match(SqlParser.T__3);
					}
					break;
				}
				this.state = 291;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 286;
					this.match(SqlParser.WITH);
					this.state = 288;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===128) {
						{
						this.state = 287;
						this.match(SqlParser.NO);
						}
					}

					this.state = 290;
					this.match(SqlParser.DATA);
					}
				}

				}
				break;
			case 8:
				localctx = new CreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 293;
				this.match(SqlParser.CREATE);
				this.state = 294;
				this.match(SqlParser.TABLE);
				this.state = 298;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 13, this._ctx) ) {
				case 1:
					{
					this.state = 295;
					this.match(SqlParser.IF);
					this.state = 296;
					this.match(SqlParser.NOT);
					this.state = 297;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 300;
				this.qualifiedName();
				this.state = 301;
				this.match(SqlParser.T__2);
				this.state = 302;
				this.tableElement();
				this.state = 307;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 303;
					this.match(SqlParser.T__0);
					this.state = 304;
					this.tableElement();
					}
					}
					this.state = 309;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 310;
				this.match(SqlParser.T__3);
				this.state = 313;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 311;
					this.match(SqlParser.COMMENT);
					this.state = 312;
					this.string_();
					}
				}

				this.state = 317;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 315;
					this.match(SqlParser.WITH);
					this.state = 316;
					this.properties();
					}
				}

				}
				break;
			case 9:
				localctx = new DropTableContext(this, localctx);
				this.enterOuterAlt(localctx, 9);
				{
				this.state = 319;
				this.match(SqlParser.DROP);
				this.state = 320;
				this.match(SqlParser.TABLE);
				this.state = 323;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 17, this._ctx) ) {
				case 1:
					{
					this.state = 321;
					this.match(SqlParser.IF);
					this.state = 322;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 325;
				this.qualifiedName();
				}
				break;
			case 10:
				localctx = new InsertIntoContext(this, localctx);
				this.enterOuterAlt(localctx, 10);
				{
				this.state = 326;
				this.match(SqlParser.INSERT);
				this.state = 327;
				this.match(SqlParser.INTO);
				this.state = 328;
				this.qualifiedName();
				this.state = 330;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 18, this._ctx) ) {
				case 1:
					{
					this.state = 329;
					this.columnAliases();
					}
					break;
				}
				this.state = 332;
				this.query();
				}
				break;
			case 11:
				localctx = new DeleteContext(this, localctx);
				this.enterOuterAlt(localctx, 11);
				{
				this.state = 334;
				this.match(SqlParser.DELETE);
				this.state = 335;
				this.match(SqlParser.FROM);
				this.state = 336;
				this.qualifiedName();
				this.state = 339;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 337;
					this.match(SqlParser.WHERE);
					this.state = 338;
					this.booleanExpression(0);
					}
				}

				}
				break;
			case 12:
				localctx = new TruncateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 12);
				{
				this.state = 341;
				this.match(SqlParser.TRUNCATE);
				this.state = 342;
				this.match(SqlParser.TABLE);
				this.state = 343;
				this.qualifiedName();
				}
				break;
			case 13:
				localctx = new RenameTableContext(this, localctx);
				this.enterOuterAlt(localctx, 13);
				{
				this.state = 344;
				this.match(SqlParser.ALTER);
				this.state = 345;
				this.match(SqlParser.TABLE);
				this.state = 348;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 20, this._ctx) ) {
				case 1:
					{
					this.state = 346;
					this.match(SqlParser.IF);
					this.state = 347;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 350;
				(localctx as RenameTableContext)._from_ = this.qualifiedName();
				this.state = 351;
				this.match(SqlParser.RENAME);
				this.state = 352;
				this.match(SqlParser.TO);
				this.state = 353;
				(localctx as RenameTableContext)._to = this.qualifiedName();
				}
				break;
			case 14:
				localctx = new RenameColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 14);
				{
				this.state = 355;
				this.match(SqlParser.ALTER);
				this.state = 356;
				this.match(SqlParser.TABLE);
				this.state = 359;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 21, this._ctx) ) {
				case 1:
					{
					this.state = 357;
					this.match(SqlParser.IF);
					this.state = 358;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 361;
				(localctx as RenameColumnContext)._tableName = this.qualifiedName();
				this.state = 362;
				this.match(SqlParser.RENAME);
				this.state = 363;
				this.match(SqlParser.COLUMN);
				this.state = 366;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 22, this._ctx) ) {
				case 1:
					{
					this.state = 364;
					this.match(SqlParser.IF);
					this.state = 365;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 368;
				(localctx as RenameColumnContext)._from_ = this.identifier();
				this.state = 369;
				this.match(SqlParser.TO);
				this.state = 370;
				(localctx as RenameColumnContext)._to = this.identifier();
				}
				break;
			case 15:
				localctx = new DropColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 15);
				{
				this.state = 372;
				this.match(SqlParser.ALTER);
				this.state = 373;
				this.match(SqlParser.TABLE);
				this.state = 376;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 23, this._ctx) ) {
				case 1:
					{
					this.state = 374;
					this.match(SqlParser.IF);
					this.state = 375;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 378;
				(localctx as DropColumnContext)._tableName = this.qualifiedName();
				this.state = 379;
				this.match(SqlParser.DROP);
				this.state = 380;
				this.match(SqlParser.COLUMN);
				this.state = 383;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 24, this._ctx) ) {
				case 1:
					{
					this.state = 381;
					this.match(SqlParser.IF);
					this.state = 382;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 385;
				(localctx as DropColumnContext)._column = this.qualifiedName();
				}
				break;
			case 16:
				localctx = new AddColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 16);
				{
				this.state = 387;
				this.match(SqlParser.ALTER);
				this.state = 388;
				this.match(SqlParser.TABLE);
				this.state = 391;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 25, this._ctx) ) {
				case 1:
					{
					this.state = 389;
					this.match(SqlParser.IF);
					this.state = 390;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 393;
				(localctx as AddColumnContext)._tableName = this.qualifiedName();
				this.state = 394;
				this.match(SqlParser.ADD);
				this.state = 395;
				this.match(SqlParser.COLUMN);
				this.state = 399;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 26, this._ctx) ) {
				case 1:
					{
					this.state = 396;
					this.match(SqlParser.IF);
					this.state = 397;
					this.match(SqlParser.NOT);
					this.state = 398;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 401;
				(localctx as AddColumnContext)._column = this.columnDefinition();
				}
				break;
			case 17:
				localctx = new AddConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 17);
				{
				this.state = 403;
				this.match(SqlParser.ALTER);
				this.state = 404;
				this.match(SqlParser.TABLE);
				this.state = 407;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 27, this._ctx) ) {
				case 1:
					{
					this.state = 405;
					this.match(SqlParser.IF);
					this.state = 406;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 409;
				(localctx as AddConstraintContext)._tableName = this.qualifiedName();
				this.state = 410;
				this.match(SqlParser.ADD);
				this.state = 411;
				this.constraintSpecification();
				}
				break;
			case 18:
				localctx = new DropConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 18);
				{
				this.state = 413;
				this.match(SqlParser.ALTER);
				this.state = 414;
				this.match(SqlParser.TABLE);
				this.state = 417;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 28, this._ctx) ) {
				case 1:
					{
					this.state = 415;
					this.match(SqlParser.IF);
					this.state = 416;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 419;
				(localctx as DropConstraintContext)._tableName = this.qualifiedName();
				this.state = 420;
				this.match(SqlParser.DROP);
				this.state = 421;
				this.match(SqlParser.CONSTRAINT);
				this.state = 424;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 29, this._ctx) ) {
				case 1:
					{
					this.state = 422;
					this.match(SqlParser.IF);
					this.state = 423;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 426;
				(localctx as DropConstraintContext)._name = this.identifier();
				}
				break;
			case 19:
				localctx = new AlterColumnSetNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 19);
				{
				this.state = 428;
				this.match(SqlParser.ALTER);
				this.state = 429;
				this.match(SqlParser.TABLE);
				this.state = 432;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 30, this._ctx) ) {
				case 1:
					{
					this.state = 430;
					this.match(SqlParser.IF);
					this.state = 431;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 434;
				(localctx as AlterColumnSetNotNullContext)._tableName = this.qualifiedName();
				this.state = 435;
				this.match(SqlParser.ALTER);
				this.state = 437;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 31, this._ctx) ) {
				case 1:
					{
					this.state = 436;
					this.match(SqlParser.COLUMN);
					}
					break;
				}
				this.state = 439;
				(localctx as AlterColumnSetNotNullContext)._column = this.identifier();
				this.state = 440;
				this.match(SqlParser.SET);
				this.state = 441;
				this.match(SqlParser.NOT);
				this.state = 442;
				this.match(SqlParser.NULL);
				}
				break;
			case 20:
				localctx = new AlterColumnDropNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 20);
				{
				this.state = 444;
				this.match(SqlParser.ALTER);
				this.state = 445;
				this.match(SqlParser.TABLE);
				this.state = 448;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 32, this._ctx) ) {
				case 1:
					{
					this.state = 446;
					this.match(SqlParser.IF);
					this.state = 447;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 450;
				(localctx as AlterColumnDropNotNullContext)._tableName = this.qualifiedName();
				this.state = 451;
				this.match(SqlParser.ALTER);
				this.state = 453;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 33, this._ctx) ) {
				case 1:
					{
					this.state = 452;
					this.match(SqlParser.COLUMN);
					}
					break;
				}
				this.state = 455;
				(localctx as AlterColumnDropNotNullContext)._column = this.identifier();
				this.state = 456;
				this.match(SqlParser.DROP);
				this.state = 457;
				this.match(SqlParser.NOT);
				this.state = 458;
				this.match(SqlParser.NULL);
				}
				break;
			case 21:
				localctx = new SetTablePropertiesContext(this, localctx);
				this.enterOuterAlt(localctx, 21);
				{
				this.state = 460;
				this.match(SqlParser.ALTER);
				this.state = 461;
				this.match(SqlParser.TABLE);
				this.state = 464;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 34, this._ctx) ) {
				case 1:
					{
					this.state = 462;
					this.match(SqlParser.IF);
					this.state = 463;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 466;
				(localctx as SetTablePropertiesContext)._tableName = this.qualifiedName();
				this.state = 467;
				this.match(SqlParser.SET);
				this.state = 468;
				this.match(SqlParser.PROPERTIES);
				this.state = 469;
				this.properties();
				}
				break;
			case 22:
				localctx = new AnalyzeContext(this, localctx);
				this.enterOuterAlt(localctx, 22);
				{
				this.state = 471;
				this.match(SqlParser.ANALYZE);
				this.state = 472;
				this.qualifiedName();
				this.state = 475;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 473;
					this.match(SqlParser.WITH);
					this.state = 474;
					this.properties();
					}
				}

				}
				break;
			case 23:
				localctx = new CreateTypeContext(this, localctx);
				this.enterOuterAlt(localctx, 23);
				{
				this.state = 477;
				this.match(SqlParser.CREATE);
				this.state = 478;
				this.match(SqlParser.TYPE);
				this.state = 479;
				this.qualifiedName();
				this.state = 480;
				this.match(SqlParser.AS);
				this.state = 493;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 3:
					{
					this.state = 481;
					this.match(SqlParser.T__2);
					this.state = 482;
					this.sqlParameterDeclaration();
					this.state = 487;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 483;
						this.match(SqlParser.T__0);
						this.state = 484;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 489;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 490;
					this.match(SqlParser.T__3);
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
					this.state = 492;
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
				this.state = 495;
				this.match(SqlParser.CREATE);
				this.state = 498;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 496;
					this.match(SqlParser.OR);
					this.state = 497;
					this.match(SqlParser.REPLACE);
					}
				}

				this.state = 500;
				this.match(SqlParser.VIEW);
				this.state = 501;
				this.qualifiedName();
				this.state = 504;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===178) {
					{
					this.state = 502;
					this.match(SqlParser.SECURITY);
					this.state = 503;
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

				this.state = 506;
				this.match(SqlParser.AS);
				this.state = 507;
				this.query();
				}
				break;
			case 25:
				localctx = new RenameViewContext(this, localctx);
				this.enterOuterAlt(localctx, 25);
				{
				this.state = 509;
				this.match(SqlParser.ALTER);
				this.state = 510;
				this.match(SqlParser.VIEW);
				this.state = 513;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 40, this._ctx) ) {
				case 1:
					{
					this.state = 511;
					this.match(SqlParser.IF);
					this.state = 512;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 515;
				(localctx as RenameViewContext)._from_ = this.qualifiedName();
				this.state = 516;
				this.match(SqlParser.RENAME);
				this.state = 517;
				this.match(SqlParser.TO);
				this.state = 518;
				(localctx as RenameViewContext)._to = this.qualifiedName();
				}
				break;
			case 26:
				localctx = new DropViewContext(this, localctx);
				this.enterOuterAlt(localctx, 26);
				{
				this.state = 520;
				this.match(SqlParser.DROP);
				this.state = 521;
				this.match(SqlParser.VIEW);
				this.state = 524;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 41, this._ctx) ) {
				case 1:
					{
					this.state = 522;
					this.match(SqlParser.IF);
					this.state = 523;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 526;
				this.qualifiedName();
				}
				break;
			case 27:
				localctx = new CreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 27);
				{
				this.state = 527;
				this.match(SqlParser.CREATE);
				this.state = 528;
				this.match(SqlParser.MATERIALIZED);
				this.state = 529;
				this.match(SqlParser.VIEW);
				this.state = 533;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 42, this._ctx) ) {
				case 1:
					{
					this.state = 530;
					this.match(SqlParser.IF);
					this.state = 531;
					this.match(SqlParser.NOT);
					this.state = 532;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 535;
				this.qualifiedName();
				this.state = 538;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 536;
					this.match(SqlParser.COMMENT);
					this.state = 537;
					this.string_();
					}
				}

				this.state = 542;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 540;
					this.match(SqlParser.WITH);
					this.state = 541;
					this.properties();
					}
				}

				this.state = 544;
				this.match(SqlParser.AS);
				this.state = 550;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 45, this._ctx) ) {
				case 1:
					{
					this.state = 545;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 546;
					this.match(SqlParser.T__2);
					this.state = 547;
					this.query();
					this.state = 548;
					this.match(SqlParser.T__3);
					}
					break;
				}
				}
				break;
			case 28:
				localctx = new DropMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 28);
				{
				this.state = 552;
				this.match(SqlParser.DROP);
				this.state = 553;
				this.match(SqlParser.MATERIALIZED);
				this.state = 554;
				this.match(SqlParser.VIEW);
				this.state = 557;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 46, this._ctx) ) {
				case 1:
					{
					this.state = 555;
					this.match(SqlParser.IF);
					this.state = 556;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 559;
				this.qualifiedName();
				}
				break;
			case 29:
				localctx = new RefreshMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 29);
				{
				this.state = 560;
				this.match(SqlParser.REFRESH);
				this.state = 561;
				this.match(SqlParser.MATERIALIZED);
				this.state = 562;
				this.match(SqlParser.VIEW);
				this.state = 563;
				this.qualifiedName();
				this.state = 564;
				this.match(SqlParser.WHERE);
				this.state = 565;
				this.booleanExpression(0);
				}
				break;
			case 30:
				localctx = new CreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 30);
				{
				this.state = 567;
				this.match(SqlParser.CREATE);
				this.state = 570;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 568;
					this.match(SqlParser.OR);
					this.state = 569;
					this.match(SqlParser.REPLACE);
					}
				}

				this.state = 573;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 572;
					this.match(SqlParser.TEMPORARY);
					}
				}

				this.state = 575;
				this.match(SqlParser.FUNCTION);
				this.state = 576;
				(localctx as CreateFunctionContext)._functionName = this.qualifiedName();
				this.state = 577;
				this.match(SqlParser.T__2);
				this.state = 586;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 578;
					this.sqlParameterDeclaration();
					this.state = 583;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 579;
						this.match(SqlParser.T__0);
						this.state = 580;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 585;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 588;
				this.match(SqlParser.T__3);
				this.state = 589;
				this.match(SqlParser.RETURNS);
				this.state = 590;
				(localctx as CreateFunctionContext)._returnType = this.type_(0);
				this.state = 593;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 591;
					this.match(SqlParser.COMMENT);
					this.state = 592;
					this.string_();
					}
				}

				this.state = 595;
				this.routineCharacteristics();
				this.state = 596;
				this.routineBody();
				}
				break;
			case 31:
				localctx = new AlterFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 31);
				{
				this.state = 598;
				this.match(SqlParser.ALTER);
				this.state = 599;
				this.match(SqlParser.FUNCTION);
				this.state = 600;
				this.qualifiedName();
				this.state = 602;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===3) {
					{
					this.state = 601;
					this.types();
					}
				}

				this.state = 604;
				this.alterRoutineCharacteristics();
				}
				break;
			case 32:
				localctx = new DropFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 32);
				{
				this.state = 606;
				this.match(SqlParser.DROP);
				this.state = 608;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 607;
					this.match(SqlParser.TEMPORARY);
					}
				}

				this.state = 610;
				this.match(SqlParser.FUNCTION);
				this.state = 613;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 54, this._ctx) ) {
				case 1:
					{
					this.state = 611;
					this.match(SqlParser.IF);
					this.state = 612;
					this.match(SqlParser.EXISTS);
					}
					break;
				}
				this.state = 615;
				this.qualifiedName();
				this.state = 617;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===3) {
					{
					this.state = 616;
					this.types();
					}
				}

				}
				break;
			case 33:
				localctx = new CallContext(this, localctx);
				this.enterOuterAlt(localctx, 33);
				{
				this.state = 619;
				this.match(SqlParser.CALL);
				this.state = 620;
				this.qualifiedName();
				this.state = 621;
				this.match(SqlParser.T__2);
				this.state = 630;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 622;
					this.callArgument();
					this.state = 627;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 623;
						this.match(SqlParser.T__0);
						this.state = 624;
						this.callArgument();
						}
						}
						this.state = 629;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 632;
				this.match(SqlParser.T__3);
				}
				break;
			case 34:
				localctx = new CreateRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 34);
				{
				this.state = 634;
				this.match(SqlParser.CREATE);
				this.state = 635;
				this.match(SqlParser.ROLE);
				this.state = 636;
				(localctx as CreateRoleContext)._name = this.identifier();
				this.state = 640;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 637;
					this.match(SqlParser.WITH);
					this.state = 638;
					this.match(SqlParser.ADMIN);
					this.state = 639;
					this.grantor();
					}
				}

				}
				break;
			case 35:
				localctx = new DropRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 35);
				{
				this.state = 642;
				this.match(SqlParser.DROP);
				this.state = 643;
				this.match(SqlParser.ROLE);
				this.state = 644;
				(localctx as DropRoleContext)._name = this.identifier();
				}
				break;
			case 36:
				localctx = new GrantRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 36);
				{
				this.state = 645;
				this.match(SqlParser.GRANT);
				this.state = 646;
				this.roles();
				this.state = 647;
				this.match(SqlParser.TO);
				this.state = 648;
				this.principal();
				this.state = 653;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 649;
					this.match(SqlParser.T__0);
					this.state = 650;
					this.principal();
					}
					}
					this.state = 655;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 659;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 656;
					this.match(SqlParser.WITH);
					this.state = 657;
					this.match(SqlParser.ADMIN);
					this.state = 658;
					this.match(SqlParser.OPTION);
					}
				}

				this.state = 664;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 661;
					this.match(SqlParser.GRANTED);
					this.state = 662;
					this.match(SqlParser.BY);
					this.state = 663;
					this.grantor();
					}
				}

				}
				break;
			case 37:
				localctx = new RevokeRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 37);
				{
				this.state = 666;
				this.match(SqlParser.REVOKE);
				this.state = 670;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 62, this._ctx) ) {
				case 1:
					{
					this.state = 667;
					this.match(SqlParser.ADMIN);
					this.state = 668;
					this.match(SqlParser.OPTION);
					this.state = 669;
					this.match(SqlParser.FOR);
					}
					break;
				}
				this.state = 672;
				this.roles();
				this.state = 673;
				this.match(SqlParser.FROM);
				this.state = 674;
				this.principal();
				this.state = 679;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 675;
					this.match(SqlParser.T__0);
					this.state = 676;
					this.principal();
					}
					}
					this.state = 681;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 685;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 682;
					this.match(SqlParser.GRANTED);
					this.state = 683;
					this.match(SqlParser.BY);
					this.state = 684;
					this.grantor();
					}
				}

				}
				break;
			case 38:
				localctx = new SetRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 38);
				{
				this.state = 687;
				this.match(SqlParser.SET);
				this.state = 688;
				this.match(SqlParser.ROLE);
				this.state = 692;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 65, this._ctx) ) {
				case 1:
					{
					this.state = 689;
					this.match(SqlParser.ALL);
					}
					break;
				case 2:
					{
					this.state = 690;
					this.match(SqlParser.NONE);
					}
					break;
				case 3:
					{
					this.state = 691;
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
				this.state = 694;
				this.match(SqlParser.GRANT);
				this.state = 705;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 67, this._ctx) ) {
				case 1:
					{
					this.state = 695;
					this.privilege();
					this.state = 700;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 696;
						this.match(SqlParser.T__0);
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
					this.match(SqlParser.ALL);
					this.state = 704;
					this.match(SqlParser.PRIVILEGES);
					}
					break;
				}
				this.state = 707;
				this.match(SqlParser.ON);
				this.state = 709;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 708;
					this.match(SqlParser.TABLE);
					}
				}

				this.state = 711;
				this.qualifiedName();
				this.state = 712;
				this.match(SqlParser.TO);
				this.state = 713;
				(localctx as GrantContext)._grantee = this.principal();
				this.state = 717;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 714;
					this.match(SqlParser.WITH);
					this.state = 715;
					this.match(SqlParser.GRANT);
					this.state = 716;
					this.match(SqlParser.OPTION);
					}
				}

				}
				break;
			case 40:
				localctx = new RevokeContext(this, localctx);
				this.enterOuterAlt(localctx, 40);
				{
				this.state = 719;
				this.match(SqlParser.REVOKE);
				this.state = 723;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 70, this._ctx) ) {
				case 1:
					{
					this.state = 720;
					this.match(SqlParser.GRANT);
					this.state = 721;
					this.match(SqlParser.OPTION);
					this.state = 722;
					this.match(SqlParser.FOR);
					}
					break;
				}
				this.state = 735;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 72, this._ctx) ) {
				case 1:
					{
					this.state = 725;
					this.privilege();
					this.state = 730;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 726;
						this.match(SqlParser.T__0);
						this.state = 727;
						this.privilege();
						}
						}
						this.state = 732;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 733;
					this.match(SqlParser.ALL);
					this.state = 734;
					this.match(SqlParser.PRIVILEGES);
					}
					break;
				}
				this.state = 737;
				this.match(SqlParser.ON);
				this.state = 739;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 738;
					this.match(SqlParser.TABLE);
					}
				}

				this.state = 741;
				this.qualifiedName();
				this.state = 742;
				this.match(SqlParser.FROM);
				this.state = 743;
				(localctx as RevokeContext)._grantee = this.principal();
				}
				break;
			case 41:
				localctx = new ShowGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 41);
				{
				this.state = 745;
				this.match(SqlParser.SHOW);
				this.state = 746;
				this.match(SqlParser.GRANTS);
				this.state = 752;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===137) {
					{
					this.state = 747;
					this.match(SqlParser.ON);
					this.state = 749;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===193) {
						{
						this.state = 748;
						this.match(SqlParser.TABLE);
						}
					}

					this.state = 751;
					this.qualifiedName();
					}
				}

				}
				break;
			case 42:
				localctx = new ExplainContext(this, localctx);
				this.enterOuterAlt(localctx, 42);
				{
				this.state = 754;
				this.match(SqlParser.EXPLAIN);
				this.state = 756;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 76, this._ctx) ) {
				case 1:
					{
					this.state = 755;
					this.match(SqlParser.ANALYZE);
					}
					break;
				}
				this.state = 759;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===219) {
					{
					this.state = 758;
					this.match(SqlParser.VERBOSE);
					}
				}

				this.state = 772;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 79, this._ctx) ) {
				case 1:
					{
					this.state = 761;
					this.match(SqlParser.T__2);
					this.state = 762;
					this.explainOption();
					this.state = 767;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 763;
						this.match(SqlParser.T__0);
						this.state = 764;
						this.explainOption();
						}
						}
						this.state = 769;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 770;
					this.match(SqlParser.T__3);
					}
					break;
				}
				this.state = 774;
				this.statement();
				}
				break;
			case 43:
				localctx = new ShowCreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 43);
				{
				this.state = 775;
				this.match(SqlParser.SHOW);
				this.state = 776;
				this.match(SqlParser.CREATE);
				this.state = 777;
				this.match(SqlParser.TABLE);
				this.state = 778;
				this.qualifiedName();
				}
				break;
			case 44:
				localctx = new ShowCreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 44);
				{
				this.state = 779;
				this.match(SqlParser.SHOW);
				this.state = 780;
				this.match(SqlParser.CREATE);
				this.state = 781;
				this.match(SqlParser.SCHEMA);
				this.state = 782;
				this.qualifiedName();
				}
				break;
			case 45:
				localctx = new ShowCreateViewContext(this, localctx);
				this.enterOuterAlt(localctx, 45);
				{
				this.state = 783;
				this.match(SqlParser.SHOW);
				this.state = 784;
				this.match(SqlParser.CREATE);
				this.state = 785;
				this.match(SqlParser.VIEW);
				this.state = 786;
				this.qualifiedName();
				}
				break;
			case 46:
				localctx = new ShowCreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 46);
				{
				this.state = 787;
				this.match(SqlParser.SHOW);
				this.state = 788;
				this.match(SqlParser.CREATE);
				this.state = 789;
				this.match(SqlParser.MATERIALIZED);
				this.state = 790;
				this.match(SqlParser.VIEW);
				this.state = 791;
				this.qualifiedName();
				}
				break;
			case 47:
				localctx = new ShowCreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 47);
				{
				this.state = 792;
				this.match(SqlParser.SHOW);
				this.state = 793;
				this.match(SqlParser.CREATE);
				this.state = 794;
				this.match(SqlParser.FUNCTION);
				this.state = 795;
				this.qualifiedName();
				this.state = 797;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===3) {
					{
					this.state = 796;
					this.types();
					}
				}

				}
				break;
			case 48:
				localctx = new ShowTablesContext(this, localctx);
				this.enterOuterAlt(localctx, 48);
				{
				this.state = 799;
				this.match(SqlParser.SHOW);
				this.state = 800;
				this.match(SqlParser.TABLES);
				this.state = 803;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 801;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 802;
					this.qualifiedName();
					}
				}

				this.state = 811;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 805;
					this.match(SqlParser.LIKE);
					this.state = 806;
					(localctx as ShowTablesContext)._pattern = this.string_();
					this.state = 809;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 807;
						this.match(SqlParser.ESCAPE);
						this.state = 808;
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
				this.state = 813;
				this.match(SqlParser.SHOW);
				this.state = 814;
				this.match(SqlParser.SCHEMAS);
				this.state = 817;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 815;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 816;
					this.identifier();
					}
				}

				this.state = 825;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 819;
					this.match(SqlParser.LIKE);
					this.state = 820;
					(localctx as ShowSchemasContext)._pattern = this.string_();
					this.state = 823;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 821;
						this.match(SqlParser.ESCAPE);
						this.state = 822;
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
				this.state = 827;
				this.match(SqlParser.SHOW);
				this.state = 828;
				this.match(SqlParser.CATALOGS);
				this.state = 835;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 829;
					this.match(SqlParser.LIKE);
					this.state = 830;
					(localctx as ShowCatalogsContext)._pattern = this.string_();
					this.state = 833;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 831;
						this.match(SqlParser.ESCAPE);
						this.state = 832;
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
				this.state = 837;
				this.match(SqlParser.SHOW);
				this.state = 838;
				this.match(SqlParser.COLUMNS);
				this.state = 839;
				_la = this._input.LA(1);
				if(!(_la===78 || _la===93)) {
				this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 840;
				this.qualifiedName();
				}
				break;
			case 52:
				localctx = new ShowStatsContext(this, localctx);
				this.enterOuterAlt(localctx, 52);
				{
				this.state = 841;
				this.match(SqlParser.SHOW);
				this.state = 842;
				this.match(SqlParser.STATS);
				this.state = 843;
				this.match(SqlParser.FOR);
				this.state = 844;
				this.qualifiedName();
				}
				break;
			case 53:
				localctx = new ShowStatsForQueryContext(this, localctx);
				this.enterOuterAlt(localctx, 53);
				{
				this.state = 845;
				this.match(SqlParser.SHOW);
				this.state = 846;
				this.match(SqlParser.STATS);
				this.state = 847;
				this.match(SqlParser.FOR);
				this.state = 848;
				this.match(SqlParser.T__2);
				this.state = 849;
				this.querySpecification();
				this.state = 850;
				this.match(SqlParser.T__3);
				}
				break;
			case 54:
				localctx = new ShowRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 54);
				{
				this.state = 852;
				this.match(SqlParser.SHOW);
				this.state = 854;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===40) {
					{
					this.state = 853;
					this.match(SqlParser.CURRENT);
					}
				}

				this.state = 856;
				this.match(SqlParser.ROLES);
				this.state = 859;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 857;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 858;
					this.identifier();
					}
				}

				}
				break;
			case 55:
				localctx = new ShowRoleGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 55);
				{
				this.state = 861;
				this.match(SqlParser.SHOW);
				this.state = 862;
				this.match(SqlParser.ROLE);
				this.state = 863;
				this.match(SqlParser.GRANTS);
				this.state = 866;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 864;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 865;
					this.identifier();
					}
				}

				}
				break;
			case 56:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 56);
				{
				this.state = 868;
				this.match(SqlParser.DESCRIBE);
				this.state = 869;
				this.qualifiedName();
				}
				break;
			case 57:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 57);
				{
				this.state = 870;
				this.match(SqlParser.DESC);
				this.state = 871;
				this.qualifiedName();
				}
				break;
			case 58:
				localctx = new ShowFunctionsContext(this, localctx);
				this.enterOuterAlt(localctx, 58);
				{
				this.state = 872;
				this.match(SqlParser.SHOW);
				this.state = 873;
				this.match(SqlParser.FUNCTIONS);
				this.state = 880;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 874;
					this.match(SqlParser.LIKE);
					this.state = 875;
					(localctx as ShowFunctionsContext)._pattern = this.string_();
					this.state = 878;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 876;
						this.match(SqlParser.ESCAPE);
						this.state = 877;
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
				this.state = 882;
				this.match(SqlParser.SHOW);
				this.state = 883;
				this.match(SqlParser.SESSION);
				this.state = 890;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 884;
					this.match(SqlParser.LIKE);
					this.state = 885;
					(localctx as ShowSessionContext)._pattern = this.string_();
					this.state = 888;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 886;
						this.match(SqlParser.ESCAPE);
						this.state = 887;
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
				this.state = 892;
				this.match(SqlParser.SET);
				this.state = 893;
				this.match(SqlParser.SESSION);
				this.state = 894;
				this.qualifiedName();
				this.state = 895;
				this.match(SqlParser.EQ);
				this.state = 896;
				this.expression();
				}
				break;
			case 61:
				localctx = new ResetSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 61);
				{
				this.state = 898;
				this.match(SqlParser.RESET);
				this.state = 899;
				this.match(SqlParser.SESSION);
				this.state = 900;
				this.qualifiedName();
				}
				break;
			case 62:
				localctx = new StartTransactionContext(this, localctx);
				this.enterOuterAlt(localctx, 62);
				{
				this.state = 901;
				this.match(SqlParser.START);
				this.state = 902;
				this.match(SqlParser.TRANSACTION);
				this.state = 911;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===104 || _la===155) {
					{
					this.state = 903;
					this.transactionMode();
					this.state = 908;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 904;
						this.match(SqlParser.T__0);
						this.state = 905;
						this.transactionMode();
						}
						}
						this.state = 910;
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
				this.state = 913;
				this.match(SqlParser.COMMIT);
				this.state = 915;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 914;
					this.match(SqlParser.WORK);
					}
				}

				}
				break;
			case 64:
				localctx = new RollbackContext(this, localctx);
				this.enterOuterAlt(localctx, 64);
				{
				this.state = 917;
				this.match(SqlParser.ROLLBACK);
				this.state = 919;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 918;
					this.match(SqlParser.WORK);
					}
				}

				}
				break;
			case 65:
				localctx = new PrepareContext(this, localctx);
				this.enterOuterAlt(localctx, 65);
				{
				this.state = 921;
				this.match(SqlParser.PREPARE);
				this.state = 922;
				this.identifier();
				this.state = 923;
				this.match(SqlParser.FROM);
				this.state = 924;
				this.statement();
				}
				break;
			case 66:
				localctx = new DeallocateContext(this, localctx);
				this.enterOuterAlt(localctx, 66);
				{
				this.state = 926;
				this.match(SqlParser.DEALLOCATE);
				this.state = 927;
				this.match(SqlParser.PREPARE);
				this.state = 928;
				this.identifier();
				}
				break;
			case 67:
				localctx = new ExecuteContext(this, localctx);
				this.enterOuterAlt(localctx, 67);
				{
				this.state = 929;
				this.match(SqlParser.EXECUTE);
				this.state = 930;
				this.identifier();
				this.state = 940;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===216) {
					{
					this.state = 931;
					this.match(SqlParser.USING);
					this.state = 932;
					this.expression();
					this.state = 937;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 933;
						this.match(SqlParser.T__0);
						this.state = 934;
						this.expression();
						}
						}
						this.state = 939;
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
				this.state = 942;
				this.match(SqlParser.DESCRIBE);
				this.state = 943;
				this.match(SqlParser.INPUT);
				this.state = 944;
				this.identifier();
				}
				break;
			case 69:
				localctx = new DescribeOutputContext(this, localctx);
				this.enterOuterAlt(localctx, 69);
				{
				this.state = 945;
				this.match(SqlParser.DESCRIBE);
				this.state = 946;
				this.match(SqlParser.OUTPUT);
				this.state = 947;
				this.identifier();
				}
				break;
			case 70:
				localctx = new UpdateContext(this, localctx);
				this.enterOuterAlt(localctx, 70);
				{
				this.state = 948;
				this.match(SqlParser.UPDATE);
				this.state = 949;
				this.qualifiedName();
				this.state = 950;
				this.match(SqlParser.SET);
				this.state = 951;
				this.updateAssignment();
				this.state = 956;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 952;
					this.match(SqlParser.T__0);
					this.state = 953;
					this.updateAssignment();
					}
					}
					this.state = 958;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 961;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 959;
					this.match(SqlParser.WHERE);
					this.state = 960;
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
		this.enterRule(localctx, 18, SqlParser.RULE_query);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 966;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 965;
				this.with_();
				}
			}

			this.state = 968;
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
		this.enterRule(localctx, 20, SqlParser.RULE_with);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 970;
			this.match(SqlParser.WITH);
			this.state = 972;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===156) {
				{
				this.state = 971;
				this.match(SqlParser.RECURSIVE);
				}
			}

			this.state = 974;
			this.namedQuery();
			this.state = 979;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 975;
				this.match(SqlParser.T__0);
				this.state = 976;
				this.namedQuery();
				}
				}
				this.state = 981;
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
		this.enterRule(localctx, 22, SqlParser.RULE_tableElement);
		try {
			this.state = 985;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 108, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 982;
				this.constraintSpecification();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 983;
				this.columnDefinition();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 984;
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
		this.enterRule(localctx, 24, SqlParser.RULE_columnDefinition);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 987;
			this.identifier();
			this.state = 988;
			this.type_(0);
			this.state = 991;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===131) {
				{
				this.state = 989;
				this.match(SqlParser.NOT);
				this.state = 990;
				this.match(SqlParser.NULL);
				}
			}

			this.state = 995;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===33) {
				{
				this.state = 993;
				this.match(SqlParser.COMMENT);
				this.state = 994;
				this.string_();
				}
			}

			this.state = 999;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 997;
				this.match(SqlParser.WITH);
				this.state = 998;
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
		this.enterRule(localctx, 26, SqlParser.RULE_likeClause);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1001;
			this.match(SqlParser.LIKE);
			this.state = 1002;
			this.qualifiedName();
			this.state = 1005;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===65 || _la===94) {
				{
				this.state = 1003;
				localctx._optionType = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===65 || _la===94)) {
				    localctx._optionType = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1004;
				this.match(SqlParser.PROPERTIES);
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
		this.enterRule(localctx, 28, SqlParser.RULE_properties);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1007;
			this.match(SqlParser.T__2);
			this.state = 1008;
			this.property();
			this.state = 1013;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 1009;
				this.match(SqlParser.T__0);
				this.state = 1010;
				this.property();
				}
				}
				this.state = 1015;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 1016;
			this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 30, SqlParser.RULE_property);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1018;
			this.identifier();
			this.state = 1019;
			this.match(SqlParser.EQ);
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
	public sqlParameterDeclaration(): SqlParameterDeclarationContext {
		let localctx: SqlParameterDeclarationContext = new SqlParameterDeclarationContext(this, this._ctx, this.state);
		this.enterRule(localctx, 32, SqlParser.RULE_sqlParameterDeclaration);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1022;
			this.identifier();
			this.state = 1023;
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
		this.enterRule(localctx, 34, SqlParser.RULE_routineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1028;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===54 || _la===108 || _la===131 || _la===166) {
				{
				{
				this.state = 1025;
				this.routineCharacteristic();
				}
				}
				this.state = 1030;
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
		this.enterRule(localctx, 36, SqlParser.RULE_routineCharacteristic);
		try {
			this.state = 1035;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 108:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1031;
				this.match(SqlParser.LANGUAGE);
				this.state = 1032;
				this.language();
				}
				break;
			case 54:
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1033;
				this.determinism();
				}
				break;
			case 26:
			case 166:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1034;
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
		this.enterRule(localctx, 38, SqlParser.RULE_alterRoutineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1040;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===166) {
				{
				{
				this.state = 1037;
				this.alterRoutineCharacteristic();
				}
				}
				this.state = 1042;
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
		this.enterRule(localctx, 40, SqlParser.RULE_alterRoutineCharacteristic);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1043;
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
		this.enterRule(localctx, 42, SqlParser.RULE_routineBody);
		try {
			this.state = 1047;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 165:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1045;
				this.returnStatement();
				}
				break;
			case 70:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1046;
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
		this.enterRule(localctx, 44, SqlParser.RULE_returnStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1049;
			this.match(SqlParser.RETURN);
			this.state = 1050;
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
		this.enterRule(localctx, 46, SqlParser.RULE_externalBodyReference);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1052;
			this.match(SqlParser.EXTERNAL);
			this.state = 1055;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===122) {
				{
				this.state = 1053;
				this.match(SqlParser.NAME);
				this.state = 1054;
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
		this.enterRule(localctx, 48, SqlParser.RULE_language);
		try {
			this.state = 1059;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 119, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1057;
				this.match(SqlParser.SQL);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1058;
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
		this.enterRule(localctx, 50, SqlParser.RULE_determinism);
		try {
			this.state = 1064;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 54:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1061;
				this.match(SqlParser.DETERMINISTIC);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1062;
				this.match(SqlParser.NOT);
				this.state = 1063;
				this.match(SqlParser.DETERMINISTIC);
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
		this.enterRule(localctx, 52, SqlParser.RULE_nullCallClause);
		try {
			this.state = 1075;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 166:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1066;
				this.match(SqlParser.RETURNS);
				this.state = 1067;
				this.match(SqlParser.NULL);
				this.state = 1068;
				this.match(SqlParser.ON);
				this.state = 1069;
				this.match(SqlParser.NULL);
				this.state = 1070;
				this.match(SqlParser.INPUT);
				}
				break;
			case 26:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1071;
				this.match(SqlParser.CALLED);
				this.state = 1072;
				this.match(SqlParser.ON);
				this.state = 1073;
				this.match(SqlParser.NULL);
				this.state = 1074;
				this.match(SqlParser.INPUT);
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
		this.enterRule(localctx, 54, SqlParser.RULE_externalRoutineName);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1077;
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
		this.enterRule(localctx, 56, SqlParser.RULE_queryNoWith);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1079;
			this.queryTerm(0);
			this.state = 1090;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1080;
				this.match(SqlParser.ORDER);
				this.state = 1081;
				this.match(SqlParser.BY);
				this.state = 1082;
				this.sortItem();
				this.state = 1087;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1083;
					this.match(SqlParser.T__0);
					this.state = 1084;
					this.sortItem();
					}
					}
					this.state = 1089;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1097;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===136) {
				{
				this.state = 1092;
				this.match(SqlParser.OFFSET);
				this.state = 1093;
				localctx._offset = this.match(SqlParser.INTEGER_VALUE);
				this.state = 1095;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===173 || _la===174) {
					{
					this.state = 1094;
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

			this.state = 1108;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===72 || _la===114) {
				{
				this.state = 1106;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 114:
					{
					this.state = 1099;
					this.match(SqlParser.LIMIT);
					this.state = 1100;
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
					this.state = 1101;
					this.match(SqlParser.FETCH);
					this.state = 1102;
					this.match(SqlParser.FIRST);
					this.state = 1103;
					localctx._fetchFirstNRows = this.match(SqlParser.INTEGER_VALUE);
					this.state = 1104;
					this.match(SqlParser.ROWS);
					this.state = 1105;
					this.match(SqlParser.ONLY);
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
		let _startState: number = 58;
		this.enterRecursionRule(localctx, 58, SqlParser.RULE_queryTerm, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new QueryTermDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1111;
			this.queryPrimary();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1127;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 131, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1125;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 130, this._ctx) ) {
					case 1:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_queryTerm);
						this.state = 1113;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1114;
						(localctx as SetOperationContext)._operator = this.match(SqlParser.INTERSECT);
						this.state = 1116;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1115;
							this.setQuantifier();
							}
						}

						this.state = 1118;
						(localctx as SetOperationContext)._right = this.queryTerm(3);
						}
						break;
					case 2:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_queryTerm);
						this.state = 1119;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1120;
						(localctx as SetOperationContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===64 || _la===210)) {
						    (localctx as SetOperationContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1122;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1121;
							this.setQuantifier();
							}
						}

						this.state = 1124;
						(localctx as SetOperationContext)._right = this.queryTerm(2);
						}
						break;
					}
					}
				}
				this.state = 1129;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 131, this._ctx);
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
		this.enterRule(localctx, 60, SqlParser.RULE_queryPrimary);
		try {
			let _alt: number;
			this.state = 1146;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				localctx = new QueryPrimaryDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1130;
				this.querySpecification();
				}
				break;
			case 193:
				localctx = new TableContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1131;
				this.match(SqlParser.TABLE);
				this.state = 1132;
				this.qualifiedName();
				}
				break;
			case 218:
				localctx = new InlineTableContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1133;
				this.match(SqlParser.VALUES);
				this.state = 1134;
				this.expression();
				this.state = 1139;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 132, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1135;
						this.match(SqlParser.T__0);
						this.state = 1136;
						this.expression();
						}
						}
					}
					this.state = 1141;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 132, this._ctx);
				}
				}
				break;
			case 3:
				localctx = new SubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1142;
				this.match(SqlParser.T__2);
				this.state = 1143;
				this.queryNoWith();
				this.state = 1144;
				this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 62, SqlParser.RULE_sortItem);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1148;
			this.expression();
			this.state = 1150;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===19 || _la===52) {
				{
				this.state = 1149;
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

			this.state = 1154;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===134) {
				{
				this.state = 1152;
				this.match(SqlParser.NULLS);
				this.state = 1153;
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
		this.enterRule(localctx, 64, SqlParser.RULE_querySpecification);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1156;
			this.match(SqlParser.SELECT);
			this.state = 1158;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 136, this._ctx) ) {
			case 1:
				{
				this.state = 1157;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1160;
			this.selectItem();
			this.state = 1165;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 137, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1161;
					this.match(SqlParser.T__0);
					this.state = 1162;
					this.selectItem();
					}
					}
				}
				this.state = 1167;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 137, this._ctx);
			}
			this.state = 1177;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 139, this._ctx) ) {
			case 1:
				{
				this.state = 1168;
				this.match(SqlParser.FROM);
				this.state = 1169;
				this.relation(0);
				this.state = 1174;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 138, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1170;
						this.match(SqlParser.T__0);
						this.state = 1171;
						this.relation(0);
						}
						}
					}
					this.state = 1176;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 138, this._ctx);
				}
				}
				break;
			}
			this.state = 1181;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 140, this._ctx) ) {
			case 1:
				{
				this.state = 1179;
				this.match(SqlParser.WHERE);
				this.state = 1180;
				localctx._where = this.booleanExpression(0);
				}
				break;
			}
			this.state = 1186;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 141, this._ctx) ) {
			case 1:
				{
				this.state = 1183;
				this.match(SqlParser.GROUP);
				this.state = 1184;
				this.match(SqlParser.BY);
				this.state = 1185;
				this.groupBy();
				}
				break;
			}
			this.state = 1190;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 142, this._ctx) ) {
			case 1:
				{
				this.state = 1188;
				this.match(SqlParser.HAVING);
				this.state = 1189;
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
		this.enterRule(localctx, 66, SqlParser.RULE_groupBy);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1193;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 143, this._ctx) ) {
			case 1:
				{
				this.state = 1192;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1195;
			this.groupingElement();
			this.state = 1200;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 144, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1196;
					this.match(SqlParser.T__0);
					this.state = 1197;
					this.groupingElement();
					}
					}
				}
				this.state = 1202;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 144, this._ctx);
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
		this.enterRule(localctx, 68, SqlParser.RULE_groupingElement);
		let _la: number;
		try {
			this.state = 1243;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 150, this._ctx) ) {
			case 1:
				localctx = new SingleGroupingSetContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1203;
				this.groupingSet();
				}
				break;
			case 2:
				localctx = new RollupContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1204;
				this.match(SqlParser.ROLLUP);
				this.state = 1205;
				this.match(SqlParser.T__2);
				this.state = 1214;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1206;
					this.expression();
					this.state = 1211;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1207;
						this.match(SqlParser.T__0);
						this.state = 1208;
						this.expression();
						}
						}
						this.state = 1213;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1216;
				this.match(SqlParser.T__3);
				}
				break;
			case 3:
				localctx = new CubeContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1217;
				this.match(SqlParser.CUBE);
				this.state = 1218;
				this.match(SqlParser.T__2);
				this.state = 1227;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1219;
					this.expression();
					this.state = 1224;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1220;
						this.match(SqlParser.T__0);
						this.state = 1221;
						this.expression();
						}
						}
						this.state = 1226;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1229;
				this.match(SqlParser.T__3);
				}
				break;
			case 4:
				localctx = new MultipleGroupingSetsContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1230;
				this.match(SqlParser.GROUPING);
				this.state = 1231;
				this.match(SqlParser.SETS);
				this.state = 1232;
				this.match(SqlParser.T__2);
				this.state = 1233;
				this.groupingSet();
				this.state = 1238;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1234;
					this.match(SqlParser.T__0);
					this.state = 1235;
					this.groupingSet();
					}
					}
					this.state = 1240;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1241;
				this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 70, SqlParser.RULE_groupingSet);
		let _la: number;
		try {
			this.state = 1258;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 153, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1245;
				this.match(SqlParser.T__2);
				this.state = 1254;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1246;
					this.expression();
					this.state = 1251;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1247;
						this.match(SqlParser.T__0);
						this.state = 1248;
						this.expression();
						}
						}
						this.state = 1253;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1256;
				this.match(SqlParser.T__3);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1257;
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
		this.enterRule(localctx, 72, SqlParser.RULE_namedQuery);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1260;
			localctx._name = this.identifier();
			this.state = 1262;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===3) {
				{
				this.state = 1261;
				this.columnAliases();
				}
			}

			this.state = 1264;
			this.match(SqlParser.AS);
			this.state = 1265;
			this.match(SqlParser.T__2);
			this.state = 1266;
			this.query();
			this.state = 1267;
			this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 74, SqlParser.RULE_setQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1269;
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
		this.enterRule(localctx, 76, SqlParser.RULE_selectItem);
		let _la: number;
		try {
			this.state = 1283;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 157, this._ctx) ) {
			case 1:
				localctx = new SelectSingleContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1271;
				this.expression();
				this.state = 1276;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 156, this._ctx) ) {
				case 1:
					{
					this.state = 1273;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===18) {
						{
						this.state = 1272;
						this.match(SqlParser.AS);
						}
					}

					this.state = 1275;
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
				this.state = 1278;
				this.qualifiedName();
				this.state = 1279;
				this.match(SqlParser.T__1);
				this.state = 1280;
				this.match(SqlParser.ASTERISK);
				}
				break;
			case 3:
				localctx = new SelectAllContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1282;
				this.match(SqlParser.ASTERISK);
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
		let _startState: number = 78;
		this.enterRecursionRule(localctx, 78, SqlParser.RULE_relation, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new RelationDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1286;
			this.sampledRelation();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1306;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 159, this._ctx);
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
					this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_relation);
					this.state = 1288;
					if (!(this.precpred(this._ctx, 2))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
					}
					this.state = 1302;
					this._errHandler.sync(this);
					switch (this._input.LA(1)) {
					case 38:
						{
						this.state = 1289;
						this.match(SqlParser.CROSS);
						this.state = 1290;
						this.match(SqlParser.JOIN);
						this.state = 1291;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					case 79:
					case 95:
					case 106:
					case 111:
					case 168:
						{
						this.state = 1292;
						this.joinType();
						this.state = 1293;
						this.match(SqlParser.JOIN);
						this.state = 1294;
						(localctx as JoinRelationContext)._rightRelation = this.relation(0);
						this.state = 1295;
						this.joinCriteria();
						}
						break;
					case 123:
						{
						this.state = 1297;
						this.match(SqlParser.NATURAL);
						this.state = 1298;
						this.joinType();
						this.state = 1299;
						this.match(SqlParser.JOIN);
						this.state = 1300;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					default:
						throw new NoViableAltException(this);
					}
					}
					}
				}
				this.state = 1308;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 159, this._ctx);
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
		this.enterRule(localctx, 80, SqlParser.RULE_joinType);
		let _la: number;
		try {
			this.state = 1324;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 95:
			case 106:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1310;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===95) {
					{
					this.state = 1309;
					this.match(SqlParser.INNER);
					}
				}

				}
				break;
			case 111:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1312;
				this.match(SqlParser.LEFT);
				this.state = 1314;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1313;
					this.match(SqlParser.OUTER);
					}
				}

				}
				break;
			case 168:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1316;
				this.match(SqlParser.RIGHT);
				this.state = 1318;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1317;
					this.match(SqlParser.OUTER);
					}
				}

				}
				break;
			case 79:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1320;
				this.match(SqlParser.FULL);
				this.state = 1322;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1321;
					this.match(SqlParser.OUTER);
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
		this.enterRule(localctx, 82, SqlParser.RULE_joinCriteria);
		let _la: number;
		try {
			this.state = 1340;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 137:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1326;
				this.match(SqlParser.ON);
				this.state = 1327;
				this.booleanExpression(0);
				}
				break;
			case 216:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1328;
				this.match(SqlParser.USING);
				this.state = 1329;
				this.match(SqlParser.T__2);
				this.state = 1330;
				this.identifier();
				this.state = 1335;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1331;
					this.match(SqlParser.T__0);
					this.state = 1332;
					this.identifier();
					}
					}
					this.state = 1337;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1338;
				this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 84, SqlParser.RULE_sampledRelation);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1342;
			this.aliasedRelation();
			this.state = 1349;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 167, this._ctx) ) {
			case 1:
				{
				this.state = 1343;
				this.match(SqlParser.TABLESAMPLE);
				this.state = 1344;
				this.sampleType();
				this.state = 1345;
				this.match(SqlParser.T__2);
				this.state = 1346;
				localctx._percentage = this.expression();
				this.state = 1347;
				this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 86, SqlParser.RULE_sampleType);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1351;
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
		this.enterRule(localctx, 88, SqlParser.RULE_aliasedRelation);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1353;
			this.relationPrimary();
			this.state = 1361;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 170, this._ctx) ) {
			case 1:
				{
				this.state = 1355;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===18) {
					{
					this.state = 1354;
					this.match(SqlParser.AS);
					}
				}

				this.state = 1357;
				this.identifier();
				this.state = 1359;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 169, this._ctx) ) {
				case 1:
					{
					this.state = 1358;
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
		this.enterRule(localctx, 90, SqlParser.RULE_columnAliases);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1363;
			this.match(SqlParser.T__2);
			this.state = 1364;
			this.identifier();
			this.state = 1369;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 1365;
				this.match(SqlParser.T__0);
				this.state = 1366;
				this.identifier();
				}
				}
				this.state = 1371;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 1372;
			this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 92, SqlParser.RULE_relationPrimary);
		let _la: number;
		try {
			this.state = 1406;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 175, this._ctx) ) {
			case 1:
				localctx = new TableNameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1374;
				this.qualifiedName();
				this.state = 1376;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 172, this._ctx) ) {
				case 1:
					{
					this.state = 1375;
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
				this.state = 1378;
				this.match(SqlParser.T__2);
				this.state = 1379;
				this.query();
				this.state = 1380;
				this.match(SqlParser.T__3);
				}
				break;
			case 3:
				localctx = new UnnestContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1382;
				this.match(SqlParser.UNNEST);
				this.state = 1383;
				this.match(SqlParser.T__2);
				this.state = 1384;
				this.expression();
				this.state = 1389;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1385;
					this.match(SqlParser.T__0);
					this.state = 1386;
					this.expression();
					}
					}
					this.state = 1391;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1392;
				this.match(SqlParser.T__3);
				this.state = 1395;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 174, this._ctx) ) {
				case 1:
					{
					this.state = 1393;
					this.match(SqlParser.WITH);
					this.state = 1394;
					this.match(SqlParser.ORDINALITY);
					}
					break;
				}
				}
				break;
			case 4:
				localctx = new LateralContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1397;
				this.match(SqlParser.LATERAL);
				this.state = 1398;
				this.match(SqlParser.T__2);
				this.state = 1399;
				this.query();
				this.state = 1400;
				this.match(SqlParser.T__3);
				}
				break;
			case 5:
				localctx = new ParenthesizedRelationContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1402;
				this.match(SqlParser.T__2);
				this.state = 1403;
				this.relation(0);
				this.state = 1404;
				this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 94, SqlParser.RULE_expression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1408;
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
		let _startState: number = 96;
		this.enterRecursionRule(localctx, 96, SqlParser.RULE_booleanExpression, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1417;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 3:
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

				this.state = 1411;
				(localctx as PredicatedContext)._valueExpression = this.valueExpression(0);
				this.state = 1413;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 176, this._ctx) ) {
				case 1:
					{
					this.state = 1412;
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
				this.state = 1415;
				this.match(SqlParser.NOT);
				this.state = 1416;
				this.booleanExpression(3);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1427;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 179, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1425;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 178, this._ctx) ) {
					case 1:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_booleanExpression);
						this.state = 1419;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1420;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlParser.AND);
						this.state = 1421;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(3);
						}
						break;
					case 2:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_booleanExpression);
						this.state = 1422;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1423;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlParser.OR);
						this.state = 1424;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(2);
						}
						break;
					}
					}
				}
				this.state = 1429;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 179, this._ctx);
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
		this.enterRule(localctx, 98, SqlParser.RULE_predicate);
		let _la: number;
		try {
			this.state = 1491;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 188, this._ctx) ) {
			case 1:
				localctx = new ComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1430;
				this.comparisonOperator();
				this.state = 1431;
				(localctx as ComparisonContext)._right = this.valueExpression(0);
				}
				break;
			case 2:
				localctx = new QuantifiedComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1433;
				this.comparisonOperator();
				this.state = 1434;
				this.comparisonQuantifier();
				this.state = 1435;
				this.match(SqlParser.T__2);
				this.state = 1436;
				this.query();
				this.state = 1437;
				this.match(SqlParser.T__3);
				}
				break;
			case 3:
				localctx = new BetweenContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1440;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1439;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1442;
				this.match(SqlParser.BETWEEN);
				this.state = 1443;
				(localctx as BetweenContext)._lower = this.valueExpression(0);
				this.state = 1444;
				this.match(SqlParser.AND);
				this.state = 1445;
				(localctx as BetweenContext)._upper = this.valueExpression(0);
				}
				break;
			case 4:
				localctx = new InListContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1448;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1447;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1450;
				this.match(SqlParser.IN);
				this.state = 1451;
				this.match(SqlParser.T__2);
				this.state = 1452;
				this.expression();
				this.state = 1457;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1453;
					this.match(SqlParser.T__0);
					this.state = 1454;
					this.expression();
					}
					}
					this.state = 1459;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1460;
				this.match(SqlParser.T__3);
				}
				break;
			case 5:
				localctx = new InSubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1463;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1462;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1465;
				this.match(SqlParser.IN);
				this.state = 1466;
				this.match(SqlParser.T__2);
				this.state = 1467;
				this.query();
				this.state = 1468;
				this.match(SqlParser.T__3);
				}
				break;
			case 6:
				localctx = new LikeContext(this, localctx);
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1471;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1470;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1473;
				this.match(SqlParser.LIKE);
				this.state = 1474;
				(localctx as LikeContext)._pattern = this.valueExpression(0);
				this.state = 1477;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 185, this._ctx) ) {
				case 1:
					{
					this.state = 1475;
					this.match(SqlParser.ESCAPE);
					this.state = 1476;
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
				this.state = 1479;
				this.match(SqlParser.IS);
				this.state = 1481;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1480;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1483;
				this.match(SqlParser.NULL);
				}
				break;
			case 8:
				localctx = new DistinctFromContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 1484;
				this.match(SqlParser.IS);
				this.state = 1486;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1485;
					this.match(SqlParser.NOT);
					}
				}

				this.state = 1488;
				this.match(SqlParser.DISTINCT);
				this.state = 1489;
				this.match(SqlParser.FROM);
				this.state = 1490;
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
		let _startState: number = 100;
		this.enterRecursionRule(localctx, 100, SqlParser.RULE_valueExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1497;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 3:
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

				this.state = 1494;
				this.primaryExpression(0);
				}
				break;
			case 235:
			case 236:
				{
				localctx = new ArithmeticUnaryContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1495;
				(localctx as ArithmeticUnaryContext)._operator = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===235 || _la===236)) {
				    (localctx as ArithmeticUnaryContext)._operator = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1496;
				this.valueExpression(4);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1513;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 191, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1511;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 190, this._ctx) ) {
					case 1:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_valueExpression);
						this.state = 1499;
						if (!(this.precpred(this._ctx, 3))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 3)");
						}
						this.state = 1500;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(((((_la - 237)) & ~0x1F) === 0 && ((1 << (_la - 237)) & 7) !== 0))) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1501;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(4);
						}
						break;
					case 2:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_valueExpression);
						this.state = 1502;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1503;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===235 || _la===236)) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1504;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(3);
						}
						break;
					case 3:
						{
						localctx = new ConcatenationContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ConcatenationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_valueExpression);
						this.state = 1505;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1506;
						this.match(SqlParser.CONCAT);
						this.state = 1507;
						(localctx as ConcatenationContext)._right = this.valueExpression(2);
						}
						break;
					case 4:
						{
						localctx = new AtTimeZoneContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_valueExpression);
						this.state = 1508;
						if (!(this.precpred(this._ctx, 5))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 5)");
						}
						this.state = 1509;
						this.match(SqlParser.AT);
						this.state = 1510;
						this.timeZoneSpecifier();
						}
						break;
					}
					}
				}
				this.state = 1515;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 191, this._ctx);
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
		let _startState: number = 102;
		this.enterRecursionRule(localctx, 102, SqlParser.RULE_primaryExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1755;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 220, this._ctx) ) {
			case 1:
				{
				localctx = new NullLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1517;
				this.match(SqlParser.NULL);
				}
				break;
			case 2:
				{
				localctx = new IntervalLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1518;
				this.interval();
				}
				break;
			case 3:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1519;
				this.identifier();
				this.state = 1520;
				this.string_();
				}
				break;
			case 4:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1522;
				this.match(SqlParser.DOUBLE_PRECISION);
				this.state = 1523;
				this.string_();
				}
				break;
			case 5:
				{
				localctx = new NumericLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1524;
				this.number_();
				}
				break;
			case 6:
				{
				localctx = new BooleanLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1525;
				this.booleanValue();
				}
				break;
			case 7:
				{
				localctx = new StringLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1526;
				this.string_();
				}
				break;
			case 8:
				{
				localctx = new BinaryLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1527;
				this.match(SqlParser.BINARY_LITERAL);
				}
				break;
			case 9:
				{
				localctx = new ParameterContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1528;
				this.match(SqlParser.T__4);
				}
				break;
			case 10:
				{
				localctx = new PositionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1529;
				this.match(SqlParser.POSITION);
				this.state = 1530;
				this.match(SqlParser.T__2);
				this.state = 1531;
				this.valueExpression(0);
				this.state = 1532;
				this.match(SqlParser.IN);
				this.state = 1533;
				this.valueExpression(0);
				this.state = 1534;
				this.match(SqlParser.T__3);
				}
				break;
			case 11:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1536;
				this.match(SqlParser.T__2);
				this.state = 1537;
				this.expression();
				this.state = 1540;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1538;
					this.match(SqlParser.T__0);
					this.state = 1539;
					this.expression();
					}
					}
					this.state = 1542;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===1);
				this.state = 1544;
				this.match(SqlParser.T__3);
				}
				break;
			case 12:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1546;
				this.match(SqlParser.ROW);
				this.state = 1547;
				this.match(SqlParser.T__2);
				this.state = 1548;
				this.expression();
				this.state = 1553;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1549;
					this.match(SqlParser.T__0);
					this.state = 1550;
					this.expression();
					}
					}
					this.state = 1555;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1556;
				this.match(SqlParser.T__3);
				}
				break;
			case 13:
				{
				localctx = new FunctionCallContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1558;
				this.qualifiedName();
				this.state = 1559;
				this.match(SqlParser.T__2);
				this.state = 1560;
				this.match(SqlParser.ASTERISK);
				this.state = 1561;
				this.match(SqlParser.T__3);
				this.state = 1563;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 194, this._ctx) ) {
				case 1:
					{
					this.state = 1562;
					this.filter();
					}
					break;
				}
				this.state = 1566;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 195, this._ctx) ) {
				case 1:
					{
					this.state = 1565;
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
				this.state = 1568;
				this.qualifiedName();
				this.state = 1569;
				this.match(SqlParser.T__2);
				this.state = 1581;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1406533391) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1571;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 196, this._ctx) ) {
					case 1:
						{
						this.state = 1570;
						this.setQuantifier();
						}
						break;
					}
					this.state = 1573;
					this.expression();
					this.state = 1578;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1574;
						this.match(SqlParser.T__0);
						this.state = 1575;
						this.expression();
						}
						}
						this.state = 1580;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1593;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===141) {
					{
					this.state = 1583;
					this.match(SqlParser.ORDER);
					this.state = 1584;
					this.match(SqlParser.BY);
					this.state = 1585;
					this.sortItem();
					this.state = 1590;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1586;
						this.match(SqlParser.T__0);
						this.state = 1587;
						this.sortItem();
						}
						}
						this.state = 1592;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1595;
				this.match(SqlParser.T__3);
				this.state = 1597;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 201, this._ctx) ) {
				case 1:
					{
					this.state = 1596;
					this.filter();
					}
					break;
				}
				this.state = 1603;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 203, this._ctx) ) {
				case 1:
					{
					this.state = 1600;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===92 || _la===163) {
						{
						this.state = 1599;
						this.nullTreatment();
						}
					}

					this.state = 1602;
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
				this.state = 1605;
				this.identifier();
				this.state = 1606;
				this.match(SqlParser.T__5);
				this.state = 1607;
				this.expression();
				}
				break;
			case 16:
				{
				localctx = new LambdaContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1609;
				this.match(SqlParser.T__2);
				this.state = 1618;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1610;
					this.identifier();
					this.state = 1615;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1611;
						this.match(SqlParser.T__0);
						this.state = 1612;
						this.identifier();
						}
						}
						this.state = 1617;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1620;
				this.match(SqlParser.T__3);
				this.state = 1621;
				this.match(SqlParser.T__5);
				this.state = 1622;
				this.expression();
				}
				break;
			case 17:
				{
				localctx = new SubqueryExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1623;
				this.match(SqlParser.T__2);
				this.state = 1624;
				this.query();
				this.state = 1625;
				this.match(SqlParser.T__3);
				}
				break;
			case 18:
				{
				localctx = new ExistsContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1627;
				this.match(SqlParser.EXISTS);
				this.state = 1628;
				this.match(SqlParser.T__2);
				this.state = 1629;
				this.query();
				this.state = 1630;
				this.match(SqlParser.T__3);
				}
				break;
			case 19:
				{
				localctx = new SimpleCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1632;
				this.match(SqlParser.CASE);
				this.state = 1633;
				this.valueExpression(0);
				this.state = 1635;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1634;
					this.whenClause();
					}
					}
					this.state = 1637;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1641;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1639;
					this.match(SqlParser.ELSE);
					this.state = 1640;
					(localctx as SimpleCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1643;
				this.match(SqlParser.END);
				}
				break;
			case 20:
				{
				localctx = new SearchedCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1645;
				this.match(SqlParser.CASE);
				this.state = 1647;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1646;
					this.whenClause();
					}
					}
					this.state = 1649;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1653;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1651;
					this.match(SqlParser.ELSE);
					this.state = 1652;
					(localctx as SearchedCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1655;
				this.match(SqlParser.END);
				}
				break;
			case 21:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1657;
				this.match(SqlParser.CAST);
				this.state = 1658;
				this.match(SqlParser.T__2);
				this.state = 1659;
				this.expression();
				this.state = 1660;
				this.match(SqlParser.AS);
				this.state = 1661;
				this.type_(0);
				this.state = 1662;
				this.match(SqlParser.T__3);
				}
				break;
			case 22:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1664;
				this.match(SqlParser.TRY_CAST);
				this.state = 1665;
				this.match(SqlParser.T__2);
				this.state = 1666;
				this.expression();
				this.state = 1667;
				this.match(SqlParser.AS);
				this.state = 1668;
				this.type_(0);
				this.state = 1669;
				this.match(SqlParser.T__3);
				}
				break;
			case 23:
				{
				localctx = new ArrayConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1671;
				this.match(SqlParser.ARRAY);
				this.state = 1672;
				this.match(SqlParser.T__6);
				this.state = 1681;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497384) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1673;
					this.expression();
					this.state = 1678;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1674;
						this.match(SqlParser.T__0);
						this.state = 1675;
						this.expression();
						}
						}
						this.state = 1680;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1683;
				this.match(SqlParser.T__7);
				}
				break;
			case 24:
				{
				localctx = new ColumnReferenceContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1684;
				this.identifier();
				}
				break;
			case 25:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1685;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlParser.CURRENT_DATE);
				}
				break;
			case 26:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1686;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlParser.CURRENT_TIME);
				this.state = 1690;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 212, this._ctx) ) {
				case 1:
					{
					this.state = 1687;
					this.match(SqlParser.T__2);
					this.state = 1688;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlParser.INTEGER_VALUE);
					this.state = 1689;
					this.match(SqlParser.T__3);
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
				this.state = 1692;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlParser.CURRENT_TIMESTAMP);
				this.state = 1696;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 213, this._ctx) ) {
				case 1:
					{
					this.state = 1693;
					this.match(SqlParser.T__2);
					this.state = 1694;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlParser.INTEGER_VALUE);
					this.state = 1695;
					this.match(SqlParser.T__3);
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
				this.state = 1698;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlParser.LOCALTIME);
				this.state = 1702;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 214, this._ctx) ) {
				case 1:
					{
					this.state = 1699;
					this.match(SqlParser.T__2);
					this.state = 1700;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlParser.INTEGER_VALUE);
					this.state = 1701;
					this.match(SqlParser.T__3);
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
				this.state = 1704;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlParser.LOCALTIMESTAMP);
				this.state = 1708;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 215, this._ctx) ) {
				case 1:
					{
					this.state = 1705;
					this.match(SqlParser.T__2);
					this.state = 1706;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlParser.INTEGER_VALUE);
					this.state = 1707;
					this.match(SqlParser.T__3);
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
				this.state = 1710;
				(localctx as CurrentUserContext)._name = this.match(SqlParser.CURRENT_USER);
				}
				break;
			case 31:
				{
				localctx = new SubstringContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1711;
				this.match(SqlParser.SUBSTRING);
				this.state = 1712;
				this.match(SqlParser.T__2);
				this.state = 1713;
				this.valueExpression(0);
				this.state = 1714;
				this.match(SqlParser.FROM);
				this.state = 1715;
				this.valueExpression(0);
				this.state = 1718;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===76) {
					{
					this.state = 1716;
					this.match(SqlParser.FOR);
					this.state = 1717;
					this.valueExpression(0);
					}
				}

				this.state = 1720;
				this.match(SqlParser.T__3);
				}
				break;
			case 32:
				{
				localctx = new NormalizeContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1722;
				this.match(SqlParser.NORMALIZE);
				this.state = 1723;
				this.match(SqlParser.T__2);
				this.state = 1724;
				this.valueExpression(0);
				this.state = 1727;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===1) {
					{
					this.state = 1725;
					this.match(SqlParser.T__0);
					this.state = 1726;
					this.normalForm();
					}
				}

				this.state = 1729;
				this.match(SqlParser.T__3);
				}
				break;
			case 33:
				{
				localctx = new ExtractContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1731;
				this.match(SqlParser.EXTRACT);
				this.state = 1732;
				this.match(SqlParser.T__2);
				this.state = 1733;
				this.identifier();
				this.state = 1734;
				this.match(SqlParser.FROM);
				this.state = 1735;
				this.valueExpression(0);
				this.state = 1736;
				this.match(SqlParser.T__3);
				}
				break;
			case 34:
				{
				localctx = new ParenthesizedExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1738;
				this.match(SqlParser.T__2);
				this.state = 1739;
				this.expression();
				this.state = 1740;
				this.match(SqlParser.T__3);
				}
				break;
			case 35:
				{
				localctx = new GroupingOperationContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1742;
				this.match(SqlParser.GROUPING);
				this.state = 1743;
				this.match(SqlParser.T__2);
				this.state = 1752;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1744;
					this.qualifiedName();
					this.state = 1749;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1745;
						this.match(SqlParser.T__0);
						this.state = 1746;
						this.qualifiedName();
						}
						}
						this.state = 1751;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1754;
				this.match(SqlParser.T__3);
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1767;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 222, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1765;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 221, this._ctx) ) {
					case 1:
						{
						localctx = new SubscriptContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as SubscriptContext)._value = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_primaryExpression);
						this.state = 1757;
						if (!(this.precpred(this._ctx, 14))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 14)");
						}
						this.state = 1758;
						this.match(SqlParser.T__6);
						this.state = 1759;
						(localctx as SubscriptContext)._index = this.valueExpression(0);
						this.state = 1760;
						this.match(SqlParser.T__7);
						}
						break;
					case 2:
						{
						localctx = new DereferenceContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as DereferenceContext)._base = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_primaryExpression);
						this.state = 1762;
						if (!(this.precpred(this._ctx, 12))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 12)");
						}
						this.state = 1763;
						this.match(SqlParser.T__1);
						this.state = 1764;
						(localctx as DereferenceContext)._fieldName = this.identifier();
						}
						break;
					}
					}
				}
				this.state = 1769;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 222, this._ctx);
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
		this.enterRule(localctx, 104, SqlParser.RULE_string);
		try {
			this.state = 1776;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 241:
				localctx = new BasicStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1770;
				this.match(SqlParser.STRING);
				}
				break;
			case 242:
				localctx = new UnicodeStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1771;
				this.match(SqlParser.UNICODE_STRING);
				this.state = 1774;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 223, this._ctx) ) {
				case 1:
					{
					this.state = 1772;
					this.match(SqlParser.UESCAPE);
					this.state = 1773;
					this.match(SqlParser.STRING);
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
		this.enterRule(localctx, 106, SqlParser.RULE_nullTreatment);
		try {
			this.state = 1782;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 92:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1778;
				this.match(SqlParser.IGNORE);
				this.state = 1779;
				this.match(SqlParser.NULLS);
				}
				break;
			case 163:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1780;
				this.match(SqlParser.RESPECT);
				this.state = 1781;
				this.match(SqlParser.NULLS);
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
		this.enterRule(localctx, 108, SqlParser.RULE_timeZoneSpecifier);
		try {
			this.state = 1790;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 226, this._ctx) ) {
			case 1:
				localctx = new TimeZoneIntervalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1784;
				this.match(SqlParser.TIME);
				this.state = 1785;
				this.match(SqlParser.ZONE);
				this.state = 1786;
				this.interval();
				}
				break;
			case 2:
				localctx = new TimeZoneStringContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1787;
				this.match(SqlParser.TIME);
				this.state = 1788;
				this.match(SqlParser.ZONE);
				this.state = 1789;
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
		this.enterRule(localctx, 110, SqlParser.RULE_comparisonOperator);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1792;
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
		this.enterRule(localctx, 112, SqlParser.RULE_comparisonQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1794;
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
		this.enterRule(localctx, 114, SqlParser.RULE_booleanValue);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1796;
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
		this.enterRule(localctx, 116, SqlParser.RULE_interval);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1798;
			this.match(SqlParser.INTERVAL);
			this.state = 1800;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===235 || _la===236) {
				{
				this.state = 1799;
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

			this.state = 1802;
			this.string_();
			this.state = 1803;
			localctx._from_ = this.intervalField();
			this.state = 1806;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 228, this._ctx) ) {
			case 1:
				{
				this.state = 1804;
				this.match(SqlParser.TO);
				this.state = 1805;
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
		this.enterRule(localctx, 118, SqlParser.RULE_intervalField);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1808;
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
		this.enterRule(localctx, 120, SqlParser.RULE_normalForm);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1810;
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
		this.enterRule(localctx, 122, SqlParser.RULE_types);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1812;
			this.match(SqlParser.T__2);
			this.state = 1821;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 127) !== 0)) {
				{
				this.state = 1813;
				this.type_(0);
				this.state = 1818;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1814;
					this.match(SqlParser.T__0);
					this.state = 1815;
					this.type_(0);
					}
					}
					this.state = 1820;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1823;
			this.match(SqlParser.T__3);
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
		let _startState: number = 124;
		this.enterRecursionRule(localctx, 124, SqlParser.RULE_type, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1872;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 234, this._ctx) ) {
			case 1:
				{
				this.state = 1826;
				this.match(SqlParser.ARRAY);
				this.state = 1827;
				this.match(SqlParser.LT);
				this.state = 1828;
				this.type_(0);
				this.state = 1829;
				this.match(SqlParser.GT);
				}
				break;
			case 2:
				{
				this.state = 1831;
				this.match(SqlParser.MAP);
				this.state = 1832;
				this.match(SqlParser.LT);
				this.state = 1833;
				this.type_(0);
				this.state = 1834;
				this.match(SqlParser.T__0);
				this.state = 1835;
				this.type_(0);
				this.state = 1836;
				this.match(SqlParser.GT);
				}
				break;
			case 3:
				{
				this.state = 1838;
				this.match(SqlParser.ROW);
				this.state = 1839;
				this.match(SqlParser.T__2);
				this.state = 1840;
				this.identifier();
				this.state = 1841;
				this.type_(0);
				this.state = 1848;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1842;
					this.match(SqlParser.T__0);
					this.state = 1843;
					this.identifier();
					this.state = 1844;
					this.type_(0);
					}
					}
					this.state = 1850;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1851;
				this.match(SqlParser.T__3);
				}
				break;
			case 4:
				{
				this.state = 1853;
				this.baseType();
				this.state = 1865;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 233, this._ctx) ) {
				case 1:
					{
					this.state = 1854;
					this.match(SqlParser.T__2);
					this.state = 1855;
					this.typeParameter();
					this.state = 1860;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===1) {
						{
						{
						this.state = 1856;
						this.match(SqlParser.T__0);
						this.state = 1857;
						this.typeParameter();
						}
						}
						this.state = 1862;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 1863;
					this.match(SqlParser.T__3);
					}
					break;
				}
				}
				break;
			case 5:
				{
				this.state = 1867;
				this.match(SqlParser.INTERVAL);
				this.state = 1868;
				localctx._from_ = this.intervalField();
				this.state = 1869;
				this.match(SqlParser.TO);
				this.state = 1870;
				localctx._to = this.intervalField();
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1878;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 235, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					{
					localctx = new TypeContext(this, _parentctx, _parentState);
					this.pushNewRecursionContext(localctx, _startState, SqlParser.RULE_type);
					this.state = 1874;
					if (!(this.precpred(this._ctx, 6))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 6)");
					}
					this.state = 1875;
					this.match(SqlParser.ARRAY);
					}
					}
				}
				this.state = 1880;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 235, this._ctx);
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
		this.enterRule(localctx, 126, SqlParser.RULE_typeParameter);
		try {
			this.state = 1883;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 244:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1881;
				this.match(SqlParser.INTEGER_VALUE);
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
				this.state = 1882;
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
		this.enterRule(localctx, 128, SqlParser.RULE_baseType);
		try {
			this.state = 1889;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 251:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1885;
				this.match(SqlParser.TIME_WITH_TIME_ZONE);
				}
				break;
			case 252:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1886;
				this.match(SqlParser.TIMESTAMP_WITH_TIME_ZONE);
				}
				break;
			case 253:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1887;
				this.match(SqlParser.DOUBLE_PRECISION);
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
				this.state = 1888;
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
		this.enterRule(localctx, 130, SqlParser.RULE_whenClause);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1891;
			this.match(SqlParser.WHEN);
			this.state = 1892;
			localctx._condition = this.expression();
			this.state = 1893;
			this.match(SqlParser.THEN);
			this.state = 1894;
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
		this.enterRule(localctx, 132, SqlParser.RULE_filter);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1896;
			this.match(SqlParser.FILTER);
			this.state = 1897;
			this.match(SqlParser.T__2);
			this.state = 1898;
			this.match(SqlParser.WHERE);
			this.state = 1899;
			this.booleanExpression(0);
			this.state = 1900;
			this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 134, SqlParser.RULE_over);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1902;
			this.match(SqlParser.OVER);
			this.state = 1903;
			this.match(SqlParser.T__2);
			this.state = 1914;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===146) {
				{
				this.state = 1904;
				this.match(SqlParser.PARTITION);
				this.state = 1905;
				this.match(SqlParser.BY);
				this.state = 1906;
				localctx._expression = this.expression();
				localctx._partition.push(localctx._expression);
				this.state = 1911;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1907;
					this.match(SqlParser.T__0);
					this.state = 1908;
					localctx._expression = this.expression();
					localctx._partition.push(localctx._expression);
					}
					}
					this.state = 1913;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1926;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1916;
				this.match(SqlParser.ORDER);
				this.state = 1917;
				this.match(SqlParser.BY);
				this.state = 1918;
				this.sortItem();
				this.state = 1923;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===1) {
					{
					{
					this.state = 1919;
					this.match(SqlParser.T__0);
					this.state = 1920;
					this.sortItem();
					}
					}
					this.state = 1925;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1929;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===88 || _la===154 || _la===174) {
				{
				this.state = 1928;
				this.windowFrame();
				}
			}

			this.state = 1931;
			this.match(SqlParser.T__3);
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
		this.enterRule(localctx, 136, SqlParser.RULE_windowFrame);
		try {
			this.state = 1957;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 243, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1933;
				localctx._frameType = this.match(SqlParser.RANGE);
				this.state = 1934;
				localctx._start = this.frameBound();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1935;
				localctx._frameType = this.match(SqlParser.ROWS);
				this.state = 1936;
				localctx._start = this.frameBound();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1937;
				localctx._frameType = this.match(SqlParser.GROUPS);
				this.state = 1938;
				localctx._start = this.frameBound();
				}
				break;
			case 4:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1939;
				localctx._frameType = this.match(SqlParser.RANGE);
				this.state = 1940;
				this.match(SqlParser.BETWEEN);
				this.state = 1941;
				localctx._start = this.frameBound();
				this.state = 1942;
				this.match(SqlParser.AND);
				this.state = 1943;
				localctx._end = this.frameBound();
				}
				break;
			case 5:
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1945;
				localctx._frameType = this.match(SqlParser.ROWS);
				this.state = 1946;
				this.match(SqlParser.BETWEEN);
				this.state = 1947;
				localctx._start = this.frameBound();
				this.state = 1948;
				this.match(SqlParser.AND);
				this.state = 1949;
				localctx._end = this.frameBound();
				}
				break;
			case 6:
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1951;
				localctx._frameType = this.match(SqlParser.GROUPS);
				this.state = 1952;
				this.match(SqlParser.BETWEEN);
				this.state = 1953;
				localctx._start = this.frameBound();
				this.state = 1954;
				this.match(SqlParser.AND);
				this.state = 1955;
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
		this.enterRule(localctx, 138, SqlParser.RULE_frameBound);
		let _la: number;
		try {
			this.state = 1968;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 244, this._ctx) ) {
			case 1:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1959;
				this.match(SqlParser.UNBOUNDED);
				this.state = 1960;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlParser.PRECEDING);
				}
				break;
			case 2:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1961;
				this.match(SqlParser.UNBOUNDED);
				this.state = 1962;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlParser.FOLLOWING);
				}
				break;
			case 3:
				localctx = new CurrentRowBoundContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1963;
				this.match(SqlParser.CURRENT);
				this.state = 1964;
				this.match(SqlParser.ROW);
				}
				break;
			case 4:
				localctx = new BoundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1965;
				this.expression();
				this.state = 1966;
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
		this.enterRule(localctx, 140, SqlParser.RULE_updateAssignment);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1970;
			this.identifier();
			this.state = 1971;
			this.match(SqlParser.EQ);
			this.state = 1972;
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
		this.enterRule(localctx, 142, SqlParser.RULE_explainOption);
		let _la: number;
		try {
			this.state = 1978;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 77:
				localctx = new ExplainFormatContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1974;
				this.match(SqlParser.FORMAT);
				this.state = 1975;
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
				this.state = 1976;
				this.match(SqlParser.TYPE);
				this.state = 1977;
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
		this.enterRule(localctx, 144, SqlParser.RULE_transactionMode);
		let _la: number;
		try {
			this.state = 1985;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 104:
				localctx = new IsolationLevelContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1980;
				this.match(SqlParser.ISOLATION);
				this.state = 1981;
				this.match(SqlParser.LEVEL);
				this.state = 1982;
				this.levelOfIsolation();
				}
				break;
			case 155:
				localctx = new TransactionAccessModeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1983;
				this.match(SqlParser.READ);
				this.state = 1984;
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
		this.enterRule(localctx, 146, SqlParser.RULE_levelOfIsolation);
		try {
			this.state = 1994;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 247, this._ctx) ) {
			case 1:
				localctx = new ReadUncommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1987;
				this.match(SqlParser.READ);
				this.state = 1988;
				this.match(SqlParser.UNCOMMITTED);
				}
				break;
			case 2:
				localctx = new ReadCommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1989;
				this.match(SqlParser.READ);
				this.state = 1990;
				this.match(SqlParser.COMMITTED);
				}
				break;
			case 3:
				localctx = new RepeatableReadContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1991;
				this.match(SqlParser.REPEATABLE);
				this.state = 1992;
				this.match(SqlParser.READ);
				}
				break;
			case 4:
				localctx = new SerializableContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1993;
				this.match(SqlParser.SERIALIZABLE);
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
		this.enterRule(localctx, 148, SqlParser.RULE_callArgument);
		try {
			this.state = 2001;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 248, this._ctx) ) {
			case 1:
				localctx = new PositionalArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1996;
				this.expression();
				}
				break;
			case 2:
				localctx = new NamedArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1997;
				this.identifier();
				this.state = 1998;
				this.match(SqlParser.T__8);
				this.state = 1999;
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
		this.enterRule(localctx, 150, SqlParser.RULE_privilege);
		try {
			this.state = 2007;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2003;
				this.match(SqlParser.SELECT);
				}
				break;
			case 51:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2004;
				this.match(SqlParser.DELETE);
				}
				break;
			case 97:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2005;
				this.match(SqlParser.INSERT);
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
				this.state = 2006;
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
		this.enterRule(localctx, 152, SqlParser.RULE_qualifiedName);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2009;
			this.identifier();
			this.state = 2014;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 250, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 2010;
					this.match(SqlParser.T__1);
					this.state = 2011;
					this.identifier();
					}
					}
				}
				this.state = 2016;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 250, this._ctx);
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
		this.enterRule(localctx, 154, SqlParser.RULE_tableVersionExpression);
		let _la: number;
		try {
			localctx = new TableVersionContext(this, localctx);
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2017;
			this.match(SqlParser.FOR);
			this.state = 2018;
			(localctx as TableVersionContext)._tableVersionType = this._input.LT(1);
			_la = this._input.LA(1);
			if(!(((((_la - 191)) & ~0x1F) === 0 && ((1 << (_la - 191)) & 536871427) !== 0))) {
			    (localctx as TableVersionContext)._tableVersionType = this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			this.state = 2019;
			this.tableVersionState();
			this.state = 2020;
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
		this.enterRule(localctx, 156, SqlParser.RULE_tableVersionState);
		try {
			this.state = 2025;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 18:
				localctx = new TableversionasofContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2022;
				this.match(SqlParser.AS);
				this.state = 2023;
				this.match(SqlParser.OF);
				}
				break;
			case 21:
				localctx = new TableversionbeforeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2024;
				this.match(SqlParser.BEFORE);
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
		this.enterRule(localctx, 158, SqlParser.RULE_grantor);
		try {
			this.state = 2030;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 252, this._ctx) ) {
			case 1:
				localctx = new CurrentUserGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2027;
				this.match(SqlParser.CURRENT_USER);
				}
				break;
			case 2:
				localctx = new CurrentRoleGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2028;
				this.match(SqlParser.CURRENT_ROLE);
				}
				break;
			case 3:
				localctx = new SpecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2029;
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
		this.enterRule(localctx, 160, SqlParser.RULE_principal);
		try {
			this.state = 2037;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 253, this._ctx) ) {
			case 1:
				localctx = new UserPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2032;
				this.match(SqlParser.USER);
				this.state = 2033;
				this.identifier();
				}
				break;
			case 2:
				localctx = new RolePrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2034;
				this.match(SqlParser.ROLE);
				this.state = 2035;
				this.identifier();
				}
				break;
			case 3:
				localctx = new UnspecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2036;
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
		this.enterRule(localctx, 162, SqlParser.RULE_roles);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2039;
			this.identifier();
			this.state = 2044;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===1) {
				{
				{
				this.state = 2040;
				this.match(SqlParser.T__0);
				this.state = 2041;
				this.identifier();
				}
				}
				this.state = 2046;
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
		this.enterRule(localctx, 164, SqlParser.RULE_identifier);
		try {
			this.state = 2052;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 247:
				localctx = new UnquotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2047;
				this.match(SqlParser.IDENTIFIER);
				}
				break;
			case 249:
				localctx = new QuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2048;
				this.match(SqlParser.QUOTED_IDENTIFIER);
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
				this.state = 2049;
				this.nonReserved();
				}
				break;
			case 250:
				localctx = new BackQuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 2050;
				this.match(SqlParser.BACKQUOTED_IDENTIFIER);
				}
				break;
			case 248:
				localctx = new DigitIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 2051;
				this.match(SqlParser.DIGIT_IDENTIFIER);
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
		this.enterRule(localctx, 166, SqlParser.RULE_number);
		try {
			this.state = 2057;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 245:
				localctx = new DecimalLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2054;
				this.match(SqlParser.DECIMAL_VALUE);
				}
				break;
			case 246:
				localctx = new DoubleLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2055;
				this.match(SqlParser.DOUBLE_VALUE);
				}
				break;
			case 244:
				localctx = new IntegerLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2056;
				this.match(SqlParser.INTEGER_VALUE);
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
		this.enterRule(localctx, 168, SqlParser.RULE_constraintSpecification);
		try {
			this.state = 2061;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 36:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2059;
				this.namedConstraintSpecification();
				}
				break;
			case 151:
			case 211:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2060;
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
		this.enterRule(localctx, 170, SqlParser.RULE_namedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2063;
			this.match(SqlParser.CONSTRAINT);
			this.state = 2064;
			localctx._name = this.identifier();
			this.state = 2065;
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
		this.enterRule(localctx, 172, SqlParser.RULE_unnamedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2067;
			this.constraintType();
			this.state = 2068;
			this.columnAliases();
			this.state = 2070;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 258, this._ctx) ) {
			case 1:
				{
				this.state = 2069;
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
		this.enterRule(localctx, 174, SqlParser.RULE_constraintType);
		try {
			this.state = 2075;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 211:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2072;
				this.match(SqlParser.UNIQUE);
				}
				break;
			case 151:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2073;
				this.match(SqlParser.PRIMARY);
				this.state = 2074;
				this.match(SqlParser.KEY);
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
		this.enterRule(localctx, 176, SqlParser.RULE_constraintQualifiers);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2080;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (((((_la - 55)) & ~0x1F) === 0 && ((1 << (_la - 55)) & 161) !== 0) || _la===131 || _la===158) {
				{
				{
				this.state = 2077;
				this.constraintQualifier();
				}
				}
				this.state = 2082;
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
		this.enterRule(localctx, 178, SqlParser.RULE_constraintQualifier);
		try {
			this.state = 2086;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 261, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2083;
				this.constraintEnabled();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2084;
				this.constraintRely();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2085;
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
		this.enterRule(localctx, 180, SqlParser.RULE_constraintRely);
		try {
			this.state = 2091;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 158:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2088;
				this.match(SqlParser.RELY);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2089;
				this.match(SqlParser.NOT);
				this.state = 2090;
				this.match(SqlParser.RELY);
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
		this.enterRule(localctx, 182, SqlParser.RULE_constraintEnabled);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2093;
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
		this.enterRule(localctx, 184, SqlParser.RULE_constraintEnforced);
		try {
			this.state = 2098;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 62:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2095;
				this.match(SqlParser.ENFORCED);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2096;
				this.match(SqlParser.NOT);
				this.state = 2097;
				this.match(SqlParser.ENFORCED);
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
		this.enterRule(localctx, 186, SqlParser.RULE_nonReserved);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2100;
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
		case 29:
			return this.queryTerm_sempred(localctx as QueryTermContext, predIndex);
		case 39:
			return this.relation_sempred(localctx as RelationContext, predIndex);
		case 48:
			return this.booleanExpression_sempred(localctx as BooleanExpressionContext, predIndex);
		case 50:
			return this.valueExpression_sempred(localctx as ValueExpressionContext, predIndex);
		case 51:
			return this.primaryExpression_sempred(localctx as PrimaryExpressionContext, predIndex);
		case 62:
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

	public static readonly _serializedATN: number[] = [4,1,258,2103,2,0,7,0,
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
	89,2,90,7,90,2,91,7,91,2,92,7,92,2,93,7,93,1,0,1,0,1,0,5,0,192,8,0,10,0,
	12,0,195,9,0,1,1,1,1,1,1,1,2,1,2,1,2,1,3,1,3,1,3,5,3,206,8,3,10,3,12,3,
	209,9,3,1,4,1,4,1,4,1,5,1,5,1,5,1,6,1,6,1,6,1,7,1,7,1,7,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,236,8,8,1,8,1,8,1,8,3,8,241,8,8,
	1,8,1,8,1,8,1,8,3,8,247,8,8,1,8,1,8,3,8,251,8,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,3,8,265,8,8,1,8,1,8,3,8,269,8,8,1,8,1,8,3,8,273,
	8,8,1,8,1,8,3,8,277,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,285,8,8,1,8,1,8,3,8,
	289,8,8,1,8,3,8,292,8,8,1,8,1,8,1,8,1,8,1,8,3,8,299,8,8,1,8,1,8,1,8,1,8,
	1,8,5,8,306,8,8,10,8,12,8,309,9,8,1,8,1,8,1,8,3,8,314,8,8,1,8,1,8,3,8,318,
	8,8,1,8,1,8,1,8,1,8,3,8,324,8,8,1,8,1,8,1,8,1,8,1,8,3,8,331,8,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,3,8,340,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,349,8,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,360,8,8,1,8,1,8,1,8,1,8,1,8,3,8,
	367,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,377,8,8,1,8,1,8,1,8,1,8,1,8,
	3,8,384,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,392,8,8,1,8,1,8,1,8,1,8,1,8,1,8,
	3,8,400,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,408,8,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,1,8,3,8,418,8,8,1,8,1,8,1,8,1,8,1,8,3,8,425,8,8,1,8,1,8,1,8,1,8,1,8,
	1,8,3,8,433,8,8,1,8,1,8,1,8,3,8,438,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,3,8,449,8,8,1,8,1,8,1,8,3,8,454,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,3,8,465,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,476,8,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,5,8,486,8,8,10,8,12,8,489,9,8,1,8,1,8,1,8,3,8,494,
	8,8,1,8,1,8,1,8,3,8,499,8,8,1,8,1,8,1,8,1,8,3,8,505,8,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,3,8,514,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,525,8,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,534,8,8,1,8,1,8,1,8,3,8,539,8,8,1,8,1,8,
	3,8,543,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,551,8,8,1,8,1,8,1,8,1,8,1,8,3,8,
	558,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,571,8,8,1,8,3,8,
	574,8,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,582,8,8,10,8,12,8,585,9,8,3,8,587,8,
	8,1,8,1,8,1,8,1,8,1,8,3,8,594,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,603,8,
	8,1,8,1,8,1,8,1,8,3,8,609,8,8,1,8,1,8,1,8,3,8,614,8,8,1,8,1,8,3,8,618,8,
	8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,626,8,8,10,8,12,8,629,9,8,3,8,631,8,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,641,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,
	1,8,5,8,652,8,8,10,8,12,8,655,9,8,1,8,1,8,1,8,3,8,660,8,8,1,8,1,8,1,8,3,
	8,665,8,8,1,8,1,8,1,8,1,8,3,8,671,8,8,1,8,1,8,1,8,1,8,1,8,5,8,678,8,8,10,
	8,12,8,681,9,8,1,8,1,8,1,8,3,8,686,8,8,1,8,1,8,1,8,1,8,1,8,3,8,693,8,8,
	1,8,1,8,1,8,1,8,5,8,699,8,8,10,8,12,8,702,9,8,1,8,1,8,3,8,706,8,8,1,8,1,
	8,3,8,710,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,718,8,8,1,8,1,8,1,8,1,8,3,8,724,
	8,8,1,8,1,8,1,8,5,8,729,8,8,10,8,12,8,732,9,8,1,8,1,8,3,8,736,8,8,1,8,1,
	8,3,8,740,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,750,8,8,1,8,3,8,753,8,
	8,1,8,1,8,3,8,757,8,8,1,8,3,8,760,8,8,1,8,1,8,1,8,1,8,5,8,766,8,8,10,8,
	12,8,769,9,8,1,8,1,8,3,8,773,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,798,8,8,1,8,1,
	8,1,8,1,8,3,8,804,8,8,1,8,1,8,1,8,1,8,3,8,810,8,8,3,8,812,8,8,1,8,1,8,1,
	8,1,8,3,8,818,8,8,1,8,1,8,1,8,1,8,3,8,824,8,8,3,8,826,8,8,1,8,1,8,1,8,1,
	8,1,8,1,8,3,8,834,8,8,3,8,836,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,
	8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,855,8,8,1,8,1,8,1,8,3,8,860,8,8,1,8,1,
	8,1,8,1,8,1,8,3,8,867,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,879,
	8,8,3,8,881,8,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,889,8,8,3,8,891,8,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,907,8,8,10,8,12,8,910,
	9,8,3,8,912,8,8,1,8,1,8,3,8,916,8,8,1,8,1,8,3,8,920,8,8,1,8,1,8,1,8,1,8,
	1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,936,8,8,10,8,12,8,939,9,8,3,
	8,941,8,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,5,8,955,8,8,10,
	8,12,8,958,9,8,1,8,1,8,3,8,962,8,8,3,8,964,8,8,1,9,3,9,967,8,9,1,9,1,9,
	1,10,1,10,3,10,973,8,10,1,10,1,10,1,10,5,10,978,8,10,10,10,12,10,981,9,
	10,1,11,1,11,1,11,3,11,986,8,11,1,12,1,12,1,12,1,12,3,12,992,8,12,1,12,
	1,12,3,12,996,8,12,1,12,1,12,3,12,1000,8,12,1,13,1,13,1,13,1,13,3,13,1006,
	8,13,1,14,1,14,1,14,1,14,5,14,1012,8,14,10,14,12,14,1015,9,14,1,14,1,14,
	1,15,1,15,1,15,1,15,1,16,1,16,1,16,1,17,5,17,1027,8,17,10,17,12,17,1030,
	9,17,1,18,1,18,1,18,1,18,3,18,1036,8,18,1,19,5,19,1039,8,19,10,19,12,19,
	1042,9,19,1,20,1,20,1,21,1,21,3,21,1048,8,21,1,22,1,22,1,22,1,23,1,23,1,
	23,3,23,1056,8,23,1,24,1,24,3,24,1060,8,24,1,25,1,25,1,25,3,25,1065,8,25,
	1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,1076,8,26,1,27,1,27,1,
	28,1,28,1,28,1,28,1,28,1,28,5,28,1086,8,28,10,28,12,28,1089,9,28,3,28,1091,
	8,28,1,28,1,28,1,28,3,28,1096,8,28,3,28,1098,8,28,1,28,1,28,1,28,1,28,1,
	28,1,28,1,28,3,28,1107,8,28,3,28,1109,8,28,1,29,1,29,1,29,1,29,1,29,1,29,
	3,29,1117,8,29,1,29,1,29,1,29,1,29,3,29,1123,8,29,1,29,5,29,1126,8,29,10,
	29,12,29,1129,9,29,1,30,1,30,1,30,1,30,1,30,1,30,1,30,5,30,1138,8,30,10,
	30,12,30,1141,9,30,1,30,1,30,1,30,1,30,3,30,1147,8,30,1,31,1,31,3,31,1151,
	8,31,1,31,1,31,3,31,1155,8,31,1,32,1,32,3,32,1159,8,32,1,32,1,32,1,32,5,
	32,1164,8,32,10,32,12,32,1167,9,32,1,32,1,32,1,32,1,32,5,32,1173,8,32,10,
	32,12,32,1176,9,32,3,32,1178,8,32,1,32,1,32,3,32,1182,8,32,1,32,1,32,1,
	32,3,32,1187,8,32,1,32,1,32,3,32,1191,8,32,1,33,3,33,1194,8,33,1,33,1,33,
	1,33,5,33,1199,8,33,10,33,12,33,1202,9,33,1,34,1,34,1,34,1,34,1,34,1,34,
	5,34,1210,8,34,10,34,12,34,1213,9,34,3,34,1215,8,34,1,34,1,34,1,34,1,34,
	1,34,1,34,5,34,1223,8,34,10,34,12,34,1226,9,34,3,34,1228,8,34,1,34,1,34,
	1,34,1,34,1,34,1,34,1,34,5,34,1237,8,34,10,34,12,34,1240,9,34,1,34,1,34,
	3,34,1244,8,34,1,35,1,35,1,35,1,35,5,35,1250,8,35,10,35,12,35,1253,9,35,
	3,35,1255,8,35,1,35,1,35,3,35,1259,8,35,1,36,1,36,3,36,1263,8,36,1,36,1,
	36,1,36,1,36,1,36,1,37,1,37,1,38,1,38,3,38,1274,8,38,1,38,3,38,1277,8,38,
	1,38,1,38,1,38,1,38,1,38,3,38,1284,8,38,1,39,1,39,1,39,1,39,1,39,1,39,1,
	39,1,39,1,39,1,39,1,39,1,39,1,39,1,39,1,39,1,39,1,39,3,39,1303,8,39,5,39,
	1305,8,39,10,39,12,39,1308,9,39,1,40,3,40,1311,8,40,1,40,1,40,3,40,1315,
	8,40,1,40,1,40,3,40,1319,8,40,1,40,1,40,3,40,1323,8,40,3,40,1325,8,40,1,
	41,1,41,1,41,1,41,1,41,1,41,1,41,5,41,1334,8,41,10,41,12,41,1337,9,41,1,
	41,1,41,3,41,1341,8,41,1,42,1,42,1,42,1,42,1,42,1,42,1,42,3,42,1350,8,42,
	1,43,1,43,1,44,1,44,3,44,1356,8,44,1,44,1,44,3,44,1360,8,44,3,44,1362,8,
	44,1,45,1,45,1,45,1,45,5,45,1368,8,45,10,45,12,45,1371,9,45,1,45,1,45,1,
	46,1,46,3,46,1377,8,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,5,46,
	1388,8,46,10,46,12,46,1391,9,46,1,46,1,46,1,46,3,46,1396,8,46,1,46,1,46,
	1,46,1,46,1,46,1,46,1,46,1,46,1,46,3,46,1407,8,46,1,47,1,47,1,48,1,48,1,
	48,3,48,1414,8,48,1,48,1,48,3,48,1418,8,48,1,48,1,48,1,48,1,48,1,48,1,48,
	5,48,1426,8,48,10,48,12,48,1429,9,48,1,49,1,49,1,49,1,49,1,49,1,49,1,49,
	1,49,1,49,1,49,3,49,1441,8,49,1,49,1,49,1,49,1,49,1,49,1,49,3,49,1449,8,
	49,1,49,1,49,1,49,1,49,1,49,5,49,1456,8,49,10,49,12,49,1459,9,49,1,49,1,
	49,1,49,3,49,1464,8,49,1,49,1,49,1,49,1,49,1,49,1,49,3,49,1472,8,49,1,49,
	1,49,1,49,1,49,3,49,1478,8,49,1,49,1,49,3,49,1482,8,49,1,49,1,49,1,49,3,
	49,1487,8,49,1,49,1,49,1,49,3,49,1492,8,49,1,50,1,50,1,50,1,50,3,50,1498,
	8,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,1,50,5,50,1512,
	8,50,10,50,12,50,1515,9,50,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,
	1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,
	51,4,51,1541,8,51,11,51,12,51,1542,1,51,1,51,1,51,1,51,1,51,1,51,1,51,5,
	51,1552,8,51,10,51,12,51,1555,9,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,3,
	51,1564,8,51,1,51,3,51,1567,8,51,1,51,1,51,1,51,3,51,1572,8,51,1,51,1,51,
	1,51,5,51,1577,8,51,10,51,12,51,1580,9,51,3,51,1582,8,51,1,51,1,51,1,51,
	1,51,1,51,5,51,1589,8,51,10,51,12,51,1592,9,51,3,51,1594,8,51,1,51,1,51,
	3,51,1598,8,51,1,51,3,51,1601,8,51,1,51,3,51,1604,8,51,1,51,1,51,1,51,1,
	51,1,51,1,51,1,51,1,51,5,51,1614,8,51,10,51,12,51,1617,9,51,3,51,1619,8,
	51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,
	1,51,4,51,1636,8,51,11,51,12,51,1637,1,51,1,51,3,51,1642,8,51,1,51,1,51,
	1,51,1,51,4,51,1648,8,51,11,51,12,51,1649,1,51,1,51,3,51,1654,8,51,1,51,
	1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,
	51,1,51,1,51,1,51,1,51,1,51,5,51,1677,8,51,10,51,12,51,1680,9,51,3,51,1682,
	8,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,3,51,1691,8,51,1,51,1,51,1,51,1,
	51,3,51,1697,8,51,1,51,1,51,1,51,1,51,3,51,1703,8,51,1,51,1,51,1,51,1,51,
	3,51,1709,8,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,3,51,1719,8,51,1,
	51,1,51,1,51,1,51,1,51,1,51,1,51,3,51,1728,8,51,1,51,1,51,1,51,1,51,1,51,
	1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,5,51,1748,
	8,51,10,51,12,51,1751,9,51,3,51,1753,8,51,1,51,3,51,1756,8,51,1,51,1,51,
	1,51,1,51,1,51,1,51,1,51,1,51,5,51,1766,8,51,10,51,12,51,1769,9,51,1,52,
	1,52,1,52,1,52,3,52,1775,8,52,3,52,1777,8,52,1,53,1,53,1,53,1,53,3,53,1783,
	8,53,1,54,1,54,1,54,1,54,1,54,1,54,3,54,1791,8,54,1,55,1,55,1,56,1,56,1,
	57,1,57,1,58,1,58,3,58,1801,8,58,1,58,1,58,1,58,1,58,3,58,1807,8,58,1,59,
	1,59,1,60,1,60,1,61,1,61,1,61,1,61,5,61,1817,8,61,10,61,12,61,1820,9,61,
	3,61,1822,8,61,1,61,1,61,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,
	62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,5,62,1847,8,62,
	10,62,12,62,1850,9,62,1,62,1,62,1,62,1,62,1,62,1,62,1,62,5,62,1859,8,62,
	10,62,12,62,1862,9,62,1,62,1,62,3,62,1866,8,62,1,62,1,62,1,62,1,62,1,62,
	3,62,1873,8,62,1,62,1,62,5,62,1877,8,62,10,62,12,62,1880,9,62,1,63,1,63,
	3,63,1884,8,63,1,64,1,64,1,64,1,64,3,64,1890,8,64,1,65,1,65,1,65,1,65,1,
	65,1,66,1,66,1,66,1,66,1,66,1,66,1,67,1,67,1,67,1,67,1,67,1,67,1,67,5,67,
	1910,8,67,10,67,12,67,1913,9,67,3,67,1915,8,67,1,67,1,67,1,67,1,67,1,67,
	5,67,1922,8,67,10,67,12,67,1925,9,67,3,67,1927,8,67,1,67,3,67,1930,8,67,
	1,67,1,67,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,
	68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,1,68,3,68,1958,8,68,
	1,69,1,69,1,69,1,69,1,69,1,69,1,69,1,69,1,69,3,69,1969,8,69,1,70,1,70,1,
	70,1,70,1,71,1,71,1,71,1,71,3,71,1979,8,71,1,72,1,72,1,72,1,72,1,72,3,72,
	1986,8,72,1,73,1,73,1,73,1,73,1,73,1,73,1,73,3,73,1995,8,73,1,74,1,74,1,
	74,1,74,1,74,3,74,2002,8,74,1,75,1,75,1,75,1,75,3,75,2008,8,75,1,76,1,76,
	1,76,5,76,2013,8,76,10,76,12,76,2016,9,76,1,77,1,77,1,77,1,77,1,77,1,78,
	1,78,1,78,3,78,2026,8,78,1,79,1,79,1,79,3,79,2031,8,79,1,80,1,80,1,80,1,
	80,1,80,3,80,2038,8,80,1,81,1,81,1,81,5,81,2043,8,81,10,81,12,81,2046,9,
	81,1,82,1,82,1,82,1,82,1,82,3,82,2053,8,82,1,83,1,83,1,83,3,83,2058,8,83,
	1,84,1,84,3,84,2062,8,84,1,85,1,85,1,85,1,85,1,86,1,86,1,86,3,86,2071,8,
	86,1,87,1,87,1,87,3,87,2076,8,87,1,88,5,88,2079,8,88,10,88,12,88,2082,9,
	88,1,89,1,89,1,89,3,89,2087,8,89,1,90,1,90,1,90,3,90,2092,8,90,1,91,1,91,
	1,92,1,92,1,92,3,92,2099,8,92,1,93,1,93,1,93,0,6,58,78,96,100,102,124,94,
	0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,
	52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,
	100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,
	136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,
	172,174,176,178,180,182,184,186,0,25,2,0,27,27,164,164,2,0,50,50,101,101,
	2,0,78,78,93,93,2,0,65,65,94,94,1,0,173,174,2,0,12,12,244,244,2,0,64,64,
	210,210,2,0,19,19,52,52,2,0,74,74,109,109,2,0,12,12,56,56,2,0,22,22,190,
	190,1,0,235,236,1,0,237,239,1,0,229,234,3,0,12,12,16,16,185,185,2,0,71,
	71,203,203,5,0,48,48,90,90,120,121,177,177,227,227,1,0,124,127,2,0,75,75,
	149,149,3,0,85,85,105,105,197,197,4,0,57,57,102,102,117,117,217,217,2,0,
	138,138,226,226,3,0,191,192,200,200,220,220,2,0,55,55,60,60,51,0,10,12,
	14,14,16,17,19,22,25,27,30,35,40,40,42,42,46,48,50,50,52,52,54,55,57,57,
	60,60,62,62,65,65,68,68,70,70,72,75,77,77,80,85,88,88,90,92,94,94,96,96,
	99,99,101,102,104,105,107,110,112,112,114,114,117,122,124,129,133,136,138,
	139,142,142,144,149,151,155,157,167,169,171,173,178,180,192,194,197,199,
	202,204,206,208,209,211,211,213,215,217,217,219,221,225,228,2417,0,188,
	1,0,0,0,2,196,1,0,0,0,4,199,1,0,0,0,6,202,1,0,0,0,8,210,1,0,0,0,10,213,
	1,0,0,0,12,216,1,0,0,0,14,219,1,0,0,0,16,963,1,0,0,0,18,966,1,0,0,0,20,
	970,1,0,0,0,22,985,1,0,0,0,24,987,1,0,0,0,26,1001,1,0,0,0,28,1007,1,0,0,
	0,30,1018,1,0,0,0,32,1022,1,0,0,0,34,1028,1,0,0,0,36,1035,1,0,0,0,38,1040,
	1,0,0,0,40,1043,1,0,0,0,42,1047,1,0,0,0,44,1049,1,0,0,0,46,1052,1,0,0,0,
	48,1059,1,0,0,0,50,1064,1,0,0,0,52,1075,1,0,0,0,54,1077,1,0,0,0,56,1079,
	1,0,0,0,58,1110,1,0,0,0,60,1146,1,0,0,0,62,1148,1,0,0,0,64,1156,1,0,0,0,
	66,1193,1,0,0,0,68,1243,1,0,0,0,70,1258,1,0,0,0,72,1260,1,0,0,0,74,1269,
	1,0,0,0,76,1283,1,0,0,0,78,1285,1,0,0,0,80,1324,1,0,0,0,82,1340,1,0,0,0,
	84,1342,1,0,0,0,86,1351,1,0,0,0,88,1353,1,0,0,0,90,1363,1,0,0,0,92,1406,
	1,0,0,0,94,1408,1,0,0,0,96,1417,1,0,0,0,98,1491,1,0,0,0,100,1497,1,0,0,
	0,102,1755,1,0,0,0,104,1776,1,0,0,0,106,1782,1,0,0,0,108,1790,1,0,0,0,110,
	1792,1,0,0,0,112,1794,1,0,0,0,114,1796,1,0,0,0,116,1798,1,0,0,0,118,1808,
	1,0,0,0,120,1810,1,0,0,0,122,1812,1,0,0,0,124,1872,1,0,0,0,126,1883,1,0,
	0,0,128,1889,1,0,0,0,130,1891,1,0,0,0,132,1896,1,0,0,0,134,1902,1,0,0,0,
	136,1957,1,0,0,0,138,1968,1,0,0,0,140,1970,1,0,0,0,142,1978,1,0,0,0,144,
	1985,1,0,0,0,146,1994,1,0,0,0,148,2001,1,0,0,0,150,2007,1,0,0,0,152,2009,
	1,0,0,0,154,2017,1,0,0,0,156,2025,1,0,0,0,158,2030,1,0,0,0,160,2037,1,0,
	0,0,162,2039,1,0,0,0,164,2052,1,0,0,0,166,2057,1,0,0,0,168,2061,1,0,0,0,
	170,2063,1,0,0,0,172,2067,1,0,0,0,174,2075,1,0,0,0,176,2080,1,0,0,0,178,
	2086,1,0,0,0,180,2091,1,0,0,0,182,2093,1,0,0,0,184,2098,1,0,0,0,186,2100,
	1,0,0,0,188,193,3,76,38,0,189,190,5,1,0,0,190,192,3,76,38,0,191,189,1,0,
	0,0,192,195,1,0,0,0,193,191,1,0,0,0,193,194,1,0,0,0,194,1,1,0,0,0,195,193,
	1,0,0,0,196,197,3,0,0,0,197,198,5,0,0,1,198,3,1,0,0,0,199,200,3,96,48,0,
	200,201,5,0,0,1,201,5,1,0,0,0,202,207,3,62,31,0,203,204,5,1,0,0,204,206,
	3,62,31,0,205,203,1,0,0,0,206,209,1,0,0,0,207,205,1,0,0,0,207,208,1,0,0,
	0,208,7,1,0,0,0,209,207,1,0,0,0,210,211,3,6,3,0,211,212,5,0,0,1,212,9,1,
	0,0,0,213,214,3,16,8,0,214,215,5,0,0,1,215,11,1,0,0,0,216,217,3,94,47,0,
	217,218,5,0,0,1,218,13,1,0,0,0,219,220,3,42,21,0,220,221,5,0,0,1,221,15,
	1,0,0,0,222,964,3,18,9,0,223,224,5,214,0,0,224,964,3,164,82,0,225,226,5,
	214,0,0,226,227,3,164,82,0,227,228,5,2,0,0,228,229,3,164,82,0,229,964,1,
	0,0,0,230,231,5,37,0,0,231,235,5,175,0,0,232,233,5,91,0,0,233,234,5,131,
	0,0,234,236,5,67,0,0,235,232,1,0,0,0,235,236,1,0,0,0,236,237,1,0,0,0,237,
	240,3,152,76,0,238,239,5,224,0,0,239,241,3,28,14,0,240,238,1,0,0,0,240,
	241,1,0,0,0,241,964,1,0,0,0,242,243,5,58,0,0,243,246,5,175,0,0,244,245,
	5,91,0,0,245,247,5,67,0,0,246,244,1,0,0,0,246,247,1,0,0,0,247,248,1,0,0,
	0,248,250,3,152,76,0,249,251,7,0,0,0,250,249,1,0,0,0,250,251,1,0,0,0,251,
	964,1,0,0,0,252,253,5,13,0,0,253,254,5,175,0,0,254,255,3,152,76,0,255,256,
	5,159,0,0,256,257,5,201,0,0,257,258,3,164,82,0,258,964,1,0,0,0,259,260,
	5,37,0,0,260,264,5,193,0,0,261,262,5,91,0,0,262,263,5,131,0,0,263,265,5,
	67,0,0,264,261,1,0,0,0,264,265,1,0,0,0,265,266,1,0,0,0,266,268,3,152,76,
	0,267,269,3,90,45,0,268,267,1,0,0,0,268,269,1,0,0,0,269,272,1,0,0,0,270,
	271,5,33,0,0,271,273,3,104,52,0,272,270,1,0,0,0,272,273,1,0,0,0,273,276,
	1,0,0,0,274,275,5,224,0,0,275,277,3,28,14,0,276,274,1,0,0,0,276,277,1,0,
	0,0,277,278,1,0,0,0,278,284,5,18,0,0,279,285,3,18,9,0,280,281,5,3,0,0,281,
	282,3,18,9,0,282,283,5,4,0,0,283,285,1,0,0,0,284,279,1,0,0,0,284,280,1,
	0,0,0,285,291,1,0,0,0,286,288,5,224,0,0,287,289,5,128,0,0,288,287,1,0,0,
	0,288,289,1,0,0,0,289,290,1,0,0,0,290,292,5,46,0,0,291,286,1,0,0,0,291,
	292,1,0,0,0,292,964,1,0,0,0,293,294,5,37,0,0,294,298,5,193,0,0,295,296,
	5,91,0,0,296,297,5,131,0,0,297,299,5,67,0,0,298,295,1,0,0,0,298,299,1,0,
	0,0,299,300,1,0,0,0,300,301,3,152,76,0,301,302,5,3,0,0,302,307,3,22,11,
	0,303,304,5,1,0,0,304,306,3,22,11,0,305,303,1,0,0,0,306,309,1,0,0,0,307,
	305,1,0,0,0,307,308,1,0,0,0,308,310,1,0,0,0,309,307,1,0,0,0,310,313,5,4,
	0,0,311,312,5,33,0,0,312,314,3,104,52,0,313,311,1,0,0,0,313,314,1,0,0,0,
	314,317,1,0,0,0,315,316,5,224,0,0,316,318,3,28,14,0,317,315,1,0,0,0,317,
	318,1,0,0,0,318,964,1,0,0,0,319,320,5,58,0,0,320,323,5,193,0,0,321,322,
	5,91,0,0,322,324,5,67,0,0,323,321,1,0,0,0,323,324,1,0,0,0,324,325,1,0,0,
	0,325,964,3,152,76,0,326,327,5,97,0,0,327,328,5,100,0,0,328,330,3,152,76,
	0,329,331,3,90,45,0,330,329,1,0,0,0,330,331,1,0,0,0,331,332,1,0,0,0,332,
	333,3,18,9,0,333,964,1,0,0,0,334,335,5,51,0,0,335,336,5,78,0,0,336,339,
	3,152,76,0,337,338,5,223,0,0,338,340,3,96,48,0,339,337,1,0,0,0,339,340,
	1,0,0,0,340,964,1,0,0,0,341,342,5,204,0,0,342,343,5,193,0,0,343,964,3,152,
	76,0,344,345,5,13,0,0,345,348,5,193,0,0,346,347,5,91,0,0,347,349,5,67,0,
	0,348,346,1,0,0,0,348,349,1,0,0,0,349,350,1,0,0,0,350,351,3,152,76,0,351,
	352,5,159,0,0,352,353,5,201,0,0,353,354,3,152,76,0,354,964,1,0,0,0,355,
	356,5,13,0,0,356,359,5,193,0,0,357,358,5,91,0,0,358,360,5,67,0,0,359,357,
	1,0,0,0,359,360,1,0,0,0,360,361,1,0,0,0,361,362,3,152,76,0,362,363,5,159,
	0,0,363,366,5,31,0,0,364,365,5,91,0,0,365,367,5,67,0,0,366,364,1,0,0,0,
	366,367,1,0,0,0,367,368,1,0,0,0,368,369,3,164,82,0,369,370,5,201,0,0,370,
	371,3,164,82,0,371,964,1,0,0,0,372,373,5,13,0,0,373,376,5,193,0,0,374,375,
	5,91,0,0,375,377,5,67,0,0,376,374,1,0,0,0,376,377,1,0,0,0,377,378,1,0,0,
	0,378,379,3,152,76,0,379,380,5,58,0,0,380,383,5,31,0,0,381,382,5,91,0,0,
	382,384,5,67,0,0,383,381,1,0,0,0,383,384,1,0,0,0,384,385,1,0,0,0,385,386,
	3,152,76,0,386,964,1,0,0,0,387,388,5,13,0,0,388,391,5,193,0,0,389,390,5,
	91,0,0,390,392,5,67,0,0,391,389,1,0,0,0,391,392,1,0,0,0,392,393,1,0,0,0,
	393,394,3,152,76,0,394,395,5,10,0,0,395,399,5,31,0,0,396,397,5,91,0,0,397,
	398,5,131,0,0,398,400,5,67,0,0,399,396,1,0,0,0,399,400,1,0,0,0,400,401,
	1,0,0,0,401,402,3,24,12,0,402,964,1,0,0,0,403,404,5,13,0,0,404,407,5,193,
	0,0,405,406,5,91,0,0,406,408,5,67,0,0,407,405,1,0,0,0,407,408,1,0,0,0,408,
	409,1,0,0,0,409,410,3,152,76,0,410,411,5,10,0,0,411,412,3,168,84,0,412,
	964,1,0,0,0,413,414,5,13,0,0,414,417,5,193,0,0,415,416,5,91,0,0,416,418,
	5,67,0,0,417,415,1,0,0,0,417,418,1,0,0,0,418,419,1,0,0,0,419,420,3,152,
	76,0,420,421,5,58,0,0,421,424,5,36,0,0,422,423,5,91,0,0,423,425,5,67,0,
	0,424,422,1,0,0,0,424,425,1,0,0,0,425,426,1,0,0,0,426,427,3,164,82,0,427,
	964,1,0,0,0,428,429,5,13,0,0,429,432,5,193,0,0,430,431,5,91,0,0,431,433,
	5,67,0,0,432,430,1,0,0,0,432,433,1,0,0,0,433,434,1,0,0,0,434,435,3,152,
	76,0,435,437,5,13,0,0,436,438,5,31,0,0,437,436,1,0,0,0,437,438,1,0,0,0,
	438,439,1,0,0,0,439,440,3,164,82,0,440,441,5,182,0,0,441,442,5,131,0,0,
	442,443,5,132,0,0,443,964,1,0,0,0,444,445,5,13,0,0,445,448,5,193,0,0,446,
	447,5,91,0,0,447,449,5,67,0,0,448,446,1,0,0,0,448,449,1,0,0,0,449,450,1,
	0,0,0,450,451,3,152,76,0,451,453,5,13,0,0,452,454,5,31,0,0,453,452,1,0,
	0,0,453,454,1,0,0,0,454,455,1,0,0,0,455,456,3,164,82,0,456,457,5,58,0,0,
	457,458,5,131,0,0,458,459,5,132,0,0,459,964,1,0,0,0,460,461,5,13,0,0,461,
	464,5,193,0,0,462,463,5,91,0,0,463,465,5,67,0,0,464,462,1,0,0,0,464,465,
	1,0,0,0,465,466,1,0,0,0,466,467,3,152,76,0,467,468,5,182,0,0,468,469,5,
	153,0,0,469,470,3,28,14,0,470,964,1,0,0,0,471,472,5,14,0,0,472,475,3,152,
	76,0,473,474,5,224,0,0,474,476,3,28,14,0,475,473,1,0,0,0,475,476,1,0,0,
	0,476,964,1,0,0,0,477,478,5,37,0,0,478,479,5,206,0,0,479,480,3,152,76,0,
	480,493,5,18,0,0,481,482,5,3,0,0,482,487,3,32,16,0,483,484,5,1,0,0,484,
	486,3,32,16,0,485,483,1,0,0,0,486,489,1,0,0,0,487,485,1,0,0,0,487,488,1,
	0,0,0,488,490,1,0,0,0,489,487,1,0,0,0,490,491,5,4,0,0,491,494,1,0,0,0,492,
	494,3,124,62,0,493,481,1,0,0,0,493,492,1,0,0,0,494,964,1,0,0,0,495,498,
	5,37,0,0,496,497,5,140,0,0,497,499,5,161,0,0,498,496,1,0,0,0,498,499,1,
	0,0,0,499,500,1,0,0,0,500,501,5,221,0,0,501,504,3,152,76,0,502,503,5,178,
	0,0,503,505,7,1,0,0,504,502,1,0,0,0,504,505,1,0,0,0,505,506,1,0,0,0,506,
	507,5,18,0,0,507,508,3,18,9,0,508,964,1,0,0,0,509,510,5,13,0,0,510,513,
	5,221,0,0,511,512,5,91,0,0,512,514,5,67,0,0,513,511,1,0,0,0,513,514,1,0,
	0,0,514,515,1,0,0,0,515,516,3,152,76,0,516,517,5,159,0,0,517,518,5,201,
	0,0,518,519,3,152,76,0,519,964,1,0,0,0,520,521,5,58,0,0,521,524,5,221,0,
	0,522,523,5,91,0,0,523,525,5,67,0,0,524,522,1,0,0,0,524,525,1,0,0,0,525,
	526,1,0,0,0,526,964,3,152,76,0,527,528,5,37,0,0,528,529,5,119,0,0,529,533,
	5,221,0,0,530,531,5,91,0,0,531,532,5,131,0,0,532,534,5,67,0,0,533,530,1,
	0,0,0,533,534,1,0,0,0,534,535,1,0,0,0,535,538,3,152,76,0,536,537,5,33,0,
	0,537,539,3,104,52,0,538,536,1,0,0,0,538,539,1,0,0,0,539,542,1,0,0,0,540,
	541,5,224,0,0,541,543,3,28,14,0,542,540,1,0,0,0,542,543,1,0,0,0,543,544,
	1,0,0,0,544,550,5,18,0,0,545,551,3,18,9,0,546,547,5,3,0,0,547,548,3,18,
	9,0,548,549,5,4,0,0,549,551,1,0,0,0,550,545,1,0,0,0,550,546,1,0,0,0,551,
	964,1,0,0,0,552,553,5,58,0,0,553,554,5,119,0,0,554,557,5,221,0,0,555,556,
	5,91,0,0,556,558,5,67,0,0,557,555,1,0,0,0,557,558,1,0,0,0,558,559,1,0,0,
	0,559,964,3,152,76,0,560,561,5,157,0,0,561,562,5,119,0,0,562,563,5,221,
	0,0,563,564,3,152,76,0,564,565,5,223,0,0,565,566,3,96,48,0,566,964,1,0,
	0,0,567,570,5,37,0,0,568,569,5,140,0,0,569,571,5,161,0,0,570,568,1,0,0,
	0,570,571,1,0,0,0,571,573,1,0,0,0,572,574,5,196,0,0,573,572,1,0,0,0,573,
	574,1,0,0,0,574,575,1,0,0,0,575,576,5,80,0,0,576,577,3,152,76,0,577,586,
	5,3,0,0,578,583,3,32,16,0,579,580,5,1,0,0,580,582,3,32,16,0,581,579,1,0,
	0,0,582,585,1,0,0,0,583,581,1,0,0,0,583,584,1,0,0,0,584,587,1,0,0,0,585,
	583,1,0,0,0,586,578,1,0,0,0,586,587,1,0,0,0,587,588,1,0,0,0,588,589,5,4,
	0,0,589,590,5,166,0,0,590,593,3,124,62,0,591,592,5,33,0,0,592,594,3,104,
	52,0,593,591,1,0,0,0,593,594,1,0,0,0,594,595,1,0,0,0,595,596,3,34,17,0,
	596,597,3,42,21,0,597,964,1,0,0,0,598,599,5,13,0,0,599,600,5,80,0,0,600,
	602,3,152,76,0,601,603,3,122,61,0,602,601,1,0,0,0,602,603,1,0,0,0,603,604,
	1,0,0,0,604,605,3,38,19,0,605,964,1,0,0,0,606,608,5,58,0,0,607,609,5,196,
	0,0,608,607,1,0,0,0,608,609,1,0,0,0,609,610,1,0,0,0,610,613,5,80,0,0,611,
	612,5,91,0,0,612,614,5,67,0,0,613,611,1,0,0,0,613,614,1,0,0,0,614,615,1,
	0,0,0,615,617,3,152,76,0,616,618,3,122,61,0,617,616,1,0,0,0,617,618,1,0,
	0,0,618,964,1,0,0,0,619,620,5,25,0,0,620,621,3,152,76,0,621,630,5,3,0,0,
	622,627,3,148,74,0,623,624,5,1,0,0,624,626,3,148,74,0,625,623,1,0,0,0,626,
	629,1,0,0,0,627,625,1,0,0,0,627,628,1,0,0,0,628,631,1,0,0,0,629,627,1,0,
	0,0,630,622,1,0,0,0,630,631,1,0,0,0,631,632,1,0,0,0,632,633,5,4,0,0,633,
	964,1,0,0,0,634,635,5,37,0,0,635,636,5,169,0,0,636,640,3,164,82,0,637,638,
	5,224,0,0,638,639,5,11,0,0,639,641,3,158,79,0,640,637,1,0,0,0,640,641,1,
	0,0,0,641,964,1,0,0,0,642,643,5,58,0,0,643,644,5,169,0,0,644,964,3,164,
	82,0,645,646,5,82,0,0,646,647,3,162,81,0,647,648,5,201,0,0,648,653,3,160,
	80,0,649,650,5,1,0,0,650,652,3,160,80,0,651,649,1,0,0,0,652,655,1,0,0,0,
	653,651,1,0,0,0,653,654,1,0,0,0,654,659,1,0,0,0,655,653,1,0,0,0,656,657,
	5,224,0,0,657,658,5,11,0,0,658,660,5,139,0,0,659,656,1,0,0,0,659,660,1,
	0,0,0,660,664,1,0,0,0,661,662,5,83,0,0,662,663,5,24,0,0,663,665,3,158,79,
	0,664,661,1,0,0,0,664,665,1,0,0,0,665,964,1,0,0,0,666,670,5,167,0,0,667,
	668,5,11,0,0,668,669,5,139,0,0,669,671,5,76,0,0,670,667,1,0,0,0,670,671,
	1,0,0,0,671,672,1,0,0,0,672,673,3,162,81,0,673,674,5,78,0,0,674,679,3,160,
	80,0,675,676,5,1,0,0,676,678,3,160,80,0,677,675,1,0,0,0,678,681,1,0,0,0,
	679,677,1,0,0,0,679,680,1,0,0,0,680,685,1,0,0,0,681,679,1,0,0,0,682,683,
	5,83,0,0,683,684,5,24,0,0,684,686,3,158,79,0,685,682,1,0,0,0,685,686,1,
	0,0,0,686,964,1,0,0,0,687,688,5,182,0,0,688,692,5,169,0,0,689,693,5,12,
	0,0,690,693,5,129,0,0,691,693,3,164,82,0,692,689,1,0,0,0,692,690,1,0,0,
	0,692,691,1,0,0,0,693,964,1,0,0,0,694,705,5,82,0,0,695,700,3,150,75,0,696,
	697,5,1,0,0,697,699,3,150,75,0,698,696,1,0,0,0,699,702,1,0,0,0,700,698,
	1,0,0,0,700,701,1,0,0,0,701,706,1,0,0,0,702,700,1,0,0,0,703,704,5,12,0,
	0,704,706,5,152,0,0,705,695,1,0,0,0,705,703,1,0,0,0,706,707,1,0,0,0,707,
	709,5,137,0,0,708,710,5,193,0,0,709,708,1,0,0,0,709,710,1,0,0,0,710,711,
	1,0,0,0,711,712,3,152,76,0,712,713,5,201,0,0,713,717,3,160,80,0,714,715,
	5,224,0,0,715,716,5,82,0,0,716,718,5,139,0,0,717,714,1,0,0,0,717,718,1,
	0,0,0,718,964,1,0,0,0,719,723,5,167,0,0,720,721,5,82,0,0,721,722,5,139,
	0,0,722,724,5,76,0,0,723,720,1,0,0,0,723,724,1,0,0,0,724,735,1,0,0,0,725,
	730,3,150,75,0,726,727,5,1,0,0,727,729,3,150,75,0,728,726,1,0,0,0,729,732,
	1,0,0,0,730,728,1,0,0,0,730,731,1,0,0,0,731,736,1,0,0,0,732,730,1,0,0,0,
	733,734,5,12,0,0,734,736,5,152,0,0,735,725,1,0,0,0,735,733,1,0,0,0,736,
	737,1,0,0,0,737,739,5,137,0,0,738,740,5,193,0,0,739,738,1,0,0,0,739,740,
	1,0,0,0,740,741,1,0,0,0,741,742,3,152,76,0,742,743,5,78,0,0,743,744,3,160,
	80,0,744,964,1,0,0,0,745,746,5,184,0,0,746,752,5,84,0,0,747,749,5,137,0,
	0,748,750,5,193,0,0,749,748,1,0,0,0,749,750,1,0,0,0,750,751,1,0,0,0,751,
	753,3,152,76,0,752,747,1,0,0,0,752,753,1,0,0,0,753,964,1,0,0,0,754,756,
	5,68,0,0,755,757,5,14,0,0,756,755,1,0,0,0,756,757,1,0,0,0,757,759,1,0,0,
	0,758,760,5,219,0,0,759,758,1,0,0,0,759,760,1,0,0,0,760,772,1,0,0,0,761,
	762,5,3,0,0,762,767,3,142,71,0,763,764,5,1,0,0,764,766,3,142,71,0,765,763,
	1,0,0,0,766,769,1,0,0,0,767,765,1,0,0,0,767,768,1,0,0,0,768,770,1,0,0,0,
	769,767,1,0,0,0,770,771,5,4,0,0,771,773,1,0,0,0,772,761,1,0,0,0,772,773,
	1,0,0,0,773,774,1,0,0,0,774,964,3,16,8,0,775,776,5,184,0,0,776,777,5,37,
	0,0,777,778,5,193,0,0,778,964,3,152,76,0,779,780,5,184,0,0,780,781,5,37,
	0,0,781,782,5,175,0,0,782,964,3,152,76,0,783,784,5,184,0,0,784,785,5,37,
	0,0,785,786,5,221,0,0,786,964,3,152,76,0,787,788,5,184,0,0,788,789,5,37,
	0,0,789,790,5,119,0,0,790,791,5,221,0,0,791,964,3,152,76,0,792,793,5,184,
	0,0,793,794,5,37,0,0,794,795,5,80,0,0,795,797,3,152,76,0,796,798,3,122,
	61,0,797,796,1,0,0,0,797,798,1,0,0,0,798,964,1,0,0,0,799,800,5,184,0,0,
	800,803,5,194,0,0,801,802,7,2,0,0,802,804,3,152,76,0,803,801,1,0,0,0,803,
	804,1,0,0,0,804,811,1,0,0,0,805,806,5,113,0,0,806,809,3,104,52,0,807,808,
	5,63,0,0,808,810,3,104,52,0,809,807,1,0,0,0,809,810,1,0,0,0,810,812,1,0,
	0,0,811,805,1,0,0,0,811,812,1,0,0,0,812,964,1,0,0,0,813,814,5,184,0,0,814,
	817,5,176,0,0,815,816,7,2,0,0,816,818,3,164,82,0,817,815,1,0,0,0,817,818,
	1,0,0,0,818,825,1,0,0,0,819,820,5,113,0,0,820,823,3,104,52,0,821,822,5,
	63,0,0,822,824,3,104,52,0,823,821,1,0,0,0,823,824,1,0,0,0,824,826,1,0,0,
	0,825,819,1,0,0,0,825,826,1,0,0,0,826,964,1,0,0,0,827,828,5,184,0,0,828,
	835,5,30,0,0,829,830,5,113,0,0,830,833,3,104,52,0,831,832,5,63,0,0,832,
	834,3,104,52,0,833,831,1,0,0,0,833,834,1,0,0,0,834,836,1,0,0,0,835,829,
	1,0,0,0,835,836,1,0,0,0,836,964,1,0,0,0,837,838,5,184,0,0,838,839,5,32,
	0,0,839,840,7,2,0,0,840,964,3,152,76,0,841,842,5,184,0,0,842,843,5,188,
	0,0,843,844,5,76,0,0,844,964,3,152,76,0,845,846,5,184,0,0,846,847,5,188,
	0,0,847,848,5,76,0,0,848,849,5,3,0,0,849,850,3,64,32,0,850,851,5,4,0,0,
	851,964,1,0,0,0,852,854,5,184,0,0,853,855,5,40,0,0,854,853,1,0,0,0,854,
	855,1,0,0,0,855,856,1,0,0,0,856,859,5,170,0,0,857,858,7,2,0,0,858,860,3,
	164,82,0,859,857,1,0,0,0,859,860,1,0,0,0,860,964,1,0,0,0,861,862,5,184,
	0,0,862,863,5,169,0,0,863,866,5,84,0,0,864,865,7,2,0,0,865,867,3,164,82,
	0,866,864,1,0,0,0,866,867,1,0,0,0,867,964,1,0,0,0,868,869,5,53,0,0,869,
	964,3,152,76,0,870,871,5,52,0,0,871,964,3,152,76,0,872,873,5,184,0,0,873,
	880,5,81,0,0,874,875,5,113,0,0,875,878,3,104,52,0,876,877,5,63,0,0,877,
	879,3,104,52,0,878,876,1,0,0,0,878,879,1,0,0,0,879,881,1,0,0,0,880,874,
	1,0,0,0,880,881,1,0,0,0,881,964,1,0,0,0,882,883,5,184,0,0,883,890,5,181,
	0,0,884,885,5,113,0,0,885,888,3,104,52,0,886,887,5,63,0,0,887,889,3,104,
	52,0,888,886,1,0,0,0,888,889,1,0,0,0,889,891,1,0,0,0,890,884,1,0,0,0,890,
	891,1,0,0,0,891,964,1,0,0,0,892,893,5,182,0,0,893,894,5,181,0,0,894,895,
	3,152,76,0,895,896,5,229,0,0,896,897,3,94,47,0,897,964,1,0,0,0,898,899,
	5,162,0,0,899,900,5,181,0,0,900,964,3,152,76,0,901,902,5,187,0,0,902,911,
	5,202,0,0,903,908,3,144,72,0,904,905,5,1,0,0,905,907,3,144,72,0,906,904,
	1,0,0,0,907,910,1,0,0,0,908,906,1,0,0,0,908,909,1,0,0,0,909,912,1,0,0,0,
	910,908,1,0,0,0,911,903,1,0,0,0,911,912,1,0,0,0,912,964,1,0,0,0,913,915,
	5,34,0,0,914,916,5,225,0,0,915,914,1,0,0,0,915,916,1,0,0,0,916,964,1,0,
	0,0,917,919,5,171,0,0,918,920,5,225,0,0,919,918,1,0,0,0,919,920,1,0,0,0,
	920,964,1,0,0,0,921,922,5,150,0,0,922,923,3,164,82,0,923,924,5,78,0,0,924,
	925,3,16,8,0,925,964,1,0,0,0,926,927,5,49,0,0,927,928,5,150,0,0,928,964,
	3,164,82,0,929,930,5,66,0,0,930,940,3,164,82,0,931,932,5,216,0,0,932,937,
	3,94,47,0,933,934,5,1,0,0,934,936,3,94,47,0,935,933,1,0,0,0,936,939,1,0,
	0,0,937,935,1,0,0,0,937,938,1,0,0,0,938,941,1,0,0,0,939,937,1,0,0,0,940,
	931,1,0,0,0,940,941,1,0,0,0,941,964,1,0,0,0,942,943,5,53,0,0,943,944,5,
	96,0,0,944,964,3,164,82,0,945,946,5,53,0,0,946,947,5,144,0,0,947,964,3,
	164,82,0,948,949,5,213,0,0,949,950,3,152,76,0,950,951,5,182,0,0,951,956,
	3,140,70,0,952,953,5,1,0,0,953,955,3,140,70,0,954,952,1,0,0,0,955,958,1,
	0,0,0,956,954,1,0,0,0,956,957,1,0,0,0,957,961,1,0,0,0,958,956,1,0,0,0,959,
	960,5,223,0,0,960,962,3,96,48,0,961,959,1,0,0,0,961,962,1,0,0,0,962,964,
	1,0,0,0,963,222,1,0,0,0,963,223,1,0,0,0,963,225,1,0,0,0,963,230,1,0,0,0,
	963,242,1,0,0,0,963,252,1,0,0,0,963,259,1,0,0,0,963,293,1,0,0,0,963,319,
	1,0,0,0,963,326,1,0,0,0,963,334,1,0,0,0,963,341,1,0,0,0,963,344,1,0,0,0,
	963,355,1,0,0,0,963,372,1,0,0,0,963,387,1,0,0,0,963,403,1,0,0,0,963,413,
	1,0,0,0,963,428,1,0,0,0,963,444,1,0,0,0,963,460,1,0,0,0,963,471,1,0,0,0,
	963,477,1,0,0,0,963,495,1,0,0,0,963,509,1,0,0,0,963,520,1,0,0,0,963,527,
	1,0,0,0,963,552,1,0,0,0,963,560,1,0,0,0,963,567,1,0,0,0,963,598,1,0,0,0,
	963,606,1,0,0,0,963,619,1,0,0,0,963,634,1,0,0,0,963,642,1,0,0,0,963,645,
	1,0,0,0,963,666,1,0,0,0,963,687,1,0,0,0,963,694,1,0,0,0,963,719,1,0,0,0,
	963,745,1,0,0,0,963,754,1,0,0,0,963,775,1,0,0,0,963,779,1,0,0,0,963,783,
	1,0,0,0,963,787,1,0,0,0,963,792,1,0,0,0,963,799,1,0,0,0,963,813,1,0,0,0,
	963,827,1,0,0,0,963,837,1,0,0,0,963,841,1,0,0,0,963,845,1,0,0,0,963,852,
	1,0,0,0,963,861,1,0,0,0,963,868,1,0,0,0,963,870,1,0,0,0,963,872,1,0,0,0,
	963,882,1,0,0,0,963,892,1,0,0,0,963,898,1,0,0,0,963,901,1,0,0,0,963,913,
	1,0,0,0,963,917,1,0,0,0,963,921,1,0,0,0,963,926,1,0,0,0,963,929,1,0,0,0,
	963,942,1,0,0,0,963,945,1,0,0,0,963,948,1,0,0,0,964,17,1,0,0,0,965,967,
	3,20,10,0,966,965,1,0,0,0,966,967,1,0,0,0,967,968,1,0,0,0,968,969,3,56,
	28,0,969,19,1,0,0,0,970,972,5,224,0,0,971,973,5,156,0,0,972,971,1,0,0,0,
	972,973,1,0,0,0,973,974,1,0,0,0,974,979,3,72,36,0,975,976,5,1,0,0,976,978,
	3,72,36,0,977,975,1,0,0,0,978,981,1,0,0,0,979,977,1,0,0,0,979,980,1,0,0,
	0,980,21,1,0,0,0,981,979,1,0,0,0,982,986,3,168,84,0,983,986,3,24,12,0,984,
	986,3,26,13,0,985,982,1,0,0,0,985,983,1,0,0,0,985,984,1,0,0,0,986,23,1,
	0,0,0,987,988,3,164,82,0,988,991,3,124,62,0,989,990,5,131,0,0,990,992,5,
	132,0,0,991,989,1,0,0,0,991,992,1,0,0,0,992,995,1,0,0,0,993,994,5,33,0,
	0,994,996,3,104,52,0,995,993,1,0,0,0,995,996,1,0,0,0,996,999,1,0,0,0,997,
	998,5,224,0,0,998,1000,3,28,14,0,999,997,1,0,0,0,999,1000,1,0,0,0,1000,
	25,1,0,0,0,1001,1002,5,113,0,0,1002,1005,3,152,76,0,1003,1004,7,3,0,0,1004,
	1006,5,153,0,0,1005,1003,1,0,0,0,1005,1006,1,0,0,0,1006,27,1,0,0,0,1007,
	1008,5,3,0,0,1008,1013,3,30,15,0,1009,1010,5,1,0,0,1010,1012,3,30,15,0,
	1011,1009,1,0,0,0,1012,1015,1,0,0,0,1013,1011,1,0,0,0,1013,1014,1,0,0,0,
	1014,1016,1,0,0,0,1015,1013,1,0,0,0,1016,1017,5,4,0,0,1017,29,1,0,0,0,1018,
	1019,3,164,82,0,1019,1020,5,229,0,0,1020,1021,3,94,47,0,1021,31,1,0,0,0,
	1022,1023,3,164,82,0,1023,1024,3,124,62,0,1024,33,1,0,0,0,1025,1027,3,36,
	18,0,1026,1025,1,0,0,0,1027,1030,1,0,0,0,1028,1026,1,0,0,0,1028,1029,1,
	0,0,0,1029,35,1,0,0,0,1030,1028,1,0,0,0,1031,1032,5,108,0,0,1032,1036,3,
	48,24,0,1033,1036,3,50,25,0,1034,1036,3,52,26,0,1035,1031,1,0,0,0,1035,
	1033,1,0,0,0,1035,1034,1,0,0,0,1036,37,1,0,0,0,1037,1039,3,40,20,0,1038,
	1037,1,0,0,0,1039,1042,1,0,0,0,1040,1038,1,0,0,0,1040,1041,1,0,0,0,1041,
	39,1,0,0,0,1042,1040,1,0,0,0,1043,1044,3,52,26,0,1044,41,1,0,0,0,1045,1048,
	3,44,22,0,1046,1048,3,46,23,0,1047,1045,1,0,0,0,1047,1046,1,0,0,0,1048,
	43,1,0,0,0,1049,1050,5,165,0,0,1050,1051,3,94,47,0,1051,45,1,0,0,0,1052,
	1055,5,70,0,0,1053,1054,5,122,0,0,1054,1056,3,54,27,0,1055,1053,1,0,0,0,
	1055,1056,1,0,0,0,1056,47,1,0,0,0,1057,1060,5,186,0,0,1058,1060,3,164,82,
	0,1059,1057,1,0,0,0,1059,1058,1,0,0,0,1060,49,1,0,0,0,1061,1065,5,54,0,
	0,1062,1063,5,131,0,0,1063,1065,5,54,0,0,1064,1061,1,0,0,0,1064,1062,1,
	0,0,0,1065,51,1,0,0,0,1066,1067,5,166,0,0,1067,1068,5,132,0,0,1068,1069,
	5,137,0,0,1069,1070,5,132,0,0,1070,1076,5,96,0,0,1071,1072,5,26,0,0,1072,
	1073,5,137,0,0,1073,1074,5,132,0,0,1074,1076,5,96,0,0,1075,1066,1,0,0,0,
	1075,1071,1,0,0,0,1076,53,1,0,0,0,1077,1078,3,164,82,0,1078,55,1,0,0,0,
	1079,1090,3,58,29,0,1080,1081,5,141,0,0,1081,1082,5,24,0,0,1082,1087,3,
	62,31,0,1083,1084,5,1,0,0,1084,1086,3,62,31,0,1085,1083,1,0,0,0,1086,1089,
	1,0,0,0,1087,1085,1,0,0,0,1087,1088,1,0,0,0,1088,1091,1,0,0,0,1089,1087,
	1,0,0,0,1090,1080,1,0,0,0,1090,1091,1,0,0,0,1091,1097,1,0,0,0,1092,1093,
	5,136,0,0,1093,1095,5,244,0,0,1094,1096,7,4,0,0,1095,1094,1,0,0,0,1095,
	1096,1,0,0,0,1096,1098,1,0,0,0,1097,1092,1,0,0,0,1097,1098,1,0,0,0,1098,
	1108,1,0,0,0,1099,1100,5,114,0,0,1100,1107,7,5,0,0,1101,1102,5,72,0,0,1102,
	1103,5,74,0,0,1103,1104,5,244,0,0,1104,1105,5,174,0,0,1105,1107,5,138,0,
	0,1106,1099,1,0,0,0,1106,1101,1,0,0,0,1107,1109,1,0,0,0,1108,1106,1,0,0,
	0,1108,1109,1,0,0,0,1109,57,1,0,0,0,1110,1111,6,29,-1,0,1111,1112,3,60,
	30,0,1112,1127,1,0,0,0,1113,1114,10,2,0,0,1114,1116,5,98,0,0,1115,1117,
	3,74,37,0,1116,1115,1,0,0,0,1116,1117,1,0,0,0,1117,1118,1,0,0,0,1118,1126,
	3,58,29,3,1119,1120,10,1,0,0,1120,1122,7,6,0,0,1121,1123,3,74,37,0,1122,
	1121,1,0,0,0,1122,1123,1,0,0,0,1123,1124,1,0,0,0,1124,1126,3,58,29,2,1125,
	1113,1,0,0,0,1125,1119,1,0,0,0,1126,1129,1,0,0,0,1127,1125,1,0,0,0,1127,
	1128,1,0,0,0,1128,59,1,0,0,0,1129,1127,1,0,0,0,1130,1147,3,64,32,0,1131,
	1132,5,193,0,0,1132,1147,3,152,76,0,1133,1134,5,218,0,0,1134,1139,3,94,
	47,0,1135,1136,5,1,0,0,1136,1138,3,94,47,0,1137,1135,1,0,0,0,1138,1141,
	1,0,0,0,1139,1137,1,0,0,0,1139,1140,1,0,0,0,1140,1147,1,0,0,0,1141,1139,
	1,0,0,0,1142,1143,5,3,0,0,1143,1144,3,56,28,0,1144,1145,5,4,0,0,1145,1147,
	1,0,0,0,1146,1130,1,0,0,0,1146,1131,1,0,0,0,1146,1133,1,0,0,0,1146,1142,
	1,0,0,0,1147,61,1,0,0,0,1148,1150,3,94,47,0,1149,1151,7,7,0,0,1150,1149,
	1,0,0,0,1150,1151,1,0,0,0,1151,1154,1,0,0,0,1152,1153,5,134,0,0,1153,1155,
	7,8,0,0,1154,1152,1,0,0,0,1154,1155,1,0,0,0,1155,63,1,0,0,0,1156,1158,5,
	179,0,0,1157,1159,3,74,37,0,1158,1157,1,0,0,0,1158,1159,1,0,0,0,1159,1160,
	1,0,0,0,1160,1165,3,76,38,0,1161,1162,5,1,0,0,1162,1164,3,76,38,0,1163,
	1161,1,0,0,0,1164,1167,1,0,0,0,1165,1163,1,0,0,0,1165,1166,1,0,0,0,1166,
	1177,1,0,0,0,1167,1165,1,0,0,0,1168,1169,5,78,0,0,1169,1174,3,78,39,0,1170,
	1171,5,1,0,0,1171,1173,3,78,39,0,1172,1170,1,0,0,0,1173,1176,1,0,0,0,1174,
	1172,1,0,0,0,1174,1175,1,0,0,0,1175,1178,1,0,0,0,1176,1174,1,0,0,0,1177,
	1168,1,0,0,0,1177,1178,1,0,0,0,1178,1181,1,0,0,0,1179,1180,5,223,0,0,1180,
	1182,3,96,48,0,1181,1179,1,0,0,0,1181,1182,1,0,0,0,1182,1186,1,0,0,0,1183,
	1184,5,86,0,0,1184,1185,5,24,0,0,1185,1187,3,66,33,0,1186,1183,1,0,0,0,
	1186,1187,1,0,0,0,1187,1190,1,0,0,0,1188,1189,5,89,0,0,1189,1191,3,96,48,
	0,1190,1188,1,0,0,0,1190,1191,1,0,0,0,1191,65,1,0,0,0,1192,1194,3,74,37,
	0,1193,1192,1,0,0,0,1193,1194,1,0,0,0,1194,1195,1,0,0,0,1195,1200,3,68,
	34,0,1196,1197,5,1,0,0,1197,1199,3,68,34,0,1198,1196,1,0,0,0,1199,1202,
	1,0,0,0,1200,1198,1,0,0,0,1200,1201,1,0,0,0,1201,67,1,0,0,0,1202,1200,1,
	0,0,0,1203,1244,3,70,35,0,1204,1205,5,172,0,0,1205,1214,5,3,0,0,1206,1211,
	3,94,47,0,1207,1208,5,1,0,0,1208,1210,3,94,47,0,1209,1207,1,0,0,0,1210,
	1213,1,0,0,0,1211,1209,1,0,0,0,1211,1212,1,0,0,0,1212,1215,1,0,0,0,1213,
	1211,1,0,0,0,1214,1206,1,0,0,0,1214,1215,1,0,0,0,1215,1216,1,0,0,0,1216,
	1244,5,4,0,0,1217,1218,5,39,0,0,1218,1227,5,3,0,0,1219,1224,3,94,47,0,1220,
	1221,5,1,0,0,1221,1223,3,94,47,0,1222,1220,1,0,0,0,1223,1226,1,0,0,0,1224,
	1222,1,0,0,0,1224,1225,1,0,0,0,1225,1228,1,0,0,0,1226,1224,1,0,0,0,1227,
	1219,1,0,0,0,1227,1228,1,0,0,0,1228,1229,1,0,0,0,1229,1244,5,4,0,0,1230,
	1231,5,87,0,0,1231,1232,5,183,0,0,1232,1233,5,3,0,0,1233,1238,3,70,35,0,
	1234,1235,5,1,0,0,1235,1237,3,70,35,0,1236,1234,1,0,0,0,1237,1240,1,0,0,
	0,1238,1236,1,0,0,0,1238,1239,1,0,0,0,1239,1241,1,0,0,0,1240,1238,1,0,0,
	0,1241,1242,5,4,0,0,1242,1244,1,0,0,0,1243,1203,1,0,0,0,1243,1204,1,0,0,
	0,1243,1217,1,0,0,0,1243,1230,1,0,0,0,1244,69,1,0,0,0,1245,1254,5,3,0,0,
	1246,1251,3,94,47,0,1247,1248,5,1,0,0,1248,1250,3,94,47,0,1249,1247,1,0,
	0,0,1250,1253,1,0,0,0,1251,1249,1,0,0,0,1251,1252,1,0,0,0,1252,1255,1,0,
	0,0,1253,1251,1,0,0,0,1254,1246,1,0,0,0,1254,1255,1,0,0,0,1255,1256,1,0,
	0,0,1256,1259,5,4,0,0,1257,1259,3,94,47,0,1258,1245,1,0,0,0,1258,1257,1,
	0,0,0,1259,71,1,0,0,0,1260,1262,3,164,82,0,1261,1263,3,90,45,0,1262,1261,
	1,0,0,0,1262,1263,1,0,0,0,1263,1264,1,0,0,0,1264,1265,5,18,0,0,1265,1266,
	5,3,0,0,1266,1267,3,18,9,0,1267,1268,5,4,0,0,1268,73,1,0,0,0,1269,1270,
	7,9,0,0,1270,75,1,0,0,0,1271,1276,3,94,47,0,1272,1274,5,18,0,0,1273,1272,
	1,0,0,0,1273,1274,1,0,0,0,1274,1275,1,0,0,0,1275,1277,3,164,82,0,1276,1273,
	1,0,0,0,1276,1277,1,0,0,0,1277,1284,1,0,0,0,1278,1279,3,152,76,0,1279,1280,
	5,2,0,0,1280,1281,5,237,0,0,1281,1284,1,0,0,0,1282,1284,5,237,0,0,1283,
	1271,1,0,0,0,1283,1278,1,0,0,0,1283,1282,1,0,0,0,1284,77,1,0,0,0,1285,1286,
	6,39,-1,0,1286,1287,3,84,42,0,1287,1306,1,0,0,0,1288,1302,10,2,0,0,1289,
	1290,5,38,0,0,1290,1291,5,106,0,0,1291,1303,3,84,42,0,1292,1293,3,80,40,
	0,1293,1294,5,106,0,0,1294,1295,3,78,39,0,1295,1296,3,82,41,0,1296,1303,
	1,0,0,0,1297,1298,5,123,0,0,1298,1299,3,80,40,0,1299,1300,5,106,0,0,1300,
	1301,3,84,42,0,1301,1303,1,0,0,0,1302,1289,1,0,0,0,1302,1292,1,0,0,0,1302,
	1297,1,0,0,0,1303,1305,1,0,0,0,1304,1288,1,0,0,0,1305,1308,1,0,0,0,1306,
	1304,1,0,0,0,1306,1307,1,0,0,0,1307,79,1,0,0,0,1308,1306,1,0,0,0,1309,1311,
	5,95,0,0,1310,1309,1,0,0,0,1310,1311,1,0,0,0,1311,1325,1,0,0,0,1312,1314,
	5,111,0,0,1313,1315,5,143,0,0,1314,1313,1,0,0,0,1314,1315,1,0,0,0,1315,
	1325,1,0,0,0,1316,1318,5,168,0,0,1317,1319,5,143,0,0,1318,1317,1,0,0,0,
	1318,1319,1,0,0,0,1319,1325,1,0,0,0,1320,1322,5,79,0,0,1321,1323,5,143,
	0,0,1322,1321,1,0,0,0,1322,1323,1,0,0,0,1323,1325,1,0,0,0,1324,1310,1,0,
	0,0,1324,1312,1,0,0,0,1324,1316,1,0,0,0,1324,1320,1,0,0,0,1325,81,1,0,0,
	0,1326,1327,5,137,0,0,1327,1341,3,96,48,0,1328,1329,5,216,0,0,1329,1330,
	5,3,0,0,1330,1335,3,164,82,0,1331,1332,5,1,0,0,1332,1334,3,164,82,0,1333,
	1331,1,0,0,0,1334,1337,1,0,0,0,1335,1333,1,0,0,0,1335,1336,1,0,0,0,1336,
	1338,1,0,0,0,1337,1335,1,0,0,0,1338,1339,5,4,0,0,1339,1341,1,0,0,0,1340,
	1326,1,0,0,0,1340,1328,1,0,0,0,1341,83,1,0,0,0,1342,1349,3,88,44,0,1343,
	1344,5,195,0,0,1344,1345,3,86,43,0,1345,1346,5,3,0,0,1346,1347,3,94,47,
	0,1347,1348,5,4,0,0,1348,1350,1,0,0,0,1349,1343,1,0,0,0,1349,1350,1,0,0,
	0,1350,85,1,0,0,0,1351,1352,7,10,0,0,1352,87,1,0,0,0,1353,1361,3,92,46,
	0,1354,1356,5,18,0,0,1355,1354,1,0,0,0,1355,1356,1,0,0,0,1356,1357,1,0,
	0,0,1357,1359,3,164,82,0,1358,1360,3,90,45,0,1359,1358,1,0,0,0,1359,1360,
	1,0,0,0,1360,1362,1,0,0,0,1361,1355,1,0,0,0,1361,1362,1,0,0,0,1362,89,1,
	0,0,0,1363,1364,5,3,0,0,1364,1369,3,164,82,0,1365,1366,5,1,0,0,1366,1368,
	3,164,82,0,1367,1365,1,0,0,0,1368,1371,1,0,0,0,1369,1367,1,0,0,0,1369,1370,
	1,0,0,0,1370,1372,1,0,0,0,1371,1369,1,0,0,0,1372,1373,5,4,0,0,1373,91,1,
	0,0,0,1374,1376,3,152,76,0,1375,1377,3,154,77,0,1376,1375,1,0,0,0,1376,
	1377,1,0,0,0,1377,1407,1,0,0,0,1378,1379,5,3,0,0,1379,1380,3,18,9,0,1380,
	1381,5,4,0,0,1381,1407,1,0,0,0,1382,1383,5,212,0,0,1383,1384,5,3,0,0,1384,
	1389,3,94,47,0,1385,1386,5,1,0,0,1386,1388,3,94,47,0,1387,1385,1,0,0,0,
	1388,1391,1,0,0,0,1389,1387,1,0,0,0,1389,1390,1,0,0,0,1390,1392,1,0,0,0,
	1391,1389,1,0,0,0,1392,1395,5,4,0,0,1393,1394,5,224,0,0,1394,1396,5,142,
	0,0,1395,1393,1,0,0,0,1395,1396,1,0,0,0,1396,1407,1,0,0,0,1397,1398,5,110,
	0,0,1398,1399,5,3,0,0,1399,1400,3,18,9,0,1400,1401,5,4,0,0,1401,1407,1,
	0,0,0,1402,1403,5,3,0,0,1403,1404,3,78,39,0,1404,1405,5,4,0,0,1405,1407,
	1,0,0,0,1406,1374,1,0,0,0,1406,1378,1,0,0,0,1406,1382,1,0,0,0,1406,1397,
	1,0,0,0,1406,1402,1,0,0,0,1407,93,1,0,0,0,1408,1409,3,96,48,0,1409,95,1,
	0,0,0,1410,1411,6,48,-1,0,1411,1413,3,100,50,0,1412,1414,3,98,49,0,1413,
	1412,1,0,0,0,1413,1414,1,0,0,0,1414,1418,1,0,0,0,1415,1416,5,131,0,0,1416,
	1418,3,96,48,3,1417,1410,1,0,0,0,1417,1415,1,0,0,0,1418,1427,1,0,0,0,1419,
	1420,10,2,0,0,1420,1421,5,15,0,0,1421,1426,3,96,48,3,1422,1423,10,1,0,0,
	1423,1424,5,140,0,0,1424,1426,3,96,48,2,1425,1419,1,0,0,0,1425,1422,1,0,
	0,0,1426,1429,1,0,0,0,1427,1425,1,0,0,0,1427,1428,1,0,0,0,1428,97,1,0,0,
	0,1429,1427,1,0,0,0,1430,1431,3,110,55,0,1431,1432,3,100,50,0,1432,1492,
	1,0,0,0,1433,1434,3,110,55,0,1434,1435,3,112,56,0,1435,1436,5,3,0,0,1436,
	1437,3,18,9,0,1437,1438,5,4,0,0,1438,1492,1,0,0,0,1439,1441,5,131,0,0,1440,
	1439,1,0,0,0,1440,1441,1,0,0,0,1441,1442,1,0,0,0,1442,1443,5,23,0,0,1443,
	1444,3,100,50,0,1444,1445,5,15,0,0,1445,1446,3,100,50,0,1446,1492,1,0,0,
	0,1447,1449,5,131,0,0,1448,1447,1,0,0,0,1448,1449,1,0,0,0,1449,1450,1,0,
	0,0,1450,1451,5,93,0,0,1451,1452,5,3,0,0,1452,1457,3,94,47,0,1453,1454,
	5,1,0,0,1454,1456,3,94,47,0,1455,1453,1,0,0,0,1456,1459,1,0,0,0,1457,1455,
	1,0,0,0,1457,1458,1,0,0,0,1458,1460,1,0,0,0,1459,1457,1,0,0,0,1460,1461,
	5,4,0,0,1461,1492,1,0,0,0,1462,1464,5,131,0,0,1463,1462,1,0,0,0,1463,1464,
	1,0,0,0,1464,1465,1,0,0,0,1465,1466,5,93,0,0,1466,1467,5,3,0,0,1467,1468,
	3,18,9,0,1468,1469,5,4,0,0,1469,1492,1,0,0,0,1470,1472,5,131,0,0,1471,1470,
	1,0,0,0,1471,1472,1,0,0,0,1472,1473,1,0,0,0,1473,1474,5,113,0,0,1474,1477,
	3,100,50,0,1475,1476,5,63,0,0,1476,1478,3,100,50,0,1477,1475,1,0,0,0,1477,
	1478,1,0,0,0,1478,1492,1,0,0,0,1479,1481,5,103,0,0,1480,1482,5,131,0,0,
	1481,1480,1,0,0,0,1481,1482,1,0,0,0,1482,1483,1,0,0,0,1483,1492,5,132,0,
	0,1484,1486,5,103,0,0,1485,1487,5,131,0,0,1486,1485,1,0,0,0,1486,1487,1,
	0,0,0,1487,1488,1,0,0,0,1488,1489,5,56,0,0,1489,1490,5,78,0,0,1490,1492,
	3,100,50,0,1491,1430,1,0,0,0,1491,1433,1,0,0,0,1491,1440,1,0,0,0,1491,1448,
	1,0,0,0,1491,1463,1,0,0,0,1491,1471,1,0,0,0,1491,1479,1,0,0,0,1491,1484,
	1,0,0,0,1492,99,1,0,0,0,1493,1494,6,50,-1,0,1494,1498,3,102,51,0,1495,1496,
	7,11,0,0,1496,1498,3,100,50,4,1497,1493,1,0,0,0,1497,1495,1,0,0,0,1498,
	1513,1,0,0,0,1499,1500,10,3,0,0,1500,1501,7,12,0,0,1501,1512,3,100,50,4,
	1502,1503,10,2,0,0,1503,1504,7,11,0,0,1504,1512,3,100,50,3,1505,1506,10,
	1,0,0,1506,1507,5,240,0,0,1507,1512,3,100,50,2,1508,1509,10,5,0,0,1509,
	1510,5,20,0,0,1510,1512,3,108,54,0,1511,1499,1,0,0,0,1511,1502,1,0,0,0,
	1511,1505,1,0,0,0,1511,1508,1,0,0,0,1512,1515,1,0,0,0,1513,1511,1,0,0,0,
	1513,1514,1,0,0,0,1514,101,1,0,0,0,1515,1513,1,0,0,0,1516,1517,6,51,-1,
	0,1517,1756,5,132,0,0,1518,1756,3,116,58,0,1519,1520,3,164,82,0,1520,1521,
	3,104,52,0,1521,1756,1,0,0,0,1522,1523,5,253,0,0,1523,1756,3,104,52,0,1524,
	1756,3,166,83,0,1525,1756,3,114,57,0,1526,1756,3,104,52,0,1527,1756,5,243,
	0,0,1528,1756,5,5,0,0,1529,1530,5,148,0,0,1530,1531,5,3,0,0,1531,1532,3,
	100,50,0,1532,1533,5,93,0,0,1533,1534,3,100,50,0,1534,1535,5,4,0,0,1535,
	1756,1,0,0,0,1536,1537,5,3,0,0,1537,1540,3,94,47,0,1538,1539,5,1,0,0,1539,
	1541,3,94,47,0,1540,1538,1,0,0,0,1541,1542,1,0,0,0,1542,1540,1,0,0,0,1542,
	1543,1,0,0,0,1543,1544,1,0,0,0,1544,1545,5,4,0,0,1545,1756,1,0,0,0,1546,
	1547,5,173,0,0,1547,1548,5,3,0,0,1548,1553,3,94,47,0,1549,1550,5,1,0,0,
	1550,1552,3,94,47,0,1551,1549,1,0,0,0,1552,1555,1,0,0,0,1553,1551,1,0,0,
	0,1553,1554,1,0,0,0,1554,1556,1,0,0,0,1555,1553,1,0,0,0,1556,1557,5,4,0,
	0,1557,1756,1,0,0,0,1558,1559,3,152,76,0,1559,1560,5,3,0,0,1560,1561,5,
	237,0,0,1561,1563,5,4,0,0,1562,1564,3,132,66,0,1563,1562,1,0,0,0,1563,1564,
	1,0,0,0,1564,1566,1,0,0,0,1565,1567,3,134,67,0,1566,1565,1,0,0,0,1566,1567,
	1,0,0,0,1567,1756,1,0,0,0,1568,1569,3,152,76,0,1569,1581,5,3,0,0,1570,1572,
	3,74,37,0,1571,1570,1,0,0,0,1571,1572,1,0,0,0,1572,1573,1,0,0,0,1573,1578,
	3,94,47,0,1574,1575,5,1,0,0,1575,1577,3,94,47,0,1576,1574,1,0,0,0,1577,
	1580,1,0,0,0,1578,1576,1,0,0,0,1578,1579,1,0,0,0,1579,1582,1,0,0,0,1580,
	1578,1,0,0,0,1581,1571,1,0,0,0,1581,1582,1,0,0,0,1582,1593,1,0,0,0,1583,
	1584,5,141,0,0,1584,1585,5,24,0,0,1585,1590,3,62,31,0,1586,1587,5,1,0,0,
	1587,1589,3,62,31,0,1588,1586,1,0,0,0,1589,1592,1,0,0,0,1590,1588,1,0,0,
	0,1590,1591,1,0,0,0,1591,1594,1,0,0,0,1592,1590,1,0,0,0,1593,1583,1,0,0,
	0,1593,1594,1,0,0,0,1594,1595,1,0,0,0,1595,1597,5,4,0,0,1596,1598,3,132,
	66,0,1597,1596,1,0,0,0,1597,1598,1,0,0,0,1598,1603,1,0,0,0,1599,1601,3,
	106,53,0,1600,1599,1,0,0,0,1600,1601,1,0,0,0,1601,1602,1,0,0,0,1602,1604,
	3,134,67,0,1603,1600,1,0,0,0,1603,1604,1,0,0,0,1604,1756,1,0,0,0,1605,1606,
	3,164,82,0,1606,1607,5,6,0,0,1607,1608,3,94,47,0,1608,1756,1,0,0,0,1609,
	1618,5,3,0,0,1610,1615,3,164,82,0,1611,1612,5,1,0,0,1612,1614,3,164,82,
	0,1613,1611,1,0,0,0,1614,1617,1,0,0,0,1615,1613,1,0,0,0,1615,1616,1,0,0,
	0,1616,1619,1,0,0,0,1617,1615,1,0,0,0,1618,1610,1,0,0,0,1618,1619,1,0,0,
	0,1619,1620,1,0,0,0,1620,1621,5,4,0,0,1621,1622,5,6,0,0,1622,1756,3,94,
	47,0,1623,1624,5,3,0,0,1624,1625,3,18,9,0,1625,1626,5,4,0,0,1626,1756,1,
	0,0,0,1627,1628,5,67,0,0,1628,1629,5,3,0,0,1629,1630,3,18,9,0,1630,1631,
	5,4,0,0,1631,1756,1,0,0,0,1632,1633,5,28,0,0,1633,1635,3,100,50,0,1634,
	1636,3,130,65,0,1635,1634,1,0,0,0,1636,1637,1,0,0,0,1637,1635,1,0,0,0,1637,
	1638,1,0,0,0,1638,1641,1,0,0,0,1639,1640,5,59,0,0,1640,1642,3,94,47,0,1641,
	1639,1,0,0,0,1641,1642,1,0,0,0,1642,1643,1,0,0,0,1643,1644,5,61,0,0,1644,
	1756,1,0,0,0,1645,1647,5,28,0,0,1646,1648,3,130,65,0,1647,1646,1,0,0,0,
	1648,1649,1,0,0,0,1649,1647,1,0,0,0,1649,1650,1,0,0,0,1650,1653,1,0,0,0,
	1651,1652,5,59,0,0,1652,1654,3,94,47,0,1653,1651,1,0,0,0,1653,1654,1,0,
	0,0,1654,1655,1,0,0,0,1655,1656,5,61,0,0,1656,1756,1,0,0,0,1657,1658,5,
	29,0,0,1658,1659,5,3,0,0,1659,1660,3,94,47,0,1660,1661,5,18,0,0,1661,1662,
	3,124,62,0,1662,1663,5,4,0,0,1663,1756,1,0,0,0,1664,1665,5,205,0,0,1665,
	1666,5,3,0,0,1666,1667,3,94,47,0,1667,1668,5,18,0,0,1668,1669,3,124,62,
	0,1669,1670,5,4,0,0,1670,1756,1,0,0,0,1671,1672,5,17,0,0,1672,1681,5,7,
	0,0,1673,1678,3,94,47,0,1674,1675,5,1,0,0,1675,1677,3,94,47,0,1676,1674,
	1,0,0,0,1677,1680,1,0,0,0,1678,1676,1,0,0,0,1678,1679,1,0,0,0,1679,1682,
	1,0,0,0,1680,1678,1,0,0,0,1681,1673,1,0,0,0,1681,1682,1,0,0,0,1682,1683,
	1,0,0,0,1683,1756,5,8,0,0,1684,1756,3,164,82,0,1685,1756,5,41,0,0,1686,
	1690,5,43,0,0,1687,1688,5,3,0,0,1688,1689,5,244,0,0,1689,1691,5,4,0,0,1690,
	1687,1,0,0,0,1690,1691,1,0,0,0,1691,1756,1,0,0,0,1692,1696,5,44,0,0,1693,
	1694,5,3,0,0,1694,1695,5,244,0,0,1695,1697,5,4,0,0,1696,1693,1,0,0,0,1696,
	1697,1,0,0,0,1697,1756,1,0,0,0,1698,1702,5,115,0,0,1699,1700,5,3,0,0,1700,
	1701,5,244,0,0,1701,1703,5,4,0,0,1702,1699,1,0,0,0,1702,1703,1,0,0,0,1703,
	1756,1,0,0,0,1704,1708,5,116,0,0,1705,1706,5,3,0,0,1706,1707,5,244,0,0,
	1707,1709,5,4,0,0,1708,1705,1,0,0,0,1708,1709,1,0,0,0,1709,1756,1,0,0,0,
	1710,1756,5,45,0,0,1711,1712,5,189,0,0,1712,1713,5,3,0,0,1713,1714,3,100,
	50,0,1714,1715,5,78,0,0,1715,1718,3,100,50,0,1716,1717,5,76,0,0,1717,1719,
	3,100,50,0,1718,1716,1,0,0,0,1718,1719,1,0,0,0,1719,1720,1,0,0,0,1720,1721,
	5,4,0,0,1721,1756,1,0,0,0,1722,1723,5,130,0,0,1723,1724,5,3,0,0,1724,1727,
	3,100,50,0,1725,1726,5,1,0,0,1726,1728,3,120,60,0,1727,1725,1,0,0,0,1727,
	1728,1,0,0,0,1728,1729,1,0,0,0,1729,1730,5,4,0,0,1730,1756,1,0,0,0,1731,
	1732,5,69,0,0,1732,1733,5,3,0,0,1733,1734,3,164,82,0,1734,1735,5,78,0,0,
	1735,1736,3,100,50,0,1736,1737,5,4,0,0,1737,1756,1,0,0,0,1738,1739,5,3,
	0,0,1739,1740,3,94,47,0,1740,1741,5,4,0,0,1741,1756,1,0,0,0,1742,1743,5,
	87,0,0,1743,1752,5,3,0,0,1744,1749,3,152,76,0,1745,1746,5,1,0,0,1746,1748,
	3,152,76,0,1747,1745,1,0,0,0,1748,1751,1,0,0,0,1749,1747,1,0,0,0,1749,1750,
	1,0,0,0,1750,1753,1,0,0,0,1751,1749,1,0,0,0,1752,1744,1,0,0,0,1752,1753,
	1,0,0,0,1753,1754,1,0,0,0,1754,1756,5,4,0,0,1755,1516,1,0,0,0,1755,1518,
	1,0,0,0,1755,1519,1,0,0,0,1755,1522,1,0,0,0,1755,1524,1,0,0,0,1755,1525,
	1,0,0,0,1755,1526,1,0,0,0,1755,1527,1,0,0,0,1755,1528,1,0,0,0,1755,1529,
	1,0,0,0,1755,1536,1,0,0,0,1755,1546,1,0,0,0,1755,1558,1,0,0,0,1755,1568,
	1,0,0,0,1755,1605,1,0,0,0,1755,1609,1,0,0,0,1755,1623,1,0,0,0,1755,1627,
	1,0,0,0,1755,1632,1,0,0,0,1755,1645,1,0,0,0,1755,1657,1,0,0,0,1755,1664,
	1,0,0,0,1755,1671,1,0,0,0,1755,1684,1,0,0,0,1755,1685,1,0,0,0,1755,1686,
	1,0,0,0,1755,1692,1,0,0,0,1755,1698,1,0,0,0,1755,1704,1,0,0,0,1755,1710,
	1,0,0,0,1755,1711,1,0,0,0,1755,1722,1,0,0,0,1755,1731,1,0,0,0,1755,1738,
	1,0,0,0,1755,1742,1,0,0,0,1756,1767,1,0,0,0,1757,1758,10,14,0,0,1758,1759,
	5,7,0,0,1759,1760,3,100,50,0,1760,1761,5,8,0,0,1761,1766,1,0,0,0,1762,1763,
	10,12,0,0,1763,1764,5,2,0,0,1764,1766,3,164,82,0,1765,1757,1,0,0,0,1765,
	1762,1,0,0,0,1766,1769,1,0,0,0,1767,1765,1,0,0,0,1767,1768,1,0,0,0,1768,
	103,1,0,0,0,1769,1767,1,0,0,0,1770,1777,5,241,0,0,1771,1774,5,242,0,0,1772,
	1773,5,207,0,0,1773,1775,5,241,0,0,1774,1772,1,0,0,0,1774,1775,1,0,0,0,
	1775,1777,1,0,0,0,1776,1770,1,0,0,0,1776,1771,1,0,0,0,1777,105,1,0,0,0,
	1778,1779,5,92,0,0,1779,1783,5,134,0,0,1780,1781,5,163,0,0,1781,1783,5,
	134,0,0,1782,1778,1,0,0,0,1782,1780,1,0,0,0,1783,107,1,0,0,0,1784,1785,
	5,199,0,0,1785,1786,5,228,0,0,1786,1791,3,116,58,0,1787,1788,5,199,0,0,
	1788,1789,5,228,0,0,1789,1791,3,104,52,0,1790,1784,1,0,0,0,1790,1787,1,
	0,0,0,1791,109,1,0,0,0,1792,1793,7,13,0,0,1793,111,1,0,0,0,1794,1795,7,
	14,0,0,1795,113,1,0,0,0,1796,1797,7,15,0,0,1797,115,1,0,0,0,1798,1800,5,
	99,0,0,1799,1801,7,11,0,0,1800,1799,1,0,0,0,1800,1801,1,0,0,0,1801,1802,
	1,0,0,0,1802,1803,3,104,52,0,1803,1806,3,118,59,0,1804,1805,5,201,0,0,1805,
	1807,3,118,59,0,1806,1804,1,0,0,0,1806,1807,1,0,0,0,1807,117,1,0,0,0,1808,
	1809,7,16,0,0,1809,119,1,0,0,0,1810,1811,7,17,0,0,1811,121,1,0,0,0,1812,
	1821,5,3,0,0,1813,1818,3,124,62,0,1814,1815,5,1,0,0,1815,1817,3,124,62,
	0,1816,1814,1,0,0,0,1817,1820,1,0,0,0,1818,1816,1,0,0,0,1818,1819,1,0,0,
	0,1819,1822,1,0,0,0,1820,1818,1,0,0,0,1821,1813,1,0,0,0,1821,1822,1,0,0,
	0,1822,1823,1,0,0,0,1823,1824,5,4,0,0,1824,123,1,0,0,0,1825,1826,6,62,-1,
	0,1826,1827,5,17,0,0,1827,1828,5,231,0,0,1828,1829,3,124,62,0,1829,1830,
	5,233,0,0,1830,1873,1,0,0,0,1831,1832,5,118,0,0,1832,1833,5,231,0,0,1833,
	1834,3,124,62,0,1834,1835,5,1,0,0,1835,1836,3,124,62,0,1836,1837,5,233,
	0,0,1837,1873,1,0,0,0,1838,1839,5,173,0,0,1839,1840,5,3,0,0,1840,1841,3,
	164,82,0,1841,1848,3,124,62,0,1842,1843,5,1,0,0,1843,1844,3,164,82,0,1844,
	1845,3,124,62,0,1845,1847,1,0,0,0,1846,1842,1,0,0,0,1847,1850,1,0,0,0,1848,
	1846,1,0,0,0,1848,1849,1,0,0,0,1849,1851,1,0,0,0,1850,1848,1,0,0,0,1851,
	1852,5,4,0,0,1852,1873,1,0,0,0,1853,1865,3,128,64,0,1854,1855,5,3,0,0,1855,
	1860,3,126,63,0,1856,1857,5,1,0,0,1857,1859,3,126,63,0,1858,1856,1,0,0,
	0,1859,1862,1,0,0,0,1860,1858,1,0,0,0,1860,1861,1,0,0,0,1861,1863,1,0,0,
	0,1862,1860,1,0,0,0,1863,1864,5,4,0,0,1864,1866,1,0,0,0,1865,1854,1,0,0,
	0,1865,1866,1,0,0,0,1866,1873,1,0,0,0,1867,1868,5,99,0,0,1868,1869,3,118,
	59,0,1869,1870,5,201,0,0,1870,1871,3,118,59,0,1871,1873,1,0,0,0,1872,1825,
	1,0,0,0,1872,1831,1,0,0,0,1872,1838,1,0,0,0,1872,1853,1,0,0,0,1872,1867,
	1,0,0,0,1873,1878,1,0,0,0,1874,1875,10,6,0,0,1875,1877,5,17,0,0,1876,1874,
	1,0,0,0,1877,1880,1,0,0,0,1878,1876,1,0,0,0,1878,1879,1,0,0,0,1879,125,
	1,0,0,0,1880,1878,1,0,0,0,1881,1884,5,244,0,0,1882,1884,3,124,62,0,1883,
	1881,1,0,0,0,1883,1882,1,0,0,0,1884,127,1,0,0,0,1885,1890,5,251,0,0,1886,
	1890,5,252,0,0,1887,1890,5,253,0,0,1888,1890,3,152,76,0,1889,1885,1,0,0,
	0,1889,1886,1,0,0,0,1889,1887,1,0,0,0,1889,1888,1,0,0,0,1890,129,1,0,0,
	0,1891,1892,5,222,0,0,1892,1893,3,94,47,0,1893,1894,5,198,0,0,1894,1895,
	3,94,47,0,1895,131,1,0,0,0,1896,1897,5,73,0,0,1897,1898,5,3,0,0,1898,1899,
	5,223,0,0,1899,1900,3,96,48,0,1900,1901,5,4,0,0,1901,133,1,0,0,0,1902,1903,
	5,145,0,0,1903,1914,5,3,0,0,1904,1905,5,146,0,0,1905,1906,5,24,0,0,1906,
	1911,3,94,47,0,1907,1908,5,1,0,0,1908,1910,3,94,47,0,1909,1907,1,0,0,0,
	1910,1913,1,0,0,0,1911,1909,1,0,0,0,1911,1912,1,0,0,0,1912,1915,1,0,0,0,
	1913,1911,1,0,0,0,1914,1904,1,0,0,0,1914,1915,1,0,0,0,1915,1926,1,0,0,0,
	1916,1917,5,141,0,0,1917,1918,5,24,0,0,1918,1923,3,62,31,0,1919,1920,5,
	1,0,0,1920,1922,3,62,31,0,1921,1919,1,0,0,0,1922,1925,1,0,0,0,1923,1921,
	1,0,0,0,1923,1924,1,0,0,0,1924,1927,1,0,0,0,1925,1923,1,0,0,0,1926,1916,
	1,0,0,0,1926,1927,1,0,0,0,1927,1929,1,0,0,0,1928,1930,3,136,68,0,1929,1928,
	1,0,0,0,1929,1930,1,0,0,0,1930,1931,1,0,0,0,1931,1932,5,4,0,0,1932,135,
	1,0,0,0,1933,1934,5,154,0,0,1934,1958,3,138,69,0,1935,1936,5,174,0,0,1936,
	1958,3,138,69,0,1937,1938,5,88,0,0,1938,1958,3,138,69,0,1939,1940,5,154,
	0,0,1940,1941,5,23,0,0,1941,1942,3,138,69,0,1942,1943,5,15,0,0,1943,1944,
	3,138,69,0,1944,1958,1,0,0,0,1945,1946,5,174,0,0,1946,1947,5,23,0,0,1947,
	1948,3,138,69,0,1948,1949,5,15,0,0,1949,1950,3,138,69,0,1950,1958,1,0,0,
	0,1951,1952,5,88,0,0,1952,1953,5,23,0,0,1953,1954,3,138,69,0,1954,1955,
	5,15,0,0,1955,1956,3,138,69,0,1956,1958,1,0,0,0,1957,1933,1,0,0,0,1957,
	1935,1,0,0,0,1957,1937,1,0,0,0,1957,1939,1,0,0,0,1957,1945,1,0,0,0,1957,
	1951,1,0,0,0,1958,137,1,0,0,0,1959,1960,5,208,0,0,1960,1969,5,149,0,0,1961,
	1962,5,208,0,0,1962,1969,5,75,0,0,1963,1964,5,40,0,0,1964,1969,5,173,0,
	0,1965,1966,3,94,47,0,1966,1967,7,18,0,0,1967,1969,1,0,0,0,1968,1959,1,
	0,0,0,1968,1961,1,0,0,0,1968,1963,1,0,0,0,1968,1965,1,0,0,0,1969,139,1,
	0,0,0,1970,1971,3,164,82,0,1971,1972,5,229,0,0,1972,1973,3,94,47,0,1973,
	141,1,0,0,0,1974,1975,5,77,0,0,1975,1979,7,19,0,0,1976,1977,5,206,0,0,1977,
	1979,7,20,0,0,1978,1974,1,0,0,0,1978,1976,1,0,0,0,1979,143,1,0,0,0,1980,
	1981,5,104,0,0,1981,1982,5,112,0,0,1982,1986,3,146,73,0,1983,1984,5,155,
	0,0,1984,1986,7,21,0,0,1985,1980,1,0,0,0,1985,1983,1,0,0,0,1986,145,1,0,
	0,0,1987,1988,5,155,0,0,1988,1995,5,209,0,0,1989,1990,5,155,0,0,1990,1995,
	5,35,0,0,1991,1992,5,160,0,0,1992,1995,5,155,0,0,1993,1995,5,180,0,0,1994,
	1987,1,0,0,0,1994,1989,1,0,0,0,1994,1991,1,0,0,0,1994,1993,1,0,0,0,1995,
	147,1,0,0,0,1996,2002,3,94,47,0,1997,1998,3,164,82,0,1998,1999,5,9,0,0,
	1999,2000,3,94,47,0,2000,2002,1,0,0,0,2001,1996,1,0,0,0,2001,1997,1,0,0,
	0,2002,149,1,0,0,0,2003,2008,5,179,0,0,2004,2008,5,51,0,0,2005,2008,5,97,
	0,0,2006,2008,3,164,82,0,2007,2003,1,0,0,0,2007,2004,1,0,0,0,2007,2005,
	1,0,0,0,2007,2006,1,0,0,0,2008,151,1,0,0,0,2009,2014,3,164,82,0,2010,2011,
	5,2,0,0,2011,2013,3,164,82,0,2012,2010,1,0,0,0,2013,2016,1,0,0,0,2014,2012,
	1,0,0,0,2014,2015,1,0,0,0,2015,153,1,0,0,0,2016,2014,1,0,0,0,2017,2018,
	5,76,0,0,2018,2019,7,22,0,0,2019,2020,3,156,78,0,2020,2021,3,100,50,0,2021,
	155,1,0,0,0,2022,2023,5,18,0,0,2023,2026,5,135,0,0,2024,2026,5,21,0,0,2025,
	2022,1,0,0,0,2025,2024,1,0,0,0,2026,157,1,0,0,0,2027,2031,5,45,0,0,2028,
	2031,5,42,0,0,2029,2031,3,160,80,0,2030,2027,1,0,0,0,2030,2028,1,0,0,0,
	2030,2029,1,0,0,0,2031,159,1,0,0,0,2032,2033,5,215,0,0,2033,2038,3,164,
	82,0,2034,2035,5,169,0,0,2035,2038,3,164,82,0,2036,2038,3,164,82,0,2037,
	2032,1,0,0,0,2037,2034,1,0,0,0,2037,2036,1,0,0,0,2038,161,1,0,0,0,2039,
	2044,3,164,82,0,2040,2041,5,1,0,0,2041,2043,3,164,82,0,2042,2040,1,0,0,
	0,2043,2046,1,0,0,0,2044,2042,1,0,0,0,2044,2045,1,0,0,0,2045,163,1,0,0,
	0,2046,2044,1,0,0,0,2047,2053,5,247,0,0,2048,2053,5,249,0,0,2049,2053,3,
	186,93,0,2050,2053,5,250,0,0,2051,2053,5,248,0,0,2052,2047,1,0,0,0,2052,
	2048,1,0,0,0,2052,2049,1,0,0,0,2052,2050,1,0,0,0,2052,2051,1,0,0,0,2053,
	165,1,0,0,0,2054,2058,5,245,0,0,2055,2058,5,246,0,0,2056,2058,5,244,0,0,
	2057,2054,1,0,0,0,2057,2055,1,0,0,0,2057,2056,1,0,0,0,2058,167,1,0,0,0,
	2059,2062,3,170,85,0,2060,2062,3,172,86,0,2061,2059,1,0,0,0,2061,2060,1,
	0,0,0,2062,169,1,0,0,0,2063,2064,5,36,0,0,2064,2065,3,164,82,0,2065,2066,
	3,172,86,0,2066,171,1,0,0,0,2067,2068,3,174,87,0,2068,2070,3,90,45,0,2069,
	2071,3,176,88,0,2070,2069,1,0,0,0,2070,2071,1,0,0,0,2071,173,1,0,0,0,2072,
	2076,5,211,0,0,2073,2074,5,151,0,0,2074,2076,5,107,0,0,2075,2072,1,0,0,
	0,2075,2073,1,0,0,0,2076,175,1,0,0,0,2077,2079,3,178,89,0,2078,2077,1,0,
	0,0,2079,2082,1,0,0,0,2080,2078,1,0,0,0,2080,2081,1,0,0,0,2081,177,1,0,
	0,0,2082,2080,1,0,0,0,2083,2087,3,182,91,0,2084,2087,3,180,90,0,2085,2087,
	3,184,92,0,2086,2083,1,0,0,0,2086,2084,1,0,0,0,2086,2085,1,0,0,0,2087,179,
	1,0,0,0,2088,2092,5,158,0,0,2089,2090,5,131,0,0,2090,2092,5,158,0,0,2091,
	2088,1,0,0,0,2091,2089,1,0,0,0,2092,181,1,0,0,0,2093,2094,7,23,0,0,2094,
	183,1,0,0,0,2095,2099,5,62,0,0,2096,2097,5,131,0,0,2097,2099,5,62,0,0,2098,
	2095,1,0,0,0,2098,2096,1,0,0,0,2099,185,1,0,0,0,2100,2101,7,24,0,0,2101,
	187,1,0,0,0,264,193,207,235,240,246,250,264,268,272,276,284,288,291,298,
	307,313,317,323,330,339,348,359,366,376,383,391,399,407,417,424,432,437,
	448,453,464,475,487,493,498,504,513,524,533,538,542,550,557,570,573,583,
	586,593,602,608,613,617,627,630,640,653,659,664,670,679,685,692,700,705,
	709,717,723,730,735,739,749,752,756,759,767,772,797,803,809,811,817,823,
	825,833,835,854,859,866,878,880,888,890,908,911,915,919,937,940,956,961,
	963,966,972,979,985,991,995,999,1005,1013,1028,1035,1040,1047,1055,1059,
	1064,1075,1087,1090,1095,1097,1106,1108,1116,1122,1125,1127,1139,1146,1150,
	1154,1158,1165,1174,1177,1181,1186,1190,1193,1200,1211,1214,1224,1227,1238,
	1243,1251,1254,1258,1262,1273,1276,1283,1302,1306,1310,1314,1318,1322,1324,
	1335,1340,1349,1355,1359,1361,1369,1376,1389,1395,1406,1413,1417,1425,1427,
	1440,1448,1457,1463,1471,1477,1481,1486,1491,1497,1511,1513,1542,1553,1563,
	1566,1571,1578,1581,1590,1593,1597,1600,1603,1615,1618,1637,1641,1649,1653,
	1678,1681,1690,1696,1702,1708,1718,1727,1749,1752,1755,1765,1767,1774,1776,
	1782,1790,1800,1806,1818,1821,1848,1860,1865,1872,1878,1883,1889,1911,1914,
	1923,1926,1929,1957,1968,1978,1985,1994,2001,2007,2014,2025,2030,2037,2044,
	2052,2057,2061,2070,2075,2080,2086,2091,2098];

	private static __ATN: ATN;
	public static get _ATN(): ATN {
		if (!SqlParser.__ATN) {
			SqlParser.__ATN = new ATNDeserializer().deserialize(SqlParser._serializedATN);
		}

		return SqlParser.__ATN;
	}


	static DecisionsToDFA = SqlParser._ATN.decisionToState.map( (ds: DecisionState, index: number) => new DFA(ds, index) );

}

export class SelectItemListContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_selectItemList;
	}
}


export class StandaloneSelectItemListContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public selectItemList(): SelectItemListContext {
		return this.getTypedRuleContext(SelectItemListContext, 0) as SelectItemListContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_standaloneSelectItemList;
	}
}


export class StandaloneBooleanExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_standaloneBooleanExpression;
	}
}


export class SortItemListContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_sortItemList;
	}
}


export class StandaloneSortItemListContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public sortItemList(): SortItemListContext {
		return this.getTypedRuleContext(SortItemListContext, 0) as SortItemListContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_standaloneSortItemList;
	}
}


export class SingleStatementContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_singleStatement;
	}
}


export class StandaloneExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_standaloneExpression;
	}
}


export class StandaloneRoutineBodyContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public routineBody(): RoutineBodyContext {
		return this.getTypedRuleContext(RoutineBodyContext, 0) as RoutineBodyContext;
	}
	public EOF(): TerminalNode {
		return this.getToken(SqlParser.EOF, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_standaloneRoutineBody;
	}
}


export class StatementContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_statement;
	}
	public override copyFrom(ctx: StatementContext): void {
		super.copyFrom(ctx);
	}
}
export class ExplainContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXPLAIN(): TerminalNode {
		return this.getToken(SqlParser.EXPLAIN, 0);
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlParser.ANALYZE, 0);
	}
	public VERBOSE(): TerminalNode {
		return this.getToken(SqlParser.VERBOSE, 0);
	}
	public explainOption_list(): ExplainOptionContext[] {
		return this.getTypedRuleContexts(ExplainOptionContext) as ExplainOptionContext[];
	}
	public explainOption(i: number): ExplainOptionContext {
		return this.getTypedRuleContext(ExplainOptionContext, i) as ExplainOptionContext;
	}
}
export class PrepareContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public PREPARE(): TerminalNode {
		return this.getToken(SqlParser.PREPARE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public statement(): StatementContext {
		return this.getTypedRuleContext(StatementContext, 0) as StatementContext;
	}
}
export class DropMaterializedViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class UseContext extends StatementContext {
	public _schema!: IdentifierContext;
	public _catalog!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public USE(): TerminalNode {
		return this.getToken(SqlParser.USE, 0);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
}
export class AddConstraintContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlParser.ADD, 0);
	}
	public constraintSpecification(): ConstraintSpecificationContext {
		return this.getTypedRuleContext(ConstraintSpecificationContext, 0) as ConstraintSpecificationContext;
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class DeallocateContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DEALLOCATE(): TerminalNode {
		return this.getToken(SqlParser.DEALLOCATE, 0);
	}
	public PREPARE(): TerminalNode {
		return this.getToken(SqlParser.PREPARE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class RenameTableContext extends StatementContext {
	public _from_!: QualifiedNameContext;
	public _to!: QualifiedNameContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class CommitContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public COMMIT(): TerminalNode {
		return this.getToken(SqlParser.COMMIT, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlParser.WORK, 0);
	}
}
export class CreateRoleContext extends StatementContext {
	public _name!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlParser.ADMIN, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
}
export class ShowCreateFunctionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlParser.FUNCTION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public types(): TypesContext {
		return this.getTypedRuleContext(TypesContext, 0) as TypesContext;
	}
}
export class DropColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: QualifiedNameContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlParser.EXISTS, i);
	}
}
export class DropViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class ShowTablesContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public TABLES(): TerminalNode {
		return this.getToken(SqlParser.TABLES, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class ShowCatalogsContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CATALOGS(): TerminalNode {
		return this.getToken(SqlParser.CATALOGS, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class ShowRolesContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public ROLES(): TerminalNode {
		return this.getToken(SqlParser.ROLES, 0);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlParser.CURRENT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
}
export class RenameColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _from_!: IdentifierContext;
	public _to!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlParser.RENAME, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
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
	    	return this.getTokens(SqlParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlParser.EXISTS, i);
	}
}
export class RevokeRolesContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlParser.REVOKE, 0);
	}
	public roles(): RolesContext {
		return this.getTypedRuleContext(RolesContext, 0) as RolesContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public principal_list(): PrincipalContext[] {
		return this.getTypedRuleContexts(PrincipalContext) as PrincipalContext[];
	}
	public principal(i: number): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, i) as PrincipalContext;
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlParser.ADMIN, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlParser.OPTION, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlParser.GRANTED, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlParser.BY, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
}
export class ShowCreateTableContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class ShowColumnsContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public COLUMNS(): TerminalNode {
		return this.getToken(SqlParser.COLUMNS, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlParser.DESCRIBE, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlParser.DESC, 0);
	}
}
export class ShowRoleGrantsContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlParser.GRANTS, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
}
export class AddColumnContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: ColumnDefinitionContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlParser.ADD, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public columnDefinition(): ColumnDefinitionContext {
		return this.getTypedRuleContext(ColumnDefinitionContext, 0) as ColumnDefinitionContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlParser.EXISTS, i);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class ResetSessionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public RESET(): TerminalNode {
		return this.getToken(SqlParser.RESET, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlParser.SESSION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class DropConstraintContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _name!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public CONSTRAINT(): TerminalNode {
		return this.getToken(SqlParser.CONSTRAINT, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.IF);
	}
	public IF(i: number): TerminalNode {
		return this.getToken(SqlParser.IF, i);
	}
	public EXISTS_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.EXISTS);
	}
	public EXISTS(i: number): TerminalNode {
		return this.getToken(SqlParser.EXISTS, i);
	}
}
export class InsertIntoContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public INSERT(): TerminalNode {
		return this.getToken(SqlParser.INSERT, 0);
	}
	public INTO(): TerminalNode {
		return this.getToken(SqlParser.INTO, 0);
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
}
export class ShowSessionContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlParser.SESSION, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class CreateSchemaContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
}
export class ExecuteContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXECUTE(): TerminalNode {
		return this.getToken(SqlParser.EXECUTE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public USING(): TerminalNode {
		return this.getToken(SqlParser.USING, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
}
export class RenameSchemaContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class DropRoleContext extends StatementContext {
	public _name!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class AnalyzeContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlParser.ANALYZE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
}
export class SetRoleContext extends StatementContext {
	public _role!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlParser.ALL, 0);
	}
	public NONE(): TerminalNode {
		return this.getToken(SqlParser.NONE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class CreateFunctionContext extends StatementContext {
	public _functionName!: QualifiedNameContext;
	public _returnType!: TypeContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlParser.FUNCTION, 0);
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlParser.RETURNS, 0);
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
		return this.getToken(SqlParser.OR, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlParser.REPLACE, 0);
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlParser.TEMPORARY, 0);
	}
	public sqlParameterDeclaration_list(): SqlParameterDeclarationContext[] {
		return this.getTypedRuleContexts(SqlParameterDeclarationContext) as SqlParameterDeclarationContext[];
	}
	public sqlParameterDeclaration(i: number): SqlParameterDeclarationContext {
		return this.getTypedRuleContext(SqlParameterDeclarationContext, i) as SqlParameterDeclarationContext;
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
}
export class ShowGrantsContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlParser.GRANTS, 0);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
}
export class DropSchemaContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public CASCADE(): TerminalNode {
		return this.getToken(SqlParser.CASCADE, 0);
	}
	public RESTRICT(): TerminalNode {
		return this.getToken(SqlParser.RESTRICT, 0);
	}
}
export class ShowCreateViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class CreateTableContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
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
		return this.getToken(SqlParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
}
export class StartTransactionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public START(): TerminalNode {
		return this.getToken(SqlParser.START, 0);
	}
	public TRANSACTION(): TerminalNode {
		return this.getToken(SqlParser.TRANSACTION, 0);
	}
	public transactionMode_list(): TransactionModeContext[] {
		return this.getTypedRuleContexts(TransactionModeContext) as TransactionModeContext[];
	}
	public transactionMode(i: number): TransactionModeContext {
		return this.getTypedRuleContext(TransactionModeContext, i) as TransactionModeContext;
	}
}
export class CreateTableAsSelectContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.WITH);
	}
	public WITH(i: number): TerminalNode {
		return this.getToken(SqlParser.WITH, i);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	public DATA(): TerminalNode {
		return this.getToken(SqlParser.DATA, 0);
	}
	public NO(): TerminalNode {
		return this.getToken(SqlParser.NO, 0);
	}
}
export class ShowStatsContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlParser.STATS, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class ShowCreateSchemaContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlParser.SCHEMA, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class DropFunctionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlParser.FUNCTION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlParser.TEMPORARY, 0);
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public types(): TypesContext {
		return this.getTypedRuleContext(TypesContext, 0) as TypesContext;
	}
}
export class RevokeContext extends StatementContext {
	public _grantee!: PrincipalContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlParser.REVOKE, 0);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
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
		return this.getToken(SqlParser.ALL, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlParser.PRIVILEGES, 0);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlParser.GRANT, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlParser.OPTION, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
}
export class UpdateContext extends StatementContext {
	public _where!: BooleanExpressionContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UPDATE(): TerminalNode {
		return this.getToken(SqlParser.UPDATE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public updateAssignment_list(): UpdateAssignmentContext[] {
		return this.getTypedRuleContexts(UpdateAssignmentContext) as UpdateAssignmentContext[];
	}
	public updateAssignment(i: number): UpdateAssignmentContext {
		return this.getTypedRuleContext(UpdateAssignmentContext, i) as UpdateAssignmentContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
}
export class CreateTypeContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlParser.TYPE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
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
}
export class DeleteContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DELETE(): TerminalNode {
		return this.getToken(SqlParser.DELETE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
}
export class DescribeInputContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlParser.DESCRIBE, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlParser.INPUT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class ShowStatsForQueryContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlParser.STATS, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
	public querySpecification(): QuerySpecificationContext {
		return this.getTypedRuleContext(QuerySpecificationContext, 0) as QuerySpecificationContext;
	}
}
export class StatementDefaultContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
}
export class TruncateTableContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TRUNCATE(): TerminalNode {
		return this.getToken(SqlParser.TRUNCATE, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class AlterColumnSetNotNullContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.ALTER);
	}
	public ALTER(i: number): TerminalNode {
		return this.getToken(SqlParser.ALTER, i);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlParser.NULL, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
}
export class CreateMaterializedViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
}
export class AlterFunctionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlParser.FUNCTION, 0);
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
}
export class SetSessionContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlParser.SESSION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class CreateViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public OR(): TerminalNode {
		return this.getToken(SqlParser.OR, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlParser.REPLACE, 0);
	}
	public SECURITY(): TerminalNode {
		return this.getToken(SqlParser.SECURITY, 0);
	}
	public DEFINER(): TerminalNode {
		return this.getToken(SqlParser.DEFINER, 0);
	}
	public INVOKER(): TerminalNode {
		return this.getToken(SqlParser.INVOKER, 0);
	}
}
export class ShowSchemasContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public SCHEMAS(): TerminalNode {
		return this.getToken(SqlParser.SCHEMAS, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class DropTableContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class RollbackContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLLBACK(): TerminalNode {
		return this.getToken(SqlParser.ROLLBACK, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlParser.WORK, 0);
	}
}
export class RenameViewContext extends StatementContext {
	public _from_!: QualifiedNameContext;
	public _to!: QualifiedNameContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlParser.RENAME, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}
export class AlterColumnDropNotNullContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	public _column!: IdentifierContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.ALTER);
	}
	public ALTER(i: number): TerminalNode {
		return this.getToken(SqlParser.ALTER, i);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public DROP(): TerminalNode {
		return this.getToken(SqlParser.DROP, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlParser.NULL, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
}
export class GrantRolesContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlParser.GRANT, 0);
	}
	public roles(): RolesContext {
		return this.getTypedRuleContext(RolesContext, 0) as RolesContext;
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public principal_list(): PrincipalContext[] {
		return this.getTypedRuleContexts(PrincipalContext) as PrincipalContext[];
	}
	public principal(i: number): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, i) as PrincipalContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlParser.ADMIN, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlParser.OPTION, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlParser.GRANTED, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlParser.BY, 0);
	}
	public grantor(): GrantorContext {
		return this.getTypedRuleContext(GrantorContext, 0) as GrantorContext;
	}
}
export class CallContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CALL(): TerminalNode {
		return this.getToken(SqlParser.CALL, 0);
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
}
export class RefreshMaterializedViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REFRESH(): TerminalNode {
		return this.getToken(SqlParser.REFRESH, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
}
export class ShowCreateMaterializedViewContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public CREATE(): TerminalNode {
		return this.getToken(SqlParser.CREATE, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlParser.MATERIALIZED, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class ShowFunctionsContext extends StatementContext {
	public _pattern!: StringContext;
	public _escape!: StringContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public FUNCTIONS(): TerminalNode {
		return this.getToken(SqlParser.FUNCTIONS, 0);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public string__list(): StringContext[] {
		return this.getTypedRuleContexts(StringContext) as StringContext[];
	}
	public string_(i: number): StringContext {
		return this.getTypedRuleContext(StringContext, i) as StringContext;
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class DescribeOutputContext extends StatementContext {
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DESCRIBE(): TerminalNode {
		return this.getToken(SqlParser.DESCRIBE, 0);
	}
	public OUTPUT(): TerminalNode {
		return this.getToken(SqlParser.OUTPUT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class GrantContext extends StatementContext {
	public _grantee!: PrincipalContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GRANT_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.GRANT);
	}
	public GRANT(i: number): TerminalNode {
		return this.getToken(SqlParser.GRANT, i);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlParser.ON, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
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
		return this.getToken(SqlParser.ALL, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlParser.PRIVILEGES, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlParser.OPTION, 0);
	}
}
export class SetTablePropertiesContext extends StatementContext {
	public _tableName!: QualifiedNameContext;
	constructor(parser: SqlParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ALTER(): TerminalNode {
		return this.getToken(SqlParser.ALTER, 0);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlParser.PROPERTIES, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
}


export class QueryContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_query;
	}
}


export class WithContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public namedQuery_list(): NamedQueryContext[] {
		return this.getTypedRuleContexts(NamedQueryContext) as NamedQueryContext[];
	}
	public namedQuery(i: number): NamedQueryContext {
		return this.getTypedRuleContext(NamedQueryContext, i) as NamedQueryContext;
	}
	public RECURSIVE(): TerminalNode {
		return this.getToken(SqlParser.RECURSIVE, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_with;
	}
}


export class TableElementContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_tableElement;
	}
}


export class ColumnDefinitionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
		return this.getToken(SqlParser.NOT, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlParser.NULL, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public properties(): PropertiesContext {
		return this.getTypedRuleContext(PropertiesContext, 0) as PropertiesContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_columnDefinition;
	}
}


export class LikeClauseContext extends ParserRuleContext {
	public _optionType!: Token;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlParser.PROPERTIES, 0);
	}
	public INCLUDING(): TerminalNode {
		return this.getToken(SqlParser.INCLUDING, 0);
	}
	public EXCLUDING(): TerminalNode {
		return this.getToken(SqlParser.EXCLUDING, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_likeClause;
	}
}


export class PropertiesContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_properties;
	}
}


export class PropertyContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_property;
	}
}


export class SqlParameterDeclarationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_sqlParameterDeclaration;
	}
}


export class RoutineCharacteristicsContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_routineCharacteristics;
	}
}


export class RoutineCharacteristicContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public LANGUAGE(): TerminalNode {
		return this.getToken(SqlParser.LANGUAGE, 0);
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
    	return SqlParser.RULE_routineCharacteristic;
	}
}


export class AlterRoutineCharacteristicsContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_alterRoutineCharacteristics;
	}
}


export class AlterRoutineCharacteristicContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public nullCallClause(): NullCallClauseContext {
		return this.getTypedRuleContext(NullCallClauseContext, 0) as NullCallClauseContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_alterRoutineCharacteristic;
	}
}


export class RoutineBodyContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_routineBody;
	}
}


export class ReturnStatementContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RETURN(): TerminalNode {
		return this.getToken(SqlParser.RETURN, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_returnStatement;
	}
}


export class ExternalBodyReferenceContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public EXTERNAL(): TerminalNode {
		return this.getToken(SqlParser.EXTERNAL, 0);
	}
	public NAME(): TerminalNode {
		return this.getToken(SqlParser.NAME, 0);
	}
	public externalRoutineName(): ExternalRoutineNameContext {
		return this.getTypedRuleContext(ExternalRoutineNameContext, 0) as ExternalRoutineNameContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_externalBodyReference;
	}
}


export class LanguageContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SQL(): TerminalNode {
		return this.getToken(SqlParser.SQL, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_language;
	}
}


export class DeterminismContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public DETERMINISTIC(): TerminalNode {
		return this.getToken(SqlParser.DETERMINISTIC, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_determinism;
	}
}


export class NullCallClauseContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlParser.RETURNS, 0);
	}
	public NULL_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.NULL);
	}
	public NULL(i: number): TerminalNode {
		return this.getToken(SqlParser.NULL, i);
	}
	public ON(): TerminalNode {
		return this.getToken(SqlParser.ON, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlParser.INPUT, 0);
	}
	public CALLED(): TerminalNode {
		return this.getToken(SqlParser.CALLED, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_nullCallClause;
	}
}


export class ExternalRoutineNameContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_externalRoutineName;
	}
}


export class QueryNoWithContext extends ParserRuleContext {
	public _offset!: Token;
	public _limit!: Token;
	public _fetchFirstNRows!: Token;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public queryTerm(): QueryTermContext {
		return this.getTypedRuleContext(QueryTermContext, 0) as QueryTermContext;
	}
	public ORDER(): TerminalNode {
		return this.getToken(SqlParser.ORDER, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlParser.BY, 0);
	}
	public sortItem_list(): SortItemContext[] {
		return this.getTypedRuleContexts(SortItemContext) as SortItemContext[];
	}
	public sortItem(i: number): SortItemContext {
		return this.getTypedRuleContext(SortItemContext, i) as SortItemContext;
	}
	public OFFSET(): TerminalNode {
		return this.getToken(SqlParser.OFFSET, 0);
	}
	public INTEGER_VALUE_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.INTEGER_VALUE);
	}
	public INTEGER_VALUE(i: number): TerminalNode {
		return this.getToken(SqlParser.INTEGER_VALUE, i);
	}
	public LIMIT(): TerminalNode {
		return this.getToken(SqlParser.LIMIT, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlParser.ROW, 0);
	}
	public ROWS_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.ROWS);
	}
	public ROWS(i: number): TerminalNode {
		return this.getToken(SqlParser.ROWS, i);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlParser.ALL, 0);
	}
	public FETCH(): TerminalNode {
		return this.getToken(SqlParser.FETCH, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlParser.FIRST, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlParser.ONLY, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_queryNoWith;
	}
}


export class QueryTermContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_queryTerm;
	}
	public override copyFrom(ctx: QueryTermContext): void {
		super.copyFrom(ctx);
	}
}
export class QueryTermDefaultContext extends QueryTermContext {
	constructor(parser: SqlParser, ctx: QueryTermContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public queryPrimary(): QueryPrimaryContext {
		return this.getTypedRuleContext(QueryPrimaryContext, 0) as QueryPrimaryContext;
	}
}
export class SetOperationContext extends QueryTermContext {
	public _left!: QueryTermContext;
	public _operator!: Token;
	public _right!: QueryTermContext;
	constructor(parser: SqlParser, ctx: QueryTermContext) {
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
		return this.getToken(SqlParser.INTERSECT, 0);
	}
	public setQuantifier(): SetQuantifierContext {
		return this.getTypedRuleContext(SetQuantifierContext, 0) as SetQuantifierContext;
	}
	public UNION(): TerminalNode {
		return this.getToken(SqlParser.UNION, 0);
	}
	public EXCEPT(): TerminalNode {
		return this.getToken(SqlParser.EXCEPT, 0);
	}
}


export class QueryPrimaryContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_queryPrimary;
	}
	public override copyFrom(ctx: QueryPrimaryContext): void {
		super.copyFrom(ctx);
	}
}
export class SubqueryContext extends QueryPrimaryContext {
	constructor(parser: SqlParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public queryNoWith(): QueryNoWithContext {
		return this.getTypedRuleContext(QueryNoWithContext, 0) as QueryNoWithContext;
	}
}
export class QueryPrimaryDefaultContext extends QueryPrimaryContext {
	constructor(parser: SqlParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public querySpecification(): QuerySpecificationContext {
		return this.getTypedRuleContext(QuerySpecificationContext, 0) as QuerySpecificationContext;
	}
}
export class TableContext extends QueryPrimaryContext {
	constructor(parser: SqlParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TABLE(): TerminalNode {
		return this.getToken(SqlParser.TABLE, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
}
export class InlineTableContext extends QueryPrimaryContext {
	constructor(parser: SqlParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public VALUES(): TerminalNode {
		return this.getToken(SqlParser.VALUES, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
}


export class SortItemContext extends ParserRuleContext {
	public _ordering!: Token;
	public _nullOrdering!: Token;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlParser.NULLS, 0);
	}
	public ASC(): TerminalNode {
		return this.getToken(SqlParser.ASC, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlParser.DESC, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlParser.FIRST, 0);
	}
	public LAST(): TerminalNode {
		return this.getToken(SqlParser.LAST, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_sortItem;
	}
}


export class QuerySpecificationContext extends ParserRuleContext {
	public _where!: BooleanExpressionContext;
	public _having!: BooleanExpressionContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SELECT(): TerminalNode {
		return this.getToken(SqlParser.SELECT, 0);
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
		return this.getToken(SqlParser.FROM, 0);
	}
	public relation_list(): RelationContext[] {
		return this.getTypedRuleContexts(RelationContext) as RelationContext[];
	}
	public relation(i: number): RelationContext {
		return this.getTypedRuleContext(RelationContext, i) as RelationContext;
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlParser.WHERE, 0);
	}
	public GROUP(): TerminalNode {
		return this.getToken(SqlParser.GROUP, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlParser.BY, 0);
	}
	public groupBy(): GroupByContext {
		return this.getTypedRuleContext(GroupByContext, 0) as GroupByContext;
	}
	public HAVING(): TerminalNode {
		return this.getToken(SqlParser.HAVING, 0);
	}
	public booleanExpression_list(): BooleanExpressionContext[] {
		return this.getTypedRuleContexts(BooleanExpressionContext) as BooleanExpressionContext[];
	}
	public booleanExpression(i: number): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, i) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_querySpecification;
	}
}


export class GroupByContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_groupBy;
	}
}


export class GroupingElementContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_groupingElement;
	}
	public override copyFrom(ctx: GroupingElementContext): void {
		super.copyFrom(ctx);
	}
}
export class MultipleGroupingSetsContext extends GroupingElementContext {
	constructor(parser: SqlParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GROUPING(): TerminalNode {
		return this.getToken(SqlParser.GROUPING, 0);
	}
	public SETS(): TerminalNode {
		return this.getToken(SqlParser.SETS, 0);
	}
	public groupingSet_list(): GroupingSetContext[] {
		return this.getTypedRuleContexts(GroupingSetContext) as GroupingSetContext[];
	}
	public groupingSet(i: number): GroupingSetContext {
		return this.getTypedRuleContext(GroupingSetContext, i) as GroupingSetContext;
	}
}
export class SingleGroupingSetContext extends GroupingElementContext {
	constructor(parser: SqlParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public groupingSet(): GroupingSetContext {
		return this.getTypedRuleContext(GroupingSetContext, 0) as GroupingSetContext;
	}
}
export class CubeContext extends GroupingElementContext {
	constructor(parser: SqlParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CUBE(): TerminalNode {
		return this.getToken(SqlParser.CUBE, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
}
export class RollupContext extends GroupingElementContext {
	constructor(parser: SqlParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLLUP(): TerminalNode {
		return this.getToken(SqlParser.ROLLUP, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
}


export class GroupingSetContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_groupingSet;
	}
}


export class NamedQueryContext extends ParserRuleContext {
	public _name!: IdentifierContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
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
    	return SqlParser.RULE_namedQuery;
	}
}


export class SetQuantifierContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public DISTINCT(): TerminalNode {
		return this.getToken(SqlParser.DISTINCT, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlParser.ALL, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_setQuantifier;
	}
}


export class SelectItemContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_selectItem;
	}
	public override copyFrom(ctx: SelectItemContext): void {
		super.copyFrom(ctx);
	}
}
export class SelectAllContext extends SelectItemContext {
	constructor(parser: SqlParser, ctx: SelectItemContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public ASTERISK(): TerminalNode {
		return this.getToken(SqlParser.ASTERISK, 0);
	}
}
export class SelectSingleContext extends SelectItemContext {
	constructor(parser: SqlParser, ctx: SelectItemContext) {
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
		return this.getToken(SqlParser.AS, 0);
	}
}


export class RelationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_relation;
	}
	public override copyFrom(ctx: RelationContext): void {
		super.copyFrom(ctx);
	}
}
export class RelationDefaultContext extends RelationContext {
	constructor(parser: SqlParser, ctx: RelationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public sampledRelation(): SampledRelationContext {
		return this.getTypedRuleContext(SampledRelationContext, 0) as SampledRelationContext;
	}
}
export class JoinRelationContext extends RelationContext {
	public _left!: RelationContext;
	public _right!: SampledRelationContext;
	public _rightRelation!: RelationContext;
	constructor(parser: SqlParser, ctx: RelationContext) {
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
		return this.getToken(SqlParser.CROSS, 0);
	}
	public JOIN(): TerminalNode {
		return this.getToken(SqlParser.JOIN, 0);
	}
	public joinType(): JoinTypeContext {
		return this.getTypedRuleContext(JoinTypeContext, 0) as JoinTypeContext;
	}
	public joinCriteria(): JoinCriteriaContext {
		return this.getTypedRuleContext(JoinCriteriaContext, 0) as JoinCriteriaContext;
	}
	public NATURAL(): TerminalNode {
		return this.getToken(SqlParser.NATURAL, 0);
	}
	public sampledRelation(): SampledRelationContext {
		return this.getTypedRuleContext(SampledRelationContext, 0) as SampledRelationContext;
	}
}


export class JoinTypeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INNER(): TerminalNode {
		return this.getToken(SqlParser.INNER, 0);
	}
	public LEFT(): TerminalNode {
		return this.getToken(SqlParser.LEFT, 0);
	}
	public OUTER(): TerminalNode {
		return this.getToken(SqlParser.OUTER, 0);
	}
	public RIGHT(): TerminalNode {
		return this.getToken(SqlParser.RIGHT, 0);
	}
	public FULL(): TerminalNode {
		return this.getToken(SqlParser.FULL, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_joinType;
	}
}


export class JoinCriteriaContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ON(): TerminalNode {
		return this.getToken(SqlParser.ON, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
	public USING(): TerminalNode {
		return this.getToken(SqlParser.USING, 0);
	}
	public identifier_list(): IdentifierContext[] {
		return this.getTypedRuleContexts(IdentifierContext) as IdentifierContext[];
	}
	public identifier(i: number): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, i) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_joinCriteria;
	}
}


export class SampledRelationContext extends ParserRuleContext {
	public _percentage!: ExpressionContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public aliasedRelation(): AliasedRelationContext {
		return this.getTypedRuleContext(AliasedRelationContext, 0) as AliasedRelationContext;
	}
	public TABLESAMPLE(): TerminalNode {
		return this.getToken(SqlParser.TABLESAMPLE, 0);
	}
	public sampleType(): SampleTypeContext {
		return this.getTypedRuleContext(SampleTypeContext, 0) as SampleTypeContext;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_sampledRelation;
	}
}


export class SampleTypeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public BERNOULLI(): TerminalNode {
		return this.getToken(SqlParser.BERNOULLI, 0);
	}
	public SYSTEM(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_sampleType;
	}
}


export class AliasedRelationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
		return this.getToken(SqlParser.AS, 0);
	}
	public columnAliases(): ColumnAliasesContext {
		return this.getTypedRuleContext(ColumnAliasesContext, 0) as ColumnAliasesContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_aliasedRelation;
	}
}


export class ColumnAliasesContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_columnAliases;
	}
}


export class RelationPrimaryContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_relationPrimary;
	}
	public override copyFrom(ctx: RelationPrimaryContext): void {
		super.copyFrom(ctx);
	}
}
export class SubqueryRelationContext extends RelationPrimaryContext {
	constructor(parser: SqlParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
}
export class ParenthesizedRelationContext extends RelationPrimaryContext {
	constructor(parser: SqlParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public relation(): RelationContext {
		return this.getTypedRuleContext(RelationContext, 0) as RelationContext;
	}
}
export class UnnestContext extends RelationPrimaryContext {
	constructor(parser: SqlParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNNEST(): TerminalNode {
		return this.getToken(SqlParser.UNNEST, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public WITH(): TerminalNode {
		return this.getToken(SqlParser.WITH, 0);
	}
	public ORDINALITY(): TerminalNode {
		return this.getToken(SqlParser.ORDINALITY, 0);
	}
}
export class LateralContext extends RelationPrimaryContext {
	constructor(parser: SqlParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public LATERAL(): TerminalNode {
		return this.getToken(SqlParser.LATERAL, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
}
export class TableNameContext extends RelationPrimaryContext {
	constructor(parser: SqlParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public tableVersionExpression(): TableVersionExpressionContext {
		return this.getTypedRuleContext(TableVersionExpressionContext, 0) as TableVersionExpressionContext;
	}
}


export class ExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_expression;
	}
}


export class BooleanExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_booleanExpression;
	}
	public override copyFrom(ctx: BooleanExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class LogicalNotContext extends BooleanExpressionContext {
	constructor(parser: SqlParser, ctx: BooleanExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
}
export class PredicatedContext extends BooleanExpressionContext {
	public _valueExpression!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: BooleanExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public predicate(): PredicateContext {
		return this.getTypedRuleContext(PredicateContext, 0) as PredicateContext;
	}
}
export class LogicalBinaryContext extends BooleanExpressionContext {
	public _left!: BooleanExpressionContext;
	public _operator!: Token;
	public _right!: BooleanExpressionContext;
	constructor(parser: SqlParser, ctx: BooleanExpressionContext) {
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
		return this.getToken(SqlParser.AND, 0);
	}
	public OR(): TerminalNode {
		return this.getToken(SqlParser.OR, 0);
	}
}


export class PredicateContext extends ParserRuleContext {
	public value: ParserRuleContext;
	constructor(parser: SqlParser, parent: ParserRuleContext, invokingState: number, value: ParserRuleContext) {
		super(parent, invokingState);
    	this.parser = parser;
        this.value = value;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_predicate;
	}
	public override copyFrom(ctx: PredicateContext): void {
		super.copyFrom(ctx);
		this.value = ctx.value;
	}
}
export class ComparisonContext extends PredicateContext {
	public _right!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public comparisonOperator(): ComparisonOperatorContext {
		return this.getTypedRuleContext(ComparisonOperatorContext, 0) as ComparisonOperatorContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
}
export class LikeContext extends PredicateContext {
	public _pattern!: ValueExpressionContext;
	public _escape!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public LIKE(): TerminalNode {
		return this.getToken(SqlParser.LIKE, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
	public ESCAPE(): TerminalNode {
		return this.getToken(SqlParser.ESCAPE, 0);
	}
}
export class InSubqueryContext extends PredicateContext {
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class DistinctFromContext extends PredicateContext {
	public _right!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IS(): TerminalNode {
		return this.getToken(SqlParser.IS, 0);
	}
	public DISTINCT(): TerminalNode {
		return this.getToken(SqlParser.DISTINCT, 0);
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class InListContext extends PredicateContext {
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class NullPredicateContext extends PredicateContext {
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public IS(): TerminalNode {
		return this.getToken(SqlParser.IS, 0);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlParser.NULL, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class BetweenContext extends PredicateContext {
	public _lower!: ValueExpressionContext;
	public _upper!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: PredicateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState, ctx.value);
		super.copyFrom(ctx);
	}
	public BETWEEN(): TerminalNode {
		return this.getToken(SqlParser.BETWEEN, 0);
	}
	public AND(): TerminalNode {
		return this.getToken(SqlParser.AND, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
}
export class QuantifiedComparisonContext extends PredicateContext {
	constructor(parser: SqlParser, ctx: PredicateContext) {
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
}


export class ValueExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_valueExpression;
	}
	public override copyFrom(ctx: ValueExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class ValueExpressionDefaultContext extends ValueExpressionContext {
	constructor(parser: SqlParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
}
export class ConcatenationContext extends ValueExpressionContext {
	public _left!: ValueExpressionContext;
	public _right!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CONCAT(): TerminalNode {
		return this.getToken(SqlParser.CONCAT, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
}
export class ArithmeticBinaryContext extends ValueExpressionContext {
	public _left!: ValueExpressionContext;
	public _operator!: Token;
	public _right!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: ValueExpressionContext) {
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
		return this.getToken(SqlParser.ASTERISK, 0);
	}
	public SLASH(): TerminalNode {
		return this.getToken(SqlParser.SLASH, 0);
	}
	public PERCENT(): TerminalNode {
		return this.getToken(SqlParser.PERCENT, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlParser.PLUS, 0);
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlParser.MINUS, 0);
	}
}
export class ArithmeticUnaryContext extends ValueExpressionContext {
	public _operator!: Token;
	constructor(parser: SqlParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlParser.MINUS, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlParser.PLUS, 0);
	}
}
export class AtTimeZoneContext extends ValueExpressionContext {
	constructor(parser: SqlParser, ctx: ValueExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public AT(): TerminalNode {
		return this.getToken(SqlParser.AT, 0);
	}
	public timeZoneSpecifier(): TimeZoneSpecifierContext {
		return this.getTypedRuleContext(TimeZoneSpecifierContext, 0) as TimeZoneSpecifierContext;
	}
}


export class PrimaryExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_primaryExpression;
	}
	public override copyFrom(ctx: PrimaryExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class DereferenceContext extends PrimaryExpressionContext {
	public _base!: PrimaryExpressionContext;
	public _fieldName!: IdentifierContext;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class TypeConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
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
		return this.getToken(SqlParser.DOUBLE_PRECISION, 0);
	}
}
export class SpecialDateTimeFunctionContext extends PrimaryExpressionContext {
	public _name!: Token;
	public _precision!: Token;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_DATE(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_DATE, 0);
	}
	public CURRENT_TIME(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_TIME, 0);
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlParser.INTEGER_VALUE, 0);
	}
	public CURRENT_TIMESTAMP(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_TIMESTAMP, 0);
	}
	public LOCALTIME(): TerminalNode {
		return this.getToken(SqlParser.LOCALTIME, 0);
	}
	public LOCALTIMESTAMP(): TerminalNode {
		return this.getToken(SqlParser.LOCALTIMESTAMP, 0);
	}
}
export class SubstringContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SUBSTRING(): TerminalNode {
		return this.getToken(SqlParser.SUBSTRING, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
}
export class CastContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CAST(): TerminalNode {
		return this.getToken(SqlParser.CAST, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
	public TRY_CAST(): TerminalNode {
		return this.getToken(SqlParser.TRY_CAST, 0);
	}
}
export class LambdaContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
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
}
export class ParenthesizedExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class ParameterContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
}
export class NormalizeContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NORMALIZE(): TerminalNode {
		return this.getToken(SqlParser.NORMALIZE, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public normalForm(): NormalFormContext {
		return this.getTypedRuleContext(NormalFormContext, 0) as NormalFormContext;
	}
}
export class IntervalLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public interval(): IntervalContext {
		return this.getTypedRuleContext(IntervalContext, 0) as IntervalContext;
	}
}
export class NumericLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public number_(): NumberContext {
		return this.getTypedRuleContext(NumberContext, 0) as NumberContext;
	}
}
export class BooleanLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public booleanValue(): BooleanValueContext {
		return this.getTypedRuleContext(BooleanValueContext, 0) as BooleanValueContext;
	}
}
export class SimpleCaseContext extends PrimaryExpressionContext {
	public _elseExpression!: ExpressionContext;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CASE(): TerminalNode {
		return this.getToken(SqlParser.CASE, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public END(): TerminalNode {
		return this.getToken(SqlParser.END, 0);
	}
	public whenClause_list(): WhenClauseContext[] {
		return this.getTypedRuleContexts(WhenClauseContext) as WhenClauseContext[];
	}
	public whenClause(i: number): WhenClauseContext {
		return this.getTypedRuleContext(WhenClauseContext, i) as WhenClauseContext;
	}
	public ELSE(): TerminalNode {
		return this.getToken(SqlParser.ELSE, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class ColumnReferenceContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class NullLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public NULL(): TerminalNode {
		return this.getToken(SqlParser.NULL, 0);
	}
}
export class RowConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
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
		return this.getToken(SqlParser.ROW, 0);
	}
}
export class SubscriptContext extends PrimaryExpressionContext {
	public _value!: PrimaryExpressionContext;
	public _index!: ValueExpressionContext;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public primaryExpression(): PrimaryExpressionContext {
		return this.getTypedRuleContext(PrimaryExpressionContext, 0) as PrimaryExpressionContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
}
export class SubqueryExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
}
export class BinaryLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BINARY_LITERAL(): TerminalNode {
		return this.getToken(SqlParser.BINARY_LITERAL, 0);
	}
}
export class CurrentUserContext extends PrimaryExpressionContext {
	public _name!: Token;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_USER(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_USER, 0);
	}
}
export class ExtractContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXTRACT(): TerminalNode {
		return this.getToken(SqlParser.EXTRACT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public FROM(): TerminalNode {
		return this.getToken(SqlParser.FROM, 0);
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
}
export class StringLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
}
export class ArrayConstructorContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlParser.ARRAY, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
}
export class FunctionCallContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
	public ASTERISK(): TerminalNode {
		return this.getToken(SqlParser.ASTERISK, 0);
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
		return this.getToken(SqlParser.ORDER, 0);
	}
	public BY(): TerminalNode {
		return this.getToken(SqlParser.BY, 0);
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
}
export class ExistsContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public EXISTS(): TerminalNode {
		return this.getToken(SqlParser.EXISTS, 0);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
	}
}
export class PositionContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public POSITION(): TerminalNode {
		return this.getToken(SqlParser.POSITION, 0);
	}
	public valueExpression_list(): ValueExpressionContext[] {
		return this.getTypedRuleContexts(ValueExpressionContext) as ValueExpressionContext[];
	}
	public valueExpression(i: number): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, i) as ValueExpressionContext;
	}
	public IN(): TerminalNode {
		return this.getToken(SqlParser.IN, 0);
	}
}
export class SearchedCaseContext extends PrimaryExpressionContext {
	public _elseExpression!: ExpressionContext;
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CASE(): TerminalNode {
		return this.getToken(SqlParser.CASE, 0);
	}
	public END(): TerminalNode {
		return this.getToken(SqlParser.END, 0);
	}
	public whenClause_list(): WhenClauseContext[] {
		return this.getTypedRuleContexts(WhenClauseContext) as WhenClauseContext[];
	}
	public whenClause(i: number): WhenClauseContext {
		return this.getTypedRuleContext(WhenClauseContext, i) as WhenClauseContext;
	}
	public ELSE(): TerminalNode {
		return this.getToken(SqlParser.ELSE, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class GroupingOperationContext extends PrimaryExpressionContext {
	constructor(parser: SqlParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public GROUPING(): TerminalNode {
		return this.getToken(SqlParser.GROUPING, 0);
	}
	public qualifiedName_list(): QualifiedNameContext[] {
		return this.getTypedRuleContexts(QualifiedNameContext) as QualifiedNameContext[];
	}
	public qualifiedName(i: number): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, i) as QualifiedNameContext;
	}
}


export class StringContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_string;
	}
	public override copyFrom(ctx: StringContext): void {
		super.copyFrom(ctx);
	}
}
export class UnicodeStringLiteralContext extends StringContext {
	constructor(parser: SqlParser, ctx: StringContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNICODE_STRING(): TerminalNode {
		return this.getToken(SqlParser.UNICODE_STRING, 0);
	}
	public UESCAPE(): TerminalNode {
		return this.getToken(SqlParser.UESCAPE, 0);
	}
	public STRING(): TerminalNode {
		return this.getToken(SqlParser.STRING, 0);
	}
}
export class BasicStringLiteralContext extends StringContext {
	constructor(parser: SqlParser, ctx: StringContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public STRING(): TerminalNode {
		return this.getToken(SqlParser.STRING, 0);
	}
}


export class NullTreatmentContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public IGNORE(): TerminalNode {
		return this.getToken(SqlParser.IGNORE, 0);
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlParser.NULLS, 0);
	}
	public RESPECT(): TerminalNode {
		return this.getToken(SqlParser.RESPECT, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_nullTreatment;
	}
}


export class TimeZoneSpecifierContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_timeZoneSpecifier;
	}
	public override copyFrom(ctx: TimeZoneSpecifierContext): void {
		super.copyFrom(ctx);
	}
}
export class TimeZoneIntervalContext extends TimeZoneSpecifierContext {
	constructor(parser: SqlParser, ctx: TimeZoneSpecifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlParser.TIME, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlParser.ZONE, 0);
	}
	public interval(): IntervalContext {
		return this.getTypedRuleContext(IntervalContext, 0) as IntervalContext;
	}
}
export class TimeZoneStringContext extends TimeZoneSpecifierContext {
	constructor(parser: SqlParser, ctx: TimeZoneSpecifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlParser.TIME, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlParser.ZONE, 0);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
	}
}


export class ComparisonOperatorContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlParser.EQ, 0);
	}
	public NEQ(): TerminalNode {
		return this.getToken(SqlParser.NEQ, 0);
	}
	public LT(): TerminalNode {
		return this.getToken(SqlParser.LT, 0);
	}
	public LTE(): TerminalNode {
		return this.getToken(SqlParser.LTE, 0);
	}
	public GT(): TerminalNode {
		return this.getToken(SqlParser.GT, 0);
	}
	public GTE(): TerminalNode {
		return this.getToken(SqlParser.GTE, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_comparisonOperator;
	}
}


export class ComparisonQuantifierContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlParser.ALL, 0);
	}
	public SOME(): TerminalNode {
		return this.getToken(SqlParser.SOME, 0);
	}
	public ANY(): TerminalNode {
		return this.getToken(SqlParser.ANY, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_comparisonQuantifier;
	}
}


export class BooleanValueContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public TRUE(): TerminalNode {
		return this.getToken(SqlParser.TRUE, 0);
	}
	public FALSE(): TerminalNode {
		return this.getToken(SqlParser.FALSE, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_booleanValue;
	}
}


export class IntervalContext extends ParserRuleContext {
	public _sign!: Token;
	public _from_!: IntervalFieldContext;
	public _to!: IntervalFieldContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INTERVAL(): TerminalNode {
		return this.getToken(SqlParser.INTERVAL, 0);
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
		return this.getToken(SqlParser.TO, 0);
	}
	public PLUS(): TerminalNode {
		return this.getToken(SqlParser.PLUS, 0);
	}
	public MINUS(): TerminalNode {
		return this.getToken(SqlParser.MINUS, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_interval;
	}
}


export class IntervalFieldContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public YEAR(): TerminalNode {
		return this.getToken(SqlParser.YEAR, 0);
	}
	public MONTH(): TerminalNode {
		return this.getToken(SqlParser.MONTH, 0);
	}
	public DAY(): TerminalNode {
		return this.getToken(SqlParser.DAY, 0);
	}
	public HOUR(): TerminalNode {
		return this.getToken(SqlParser.HOUR, 0);
	}
	public MINUTE(): TerminalNode {
		return this.getToken(SqlParser.MINUTE, 0);
	}
	public SECOND(): TerminalNode {
		return this.getToken(SqlParser.SECOND, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_intervalField;
	}
}


export class NormalFormContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public NFD(): TerminalNode {
		return this.getToken(SqlParser.NFD, 0);
	}
	public NFC(): TerminalNode {
		return this.getToken(SqlParser.NFC, 0);
	}
	public NFKD(): TerminalNode {
		return this.getToken(SqlParser.NFKD, 0);
	}
	public NFKC(): TerminalNode {
		return this.getToken(SqlParser.NFKC, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_normalForm;
	}
}


export class TypesContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_types;
	}
}


export class TypeContext extends ParserRuleContext {
	public _from_!: IntervalFieldContext;
	public _to!: IntervalFieldContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlParser.ARRAY, 0);
	}
	public LT(): TerminalNode {
		return this.getToken(SqlParser.LT, 0);
	}
	public type__list(): TypeContext[] {
		return this.getTypedRuleContexts(TypeContext) as TypeContext[];
	}
	public type_(i: number): TypeContext {
		return this.getTypedRuleContext(TypeContext, i) as TypeContext;
	}
	public GT(): TerminalNode {
		return this.getToken(SqlParser.GT, 0);
	}
	public MAP(): TerminalNode {
		return this.getToken(SqlParser.MAP, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlParser.ROW, 0);
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
		return this.getToken(SqlParser.INTERVAL, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public intervalField_list(): IntervalFieldContext[] {
		return this.getTypedRuleContexts(IntervalFieldContext) as IntervalFieldContext[];
	}
	public intervalField(i: number): IntervalFieldContext {
		return this.getTypedRuleContext(IntervalFieldContext, i) as IntervalFieldContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_type;
	}
}


export class TypeParameterContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlParser.INTEGER_VALUE, 0);
	}
	public type_(): TypeContext {
		return this.getTypedRuleContext(TypeContext, 0) as TypeContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_typeParameter;
	}
}


export class BaseTypeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public TIME_WITH_TIME_ZONE(): TerminalNode {
		return this.getToken(SqlParser.TIME_WITH_TIME_ZONE, 0);
	}
	public TIMESTAMP_WITH_TIME_ZONE(): TerminalNode {
		return this.getToken(SqlParser.TIMESTAMP_WITH_TIME_ZONE, 0);
	}
	public DOUBLE_PRECISION(): TerminalNode {
		return this.getToken(SqlParser.DOUBLE_PRECISION, 0);
	}
	public qualifiedName(): QualifiedNameContext {
		return this.getTypedRuleContext(QualifiedNameContext, 0) as QualifiedNameContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_baseType;
	}
}


export class WhenClauseContext extends ParserRuleContext {
	public _condition!: ExpressionContext;
	public _result!: ExpressionContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public WHEN(): TerminalNode {
		return this.getToken(SqlParser.WHEN, 0);
	}
	public THEN(): TerminalNode {
		return this.getToken(SqlParser.THEN, 0);
	}
	public expression_list(): ExpressionContext[] {
		return this.getTypedRuleContexts(ExpressionContext) as ExpressionContext[];
	}
	public expression(i: number): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, i) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_whenClause;
	}
}


export class FilterContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public FILTER(): TerminalNode {
		return this.getToken(SqlParser.FILTER, 0);
	}
	public WHERE(): TerminalNode {
		return this.getToken(SqlParser.WHERE, 0);
	}
	public booleanExpression(): BooleanExpressionContext {
		return this.getTypedRuleContext(BooleanExpressionContext, 0) as BooleanExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_filter;
	}
}


export class OverContext extends ParserRuleContext {
	public _expression!: ExpressionContext;
	public _partition: ExpressionContext[] = [];
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public OVER(): TerminalNode {
		return this.getToken(SqlParser.OVER, 0);
	}
	public PARTITION(): TerminalNode {
		return this.getToken(SqlParser.PARTITION, 0);
	}
	public BY_list(): TerminalNode[] {
	    	return this.getTokens(SqlParser.BY);
	}
	public BY(i: number): TerminalNode {
		return this.getToken(SqlParser.BY, i);
	}
	public ORDER(): TerminalNode {
		return this.getToken(SqlParser.ORDER, 0);
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
    	return SqlParser.RULE_over;
	}
}


export class WindowFrameContext extends ParserRuleContext {
	public _frameType!: Token;
	public _start!: FrameBoundContext;
	public _end!: FrameBoundContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RANGE(): TerminalNode {
		return this.getToken(SqlParser.RANGE, 0);
	}
	public frameBound_list(): FrameBoundContext[] {
		return this.getTypedRuleContexts(FrameBoundContext) as FrameBoundContext[];
	}
	public frameBound(i: number): FrameBoundContext {
		return this.getTypedRuleContext(FrameBoundContext, i) as FrameBoundContext;
	}
	public ROWS(): TerminalNode {
		return this.getToken(SqlParser.ROWS, 0);
	}
	public GROUPS(): TerminalNode {
		return this.getToken(SqlParser.GROUPS, 0);
	}
	public BETWEEN(): TerminalNode {
		return this.getToken(SqlParser.BETWEEN, 0);
	}
	public AND(): TerminalNode {
		return this.getToken(SqlParser.AND, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_windowFrame;
	}
}


export class FrameBoundContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_frameBound;
	}
	public override copyFrom(ctx: FrameBoundContext): void {
		super.copyFrom(ctx);
	}
}
export class BoundedFrameContext extends FrameBoundContext {
	public _boundType!: Token;
	constructor(parser: SqlParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlParser.PRECEDING, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlParser.FOLLOWING, 0);
	}
}
export class UnboundedFrameContext extends FrameBoundContext {
	public _boundType!: Token;
	constructor(parser: SqlParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public UNBOUNDED(): TerminalNode {
		return this.getToken(SqlParser.UNBOUNDED, 0);
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlParser.PRECEDING, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlParser.FOLLOWING, 0);
	}
}
export class CurrentRowBoundContext extends FrameBoundContext {
	constructor(parser: SqlParser, ctx: FrameBoundContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlParser.CURRENT, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlParser.ROW, 0);
	}
}


export class UpdateAssignmentContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public EQ(): TerminalNode {
		return this.getToken(SqlParser.EQ, 0);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_updateAssignment;
	}
}


export class ExplainOptionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_explainOption;
	}
	public override copyFrom(ctx: ExplainOptionContext): void {
		super.copyFrom(ctx);
	}
}
export class ExplainFormatContext extends ExplainOptionContext {
	public _value!: Token;
	constructor(parser: SqlParser, ctx: ExplainOptionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public FORMAT(): TerminalNode {
		return this.getToken(SqlParser.FORMAT, 0);
	}
	public TEXT(): TerminalNode {
		return this.getToken(SqlParser.TEXT, 0);
	}
	public GRAPHVIZ(): TerminalNode {
		return this.getToken(SqlParser.GRAPHVIZ, 0);
	}
	public JSON(): TerminalNode {
		return this.getToken(SqlParser.JSON, 0);
	}
}
export class ExplainTypeContext extends ExplainOptionContext {
	public _value!: Token;
	constructor(parser: SqlParser, ctx: ExplainOptionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlParser.TYPE, 0);
	}
	public LOGICAL(): TerminalNode {
		return this.getToken(SqlParser.LOGICAL, 0);
	}
	public DISTRIBUTED(): TerminalNode {
		return this.getToken(SqlParser.DISTRIBUTED, 0);
	}
	public VALIDATE(): TerminalNode {
		return this.getToken(SqlParser.VALIDATE, 0);
	}
	public IO(): TerminalNode {
		return this.getToken(SqlParser.IO, 0);
	}
}


export class TransactionModeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_transactionMode;
	}
	public override copyFrom(ctx: TransactionModeContext): void {
		super.copyFrom(ctx);
	}
}
export class TransactionAccessModeContext extends TransactionModeContext {
	public _accessMode!: Token;
	constructor(parser: SqlParser, ctx: TransactionModeContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlParser.READ, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlParser.ONLY, 0);
	}
	public WRITE(): TerminalNode {
		return this.getToken(SqlParser.WRITE, 0);
	}
}
export class IsolationLevelContext extends TransactionModeContext {
	constructor(parser: SqlParser, ctx: TransactionModeContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ISOLATION(): TerminalNode {
		return this.getToken(SqlParser.ISOLATION, 0);
	}
	public LEVEL(): TerminalNode {
		return this.getToken(SqlParser.LEVEL, 0);
	}
	public levelOfIsolation(): LevelOfIsolationContext {
		return this.getTypedRuleContext(LevelOfIsolationContext, 0) as LevelOfIsolationContext;
	}
}


export class LevelOfIsolationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_levelOfIsolation;
	}
	public override copyFrom(ctx: LevelOfIsolationContext): void {
		super.copyFrom(ctx);
	}
}
export class ReadUncommittedContext extends LevelOfIsolationContext {
	constructor(parser: SqlParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlParser.READ, 0);
	}
	public UNCOMMITTED(): TerminalNode {
		return this.getToken(SqlParser.UNCOMMITTED, 0);
	}
}
export class SerializableContext extends LevelOfIsolationContext {
	constructor(parser: SqlParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SERIALIZABLE(): TerminalNode {
		return this.getToken(SqlParser.SERIALIZABLE, 0);
	}
}
export class ReadCommittedContext extends LevelOfIsolationContext {
	constructor(parser: SqlParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlParser.READ, 0);
	}
	public COMMITTED(): TerminalNode {
		return this.getToken(SqlParser.COMMITTED, 0);
	}
}
export class RepeatableReadContext extends LevelOfIsolationContext {
	constructor(parser: SqlParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public REPEATABLE(): TerminalNode {
		return this.getToken(SqlParser.REPEATABLE, 0);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlParser.READ, 0);
	}
}


export class CallArgumentContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_callArgument;
	}
	public override copyFrom(ctx: CallArgumentContext): void {
		super.copyFrom(ctx);
	}
}
export class PositionalArgumentContext extends CallArgumentContext {
	constructor(parser: SqlParser, ctx: CallArgumentContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class NamedArgumentContext extends CallArgumentContext {
	constructor(parser: SqlParser, ctx: CallArgumentContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}


export class PrivilegeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public SELECT(): TerminalNode {
		return this.getToken(SqlParser.SELECT, 0);
	}
	public DELETE(): TerminalNode {
		return this.getToken(SqlParser.DELETE, 0);
	}
	public INSERT(): TerminalNode {
		return this.getToken(SqlParser.INSERT, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_privilege;
	}
}


export class QualifiedNameContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_qualifiedName;
	}
}


export class TableVersionExpressionContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_tableVersionExpression;
	}
	public override copyFrom(ctx: TableVersionExpressionContext): void {
		super.copyFrom(ctx);
	}
}
export class TableVersionContext extends TableVersionExpressionContext {
	public _tableVersionType!: Token;
	constructor(parser: SqlParser, ctx: TableVersionExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public FOR(): TerminalNode {
		return this.getToken(SqlParser.FOR, 0);
	}
	public tableVersionState(): TableVersionStateContext {
		return this.getTypedRuleContext(TableVersionStateContext, 0) as TableVersionStateContext;
	}
	public valueExpression(): ValueExpressionContext {
		return this.getTypedRuleContext(ValueExpressionContext, 0) as ValueExpressionContext;
	}
	public SYSTEM_TIME(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM_TIME, 0);
	}
	public SYSTEM_VERSION(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM_VERSION, 0);
	}
	public TIMESTAMP(): TerminalNode {
		return this.getToken(SqlParser.TIMESTAMP, 0);
	}
	public VERSION(): TerminalNode {
		return this.getToken(SqlParser.VERSION, 0);
	}
}


export class TableVersionStateContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_tableVersionState;
	}
	public override copyFrom(ctx: TableVersionStateContext): void {
		super.copyFrom(ctx);
	}
}
export class TableversionbeforeContext extends TableVersionStateContext {
	constructor(parser: SqlParser, ctx: TableVersionStateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BEFORE(): TerminalNode {
		return this.getToken(SqlParser.BEFORE, 0);
	}
}
export class TableversionasofContext extends TableVersionStateContext {
	constructor(parser: SqlParser, ctx: TableVersionStateContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public AS(): TerminalNode {
		return this.getToken(SqlParser.AS, 0);
	}
	public OF(): TerminalNode {
		return this.getToken(SqlParser.OF, 0);
	}
}


export class GrantorContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_grantor;
	}
	public override copyFrom(ctx: GrantorContext): void {
		super.copyFrom(ctx);
	}
}
export class CurrentUserGrantorContext extends GrantorContext {
	constructor(parser: SqlParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_USER(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_USER, 0);
	}
}
export class SpecifiedPrincipalContext extends GrantorContext {
	constructor(parser: SqlParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public principal(): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, 0) as PrincipalContext;
	}
}
export class CurrentRoleGrantorContext extends GrantorContext {
	constructor(parser: SqlParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public CURRENT_ROLE(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_ROLE, 0);
	}
}


export class PrincipalContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_principal;
	}
	public override copyFrom(ctx: PrincipalContext): void {
		super.copyFrom(ctx);
	}
}
export class UnspecifiedPrincipalContext extends PrincipalContext {
	constructor(parser: SqlParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class UserPrincipalContext extends PrincipalContext {
	constructor(parser: SqlParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public USER(): TerminalNode {
		return this.getToken(SqlParser.USER, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}
export class RolePrincipalContext extends PrincipalContext {
	constructor(parser: SqlParser, ctx: PrincipalContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
}


export class RolesContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_roles;
	}
}


export class IdentifierContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_identifier;
	}
	public override copyFrom(ctx: IdentifierContext): void {
		super.copyFrom(ctx);
	}
}
export class BackQuotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public BACKQUOTED_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlParser.BACKQUOTED_IDENTIFIER, 0);
	}
}
export class QuotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public QUOTED_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlParser.QUOTED_IDENTIFIER, 0);
	}
}
export class DigitIdentifierContext extends IdentifierContext {
	constructor(parser: SqlParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DIGIT_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlParser.DIGIT_IDENTIFIER, 0);
	}
}
export class UnquotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public IDENTIFIER(): TerminalNode {
		return this.getToken(SqlParser.IDENTIFIER, 0);
	}
	public nonReserved(): NonReservedContext {
		return this.getTypedRuleContext(NonReservedContext, 0) as NonReservedContext;
	}
}


export class NumberContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_number;
	}
	public override copyFrom(ctx: NumberContext): void {
		super.copyFrom(ctx);
	}
}
export class DecimalLiteralContext extends NumberContext {
	constructor(parser: SqlParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DECIMAL_VALUE(): TerminalNode {
		return this.getToken(SqlParser.DECIMAL_VALUE, 0);
	}
}
export class DoubleLiteralContext extends NumberContext {
	constructor(parser: SqlParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DOUBLE_VALUE(): TerminalNode {
		return this.getToken(SqlParser.DOUBLE_VALUE, 0);
	}
}
export class IntegerLiteralContext extends NumberContext {
	constructor(parser: SqlParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public INTEGER_VALUE(): TerminalNode {
		return this.getToken(SqlParser.INTEGER_VALUE, 0);
	}
}


export class ConstraintSpecificationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_constraintSpecification;
	}
}


export class NamedConstraintSpecificationContext extends ParserRuleContext {
	public _name!: IdentifierContext;
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public CONSTRAINT(): TerminalNode {
		return this.getToken(SqlParser.CONSTRAINT, 0);
	}
	public unnamedConstraintSpecification(): UnnamedConstraintSpecificationContext {
		return this.getTypedRuleContext(UnnamedConstraintSpecificationContext, 0) as UnnamedConstraintSpecificationContext;
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_namedConstraintSpecification;
	}
}


export class UnnamedConstraintSpecificationContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_unnamedConstraintSpecification;
	}
}


export class ConstraintTypeContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public UNIQUE(): TerminalNode {
		return this.getToken(SqlParser.UNIQUE, 0);
	}
	public PRIMARY(): TerminalNode {
		return this.getToken(SqlParser.PRIMARY, 0);
	}
	public KEY(): TerminalNode {
		return this.getToken(SqlParser.KEY, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_constraintType;
	}
}


export class ConstraintQualifiersContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_constraintQualifiers;
	}
}


export class ConstraintQualifierContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
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
    	return SqlParser.RULE_constraintQualifier;
	}
}


export class ConstraintRelyContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public RELY(): TerminalNode {
		return this.getToken(SqlParser.RELY, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_constraintRely;
	}
}


export class ConstraintEnabledContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ENABLED(): TerminalNode {
		return this.getToken(SqlParser.ENABLED, 0);
	}
	public DISABLED(): TerminalNode {
		return this.getToken(SqlParser.DISABLED, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_constraintEnabled;
	}
}


export class ConstraintEnforcedContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ENFORCED(): TerminalNode {
		return this.getToken(SqlParser.ENFORCED, 0);
	}
	public NOT(): TerminalNode {
		return this.getToken(SqlParser.NOT, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_constraintEnforced;
	}
}


export class NonReservedContext extends ParserRuleContext {
	constructor(parser?: SqlParser, parent?: ParserRuleContext, invokingState?: number) {
		super(parent, invokingState);
    	this.parser = parser;
	}
	public ADD(): TerminalNode {
		return this.getToken(SqlParser.ADD, 0);
	}
	public ADMIN(): TerminalNode {
		return this.getToken(SqlParser.ADMIN, 0);
	}
	public ALL(): TerminalNode {
		return this.getToken(SqlParser.ALL, 0);
	}
	public ANALYZE(): TerminalNode {
		return this.getToken(SqlParser.ANALYZE, 0);
	}
	public ANY(): TerminalNode {
		return this.getToken(SqlParser.ANY, 0);
	}
	public ARRAY(): TerminalNode {
		return this.getToken(SqlParser.ARRAY, 0);
	}
	public ASC(): TerminalNode {
		return this.getToken(SqlParser.ASC, 0);
	}
	public AT(): TerminalNode {
		return this.getToken(SqlParser.AT, 0);
	}
	public BEFORE(): TerminalNode {
		return this.getToken(SqlParser.BEFORE, 0);
	}
	public BERNOULLI(): TerminalNode {
		return this.getToken(SqlParser.BERNOULLI, 0);
	}
	public CALL(): TerminalNode {
		return this.getToken(SqlParser.CALL, 0);
	}
	public CALLED(): TerminalNode {
		return this.getToken(SqlParser.CALLED, 0);
	}
	public CASCADE(): TerminalNode {
		return this.getToken(SqlParser.CASCADE, 0);
	}
	public CATALOGS(): TerminalNode {
		return this.getToken(SqlParser.CATALOGS, 0);
	}
	public COLUMN(): TerminalNode {
		return this.getToken(SqlParser.COLUMN, 0);
	}
	public COLUMNS(): TerminalNode {
		return this.getToken(SqlParser.COLUMNS, 0);
	}
	public COMMENT(): TerminalNode {
		return this.getToken(SqlParser.COMMENT, 0);
	}
	public COMMIT(): TerminalNode {
		return this.getToken(SqlParser.COMMIT, 0);
	}
	public COMMITTED(): TerminalNode {
		return this.getToken(SqlParser.COMMITTED, 0);
	}
	public CURRENT(): TerminalNode {
		return this.getToken(SqlParser.CURRENT, 0);
	}
	public CURRENT_ROLE(): TerminalNode {
		return this.getToken(SqlParser.CURRENT_ROLE, 0);
	}
	public DATA(): TerminalNode {
		return this.getToken(SqlParser.DATA, 0);
	}
	public DATE(): TerminalNode {
		return this.getToken(SqlParser.DATE, 0);
	}
	public DAY(): TerminalNode {
		return this.getToken(SqlParser.DAY, 0);
	}
	public DEFINER(): TerminalNode {
		return this.getToken(SqlParser.DEFINER, 0);
	}
	public DESC(): TerminalNode {
		return this.getToken(SqlParser.DESC, 0);
	}
	public DETERMINISTIC(): TerminalNode {
		return this.getToken(SqlParser.DETERMINISTIC, 0);
	}
	public DISABLED(): TerminalNode {
		return this.getToken(SqlParser.DISABLED, 0);
	}
	public DISTRIBUTED(): TerminalNode {
		return this.getToken(SqlParser.DISTRIBUTED, 0);
	}
	public ENABLED(): TerminalNode {
		return this.getToken(SqlParser.ENABLED, 0);
	}
	public ENFORCED(): TerminalNode {
		return this.getToken(SqlParser.ENFORCED, 0);
	}
	public EXCLUDING(): TerminalNode {
		return this.getToken(SqlParser.EXCLUDING, 0);
	}
	public EXPLAIN(): TerminalNode {
		return this.getToken(SqlParser.EXPLAIN, 0);
	}
	public EXTERNAL(): TerminalNode {
		return this.getToken(SqlParser.EXTERNAL, 0);
	}
	public FETCH(): TerminalNode {
		return this.getToken(SqlParser.FETCH, 0);
	}
	public FILTER(): TerminalNode {
		return this.getToken(SqlParser.FILTER, 0);
	}
	public FIRST(): TerminalNode {
		return this.getToken(SqlParser.FIRST, 0);
	}
	public FOLLOWING(): TerminalNode {
		return this.getToken(SqlParser.FOLLOWING, 0);
	}
	public FORMAT(): TerminalNode {
		return this.getToken(SqlParser.FORMAT, 0);
	}
	public FUNCTION(): TerminalNode {
		return this.getToken(SqlParser.FUNCTION, 0);
	}
	public FUNCTIONS(): TerminalNode {
		return this.getToken(SqlParser.FUNCTIONS, 0);
	}
	public GRANT(): TerminalNode {
		return this.getToken(SqlParser.GRANT, 0);
	}
	public GRANTED(): TerminalNode {
		return this.getToken(SqlParser.GRANTED, 0);
	}
	public GRANTS(): TerminalNode {
		return this.getToken(SqlParser.GRANTS, 0);
	}
	public GRAPHVIZ(): TerminalNode {
		return this.getToken(SqlParser.GRAPHVIZ, 0);
	}
	public GROUPS(): TerminalNode {
		return this.getToken(SqlParser.GROUPS, 0);
	}
	public HOUR(): TerminalNode {
		return this.getToken(SqlParser.HOUR, 0);
	}
	public IF(): TerminalNode {
		return this.getToken(SqlParser.IF, 0);
	}
	public IGNORE(): TerminalNode {
		return this.getToken(SqlParser.IGNORE, 0);
	}
	public INCLUDING(): TerminalNode {
		return this.getToken(SqlParser.INCLUDING, 0);
	}
	public INPUT(): TerminalNode {
		return this.getToken(SqlParser.INPUT, 0);
	}
	public INTERVAL(): TerminalNode {
		return this.getToken(SqlParser.INTERVAL, 0);
	}
	public INVOKER(): TerminalNode {
		return this.getToken(SqlParser.INVOKER, 0);
	}
	public IO(): TerminalNode {
		return this.getToken(SqlParser.IO, 0);
	}
	public ISOLATION(): TerminalNode {
		return this.getToken(SqlParser.ISOLATION, 0);
	}
	public JSON(): TerminalNode {
		return this.getToken(SqlParser.JSON, 0);
	}
	public KEY(): TerminalNode {
		return this.getToken(SqlParser.KEY, 0);
	}
	public LANGUAGE(): TerminalNode {
		return this.getToken(SqlParser.LANGUAGE, 0);
	}
	public LAST(): TerminalNode {
		return this.getToken(SqlParser.LAST, 0);
	}
	public LATERAL(): TerminalNode {
		return this.getToken(SqlParser.LATERAL, 0);
	}
	public LEVEL(): TerminalNode {
		return this.getToken(SqlParser.LEVEL, 0);
	}
	public LIMIT(): TerminalNode {
		return this.getToken(SqlParser.LIMIT, 0);
	}
	public LOGICAL(): TerminalNode {
		return this.getToken(SqlParser.LOGICAL, 0);
	}
	public MAP(): TerminalNode {
		return this.getToken(SqlParser.MAP, 0);
	}
	public MATERIALIZED(): TerminalNode {
		return this.getToken(SqlParser.MATERIALIZED, 0);
	}
	public MINUTE(): TerminalNode {
		return this.getToken(SqlParser.MINUTE, 0);
	}
	public MONTH(): TerminalNode {
		return this.getToken(SqlParser.MONTH, 0);
	}
	public NAME(): TerminalNode {
		return this.getToken(SqlParser.NAME, 0);
	}
	public NFC(): TerminalNode {
		return this.getToken(SqlParser.NFC, 0);
	}
	public NFD(): TerminalNode {
		return this.getToken(SqlParser.NFD, 0);
	}
	public NFKC(): TerminalNode {
		return this.getToken(SqlParser.NFKC, 0);
	}
	public NFKD(): TerminalNode {
		return this.getToken(SqlParser.NFKD, 0);
	}
	public NO(): TerminalNode {
		return this.getToken(SqlParser.NO, 0);
	}
	public NONE(): TerminalNode {
		return this.getToken(SqlParser.NONE, 0);
	}
	public NULLIF(): TerminalNode {
		return this.getToken(SqlParser.NULLIF, 0);
	}
	public NULLS(): TerminalNode {
		return this.getToken(SqlParser.NULLS, 0);
	}
	public OF(): TerminalNode {
		return this.getToken(SqlParser.OF, 0);
	}
	public OFFSET(): TerminalNode {
		return this.getToken(SqlParser.OFFSET, 0);
	}
	public ONLY(): TerminalNode {
		return this.getToken(SqlParser.ONLY, 0);
	}
	public OPTION(): TerminalNode {
		return this.getToken(SqlParser.OPTION, 0);
	}
	public ORDINALITY(): TerminalNode {
		return this.getToken(SqlParser.ORDINALITY, 0);
	}
	public OUTPUT(): TerminalNode {
		return this.getToken(SqlParser.OUTPUT, 0);
	}
	public OVER(): TerminalNode {
		return this.getToken(SqlParser.OVER, 0);
	}
	public PARTITION(): TerminalNode {
		return this.getToken(SqlParser.PARTITION, 0);
	}
	public PARTITIONS(): TerminalNode {
		return this.getToken(SqlParser.PARTITIONS, 0);
	}
	public POSITION(): TerminalNode {
		return this.getToken(SqlParser.POSITION, 0);
	}
	public PRECEDING(): TerminalNode {
		return this.getToken(SqlParser.PRECEDING, 0);
	}
	public PRIMARY(): TerminalNode {
		return this.getToken(SqlParser.PRIMARY, 0);
	}
	public PRIVILEGES(): TerminalNode {
		return this.getToken(SqlParser.PRIVILEGES, 0);
	}
	public PROPERTIES(): TerminalNode {
		return this.getToken(SqlParser.PROPERTIES, 0);
	}
	public RANGE(): TerminalNode {
		return this.getToken(SqlParser.RANGE, 0);
	}
	public READ(): TerminalNode {
		return this.getToken(SqlParser.READ, 0);
	}
	public REFRESH(): TerminalNode {
		return this.getToken(SqlParser.REFRESH, 0);
	}
	public RELY(): TerminalNode {
		return this.getToken(SqlParser.RELY, 0);
	}
	public RENAME(): TerminalNode {
		return this.getToken(SqlParser.RENAME, 0);
	}
	public REPEATABLE(): TerminalNode {
		return this.getToken(SqlParser.REPEATABLE, 0);
	}
	public REPLACE(): TerminalNode {
		return this.getToken(SqlParser.REPLACE, 0);
	}
	public RESET(): TerminalNode {
		return this.getToken(SqlParser.RESET, 0);
	}
	public RESPECT(): TerminalNode {
		return this.getToken(SqlParser.RESPECT, 0);
	}
	public RESTRICT(): TerminalNode {
		return this.getToken(SqlParser.RESTRICT, 0);
	}
	public RETURN(): TerminalNode {
		return this.getToken(SqlParser.RETURN, 0);
	}
	public RETURNS(): TerminalNode {
		return this.getToken(SqlParser.RETURNS, 0);
	}
	public REVOKE(): TerminalNode {
		return this.getToken(SqlParser.REVOKE, 0);
	}
	public ROLE(): TerminalNode {
		return this.getToken(SqlParser.ROLE, 0);
	}
	public ROLES(): TerminalNode {
		return this.getToken(SqlParser.ROLES, 0);
	}
	public ROLLBACK(): TerminalNode {
		return this.getToken(SqlParser.ROLLBACK, 0);
	}
	public ROW(): TerminalNode {
		return this.getToken(SqlParser.ROW, 0);
	}
	public ROWS(): TerminalNode {
		return this.getToken(SqlParser.ROWS, 0);
	}
	public SCHEMA(): TerminalNode {
		return this.getToken(SqlParser.SCHEMA, 0);
	}
	public SCHEMAS(): TerminalNode {
		return this.getToken(SqlParser.SCHEMAS, 0);
	}
	public SECOND(): TerminalNode {
		return this.getToken(SqlParser.SECOND, 0);
	}
	public SECURITY(): TerminalNode {
		return this.getToken(SqlParser.SECURITY, 0);
	}
	public SERIALIZABLE(): TerminalNode {
		return this.getToken(SqlParser.SERIALIZABLE, 0);
	}
	public SESSION(): TerminalNode {
		return this.getToken(SqlParser.SESSION, 0);
	}
	public SET(): TerminalNode {
		return this.getToken(SqlParser.SET, 0);
	}
	public SETS(): TerminalNode {
		return this.getToken(SqlParser.SETS, 0);
	}
	public SQL(): TerminalNode {
		return this.getToken(SqlParser.SQL, 0);
	}
	public SHOW(): TerminalNode {
		return this.getToken(SqlParser.SHOW, 0);
	}
	public SOME(): TerminalNode {
		return this.getToken(SqlParser.SOME, 0);
	}
	public START(): TerminalNode {
		return this.getToken(SqlParser.START, 0);
	}
	public STATS(): TerminalNode {
		return this.getToken(SqlParser.STATS, 0);
	}
	public SUBSTRING(): TerminalNode {
		return this.getToken(SqlParser.SUBSTRING, 0);
	}
	public SYSTEM(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM, 0);
	}
	public SYSTEM_TIME(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM_TIME, 0);
	}
	public SYSTEM_VERSION(): TerminalNode {
		return this.getToken(SqlParser.SYSTEM_VERSION, 0);
	}
	public TABLES(): TerminalNode {
		return this.getToken(SqlParser.TABLES, 0);
	}
	public TABLESAMPLE(): TerminalNode {
		return this.getToken(SqlParser.TABLESAMPLE, 0);
	}
	public TEMPORARY(): TerminalNode {
		return this.getToken(SqlParser.TEMPORARY, 0);
	}
	public TEXT(): TerminalNode {
		return this.getToken(SqlParser.TEXT, 0);
	}
	public TIME(): TerminalNode {
		return this.getToken(SqlParser.TIME, 0);
	}
	public TIMESTAMP(): TerminalNode {
		return this.getToken(SqlParser.TIMESTAMP, 0);
	}
	public TO(): TerminalNode {
		return this.getToken(SqlParser.TO, 0);
	}
	public TRANSACTION(): TerminalNode {
		return this.getToken(SqlParser.TRANSACTION, 0);
	}
	public TRUNCATE(): TerminalNode {
		return this.getToken(SqlParser.TRUNCATE, 0);
	}
	public TRY_CAST(): TerminalNode {
		return this.getToken(SqlParser.TRY_CAST, 0);
	}
	public TYPE(): TerminalNode {
		return this.getToken(SqlParser.TYPE, 0);
	}
	public UNBOUNDED(): TerminalNode {
		return this.getToken(SqlParser.UNBOUNDED, 0);
	}
	public UNCOMMITTED(): TerminalNode {
		return this.getToken(SqlParser.UNCOMMITTED, 0);
	}
	public UNIQUE(): TerminalNode {
		return this.getToken(SqlParser.UNIQUE, 0);
	}
	public UPDATE(): TerminalNode {
		return this.getToken(SqlParser.UPDATE, 0);
	}
	public USE(): TerminalNode {
		return this.getToken(SqlParser.USE, 0);
	}
	public USER(): TerminalNode {
		return this.getToken(SqlParser.USER, 0);
	}
	public VALIDATE(): TerminalNode {
		return this.getToken(SqlParser.VALIDATE, 0);
	}
	public VERBOSE(): TerminalNode {
		return this.getToken(SqlParser.VERBOSE, 0);
	}
	public VERSION(): TerminalNode {
		return this.getToken(SqlParser.VERSION, 0);
	}
	public VIEW(): TerminalNode {
		return this.getToken(SqlParser.VIEW, 0);
	}
	public WORK(): TerminalNode {
		return this.getToken(SqlParser.WORK, 0);
	}
	public WRITE(): TerminalNode {
		return this.getToken(SqlParser.WRITE, 0);
	}
	public YEAR(): TerminalNode {
		return this.getToken(SqlParser.YEAR, 0);
	}
	public ZONE(): TerminalNode {
		return this.getToken(SqlParser.ZONE, 0);
	}
    public get ruleIndex(): number {
    	return SqlParser.RULE_nonReserved;
	}
}
