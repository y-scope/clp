# Generated from /home/huangshi/clp/components/core/src/clp_s/search/kql/Kql.g4 by ANTLR 4.13.2
import sys

from antlr4 import *

if sys.version_info[1] > 5:
	from typing import TextIO
else:
	from typing.io import TextIO

def serializedATN():
    return [
        4,1,14,78,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
        6,2,7,7,7,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,3,1,34,8,1,1,1,1,1,1,1,5,1,39,8,1,10,1,12,1,42,9,1,
        1,2,1,2,1,2,3,2,47,8,2,1,3,1,3,1,3,1,3,3,3,53,8,3,1,4,1,4,1,4,1,
        4,1,4,3,4,60,8,4,1,5,1,5,1,6,1,6,1,7,1,7,3,7,68,8,7,1,7,5,7,71,8,
        7,10,7,12,7,74,9,7,1,7,1,7,1,7,0,1,2,8,0,2,4,6,8,10,12,14,0,2,1,
        0,6,7,1,0,6,8,80,0,16,1,0,0,0,2,33,1,0,0,0,4,46,1,0,0,0,6,48,1,0,
        0,0,8,54,1,0,0,0,10,61,1,0,0,0,12,63,1,0,0,0,14,65,1,0,0,0,16,17,
        3,2,1,0,17,18,5,0,0,1,18,1,1,0,0,0,19,20,6,1,-1,0,20,21,3,10,5,0,
        21,22,5,1,0,0,22,23,5,2,0,0,23,24,3,2,1,0,24,25,5,3,0,0,25,34,1,
        0,0,0,26,27,5,4,0,0,27,28,3,2,1,0,28,29,5,5,0,0,29,34,1,0,0,0,30,
        31,5,8,0,0,31,34,3,2,1,3,32,34,3,4,2,0,33,19,1,0,0,0,33,26,1,0,0,
        0,33,30,1,0,0,0,33,32,1,0,0,0,34,40,1,0,0,0,35,36,10,2,0,0,36,37,
        7,0,0,0,37,39,3,2,1,3,38,35,1,0,0,0,39,42,1,0,0,0,40,38,1,0,0,0,
        40,41,1,0,0,0,41,3,1,0,0,0,42,40,1,0,0,0,43,47,3,6,3,0,44,47,3,8,
        4,0,45,47,3,12,6,0,46,43,1,0,0,0,46,44,1,0,0,0,46,45,1,0,0,0,47,
        5,1,0,0,0,48,49,3,10,5,0,49,52,5,13,0,0,50,53,5,9,0,0,51,53,5,10,
        0,0,52,50,1,0,0,0,52,51,1,0,0,0,53,7,1,0,0,0,54,55,3,10,5,0,55,59,
        5,1,0,0,56,60,3,14,7,0,57,60,5,9,0,0,58,60,5,10,0,0,59,56,1,0,0,
        0,59,57,1,0,0,0,59,58,1,0,0,0,60,9,1,0,0,0,61,62,5,10,0,0,62,11,
        1,0,0,0,63,64,5,10,0,0,64,13,1,0,0,0,65,67,5,4,0,0,66,68,7,1,0,0,
        67,66,1,0,0,0,67,68,1,0,0,0,68,72,1,0,0,0,69,71,5,10,0,0,70,69,1,
        0,0,0,71,74,1,0,0,0,72,70,1,0,0,0,72,73,1,0,0,0,73,75,1,0,0,0,74,
        72,1,0,0,0,75,76,5,5,0,0,76,15,1,0,0,0,7,33,40,46,52,59,67,72
    ]

class KqlParser ( Parser ):

    grammarFileName = "Kql.g4"

    atn = ATNDeserializer().deserialize(serializedATN())

    decisionsToDFA = [ DFA(ds, i) for i, ds in enumerate(atn.decisionToState) ]

    sharedContextCache = PredictionContextCache()

    literalNames = [ "<INVALID>", "':'", "'{'", "'}'", "'('", "')'" ]

    symbolicNames = [ "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>",
                      "<INVALID>", "<INVALID>", "AND", "OR", "NOT", "DATE_LITERAL",
                      "LITERAL", "QUOTED_STRING", "UNQUOTED_LITERAL", "RANGE_OPERATOR",
                      "SPACE" ]

    RULE_start = 0
    RULE_query = 1
    RULE_expression = 2
    RULE_column_range_expression = 3
    RULE_column_value_expression = 4
    RULE_column = 5
    RULE_value_expression = 6
    RULE_list_of_values = 7

    ruleNames =  [ "start", "query", "expression", "column_range_expression",
                   "column_value_expression", "column", "value_expression",
                   "list_of_values" ]

    EOF = Token.EOF
    T__0=1
    T__1=2
    T__2=3
    T__3=4
    T__4=5
    AND=6
    OR=7
    NOT=8
    DATE_LITERAL=9
    LITERAL=10
    QUOTED_STRING=11
    UNQUOTED_LITERAL=12
    RANGE_OPERATOR=13
    SPACE=14

    def __init__(self, input:TokenStream, output:TextIO = sys.stdout):
        super().__init__(input, output)
        self.checkVersion("4.13.2")
        self._interp = ParserATNSimulator(self, self.atn, self.decisionsToDFA, self.sharedContextCache)
        self._predicates = None




    class StartContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def query(self):
            return self.getTypedRuleContext(KqlParser.QueryContext,0)


        def EOF(self):
            return self.getToken(KqlParser.EOF, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_start

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterStart" ):
                listener.enterStart(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitStart" ):
                listener.exitStart(self)




    def start(self):

        localctx = KqlParser.StartContext(self, self._ctx, self.state)
        self.enterRule(localctx, 0, self.RULE_start)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 16
            self.query(0)
            self.state = 17
            self.match(KqlParser.EOF)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class QueryContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser


        def getRuleIndex(self):
            return KqlParser.RULE_query


        def copyFrom(self, ctx:ParserRuleContext):
            super().copyFrom(ctx)


    class ExprContext(QueryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a KqlParser.QueryContext
            super().__init__(parser)
            self.copyFrom(ctx)

        def expression(self):
            return self.getTypedRuleContext(KqlParser.ExpressionContext,0)


        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterExpr" ):
                listener.enterExpr(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitExpr" ):
                listener.exitExpr(self)


    class NestedQueryContext(QueryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a KqlParser.QueryContext
            super().__init__(parser)
            self.col = None # ColumnContext
            self.q = None # QueryContext
            self.copyFrom(ctx)

        def column(self):
            return self.getTypedRuleContext(KqlParser.ColumnContext,0)

        def query(self):
            return self.getTypedRuleContext(KqlParser.QueryContext,0)


        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterNestedQuery" ):
                listener.enterNestedQuery(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitNestedQuery" ):
                listener.exitNestedQuery(self)


    class NotQueryContext(QueryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a KqlParser.QueryContext
            super().__init__(parser)
            self.q = None # QueryContext
            self.copyFrom(ctx)

        def NOT(self):
            return self.getToken(KqlParser.NOT, 0)
        def query(self):
            return self.getTypedRuleContext(KqlParser.QueryContext,0)


        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterNotQuery" ):
                listener.enterNotQuery(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitNotQuery" ):
                listener.exitNotQuery(self)


    class SubQueryContext(QueryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a KqlParser.QueryContext
            super().__init__(parser)
            self.q = None # QueryContext
            self.copyFrom(ctx)

        def query(self):
            return self.getTypedRuleContext(KqlParser.QueryContext,0)


        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterSubQuery" ):
                listener.enterSubQuery(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitSubQuery" ):
                listener.exitSubQuery(self)


    class OrAndQueryContext(QueryContext):

        def __init__(self, parser, ctx:ParserRuleContext): # actually a KqlParser.QueryContext
            super().__init__(parser)
            self.lhs = None # QueryContext
            self.op = None # Token
            self.rhs = None # QueryContext
            self.copyFrom(ctx)

        def query(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(KqlParser.QueryContext)
            return self.getTypedRuleContext(KqlParser.QueryContext,i)

        def OR(self):
            return self.getToken(KqlParser.OR, 0)
        def AND(self):
            return self.getToken(KqlParser.AND, 0)

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterOrAndQuery" ):
                listener.enterOrAndQuery(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitOrAndQuery" ):
                listener.exitOrAndQuery(self)



    def query(self, _p:int=0):
        _parentctx = self._ctx
        _parentState = self.state
        localctx = KqlParser.QueryContext(self, self._ctx, _parentState)
        _prevctx = localctx
        _startState = 2
        self.enterRecursionRule(localctx, 2, self.RULE_query, _p)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 33
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,0,self._ctx)
            if la_ == 1:
                localctx = KqlParser.NestedQueryContext(self, localctx)
                self._ctx = localctx
                _prevctx = localctx

                self.state = 20
                localctx.col = self.column()
                self.state = 21
                self.match(KqlParser.T__0)
                self.state = 22
                self.match(KqlParser.T__1)
                self.state = 23
                localctx.q = self.query(0)
                self.state = 24
                self.match(KqlParser.T__2)

            elif la_ == 2:
                localctx = KqlParser.SubQueryContext(self, localctx)
                self._ctx = localctx
                _prevctx = localctx
                self.state = 26
                self.match(KqlParser.T__3)
                self.state = 27
                localctx.q = self.query(0)
                self.state = 28
                self.match(KqlParser.T__4)

            elif la_ == 3:
                localctx = KqlParser.NotQueryContext(self, localctx)
                self._ctx = localctx
                _prevctx = localctx
                self.state = 30
                self.match(KqlParser.NOT)
                self.state = 31
                localctx.q = self.query(3)

            elif la_ == 4:
                localctx = KqlParser.ExprContext(self, localctx)
                self._ctx = localctx
                _prevctx = localctx
                self.state = 32
                self.expression()


            self._ctx.stop = self._input.LT(-1)
            self.state = 40
            self._errHandler.sync(self)
            _alt = self._interp.adaptivePredict(self._input,1,self._ctx)
            while _alt!=2 and _alt!=ATN.INVALID_ALT_NUMBER:
                if _alt==1:
                    if self._parseListeners is not None:
                        self.triggerExitRuleEvent()
                    _prevctx = localctx
                    localctx = KqlParser.OrAndQueryContext(self, KqlParser.QueryContext(self, _parentctx, _parentState))
                    localctx.lhs = _prevctx
                    self.pushNewRecursionContext(localctx, _startState, self.RULE_query)
                    self.state = 35
                    if not self.precpred(self._ctx, 2):
                        from antlr4.error.Errors import FailedPredicateException
                        raise FailedPredicateException(self, "self.precpred(self._ctx, 2)")
                    self.state = 36
                    localctx.op = self._input.LT(1)
                    _la = self._input.LA(1)
                    if not(_la==6 or _la==7):
                        localctx.op = self._errHandler.recoverInline(self)
                    else:
                        self._errHandler.reportMatch(self)
                        self.consume()
                    self.state = 37
                    localctx.rhs = self.query(3)
                self.state = 42
                self._errHandler.sync(self)
                _alt = self._interp.adaptivePredict(self._input,1,self._ctx)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.unrollRecursionContexts(_parentctx)
        return localctx


    class ExpressionContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def column_range_expression(self):
            return self.getTypedRuleContext(KqlParser.Column_range_expressionContext,0)


        def column_value_expression(self):
            return self.getTypedRuleContext(KqlParser.Column_value_expressionContext,0)


        def value_expression(self):
            return self.getTypedRuleContext(KqlParser.Value_expressionContext,0)


        def getRuleIndex(self):
            return KqlParser.RULE_expression

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterExpression" ):
                listener.enterExpression(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitExpression" ):
                listener.exitExpression(self)




    def expression(self):

        localctx = KqlParser.ExpressionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 4, self.RULE_expression)
        try:
            self.state = 46
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,2,self._ctx)
            if la_ == 1:
                self.enterOuterAlt(localctx, 1)
                self.state = 43
                self.column_range_expression()

            elif la_ == 2:
                self.enterOuterAlt(localctx, 2)
                self.state = 44
                self.column_value_expression()

            elif la_ == 3:
                self.enterOuterAlt(localctx, 3)
                self.state = 45
                self.value_expression()


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Column_range_expressionContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.col = None # ColumnContext
            self.date_lit = None # Token
            self.lit = None # Token

        def RANGE_OPERATOR(self):
            return self.getToken(KqlParser.RANGE_OPERATOR, 0)

        def column(self):
            return self.getTypedRuleContext(KqlParser.ColumnContext,0)


        def DATE_LITERAL(self):
            return self.getToken(KqlParser.DATE_LITERAL, 0)

        def LITERAL(self):
            return self.getToken(KqlParser.LITERAL, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_column_range_expression

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterColumn_range_expression" ):
                listener.enterColumn_range_expression(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitColumn_range_expression" ):
                listener.exitColumn_range_expression(self)




    def column_range_expression(self):

        localctx = KqlParser.Column_range_expressionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 6, self.RULE_column_range_expression)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 48
            localctx.col = self.column()
            self.state = 49
            self.match(KqlParser.RANGE_OPERATOR)
            self.state = 52
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [9]:
                self.state = 50
                localctx.date_lit = self.match(KqlParser.DATE_LITERAL)
            elif token in [10]:
                self.state = 51
                localctx.lit = self.match(KqlParser.LITERAL)
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Column_value_expressionContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.col = None # ColumnContext
            self.list_ = None # List_of_valuesContext
            self.date_lit = None # Token
            self.lit = None # Token

        def column(self):
            return self.getTypedRuleContext(KqlParser.ColumnContext,0)


        def list_of_values(self):
            return self.getTypedRuleContext(KqlParser.List_of_valuesContext,0)


        def DATE_LITERAL(self):
            return self.getToken(KqlParser.DATE_LITERAL, 0)

        def LITERAL(self):
            return self.getToken(KqlParser.LITERAL, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_column_value_expression

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterColumn_value_expression" ):
                listener.enterColumn_value_expression(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitColumn_value_expression" ):
                listener.exitColumn_value_expression(self)




    def column_value_expression(self):

        localctx = KqlParser.Column_value_expressionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 8, self.RULE_column_value_expression)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 54
            localctx.col = self.column()
            self.state = 55
            self.match(KqlParser.T__0)
            self.state = 59
            self._errHandler.sync(self)
            token = self._input.LA(1)
            if token in [4]:
                self.state = 56
                localctx.list_ = self.list_of_values()
            elif token in [9]:
                self.state = 57
                localctx.date_lit = self.match(KqlParser.DATE_LITERAL)
            elif token in [10]:
                self.state = 58
                localctx.lit = self.match(KqlParser.LITERAL)
            else:
                raise NoViableAltException(self)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ColumnContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def LITERAL(self):
            return self.getToken(KqlParser.LITERAL, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_column

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterColumn" ):
                listener.enterColumn(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitColumn" ):
                listener.exitColumn(self)




    def column(self):

        localctx = KqlParser.ColumnContext(self, self._ctx, self.state)
        self.enterRule(localctx, 10, self.RULE_column)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 61
            self.match(KqlParser.LITERAL)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class Value_expressionContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def LITERAL(self):
            return self.getToken(KqlParser.LITERAL, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_value_expression

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterValue_expression" ):
                listener.enterValue_expression(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitValue_expression" ):
                listener.exitValue_expression(self)




    def value_expression(self):

        localctx = KqlParser.Value_expressionContext(self, self._ctx, self.state)
        self.enterRule(localctx, 12, self.RULE_value_expression)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 63
            self.match(KqlParser.LITERAL)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class List_of_valuesContext(ParserRuleContext):
        __slots__ = "parser"

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser
            self.condition = None # Token
            self._LITERAL = None # Token
            self.literals = list() # of Tokens

        def LITERAL(self, i:int=None):
            if i is None:
                return self.getTokens(KqlParser.LITERAL)
            return self.getToken(KqlParser.LITERAL, i)

        def AND(self):
            return self.getToken(KqlParser.AND, 0)

        def OR(self):
            return self.getToken(KqlParser.OR, 0)

        def NOT(self):
            return self.getToken(KqlParser.NOT, 0)

        def getRuleIndex(self):
            return KqlParser.RULE_list_of_values

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterList_of_values" ):
                listener.enterList_of_values(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitList_of_values" ):
                listener.exitList_of_values(self)




    def list_of_values(self):

        localctx = KqlParser.List_of_valuesContext(self, self._ctx, self.state)
        self.enterRule(localctx, 14, self.RULE_list_of_values)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 65
            self.match(KqlParser.T__3)
            self.state = 67
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            if (((_la) & ~0x3f) == 0 and ((1 << _la) & 448) != 0):
                self.state = 66
                localctx.condition = self._input.LT(1)
                _la = self._input.LA(1)
                if not(((_la) & ~0x3f) == 0 and ((1 << _la) & 448) != 0):
                    localctx.condition = self._errHandler.recoverInline(self)
                else:
                    self._errHandler.reportMatch(self)
                    self.consume()


            self.state = 72
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==10:
                self.state = 69
                localctx._LITERAL = self.match(KqlParser.LITERAL)
                localctx.literals.append(localctx._LITERAL)
                self.state = 74
                self._errHandler.sync(self)
                _la = self._input.LA(1)

            self.state = 75
            self.match(KqlParser.T__4)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx



    def sempred(self, localctx:RuleContext, ruleIndex:int, predIndex:int):
        if self._predicates == None:
            self._predicates = dict()
        self._predicates[1] = self.query_sempred
        pred = self._predicates.get(ruleIndex, None)
        if pred is None:
            raise Exception("No predicate with index:" + str(ruleIndex))
        return pred(localctx, predIndex)

    def query_sempred(self, localctx:QueryContext, predIndex:int):
            if predIndex == 0:
                return self.precpred(self._ctx, 2)
