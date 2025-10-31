grammar Sql;

import SqlBase;

selectItemList
    : selectItem (',' selectItem)*
    ;

standaloneSelectItemList
    : selectItemList EOF
    ;

standaloneBooleanExpression
    : booleanExpression EOF
    ;

sortItemList
    : sortItem (',' sortItem)*
    ;

standaloneSortItemList
    : sortItemList EOF
    ;
