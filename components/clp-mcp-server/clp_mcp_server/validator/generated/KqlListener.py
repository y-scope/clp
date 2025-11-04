# Generated from /home/huangshi/clp/components/core/src/clp_s/search/kql/Kql.g4 by ANTLR 4.13.2
from antlr4 import *

if "." in __name__:
    from .KqlParser import KqlParser
else:
    from KqlParser import KqlParser

# This class defines a complete listener for a parse tree produced by KqlParser.
class KqlListener(ParseTreeListener):

    # Enter a parse tree produced by KqlParser#start.
    def enterStart(self, ctx:KqlParser.StartContext):
        pass

    # Exit a parse tree produced by KqlParser#start.
    def exitStart(self, ctx:KqlParser.StartContext):
        pass


    # Enter a parse tree produced by KqlParser#Expr.
    def enterExpr(self, ctx:KqlParser.ExprContext):
        pass

    # Exit a parse tree produced by KqlParser#Expr.
    def exitExpr(self, ctx:KqlParser.ExprContext):
        pass


    # Enter a parse tree produced by KqlParser#NestedQuery.
    def enterNestedQuery(self, ctx:KqlParser.NestedQueryContext):
        pass

    # Exit a parse tree produced by KqlParser#NestedQuery.
    def exitNestedQuery(self, ctx:KqlParser.NestedQueryContext):
        pass


    # Enter a parse tree produced by KqlParser#NotQuery.
    def enterNotQuery(self, ctx:KqlParser.NotQueryContext):
        pass

    # Exit a parse tree produced by KqlParser#NotQuery.
    def exitNotQuery(self, ctx:KqlParser.NotQueryContext):
        pass


    # Enter a parse tree produced by KqlParser#SubQuery.
    def enterSubQuery(self, ctx:KqlParser.SubQueryContext):
        pass

    # Exit a parse tree produced by KqlParser#SubQuery.
    def exitSubQuery(self, ctx:KqlParser.SubQueryContext):
        pass


    # Enter a parse tree produced by KqlParser#OrAndQuery.
    def enterOrAndQuery(self, ctx:KqlParser.OrAndQueryContext):
        pass

    # Exit a parse tree produced by KqlParser#OrAndQuery.
    def exitOrAndQuery(self, ctx:KqlParser.OrAndQueryContext):
        pass


    # Enter a parse tree produced by KqlParser#expression.
    def enterExpression(self, ctx:KqlParser.ExpressionContext):
        pass

    # Exit a parse tree produced by KqlParser#expression.
    def exitExpression(self, ctx:KqlParser.ExpressionContext):
        pass


    # Enter a parse tree produced by KqlParser#column_range_expression.
    def enterColumn_range_expression(self, ctx:KqlParser.Column_range_expressionContext):
        pass

    # Exit a parse tree produced by KqlParser#column_range_expression.
    def exitColumn_range_expression(self, ctx:KqlParser.Column_range_expressionContext):
        pass


    # Enter a parse tree produced by KqlParser#column_value_expression.
    def enterColumn_value_expression(self, ctx:KqlParser.Column_value_expressionContext):
        pass

    # Exit a parse tree produced by KqlParser#column_value_expression.
    def exitColumn_value_expression(self, ctx:KqlParser.Column_value_expressionContext):
        pass


    # Enter a parse tree produced by KqlParser#column.
    def enterColumn(self, ctx:KqlParser.ColumnContext):
        pass

    # Exit a parse tree produced by KqlParser#column.
    def exitColumn(self, ctx:KqlParser.ColumnContext):
        pass


    # Enter a parse tree produced by KqlParser#value_expression.
    def enterValue_expression(self, ctx:KqlParser.Value_expressionContext):
        pass

    # Exit a parse tree produced by KqlParser#value_expression.
    def exitValue_expression(self, ctx:KqlParser.Value_expressionContext):
        pass


    # Enter a parse tree produced by KqlParser#list_of_values.
    def enterList_of_values(self, ctx:KqlParser.List_of_valuesContext):
        pass

    # Exit a parse tree produced by KqlParser#list_of_values.
    def exitList_of_values(self, ctx:KqlParser.List_of_valuesContext):
        pass



del KqlParser
