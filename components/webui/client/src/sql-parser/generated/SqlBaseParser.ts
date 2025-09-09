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
	public static readonly RULE_statement = 3;
	public static readonly RULE_query = 4;
	public static readonly RULE_with = 5;
	public static readonly RULE_tableElement = 6;
	public static readonly RULE_columnDefinition = 7;
	public static readonly RULE_likeClause = 8;
	public static readonly RULE_properties = 9;
	public static readonly RULE_property = 10;
	public static readonly RULE_sqlParameterDeclaration = 11;
	public static readonly RULE_routineCharacteristics = 12;
	public static readonly RULE_routineCharacteristic = 13;
	public static readonly RULE_alterRoutineCharacteristics = 14;
	public static readonly RULE_alterRoutineCharacteristic = 15;
	public static readonly RULE_routineBody = 16;
	public static readonly RULE_returnStatement = 17;
	public static readonly RULE_externalBodyReference = 18;
	public static readonly RULE_language = 19;
	public static readonly RULE_determinism = 20;
	public static readonly RULE_nullCallClause = 21;
	public static readonly RULE_externalRoutineName = 22;
	public static readonly RULE_queryNoWith = 23;
	public static readonly RULE_queryTerm = 24;
	public static readonly RULE_queryPrimary = 25;
	public static readonly RULE_sortItem = 26;
	public static readonly RULE_querySpecification = 27;
	public static readonly RULE_groupBy = 28;
	public static readonly RULE_groupingElement = 29;
	public static readonly RULE_groupingSet = 30;
	public static readonly RULE_namedQuery = 31;
	public static readonly RULE_setQuantifier = 32;
	public static readonly RULE_selectItem = 33;
	public static readonly RULE_relation = 34;
	public static readonly RULE_joinType = 35;
	public static readonly RULE_joinCriteria = 36;
	public static readonly RULE_sampledRelation = 37;
	public static readonly RULE_sampleType = 38;
	public static readonly RULE_aliasedRelation = 39;
	public static readonly RULE_columnAliases = 40;
	public static readonly RULE_relationPrimary = 41;
	public static readonly RULE_expression = 42;
	public static readonly RULE_booleanExpression = 43;
	public static readonly RULE_predicate = 44;
	public static readonly RULE_valueExpression = 45;
	public static readonly RULE_primaryExpression = 46;
	public static readonly RULE_string = 47;
	public static readonly RULE_nullTreatment = 48;
	public static readonly RULE_timeZoneSpecifier = 49;
	public static readonly RULE_comparisonOperator = 50;
	public static readonly RULE_comparisonQuantifier = 51;
	public static readonly RULE_booleanValue = 52;
	public static readonly RULE_interval = 53;
	public static readonly RULE_intervalField = 54;
	public static readonly RULE_normalForm = 55;
	public static readonly RULE_types = 56;
	public static readonly RULE_type = 57;
	public static readonly RULE_typeParameter = 58;
	public static readonly RULE_baseType = 59;
	public static readonly RULE_whenClause = 60;
	public static readonly RULE_filter = 61;
	public static readonly RULE_over = 62;
	public static readonly RULE_windowFrame = 63;
	public static readonly RULE_frameBound = 64;
	public static readonly RULE_updateAssignment = 65;
	public static readonly RULE_explainOption = 66;
	public static readonly RULE_transactionMode = 67;
	public static readonly RULE_levelOfIsolation = 68;
	public static readonly RULE_callArgument = 69;
	public static readonly RULE_privilege = 70;
	public static readonly RULE_qualifiedName = 71;
	public static readonly RULE_tableVersionExpression = 72;
	public static readonly RULE_tableVersionState = 73;
	public static readonly RULE_grantor = 74;
	public static readonly RULE_principal = 75;
	public static readonly RULE_roles = 76;
	public static readonly RULE_identifier = 77;
	public static readonly RULE_number = 78;
	public static readonly RULE_constraintSpecification = 79;
	public static readonly RULE_namedConstraintSpecification = 80;
	public static readonly RULE_unnamedConstraintSpecification = 81;
	public static readonly RULE_constraintType = 82;
	public static readonly RULE_constraintQualifiers = 83;
	public static readonly RULE_constraintQualifier = 84;
	public static readonly RULE_constraintRely = 85;
	public static readonly RULE_constraintEnabled = 86;
	public static readonly RULE_constraintEnforced = 87;
	public static readonly RULE_nonReserved = 88;
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
		"singleStatement", "standaloneExpression", "standaloneRoutineBody", "statement", 
		"query", "with", "tableElement", "columnDefinition", "likeClause", "properties", 
		"property", "sqlParameterDeclaration", "routineCharacteristics", "routineCharacteristic", 
		"alterRoutineCharacteristics", "alterRoutineCharacteristic", "routineBody", 
		"returnStatement", "externalBodyReference", "language", "determinism", 
		"nullCallClause", "externalRoutineName", "queryNoWith", "queryTerm", "queryPrimary", 
		"sortItem", "querySpecification", "groupBy", "groupingElement", "groupingSet", 
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
			this.state = 178;
			this.statement();
			this.state = 179;
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
			this.state = 181;
			this.expression();
			this.state = 182;
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
			this.state = 184;
			this.routineBody();
			this.state = 185;
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
		this.enterRule(localctx, 6, SqlBaseParser.RULE_statement);
		let _la: number;
		try {
			this.state = 928;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 102, this._ctx) ) {
			case 1:
				localctx = new StatementDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 187;
				this.query();
				}
				break;
			case 2:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 188;
				this.match(SqlBaseParser.USE);
				this.state = 189;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 3:
				localctx = new UseContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 190;
				this.match(SqlBaseParser.USE);
				this.state = 191;
				(localctx as UseContext)._catalog = this.identifier();
				this.state = 192;
				this.match(SqlBaseParser.T__0);
				this.state = 193;
				(localctx as UseContext)._schema = this.identifier();
				}
				break;
			case 4:
				localctx = new CreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 195;
				this.match(SqlBaseParser.CREATE);
				this.state = 196;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 200;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 0, this._ctx) ) {
				case 1:
					{
					this.state = 197;
					this.match(SqlBaseParser.IF);
					this.state = 198;
					this.match(SqlBaseParser.NOT);
					this.state = 199;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 202;
				this.qualifiedName();
				this.state = 205;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 203;
					this.match(SqlBaseParser.WITH);
					this.state = 204;
					this.properties();
					}
				}

				}
				break;
			case 5:
				localctx = new DropSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 207;
				this.match(SqlBaseParser.DROP);
				this.state = 208;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 211;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 2, this._ctx) ) {
				case 1:
					{
					this.state = 209;
					this.match(SqlBaseParser.IF);
					this.state = 210;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 213;
				this.qualifiedName();
				this.state = 215;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===27 || _la===164) {
					{
					this.state = 214;
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
				this.state = 217;
				this.match(SqlBaseParser.ALTER);
				this.state = 218;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 219;
				this.qualifiedName();
				this.state = 220;
				this.match(SqlBaseParser.RENAME);
				this.state = 221;
				this.match(SqlBaseParser.TO);
				this.state = 222;
				this.identifier();
				}
				break;
			case 7:
				localctx = new CreateTableAsSelectContext(this, localctx);
				this.enterOuterAlt(localctx, 7);
				{
				this.state = 224;
				this.match(SqlBaseParser.CREATE);
				this.state = 225;
				this.match(SqlBaseParser.TABLE);
				this.state = 229;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 4, this._ctx) ) {
				case 1:
					{
					this.state = 226;
					this.match(SqlBaseParser.IF);
					this.state = 227;
					this.match(SqlBaseParser.NOT);
					this.state = 228;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 231;
				this.qualifiedName();
				this.state = 233;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 232;
					this.columnAliases();
					}
				}

				this.state = 237;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 235;
					this.match(SqlBaseParser.COMMENT);
					this.state = 236;
					this.string_();
					}
				}

				this.state = 241;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 239;
					this.match(SqlBaseParser.WITH);
					this.state = 240;
					this.properties();
					}
				}

				this.state = 243;
				this.match(SqlBaseParser.AS);
				this.state = 249;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 8, this._ctx) ) {
				case 1:
					{
					this.state = 244;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 245;
					this.match(SqlBaseParser.T__1);
					this.state = 246;
					this.query();
					this.state = 247;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 256;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 251;
					this.match(SqlBaseParser.WITH);
					this.state = 253;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===128) {
						{
						this.state = 252;
						this.match(SqlBaseParser.NO);
						}
					}

					this.state = 255;
					this.match(SqlBaseParser.DATA);
					}
				}

				}
				break;
			case 8:
				localctx = new CreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
				{
				this.state = 258;
				this.match(SqlBaseParser.CREATE);
				this.state = 259;
				this.match(SqlBaseParser.TABLE);
				this.state = 263;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 11, this._ctx) ) {
				case 1:
					{
					this.state = 260;
					this.match(SqlBaseParser.IF);
					this.state = 261;
					this.match(SqlBaseParser.NOT);
					this.state = 262;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 265;
				this.qualifiedName();
				this.state = 266;
				this.match(SqlBaseParser.T__1);
				this.state = 267;
				this.tableElement();
				this.state = 272;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 268;
					this.match(SqlBaseParser.T__3);
					this.state = 269;
					this.tableElement();
					}
					}
					this.state = 274;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 275;
				this.match(SqlBaseParser.T__2);
				this.state = 278;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 276;
					this.match(SqlBaseParser.COMMENT);
					this.state = 277;
					this.string_();
					}
				}

				this.state = 282;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 280;
					this.match(SqlBaseParser.WITH);
					this.state = 281;
					this.properties();
					}
				}

				}
				break;
			case 9:
				localctx = new DropTableContext(this, localctx);
				this.enterOuterAlt(localctx, 9);
				{
				this.state = 284;
				this.match(SqlBaseParser.DROP);
				this.state = 285;
				this.match(SqlBaseParser.TABLE);
				this.state = 288;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 15, this._ctx) ) {
				case 1:
					{
					this.state = 286;
					this.match(SqlBaseParser.IF);
					this.state = 287;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 290;
				this.qualifiedName();
				}
				break;
			case 10:
				localctx = new InsertIntoContext(this, localctx);
				this.enterOuterAlt(localctx, 10);
				{
				this.state = 291;
				this.match(SqlBaseParser.INSERT);
				this.state = 292;
				this.match(SqlBaseParser.INTO);
				this.state = 293;
				this.qualifiedName();
				this.state = 295;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 16, this._ctx) ) {
				case 1:
					{
					this.state = 294;
					this.columnAliases();
					}
					break;
				}
				this.state = 297;
				this.query();
				}
				break;
			case 11:
				localctx = new DeleteContext(this, localctx);
				this.enterOuterAlt(localctx, 11);
				{
				this.state = 299;
				this.match(SqlBaseParser.DELETE);
				this.state = 300;
				this.match(SqlBaseParser.FROM);
				this.state = 301;
				this.qualifiedName();
				this.state = 304;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 302;
					this.match(SqlBaseParser.WHERE);
					this.state = 303;
					this.booleanExpression(0);
					}
				}

				}
				break;
			case 12:
				localctx = new TruncateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 12);
				{
				this.state = 306;
				this.match(SqlBaseParser.TRUNCATE);
				this.state = 307;
				this.match(SqlBaseParser.TABLE);
				this.state = 308;
				this.qualifiedName();
				}
				break;
			case 13:
				localctx = new RenameTableContext(this, localctx);
				this.enterOuterAlt(localctx, 13);
				{
				this.state = 309;
				this.match(SqlBaseParser.ALTER);
				this.state = 310;
				this.match(SqlBaseParser.TABLE);
				this.state = 313;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 18, this._ctx) ) {
				case 1:
					{
					this.state = 311;
					this.match(SqlBaseParser.IF);
					this.state = 312;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 315;
				(localctx as RenameTableContext)._from_ = this.qualifiedName();
				this.state = 316;
				this.match(SqlBaseParser.RENAME);
				this.state = 317;
				this.match(SqlBaseParser.TO);
				this.state = 318;
				(localctx as RenameTableContext)._to = this.qualifiedName();
				}
				break;
			case 14:
				localctx = new RenameColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 14);
				{
				this.state = 320;
				this.match(SqlBaseParser.ALTER);
				this.state = 321;
				this.match(SqlBaseParser.TABLE);
				this.state = 324;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 19, this._ctx) ) {
				case 1:
					{
					this.state = 322;
					this.match(SqlBaseParser.IF);
					this.state = 323;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 326;
				(localctx as RenameColumnContext)._tableName = this.qualifiedName();
				this.state = 327;
				this.match(SqlBaseParser.RENAME);
				this.state = 328;
				this.match(SqlBaseParser.COLUMN);
				this.state = 331;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 20, this._ctx) ) {
				case 1:
					{
					this.state = 329;
					this.match(SqlBaseParser.IF);
					this.state = 330;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 333;
				(localctx as RenameColumnContext)._from_ = this.identifier();
				this.state = 334;
				this.match(SqlBaseParser.TO);
				this.state = 335;
				(localctx as RenameColumnContext)._to = this.identifier();
				}
				break;
			case 15:
				localctx = new DropColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 15);
				{
				this.state = 337;
				this.match(SqlBaseParser.ALTER);
				this.state = 338;
				this.match(SqlBaseParser.TABLE);
				this.state = 341;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 21, this._ctx) ) {
				case 1:
					{
					this.state = 339;
					this.match(SqlBaseParser.IF);
					this.state = 340;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 343;
				(localctx as DropColumnContext)._tableName = this.qualifiedName();
				this.state = 344;
				this.match(SqlBaseParser.DROP);
				this.state = 345;
				this.match(SqlBaseParser.COLUMN);
				this.state = 348;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 22, this._ctx) ) {
				case 1:
					{
					this.state = 346;
					this.match(SqlBaseParser.IF);
					this.state = 347;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 350;
				(localctx as DropColumnContext)._column = this.qualifiedName();
				}
				break;
			case 16:
				localctx = new AddColumnContext(this, localctx);
				this.enterOuterAlt(localctx, 16);
				{
				this.state = 352;
				this.match(SqlBaseParser.ALTER);
				this.state = 353;
				this.match(SqlBaseParser.TABLE);
				this.state = 356;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 23, this._ctx) ) {
				case 1:
					{
					this.state = 354;
					this.match(SqlBaseParser.IF);
					this.state = 355;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 358;
				(localctx as AddColumnContext)._tableName = this.qualifiedName();
				this.state = 359;
				this.match(SqlBaseParser.ADD);
				this.state = 360;
				this.match(SqlBaseParser.COLUMN);
				this.state = 364;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 24, this._ctx) ) {
				case 1:
					{
					this.state = 361;
					this.match(SqlBaseParser.IF);
					this.state = 362;
					this.match(SqlBaseParser.NOT);
					this.state = 363;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 366;
				(localctx as AddColumnContext)._column = this.columnDefinition();
				}
				break;
			case 17:
				localctx = new AddConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 17);
				{
				this.state = 368;
				this.match(SqlBaseParser.ALTER);
				this.state = 369;
				this.match(SqlBaseParser.TABLE);
				this.state = 372;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 25, this._ctx) ) {
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
				(localctx as AddConstraintContext)._tableName = this.qualifiedName();
				this.state = 375;
				this.match(SqlBaseParser.ADD);
				this.state = 376;
				this.constraintSpecification();
				}
				break;
			case 18:
				localctx = new DropConstraintContext(this, localctx);
				this.enterOuterAlt(localctx, 18);
				{
				this.state = 378;
				this.match(SqlBaseParser.ALTER);
				this.state = 379;
				this.match(SqlBaseParser.TABLE);
				this.state = 382;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 26, this._ctx) ) {
				case 1:
					{
					this.state = 380;
					this.match(SqlBaseParser.IF);
					this.state = 381;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 384;
				(localctx as DropConstraintContext)._tableName = this.qualifiedName();
				this.state = 385;
				this.match(SqlBaseParser.DROP);
				this.state = 386;
				this.match(SqlBaseParser.CONSTRAINT);
				this.state = 389;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 27, this._ctx) ) {
				case 1:
					{
					this.state = 387;
					this.match(SqlBaseParser.IF);
					this.state = 388;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 391;
				(localctx as DropConstraintContext)._name = this.identifier();
				}
				break;
			case 19:
				localctx = new AlterColumnSetNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 19);
				{
				this.state = 393;
				this.match(SqlBaseParser.ALTER);
				this.state = 394;
				this.match(SqlBaseParser.TABLE);
				this.state = 397;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 28, this._ctx) ) {
				case 1:
					{
					this.state = 395;
					this.match(SqlBaseParser.IF);
					this.state = 396;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 399;
				(localctx as AlterColumnSetNotNullContext)._tableName = this.qualifiedName();
				this.state = 400;
				this.match(SqlBaseParser.ALTER);
				this.state = 402;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 29, this._ctx) ) {
				case 1:
					{
					this.state = 401;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 404;
				(localctx as AlterColumnSetNotNullContext)._column = this.identifier();
				this.state = 405;
				this.match(SqlBaseParser.SET);
				this.state = 406;
				this.match(SqlBaseParser.NOT);
				this.state = 407;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 20:
				localctx = new AlterColumnDropNotNullContext(this, localctx);
				this.enterOuterAlt(localctx, 20);
				{
				this.state = 409;
				this.match(SqlBaseParser.ALTER);
				this.state = 410;
				this.match(SqlBaseParser.TABLE);
				this.state = 413;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 30, this._ctx) ) {
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
				(localctx as AlterColumnDropNotNullContext)._tableName = this.qualifiedName();
				this.state = 416;
				this.match(SqlBaseParser.ALTER);
				this.state = 418;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 31, this._ctx) ) {
				case 1:
					{
					this.state = 417;
					this.match(SqlBaseParser.COLUMN);
					}
					break;
				}
				this.state = 420;
				(localctx as AlterColumnDropNotNullContext)._column = this.identifier();
				this.state = 421;
				this.match(SqlBaseParser.DROP);
				this.state = 422;
				this.match(SqlBaseParser.NOT);
				this.state = 423;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 21:
				localctx = new SetTablePropertiesContext(this, localctx);
				this.enterOuterAlt(localctx, 21);
				{
				this.state = 425;
				this.match(SqlBaseParser.ALTER);
				this.state = 426;
				this.match(SqlBaseParser.TABLE);
				this.state = 429;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 32, this._ctx) ) {
				case 1:
					{
					this.state = 427;
					this.match(SqlBaseParser.IF);
					this.state = 428;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 431;
				(localctx as SetTablePropertiesContext)._tableName = this.qualifiedName();
				this.state = 432;
				this.match(SqlBaseParser.SET);
				this.state = 433;
				this.match(SqlBaseParser.PROPERTIES);
				this.state = 434;
				this.properties();
				}
				break;
			case 22:
				localctx = new AnalyzeContext(this, localctx);
				this.enterOuterAlt(localctx, 22);
				{
				this.state = 436;
				this.match(SqlBaseParser.ANALYZE);
				this.state = 437;
				this.qualifiedName();
				this.state = 440;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 438;
					this.match(SqlBaseParser.WITH);
					this.state = 439;
					this.properties();
					}
				}

				}
				break;
			case 23:
				localctx = new CreateTypeContext(this, localctx);
				this.enterOuterAlt(localctx, 23);
				{
				this.state = 442;
				this.match(SqlBaseParser.CREATE);
				this.state = 443;
				this.match(SqlBaseParser.TYPE);
				this.state = 444;
				this.qualifiedName();
				this.state = 445;
				this.match(SqlBaseParser.AS);
				this.state = 458;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 2:
					{
					this.state = 446;
					this.match(SqlBaseParser.T__1);
					this.state = 447;
					this.sqlParameterDeclaration();
					this.state = 452;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 448;
						this.match(SqlBaseParser.T__3);
						this.state = 449;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 454;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 455;
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
					this.state = 457;
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
				this.state = 460;
				this.match(SqlBaseParser.CREATE);
				this.state = 463;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 461;
					this.match(SqlBaseParser.OR);
					this.state = 462;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 465;
				this.match(SqlBaseParser.VIEW);
				this.state = 466;
				this.qualifiedName();
				this.state = 469;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===178) {
					{
					this.state = 467;
					this.match(SqlBaseParser.SECURITY);
					this.state = 468;
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

				this.state = 471;
				this.match(SqlBaseParser.AS);
				this.state = 472;
				this.query();
				}
				break;
			case 25:
				localctx = new RenameViewContext(this, localctx);
				this.enterOuterAlt(localctx, 25);
				{
				this.state = 474;
				this.match(SqlBaseParser.ALTER);
				this.state = 475;
				this.match(SqlBaseParser.VIEW);
				this.state = 478;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 38, this._ctx) ) {
				case 1:
					{
					this.state = 476;
					this.match(SqlBaseParser.IF);
					this.state = 477;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 480;
				(localctx as RenameViewContext)._from_ = this.qualifiedName();
				this.state = 481;
				this.match(SqlBaseParser.RENAME);
				this.state = 482;
				this.match(SqlBaseParser.TO);
				this.state = 483;
				(localctx as RenameViewContext)._to = this.qualifiedName();
				}
				break;
			case 26:
				localctx = new DropViewContext(this, localctx);
				this.enterOuterAlt(localctx, 26);
				{
				this.state = 485;
				this.match(SqlBaseParser.DROP);
				this.state = 486;
				this.match(SqlBaseParser.VIEW);
				this.state = 489;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 39, this._ctx) ) {
				case 1:
					{
					this.state = 487;
					this.match(SqlBaseParser.IF);
					this.state = 488;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 491;
				this.qualifiedName();
				}
				break;
			case 27:
				localctx = new CreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 27);
				{
				this.state = 492;
				this.match(SqlBaseParser.CREATE);
				this.state = 493;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 494;
				this.match(SqlBaseParser.VIEW);
				this.state = 498;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 40, this._ctx) ) {
				case 1:
					{
					this.state = 495;
					this.match(SqlBaseParser.IF);
					this.state = 496;
					this.match(SqlBaseParser.NOT);
					this.state = 497;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 500;
				this.qualifiedName();
				this.state = 503;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 501;
					this.match(SqlBaseParser.COMMENT);
					this.state = 502;
					this.string_();
					}
				}

				this.state = 507;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 505;
					this.match(SqlBaseParser.WITH);
					this.state = 506;
					this.properties();
					}
				}

				this.state = 509;
				this.match(SqlBaseParser.AS);
				this.state = 515;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 43, this._ctx) ) {
				case 1:
					{
					this.state = 510;
					this.query();
					}
					break;
				case 2:
					{
					this.state = 511;
					this.match(SqlBaseParser.T__1);
					this.state = 512;
					this.query();
					this.state = 513;
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
				this.state = 517;
				this.match(SqlBaseParser.DROP);
				this.state = 518;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 519;
				this.match(SqlBaseParser.VIEW);
				this.state = 522;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 44, this._ctx) ) {
				case 1:
					{
					this.state = 520;
					this.match(SqlBaseParser.IF);
					this.state = 521;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 524;
				this.qualifiedName();
				}
				break;
			case 29:
				localctx = new RefreshMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 29);
				{
				this.state = 525;
				this.match(SqlBaseParser.REFRESH);
				this.state = 526;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 527;
				this.match(SqlBaseParser.VIEW);
				this.state = 528;
				this.qualifiedName();
				this.state = 529;
				this.match(SqlBaseParser.WHERE);
				this.state = 530;
				this.booleanExpression(0);
				}
				break;
			case 30:
				localctx = new CreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 30);
				{
				this.state = 532;
				this.match(SqlBaseParser.CREATE);
				this.state = 535;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===140) {
					{
					this.state = 533;
					this.match(SqlBaseParser.OR);
					this.state = 534;
					this.match(SqlBaseParser.REPLACE);
					}
				}

				this.state = 538;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 537;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 540;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 541;
				(localctx as CreateFunctionContext)._functionName = this.qualifiedName();
				this.state = 542;
				this.match(SqlBaseParser.T__1);
				this.state = 551;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 543;
					this.sqlParameterDeclaration();
					this.state = 548;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 544;
						this.match(SqlBaseParser.T__3);
						this.state = 545;
						this.sqlParameterDeclaration();
						}
						}
						this.state = 550;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 553;
				this.match(SqlBaseParser.T__2);
				this.state = 554;
				this.match(SqlBaseParser.RETURNS);
				this.state = 555;
				(localctx as CreateFunctionContext)._returnType = this.type_(0);
				this.state = 558;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===33) {
					{
					this.state = 556;
					this.match(SqlBaseParser.COMMENT);
					this.state = 557;
					this.string_();
					}
				}

				this.state = 560;
				this.routineCharacteristics();
				this.state = 561;
				this.routineBody();
				}
				break;
			case 31:
				localctx = new AlterFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 31);
				{
				this.state = 563;
				this.match(SqlBaseParser.ALTER);
				this.state = 564;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 565;
				this.qualifiedName();
				this.state = 567;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 566;
					this.types();
					}
				}

				this.state = 569;
				this.alterRoutineCharacteristics();
				}
				break;
			case 32:
				localctx = new DropFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 32);
				{
				this.state = 571;
				this.match(SqlBaseParser.DROP);
				this.state = 573;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===196) {
					{
					this.state = 572;
					this.match(SqlBaseParser.TEMPORARY);
					}
				}

				this.state = 575;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 578;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 52, this._ctx) ) {
				case 1:
					{
					this.state = 576;
					this.match(SqlBaseParser.IF);
					this.state = 577;
					this.match(SqlBaseParser.EXISTS);
					}
					break;
				}
				this.state = 580;
				this.qualifiedName();
				this.state = 582;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 581;
					this.types();
					}
				}

				}
				break;
			case 33:
				localctx = new CallContext(this, localctx);
				this.enterOuterAlt(localctx, 33);
				{
				this.state = 584;
				this.match(SqlBaseParser.CALL);
				this.state = 585;
				this.qualifiedName();
				this.state = 586;
				this.match(SqlBaseParser.T__1);
				this.state = 595;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 587;
					this.callArgument();
					this.state = 592;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 588;
						this.match(SqlBaseParser.T__3);
						this.state = 589;
						this.callArgument();
						}
						}
						this.state = 594;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 597;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				localctx = new CreateRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 34);
				{
				this.state = 599;
				this.match(SqlBaseParser.CREATE);
				this.state = 600;
				this.match(SqlBaseParser.ROLE);
				this.state = 601;
				(localctx as CreateRoleContext)._name = this.identifier();
				this.state = 605;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 602;
					this.match(SqlBaseParser.WITH);
					this.state = 603;
					this.match(SqlBaseParser.ADMIN);
					this.state = 604;
					this.grantor();
					}
				}

				}
				break;
			case 35:
				localctx = new DropRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 35);
				{
				this.state = 607;
				this.match(SqlBaseParser.DROP);
				this.state = 608;
				this.match(SqlBaseParser.ROLE);
				this.state = 609;
				(localctx as DropRoleContext)._name = this.identifier();
				}
				break;
			case 36:
				localctx = new GrantRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 36);
				{
				this.state = 610;
				this.match(SqlBaseParser.GRANT);
				this.state = 611;
				this.roles();
				this.state = 612;
				this.match(SqlBaseParser.TO);
				this.state = 613;
				this.principal();
				this.state = 618;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 614;
					this.match(SqlBaseParser.T__3);
					this.state = 615;
					this.principal();
					}
					}
					this.state = 620;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 624;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 621;
					this.match(SqlBaseParser.WITH);
					this.state = 622;
					this.match(SqlBaseParser.ADMIN);
					this.state = 623;
					this.match(SqlBaseParser.OPTION);
					}
				}

				this.state = 629;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 626;
					this.match(SqlBaseParser.GRANTED);
					this.state = 627;
					this.match(SqlBaseParser.BY);
					this.state = 628;
					this.grantor();
					}
				}

				}
				break;
			case 37:
				localctx = new RevokeRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 37);
				{
				this.state = 631;
				this.match(SqlBaseParser.REVOKE);
				this.state = 635;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 60, this._ctx) ) {
				case 1:
					{
					this.state = 632;
					this.match(SqlBaseParser.ADMIN);
					this.state = 633;
					this.match(SqlBaseParser.OPTION);
					this.state = 634;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 637;
				this.roles();
				this.state = 638;
				this.match(SqlBaseParser.FROM);
				this.state = 639;
				this.principal();
				this.state = 644;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 640;
					this.match(SqlBaseParser.T__3);
					this.state = 641;
					this.principal();
					}
					}
					this.state = 646;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 650;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===83) {
					{
					this.state = 647;
					this.match(SqlBaseParser.GRANTED);
					this.state = 648;
					this.match(SqlBaseParser.BY);
					this.state = 649;
					this.grantor();
					}
				}

				}
				break;
			case 38:
				localctx = new SetRoleContext(this, localctx);
				this.enterOuterAlt(localctx, 38);
				{
				this.state = 652;
				this.match(SqlBaseParser.SET);
				this.state = 653;
				this.match(SqlBaseParser.ROLE);
				this.state = 657;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 63, this._ctx) ) {
				case 1:
					{
					this.state = 654;
					this.match(SqlBaseParser.ALL);
					}
					break;
				case 2:
					{
					this.state = 655;
					this.match(SqlBaseParser.NONE);
					}
					break;
				case 3:
					{
					this.state = 656;
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
				this.state = 659;
				this.match(SqlBaseParser.GRANT);
				this.state = 670;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 65, this._ctx) ) {
				case 1:
					{
					this.state = 660;
					this.privilege();
					this.state = 665;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 661;
						this.match(SqlBaseParser.T__3);
						this.state = 662;
						this.privilege();
						}
						}
						this.state = 667;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 668;
					this.match(SqlBaseParser.ALL);
					this.state = 669;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 672;
				this.match(SqlBaseParser.ON);
				this.state = 674;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 673;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 676;
				this.qualifiedName();
				this.state = 677;
				this.match(SqlBaseParser.TO);
				this.state = 678;
				(localctx as GrantContext)._grantee = this.principal();
				this.state = 682;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===224) {
					{
					this.state = 679;
					this.match(SqlBaseParser.WITH);
					this.state = 680;
					this.match(SqlBaseParser.GRANT);
					this.state = 681;
					this.match(SqlBaseParser.OPTION);
					}
				}

				}
				break;
			case 40:
				localctx = new RevokeContext(this, localctx);
				this.enterOuterAlt(localctx, 40);
				{
				this.state = 684;
				this.match(SqlBaseParser.REVOKE);
				this.state = 688;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 68, this._ctx) ) {
				case 1:
					{
					this.state = 685;
					this.match(SqlBaseParser.GRANT);
					this.state = 686;
					this.match(SqlBaseParser.OPTION);
					this.state = 687;
					this.match(SqlBaseParser.FOR);
					}
					break;
				}
				this.state = 700;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 70, this._ctx) ) {
				case 1:
					{
					this.state = 690;
					this.privilege();
					this.state = 695;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 691;
						this.match(SqlBaseParser.T__3);
						this.state = 692;
						this.privilege();
						}
						}
						this.state = 697;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
					break;
				case 2:
					{
					this.state = 698;
					this.match(SqlBaseParser.ALL);
					this.state = 699;
					this.match(SqlBaseParser.PRIVILEGES);
					}
					break;
				}
				this.state = 702;
				this.match(SqlBaseParser.ON);
				this.state = 704;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===193) {
					{
					this.state = 703;
					this.match(SqlBaseParser.TABLE);
					}
				}

				this.state = 706;
				this.qualifiedName();
				this.state = 707;
				this.match(SqlBaseParser.FROM);
				this.state = 708;
				(localctx as RevokeContext)._grantee = this.principal();
				}
				break;
			case 41:
				localctx = new ShowGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 41);
				{
				this.state = 710;
				this.match(SqlBaseParser.SHOW);
				this.state = 711;
				this.match(SqlBaseParser.GRANTS);
				this.state = 717;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===137) {
					{
					this.state = 712;
					this.match(SqlBaseParser.ON);
					this.state = 714;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===193) {
						{
						this.state = 713;
						this.match(SqlBaseParser.TABLE);
						}
					}

					this.state = 716;
					this.qualifiedName();
					}
				}

				}
				break;
			case 42:
				localctx = new ExplainContext(this, localctx);
				this.enterOuterAlt(localctx, 42);
				{
				this.state = 719;
				this.match(SqlBaseParser.EXPLAIN);
				this.state = 721;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 74, this._ctx) ) {
				case 1:
					{
					this.state = 720;
					this.match(SqlBaseParser.ANALYZE);
					}
					break;
				}
				this.state = 724;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===219) {
					{
					this.state = 723;
					this.match(SqlBaseParser.VERBOSE);
					}
				}

				this.state = 737;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 77, this._ctx) ) {
				case 1:
					{
					this.state = 726;
					this.match(SqlBaseParser.T__1);
					this.state = 727;
					this.explainOption();
					this.state = 732;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 728;
						this.match(SqlBaseParser.T__3);
						this.state = 729;
						this.explainOption();
						}
						}
						this.state = 734;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 735;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				this.state = 739;
				this.statement();
				}
				break;
			case 43:
				localctx = new ShowCreateTableContext(this, localctx);
				this.enterOuterAlt(localctx, 43);
				{
				this.state = 740;
				this.match(SqlBaseParser.SHOW);
				this.state = 741;
				this.match(SqlBaseParser.CREATE);
				this.state = 742;
				this.match(SqlBaseParser.TABLE);
				this.state = 743;
				this.qualifiedName();
				}
				break;
			case 44:
				localctx = new ShowCreateSchemaContext(this, localctx);
				this.enterOuterAlt(localctx, 44);
				{
				this.state = 744;
				this.match(SqlBaseParser.SHOW);
				this.state = 745;
				this.match(SqlBaseParser.CREATE);
				this.state = 746;
				this.match(SqlBaseParser.SCHEMA);
				this.state = 747;
				this.qualifiedName();
				}
				break;
			case 45:
				localctx = new ShowCreateViewContext(this, localctx);
				this.enterOuterAlt(localctx, 45);
				{
				this.state = 748;
				this.match(SqlBaseParser.SHOW);
				this.state = 749;
				this.match(SqlBaseParser.CREATE);
				this.state = 750;
				this.match(SqlBaseParser.VIEW);
				this.state = 751;
				this.qualifiedName();
				}
				break;
			case 46:
				localctx = new ShowCreateMaterializedViewContext(this, localctx);
				this.enterOuterAlt(localctx, 46);
				{
				this.state = 752;
				this.match(SqlBaseParser.SHOW);
				this.state = 753;
				this.match(SqlBaseParser.CREATE);
				this.state = 754;
				this.match(SqlBaseParser.MATERIALIZED);
				this.state = 755;
				this.match(SqlBaseParser.VIEW);
				this.state = 756;
				this.qualifiedName();
				}
				break;
			case 47:
				localctx = new ShowCreateFunctionContext(this, localctx);
				this.enterOuterAlt(localctx, 47);
				{
				this.state = 757;
				this.match(SqlBaseParser.SHOW);
				this.state = 758;
				this.match(SqlBaseParser.CREATE);
				this.state = 759;
				this.match(SqlBaseParser.FUNCTION);
				this.state = 760;
				this.qualifiedName();
				this.state = 762;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===2) {
					{
					this.state = 761;
					this.types();
					}
				}

				}
				break;
			case 48:
				localctx = new ShowTablesContext(this, localctx);
				this.enterOuterAlt(localctx, 48);
				{
				this.state = 764;
				this.match(SqlBaseParser.SHOW);
				this.state = 765;
				this.match(SqlBaseParser.TABLES);
				this.state = 768;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 766;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 767;
					this.qualifiedName();
					}
				}

				this.state = 776;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 770;
					this.match(SqlBaseParser.LIKE);
					this.state = 771;
					(localctx as ShowTablesContext)._pattern = this.string_();
					this.state = 774;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 772;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 773;
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
				this.state = 778;
				this.match(SqlBaseParser.SHOW);
				this.state = 779;
				this.match(SqlBaseParser.SCHEMAS);
				this.state = 782;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 780;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 781;
					this.identifier();
					}
				}

				this.state = 790;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 784;
					this.match(SqlBaseParser.LIKE);
					this.state = 785;
					(localctx as ShowSchemasContext)._pattern = this.string_();
					this.state = 788;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 786;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 787;
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
				this.state = 792;
				this.match(SqlBaseParser.SHOW);
				this.state = 793;
				this.match(SqlBaseParser.CATALOGS);
				this.state = 800;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 794;
					this.match(SqlBaseParser.LIKE);
					this.state = 795;
					(localctx as ShowCatalogsContext)._pattern = this.string_();
					this.state = 798;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 796;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 797;
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
				this.state = 802;
				this.match(SqlBaseParser.SHOW);
				this.state = 803;
				this.match(SqlBaseParser.COLUMNS);
				this.state = 804;
				_la = this._input.LA(1);
				if(!(_la===78 || _la===93)) {
				this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 805;
				this.qualifiedName();
				}
				break;
			case 52:
				localctx = new ShowStatsContext(this, localctx);
				this.enterOuterAlt(localctx, 52);
				{
				this.state = 806;
				this.match(SqlBaseParser.SHOW);
				this.state = 807;
				this.match(SqlBaseParser.STATS);
				this.state = 808;
				this.match(SqlBaseParser.FOR);
				this.state = 809;
				this.qualifiedName();
				}
				break;
			case 53:
				localctx = new ShowStatsForQueryContext(this, localctx);
				this.enterOuterAlt(localctx, 53);
				{
				this.state = 810;
				this.match(SqlBaseParser.SHOW);
				this.state = 811;
				this.match(SqlBaseParser.STATS);
				this.state = 812;
				this.match(SqlBaseParser.FOR);
				this.state = 813;
				this.match(SqlBaseParser.T__1);
				this.state = 814;
				this.querySpecification();
				this.state = 815;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 54:
				localctx = new ShowRolesContext(this, localctx);
				this.enterOuterAlt(localctx, 54);
				{
				this.state = 817;
				this.match(SqlBaseParser.SHOW);
				this.state = 819;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===40) {
					{
					this.state = 818;
					this.match(SqlBaseParser.CURRENT);
					}
				}

				this.state = 821;
				this.match(SqlBaseParser.ROLES);
				this.state = 824;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 822;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 823;
					this.identifier();
					}
				}

				}
				break;
			case 55:
				localctx = new ShowRoleGrantsContext(this, localctx);
				this.enterOuterAlt(localctx, 55);
				{
				this.state = 826;
				this.match(SqlBaseParser.SHOW);
				this.state = 827;
				this.match(SqlBaseParser.ROLE);
				this.state = 828;
				this.match(SqlBaseParser.GRANTS);
				this.state = 831;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===78 || _la===93) {
					{
					this.state = 829;
					_la = this._input.LA(1);
					if(!(_la===78 || _la===93)) {
					this._errHandler.recoverInline(this);
					}
					else {
						this._errHandler.reportMatch(this);
					    this.consume();
					}
					this.state = 830;
					this.identifier();
					}
				}

				}
				break;
			case 56:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 56);
				{
				this.state = 833;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 834;
				this.qualifiedName();
				}
				break;
			case 57:
				localctx = new ShowColumnsContext(this, localctx);
				this.enterOuterAlt(localctx, 57);
				{
				this.state = 835;
				this.match(SqlBaseParser.DESC);
				this.state = 836;
				this.qualifiedName();
				}
				break;
			case 58:
				localctx = new ShowFunctionsContext(this, localctx);
				this.enterOuterAlt(localctx, 58);
				{
				this.state = 837;
				this.match(SqlBaseParser.SHOW);
				this.state = 838;
				this.match(SqlBaseParser.FUNCTIONS);
				this.state = 845;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 839;
					this.match(SqlBaseParser.LIKE);
					this.state = 840;
					(localctx as ShowFunctionsContext)._pattern = this.string_();
					this.state = 843;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 841;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 842;
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
				this.state = 847;
				this.match(SqlBaseParser.SHOW);
				this.state = 848;
				this.match(SqlBaseParser.SESSION);
				this.state = 855;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===113) {
					{
					this.state = 849;
					this.match(SqlBaseParser.LIKE);
					this.state = 850;
					(localctx as ShowSessionContext)._pattern = this.string_();
					this.state = 853;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===63) {
						{
						this.state = 851;
						this.match(SqlBaseParser.ESCAPE);
						this.state = 852;
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
				this.state = 857;
				this.match(SqlBaseParser.SET);
				this.state = 858;
				this.match(SqlBaseParser.SESSION);
				this.state = 859;
				this.qualifiedName();
				this.state = 860;
				this.match(SqlBaseParser.EQ);
				this.state = 861;
				this.expression();
				}
				break;
			case 61:
				localctx = new ResetSessionContext(this, localctx);
				this.enterOuterAlt(localctx, 61);
				{
				this.state = 863;
				this.match(SqlBaseParser.RESET);
				this.state = 864;
				this.match(SqlBaseParser.SESSION);
				this.state = 865;
				this.qualifiedName();
				}
				break;
			case 62:
				localctx = new StartTransactionContext(this, localctx);
				this.enterOuterAlt(localctx, 62);
				{
				this.state = 866;
				this.match(SqlBaseParser.START);
				this.state = 867;
				this.match(SqlBaseParser.TRANSACTION);
				this.state = 876;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===104 || _la===155) {
					{
					this.state = 868;
					this.transactionMode();
					this.state = 873;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 869;
						this.match(SqlBaseParser.T__3);
						this.state = 870;
						this.transactionMode();
						}
						}
						this.state = 875;
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
				this.state = 878;
				this.match(SqlBaseParser.COMMIT);
				this.state = 880;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 879;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 64:
				localctx = new RollbackContext(this, localctx);
				this.enterOuterAlt(localctx, 64);
				{
				this.state = 882;
				this.match(SqlBaseParser.ROLLBACK);
				this.state = 884;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===225) {
					{
					this.state = 883;
					this.match(SqlBaseParser.WORK);
					}
				}

				}
				break;
			case 65:
				localctx = new PrepareContext(this, localctx);
				this.enterOuterAlt(localctx, 65);
				{
				this.state = 886;
				this.match(SqlBaseParser.PREPARE);
				this.state = 887;
				this.identifier();
				this.state = 888;
				this.match(SqlBaseParser.FROM);
				this.state = 889;
				this.statement();
				}
				break;
			case 66:
				localctx = new DeallocateContext(this, localctx);
				this.enterOuterAlt(localctx, 66);
				{
				this.state = 891;
				this.match(SqlBaseParser.DEALLOCATE);
				this.state = 892;
				this.match(SqlBaseParser.PREPARE);
				this.state = 893;
				this.identifier();
				}
				break;
			case 67:
				localctx = new ExecuteContext(this, localctx);
				this.enterOuterAlt(localctx, 67);
				{
				this.state = 894;
				this.match(SqlBaseParser.EXECUTE);
				this.state = 895;
				this.identifier();
				this.state = 905;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===216) {
					{
					this.state = 896;
					this.match(SqlBaseParser.USING);
					this.state = 897;
					this.expression();
					this.state = 902;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 898;
						this.match(SqlBaseParser.T__3);
						this.state = 899;
						this.expression();
						}
						}
						this.state = 904;
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
				this.state = 907;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 908;
				this.match(SqlBaseParser.INPUT);
				this.state = 909;
				this.identifier();
				}
				break;
			case 69:
				localctx = new DescribeOutputContext(this, localctx);
				this.enterOuterAlt(localctx, 69);
				{
				this.state = 910;
				this.match(SqlBaseParser.DESCRIBE);
				this.state = 911;
				this.match(SqlBaseParser.OUTPUT);
				this.state = 912;
				this.identifier();
				}
				break;
			case 70:
				localctx = new UpdateContext(this, localctx);
				this.enterOuterAlt(localctx, 70);
				{
				this.state = 913;
				this.match(SqlBaseParser.UPDATE);
				this.state = 914;
				this.qualifiedName();
				this.state = 915;
				this.match(SqlBaseParser.SET);
				this.state = 916;
				this.updateAssignment();
				this.state = 921;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 917;
					this.match(SqlBaseParser.T__3);
					this.state = 918;
					this.updateAssignment();
					}
					}
					this.state = 923;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 926;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===223) {
					{
					this.state = 924;
					this.match(SqlBaseParser.WHERE);
					this.state = 925;
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
		this.enterRule(localctx, 8, SqlBaseParser.RULE_query);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 931;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 930;
				this.with_();
				}
			}

			this.state = 933;
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
		this.enterRule(localctx, 10, SqlBaseParser.RULE_with);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 935;
			this.match(SqlBaseParser.WITH);
			this.state = 937;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===156) {
				{
				this.state = 936;
				this.match(SqlBaseParser.RECURSIVE);
				}
			}

			this.state = 939;
			this.namedQuery();
			this.state = 944;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 940;
				this.match(SqlBaseParser.T__3);
				this.state = 941;
				this.namedQuery();
				}
				}
				this.state = 946;
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
		this.enterRule(localctx, 12, SqlBaseParser.RULE_tableElement);
		try {
			this.state = 950;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 106, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 947;
				this.constraintSpecification();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 948;
				this.columnDefinition();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 949;
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
		this.enterRule(localctx, 14, SqlBaseParser.RULE_columnDefinition);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 952;
			this.identifier();
			this.state = 953;
			this.type_(0);
			this.state = 956;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===131) {
				{
				this.state = 954;
				this.match(SqlBaseParser.NOT);
				this.state = 955;
				this.match(SqlBaseParser.NULL);
				}
			}

			this.state = 960;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===33) {
				{
				this.state = 958;
				this.match(SqlBaseParser.COMMENT);
				this.state = 959;
				this.string_();
				}
			}

			this.state = 964;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===224) {
				{
				this.state = 962;
				this.match(SqlBaseParser.WITH);
				this.state = 963;
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
		this.enterRule(localctx, 16, SqlBaseParser.RULE_likeClause);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 966;
			this.match(SqlBaseParser.LIKE);
			this.state = 967;
			this.qualifiedName();
			this.state = 970;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===65 || _la===94) {
				{
				this.state = 968;
				localctx._optionType = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===65 || _la===94)) {
				    localctx._optionType = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 969;
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
		this.enterRule(localctx, 18, SqlBaseParser.RULE_properties);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 972;
			this.match(SqlBaseParser.T__1);
			this.state = 973;
			this.property();
			this.state = 978;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 974;
				this.match(SqlBaseParser.T__3);
				this.state = 975;
				this.property();
				}
				}
				this.state = 980;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
			}
			this.state = 981;
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
		this.enterRule(localctx, 20, SqlBaseParser.RULE_property);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 983;
			this.identifier();
			this.state = 984;
			this.match(SqlBaseParser.EQ);
			this.state = 985;
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
		this.enterRule(localctx, 22, SqlBaseParser.RULE_sqlParameterDeclaration);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 987;
			this.identifier();
			this.state = 988;
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
		this.enterRule(localctx, 24, SqlBaseParser.RULE_routineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 993;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===54 || _la===108 || _la===131 || _la===166) {
				{
				{
				this.state = 990;
				this.routineCharacteristic();
				}
				}
				this.state = 995;
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
		this.enterRule(localctx, 26, SqlBaseParser.RULE_routineCharacteristic);
		try {
			this.state = 1000;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 108:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 996;
				this.match(SqlBaseParser.LANGUAGE);
				this.state = 997;
				this.language();
				}
				break;
			case 54:
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 998;
				this.determinism();
				}
				break;
			case 26:
			case 166:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 999;
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
		this.enterRule(localctx, 28, SqlBaseParser.RULE_alterRoutineCharacteristics);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1005;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===26 || _la===166) {
				{
				{
				this.state = 1002;
				this.alterRoutineCharacteristic();
				}
				}
				this.state = 1007;
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
		this.enterRule(localctx, 30, SqlBaseParser.RULE_alterRoutineCharacteristic);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1008;
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
		this.enterRule(localctx, 32, SqlBaseParser.RULE_routineBody);
		try {
			this.state = 1012;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 165:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1010;
				this.returnStatement();
				}
				break;
			case 70:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1011;
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
		this.enterRule(localctx, 34, SqlBaseParser.RULE_returnStatement);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1014;
			this.match(SqlBaseParser.RETURN);
			this.state = 1015;
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
		this.enterRule(localctx, 36, SqlBaseParser.RULE_externalBodyReference);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1017;
			this.match(SqlBaseParser.EXTERNAL);
			this.state = 1020;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===122) {
				{
				this.state = 1018;
				this.match(SqlBaseParser.NAME);
				this.state = 1019;
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
		this.enterRule(localctx, 38, SqlBaseParser.RULE_language);
		try {
			this.state = 1024;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 117, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1022;
				this.match(SqlBaseParser.SQL);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1023;
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
		this.enterRule(localctx, 40, SqlBaseParser.RULE_determinism);
		try {
			this.state = 1029;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 54:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1026;
				this.match(SqlBaseParser.DETERMINISTIC);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1027;
				this.match(SqlBaseParser.NOT);
				this.state = 1028;
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
		this.enterRule(localctx, 42, SqlBaseParser.RULE_nullCallClause);
		try {
			this.state = 1040;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 166:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1031;
				this.match(SqlBaseParser.RETURNS);
				this.state = 1032;
				this.match(SqlBaseParser.NULL);
				this.state = 1033;
				this.match(SqlBaseParser.ON);
				this.state = 1034;
				this.match(SqlBaseParser.NULL);
				this.state = 1035;
				this.match(SqlBaseParser.INPUT);
				}
				break;
			case 26:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1036;
				this.match(SqlBaseParser.CALLED);
				this.state = 1037;
				this.match(SqlBaseParser.ON);
				this.state = 1038;
				this.match(SqlBaseParser.NULL);
				this.state = 1039;
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
		this.enterRule(localctx, 44, SqlBaseParser.RULE_externalRoutineName);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1042;
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
		this.enterRule(localctx, 46, SqlBaseParser.RULE_queryNoWith);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1044;
			this.queryTerm(0);
			this.state = 1055;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1045;
				this.match(SqlBaseParser.ORDER);
				this.state = 1046;
				this.match(SqlBaseParser.BY);
				this.state = 1047;
				this.sortItem();
				this.state = 1052;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1048;
					this.match(SqlBaseParser.T__3);
					this.state = 1049;
					this.sortItem();
					}
					}
					this.state = 1054;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1062;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===136) {
				{
				this.state = 1057;
				this.match(SqlBaseParser.OFFSET);
				this.state = 1058;
				localctx._offset = this.match(SqlBaseParser.INTEGER_VALUE);
				this.state = 1060;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===173 || _la===174) {
					{
					this.state = 1059;
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

			this.state = 1073;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===72 || _la===114) {
				{
				this.state = 1071;
				this._errHandler.sync(this);
				switch (this._input.LA(1)) {
				case 114:
					{
					this.state = 1064;
					this.match(SqlBaseParser.LIMIT);
					this.state = 1065;
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
					this.state = 1066;
					this.match(SqlBaseParser.FETCH);
					this.state = 1067;
					this.match(SqlBaseParser.FIRST);
					this.state = 1068;
					localctx._fetchFirstNRows = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1069;
					this.match(SqlBaseParser.ROWS);
					this.state = 1070;
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
		let _startState: number = 48;
		this.enterRecursionRule(localctx, 48, SqlBaseParser.RULE_queryTerm, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new QueryTermDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1076;
			this.queryPrimary();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1092;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 129, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1090;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 128, this._ctx) ) {
					case 1:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1078;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1079;
						(localctx as SetOperationContext)._operator = this.match(SqlBaseParser.INTERSECT);
						this.state = 1081;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1080;
							this.setQuantifier();
							}
						}

						this.state = 1083;
						(localctx as SetOperationContext)._right = this.queryTerm(3);
						}
						break;
					case 2:
						{
						localctx = new SetOperationContext(this, new QueryTermContext(this, _parentctx, _parentState));
						(localctx as SetOperationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_queryTerm);
						this.state = 1084;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1085;
						(localctx as SetOperationContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===64 || _la===210)) {
						    (localctx as SetOperationContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1087;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
						if (_la===12 || _la===56) {
							{
							this.state = 1086;
							this.setQuantifier();
							}
						}

						this.state = 1089;
						(localctx as SetOperationContext)._right = this.queryTerm(2);
						}
						break;
					}
					}
				}
				this.state = 1094;
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
		this.enterRule(localctx, 50, SqlBaseParser.RULE_queryPrimary);
		try {
			let _alt: number;
			this.state = 1111;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				localctx = new QueryPrimaryDefaultContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1095;
				this.querySpecification();
				}
				break;
			case 193:
				localctx = new TableContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1096;
				this.match(SqlBaseParser.TABLE);
				this.state = 1097;
				this.qualifiedName();
				}
				break;
			case 218:
				localctx = new InlineTableContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1098;
				this.match(SqlBaseParser.VALUES);
				this.state = 1099;
				this.expression();
				this.state = 1104;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1100;
						this.match(SqlBaseParser.T__3);
						this.state = 1101;
						this.expression();
						}
						}
					}
					this.state = 1106;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 130, this._ctx);
				}
				}
				break;
			case 2:
				localctx = new SubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1107;
				this.match(SqlBaseParser.T__1);
				this.state = 1108;
				this.queryNoWith();
				this.state = 1109;
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
		this.enterRule(localctx, 52, SqlBaseParser.RULE_sortItem);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1113;
			this.expression();
			this.state = 1115;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===19 || _la===52) {
				{
				this.state = 1114;
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

			this.state = 1119;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===134) {
				{
				this.state = 1117;
				this.match(SqlBaseParser.NULLS);
				this.state = 1118;
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
		this.enterRule(localctx, 54, SqlBaseParser.RULE_querySpecification);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1121;
			this.match(SqlBaseParser.SELECT);
			this.state = 1123;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 134, this._ctx) ) {
			case 1:
				{
				this.state = 1122;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1125;
			this.selectItem();
			this.state = 1130;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 135, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1126;
					this.match(SqlBaseParser.T__3);
					this.state = 1127;
					this.selectItem();
					}
					}
				}
				this.state = 1132;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 135, this._ctx);
			}
			this.state = 1142;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 137, this._ctx) ) {
			case 1:
				{
				this.state = 1133;
				this.match(SqlBaseParser.FROM);
				this.state = 1134;
				this.relation(0);
				this.state = 1139;
				this._errHandler.sync(this);
				_alt = this._interp.adaptivePredict(this._input, 136, this._ctx);
				while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
					if (_alt === 1) {
						{
						{
						this.state = 1135;
						this.match(SqlBaseParser.T__3);
						this.state = 1136;
						this.relation(0);
						}
						}
					}
					this.state = 1141;
					this._errHandler.sync(this);
					_alt = this._interp.adaptivePredict(this._input, 136, this._ctx);
				}
				}
				break;
			}
			this.state = 1146;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 138, this._ctx) ) {
			case 1:
				{
				this.state = 1144;
				this.match(SqlBaseParser.WHERE);
				this.state = 1145;
				localctx._where = this.booleanExpression(0);
				}
				break;
			}
			this.state = 1151;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 139, this._ctx) ) {
			case 1:
				{
				this.state = 1148;
				this.match(SqlBaseParser.GROUP);
				this.state = 1149;
				this.match(SqlBaseParser.BY);
				this.state = 1150;
				this.groupBy();
				}
				break;
			}
			this.state = 1155;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 140, this._ctx) ) {
			case 1:
				{
				this.state = 1153;
				this.match(SqlBaseParser.HAVING);
				this.state = 1154;
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
		this.enterRule(localctx, 56, SqlBaseParser.RULE_groupBy);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1158;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 141, this._ctx) ) {
			case 1:
				{
				this.state = 1157;
				this.setQuantifier();
				}
				break;
			}
			this.state = 1160;
			this.groupingElement();
			this.state = 1165;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 142, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1161;
					this.match(SqlBaseParser.T__3);
					this.state = 1162;
					this.groupingElement();
					}
					}
				}
				this.state = 1167;
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
		this.enterRule(localctx, 58, SqlBaseParser.RULE_groupingElement);
		let _la: number;
		try {
			this.state = 1208;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 148, this._ctx) ) {
			case 1:
				localctx = new SingleGroupingSetContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1168;
				this.groupingSet();
				}
				break;
			case 2:
				localctx = new RollupContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1169;
				this.match(SqlBaseParser.ROLLUP);
				this.state = 1170;
				this.match(SqlBaseParser.T__1);
				this.state = 1179;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1171;
					this.expression();
					this.state = 1176;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1172;
						this.match(SqlBaseParser.T__3);
						this.state = 1173;
						this.expression();
						}
						}
						this.state = 1178;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1181;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new CubeContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1182;
				this.match(SqlBaseParser.CUBE);
				this.state = 1183;
				this.match(SqlBaseParser.T__1);
				this.state = 1192;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1184;
					this.expression();
					this.state = 1189;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1185;
						this.match(SqlBaseParser.T__3);
						this.state = 1186;
						this.expression();
						}
						}
						this.state = 1191;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1194;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				localctx = new MultipleGroupingSetsContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1195;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1196;
				this.match(SqlBaseParser.SETS);
				this.state = 1197;
				this.match(SqlBaseParser.T__1);
				this.state = 1198;
				this.groupingSet();
				this.state = 1203;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1199;
					this.match(SqlBaseParser.T__3);
					this.state = 1200;
					this.groupingSet();
					}
					}
					this.state = 1205;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1206;
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
		this.enterRule(localctx, 60, SqlBaseParser.RULE_groupingSet);
		let _la: number;
		try {
			this.state = 1223;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 151, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1210;
				this.match(SqlBaseParser.T__1);
				this.state = 1219;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1211;
					this.expression();
					this.state = 1216;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1212;
						this.match(SqlBaseParser.T__3);
						this.state = 1213;
						this.expression();
						}
						}
						this.state = 1218;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1221;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1222;
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
		this.enterRule(localctx, 62, SqlBaseParser.RULE_namedQuery);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1225;
			localctx._name = this.identifier();
			this.state = 1227;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===2) {
				{
				this.state = 1226;
				this.columnAliases();
				}
			}

			this.state = 1229;
			this.match(SqlBaseParser.AS);
			this.state = 1230;
			this.match(SqlBaseParser.T__1);
			this.state = 1231;
			this.query();
			this.state = 1232;
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
		this.enterRule(localctx, 64, SqlBaseParser.RULE_setQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1234;
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
		this.enterRule(localctx, 66, SqlBaseParser.RULE_selectItem);
		let _la: number;
		try {
			this.state = 1248;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 155, this._ctx) ) {
			case 1:
				localctx = new SelectSingleContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1236;
				this.expression();
				this.state = 1241;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 154, this._ctx) ) {
				case 1:
					{
					this.state = 1238;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===18) {
						{
						this.state = 1237;
						this.match(SqlBaseParser.AS);
						}
					}

					this.state = 1240;
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
				this.state = 1243;
				this.qualifiedName();
				this.state = 1244;
				this.match(SqlBaseParser.T__0);
				this.state = 1245;
				this.match(SqlBaseParser.ASTERISK);
				}
				break;
			case 3:
				localctx = new SelectAllContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1247;
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
		let _startState: number = 68;
		this.enterRecursionRule(localctx, 68, SqlBaseParser.RULE_relation, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			{
			localctx = new RelationDefaultContext(this, localctx);
			this._ctx = localctx;
			_prevctx = localctx;

			this.state = 1251;
			this.sampledRelation();
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1271;
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
					this.state = 1253;
					if (!(this.precpred(this._ctx, 2))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
					}
					this.state = 1267;
					this._errHandler.sync(this);
					switch (this._input.LA(1)) {
					case 38:
						{
						this.state = 1254;
						this.match(SqlBaseParser.CROSS);
						this.state = 1255;
						this.match(SqlBaseParser.JOIN);
						this.state = 1256;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					case 79:
					case 95:
					case 106:
					case 111:
					case 168:
						{
						this.state = 1257;
						this.joinType();
						this.state = 1258;
						this.match(SqlBaseParser.JOIN);
						this.state = 1259;
						(localctx as JoinRelationContext)._rightRelation = this.relation(0);
						this.state = 1260;
						this.joinCriteria();
						}
						break;
					case 123:
						{
						this.state = 1262;
						this.match(SqlBaseParser.NATURAL);
						this.state = 1263;
						this.joinType();
						this.state = 1264;
						this.match(SqlBaseParser.JOIN);
						this.state = 1265;
						(localctx as JoinRelationContext)._right = this.sampledRelation();
						}
						break;
					default:
						throw new NoViableAltException(this);
					}
					}
					}
				}
				this.state = 1273;
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
		this.enterRule(localctx, 70, SqlBaseParser.RULE_joinType);
		let _la: number;
		try {
			this.state = 1289;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 95:
			case 106:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1275;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===95) {
					{
					this.state = 1274;
					this.match(SqlBaseParser.INNER);
					}
				}

				}
				break;
			case 111:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1277;
				this.match(SqlBaseParser.LEFT);
				this.state = 1279;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1278;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 168:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1281;
				this.match(SqlBaseParser.RIGHT);
				this.state = 1283;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1282;
					this.match(SqlBaseParser.OUTER);
					}
				}

				}
				break;
			case 79:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1285;
				this.match(SqlBaseParser.FULL);
				this.state = 1287;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===143) {
					{
					this.state = 1286;
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
		this.enterRule(localctx, 72, SqlBaseParser.RULE_joinCriteria);
		let _la: number;
		try {
			this.state = 1305;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 137:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1291;
				this.match(SqlBaseParser.ON);
				this.state = 1292;
				this.booleanExpression(0);
				}
				break;
			case 216:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1293;
				this.match(SqlBaseParser.USING);
				this.state = 1294;
				this.match(SqlBaseParser.T__1);
				this.state = 1295;
				this.identifier();
				this.state = 1300;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1296;
					this.match(SqlBaseParser.T__3);
					this.state = 1297;
					this.identifier();
					}
					}
					this.state = 1302;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1303;
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
		this.enterRule(localctx, 74, SqlBaseParser.RULE_sampledRelation);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1307;
			this.aliasedRelation();
			this.state = 1314;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 165, this._ctx) ) {
			case 1:
				{
				this.state = 1308;
				this.match(SqlBaseParser.TABLESAMPLE);
				this.state = 1309;
				this.sampleType();
				this.state = 1310;
				this.match(SqlBaseParser.T__1);
				this.state = 1311;
				localctx._percentage = this.expression();
				this.state = 1312;
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
		this.enterRule(localctx, 76, SqlBaseParser.RULE_sampleType);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1316;
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
		this.enterRule(localctx, 78, SqlBaseParser.RULE_aliasedRelation);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1318;
			this.relationPrimary();
			this.state = 1326;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 168, this._ctx) ) {
			case 1:
				{
				this.state = 1320;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===18) {
					{
					this.state = 1319;
					this.match(SqlBaseParser.AS);
					}
				}

				this.state = 1322;
				this.identifier();
				this.state = 1324;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 167, this._ctx) ) {
				case 1:
					{
					this.state = 1323;
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
		this.enterRule(localctx, 80, SqlBaseParser.RULE_columnAliases);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
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
		}
		catch (re) {
			if (re instanceof RecognitionException) {
				localctx.exception = re;
				this._errHandler.reportError(this, re);
				this._errHandler.recover(this, re);
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
		this.enterRule(localctx, 82, SqlBaseParser.RULE_relationPrimary);
		let _la: number;
		try {
			this.state = 1371;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 173, this._ctx) ) {
			case 1:
				localctx = new TableNameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1339;
				this.qualifiedName();
				this.state = 1341;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 170, this._ctx) ) {
				case 1:
					{
					this.state = 1340;
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
				this.state = 1343;
				this.match(SqlBaseParser.T__1);
				this.state = 1344;
				this.query();
				this.state = 1345;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new UnnestContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1347;
				this.match(SqlBaseParser.UNNEST);
				this.state = 1348;
				this.match(SqlBaseParser.T__1);
				this.state = 1349;
				this.expression();
				this.state = 1354;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1350;
					this.match(SqlBaseParser.T__3);
					this.state = 1351;
					this.expression();
					}
					}
					this.state = 1356;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1357;
				this.match(SqlBaseParser.T__2);
				this.state = 1360;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 172, this._ctx) ) {
				case 1:
					{
					this.state = 1358;
					this.match(SqlBaseParser.WITH);
					this.state = 1359;
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
				this.state = 1362;
				this.match(SqlBaseParser.LATERAL);
				this.state = 1363;
				this.match(SqlBaseParser.T__1);
				this.state = 1364;
				this.query();
				this.state = 1365;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new ParenthesizedRelationContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1367;
				this.match(SqlBaseParser.T__1);
				this.state = 1368;
				this.relation(0);
				this.state = 1369;
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
		this.enterRule(localctx, 84, SqlBaseParser.RULE_expression);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1373;
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
		let _startState: number = 86;
		this.enterRecursionRule(localctx, 86, SqlBaseParser.RULE_booleanExpression, _p);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1382;
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

				this.state = 1376;
				(localctx as PredicatedContext)._valueExpression = this.valueExpression(0);
				this.state = 1378;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 174, this._ctx) ) {
				case 1:
					{
					this.state = 1377;
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
				this.state = 1380;
				this.match(SqlBaseParser.NOT);
				this.state = 1381;
				this.booleanExpression(3);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1392;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 177, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1390;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 176, this._ctx) ) {
					case 1:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1384;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1385;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.AND);
						this.state = 1386;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(3);
						}
						break;
					case 2:
						{
						localctx = new LogicalBinaryContext(this, new BooleanExpressionContext(this, _parentctx, _parentState));
						(localctx as LogicalBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_booleanExpression);
						this.state = 1387;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1388;
						(localctx as LogicalBinaryContext)._operator = this.match(SqlBaseParser.OR);
						this.state = 1389;
						(localctx as LogicalBinaryContext)._right = this.booleanExpression(2);
						}
						break;
					}
					}
				}
				this.state = 1394;
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
		this.enterRule(localctx, 88, SqlBaseParser.RULE_predicate);
		let _la: number;
		try {
			this.state = 1456;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 186, this._ctx) ) {
			case 1:
				localctx = new ComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1395;
				this.comparisonOperator();
				this.state = 1396;
				(localctx as ComparisonContext)._right = this.valueExpression(0);
				}
				break;
			case 2:
				localctx = new QuantifiedComparisonContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1398;
				this.comparisonOperator();
				this.state = 1399;
				this.comparisonQuantifier();
				this.state = 1400;
				this.match(SqlBaseParser.T__1);
				this.state = 1401;
				this.query();
				this.state = 1402;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 3:
				localctx = new BetweenContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1405;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1404;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1407;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1408;
				(localctx as BetweenContext)._lower = this.valueExpression(0);
				this.state = 1409;
				this.match(SqlBaseParser.AND);
				this.state = 1410;
				(localctx as BetweenContext)._upper = this.valueExpression(0);
				}
				break;
			case 4:
				localctx = new InListContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1413;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1412;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1415;
				this.match(SqlBaseParser.IN);
				this.state = 1416;
				this.match(SqlBaseParser.T__1);
				this.state = 1417;
				this.expression();
				this.state = 1422;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1418;
					this.match(SqlBaseParser.T__3);
					this.state = 1419;
					this.expression();
					}
					}
					this.state = 1424;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1425;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 5:
				localctx = new InSubqueryContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1428;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1427;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1430;
				this.match(SqlBaseParser.IN);
				this.state = 1431;
				this.match(SqlBaseParser.T__1);
				this.state = 1432;
				this.query();
				this.state = 1433;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 6:
				localctx = new LikeContext(this, localctx);
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1436;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1435;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1438;
				this.match(SqlBaseParser.LIKE);
				this.state = 1439;
				(localctx as LikeContext)._pattern = this.valueExpression(0);
				this.state = 1442;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 183, this._ctx) ) {
				case 1:
					{
					this.state = 1440;
					this.match(SqlBaseParser.ESCAPE);
					this.state = 1441;
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
				this.state = 1444;
				this.match(SqlBaseParser.IS);
				this.state = 1446;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===131) {
					{
					this.state = 1445;
					this.match(SqlBaseParser.NOT);
					}
				}

				this.state = 1448;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 8:
				localctx = new DistinctFromContext(this, localctx);
				this.enterOuterAlt(localctx, 8);
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
				this.match(SqlBaseParser.DISTINCT);
				this.state = 1454;
				this.match(SqlBaseParser.FROM);
				this.state = 1455;
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
		let _startState: number = 90;
		this.enterRecursionRule(localctx, 90, SqlBaseParser.RULE_valueExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1462;
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

				this.state = 1459;
				this.primaryExpression(0);
				}
				break;
			case 235:
			case 236:
				{
				localctx = new ArithmeticUnaryContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1460;
				(localctx as ArithmeticUnaryContext)._operator = this._input.LT(1);
				_la = this._input.LA(1);
				if(!(_la===235 || _la===236)) {
				    (localctx as ArithmeticUnaryContext)._operator = this._errHandler.recoverInline(this);
				}
				else {
					this._errHandler.reportMatch(this);
				    this.consume();
				}
				this.state = 1461;
				this.valueExpression(4);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1478;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 189, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1476;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 188, this._ctx) ) {
					case 1:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1464;
						if (!(this.precpred(this._ctx, 3))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 3)");
						}
						this.state = 1465;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(((((_la - 237)) & ~0x1F) === 0 && ((1 << (_la - 237)) & 7) !== 0))) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1466;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(4);
						}
						break;
					case 2:
						{
						localctx = new ArithmeticBinaryContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ArithmeticBinaryContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1467;
						if (!(this.precpred(this._ctx, 2))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 2)");
						}
						this.state = 1468;
						(localctx as ArithmeticBinaryContext)._operator = this._input.LT(1);
						_la = this._input.LA(1);
						if(!(_la===235 || _la===236)) {
						    (localctx as ArithmeticBinaryContext)._operator = this._errHandler.recoverInline(this);
						}
						else {
							this._errHandler.reportMatch(this);
						    this.consume();
						}
						this.state = 1469;
						(localctx as ArithmeticBinaryContext)._right = this.valueExpression(3);
						}
						break;
					case 3:
						{
						localctx = new ConcatenationContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						(localctx as ConcatenationContext)._left = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1470;
						if (!(this.precpred(this._ctx, 1))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 1)");
						}
						this.state = 1471;
						this.match(SqlBaseParser.CONCAT);
						this.state = 1472;
						(localctx as ConcatenationContext)._right = this.valueExpression(2);
						}
						break;
					case 4:
						{
						localctx = new AtTimeZoneContext(this, new ValueExpressionContext(this, _parentctx, _parentState));
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_valueExpression);
						this.state = 1473;
						if (!(this.precpred(this._ctx, 5))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 5)");
						}
						this.state = 1474;
						this.match(SqlBaseParser.AT);
						this.state = 1475;
						this.timeZoneSpecifier();
						}
						break;
					}
					}
				}
				this.state = 1480;
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
		let _startState: number = 92;
		this.enterRecursionRule(localctx, 92, SqlBaseParser.RULE_primaryExpression, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1720;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 218, this._ctx) ) {
			case 1:
				{
				localctx = new NullLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;

				this.state = 1482;
				this.match(SqlBaseParser.NULL);
				}
				break;
			case 2:
				{
				localctx = new IntervalLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1483;
				this.interval();
				}
				break;
			case 3:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1484;
				this.identifier();
				this.state = 1485;
				this.string_();
				}
				break;
			case 4:
				{
				localctx = new TypeConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1487;
				this.match(SqlBaseParser.DOUBLE_PRECISION);
				this.state = 1488;
				this.string_();
				}
				break;
			case 5:
				{
				localctx = new NumericLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1489;
				this.number_();
				}
				break;
			case 6:
				{
				localctx = new BooleanLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1490;
				this.booleanValue();
				}
				break;
			case 7:
				{
				localctx = new StringLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1491;
				this.string_();
				}
				break;
			case 8:
				{
				localctx = new BinaryLiteralContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1492;
				this.match(SqlBaseParser.BINARY_LITERAL);
				}
				break;
			case 9:
				{
				localctx = new ParameterContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1493;
				this.match(SqlBaseParser.T__4);
				}
				break;
			case 10:
				{
				localctx = new PositionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1494;
				this.match(SqlBaseParser.POSITION);
				this.state = 1495;
				this.match(SqlBaseParser.T__1);
				this.state = 1496;
				this.valueExpression(0);
				this.state = 1497;
				this.match(SqlBaseParser.IN);
				this.state = 1498;
				this.valueExpression(0);
				this.state = 1499;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 11:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1501;
				this.match(SqlBaseParser.T__1);
				this.state = 1502;
				this.expression();
				this.state = 1505;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1503;
					this.match(SqlBaseParser.T__3);
					this.state = 1504;
					this.expression();
					}
					}
					this.state = 1507;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===4);
				this.state = 1509;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 12:
				{
				localctx = new RowConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1511;
				this.match(SqlBaseParser.ROW);
				this.state = 1512;
				this.match(SqlBaseParser.T__1);
				this.state = 1513;
				this.expression();
				this.state = 1518;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1514;
					this.match(SqlBaseParser.T__3);
					this.state = 1515;
					this.expression();
					}
					}
					this.state = 1520;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1521;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 13:
				{
				localctx = new FunctionCallContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1523;
				this.qualifiedName();
				this.state = 1524;
				this.match(SqlBaseParser.T__1);
				this.state = 1525;
				this.match(SqlBaseParser.ASTERISK);
				this.state = 1526;
				this.match(SqlBaseParser.T__2);
				this.state = 1528;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 192, this._ctx) ) {
				case 1:
					{
					this.state = 1527;
					this.filter();
					}
					break;
				}
				this.state = 1531;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 193, this._ctx) ) {
				case 1:
					{
					this.state = 1530;
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
				this.state = 1533;
				this.qualifiedName();
				this.state = 1534;
				this.match(SqlBaseParser.T__1);
				this.state = 1546;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1406533391) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1536;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 194, this._ctx) ) {
					case 1:
						{
						this.state = 1535;
						this.setQuantifier();
						}
						break;
					}
					this.state = 1538;
					this.expression();
					this.state = 1543;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1539;
						this.match(SqlBaseParser.T__3);
						this.state = 1540;
						this.expression();
						}
						}
						this.state = 1545;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1558;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===141) {
					{
					this.state = 1548;
					this.match(SqlBaseParser.ORDER);
					this.state = 1549;
					this.match(SqlBaseParser.BY);
					this.state = 1550;
					this.sortItem();
					this.state = 1555;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1551;
						this.match(SqlBaseParser.T__3);
						this.state = 1552;
						this.sortItem();
						}
						}
						this.state = 1557;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1560;
				this.match(SqlBaseParser.T__2);
				this.state = 1562;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 199, this._ctx) ) {
				case 1:
					{
					this.state = 1561;
					this.filter();
					}
					break;
				}
				this.state = 1568;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 201, this._ctx) ) {
				case 1:
					{
					this.state = 1565;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					if (_la===92 || _la===163) {
						{
						this.state = 1564;
						this.nullTreatment();
						}
					}

					this.state = 1567;
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
				this.state = 1570;
				this.identifier();
				this.state = 1571;
				this.match(SqlBaseParser.T__5);
				this.state = 1572;
				this.expression();
				}
				break;
			case 16:
				{
				localctx = new LambdaContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1574;
				this.match(SqlBaseParser.T__1);
				this.state = 1583;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1575;
					this.identifier();
					this.state = 1580;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1576;
						this.match(SqlBaseParser.T__3);
						this.state = 1577;
						this.identifier();
						}
						}
						this.state = 1582;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1585;
				this.match(SqlBaseParser.T__2);
				this.state = 1586;
				this.match(SqlBaseParser.T__5);
				this.state = 1587;
				this.expression();
				}
				break;
			case 17:
				{
				localctx = new SubqueryExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1588;
				this.match(SqlBaseParser.T__1);
				this.state = 1589;
				this.query();
				this.state = 1590;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 18:
				{
				localctx = new ExistsContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1592;
				this.match(SqlBaseParser.EXISTS);
				this.state = 1593;
				this.match(SqlBaseParser.T__1);
				this.state = 1594;
				this.query();
				this.state = 1595;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 19:
				{
				localctx = new SimpleCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1597;
				this.match(SqlBaseParser.CASE);
				this.state = 1598;
				this.valueExpression(0);
				this.state = 1600;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1599;
					this.whenClause();
					}
					}
					this.state = 1602;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1606;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1604;
					this.match(SqlBaseParser.ELSE);
					this.state = 1605;
					(localctx as SimpleCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1608;
				this.match(SqlBaseParser.END);
				}
				break;
			case 20:
				{
				localctx = new SearchedCaseContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1610;
				this.match(SqlBaseParser.CASE);
				this.state = 1612;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				do {
					{
					{
					this.state = 1611;
					this.whenClause();
					}
					}
					this.state = 1614;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				} while (_la===222);
				this.state = 1618;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===59) {
					{
					this.state = 1616;
					this.match(SqlBaseParser.ELSE);
					this.state = 1617;
					(localctx as SearchedCaseContext)._elseExpression = this.expression();
					}
				}

				this.state = 1620;
				this.match(SqlBaseParser.END);
				}
				break;
			case 21:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1622;
				this.match(SqlBaseParser.CAST);
				this.state = 1623;
				this.match(SqlBaseParser.T__1);
				this.state = 1624;
				this.expression();
				this.state = 1625;
				this.match(SqlBaseParser.AS);
				this.state = 1626;
				this.type_(0);
				this.state = 1627;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 22:
				{
				localctx = new CastContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1629;
				this.match(SqlBaseParser.TRY_CAST);
				this.state = 1630;
				this.match(SqlBaseParser.T__1);
				this.state = 1631;
				this.expression();
				this.state = 1632;
				this.match(SqlBaseParser.AS);
				this.state = 1633;
				this.type_(0);
				this.state = 1634;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 23:
				{
				localctx = new ArrayConstructorContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1636;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1637;
				this.match(SqlBaseParser.T__6);
				this.state = 1646;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 4269497380) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389756175) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2933889021) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 4278169453) !== 0) || ((((_la - 131)) & ~0x1F) === 0 && ((1 << (_la - 131)) & 4260882879) !== 0) || ((((_la - 163)) & ~0x1F) === 0 && ((1 << (_la - 163)) & 3221159391) !== 0) || ((((_la - 195)) & ~0x1F) === 0 && ((1 << (_la - 195)) & 3344789495) !== 0) || ((((_la - 227)) & ~0x1F) === 0 && ((1 << (_la - 227)) & 83870467) !== 0)) {
					{
					this.state = 1638;
					this.expression();
					this.state = 1643;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1639;
						this.match(SqlBaseParser.T__3);
						this.state = 1640;
						this.expression();
						}
						}
						this.state = 1645;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1648;
				this.match(SqlBaseParser.T__7);
				}
				break;
			case 24:
				{
				localctx = new ColumnReferenceContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1649;
				this.identifier();
				}
				break;
			case 25:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1650;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_DATE);
				}
				break;
			case 26:
				{
				localctx = new SpecialDateTimeFunctionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1651;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIME);
				this.state = 1655;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 210, this._ctx) ) {
				case 1:
					{
					this.state = 1652;
					this.match(SqlBaseParser.T__1);
					this.state = 1653;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1654;
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
				this.state = 1657;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.CURRENT_TIMESTAMP);
				this.state = 1661;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 211, this._ctx) ) {
				case 1:
					{
					this.state = 1658;
					this.match(SqlBaseParser.T__1);
					this.state = 1659;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1660;
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
				this.state = 1663;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIME);
				this.state = 1667;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 212, this._ctx) ) {
				case 1:
					{
					this.state = 1664;
					this.match(SqlBaseParser.T__1);
					this.state = 1665;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1666;
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
				this.state = 1669;
				(localctx as SpecialDateTimeFunctionContext)._name = this.match(SqlBaseParser.LOCALTIMESTAMP);
				this.state = 1673;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 213, this._ctx) ) {
				case 1:
					{
					this.state = 1670;
					this.match(SqlBaseParser.T__1);
					this.state = 1671;
					(localctx as SpecialDateTimeFunctionContext)._precision = this.match(SqlBaseParser.INTEGER_VALUE);
					this.state = 1672;
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
				this.state = 1675;
				(localctx as CurrentUserContext)._name = this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 31:
				{
				localctx = new SubstringContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1676;
				this.match(SqlBaseParser.SUBSTRING);
				this.state = 1677;
				this.match(SqlBaseParser.T__1);
				this.state = 1678;
				this.valueExpression(0);
				this.state = 1679;
				this.match(SqlBaseParser.FROM);
				this.state = 1680;
				this.valueExpression(0);
				this.state = 1683;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===76) {
					{
					this.state = 1681;
					this.match(SqlBaseParser.FOR);
					this.state = 1682;
					this.valueExpression(0);
					}
				}

				this.state = 1685;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 32:
				{
				localctx = new NormalizeContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1687;
				this.match(SqlBaseParser.NORMALIZE);
				this.state = 1688;
				this.match(SqlBaseParser.T__1);
				this.state = 1689;
				this.valueExpression(0);
				this.state = 1692;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if (_la===4) {
					{
					this.state = 1690;
					this.match(SqlBaseParser.T__3);
					this.state = 1691;
					this.normalForm();
					}
				}

				this.state = 1694;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 33:
				{
				localctx = new ExtractContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1696;
				this.match(SqlBaseParser.EXTRACT);
				this.state = 1697;
				this.match(SqlBaseParser.T__1);
				this.state = 1698;
				this.identifier();
				this.state = 1699;
				this.match(SqlBaseParser.FROM);
				this.state = 1700;
				this.valueExpression(0);
				this.state = 1701;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 34:
				{
				localctx = new ParenthesizedExpressionContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1703;
				this.match(SqlBaseParser.T__1);
				this.state = 1704;
				this.expression();
				this.state = 1705;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 35:
				{
				localctx = new GroupingOperationContext(this, localctx);
				this._ctx = localctx;
				_prevctx = localctx;
				this.state = 1707;
				this.match(SqlBaseParser.GROUPING);
				this.state = 1708;
				this.match(SqlBaseParser.T__1);
				this.state = 1717;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 15) !== 0)) {
					{
					this.state = 1709;
					this.qualifiedName();
					this.state = 1714;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1710;
						this.match(SqlBaseParser.T__3);
						this.state = 1711;
						this.qualifiedName();
						}
						}
						this.state = 1716;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					}
				}

				this.state = 1719;
				this.match(SqlBaseParser.T__2);
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1732;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 220, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					if (this._parseListeners != null) {
						this.triggerExitRuleEvent();
					}
					_prevctx = localctx;
					{
					this.state = 1730;
					this._errHandler.sync(this);
					switch ( this._interp.adaptivePredict(this._input, 219, this._ctx) ) {
					case 1:
						{
						localctx = new SubscriptContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as SubscriptContext)._value = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1722;
						if (!(this.precpred(this._ctx, 14))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 14)");
						}
						this.state = 1723;
						this.match(SqlBaseParser.T__6);
						this.state = 1724;
						(localctx as SubscriptContext)._index = this.valueExpression(0);
						this.state = 1725;
						this.match(SqlBaseParser.T__7);
						}
						break;
					case 2:
						{
						localctx = new DereferenceContext(this, new PrimaryExpressionContext(this, _parentctx, _parentState));
						(localctx as DereferenceContext)._base = _prevctx;
						this.pushNewRecursionContext(localctx, _startState, SqlBaseParser.RULE_primaryExpression);
						this.state = 1727;
						if (!(this.precpred(this._ctx, 12))) {
							throw this.createFailedPredicateException("this.precpred(this._ctx, 12)");
						}
						this.state = 1728;
						this.match(SqlBaseParser.T__0);
						this.state = 1729;
						(localctx as DereferenceContext)._fieldName = this.identifier();
						}
						break;
					}
					}
				}
				this.state = 1734;
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
		this.enterRule(localctx, 94, SqlBaseParser.RULE_string);
		try {
			this.state = 1741;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 241:
				localctx = new BasicStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1735;
				this.match(SqlBaseParser.STRING);
				}
				break;
			case 242:
				localctx = new UnicodeStringLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1736;
				this.match(SqlBaseParser.UNICODE_STRING);
				this.state = 1739;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 221, this._ctx) ) {
				case 1:
					{
					this.state = 1737;
					this.match(SqlBaseParser.UESCAPE);
					this.state = 1738;
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
		this.enterRule(localctx, 96, SqlBaseParser.RULE_nullTreatment);
		try {
			this.state = 1747;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 92:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1743;
				this.match(SqlBaseParser.IGNORE);
				this.state = 1744;
				this.match(SqlBaseParser.NULLS);
				}
				break;
			case 163:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1745;
				this.match(SqlBaseParser.RESPECT);
				this.state = 1746;
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
		this.enterRule(localctx, 98, SqlBaseParser.RULE_timeZoneSpecifier);
		try {
			this.state = 1755;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 224, this._ctx) ) {
			case 1:
				localctx = new TimeZoneIntervalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1749;
				this.match(SqlBaseParser.TIME);
				this.state = 1750;
				this.match(SqlBaseParser.ZONE);
				this.state = 1751;
				this.interval();
				}
				break;
			case 2:
				localctx = new TimeZoneStringContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1752;
				this.match(SqlBaseParser.TIME);
				this.state = 1753;
				this.match(SqlBaseParser.ZONE);
				this.state = 1754;
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
		this.enterRule(localctx, 100, SqlBaseParser.RULE_comparisonOperator);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1757;
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
		this.enterRule(localctx, 102, SqlBaseParser.RULE_comparisonQuantifier);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1759;
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
		this.enterRule(localctx, 104, SqlBaseParser.RULE_booleanValue);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1761;
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
		this.enterRule(localctx, 106, SqlBaseParser.RULE_interval);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1763;
			this.match(SqlBaseParser.INTERVAL);
			this.state = 1765;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===235 || _la===236) {
				{
				this.state = 1764;
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

			this.state = 1767;
			this.string_();
			this.state = 1768;
			localctx._from_ = this.intervalField();
			this.state = 1771;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 226, this._ctx) ) {
			case 1:
				{
				this.state = 1769;
				this.match(SqlBaseParser.TO);
				this.state = 1770;
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
		this.enterRule(localctx, 108, SqlBaseParser.RULE_intervalField);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1773;
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
		this.enterRule(localctx, 110, SqlBaseParser.RULE_normalForm);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1775;
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
		this.enterRule(localctx, 112, SqlBaseParser.RULE_types);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1777;
			this.match(SqlBaseParser.T__1);
			this.state = 1786;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if ((((_la) & ~0x1F) === 0 && ((1 << _la) & 3464190976) !== 0) || ((((_la - 32)) & ~0x1F) === 0 && ((1 << (_la - 32)) & 1389741327) !== 0) || ((((_la - 65)) & ~0x1F) === 0 && ((1 << (_la - 65)) & 2929694633) !== 0) || ((((_la - 99)) & ~0x1F) === 0 && ((1 << (_la - 99)) & 2130489197) !== 0) || ((((_la - 133)) & ~0x1F) === 0 && ((1 << (_la - 133)) & 4286446191) !== 0) || ((((_la - 165)) & ~0x1F) === 0 && ((1 << (_la - 165)) & 4026515319) !== 0) || ((((_la - 197)) & ~0x1F) === 0 && ((1 << (_la - 197)) & 4057422781) !== 0) || ((((_la - 247)) & ~0x1F) === 0 && ((1 << (_la - 247)) & 127) !== 0)) {
				{
				this.state = 1778;
				this.type_(0);
				this.state = 1783;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1779;
					this.match(SqlBaseParser.T__3);
					this.state = 1780;
					this.type_(0);
					}
					}
					this.state = 1785;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1788;
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
		let _startState: number = 114;
		this.enterRecursionRule(localctx, 114, SqlBaseParser.RULE_type, _p);
		let _la: number;
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1837;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 232, this._ctx) ) {
			case 1:
				{
				this.state = 1791;
				this.match(SqlBaseParser.ARRAY);
				this.state = 1792;
				this.match(SqlBaseParser.LT);
				this.state = 1793;
				this.type_(0);
				this.state = 1794;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 2:
				{
				this.state = 1796;
				this.match(SqlBaseParser.MAP);
				this.state = 1797;
				this.match(SqlBaseParser.LT);
				this.state = 1798;
				this.type_(0);
				this.state = 1799;
				this.match(SqlBaseParser.T__3);
				this.state = 1800;
				this.type_(0);
				this.state = 1801;
				this.match(SqlBaseParser.GT);
				}
				break;
			case 3:
				{
				this.state = 1803;
				this.match(SqlBaseParser.ROW);
				this.state = 1804;
				this.match(SqlBaseParser.T__1);
				this.state = 1805;
				this.identifier();
				this.state = 1806;
				this.type_(0);
				this.state = 1813;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1807;
					this.match(SqlBaseParser.T__3);
					this.state = 1808;
					this.identifier();
					this.state = 1809;
					this.type_(0);
					}
					}
					this.state = 1815;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				this.state = 1816;
				this.match(SqlBaseParser.T__2);
				}
				break;
			case 4:
				{
				this.state = 1818;
				this.baseType();
				this.state = 1830;
				this._errHandler.sync(this);
				switch ( this._interp.adaptivePredict(this._input, 231, this._ctx) ) {
				case 1:
					{
					this.state = 1819;
					this.match(SqlBaseParser.T__1);
					this.state = 1820;
					this.typeParameter();
					this.state = 1825;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
					while (_la===4) {
						{
						{
						this.state = 1821;
						this.match(SqlBaseParser.T__3);
						this.state = 1822;
						this.typeParameter();
						}
						}
						this.state = 1827;
						this._errHandler.sync(this);
						_la = this._input.LA(1);
					}
					this.state = 1828;
					this.match(SqlBaseParser.T__2);
					}
					break;
				}
				}
				break;
			case 5:
				{
				this.state = 1832;
				this.match(SqlBaseParser.INTERVAL);
				this.state = 1833;
				localctx._from_ = this.intervalField();
				this.state = 1834;
				this.match(SqlBaseParser.TO);
				this.state = 1835;
				localctx._to = this.intervalField();
				}
				break;
			}
			this._ctx.stop = this._input.LT(-1);
			this.state = 1843;
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
					this.state = 1839;
					if (!(this.precpred(this._ctx, 6))) {
						throw this.createFailedPredicateException("this.precpred(this._ctx, 6)");
					}
					this.state = 1840;
					this.match(SqlBaseParser.ARRAY);
					}
					}
				}
				this.state = 1845;
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
		this.enterRule(localctx, 116, SqlBaseParser.RULE_typeParameter);
		try {
			this.state = 1848;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 244:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1846;
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
				this.state = 1847;
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
		this.enterRule(localctx, 118, SqlBaseParser.RULE_baseType);
		try {
			this.state = 1854;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 251:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1850;
				this.match(SqlBaseParser.TIME_WITH_TIME_ZONE);
				}
				break;
			case 252:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1851;
				this.match(SqlBaseParser.TIMESTAMP_WITH_TIME_ZONE);
				}
				break;
			case 253:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1852;
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
				this.state = 1853;
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
		this.enterRule(localctx, 120, SqlBaseParser.RULE_whenClause);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1856;
			this.match(SqlBaseParser.WHEN);
			this.state = 1857;
			localctx._condition = this.expression();
			this.state = 1858;
			this.match(SqlBaseParser.THEN);
			this.state = 1859;
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
		this.enterRule(localctx, 122, SqlBaseParser.RULE_filter);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1861;
			this.match(SqlBaseParser.FILTER);
			this.state = 1862;
			this.match(SqlBaseParser.T__1);
			this.state = 1863;
			this.match(SqlBaseParser.WHERE);
			this.state = 1864;
			this.booleanExpression(0);
			this.state = 1865;
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
		this.enterRule(localctx, 124, SqlBaseParser.RULE_over);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1867;
			this.match(SqlBaseParser.OVER);
			this.state = 1868;
			this.match(SqlBaseParser.T__1);
			this.state = 1879;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===146) {
				{
				this.state = 1869;
				this.match(SqlBaseParser.PARTITION);
				this.state = 1870;
				this.match(SqlBaseParser.BY);
				this.state = 1871;
				localctx._expression = this.expression();
				localctx._partition.push(localctx._expression);
				this.state = 1876;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1872;
					this.match(SqlBaseParser.T__3);
					this.state = 1873;
					localctx._expression = this.expression();
					localctx._partition.push(localctx._expression);
					}
					}
					this.state = 1878;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1891;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===141) {
				{
				this.state = 1881;
				this.match(SqlBaseParser.ORDER);
				this.state = 1882;
				this.match(SqlBaseParser.BY);
				this.state = 1883;
				this.sortItem();
				this.state = 1888;
				this._errHandler.sync(this);
				_la = this._input.LA(1);
				while (_la===4) {
					{
					{
					this.state = 1884;
					this.match(SqlBaseParser.T__3);
					this.state = 1885;
					this.sortItem();
					}
					}
					this.state = 1890;
					this._errHandler.sync(this);
					_la = this._input.LA(1);
				}
				}
			}

			this.state = 1894;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			if (_la===88 || _la===154 || _la===174) {
				{
				this.state = 1893;
				this.windowFrame();
				}
			}

			this.state = 1896;
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
		this.enterRule(localctx, 126, SqlBaseParser.RULE_windowFrame);
		try {
			this.state = 1922;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 241, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1898;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1899;
				localctx._start = this.frameBound();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1900;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1901;
				localctx._start = this.frameBound();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1902;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1903;
				localctx._start = this.frameBound();
				}
				break;
			case 4:
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1904;
				localctx._frameType = this.match(SqlBaseParser.RANGE);
				this.state = 1905;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1906;
				localctx._start = this.frameBound();
				this.state = 1907;
				this.match(SqlBaseParser.AND);
				this.state = 1908;
				localctx._end = this.frameBound();
				}
				break;
			case 5:
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 1910;
				localctx._frameType = this.match(SqlBaseParser.ROWS);
				this.state = 1911;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1912;
				localctx._start = this.frameBound();
				this.state = 1913;
				this.match(SqlBaseParser.AND);
				this.state = 1914;
				localctx._end = this.frameBound();
				}
				break;
			case 6:
				this.enterOuterAlt(localctx, 6);
				{
				this.state = 1916;
				localctx._frameType = this.match(SqlBaseParser.GROUPS);
				this.state = 1917;
				this.match(SqlBaseParser.BETWEEN);
				this.state = 1918;
				localctx._start = this.frameBound();
				this.state = 1919;
				this.match(SqlBaseParser.AND);
				this.state = 1920;
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
		this.enterRule(localctx, 128, SqlBaseParser.RULE_frameBound);
		let _la: number;
		try {
			this.state = 1933;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 242, this._ctx) ) {
			case 1:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1924;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1925;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.PRECEDING);
				}
				break;
			case 2:
				localctx = new UnboundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1926;
				this.match(SqlBaseParser.UNBOUNDED);
				this.state = 1927;
				(localctx as UnboundedFrameContext)._boundType = this.match(SqlBaseParser.FOLLOWING);
				}
				break;
			case 3:
				localctx = new CurrentRowBoundContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1928;
				this.match(SqlBaseParser.CURRENT);
				this.state = 1929;
				this.match(SqlBaseParser.ROW);
				}
				break;
			case 4:
				localctx = new BoundedFrameContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1930;
				this.expression();
				this.state = 1931;
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
		this.enterRule(localctx, 130, SqlBaseParser.RULE_updateAssignment);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1935;
			this.identifier();
			this.state = 1936;
			this.match(SqlBaseParser.EQ);
			this.state = 1937;
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
		this.enterRule(localctx, 132, SqlBaseParser.RULE_explainOption);
		let _la: number;
		try {
			this.state = 1943;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 77:
				localctx = new ExplainFormatContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1939;
				this.match(SqlBaseParser.FORMAT);
				this.state = 1940;
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
				this.state = 1941;
				this.match(SqlBaseParser.TYPE);
				this.state = 1942;
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
		this.enterRule(localctx, 134, SqlBaseParser.RULE_transactionMode);
		let _la: number;
		try {
			this.state = 1950;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 104:
				localctx = new IsolationLevelContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1945;
				this.match(SqlBaseParser.ISOLATION);
				this.state = 1946;
				this.match(SqlBaseParser.LEVEL);
				this.state = 1947;
				this.levelOfIsolation();
				}
				break;
			case 155:
				localctx = new TransactionAccessModeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1948;
				this.match(SqlBaseParser.READ);
				this.state = 1949;
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
		this.enterRule(localctx, 136, SqlBaseParser.RULE_levelOfIsolation);
		try {
			this.state = 1959;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 245, this._ctx) ) {
			case 1:
				localctx = new ReadUncommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1952;
				this.match(SqlBaseParser.READ);
				this.state = 1953;
				this.match(SqlBaseParser.UNCOMMITTED);
				}
				break;
			case 2:
				localctx = new ReadCommittedContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1954;
				this.match(SqlBaseParser.READ);
				this.state = 1955;
				this.match(SqlBaseParser.COMMITTED);
				}
				break;
			case 3:
				localctx = new RepeatableReadContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1956;
				this.match(SqlBaseParser.REPEATABLE);
				this.state = 1957;
				this.match(SqlBaseParser.READ);
				}
				break;
			case 4:
				localctx = new SerializableContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 1958;
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
		this.enterRule(localctx, 138, SqlBaseParser.RULE_callArgument);
		try {
			this.state = 1966;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 246, this._ctx) ) {
			case 1:
				localctx = new PositionalArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1961;
				this.expression();
				}
				break;
			case 2:
				localctx = new NamedArgumentContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1962;
				this.identifier();
				this.state = 1963;
				this.match(SqlBaseParser.T__8);
				this.state = 1964;
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
		this.enterRule(localctx, 140, SqlBaseParser.RULE_privilege);
		try {
			this.state = 1972;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 179:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1968;
				this.match(SqlBaseParser.SELECT);
				}
				break;
			case 51:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1969;
				this.match(SqlBaseParser.DELETE);
				}
				break;
			case 97:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1970;
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
				this.state = 1971;
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
		this.enterRule(localctx, 142, SqlBaseParser.RULE_qualifiedName);
		try {
			let _alt: number;
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1974;
			this.identifier();
			this.state = 1979;
			this._errHandler.sync(this);
			_alt = this._interp.adaptivePredict(this._input, 248, this._ctx);
			while (_alt !== 2 && _alt !== ATN.INVALID_ALT_NUMBER) {
				if (_alt === 1) {
					{
					{
					this.state = 1975;
					this.match(SqlBaseParser.T__0);
					this.state = 1976;
					this.identifier();
					}
					}
				}
				this.state = 1981;
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
		this.enterRule(localctx, 144, SqlBaseParser.RULE_tableVersionExpression);
		let _la: number;
		try {
			localctx = new TableVersionContext(this, localctx);
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 1982;
			this.match(SqlBaseParser.FOR);
			this.state = 1983;
			(localctx as TableVersionContext)._tableVersionType = this._input.LT(1);
			_la = this._input.LA(1);
			if(!(((((_la - 191)) & ~0x1F) === 0 && ((1 << (_la - 191)) & 536871427) !== 0))) {
			    (localctx as TableVersionContext)._tableVersionType = this._errHandler.recoverInline(this);
			}
			else {
				this._errHandler.reportMatch(this);
			    this.consume();
			}
			this.state = 1984;
			this.tableVersionState();
			this.state = 1985;
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
		this.enterRule(localctx, 146, SqlBaseParser.RULE_tableVersionState);
		try {
			this.state = 1990;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 18:
				localctx = new TableversionasofContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1987;
				this.match(SqlBaseParser.AS);
				this.state = 1988;
				this.match(SqlBaseParser.OF);
				}
				break;
			case 21:
				localctx = new TableversionbeforeContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1989;
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
		this.enterRule(localctx, 148, SqlBaseParser.RULE_grantor);
		try {
			this.state = 1995;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 250, this._ctx) ) {
			case 1:
				localctx = new CurrentUserGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1992;
				this.match(SqlBaseParser.CURRENT_USER);
				}
				break;
			case 2:
				localctx = new CurrentRoleGrantorContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1993;
				this.match(SqlBaseParser.CURRENT_ROLE);
				}
				break;
			case 3:
				localctx = new SpecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 1994;
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
		this.enterRule(localctx, 150, SqlBaseParser.RULE_principal);
		try {
			this.state = 2002;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 251, this._ctx) ) {
			case 1:
				localctx = new UserPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 1997;
				this.match(SqlBaseParser.USER);
				this.state = 1998;
				this.identifier();
				}
				break;
			case 2:
				localctx = new RolePrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 1999;
				this.match(SqlBaseParser.ROLE);
				this.state = 2000;
				this.identifier();
				}
				break;
			case 3:
				localctx = new UnspecifiedPrincipalContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2001;
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
		this.enterRule(localctx, 152, SqlBaseParser.RULE_roles);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2004;
			this.identifier();
			this.state = 2009;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (_la===4) {
				{
				{
				this.state = 2005;
				this.match(SqlBaseParser.T__3);
				this.state = 2006;
				this.identifier();
				}
				}
				this.state = 2011;
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
		this.enterRule(localctx, 154, SqlBaseParser.RULE_identifier);
		try {
			this.state = 2017;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 247:
				localctx = new UnquotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2012;
				this.match(SqlBaseParser.IDENTIFIER);
				}
				break;
			case 249:
				localctx = new QuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2013;
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
				this.state = 2014;
				this.nonReserved();
				}
				break;
			case 250:
				localctx = new BackQuotedIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 4);
				{
				this.state = 2015;
				this.match(SqlBaseParser.BACKQUOTED_IDENTIFIER);
				}
				break;
			case 248:
				localctx = new DigitIdentifierContext(this, localctx);
				this.enterOuterAlt(localctx, 5);
				{
				this.state = 2016;
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
		this.enterRule(localctx, 156, SqlBaseParser.RULE_number);
		try {
			this.state = 2022;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 245:
				localctx = new DecimalLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2019;
				this.match(SqlBaseParser.DECIMAL_VALUE);
				}
				break;
			case 246:
				localctx = new DoubleLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2020;
				this.match(SqlBaseParser.DOUBLE_VALUE);
				}
				break;
			case 244:
				localctx = new IntegerLiteralContext(this, localctx);
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2021;
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
		this.enterRule(localctx, 158, SqlBaseParser.RULE_constraintSpecification);
		try {
			this.state = 2026;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 36:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2024;
				this.namedConstraintSpecification();
				}
				break;
			case 151:
			case 211:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2025;
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
		this.enterRule(localctx, 160, SqlBaseParser.RULE_namedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2028;
			this.match(SqlBaseParser.CONSTRAINT);
			this.state = 2029;
			localctx._name = this.identifier();
			this.state = 2030;
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
		this.enterRule(localctx, 162, SqlBaseParser.RULE_unnamedConstraintSpecification);
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2032;
			this.constraintType();
			this.state = 2033;
			this.columnAliases();
			this.state = 2035;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 256, this._ctx) ) {
			case 1:
				{
				this.state = 2034;
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
		this.enterRule(localctx, 164, SqlBaseParser.RULE_constraintType);
		try {
			this.state = 2040;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 211:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2037;
				this.match(SqlBaseParser.UNIQUE);
				}
				break;
			case 151:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2038;
				this.match(SqlBaseParser.PRIMARY);
				this.state = 2039;
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
		this.enterRule(localctx, 166, SqlBaseParser.RULE_constraintQualifiers);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2045;
			this._errHandler.sync(this);
			_la = this._input.LA(1);
			while (((((_la - 55)) & ~0x1F) === 0 && ((1 << (_la - 55)) & 161) !== 0) || _la===131 || _la===158) {
				{
				{
				this.state = 2042;
				this.constraintQualifier();
				}
				}
				this.state = 2047;
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
		this.enterRule(localctx, 168, SqlBaseParser.RULE_constraintQualifier);
		try {
			this.state = 2051;
			this._errHandler.sync(this);
			switch ( this._interp.adaptivePredict(this._input, 259, this._ctx) ) {
			case 1:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2048;
				this.constraintEnabled();
				}
				break;
			case 2:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2049;
				this.constraintRely();
				}
				break;
			case 3:
				this.enterOuterAlt(localctx, 3);
				{
				this.state = 2050;
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
		this.enterRule(localctx, 170, SqlBaseParser.RULE_constraintRely);
		try {
			this.state = 2056;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 158:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2053;
				this.match(SqlBaseParser.RELY);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2054;
				this.match(SqlBaseParser.NOT);
				this.state = 2055;
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
		this.enterRule(localctx, 172, SqlBaseParser.RULE_constraintEnabled);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2058;
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
		this.enterRule(localctx, 174, SqlBaseParser.RULE_constraintEnforced);
		try {
			this.state = 2063;
			this._errHandler.sync(this);
			switch (this._input.LA(1)) {
			case 62:
				this.enterOuterAlt(localctx, 1);
				{
				this.state = 2060;
				this.match(SqlBaseParser.ENFORCED);
				}
				break;
			case 131:
				this.enterOuterAlt(localctx, 2);
				{
				this.state = 2061;
				this.match(SqlBaseParser.NOT);
				this.state = 2062;
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
		this.enterRule(localctx, 176, SqlBaseParser.RULE_nonReserved);
		let _la: number;
		try {
			this.enterOuterAlt(localctx, 1);
			{
			this.state = 2065;
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
		case 24:
			return this.queryTerm_sempred(localctx as QueryTermContext, predIndex);
		case 34:
			return this.relation_sempred(localctx as RelationContext, predIndex);
		case 43:
			return this.booleanExpression_sempred(localctx as BooleanExpressionContext, predIndex);
		case 45:
			return this.valueExpression_sempred(localctx as ValueExpressionContext, predIndex);
		case 46:
			return this.primaryExpression_sempred(localctx as PrimaryExpressionContext, predIndex);
		case 57:
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

	public static readonly _serializedATN: number[] = [4,1,258,2068,2,0,7,0,
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
	7,82,2,83,7,83,2,84,7,84,2,85,7,85,2,86,7,86,2,87,7,87,2,88,7,88,1,0,1,
	0,1,0,1,1,1,1,1,1,1,2,1,2,1,2,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
	3,1,3,1,3,3,3,201,8,3,1,3,1,3,1,3,3,3,206,8,3,1,3,1,3,1,3,1,3,3,3,212,8,
	3,1,3,1,3,3,3,216,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,
	3,230,8,3,1,3,1,3,3,3,234,8,3,1,3,1,3,3,3,238,8,3,1,3,1,3,3,3,242,8,3,1,
	3,1,3,1,3,1,3,1,3,1,3,3,3,250,8,3,1,3,1,3,3,3,254,8,3,1,3,3,3,257,8,3,1,
	3,1,3,1,3,1,3,1,3,3,3,264,8,3,1,3,1,3,1,3,1,3,1,3,5,3,271,8,3,10,3,12,3,
	274,9,3,1,3,1,3,1,3,3,3,279,8,3,1,3,1,3,3,3,283,8,3,1,3,1,3,1,3,1,3,3,3,
	289,8,3,1,3,1,3,1,3,1,3,1,3,3,3,296,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,
	305,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,314,8,3,1,3,1,3,1,3,1,3,1,3,1,3,
	1,3,1,3,1,3,3,3,325,8,3,1,3,1,3,1,3,1,3,1,3,3,3,332,8,3,1,3,1,3,1,3,1,3,
	1,3,1,3,1,3,1,3,3,3,342,8,3,1,3,1,3,1,3,1,3,1,3,3,3,349,8,3,1,3,1,3,1,3,
	1,3,1,3,1,3,3,3,357,8,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,365,8,3,1,3,1,3,1,3,
	1,3,1,3,1,3,3,3,373,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,383,8,3,1,3,
	1,3,1,3,1,3,1,3,3,3,390,8,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,398,8,3,1,3,1,3,
	1,3,3,3,403,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,414,8,3,1,3,1,3,
	1,3,3,3,419,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,430,8,3,1,3,1,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,441,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
	5,3,451,8,3,10,3,12,3,454,9,3,1,3,1,3,1,3,3,3,459,8,3,1,3,1,3,1,3,3,3,464,
	8,3,1,3,1,3,1,3,1,3,3,3,470,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,479,8,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,490,8,3,1,3,1,3,1,3,1,3,1,3,1,3,
	1,3,3,3,499,8,3,1,3,1,3,1,3,3,3,504,8,3,1,3,1,3,3,3,508,8,3,1,3,1,3,1,3,
	1,3,1,3,1,3,3,3,516,8,3,1,3,1,3,1,3,1,3,1,3,3,3,523,8,3,1,3,1,3,1,3,1,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,536,8,3,1,3,3,3,539,8,3,1,3,1,3,1,3,1,3,
	1,3,1,3,5,3,547,8,3,10,3,12,3,550,9,3,3,3,552,8,3,1,3,1,3,1,3,1,3,1,3,3,
	3,559,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,568,8,3,1,3,1,3,1,3,1,3,3,3,574,
	8,3,1,3,1,3,1,3,3,3,579,8,3,1,3,1,3,3,3,583,8,3,1,3,1,3,1,3,1,3,1,3,1,3,
	5,3,591,8,3,10,3,12,3,594,9,3,3,3,596,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
	3,3,3,606,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,5,3,617,8,3,10,3,12,3,
	620,9,3,1,3,1,3,1,3,3,3,625,8,3,1,3,1,3,1,3,3,3,630,8,3,1,3,1,3,1,3,1,3,
	3,3,636,8,3,1,3,1,3,1,3,1,3,1,3,5,3,643,8,3,10,3,12,3,646,9,3,1,3,1,3,1,
	3,3,3,651,8,3,1,3,1,3,1,3,1,3,1,3,3,3,658,8,3,1,3,1,3,1,3,1,3,5,3,664,8,
	3,10,3,12,3,667,9,3,1,3,1,3,3,3,671,8,3,1,3,1,3,3,3,675,8,3,1,3,1,3,1,3,
	1,3,1,3,1,3,3,3,683,8,3,1,3,1,3,1,3,1,3,3,3,689,8,3,1,3,1,3,1,3,5,3,694,
	8,3,10,3,12,3,697,9,3,1,3,1,3,3,3,701,8,3,1,3,1,3,3,3,705,8,3,1,3,1,3,1,
	3,1,3,1,3,1,3,1,3,1,3,3,3,715,8,3,1,3,3,3,718,8,3,1,3,1,3,3,3,722,8,3,1,
	3,3,3,725,8,3,1,3,1,3,1,3,1,3,5,3,731,8,3,10,3,12,3,734,9,3,1,3,1,3,3,3,
	738,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,763,8,3,1,3,1,3,1,3,1,3,3,3,769,8,3,1,3,
	1,3,1,3,1,3,3,3,775,8,3,3,3,777,8,3,1,3,1,3,1,3,1,3,3,3,783,8,3,1,3,1,3,
	1,3,1,3,3,3,789,8,3,3,3,791,8,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,799,8,3,3,3,
	801,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
	1,3,3,3,820,8,3,1,3,1,3,1,3,3,3,825,8,3,1,3,1,3,1,3,1,3,1,3,3,3,832,8,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,844,8,3,3,3,846,8,3,1,3,1,3,
	1,3,1,3,1,3,1,3,3,3,854,8,3,3,3,856,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,
	1,3,1,3,1,3,1,3,1,3,1,3,5,3,872,8,3,10,3,12,3,875,9,3,3,3,877,8,3,1,3,1,
	3,3,3,881,8,3,1,3,1,3,3,3,885,8,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
	3,1,3,1,3,1,3,1,3,5,3,901,8,3,10,3,12,3,904,9,3,3,3,906,8,3,1,3,1,3,1,3,
	1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,5,3,920,8,3,10,3,12,3,923,9,3,1,3,1,
	3,3,3,927,8,3,3,3,929,8,3,1,4,3,4,932,8,4,1,4,1,4,1,5,1,5,3,5,938,8,5,1,
	5,1,5,1,5,5,5,943,8,5,10,5,12,5,946,9,5,1,6,1,6,1,6,3,6,951,8,6,1,7,1,7,
	1,7,1,7,3,7,957,8,7,1,7,1,7,3,7,961,8,7,1,7,1,7,3,7,965,8,7,1,8,1,8,1,8,
	1,8,3,8,971,8,8,1,9,1,9,1,9,1,9,5,9,977,8,9,10,9,12,9,980,9,9,1,9,1,9,1,
	10,1,10,1,10,1,10,1,11,1,11,1,11,1,12,5,12,992,8,12,10,12,12,12,995,9,12,
	1,13,1,13,1,13,1,13,3,13,1001,8,13,1,14,5,14,1004,8,14,10,14,12,14,1007,
	9,14,1,15,1,15,1,16,1,16,3,16,1013,8,16,1,17,1,17,1,17,1,18,1,18,1,18,3,
	18,1021,8,18,1,19,1,19,3,19,1025,8,19,1,20,1,20,1,20,3,20,1030,8,20,1,21,
	1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,3,21,1041,8,21,1,22,1,22,1,23,1,
	23,1,23,1,23,1,23,1,23,5,23,1051,8,23,10,23,12,23,1054,9,23,3,23,1056,8,
	23,1,23,1,23,1,23,3,23,1061,8,23,3,23,1063,8,23,1,23,1,23,1,23,1,23,1,23,
	1,23,1,23,3,23,1072,8,23,3,23,1074,8,23,1,24,1,24,1,24,1,24,1,24,1,24,3,
	24,1082,8,24,1,24,1,24,1,24,1,24,3,24,1088,8,24,1,24,5,24,1091,8,24,10,
	24,12,24,1094,9,24,1,25,1,25,1,25,1,25,1,25,1,25,1,25,5,25,1103,8,25,10,
	25,12,25,1106,9,25,1,25,1,25,1,25,1,25,3,25,1112,8,25,1,26,1,26,3,26,1116,
	8,26,1,26,1,26,3,26,1120,8,26,1,27,1,27,3,27,1124,8,27,1,27,1,27,1,27,5,
	27,1129,8,27,10,27,12,27,1132,9,27,1,27,1,27,1,27,1,27,5,27,1138,8,27,10,
	27,12,27,1141,9,27,3,27,1143,8,27,1,27,1,27,3,27,1147,8,27,1,27,1,27,1,
	27,3,27,1152,8,27,1,27,1,27,3,27,1156,8,27,1,28,3,28,1159,8,28,1,28,1,28,
	1,28,5,28,1164,8,28,10,28,12,28,1167,9,28,1,29,1,29,1,29,1,29,1,29,1,29,
	5,29,1175,8,29,10,29,12,29,1178,9,29,3,29,1180,8,29,1,29,1,29,1,29,1,29,
	1,29,1,29,5,29,1188,8,29,10,29,12,29,1191,9,29,3,29,1193,8,29,1,29,1,29,
	1,29,1,29,1,29,1,29,1,29,5,29,1202,8,29,10,29,12,29,1205,9,29,1,29,1,29,
	3,29,1209,8,29,1,30,1,30,1,30,1,30,5,30,1215,8,30,10,30,12,30,1218,9,30,
	3,30,1220,8,30,1,30,1,30,3,30,1224,8,30,1,31,1,31,3,31,1228,8,31,1,31,1,
	31,1,31,1,31,1,31,1,32,1,32,1,33,1,33,3,33,1239,8,33,1,33,3,33,1242,8,33,
	1,33,1,33,1,33,1,33,1,33,3,33,1249,8,33,1,34,1,34,1,34,1,34,1,34,1,34,1,
	34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,3,34,1268,8,34,5,34,
	1270,8,34,10,34,12,34,1273,9,34,1,35,3,35,1276,8,35,1,35,1,35,3,35,1280,
	8,35,1,35,1,35,3,35,1284,8,35,1,35,1,35,3,35,1288,8,35,3,35,1290,8,35,1,
	36,1,36,1,36,1,36,1,36,1,36,1,36,5,36,1299,8,36,10,36,12,36,1302,9,36,1,
	36,1,36,3,36,1306,8,36,1,37,1,37,1,37,1,37,1,37,1,37,1,37,3,37,1315,8,37,
	1,38,1,38,1,39,1,39,3,39,1321,8,39,1,39,1,39,3,39,1325,8,39,3,39,1327,8,
	39,1,40,1,40,1,40,1,40,5,40,1333,8,40,10,40,12,40,1336,9,40,1,40,1,40,1,
	41,1,41,3,41,1342,8,41,1,41,1,41,1,41,1,41,1,41,1,41,1,41,1,41,1,41,5,41,
	1353,8,41,10,41,12,41,1356,9,41,1,41,1,41,1,41,3,41,1361,8,41,1,41,1,41,
	1,41,1,41,1,41,1,41,1,41,1,41,1,41,3,41,1372,8,41,1,42,1,42,1,43,1,43,1,
	43,3,43,1379,8,43,1,43,1,43,3,43,1383,8,43,1,43,1,43,1,43,1,43,1,43,1,43,
	5,43,1391,8,43,10,43,12,43,1394,9,43,1,44,1,44,1,44,1,44,1,44,1,44,1,44,
	1,44,1,44,1,44,3,44,1406,8,44,1,44,1,44,1,44,1,44,1,44,1,44,3,44,1414,8,
	44,1,44,1,44,1,44,1,44,1,44,5,44,1421,8,44,10,44,12,44,1424,9,44,1,44,1,
	44,1,44,3,44,1429,8,44,1,44,1,44,1,44,1,44,1,44,1,44,3,44,1437,8,44,1,44,
	1,44,1,44,1,44,3,44,1443,8,44,1,44,1,44,3,44,1447,8,44,1,44,1,44,1,44,3,
	44,1452,8,44,1,44,1,44,1,44,3,44,1457,8,44,1,45,1,45,1,45,1,45,3,45,1463,
	8,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,1,45,5,45,1477,
	8,45,10,45,12,45,1480,9,45,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,
	1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,
	46,4,46,1506,8,46,11,46,12,46,1507,1,46,1,46,1,46,1,46,1,46,1,46,1,46,5,
	46,1517,8,46,10,46,12,46,1520,9,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,3,
	46,1529,8,46,1,46,3,46,1532,8,46,1,46,1,46,1,46,3,46,1537,8,46,1,46,1,46,
	1,46,5,46,1542,8,46,10,46,12,46,1545,9,46,3,46,1547,8,46,1,46,1,46,1,46,
	1,46,1,46,5,46,1554,8,46,10,46,12,46,1557,9,46,3,46,1559,8,46,1,46,1,46,
	3,46,1563,8,46,1,46,3,46,1566,8,46,1,46,3,46,1569,8,46,1,46,1,46,1,46,1,
	46,1,46,1,46,1,46,1,46,5,46,1579,8,46,10,46,12,46,1582,9,46,3,46,1584,8,
	46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,
	1,46,4,46,1601,8,46,11,46,12,46,1602,1,46,1,46,3,46,1607,8,46,1,46,1,46,
	1,46,1,46,4,46,1613,8,46,11,46,12,46,1614,1,46,1,46,3,46,1619,8,46,1,46,
	1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,
	46,1,46,1,46,1,46,1,46,1,46,5,46,1642,8,46,10,46,12,46,1645,9,46,3,46,1647,
	8,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,3,46,1656,8,46,1,46,1,46,1,46,1,
	46,3,46,1662,8,46,1,46,1,46,1,46,1,46,3,46,1668,8,46,1,46,1,46,1,46,1,46,
	3,46,1674,8,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,3,46,1684,8,46,1,
	46,1,46,1,46,1,46,1,46,1,46,1,46,3,46,1693,8,46,1,46,1,46,1,46,1,46,1,46,
	1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,46,5,46,1713,
	8,46,10,46,12,46,1716,9,46,3,46,1718,8,46,1,46,3,46,1721,8,46,1,46,1,46,
	1,46,1,46,1,46,1,46,1,46,1,46,5,46,1731,8,46,10,46,12,46,1734,9,46,1,47,
	1,47,1,47,1,47,3,47,1740,8,47,3,47,1742,8,47,1,48,1,48,1,48,1,48,3,48,1748,
	8,48,1,49,1,49,1,49,1,49,1,49,1,49,3,49,1756,8,49,1,50,1,50,1,51,1,51,1,
	52,1,52,1,53,1,53,3,53,1766,8,53,1,53,1,53,1,53,1,53,3,53,1772,8,53,1,54,
	1,54,1,55,1,55,1,56,1,56,1,56,1,56,5,56,1782,8,56,10,56,12,56,1785,9,56,
	3,56,1787,8,56,1,56,1,56,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,
	57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,5,57,1812,8,57,
	10,57,12,57,1815,9,57,1,57,1,57,1,57,1,57,1,57,1,57,1,57,5,57,1824,8,57,
	10,57,12,57,1827,9,57,1,57,1,57,3,57,1831,8,57,1,57,1,57,1,57,1,57,1,57,
	3,57,1838,8,57,1,57,1,57,5,57,1842,8,57,10,57,12,57,1845,9,57,1,58,1,58,
	3,58,1849,8,58,1,59,1,59,1,59,1,59,3,59,1855,8,59,1,60,1,60,1,60,1,60,1,
	60,1,61,1,61,1,61,1,61,1,61,1,61,1,62,1,62,1,62,1,62,1,62,1,62,1,62,5,62,
	1875,8,62,10,62,12,62,1878,9,62,3,62,1880,8,62,1,62,1,62,1,62,1,62,1,62,
	5,62,1887,8,62,10,62,12,62,1890,9,62,3,62,1892,8,62,1,62,3,62,1895,8,62,
	1,62,1,62,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,
	63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,3,63,1923,8,63,
	1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,1,64,3,64,1934,8,64,1,65,1,65,1,
	65,1,65,1,66,1,66,1,66,1,66,3,66,1944,8,66,1,67,1,67,1,67,1,67,1,67,3,67,
	1951,8,67,1,68,1,68,1,68,1,68,1,68,1,68,1,68,3,68,1960,8,68,1,69,1,69,1,
	69,1,69,1,69,3,69,1967,8,69,1,70,1,70,1,70,1,70,3,70,1973,8,70,1,71,1,71,
	1,71,5,71,1978,8,71,10,71,12,71,1981,9,71,1,72,1,72,1,72,1,72,1,72,1,73,
	1,73,1,73,3,73,1991,8,73,1,74,1,74,1,74,3,74,1996,8,74,1,75,1,75,1,75,1,
	75,1,75,3,75,2003,8,75,1,76,1,76,1,76,5,76,2008,8,76,10,76,12,76,2011,9,
	76,1,77,1,77,1,77,1,77,1,77,3,77,2018,8,77,1,78,1,78,1,78,3,78,2023,8,78,
	1,79,1,79,3,79,2027,8,79,1,80,1,80,1,80,1,80,1,81,1,81,1,81,3,81,2036,8,
	81,1,82,1,82,1,82,3,82,2041,8,82,1,83,5,83,2044,8,83,10,83,12,83,2047,9,
	83,1,84,1,84,1,84,3,84,2052,8,84,1,85,1,85,1,85,3,85,2057,8,85,1,86,1,86,
	1,87,1,87,1,87,3,87,2064,8,87,1,88,1,88,1,88,0,6,48,68,86,90,92,114,89,
	0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,
	52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,
	100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,
	136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,
	172,174,176,0,25,2,0,27,27,164,164,2,0,50,50,101,101,2,0,78,78,93,93,2,
	0,65,65,94,94,1,0,173,174,2,0,12,12,244,244,2,0,64,64,210,210,2,0,19,19,
	52,52,2,0,74,74,109,109,2,0,12,12,56,56,2,0,22,22,190,190,1,0,235,236,1,
	0,237,239,1,0,229,234,3,0,12,12,16,16,185,185,2,0,71,71,203,203,5,0,48,
	48,90,90,120,121,177,177,227,227,1,0,124,127,2,0,75,75,149,149,3,0,85,85,
	105,105,197,197,4,0,57,57,102,102,117,117,217,217,2,0,138,138,226,226,3,
	0,191,192,200,200,220,220,2,0,55,55,60,60,51,0,10,12,14,14,16,17,19,22,
	25,27,30,35,40,40,42,42,46,48,50,50,52,52,54,55,57,57,60,60,62,62,65,65,
	68,68,70,70,72,75,77,77,80,85,88,88,90,92,94,94,96,96,99,99,101,102,104,
	105,107,110,112,112,114,114,117,122,124,129,133,136,138,139,142,142,144,
	149,151,155,157,167,169,171,173,178,180,192,194,197,199,202,204,206,208,
	209,211,211,213,215,217,217,219,221,225,228,2385,0,178,1,0,0,0,2,181,1,
	0,0,0,4,184,1,0,0,0,6,928,1,0,0,0,8,931,1,0,0,0,10,935,1,0,0,0,12,950,1,
	0,0,0,14,952,1,0,0,0,16,966,1,0,0,0,18,972,1,0,0,0,20,983,1,0,0,0,22,987,
	1,0,0,0,24,993,1,0,0,0,26,1000,1,0,0,0,28,1005,1,0,0,0,30,1008,1,0,0,0,
	32,1012,1,0,0,0,34,1014,1,0,0,0,36,1017,1,0,0,0,38,1024,1,0,0,0,40,1029,
	1,0,0,0,42,1040,1,0,0,0,44,1042,1,0,0,0,46,1044,1,0,0,0,48,1075,1,0,0,0,
	50,1111,1,0,0,0,52,1113,1,0,0,0,54,1121,1,0,0,0,56,1158,1,0,0,0,58,1208,
	1,0,0,0,60,1223,1,0,0,0,62,1225,1,0,0,0,64,1234,1,0,0,0,66,1248,1,0,0,0,
	68,1250,1,0,0,0,70,1289,1,0,0,0,72,1305,1,0,0,0,74,1307,1,0,0,0,76,1316,
	1,0,0,0,78,1318,1,0,0,0,80,1328,1,0,0,0,82,1371,1,0,0,0,84,1373,1,0,0,0,
	86,1382,1,0,0,0,88,1456,1,0,0,0,90,1462,1,0,0,0,92,1720,1,0,0,0,94,1741,
	1,0,0,0,96,1747,1,0,0,0,98,1755,1,0,0,0,100,1757,1,0,0,0,102,1759,1,0,0,
	0,104,1761,1,0,0,0,106,1763,1,0,0,0,108,1773,1,0,0,0,110,1775,1,0,0,0,112,
	1777,1,0,0,0,114,1837,1,0,0,0,116,1848,1,0,0,0,118,1854,1,0,0,0,120,1856,
	1,0,0,0,122,1861,1,0,0,0,124,1867,1,0,0,0,126,1922,1,0,0,0,128,1933,1,0,
	0,0,130,1935,1,0,0,0,132,1943,1,0,0,0,134,1950,1,0,0,0,136,1959,1,0,0,0,
	138,1966,1,0,0,0,140,1972,1,0,0,0,142,1974,1,0,0,0,144,1982,1,0,0,0,146,
	1990,1,0,0,0,148,1995,1,0,0,0,150,2002,1,0,0,0,152,2004,1,0,0,0,154,2017,
	1,0,0,0,156,2022,1,0,0,0,158,2026,1,0,0,0,160,2028,1,0,0,0,162,2032,1,0,
	0,0,164,2040,1,0,0,0,166,2045,1,0,0,0,168,2051,1,0,0,0,170,2056,1,0,0,0,
	172,2058,1,0,0,0,174,2063,1,0,0,0,176,2065,1,0,0,0,178,179,3,6,3,0,179,
	180,5,0,0,1,180,1,1,0,0,0,181,182,3,84,42,0,182,183,5,0,0,1,183,3,1,0,0,
	0,184,185,3,32,16,0,185,186,5,0,0,1,186,5,1,0,0,0,187,929,3,8,4,0,188,189,
	5,214,0,0,189,929,3,154,77,0,190,191,5,214,0,0,191,192,3,154,77,0,192,193,
	5,1,0,0,193,194,3,154,77,0,194,929,1,0,0,0,195,196,5,37,0,0,196,200,5,175,
	0,0,197,198,5,91,0,0,198,199,5,131,0,0,199,201,5,67,0,0,200,197,1,0,0,0,
	200,201,1,0,0,0,201,202,1,0,0,0,202,205,3,142,71,0,203,204,5,224,0,0,204,
	206,3,18,9,0,205,203,1,0,0,0,205,206,1,0,0,0,206,929,1,0,0,0,207,208,5,
	58,0,0,208,211,5,175,0,0,209,210,5,91,0,0,210,212,5,67,0,0,211,209,1,0,
	0,0,211,212,1,0,0,0,212,213,1,0,0,0,213,215,3,142,71,0,214,216,7,0,0,0,
	215,214,1,0,0,0,215,216,1,0,0,0,216,929,1,0,0,0,217,218,5,13,0,0,218,219,
	5,175,0,0,219,220,3,142,71,0,220,221,5,159,0,0,221,222,5,201,0,0,222,223,
	3,154,77,0,223,929,1,0,0,0,224,225,5,37,0,0,225,229,5,193,0,0,226,227,5,
	91,0,0,227,228,5,131,0,0,228,230,5,67,0,0,229,226,1,0,0,0,229,230,1,0,0,
	0,230,231,1,0,0,0,231,233,3,142,71,0,232,234,3,80,40,0,233,232,1,0,0,0,
	233,234,1,0,0,0,234,237,1,0,0,0,235,236,5,33,0,0,236,238,3,94,47,0,237,
	235,1,0,0,0,237,238,1,0,0,0,238,241,1,0,0,0,239,240,5,224,0,0,240,242,3,
	18,9,0,241,239,1,0,0,0,241,242,1,0,0,0,242,243,1,0,0,0,243,249,5,18,0,0,
	244,250,3,8,4,0,245,246,5,2,0,0,246,247,3,8,4,0,247,248,5,3,0,0,248,250,
	1,0,0,0,249,244,1,0,0,0,249,245,1,0,0,0,250,256,1,0,0,0,251,253,5,224,0,
	0,252,254,5,128,0,0,253,252,1,0,0,0,253,254,1,0,0,0,254,255,1,0,0,0,255,
	257,5,46,0,0,256,251,1,0,0,0,256,257,1,0,0,0,257,929,1,0,0,0,258,259,5,
	37,0,0,259,263,5,193,0,0,260,261,5,91,0,0,261,262,5,131,0,0,262,264,5,67,
	0,0,263,260,1,0,0,0,263,264,1,0,0,0,264,265,1,0,0,0,265,266,3,142,71,0,
	266,267,5,2,0,0,267,272,3,12,6,0,268,269,5,4,0,0,269,271,3,12,6,0,270,268,
	1,0,0,0,271,274,1,0,0,0,272,270,1,0,0,0,272,273,1,0,0,0,273,275,1,0,0,0,
	274,272,1,0,0,0,275,278,5,3,0,0,276,277,5,33,0,0,277,279,3,94,47,0,278,
	276,1,0,0,0,278,279,1,0,0,0,279,282,1,0,0,0,280,281,5,224,0,0,281,283,3,
	18,9,0,282,280,1,0,0,0,282,283,1,0,0,0,283,929,1,0,0,0,284,285,5,58,0,0,
	285,288,5,193,0,0,286,287,5,91,0,0,287,289,5,67,0,0,288,286,1,0,0,0,288,
	289,1,0,0,0,289,290,1,0,0,0,290,929,3,142,71,0,291,292,5,97,0,0,292,293,
	5,100,0,0,293,295,3,142,71,0,294,296,3,80,40,0,295,294,1,0,0,0,295,296,
	1,0,0,0,296,297,1,0,0,0,297,298,3,8,4,0,298,929,1,0,0,0,299,300,5,51,0,
	0,300,301,5,78,0,0,301,304,3,142,71,0,302,303,5,223,0,0,303,305,3,86,43,
	0,304,302,1,0,0,0,304,305,1,0,0,0,305,929,1,0,0,0,306,307,5,204,0,0,307,
	308,5,193,0,0,308,929,3,142,71,0,309,310,5,13,0,0,310,313,5,193,0,0,311,
	312,5,91,0,0,312,314,5,67,0,0,313,311,1,0,0,0,313,314,1,0,0,0,314,315,1,
	0,0,0,315,316,3,142,71,0,316,317,5,159,0,0,317,318,5,201,0,0,318,319,3,
	142,71,0,319,929,1,0,0,0,320,321,5,13,0,0,321,324,5,193,0,0,322,323,5,91,
	0,0,323,325,5,67,0,0,324,322,1,0,0,0,324,325,1,0,0,0,325,326,1,0,0,0,326,
	327,3,142,71,0,327,328,5,159,0,0,328,331,5,31,0,0,329,330,5,91,0,0,330,
	332,5,67,0,0,331,329,1,0,0,0,331,332,1,0,0,0,332,333,1,0,0,0,333,334,3,
	154,77,0,334,335,5,201,0,0,335,336,3,154,77,0,336,929,1,0,0,0,337,338,5,
	13,0,0,338,341,5,193,0,0,339,340,5,91,0,0,340,342,5,67,0,0,341,339,1,0,
	0,0,341,342,1,0,0,0,342,343,1,0,0,0,343,344,3,142,71,0,344,345,5,58,0,0,
	345,348,5,31,0,0,346,347,5,91,0,0,347,349,5,67,0,0,348,346,1,0,0,0,348,
	349,1,0,0,0,349,350,1,0,0,0,350,351,3,142,71,0,351,929,1,0,0,0,352,353,
	5,13,0,0,353,356,5,193,0,0,354,355,5,91,0,0,355,357,5,67,0,0,356,354,1,
	0,0,0,356,357,1,0,0,0,357,358,1,0,0,0,358,359,3,142,71,0,359,360,5,10,0,
	0,360,364,5,31,0,0,361,362,5,91,0,0,362,363,5,131,0,0,363,365,5,67,0,0,
	364,361,1,0,0,0,364,365,1,0,0,0,365,366,1,0,0,0,366,367,3,14,7,0,367,929,
	1,0,0,0,368,369,5,13,0,0,369,372,5,193,0,0,370,371,5,91,0,0,371,373,5,67,
	0,0,372,370,1,0,0,0,372,373,1,0,0,0,373,374,1,0,0,0,374,375,3,142,71,0,
	375,376,5,10,0,0,376,377,3,158,79,0,377,929,1,0,0,0,378,379,5,13,0,0,379,
	382,5,193,0,0,380,381,5,91,0,0,381,383,5,67,0,0,382,380,1,0,0,0,382,383,
	1,0,0,0,383,384,1,0,0,0,384,385,3,142,71,0,385,386,5,58,0,0,386,389,5,36,
	0,0,387,388,5,91,0,0,388,390,5,67,0,0,389,387,1,0,0,0,389,390,1,0,0,0,390,
	391,1,0,0,0,391,392,3,154,77,0,392,929,1,0,0,0,393,394,5,13,0,0,394,397,
	5,193,0,0,395,396,5,91,0,0,396,398,5,67,0,0,397,395,1,0,0,0,397,398,1,0,
	0,0,398,399,1,0,0,0,399,400,3,142,71,0,400,402,5,13,0,0,401,403,5,31,0,
	0,402,401,1,0,0,0,402,403,1,0,0,0,403,404,1,0,0,0,404,405,3,154,77,0,405,
	406,5,182,0,0,406,407,5,131,0,0,407,408,5,132,0,0,408,929,1,0,0,0,409,410,
	5,13,0,0,410,413,5,193,0,0,411,412,5,91,0,0,412,414,5,67,0,0,413,411,1,
	0,0,0,413,414,1,0,0,0,414,415,1,0,0,0,415,416,3,142,71,0,416,418,5,13,0,
	0,417,419,5,31,0,0,418,417,1,0,0,0,418,419,1,0,0,0,419,420,1,0,0,0,420,
	421,3,154,77,0,421,422,5,58,0,0,422,423,5,131,0,0,423,424,5,132,0,0,424,
	929,1,0,0,0,425,426,5,13,0,0,426,429,5,193,0,0,427,428,5,91,0,0,428,430,
	5,67,0,0,429,427,1,0,0,0,429,430,1,0,0,0,430,431,1,0,0,0,431,432,3,142,
	71,0,432,433,5,182,0,0,433,434,5,153,0,0,434,435,3,18,9,0,435,929,1,0,0,
	0,436,437,5,14,0,0,437,440,3,142,71,0,438,439,5,224,0,0,439,441,3,18,9,
	0,440,438,1,0,0,0,440,441,1,0,0,0,441,929,1,0,0,0,442,443,5,37,0,0,443,
	444,5,206,0,0,444,445,3,142,71,0,445,458,5,18,0,0,446,447,5,2,0,0,447,452,
	3,22,11,0,448,449,5,4,0,0,449,451,3,22,11,0,450,448,1,0,0,0,451,454,1,0,
	0,0,452,450,1,0,0,0,452,453,1,0,0,0,453,455,1,0,0,0,454,452,1,0,0,0,455,
	456,5,3,0,0,456,459,1,0,0,0,457,459,3,114,57,0,458,446,1,0,0,0,458,457,
	1,0,0,0,459,929,1,0,0,0,460,463,5,37,0,0,461,462,5,140,0,0,462,464,5,161,
	0,0,463,461,1,0,0,0,463,464,1,0,0,0,464,465,1,0,0,0,465,466,5,221,0,0,466,
	469,3,142,71,0,467,468,5,178,0,0,468,470,7,1,0,0,469,467,1,0,0,0,469,470,
	1,0,0,0,470,471,1,0,0,0,471,472,5,18,0,0,472,473,3,8,4,0,473,929,1,0,0,
	0,474,475,5,13,0,0,475,478,5,221,0,0,476,477,5,91,0,0,477,479,5,67,0,0,
	478,476,1,0,0,0,478,479,1,0,0,0,479,480,1,0,0,0,480,481,3,142,71,0,481,
	482,5,159,0,0,482,483,5,201,0,0,483,484,3,142,71,0,484,929,1,0,0,0,485,
	486,5,58,0,0,486,489,5,221,0,0,487,488,5,91,0,0,488,490,5,67,0,0,489,487,
	1,0,0,0,489,490,1,0,0,0,490,491,1,0,0,0,491,929,3,142,71,0,492,493,5,37,
	0,0,493,494,5,119,0,0,494,498,5,221,0,0,495,496,5,91,0,0,496,497,5,131,
	0,0,497,499,5,67,0,0,498,495,1,0,0,0,498,499,1,0,0,0,499,500,1,0,0,0,500,
	503,3,142,71,0,501,502,5,33,0,0,502,504,3,94,47,0,503,501,1,0,0,0,503,504,
	1,0,0,0,504,507,1,0,0,0,505,506,5,224,0,0,506,508,3,18,9,0,507,505,1,0,
	0,0,507,508,1,0,0,0,508,509,1,0,0,0,509,515,5,18,0,0,510,516,3,8,4,0,511,
	512,5,2,0,0,512,513,3,8,4,0,513,514,5,3,0,0,514,516,1,0,0,0,515,510,1,0,
	0,0,515,511,1,0,0,0,516,929,1,0,0,0,517,518,5,58,0,0,518,519,5,119,0,0,
	519,522,5,221,0,0,520,521,5,91,0,0,521,523,5,67,0,0,522,520,1,0,0,0,522,
	523,1,0,0,0,523,524,1,0,0,0,524,929,3,142,71,0,525,526,5,157,0,0,526,527,
	5,119,0,0,527,528,5,221,0,0,528,529,3,142,71,0,529,530,5,223,0,0,530,531,
	3,86,43,0,531,929,1,0,0,0,532,535,5,37,0,0,533,534,5,140,0,0,534,536,5,
	161,0,0,535,533,1,0,0,0,535,536,1,0,0,0,536,538,1,0,0,0,537,539,5,196,0,
	0,538,537,1,0,0,0,538,539,1,0,0,0,539,540,1,0,0,0,540,541,5,80,0,0,541,
	542,3,142,71,0,542,551,5,2,0,0,543,548,3,22,11,0,544,545,5,4,0,0,545,547,
	3,22,11,0,546,544,1,0,0,0,547,550,1,0,0,0,548,546,1,0,0,0,548,549,1,0,0,
	0,549,552,1,0,0,0,550,548,1,0,0,0,551,543,1,0,0,0,551,552,1,0,0,0,552,553,
	1,0,0,0,553,554,5,3,0,0,554,555,5,166,0,0,555,558,3,114,57,0,556,557,5,
	33,0,0,557,559,3,94,47,0,558,556,1,0,0,0,558,559,1,0,0,0,559,560,1,0,0,
	0,560,561,3,24,12,0,561,562,3,32,16,0,562,929,1,0,0,0,563,564,5,13,0,0,
	564,565,5,80,0,0,565,567,3,142,71,0,566,568,3,112,56,0,567,566,1,0,0,0,
	567,568,1,0,0,0,568,569,1,0,0,0,569,570,3,28,14,0,570,929,1,0,0,0,571,573,
	5,58,0,0,572,574,5,196,0,0,573,572,1,0,0,0,573,574,1,0,0,0,574,575,1,0,
	0,0,575,578,5,80,0,0,576,577,5,91,0,0,577,579,5,67,0,0,578,576,1,0,0,0,
	578,579,1,0,0,0,579,580,1,0,0,0,580,582,3,142,71,0,581,583,3,112,56,0,582,
	581,1,0,0,0,582,583,1,0,0,0,583,929,1,0,0,0,584,585,5,25,0,0,585,586,3,
	142,71,0,586,595,5,2,0,0,587,592,3,138,69,0,588,589,5,4,0,0,589,591,3,138,
	69,0,590,588,1,0,0,0,591,594,1,0,0,0,592,590,1,0,0,0,592,593,1,0,0,0,593,
	596,1,0,0,0,594,592,1,0,0,0,595,587,1,0,0,0,595,596,1,0,0,0,596,597,1,0,
	0,0,597,598,5,3,0,0,598,929,1,0,0,0,599,600,5,37,0,0,600,601,5,169,0,0,
	601,605,3,154,77,0,602,603,5,224,0,0,603,604,5,11,0,0,604,606,3,148,74,
	0,605,602,1,0,0,0,605,606,1,0,0,0,606,929,1,0,0,0,607,608,5,58,0,0,608,
	609,5,169,0,0,609,929,3,154,77,0,610,611,5,82,0,0,611,612,3,152,76,0,612,
	613,5,201,0,0,613,618,3,150,75,0,614,615,5,4,0,0,615,617,3,150,75,0,616,
	614,1,0,0,0,617,620,1,0,0,0,618,616,1,0,0,0,618,619,1,0,0,0,619,624,1,0,
	0,0,620,618,1,0,0,0,621,622,5,224,0,0,622,623,5,11,0,0,623,625,5,139,0,
	0,624,621,1,0,0,0,624,625,1,0,0,0,625,629,1,0,0,0,626,627,5,83,0,0,627,
	628,5,24,0,0,628,630,3,148,74,0,629,626,1,0,0,0,629,630,1,0,0,0,630,929,
	1,0,0,0,631,635,5,167,0,0,632,633,5,11,0,0,633,634,5,139,0,0,634,636,5,
	76,0,0,635,632,1,0,0,0,635,636,1,0,0,0,636,637,1,0,0,0,637,638,3,152,76,
	0,638,639,5,78,0,0,639,644,3,150,75,0,640,641,5,4,0,0,641,643,3,150,75,
	0,642,640,1,0,0,0,643,646,1,0,0,0,644,642,1,0,0,0,644,645,1,0,0,0,645,650,
	1,0,0,0,646,644,1,0,0,0,647,648,5,83,0,0,648,649,5,24,0,0,649,651,3,148,
	74,0,650,647,1,0,0,0,650,651,1,0,0,0,651,929,1,0,0,0,652,653,5,182,0,0,
	653,657,5,169,0,0,654,658,5,12,0,0,655,658,5,129,0,0,656,658,3,154,77,0,
	657,654,1,0,0,0,657,655,1,0,0,0,657,656,1,0,0,0,658,929,1,0,0,0,659,670,
	5,82,0,0,660,665,3,140,70,0,661,662,5,4,0,0,662,664,3,140,70,0,663,661,
	1,0,0,0,664,667,1,0,0,0,665,663,1,0,0,0,665,666,1,0,0,0,666,671,1,0,0,0,
	667,665,1,0,0,0,668,669,5,12,0,0,669,671,5,152,0,0,670,660,1,0,0,0,670,
	668,1,0,0,0,671,672,1,0,0,0,672,674,5,137,0,0,673,675,5,193,0,0,674,673,
	1,0,0,0,674,675,1,0,0,0,675,676,1,0,0,0,676,677,3,142,71,0,677,678,5,201,
	0,0,678,682,3,150,75,0,679,680,5,224,0,0,680,681,5,82,0,0,681,683,5,139,
	0,0,682,679,1,0,0,0,682,683,1,0,0,0,683,929,1,0,0,0,684,688,5,167,0,0,685,
	686,5,82,0,0,686,687,5,139,0,0,687,689,5,76,0,0,688,685,1,0,0,0,688,689,
	1,0,0,0,689,700,1,0,0,0,690,695,3,140,70,0,691,692,5,4,0,0,692,694,3,140,
	70,0,693,691,1,0,0,0,694,697,1,0,0,0,695,693,1,0,0,0,695,696,1,0,0,0,696,
	701,1,0,0,0,697,695,1,0,0,0,698,699,5,12,0,0,699,701,5,152,0,0,700,690,
	1,0,0,0,700,698,1,0,0,0,701,702,1,0,0,0,702,704,5,137,0,0,703,705,5,193,
	0,0,704,703,1,0,0,0,704,705,1,0,0,0,705,706,1,0,0,0,706,707,3,142,71,0,
	707,708,5,78,0,0,708,709,3,150,75,0,709,929,1,0,0,0,710,711,5,184,0,0,711,
	717,5,84,0,0,712,714,5,137,0,0,713,715,5,193,0,0,714,713,1,0,0,0,714,715,
	1,0,0,0,715,716,1,0,0,0,716,718,3,142,71,0,717,712,1,0,0,0,717,718,1,0,
	0,0,718,929,1,0,0,0,719,721,5,68,0,0,720,722,5,14,0,0,721,720,1,0,0,0,721,
	722,1,0,0,0,722,724,1,0,0,0,723,725,5,219,0,0,724,723,1,0,0,0,724,725,1,
	0,0,0,725,737,1,0,0,0,726,727,5,2,0,0,727,732,3,132,66,0,728,729,5,4,0,
	0,729,731,3,132,66,0,730,728,1,0,0,0,731,734,1,0,0,0,732,730,1,0,0,0,732,
	733,1,0,0,0,733,735,1,0,0,0,734,732,1,0,0,0,735,736,5,3,0,0,736,738,1,0,
	0,0,737,726,1,0,0,0,737,738,1,0,0,0,738,739,1,0,0,0,739,929,3,6,3,0,740,
	741,5,184,0,0,741,742,5,37,0,0,742,743,5,193,0,0,743,929,3,142,71,0,744,
	745,5,184,0,0,745,746,5,37,0,0,746,747,5,175,0,0,747,929,3,142,71,0,748,
	749,5,184,0,0,749,750,5,37,0,0,750,751,5,221,0,0,751,929,3,142,71,0,752,
	753,5,184,0,0,753,754,5,37,0,0,754,755,5,119,0,0,755,756,5,221,0,0,756,
	929,3,142,71,0,757,758,5,184,0,0,758,759,5,37,0,0,759,760,5,80,0,0,760,
	762,3,142,71,0,761,763,3,112,56,0,762,761,1,0,0,0,762,763,1,0,0,0,763,929,
	1,0,0,0,764,765,5,184,0,0,765,768,5,194,0,0,766,767,7,2,0,0,767,769,3,142,
	71,0,768,766,1,0,0,0,768,769,1,0,0,0,769,776,1,0,0,0,770,771,5,113,0,0,
	771,774,3,94,47,0,772,773,5,63,0,0,773,775,3,94,47,0,774,772,1,0,0,0,774,
	775,1,0,0,0,775,777,1,0,0,0,776,770,1,0,0,0,776,777,1,0,0,0,777,929,1,0,
	0,0,778,779,5,184,0,0,779,782,5,176,0,0,780,781,7,2,0,0,781,783,3,154,77,
	0,782,780,1,0,0,0,782,783,1,0,0,0,783,790,1,0,0,0,784,785,5,113,0,0,785,
	788,3,94,47,0,786,787,5,63,0,0,787,789,3,94,47,0,788,786,1,0,0,0,788,789,
	1,0,0,0,789,791,1,0,0,0,790,784,1,0,0,0,790,791,1,0,0,0,791,929,1,0,0,0,
	792,793,5,184,0,0,793,800,5,30,0,0,794,795,5,113,0,0,795,798,3,94,47,0,
	796,797,5,63,0,0,797,799,3,94,47,0,798,796,1,0,0,0,798,799,1,0,0,0,799,
	801,1,0,0,0,800,794,1,0,0,0,800,801,1,0,0,0,801,929,1,0,0,0,802,803,5,184,
	0,0,803,804,5,32,0,0,804,805,7,2,0,0,805,929,3,142,71,0,806,807,5,184,0,
	0,807,808,5,188,0,0,808,809,5,76,0,0,809,929,3,142,71,0,810,811,5,184,0,
	0,811,812,5,188,0,0,812,813,5,76,0,0,813,814,5,2,0,0,814,815,3,54,27,0,
	815,816,5,3,0,0,816,929,1,0,0,0,817,819,5,184,0,0,818,820,5,40,0,0,819,
	818,1,0,0,0,819,820,1,0,0,0,820,821,1,0,0,0,821,824,5,170,0,0,822,823,7,
	2,0,0,823,825,3,154,77,0,824,822,1,0,0,0,824,825,1,0,0,0,825,929,1,0,0,
	0,826,827,5,184,0,0,827,828,5,169,0,0,828,831,5,84,0,0,829,830,7,2,0,0,
	830,832,3,154,77,0,831,829,1,0,0,0,831,832,1,0,0,0,832,929,1,0,0,0,833,
	834,5,53,0,0,834,929,3,142,71,0,835,836,5,52,0,0,836,929,3,142,71,0,837,
	838,5,184,0,0,838,845,5,81,0,0,839,840,5,113,0,0,840,843,3,94,47,0,841,
	842,5,63,0,0,842,844,3,94,47,0,843,841,1,0,0,0,843,844,1,0,0,0,844,846,
	1,0,0,0,845,839,1,0,0,0,845,846,1,0,0,0,846,929,1,0,0,0,847,848,5,184,0,
	0,848,855,5,181,0,0,849,850,5,113,0,0,850,853,3,94,47,0,851,852,5,63,0,
	0,852,854,3,94,47,0,853,851,1,0,0,0,853,854,1,0,0,0,854,856,1,0,0,0,855,
	849,1,0,0,0,855,856,1,0,0,0,856,929,1,0,0,0,857,858,5,182,0,0,858,859,5,
	181,0,0,859,860,3,142,71,0,860,861,5,229,0,0,861,862,3,84,42,0,862,929,
	1,0,0,0,863,864,5,162,0,0,864,865,5,181,0,0,865,929,3,142,71,0,866,867,
	5,187,0,0,867,876,5,202,0,0,868,873,3,134,67,0,869,870,5,4,0,0,870,872,
	3,134,67,0,871,869,1,0,0,0,872,875,1,0,0,0,873,871,1,0,0,0,873,874,1,0,
	0,0,874,877,1,0,0,0,875,873,1,0,0,0,876,868,1,0,0,0,876,877,1,0,0,0,877,
	929,1,0,0,0,878,880,5,34,0,0,879,881,5,225,0,0,880,879,1,0,0,0,880,881,
	1,0,0,0,881,929,1,0,0,0,882,884,5,171,0,0,883,885,5,225,0,0,884,883,1,0,
	0,0,884,885,1,0,0,0,885,929,1,0,0,0,886,887,5,150,0,0,887,888,3,154,77,
	0,888,889,5,78,0,0,889,890,3,6,3,0,890,929,1,0,0,0,891,892,5,49,0,0,892,
	893,5,150,0,0,893,929,3,154,77,0,894,895,5,66,0,0,895,905,3,154,77,0,896,
	897,5,216,0,0,897,902,3,84,42,0,898,899,5,4,0,0,899,901,3,84,42,0,900,898,
	1,0,0,0,901,904,1,0,0,0,902,900,1,0,0,0,902,903,1,0,0,0,903,906,1,0,0,0,
	904,902,1,0,0,0,905,896,1,0,0,0,905,906,1,0,0,0,906,929,1,0,0,0,907,908,
	5,53,0,0,908,909,5,96,0,0,909,929,3,154,77,0,910,911,5,53,0,0,911,912,5,
	144,0,0,912,929,3,154,77,0,913,914,5,213,0,0,914,915,3,142,71,0,915,916,
	5,182,0,0,916,921,3,130,65,0,917,918,5,4,0,0,918,920,3,130,65,0,919,917,
	1,0,0,0,920,923,1,0,0,0,921,919,1,0,0,0,921,922,1,0,0,0,922,926,1,0,0,0,
	923,921,1,0,0,0,924,925,5,223,0,0,925,927,3,86,43,0,926,924,1,0,0,0,926,
	927,1,0,0,0,927,929,1,0,0,0,928,187,1,0,0,0,928,188,1,0,0,0,928,190,1,0,
	0,0,928,195,1,0,0,0,928,207,1,0,0,0,928,217,1,0,0,0,928,224,1,0,0,0,928,
	258,1,0,0,0,928,284,1,0,0,0,928,291,1,0,0,0,928,299,1,0,0,0,928,306,1,0,
	0,0,928,309,1,0,0,0,928,320,1,0,0,0,928,337,1,0,0,0,928,352,1,0,0,0,928,
	368,1,0,0,0,928,378,1,0,0,0,928,393,1,0,0,0,928,409,1,0,0,0,928,425,1,0,
	0,0,928,436,1,0,0,0,928,442,1,0,0,0,928,460,1,0,0,0,928,474,1,0,0,0,928,
	485,1,0,0,0,928,492,1,0,0,0,928,517,1,0,0,0,928,525,1,0,0,0,928,532,1,0,
	0,0,928,563,1,0,0,0,928,571,1,0,0,0,928,584,1,0,0,0,928,599,1,0,0,0,928,
	607,1,0,0,0,928,610,1,0,0,0,928,631,1,0,0,0,928,652,1,0,0,0,928,659,1,0,
	0,0,928,684,1,0,0,0,928,710,1,0,0,0,928,719,1,0,0,0,928,740,1,0,0,0,928,
	744,1,0,0,0,928,748,1,0,0,0,928,752,1,0,0,0,928,757,1,0,0,0,928,764,1,0,
	0,0,928,778,1,0,0,0,928,792,1,0,0,0,928,802,1,0,0,0,928,806,1,0,0,0,928,
	810,1,0,0,0,928,817,1,0,0,0,928,826,1,0,0,0,928,833,1,0,0,0,928,835,1,0,
	0,0,928,837,1,0,0,0,928,847,1,0,0,0,928,857,1,0,0,0,928,863,1,0,0,0,928,
	866,1,0,0,0,928,878,1,0,0,0,928,882,1,0,0,0,928,886,1,0,0,0,928,891,1,0,
	0,0,928,894,1,0,0,0,928,907,1,0,0,0,928,910,1,0,0,0,928,913,1,0,0,0,929,
	7,1,0,0,0,930,932,3,10,5,0,931,930,1,0,0,0,931,932,1,0,0,0,932,933,1,0,
	0,0,933,934,3,46,23,0,934,9,1,0,0,0,935,937,5,224,0,0,936,938,5,156,0,0,
	937,936,1,0,0,0,937,938,1,0,0,0,938,939,1,0,0,0,939,944,3,62,31,0,940,941,
	5,4,0,0,941,943,3,62,31,0,942,940,1,0,0,0,943,946,1,0,0,0,944,942,1,0,0,
	0,944,945,1,0,0,0,945,11,1,0,0,0,946,944,1,0,0,0,947,951,3,158,79,0,948,
	951,3,14,7,0,949,951,3,16,8,0,950,947,1,0,0,0,950,948,1,0,0,0,950,949,1,
	0,0,0,951,13,1,0,0,0,952,953,3,154,77,0,953,956,3,114,57,0,954,955,5,131,
	0,0,955,957,5,132,0,0,956,954,1,0,0,0,956,957,1,0,0,0,957,960,1,0,0,0,958,
	959,5,33,0,0,959,961,3,94,47,0,960,958,1,0,0,0,960,961,1,0,0,0,961,964,
	1,0,0,0,962,963,5,224,0,0,963,965,3,18,9,0,964,962,1,0,0,0,964,965,1,0,
	0,0,965,15,1,0,0,0,966,967,5,113,0,0,967,970,3,142,71,0,968,969,7,3,0,0,
	969,971,5,153,0,0,970,968,1,0,0,0,970,971,1,0,0,0,971,17,1,0,0,0,972,973,
	5,2,0,0,973,978,3,20,10,0,974,975,5,4,0,0,975,977,3,20,10,0,976,974,1,0,
	0,0,977,980,1,0,0,0,978,976,1,0,0,0,978,979,1,0,0,0,979,981,1,0,0,0,980,
	978,1,0,0,0,981,982,5,3,0,0,982,19,1,0,0,0,983,984,3,154,77,0,984,985,5,
	229,0,0,985,986,3,84,42,0,986,21,1,0,0,0,987,988,3,154,77,0,988,989,3,114,
	57,0,989,23,1,0,0,0,990,992,3,26,13,0,991,990,1,0,0,0,992,995,1,0,0,0,993,
	991,1,0,0,0,993,994,1,0,0,0,994,25,1,0,0,0,995,993,1,0,0,0,996,997,5,108,
	0,0,997,1001,3,38,19,0,998,1001,3,40,20,0,999,1001,3,42,21,0,1000,996,1,
	0,0,0,1000,998,1,0,0,0,1000,999,1,0,0,0,1001,27,1,0,0,0,1002,1004,3,30,
	15,0,1003,1002,1,0,0,0,1004,1007,1,0,0,0,1005,1003,1,0,0,0,1005,1006,1,
	0,0,0,1006,29,1,0,0,0,1007,1005,1,0,0,0,1008,1009,3,42,21,0,1009,31,1,0,
	0,0,1010,1013,3,34,17,0,1011,1013,3,36,18,0,1012,1010,1,0,0,0,1012,1011,
	1,0,0,0,1013,33,1,0,0,0,1014,1015,5,165,0,0,1015,1016,3,84,42,0,1016,35,
	1,0,0,0,1017,1020,5,70,0,0,1018,1019,5,122,0,0,1019,1021,3,44,22,0,1020,
	1018,1,0,0,0,1020,1021,1,0,0,0,1021,37,1,0,0,0,1022,1025,5,186,0,0,1023,
	1025,3,154,77,0,1024,1022,1,0,0,0,1024,1023,1,0,0,0,1025,39,1,0,0,0,1026,
	1030,5,54,0,0,1027,1028,5,131,0,0,1028,1030,5,54,0,0,1029,1026,1,0,0,0,
	1029,1027,1,0,0,0,1030,41,1,0,0,0,1031,1032,5,166,0,0,1032,1033,5,132,0,
	0,1033,1034,5,137,0,0,1034,1035,5,132,0,0,1035,1041,5,96,0,0,1036,1037,
	5,26,0,0,1037,1038,5,137,0,0,1038,1039,5,132,0,0,1039,1041,5,96,0,0,1040,
	1031,1,0,0,0,1040,1036,1,0,0,0,1041,43,1,0,0,0,1042,1043,3,154,77,0,1043,
	45,1,0,0,0,1044,1055,3,48,24,0,1045,1046,5,141,0,0,1046,1047,5,24,0,0,1047,
	1052,3,52,26,0,1048,1049,5,4,0,0,1049,1051,3,52,26,0,1050,1048,1,0,0,0,
	1051,1054,1,0,0,0,1052,1050,1,0,0,0,1052,1053,1,0,0,0,1053,1056,1,0,0,0,
	1054,1052,1,0,0,0,1055,1045,1,0,0,0,1055,1056,1,0,0,0,1056,1062,1,0,0,0,
	1057,1058,5,136,0,0,1058,1060,5,244,0,0,1059,1061,7,4,0,0,1060,1059,1,0,
	0,0,1060,1061,1,0,0,0,1061,1063,1,0,0,0,1062,1057,1,0,0,0,1062,1063,1,0,
	0,0,1063,1073,1,0,0,0,1064,1065,5,114,0,0,1065,1072,7,5,0,0,1066,1067,5,
	72,0,0,1067,1068,5,74,0,0,1068,1069,5,244,0,0,1069,1070,5,174,0,0,1070,
	1072,5,138,0,0,1071,1064,1,0,0,0,1071,1066,1,0,0,0,1072,1074,1,0,0,0,1073,
	1071,1,0,0,0,1073,1074,1,0,0,0,1074,47,1,0,0,0,1075,1076,6,24,-1,0,1076,
	1077,3,50,25,0,1077,1092,1,0,0,0,1078,1079,10,2,0,0,1079,1081,5,98,0,0,
	1080,1082,3,64,32,0,1081,1080,1,0,0,0,1081,1082,1,0,0,0,1082,1083,1,0,0,
	0,1083,1091,3,48,24,3,1084,1085,10,1,0,0,1085,1087,7,6,0,0,1086,1088,3,
	64,32,0,1087,1086,1,0,0,0,1087,1088,1,0,0,0,1088,1089,1,0,0,0,1089,1091,
	3,48,24,2,1090,1078,1,0,0,0,1090,1084,1,0,0,0,1091,1094,1,0,0,0,1092,1090,
	1,0,0,0,1092,1093,1,0,0,0,1093,49,1,0,0,0,1094,1092,1,0,0,0,1095,1112,3,
	54,27,0,1096,1097,5,193,0,0,1097,1112,3,142,71,0,1098,1099,5,218,0,0,1099,
	1104,3,84,42,0,1100,1101,5,4,0,0,1101,1103,3,84,42,0,1102,1100,1,0,0,0,
	1103,1106,1,0,0,0,1104,1102,1,0,0,0,1104,1105,1,0,0,0,1105,1112,1,0,0,0,
	1106,1104,1,0,0,0,1107,1108,5,2,0,0,1108,1109,3,46,23,0,1109,1110,5,3,0,
	0,1110,1112,1,0,0,0,1111,1095,1,0,0,0,1111,1096,1,0,0,0,1111,1098,1,0,0,
	0,1111,1107,1,0,0,0,1112,51,1,0,0,0,1113,1115,3,84,42,0,1114,1116,7,7,0,
	0,1115,1114,1,0,0,0,1115,1116,1,0,0,0,1116,1119,1,0,0,0,1117,1118,5,134,
	0,0,1118,1120,7,8,0,0,1119,1117,1,0,0,0,1119,1120,1,0,0,0,1120,53,1,0,0,
	0,1121,1123,5,179,0,0,1122,1124,3,64,32,0,1123,1122,1,0,0,0,1123,1124,1,
	0,0,0,1124,1125,1,0,0,0,1125,1130,3,66,33,0,1126,1127,5,4,0,0,1127,1129,
	3,66,33,0,1128,1126,1,0,0,0,1129,1132,1,0,0,0,1130,1128,1,0,0,0,1130,1131,
	1,0,0,0,1131,1142,1,0,0,0,1132,1130,1,0,0,0,1133,1134,5,78,0,0,1134,1139,
	3,68,34,0,1135,1136,5,4,0,0,1136,1138,3,68,34,0,1137,1135,1,0,0,0,1138,
	1141,1,0,0,0,1139,1137,1,0,0,0,1139,1140,1,0,0,0,1140,1143,1,0,0,0,1141,
	1139,1,0,0,0,1142,1133,1,0,0,0,1142,1143,1,0,0,0,1143,1146,1,0,0,0,1144,
	1145,5,223,0,0,1145,1147,3,86,43,0,1146,1144,1,0,0,0,1146,1147,1,0,0,0,
	1147,1151,1,0,0,0,1148,1149,5,86,0,0,1149,1150,5,24,0,0,1150,1152,3,56,
	28,0,1151,1148,1,0,0,0,1151,1152,1,0,0,0,1152,1155,1,0,0,0,1153,1154,5,
	89,0,0,1154,1156,3,86,43,0,1155,1153,1,0,0,0,1155,1156,1,0,0,0,1156,55,
	1,0,0,0,1157,1159,3,64,32,0,1158,1157,1,0,0,0,1158,1159,1,0,0,0,1159,1160,
	1,0,0,0,1160,1165,3,58,29,0,1161,1162,5,4,0,0,1162,1164,3,58,29,0,1163,
	1161,1,0,0,0,1164,1167,1,0,0,0,1165,1163,1,0,0,0,1165,1166,1,0,0,0,1166,
	57,1,0,0,0,1167,1165,1,0,0,0,1168,1209,3,60,30,0,1169,1170,5,172,0,0,1170,
	1179,5,2,0,0,1171,1176,3,84,42,0,1172,1173,5,4,0,0,1173,1175,3,84,42,0,
	1174,1172,1,0,0,0,1175,1178,1,0,0,0,1176,1174,1,0,0,0,1176,1177,1,0,0,0,
	1177,1180,1,0,0,0,1178,1176,1,0,0,0,1179,1171,1,0,0,0,1179,1180,1,0,0,0,
	1180,1181,1,0,0,0,1181,1209,5,3,0,0,1182,1183,5,39,0,0,1183,1192,5,2,0,
	0,1184,1189,3,84,42,0,1185,1186,5,4,0,0,1186,1188,3,84,42,0,1187,1185,1,
	0,0,0,1188,1191,1,0,0,0,1189,1187,1,0,0,0,1189,1190,1,0,0,0,1190,1193,1,
	0,0,0,1191,1189,1,0,0,0,1192,1184,1,0,0,0,1192,1193,1,0,0,0,1193,1194,1,
	0,0,0,1194,1209,5,3,0,0,1195,1196,5,87,0,0,1196,1197,5,183,0,0,1197,1198,
	5,2,0,0,1198,1203,3,60,30,0,1199,1200,5,4,0,0,1200,1202,3,60,30,0,1201,
	1199,1,0,0,0,1202,1205,1,0,0,0,1203,1201,1,0,0,0,1203,1204,1,0,0,0,1204,
	1206,1,0,0,0,1205,1203,1,0,0,0,1206,1207,5,3,0,0,1207,1209,1,0,0,0,1208,
	1168,1,0,0,0,1208,1169,1,0,0,0,1208,1182,1,0,0,0,1208,1195,1,0,0,0,1209,
	59,1,0,0,0,1210,1219,5,2,0,0,1211,1216,3,84,42,0,1212,1213,5,4,0,0,1213,
	1215,3,84,42,0,1214,1212,1,0,0,0,1215,1218,1,0,0,0,1216,1214,1,0,0,0,1216,
	1217,1,0,0,0,1217,1220,1,0,0,0,1218,1216,1,0,0,0,1219,1211,1,0,0,0,1219,
	1220,1,0,0,0,1220,1221,1,0,0,0,1221,1224,5,3,0,0,1222,1224,3,84,42,0,1223,
	1210,1,0,0,0,1223,1222,1,0,0,0,1224,61,1,0,0,0,1225,1227,3,154,77,0,1226,
	1228,3,80,40,0,1227,1226,1,0,0,0,1227,1228,1,0,0,0,1228,1229,1,0,0,0,1229,
	1230,5,18,0,0,1230,1231,5,2,0,0,1231,1232,3,8,4,0,1232,1233,5,3,0,0,1233,
	63,1,0,0,0,1234,1235,7,9,0,0,1235,65,1,0,0,0,1236,1241,3,84,42,0,1237,1239,
	5,18,0,0,1238,1237,1,0,0,0,1238,1239,1,0,0,0,1239,1240,1,0,0,0,1240,1242,
	3,154,77,0,1241,1238,1,0,0,0,1241,1242,1,0,0,0,1242,1249,1,0,0,0,1243,1244,
	3,142,71,0,1244,1245,5,1,0,0,1245,1246,5,237,0,0,1246,1249,1,0,0,0,1247,
	1249,5,237,0,0,1248,1236,1,0,0,0,1248,1243,1,0,0,0,1248,1247,1,0,0,0,1249,
	67,1,0,0,0,1250,1251,6,34,-1,0,1251,1252,3,74,37,0,1252,1271,1,0,0,0,1253,
	1267,10,2,0,0,1254,1255,5,38,0,0,1255,1256,5,106,0,0,1256,1268,3,74,37,
	0,1257,1258,3,70,35,0,1258,1259,5,106,0,0,1259,1260,3,68,34,0,1260,1261,
	3,72,36,0,1261,1268,1,0,0,0,1262,1263,5,123,0,0,1263,1264,3,70,35,0,1264,
	1265,5,106,0,0,1265,1266,3,74,37,0,1266,1268,1,0,0,0,1267,1254,1,0,0,0,
	1267,1257,1,0,0,0,1267,1262,1,0,0,0,1268,1270,1,0,0,0,1269,1253,1,0,0,0,
	1270,1273,1,0,0,0,1271,1269,1,0,0,0,1271,1272,1,0,0,0,1272,69,1,0,0,0,1273,
	1271,1,0,0,0,1274,1276,5,95,0,0,1275,1274,1,0,0,0,1275,1276,1,0,0,0,1276,
	1290,1,0,0,0,1277,1279,5,111,0,0,1278,1280,5,143,0,0,1279,1278,1,0,0,0,
	1279,1280,1,0,0,0,1280,1290,1,0,0,0,1281,1283,5,168,0,0,1282,1284,5,143,
	0,0,1283,1282,1,0,0,0,1283,1284,1,0,0,0,1284,1290,1,0,0,0,1285,1287,5,79,
	0,0,1286,1288,5,143,0,0,1287,1286,1,0,0,0,1287,1288,1,0,0,0,1288,1290,1,
	0,0,0,1289,1275,1,0,0,0,1289,1277,1,0,0,0,1289,1281,1,0,0,0,1289,1285,1,
	0,0,0,1290,71,1,0,0,0,1291,1292,5,137,0,0,1292,1306,3,86,43,0,1293,1294,
	5,216,0,0,1294,1295,5,2,0,0,1295,1300,3,154,77,0,1296,1297,5,4,0,0,1297,
	1299,3,154,77,0,1298,1296,1,0,0,0,1299,1302,1,0,0,0,1300,1298,1,0,0,0,1300,
	1301,1,0,0,0,1301,1303,1,0,0,0,1302,1300,1,0,0,0,1303,1304,5,3,0,0,1304,
	1306,1,0,0,0,1305,1291,1,0,0,0,1305,1293,1,0,0,0,1306,73,1,0,0,0,1307,1314,
	3,78,39,0,1308,1309,5,195,0,0,1309,1310,3,76,38,0,1310,1311,5,2,0,0,1311,
	1312,3,84,42,0,1312,1313,5,3,0,0,1313,1315,1,0,0,0,1314,1308,1,0,0,0,1314,
	1315,1,0,0,0,1315,75,1,0,0,0,1316,1317,7,10,0,0,1317,77,1,0,0,0,1318,1326,
	3,82,41,0,1319,1321,5,18,0,0,1320,1319,1,0,0,0,1320,1321,1,0,0,0,1321,1322,
	1,0,0,0,1322,1324,3,154,77,0,1323,1325,3,80,40,0,1324,1323,1,0,0,0,1324,
	1325,1,0,0,0,1325,1327,1,0,0,0,1326,1320,1,0,0,0,1326,1327,1,0,0,0,1327,
	79,1,0,0,0,1328,1329,5,2,0,0,1329,1334,3,154,77,0,1330,1331,5,4,0,0,1331,
	1333,3,154,77,0,1332,1330,1,0,0,0,1333,1336,1,0,0,0,1334,1332,1,0,0,0,1334,
	1335,1,0,0,0,1335,1337,1,0,0,0,1336,1334,1,0,0,0,1337,1338,5,3,0,0,1338,
	81,1,0,0,0,1339,1341,3,142,71,0,1340,1342,3,144,72,0,1341,1340,1,0,0,0,
	1341,1342,1,0,0,0,1342,1372,1,0,0,0,1343,1344,5,2,0,0,1344,1345,3,8,4,0,
	1345,1346,5,3,0,0,1346,1372,1,0,0,0,1347,1348,5,212,0,0,1348,1349,5,2,0,
	0,1349,1354,3,84,42,0,1350,1351,5,4,0,0,1351,1353,3,84,42,0,1352,1350,1,
	0,0,0,1353,1356,1,0,0,0,1354,1352,1,0,0,0,1354,1355,1,0,0,0,1355,1357,1,
	0,0,0,1356,1354,1,0,0,0,1357,1360,5,3,0,0,1358,1359,5,224,0,0,1359,1361,
	5,142,0,0,1360,1358,1,0,0,0,1360,1361,1,0,0,0,1361,1372,1,0,0,0,1362,1363,
	5,110,0,0,1363,1364,5,2,0,0,1364,1365,3,8,4,0,1365,1366,5,3,0,0,1366,1372,
	1,0,0,0,1367,1368,5,2,0,0,1368,1369,3,68,34,0,1369,1370,5,3,0,0,1370,1372,
	1,0,0,0,1371,1339,1,0,0,0,1371,1343,1,0,0,0,1371,1347,1,0,0,0,1371,1362,
	1,0,0,0,1371,1367,1,0,0,0,1372,83,1,0,0,0,1373,1374,3,86,43,0,1374,85,1,
	0,0,0,1375,1376,6,43,-1,0,1376,1378,3,90,45,0,1377,1379,3,88,44,0,1378,
	1377,1,0,0,0,1378,1379,1,0,0,0,1379,1383,1,0,0,0,1380,1381,5,131,0,0,1381,
	1383,3,86,43,3,1382,1375,1,0,0,0,1382,1380,1,0,0,0,1383,1392,1,0,0,0,1384,
	1385,10,2,0,0,1385,1386,5,15,0,0,1386,1391,3,86,43,3,1387,1388,10,1,0,0,
	1388,1389,5,140,0,0,1389,1391,3,86,43,2,1390,1384,1,0,0,0,1390,1387,1,0,
	0,0,1391,1394,1,0,0,0,1392,1390,1,0,0,0,1392,1393,1,0,0,0,1393,87,1,0,0,
	0,1394,1392,1,0,0,0,1395,1396,3,100,50,0,1396,1397,3,90,45,0,1397,1457,
	1,0,0,0,1398,1399,3,100,50,0,1399,1400,3,102,51,0,1400,1401,5,2,0,0,1401,
	1402,3,8,4,0,1402,1403,5,3,0,0,1403,1457,1,0,0,0,1404,1406,5,131,0,0,1405,
	1404,1,0,0,0,1405,1406,1,0,0,0,1406,1407,1,0,0,0,1407,1408,5,23,0,0,1408,
	1409,3,90,45,0,1409,1410,5,15,0,0,1410,1411,3,90,45,0,1411,1457,1,0,0,0,
	1412,1414,5,131,0,0,1413,1412,1,0,0,0,1413,1414,1,0,0,0,1414,1415,1,0,0,
	0,1415,1416,5,93,0,0,1416,1417,5,2,0,0,1417,1422,3,84,42,0,1418,1419,5,
	4,0,0,1419,1421,3,84,42,0,1420,1418,1,0,0,0,1421,1424,1,0,0,0,1422,1420,
	1,0,0,0,1422,1423,1,0,0,0,1423,1425,1,0,0,0,1424,1422,1,0,0,0,1425,1426,
	5,3,0,0,1426,1457,1,0,0,0,1427,1429,5,131,0,0,1428,1427,1,0,0,0,1428,1429,
	1,0,0,0,1429,1430,1,0,0,0,1430,1431,5,93,0,0,1431,1432,5,2,0,0,1432,1433,
	3,8,4,0,1433,1434,5,3,0,0,1434,1457,1,0,0,0,1435,1437,5,131,0,0,1436,1435,
	1,0,0,0,1436,1437,1,0,0,0,1437,1438,1,0,0,0,1438,1439,5,113,0,0,1439,1442,
	3,90,45,0,1440,1441,5,63,0,0,1441,1443,3,90,45,0,1442,1440,1,0,0,0,1442,
	1443,1,0,0,0,1443,1457,1,0,0,0,1444,1446,5,103,0,0,1445,1447,5,131,0,0,
	1446,1445,1,0,0,0,1446,1447,1,0,0,0,1447,1448,1,0,0,0,1448,1457,5,132,0,
	0,1449,1451,5,103,0,0,1450,1452,5,131,0,0,1451,1450,1,0,0,0,1451,1452,1,
	0,0,0,1452,1453,1,0,0,0,1453,1454,5,56,0,0,1454,1455,5,78,0,0,1455,1457,
	3,90,45,0,1456,1395,1,0,0,0,1456,1398,1,0,0,0,1456,1405,1,0,0,0,1456,1413,
	1,0,0,0,1456,1428,1,0,0,0,1456,1436,1,0,0,0,1456,1444,1,0,0,0,1456,1449,
	1,0,0,0,1457,89,1,0,0,0,1458,1459,6,45,-1,0,1459,1463,3,92,46,0,1460,1461,
	7,11,0,0,1461,1463,3,90,45,4,1462,1458,1,0,0,0,1462,1460,1,0,0,0,1463,1478,
	1,0,0,0,1464,1465,10,3,0,0,1465,1466,7,12,0,0,1466,1477,3,90,45,4,1467,
	1468,10,2,0,0,1468,1469,7,11,0,0,1469,1477,3,90,45,3,1470,1471,10,1,0,0,
	1471,1472,5,240,0,0,1472,1477,3,90,45,2,1473,1474,10,5,0,0,1474,1475,5,
	20,0,0,1475,1477,3,98,49,0,1476,1464,1,0,0,0,1476,1467,1,0,0,0,1476,1470,
	1,0,0,0,1476,1473,1,0,0,0,1477,1480,1,0,0,0,1478,1476,1,0,0,0,1478,1479,
	1,0,0,0,1479,91,1,0,0,0,1480,1478,1,0,0,0,1481,1482,6,46,-1,0,1482,1721,
	5,132,0,0,1483,1721,3,106,53,0,1484,1485,3,154,77,0,1485,1486,3,94,47,0,
	1486,1721,1,0,0,0,1487,1488,5,253,0,0,1488,1721,3,94,47,0,1489,1721,3,156,
	78,0,1490,1721,3,104,52,0,1491,1721,3,94,47,0,1492,1721,5,243,0,0,1493,
	1721,5,5,0,0,1494,1495,5,148,0,0,1495,1496,5,2,0,0,1496,1497,3,90,45,0,
	1497,1498,5,93,0,0,1498,1499,3,90,45,0,1499,1500,5,3,0,0,1500,1721,1,0,
	0,0,1501,1502,5,2,0,0,1502,1505,3,84,42,0,1503,1504,5,4,0,0,1504,1506,3,
	84,42,0,1505,1503,1,0,0,0,1506,1507,1,0,0,0,1507,1505,1,0,0,0,1507,1508,
	1,0,0,0,1508,1509,1,0,0,0,1509,1510,5,3,0,0,1510,1721,1,0,0,0,1511,1512,
	5,173,0,0,1512,1513,5,2,0,0,1513,1518,3,84,42,0,1514,1515,5,4,0,0,1515,
	1517,3,84,42,0,1516,1514,1,0,0,0,1517,1520,1,0,0,0,1518,1516,1,0,0,0,1518,
	1519,1,0,0,0,1519,1521,1,0,0,0,1520,1518,1,0,0,0,1521,1522,5,3,0,0,1522,
	1721,1,0,0,0,1523,1524,3,142,71,0,1524,1525,5,2,0,0,1525,1526,5,237,0,0,
	1526,1528,5,3,0,0,1527,1529,3,122,61,0,1528,1527,1,0,0,0,1528,1529,1,0,
	0,0,1529,1531,1,0,0,0,1530,1532,3,124,62,0,1531,1530,1,0,0,0,1531,1532,
	1,0,0,0,1532,1721,1,0,0,0,1533,1534,3,142,71,0,1534,1546,5,2,0,0,1535,1537,
	3,64,32,0,1536,1535,1,0,0,0,1536,1537,1,0,0,0,1537,1538,1,0,0,0,1538,1543,
	3,84,42,0,1539,1540,5,4,0,0,1540,1542,3,84,42,0,1541,1539,1,0,0,0,1542,
	1545,1,0,0,0,1543,1541,1,0,0,0,1543,1544,1,0,0,0,1544,1547,1,0,0,0,1545,
	1543,1,0,0,0,1546,1536,1,0,0,0,1546,1547,1,0,0,0,1547,1558,1,0,0,0,1548,
	1549,5,141,0,0,1549,1550,5,24,0,0,1550,1555,3,52,26,0,1551,1552,5,4,0,0,
	1552,1554,3,52,26,0,1553,1551,1,0,0,0,1554,1557,1,0,0,0,1555,1553,1,0,0,
	0,1555,1556,1,0,0,0,1556,1559,1,0,0,0,1557,1555,1,0,0,0,1558,1548,1,0,0,
	0,1558,1559,1,0,0,0,1559,1560,1,0,0,0,1560,1562,5,3,0,0,1561,1563,3,122,
	61,0,1562,1561,1,0,0,0,1562,1563,1,0,0,0,1563,1568,1,0,0,0,1564,1566,3,
	96,48,0,1565,1564,1,0,0,0,1565,1566,1,0,0,0,1566,1567,1,0,0,0,1567,1569,
	3,124,62,0,1568,1565,1,0,0,0,1568,1569,1,0,0,0,1569,1721,1,0,0,0,1570,1571,
	3,154,77,0,1571,1572,5,6,0,0,1572,1573,3,84,42,0,1573,1721,1,0,0,0,1574,
	1583,5,2,0,0,1575,1580,3,154,77,0,1576,1577,5,4,0,0,1577,1579,3,154,77,
	0,1578,1576,1,0,0,0,1579,1582,1,0,0,0,1580,1578,1,0,0,0,1580,1581,1,0,0,
	0,1581,1584,1,0,0,0,1582,1580,1,0,0,0,1583,1575,1,0,0,0,1583,1584,1,0,0,
	0,1584,1585,1,0,0,0,1585,1586,5,3,0,0,1586,1587,5,6,0,0,1587,1721,3,84,
	42,0,1588,1589,5,2,0,0,1589,1590,3,8,4,0,1590,1591,5,3,0,0,1591,1721,1,
	0,0,0,1592,1593,5,67,0,0,1593,1594,5,2,0,0,1594,1595,3,8,4,0,1595,1596,
	5,3,0,0,1596,1721,1,0,0,0,1597,1598,5,28,0,0,1598,1600,3,90,45,0,1599,1601,
	3,120,60,0,1600,1599,1,0,0,0,1601,1602,1,0,0,0,1602,1600,1,0,0,0,1602,1603,
	1,0,0,0,1603,1606,1,0,0,0,1604,1605,5,59,0,0,1605,1607,3,84,42,0,1606,1604,
	1,0,0,0,1606,1607,1,0,0,0,1607,1608,1,0,0,0,1608,1609,5,61,0,0,1609,1721,
	1,0,0,0,1610,1612,5,28,0,0,1611,1613,3,120,60,0,1612,1611,1,0,0,0,1613,
	1614,1,0,0,0,1614,1612,1,0,0,0,1614,1615,1,0,0,0,1615,1618,1,0,0,0,1616,
	1617,5,59,0,0,1617,1619,3,84,42,0,1618,1616,1,0,0,0,1618,1619,1,0,0,0,1619,
	1620,1,0,0,0,1620,1621,5,61,0,0,1621,1721,1,0,0,0,1622,1623,5,29,0,0,1623,
	1624,5,2,0,0,1624,1625,3,84,42,0,1625,1626,5,18,0,0,1626,1627,3,114,57,
	0,1627,1628,5,3,0,0,1628,1721,1,0,0,0,1629,1630,5,205,0,0,1630,1631,5,2,
	0,0,1631,1632,3,84,42,0,1632,1633,5,18,0,0,1633,1634,3,114,57,0,1634,1635,
	5,3,0,0,1635,1721,1,0,0,0,1636,1637,5,17,0,0,1637,1646,5,7,0,0,1638,1643,
	3,84,42,0,1639,1640,5,4,0,0,1640,1642,3,84,42,0,1641,1639,1,0,0,0,1642,
	1645,1,0,0,0,1643,1641,1,0,0,0,1643,1644,1,0,0,0,1644,1647,1,0,0,0,1645,
	1643,1,0,0,0,1646,1638,1,0,0,0,1646,1647,1,0,0,0,1647,1648,1,0,0,0,1648,
	1721,5,8,0,0,1649,1721,3,154,77,0,1650,1721,5,41,0,0,1651,1655,5,43,0,0,
	1652,1653,5,2,0,0,1653,1654,5,244,0,0,1654,1656,5,3,0,0,1655,1652,1,0,0,
	0,1655,1656,1,0,0,0,1656,1721,1,0,0,0,1657,1661,5,44,0,0,1658,1659,5,2,
	0,0,1659,1660,5,244,0,0,1660,1662,5,3,0,0,1661,1658,1,0,0,0,1661,1662,1,
	0,0,0,1662,1721,1,0,0,0,1663,1667,5,115,0,0,1664,1665,5,2,0,0,1665,1666,
	5,244,0,0,1666,1668,5,3,0,0,1667,1664,1,0,0,0,1667,1668,1,0,0,0,1668,1721,
	1,0,0,0,1669,1673,5,116,0,0,1670,1671,5,2,0,0,1671,1672,5,244,0,0,1672,
	1674,5,3,0,0,1673,1670,1,0,0,0,1673,1674,1,0,0,0,1674,1721,1,0,0,0,1675,
	1721,5,45,0,0,1676,1677,5,189,0,0,1677,1678,5,2,0,0,1678,1679,3,90,45,0,
	1679,1680,5,78,0,0,1680,1683,3,90,45,0,1681,1682,5,76,0,0,1682,1684,3,90,
	45,0,1683,1681,1,0,0,0,1683,1684,1,0,0,0,1684,1685,1,0,0,0,1685,1686,5,
	3,0,0,1686,1721,1,0,0,0,1687,1688,5,130,0,0,1688,1689,5,2,0,0,1689,1692,
	3,90,45,0,1690,1691,5,4,0,0,1691,1693,3,110,55,0,1692,1690,1,0,0,0,1692,
	1693,1,0,0,0,1693,1694,1,0,0,0,1694,1695,5,3,0,0,1695,1721,1,0,0,0,1696,
	1697,5,69,0,0,1697,1698,5,2,0,0,1698,1699,3,154,77,0,1699,1700,5,78,0,0,
	1700,1701,3,90,45,0,1701,1702,5,3,0,0,1702,1721,1,0,0,0,1703,1704,5,2,0,
	0,1704,1705,3,84,42,0,1705,1706,5,3,0,0,1706,1721,1,0,0,0,1707,1708,5,87,
	0,0,1708,1717,5,2,0,0,1709,1714,3,142,71,0,1710,1711,5,4,0,0,1711,1713,
	3,142,71,0,1712,1710,1,0,0,0,1713,1716,1,0,0,0,1714,1712,1,0,0,0,1714,1715,
	1,0,0,0,1715,1718,1,0,0,0,1716,1714,1,0,0,0,1717,1709,1,0,0,0,1717,1718,
	1,0,0,0,1718,1719,1,0,0,0,1719,1721,5,3,0,0,1720,1481,1,0,0,0,1720,1483,
	1,0,0,0,1720,1484,1,0,0,0,1720,1487,1,0,0,0,1720,1489,1,0,0,0,1720,1490,
	1,0,0,0,1720,1491,1,0,0,0,1720,1492,1,0,0,0,1720,1493,1,0,0,0,1720,1494,
	1,0,0,0,1720,1501,1,0,0,0,1720,1511,1,0,0,0,1720,1523,1,0,0,0,1720,1533,
	1,0,0,0,1720,1570,1,0,0,0,1720,1574,1,0,0,0,1720,1588,1,0,0,0,1720,1592,
	1,0,0,0,1720,1597,1,0,0,0,1720,1610,1,0,0,0,1720,1622,1,0,0,0,1720,1629,
	1,0,0,0,1720,1636,1,0,0,0,1720,1649,1,0,0,0,1720,1650,1,0,0,0,1720,1651,
	1,0,0,0,1720,1657,1,0,0,0,1720,1663,1,0,0,0,1720,1669,1,0,0,0,1720,1675,
	1,0,0,0,1720,1676,1,0,0,0,1720,1687,1,0,0,0,1720,1696,1,0,0,0,1720,1703,
	1,0,0,0,1720,1707,1,0,0,0,1721,1732,1,0,0,0,1722,1723,10,14,0,0,1723,1724,
	5,7,0,0,1724,1725,3,90,45,0,1725,1726,5,8,0,0,1726,1731,1,0,0,0,1727,1728,
	10,12,0,0,1728,1729,5,1,0,0,1729,1731,3,154,77,0,1730,1722,1,0,0,0,1730,
	1727,1,0,0,0,1731,1734,1,0,0,0,1732,1730,1,0,0,0,1732,1733,1,0,0,0,1733,
	93,1,0,0,0,1734,1732,1,0,0,0,1735,1742,5,241,0,0,1736,1739,5,242,0,0,1737,
	1738,5,207,0,0,1738,1740,5,241,0,0,1739,1737,1,0,0,0,1739,1740,1,0,0,0,
	1740,1742,1,0,0,0,1741,1735,1,0,0,0,1741,1736,1,0,0,0,1742,95,1,0,0,0,1743,
	1744,5,92,0,0,1744,1748,5,134,0,0,1745,1746,5,163,0,0,1746,1748,5,134,0,
	0,1747,1743,1,0,0,0,1747,1745,1,0,0,0,1748,97,1,0,0,0,1749,1750,5,199,0,
	0,1750,1751,5,228,0,0,1751,1756,3,106,53,0,1752,1753,5,199,0,0,1753,1754,
	5,228,0,0,1754,1756,3,94,47,0,1755,1749,1,0,0,0,1755,1752,1,0,0,0,1756,
	99,1,0,0,0,1757,1758,7,13,0,0,1758,101,1,0,0,0,1759,1760,7,14,0,0,1760,
	103,1,0,0,0,1761,1762,7,15,0,0,1762,105,1,0,0,0,1763,1765,5,99,0,0,1764,
	1766,7,11,0,0,1765,1764,1,0,0,0,1765,1766,1,0,0,0,1766,1767,1,0,0,0,1767,
	1768,3,94,47,0,1768,1771,3,108,54,0,1769,1770,5,201,0,0,1770,1772,3,108,
	54,0,1771,1769,1,0,0,0,1771,1772,1,0,0,0,1772,107,1,0,0,0,1773,1774,7,16,
	0,0,1774,109,1,0,0,0,1775,1776,7,17,0,0,1776,111,1,0,0,0,1777,1786,5,2,
	0,0,1778,1783,3,114,57,0,1779,1780,5,4,0,0,1780,1782,3,114,57,0,1781,1779,
	1,0,0,0,1782,1785,1,0,0,0,1783,1781,1,0,0,0,1783,1784,1,0,0,0,1784,1787,
	1,0,0,0,1785,1783,1,0,0,0,1786,1778,1,0,0,0,1786,1787,1,0,0,0,1787,1788,
	1,0,0,0,1788,1789,5,3,0,0,1789,113,1,0,0,0,1790,1791,6,57,-1,0,1791,1792,
	5,17,0,0,1792,1793,5,231,0,0,1793,1794,3,114,57,0,1794,1795,5,233,0,0,1795,
	1838,1,0,0,0,1796,1797,5,118,0,0,1797,1798,5,231,0,0,1798,1799,3,114,57,
	0,1799,1800,5,4,0,0,1800,1801,3,114,57,0,1801,1802,5,233,0,0,1802,1838,
	1,0,0,0,1803,1804,5,173,0,0,1804,1805,5,2,0,0,1805,1806,3,154,77,0,1806,
	1813,3,114,57,0,1807,1808,5,4,0,0,1808,1809,3,154,77,0,1809,1810,3,114,
	57,0,1810,1812,1,0,0,0,1811,1807,1,0,0,0,1812,1815,1,0,0,0,1813,1811,1,
	0,0,0,1813,1814,1,0,0,0,1814,1816,1,0,0,0,1815,1813,1,0,0,0,1816,1817,5,
	3,0,0,1817,1838,1,0,0,0,1818,1830,3,118,59,0,1819,1820,5,2,0,0,1820,1825,
	3,116,58,0,1821,1822,5,4,0,0,1822,1824,3,116,58,0,1823,1821,1,0,0,0,1824,
	1827,1,0,0,0,1825,1823,1,0,0,0,1825,1826,1,0,0,0,1826,1828,1,0,0,0,1827,
	1825,1,0,0,0,1828,1829,5,3,0,0,1829,1831,1,0,0,0,1830,1819,1,0,0,0,1830,
	1831,1,0,0,0,1831,1838,1,0,0,0,1832,1833,5,99,0,0,1833,1834,3,108,54,0,
	1834,1835,5,201,0,0,1835,1836,3,108,54,0,1836,1838,1,0,0,0,1837,1790,1,
	0,0,0,1837,1796,1,0,0,0,1837,1803,1,0,0,0,1837,1818,1,0,0,0,1837,1832,1,
	0,0,0,1838,1843,1,0,0,0,1839,1840,10,6,0,0,1840,1842,5,17,0,0,1841,1839,
	1,0,0,0,1842,1845,1,0,0,0,1843,1841,1,0,0,0,1843,1844,1,0,0,0,1844,115,
	1,0,0,0,1845,1843,1,0,0,0,1846,1849,5,244,0,0,1847,1849,3,114,57,0,1848,
	1846,1,0,0,0,1848,1847,1,0,0,0,1849,117,1,0,0,0,1850,1855,5,251,0,0,1851,
	1855,5,252,0,0,1852,1855,5,253,0,0,1853,1855,3,142,71,0,1854,1850,1,0,0,
	0,1854,1851,1,0,0,0,1854,1852,1,0,0,0,1854,1853,1,0,0,0,1855,119,1,0,0,
	0,1856,1857,5,222,0,0,1857,1858,3,84,42,0,1858,1859,5,198,0,0,1859,1860,
	3,84,42,0,1860,121,1,0,0,0,1861,1862,5,73,0,0,1862,1863,5,2,0,0,1863,1864,
	5,223,0,0,1864,1865,3,86,43,0,1865,1866,5,3,0,0,1866,123,1,0,0,0,1867,1868,
	5,145,0,0,1868,1879,5,2,0,0,1869,1870,5,146,0,0,1870,1871,5,24,0,0,1871,
	1876,3,84,42,0,1872,1873,5,4,0,0,1873,1875,3,84,42,0,1874,1872,1,0,0,0,
	1875,1878,1,0,0,0,1876,1874,1,0,0,0,1876,1877,1,0,0,0,1877,1880,1,0,0,0,
	1878,1876,1,0,0,0,1879,1869,1,0,0,0,1879,1880,1,0,0,0,1880,1891,1,0,0,0,
	1881,1882,5,141,0,0,1882,1883,5,24,0,0,1883,1888,3,52,26,0,1884,1885,5,
	4,0,0,1885,1887,3,52,26,0,1886,1884,1,0,0,0,1887,1890,1,0,0,0,1888,1886,
	1,0,0,0,1888,1889,1,0,0,0,1889,1892,1,0,0,0,1890,1888,1,0,0,0,1891,1881,
	1,0,0,0,1891,1892,1,0,0,0,1892,1894,1,0,0,0,1893,1895,3,126,63,0,1894,1893,
	1,0,0,0,1894,1895,1,0,0,0,1895,1896,1,0,0,0,1896,1897,5,3,0,0,1897,125,
	1,0,0,0,1898,1899,5,154,0,0,1899,1923,3,128,64,0,1900,1901,5,174,0,0,1901,
	1923,3,128,64,0,1902,1903,5,88,0,0,1903,1923,3,128,64,0,1904,1905,5,154,
	0,0,1905,1906,5,23,0,0,1906,1907,3,128,64,0,1907,1908,5,15,0,0,1908,1909,
	3,128,64,0,1909,1923,1,0,0,0,1910,1911,5,174,0,0,1911,1912,5,23,0,0,1912,
	1913,3,128,64,0,1913,1914,5,15,0,0,1914,1915,3,128,64,0,1915,1923,1,0,0,
	0,1916,1917,5,88,0,0,1917,1918,5,23,0,0,1918,1919,3,128,64,0,1919,1920,
	5,15,0,0,1920,1921,3,128,64,0,1921,1923,1,0,0,0,1922,1898,1,0,0,0,1922,
	1900,1,0,0,0,1922,1902,1,0,0,0,1922,1904,1,0,0,0,1922,1910,1,0,0,0,1922,
	1916,1,0,0,0,1923,127,1,0,0,0,1924,1925,5,208,0,0,1925,1934,5,149,0,0,1926,
	1927,5,208,0,0,1927,1934,5,75,0,0,1928,1929,5,40,0,0,1929,1934,5,173,0,
	0,1930,1931,3,84,42,0,1931,1932,7,18,0,0,1932,1934,1,0,0,0,1933,1924,1,
	0,0,0,1933,1926,1,0,0,0,1933,1928,1,0,0,0,1933,1930,1,0,0,0,1934,129,1,
	0,0,0,1935,1936,3,154,77,0,1936,1937,5,229,0,0,1937,1938,3,84,42,0,1938,
	131,1,0,0,0,1939,1940,5,77,0,0,1940,1944,7,19,0,0,1941,1942,5,206,0,0,1942,
	1944,7,20,0,0,1943,1939,1,0,0,0,1943,1941,1,0,0,0,1944,133,1,0,0,0,1945,
	1946,5,104,0,0,1946,1947,5,112,0,0,1947,1951,3,136,68,0,1948,1949,5,155,
	0,0,1949,1951,7,21,0,0,1950,1945,1,0,0,0,1950,1948,1,0,0,0,1951,135,1,0,
	0,0,1952,1953,5,155,0,0,1953,1960,5,209,0,0,1954,1955,5,155,0,0,1955,1960,
	5,35,0,0,1956,1957,5,160,0,0,1957,1960,5,155,0,0,1958,1960,5,180,0,0,1959,
	1952,1,0,0,0,1959,1954,1,0,0,0,1959,1956,1,0,0,0,1959,1958,1,0,0,0,1960,
	137,1,0,0,0,1961,1967,3,84,42,0,1962,1963,3,154,77,0,1963,1964,5,9,0,0,
	1964,1965,3,84,42,0,1965,1967,1,0,0,0,1966,1961,1,0,0,0,1966,1962,1,0,0,
	0,1967,139,1,0,0,0,1968,1973,5,179,0,0,1969,1973,5,51,0,0,1970,1973,5,97,
	0,0,1971,1973,3,154,77,0,1972,1968,1,0,0,0,1972,1969,1,0,0,0,1972,1970,
	1,0,0,0,1972,1971,1,0,0,0,1973,141,1,0,0,0,1974,1979,3,154,77,0,1975,1976,
	5,1,0,0,1976,1978,3,154,77,0,1977,1975,1,0,0,0,1978,1981,1,0,0,0,1979,1977,
	1,0,0,0,1979,1980,1,0,0,0,1980,143,1,0,0,0,1981,1979,1,0,0,0,1982,1983,
	5,76,0,0,1983,1984,7,22,0,0,1984,1985,3,146,73,0,1985,1986,3,90,45,0,1986,
	145,1,0,0,0,1987,1988,5,18,0,0,1988,1991,5,135,0,0,1989,1991,5,21,0,0,1990,
	1987,1,0,0,0,1990,1989,1,0,0,0,1991,147,1,0,0,0,1992,1996,5,45,0,0,1993,
	1996,5,42,0,0,1994,1996,3,150,75,0,1995,1992,1,0,0,0,1995,1993,1,0,0,0,
	1995,1994,1,0,0,0,1996,149,1,0,0,0,1997,1998,5,215,0,0,1998,2003,3,154,
	77,0,1999,2000,5,169,0,0,2000,2003,3,154,77,0,2001,2003,3,154,77,0,2002,
	1997,1,0,0,0,2002,1999,1,0,0,0,2002,2001,1,0,0,0,2003,151,1,0,0,0,2004,
	2009,3,154,77,0,2005,2006,5,4,0,0,2006,2008,3,154,77,0,2007,2005,1,0,0,
	0,2008,2011,1,0,0,0,2009,2007,1,0,0,0,2009,2010,1,0,0,0,2010,153,1,0,0,
	0,2011,2009,1,0,0,0,2012,2018,5,247,0,0,2013,2018,5,249,0,0,2014,2018,3,
	176,88,0,2015,2018,5,250,0,0,2016,2018,5,248,0,0,2017,2012,1,0,0,0,2017,
	2013,1,0,0,0,2017,2014,1,0,0,0,2017,2015,1,0,0,0,2017,2016,1,0,0,0,2018,
	155,1,0,0,0,2019,2023,5,245,0,0,2020,2023,5,246,0,0,2021,2023,5,244,0,0,
	2022,2019,1,0,0,0,2022,2020,1,0,0,0,2022,2021,1,0,0,0,2023,157,1,0,0,0,
	2024,2027,3,160,80,0,2025,2027,3,162,81,0,2026,2024,1,0,0,0,2026,2025,1,
	0,0,0,2027,159,1,0,0,0,2028,2029,5,36,0,0,2029,2030,3,154,77,0,2030,2031,
	3,162,81,0,2031,161,1,0,0,0,2032,2033,3,164,82,0,2033,2035,3,80,40,0,2034,
	2036,3,166,83,0,2035,2034,1,0,0,0,2035,2036,1,0,0,0,2036,163,1,0,0,0,2037,
	2041,5,211,0,0,2038,2039,5,151,0,0,2039,2041,5,107,0,0,2040,2037,1,0,0,
	0,2040,2038,1,0,0,0,2041,165,1,0,0,0,2042,2044,3,168,84,0,2043,2042,1,0,
	0,0,2044,2047,1,0,0,0,2045,2043,1,0,0,0,2045,2046,1,0,0,0,2046,167,1,0,
	0,0,2047,2045,1,0,0,0,2048,2052,3,172,86,0,2049,2052,3,170,85,0,2050,2052,
	3,174,87,0,2051,2048,1,0,0,0,2051,2049,1,0,0,0,2051,2050,1,0,0,0,2052,169,
	1,0,0,0,2053,2057,5,158,0,0,2054,2055,5,131,0,0,2055,2057,5,158,0,0,2056,
	2053,1,0,0,0,2056,2054,1,0,0,0,2057,171,1,0,0,0,2058,2059,7,23,0,0,2059,
	173,1,0,0,0,2060,2064,5,62,0,0,2061,2062,5,131,0,0,2062,2064,5,62,0,0,2063,
	2060,1,0,0,0,2063,2061,1,0,0,0,2064,175,1,0,0,0,2065,2066,7,24,0,0,2066,
	177,1,0,0,0,262,200,205,211,215,229,233,237,241,249,253,256,263,272,278,
	282,288,295,304,313,324,331,341,348,356,364,372,382,389,397,402,413,418,
	429,440,452,458,463,469,478,489,498,503,507,515,522,535,538,548,551,558,
	567,573,578,582,592,595,605,618,624,629,635,644,650,657,665,670,674,682,
	688,695,700,704,714,717,721,724,732,737,762,768,774,776,782,788,790,798,
	800,819,824,831,843,845,853,855,873,876,880,884,902,905,921,926,928,931,
	937,944,950,956,960,964,970,978,993,1000,1005,1012,1020,1024,1029,1040,
	1052,1055,1060,1062,1071,1073,1081,1087,1090,1092,1104,1111,1115,1119,1123,
	1130,1139,1142,1146,1151,1155,1158,1165,1176,1179,1189,1192,1203,1208,1216,
	1219,1223,1227,1238,1241,1248,1267,1271,1275,1279,1283,1287,1289,1300,1305,
	1314,1320,1324,1326,1334,1341,1354,1360,1371,1378,1382,1390,1392,1405,1413,
	1422,1428,1436,1442,1446,1451,1456,1462,1476,1478,1507,1518,1528,1531,1536,
	1543,1546,1555,1558,1562,1565,1568,1580,1583,1602,1606,1614,1618,1643,1646,
	1655,1661,1667,1673,1683,1692,1714,1717,1720,1730,1732,1739,1741,1747,1755,
	1765,1771,1783,1786,1813,1825,1830,1837,1843,1848,1854,1876,1879,1888,1891,
	1894,1922,1933,1943,1950,1959,1966,1972,1979,1990,1995,2002,2009,2017,2022,
	2026,2035,2040,2045,2051,2056,2063];

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
}
export class StatementDefaultContext extends StatementContext {
	constructor(parser: SqlBaseParser, ctx: StatementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
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
}
export class QueryPrimaryDefaultContext extends QueryPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: QueryPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public querySpecification(): QuerySpecificationContext {
		return this.getTypedRuleContext(QuerySpecificationContext, 0) as QuerySpecificationContext;
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
}
export class SingleGroupingSetContext extends GroupingElementContext {
	constructor(parser: SqlBaseParser, ctx: GroupingElementContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public groupingSet(): GroupingSetContext {
		return this.getTypedRuleContext(GroupingSetContext, 0) as GroupingSetContext;
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
}
export class ParenthesizedRelationContext extends RelationPrimaryContext {
	constructor(parser: SqlBaseParser, ctx: RelationPrimaryContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public relation(): RelationContext {
		return this.getTypedRuleContext(RelationContext, 0) as RelationContext;
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
}
export class ParenthesizedExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public expression(): ExpressionContext {
		return this.getTypedRuleContext(ExpressionContext, 0) as ExpressionContext;
	}
}
export class ParameterContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
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
}
export class IntervalLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public interval(): IntervalContext {
		return this.getTypedRuleContext(IntervalContext, 0) as IntervalContext;
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
}
export class BooleanLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public booleanValue(): BooleanValueContext {
		return this.getTypedRuleContext(BooleanValueContext, 0) as BooleanValueContext;
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
}
export class ColumnReferenceContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public identifier(): IdentifierContext {
		return this.getTypedRuleContext(IdentifierContext, 0) as IdentifierContext;
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
}
export class SubqueryExpressionContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public query(): QueryContext {
		return this.getTypedRuleContext(QueryContext, 0) as QueryContext;
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
}
export class StringLiteralContext extends PrimaryExpressionContext {
	constructor(parser: SqlBaseParser, ctx: PrimaryExpressionContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public string_(): StringContext {
		return this.getTypedRuleContext(StringContext, 0) as StringContext;
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
}
export class BasicStringLiteralContext extends StringContext {
	constructor(parser: SqlBaseParser, ctx: StringContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public STRING(): TerminalNode {
		return this.getToken(SqlBaseParser.STRING, 0);
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
}
export class SerializableContext extends LevelOfIsolationContext {
	constructor(parser: SqlBaseParser, ctx: LevelOfIsolationContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public SERIALIZABLE(): TerminalNode {
		return this.getToken(SqlBaseParser.SERIALIZABLE, 0);
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
}
export class SpecifiedPrincipalContext extends GrantorContext {
	constructor(parser: SqlBaseParser, ctx: GrantorContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public principal(): PrincipalContext {
		return this.getTypedRuleContext(PrincipalContext, 0) as PrincipalContext;
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
}
export class QuotedIdentifierContext extends IdentifierContext {
	constructor(parser: SqlBaseParser, ctx: IdentifierContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public QUOTED_IDENTIFIER(): TerminalNode {
		return this.getToken(SqlBaseParser.QUOTED_IDENTIFIER, 0);
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
}
export class DoubleLiteralContext extends NumberContext {
	constructor(parser: SqlBaseParser, ctx: NumberContext) {
		super(parser, ctx.parentCtx, ctx.invokingState);
		super.copyFrom(ctx);
	}
	public DOUBLE_VALUE(): TerminalNode {
		return this.getToken(SqlBaseParser.DOUBLE_VALUE, 0);
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
}
