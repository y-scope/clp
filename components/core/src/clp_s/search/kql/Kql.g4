grammar Kql;

start: query EOF ;

query
    : col=column ':' '{' q=query '}'     #NestedQuery
    | '(' q=query ')'                    #SubQuery
    | NOT q=query                        #NotQuery
    | lhs=query op=(OR | AND) rhs=query  #OrAndQuery
    | expression                         #Expr
    ;

expression
    : column_range_expression 
    | column_value_expression
    | value_expression
    ;

column_range_expression
    : col=column RANGE_OPERATOR ( timestamp=timestamp_expression | lit=literal )
    ;

column_value_expression
    : col=column ':' ( list=list_of_values | timestamp=timestamp_expression | lit=literal )
    ;

column: 
    literal
    ;

value_expression
    : literal
    ;

list_of_values
    : '(' condition=(AND | OR | NOT)? (literals+=literal)* ')'
    ;

timestamp_expression
    : 'timestamp(' timestamp=QUOTED_STRING (',' pattern=QUOTED_STRING )? ')';

literal: QUOTED_STRING | UNQUOTED_LITERAL ;

AND:        [Aa] [Nn] [Dd] ;
OR:         [Oo] [Rr] ;
NOT:        [Nn] [Oo] [Tt] ;

QUOTED_STRING: '"' QUOTED_CHARACTER* '"' ;

UNQUOTED_LITERAL: UNQUOTED_CHARACTER+ ;

fragment QUOTED_CHARACTER
    : ESCAPED_SPACE
    | '\\"'
    | ~'"'
    ;

fragment UNQUOTED_CHARACTER
    : ESCAPED_SPACE
    | ESCAPED_SPECIAL_CHARACTER
    | WILDCARD
    | UNICODE
    |  ~[\\():<>"{} \r\n\t]
    ;

fragment WILDCARD:   '*' | '?';

RANGE_OPERATOR
    : '<='
    | '>='
    | '<'
    | '>'
    ;

fragment ESCAPED_SPECIAL_CHARACTER
    : '\\' SPECIAL_CHARACTER
    ;

fragment ESCAPED_SPACE
    : '\\t'
    | '\\r'
    | '\\n'
    ;

fragment SPECIAL_CHARACTER
    : [\\():<>"*?{}.@$!#]
    ;

// For unicode hex
fragment UNICODE: '\\u' HEXDIGIT HEXDIGIT HEXDIGIT HEXDIGIT ;
fragment HEXDIGIT: [0-9a-fA-F] ;

SPACE:  [ \t\r\n] -> skip ;
