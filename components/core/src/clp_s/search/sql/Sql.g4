// Boilerplate for work in progress SQL grammar.
grammar Sql;

start: EOF ;

SPACE:  [ \t\r\n] -> skip ;
