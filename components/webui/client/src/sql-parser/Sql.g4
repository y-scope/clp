grammar Sql;

import SqlBase;


standaloneSelectItemList
    : selectItemList EOF
    ;


standaloneBooleanExpression
    : booleanExpression EOF
    ;

standaloneSortItemList
    : sortItemList EOF
    ;
