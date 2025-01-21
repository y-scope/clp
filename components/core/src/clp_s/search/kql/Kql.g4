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
    : col=column RANGE_OPERATOR ( date_lit=DATE_LITERAL | lit=LITERAL )
    ;

column_value_expression
    : col=column ':' ( list=list_of_values | date_lit=DATE_LITERAL | lit=LITERAL )
    ;

column: 
    LITERAL
    ;

value_expression
    : LITERAL
    ;

list_of_values
    : '(' condition=(AND | OR | NOT)? (literals+=LITERAL)* ')'
    ;

AND:        [Aa] [Nn] [Dd] ;
OR:         [Oo] [Rr] ;
NOT:        [Nn] [Oo] [Tt] ;

DATE_LITERAL: 'date(' (('"' QUOTED_CHARACTER+ '"') | QUOTED_CHARACTER+) ')' ;

LITERAL: QUOTED_STRING | UNQUOTED_LITERAL ;

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

fragment KEYWORD
    : AND
    | OR
    | NOT
    ;

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
    : [\\():<>"*?{}.]
    ;

// For unicode hex
fragment UNICODE: '\\u' HEXDIGIT HEXDIGIT HEXDIGIT HEXDIGIT ;
fragment HEXDIGIT: [0-9a-fA-F] ;

SPACE:  [ \t\r\n] -> skip ;
