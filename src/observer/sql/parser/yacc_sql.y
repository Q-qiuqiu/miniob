
/*
 * yacc_sql.y - SQL语法解析器的语法定义文件
 * 使用yacc/bison工具根据此文件生成SQL解析器的C代码
 */

%{
/*
 * 这个块（%{ ... %}）中的C代码会被原样复制到生成的C文件中
 * 主要包含必要的头文件、工具函数和辅助函数定义
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/log/log.h"          // 日志库，用于错误记录
#include "common/lang/string.h"      // 字符串处理工具
#include "sql/parser/parse_defs.h"   // SQL解析相关的数据结构定义
#include "sql/parser/yacc_sql.hpp"   // 由yacc生成的头文件
#include "sql/parser/lex_sql.h"      // 词法分析器的头文件
#include "sql/expr/expression.h"     // 表达式相关类的定义

using namespace std;

/**
 * 从SQL字符串中提取token的实际内容
 * @param sql_string 原始SQL语句字符串
 * @param llocp token的位置信息
 * @return token的实际字符串内容
 */
string token_name(const char *sql_string, YYLTYPE *llocp)
{
  // 根据token的起始和结束列位置，从SQL字符串中提取对应的子串
  return string(sql_string + llocp->first_column, llocp->last_column - llocp->first_column + 1);
}

/**
 * 语法错误处理函数
 * 当解析器遇到语法错误时会调用此函数
 * @param llocp 错误发生的位置信息
 * @param sql_string 原始SQL语句字符串
 * @param sql_result 解析结果，用于存储错误信息
 * @param scanner 词法分析器状态
 * @param msg 错误信息
 * @return 始终返回0，表示错误已处理
 */
int yyerror(YYLTYPE *llocp, const char *sql_string, ParsedSqlResult *sql_result, yyscan_t scanner, const char *msg)
{
  // 创建错误类型的解析节点
  unique_ptr<ParsedSqlNode> error_sql_node = make_unique<ParsedSqlNode>(SCF_ERROR);
  // 设置错误信息和位置
  error_sql_node->error.error_msg = msg;
  error_sql_node->error.line = llocp->first_line;
  error_sql_node->error.column = llocp->first_column;
  // 将错误节点添加到解析结果中
  sql_result->add_sql_node(std::move(error_sql_node));
  return 0;
}

/**
 * 创建算术表达式
 * @param type 算术表达式类型（加、减、乘、除、负号）
 * @param left 左操作数表达式
 * @param right 右操作数表达式（对于负号操作可为nullptr）
 * @param sql_string 原始SQL语句字符串
 * @param llocp 表达式在SQL中的位置信息
 * @return 创建的算术表达式对象
 */
ArithmeticExpr *create_arithmetic_expression(ArithmeticExpr::Type type,
                                             Expression *left,
                                             Expression *right,
                                             const char *sql_string,
                                             YYLTYPE *llocp)
{
  // 创建新的算术表达式对象
  ArithmeticExpr *expr = new ArithmeticExpr(type, left, right);
  // 设置表达式名称（从SQL字符串中提取）
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

/**
 * 创建聚合函数表达式
 * @param aggregate_name 聚合函数名称（如SUM、AVG等）
 * @param child 聚合函数的参数表达式
 * @param sql_string 原始SQL语句字符串
 * @param llocp 表达式在SQL中的位置信息
 * @return 创建的聚合表达式对象
 */
UnboundAggregateExpr *create_aggregate_expression(const char *aggregate_name,
                                           Expression *child,
                                           const char *sql_string,
                                           YYLTYPE *llocp)
{
  // 创建新的聚合表达式对象
  UnboundAggregateExpr *expr = new UnboundAggregateExpr(aggregate_name, child);
  // 设置表达式名称
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

%}

/*
 * yacc/bison 配置选项部分
 * 以下选项控制解析器的行为和生成代码的特性
 */

/* 定义API为纯函数形式，确保解析器是可重入的（线程安全的） */
%define api.pure full
/* 启用详细的错误消息输出，使语法错误信息更加清晰明了 */
%define parse.error verbose
/* 启用位置标识功能，记录语法分析过程中的行号和列号信息 */
%locations
/* 定义词法分析器（lexer）的参数 */
%lex-param { yyscan_t scanner }
/* 这些定义了在yyparse函数中的参数，用于传递上下文信息 */
%parse-param { const char * sql_string }      // 原始SQL语句字符串
%parse-param { ParsedSqlResult * sql_result } // 解析结果存储对象
%parse-param { void * scanner }              // 词法分析器状态

/* 标识词法标记（tokens）定义 */
/* 以下是所有SQL关键字和操作符的词法标记定义 */
%token  SEMICOLON         // 分号
        BY               // GROUP BY等语句中使用
        CREATE           // 创建对象关键字
        DROP             // 删除对象关键字
        GROUP            // GROUP BY子句关键字
        TABLE            // 表关键字
        TABLES           // 表的复数形式
        INDEX            // 索引关键字
        CALC             // 计算语句关键字
        SELECT           // 查询语句关键字
        DESC             // 描述表结构关键字
        SHOW             // 显示信息关键字
        SYNC             // 同步数据关键字
        INSERT           // 插入数据关键字
        DELETE           // 删除数据关键字
        UPDATE           // 更新数据关键字
        LBRACE           // 左花括号
        RBRACE           // 右花括号
        COMMA            // 逗号
        TRX_BEGIN        // 开始事务关键字
        TRX_COMMIT       // 提交事务关键字
        TRX_ROLLBACK     // 回滚事务关键字
        INT_T            // 整数类型
        STRING_T         // 字符串类型
        FLOAT_T          // 浮点数类型
        VECTOR_T         // 向量类型
        HELP             // 帮助命令关键字
        EXIT             // 退出命令关键字
        DOT              // 点号（如table.column）
        INTO             // INSERT INTO关键字
        VALUES           // VALUES关键字
        FROM             // FROM子句关键字
        WHERE            // WHERE子句关键字
        AND              // AND逻辑操作符
        SET              // SET关键字
        ON               // ON关键字（如JOIN ON）
        LOAD             // LOAD DATA关键字
        DATA             // 数据关键字
        INFILE           // 输入文件关键字
        EXPLAIN          // 解释执行计划关键字
        STORAGE          // 存储关键字
        FORMAT           // 格式关键字
        PRIMARY          // 主键关键字
        KEY              // 键关键字
        ANALYZE          // 分析表关键字
        EQ               // 等于操作符(=)
        LT               // 小于操作符(<)
        GT               // 大于操作符(>)
        LE               // 小于等于操作符(<=)
        GE               // 大于等于操作符(>=)
        NE               // 不等于操作符(!=)

/*
 * %union 中定义各种数据类型
 * 真实生成的代码会使用union类型，所以不能包含非POD类型的数据
 * 此联合定义了语法分析过程中可能使用的各种值类型
 */
%union {
  ParsedSqlNode *                            sql_node;          // SQL节点基类指针
  ConditionSqlNode *                         condition;         // 条件节点指针
  Value *                                    value;             // 值对象指针
  enum CompOp                                comp;              // 比较操作符枚举
  RelAttrSqlNode *                           rel_attr;          // 关系属性节点指针
  vector<AttrInfoSqlNode> *                  attr_infos;        // 属性信息列表指针
  AttrInfoSqlNode *                          attr_info;         // 属性信息节点指针
  Expression *                               expression;        // 表达式基类指针
  vector<unique_ptr<Expression>> *           expression_list;   // 表达式列表指针
  vector<Value> *                            value_list;        // 值列表指针
  vector<ConditionSqlNode> *                 condition_list;    // 条件列表指针
  vector<RelAttrSqlNode> *                   rel_attr_list;     // 关系属性列表指针
  vector<string> *                           relation_list;     // 关系名列表指针
  vector<string> *                           key_list;          // 键列表指针
  char *                                     cstring;           // C风格字符串
  int                                        number;            // 整数
  float                                      floats;            // 浮点数
}

/* 终结符的值类型定义 */
%token <number> NUMBER  // 数值类型标记，其值存储为整数
%token <floats> FLOAT   // 浮点数类型标记，其值存储为浮点数
%token <cstring> ID     // 标识符标记，其值存储为字符串
%token <cstring> SSS    // 字符串字面量标记，其值存储为字符串

/* 非终结符定义 */

/*
 * %type 定义了各种解析后的结果输出的是什么类型
 * 类型对应了 union 中定义的成员变量名称
 */
%type <number>              type                // 数据类型
%type <condition>           condition           // 条件表达式
%type <value>               value               // 字面量值
%type <number>              number              // 数字
%type <cstring>             relation            // 关系名
%type <comp>                comp_op             // 比较操作符
%type <rel_attr>            rel_attr            // 关系属性
%type <attr_infos>          attr_def_list       // 属性定义列表
%type <attr_info>           attr_def            // 属性定义
%type <value_list>          value_list          // 值列表
%type <condition_list>      where               // WHERE子句
%type <condition_list>      condition_list      // 条件列表
%type <cstring>             storage_format      // 存储格式
%type <key_list>            primary_key         // 主键定义
%type <key_list>            attr_list           // 属性列表
%type <relation_list>       rel_list            // 关系列表
%type <expression>          expression          // 表达式
%type <expression_list>     expression_list     // 表达式列表
%type <expression_list>     group_by            // GROUP BY子句
%type <sql_node>            calc_stmt           // 计算语句
%type <sql_node>            select_stmt         // 查询语句
%type <sql_node>            insert_stmt         // 插入语句
%type <sql_node>            update_stmt         // 更新语句
%type <sql_node>            delete_stmt         // 删除语句
%type <sql_node>            create_table_stmt   // 创建表语句
%type <sql_node>            drop_table_stmt     // 删除表语句
%type <sql_node>            analyze_table_stmt  // 分析表语句
%type <sql_node>            show_tables_stmt    // 显示表列表语句
%type <sql_node>            desc_table_stmt     // 描述表结构语句
%type <sql_node>            create_index_stmt   // 创建索引语句
%type <sql_node>            drop_index_stmt     // 删除索引语句
%type <sql_node>            sync_stmt           // 同步数据语句
%type <sql_node>            begin_stmt          // 开始事务语句
%type <sql_node>            commit_stmt         // 提交事务语句
%type <sql_node>            rollback_stmt       // 回滚事务语句
%type <sql_node>            load_data_stmt      // 加载数据语句
%type <sql_node>            explain_stmt        // 解释执行计划语句
%type <sql_node>            set_variable_stmt   // 设置变量语句
%type <sql_node>            help_stmt           // 帮助语句
%type <sql_node>            exit_stmt           // 退出语句
%type <sql_node>            command_wrapper     // 命令包装器
%type <sql_node>            commands            // 命令列表（实际使用单个命令）

/*
 * 操作符优先级和结合性定义
 * 定义了算术操作符的优先级和结合方向
 */
%left '+' '-'          // 加法和减法，左结合，优先级相同
%left '*' '/'          // 乘法和除法，左结合，优先级高于加减法
%right UMINUS          // 一元负号操作符，右结合，优先级最高
%%

/*
 * 主要语法规则部分
 * 这部分定义了各种SQL语句的语法结构和解析逻辑
 */

/*
 * 命令列表规则（commands）
 * 定义了整个SQL输入可以是单个命令，这是解析器的入口点
 */
commands: command_wrapper opt_semicolon  //commands or sqls. parser starts here.
  { 
    // 将解析结果封装为智能指针
    unique_ptr<ParsedSqlNode> sql_node = unique_ptr<ParsedSqlNode>($1);
    // 将解析节点添加到结果列表中
    sql_result->add_sql_node(std::move(sql_node));
  }
  ;

/*
 * 命令包装器规则（command_wrapper）
 * 定义了所有支持的SQL语句类型
 */

command_wrapper:
    calc_stmt         // 计算表达式语句
  | select_stmt       // SELECT查询语句
  | insert_stmt       // INSERT插入语句
  | update_stmt       // UPDATE更新语句
  | delete_stmt       // DELETE删除语句
  | create_table_stmt // CREATE TABLE建表语句
  | drop_table_stmt     // DROP TABLE删除表语句
  | analyze_table_stmt  // ANALYZE分析表语句
  | show_tables_stmt    // SHOW TABLES显示表列表语句
  | desc_table_stmt     // DESC/DESCRIBE描述表结构语句
  | create_index_stmt   // CREATE INDEX创建索引语句
  | drop_index_stmt     // DROP INDEX删除索引语句
  | sync_stmt           // SYNC同步数据语句
  | begin_stmt          // BEGIN开始事务语句
  | commit_stmt         // COMMIT提交事务语句
  | rollback_stmt       // ROLLBACK回滚事务语句
  | load_data_stmt      // LOAD DATA加载数据语句
  | explain_stmt        // EXPLAIN解释执行计划语句
  | set_variable_stmt   // SET设置变量语句
  | help_stmt           // HELP帮助语句
  | exit_stmt           // EXIT退出语句
    ;

exit_stmt:      
    EXIT {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      $$ = new ParsedSqlNode(SCF_EXIT);
    };

help_stmt:
    HELP {
      $$ = new ParsedSqlNode(SCF_HELP);
    };

sync_stmt:
    SYNC {
      $$ = new ParsedSqlNode(SCF_SYNC);
    }
    ;

begin_stmt:
    TRX_BEGIN  {
      $$ = new ParsedSqlNode(SCF_BEGIN);
    }
    ;

commit_stmt:
    TRX_COMMIT {
      $$ = new ParsedSqlNode(SCF_COMMIT);
    }
    ;

rollback_stmt:
    TRX_ROLLBACK  {
      $$ = new ParsedSqlNode(SCF_ROLLBACK);
    }
    ;

drop_table_stmt:    /*drop table 语句的语法解析树*/
    DROP TABLE ID {
      $$ = new ParsedSqlNode(SCF_DROP_TABLE);
      $$->drop_table.relation_name = $3;
    };

analyze_table_stmt:  /* analyze table 语法的语法解析树*/
    ANALYZE TABLE ID {
      $$ = new ParsedSqlNode(SCF_ANALYZE_TABLE);
      $$->analyze_table.relation_name = $3;
    }
    ;

show_tables_stmt:
    SHOW TABLES {
      $$ = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
    ;

desc_table_stmt:
    DESC ID  {
      $$ = new ParsedSqlNode(SCF_DESC_TABLE);
      $$->desc_table.relation_name = $2;
    }
    ;

create_index_stmt:    /*create index 语句的语法解析树*/
    CREATE INDEX ID ON ID LBRACE ID RBRACE
    {
      $$ = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = $$->create_index;
      create_index.index_name = $3;
      create_index.relation_name = $5;
      create_index.attribute_name = $7;
    }
    ;

drop_index_stmt:      /*drop index 语句的语法解析树*/
    DROP INDEX ID ON ID
    {
      $$ = new ParsedSqlNode(SCF_DROP_INDEX);
      $$->drop_index.index_name = $3;
      $$->drop_index.relation_name = $5;
    }
    ;
/*
 * CREATE TABLE语句规则
 * 定义创建表的语法结构：CREATE TABLE 表名 (第一个属性定义 后续属性定义列表 主键定义) 存储格式
 * 这里语法规则采用了特殊设计，第一个属性单独列出，其他属性通过列表添加，便于处理顺序问题
 */
create_table_stmt:    /*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list primary_key RBRACE storage_format
    {
      // 创建一个新的解析节点用于存储CREATE TABLE语句信息
      $$ = new ParsedSqlNode(SCF_CREATE_TABLE);
      // 获取创建表语句的具体数据结构（通过引用访问）
      CreateTableSqlNode &create_table = $$->create_table;
      // 设置表名（从第三个token获取，即ID）
      create_table.relation_name = $3;
      //free($3);

      // 获取属性定义列表（从第六个token获取，即attr_def_list）
      vector<AttrInfoSqlNode> *src_attrs = $6;

      // 如果存在后续属性定义，则将它们添加到属性信息列表中
      if (src_attrs != nullptr) {
        create_table.attr_infos.swap(*src_attrs);
        delete src_attrs; // 释放临时变量
      }
      // 添加第一个属性定义（从第五个token获取）
      create_table.attr_infos.emplace_back(*$5);
      // 由于属性是倒序添加的，需要反转列表使其保持原始顺序
      reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
      // 释放第一个属性定义的临时变量
      delete $5;
      
      // 如果指定了主键定义，则设置主键信息
      if ($7 != nullptr) {
        create_table.primary_keys.swap(*$7);
        delete $7; // 释放主键定义的临时变量
      }
      
      // 如果指定了存储格式，则设置存储格式
      if ($9 != nullptr) {
        create_table.storage_format = $9;
      }
    }
    ;
    
attr_def_list:
    /* empty */  // 空属性列表
    {
      $$ = nullptr;  // 空属性列表返回nullptr
    }
    | attr_def_list COMMA attr_def
    {
      if ($1 != nullptr) {
        $$ = $1;
      } else {
        $$ = new vector<AttrInfoSqlNode>;
      }
      $$->insert($$->begin(), *$3);
      delete $3;
    }
    ;
    
/*
 * 属性定义规则（attr_def）
 * 定义表中列的语法结构，支持两种形式：
 * 1. 带长度的属性定义：列名 数据类型(长度)
 * 2. 不带长度的属性定义：列名 数据类型
 */
attr_def:
    /* 带长度的属性定义，如 name STRING(20) */
    ID type LBRACE number RBRACE 
    {
      // 创建新的属性信息节点
      $$ = new AttrInfoSqlNode;
      // 设置属性类型（从第二个token获取，转换为AttrType枚举）
      $$->type = (AttrType)$2;
      // 设置属性名称（从第一个token获取，即ID）
      $$->name = $1;
      // 设置属性长度（从第四个token获取，即number）
      $$->length = $4;
    }
    /* 不带长度的属性定义，如 age INT */
    | ID type
    {
      // 创建新的属性信息节点
      $$ = new AttrInfoSqlNode;
      // 设置属性类型（从第二个token获取，转换为AttrType枚举）
      $$->type = (AttrType)$2;
      // 设置属性名称（从第一个token获取，即ID）
      $$->name = $1;
      // 对于不带长度的数据类型，默认为4字节
      $$->length = 4;
    }
    ;
number:
    NUMBER {$$ = $1;}
    ;
type:
    INT_T      { $$ = static_cast<int>(AttrType::INTS); }
    | STRING_T { $$ = static_cast<int>(AttrType::CHARS); }
    | FLOAT_T  { $$ = static_cast<int>(AttrType::FLOATS); }
    | VECTOR_T { $$ = static_cast<int>(AttrType::VECTORS); }
    ;
primary_key:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA PRIMARY KEY LBRACE attr_list RBRACE
    {
      $$ = $5;
    }
    ;

attr_list:
    ID {
      $$ = new vector<string>();
      $$->push_back($1);
    }
    | ID COMMA attr_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new vector<string>;
      }

      $$->insert($$->begin(), $1);
    }
    ;

insert_stmt:        /*insert   语句的语法解析树*/
    INSERT INTO ID VALUES LBRACE value value_list RBRACE 
    {
      $$ = new ParsedSqlNode(SCF_INSERT);
      $$->insertion.relation_name = $3;
      if ($7 != nullptr) {
        $$->insertion.values.swap(*$7);
        delete $7;
      }
      $$->insertion.values.emplace_back(*$6);
      reverse($$->insertion.values.begin(), $$->insertion.values.end());
      delete $6;
    }
    ;

value_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA value value_list  { 
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new vector<Value>;
      }
      $$->emplace_back(*$2);
      delete $2;
    }
    ;
value:
    NUMBER {
      $$ = new Value((int)$1);
      @$ = @1;
    }
    |FLOAT {
      $$ = new Value((float)$1);
      @$ = @1;
    }
    |SSS {
      char *tmp = common::substr($1,1,strlen($1)-2);
      $$ = new Value(tmp);
      free(tmp);
    }
    ;
storage_format:
    /* empty */
    {
      $$ = nullptr;
    }
    | STORAGE FORMAT EQ ID
    {
      $$ = $4;
    }
    ;
    
delete_stmt:    /*  delete 语句的语法解析树*/
    DELETE FROM ID where 
    {
      $$ = new ParsedSqlNode(SCF_DELETE);
      $$->deletion.relation_name = $3;
      if ($4 != nullptr) {
        $$->deletion.conditions.swap(*$4);
        delete $4;
      }
    }
    ;
update_stmt:      /*  update 语句的语法解析树*/
    UPDATE ID SET ID EQ value where 
    {
      $$ = new ParsedSqlNode(SCF_UPDATE);
      $$->update.relation_name = $2;
      $$->update.attribute_name = $4;
      $$->update.value = *$6;
      if ($7 != nullptr) {
        $$->update.conditions.swap(*$7);
        delete $7;
      }
    }
    ;
/*
 * SELECT语句规则
 * 定义查询语句的完整语法结构：SELECT 表达式列表 FROM 表列表 [WHERE 条件列表] [GROUP BY 分组列表]
 */
select_stmt: 
    SELECT expression_list FROM rel_list where group_by
    {
      // 创建新的解析节点用于存储SELECT语句信息，设置语句类型为SCF_SELECT
      $$ = new ParsedSqlNode(SCF_SELECT);
      // 处理表达式列表（要查询的列或计算表达式）
      if ($2 != nullptr) {
        $$->selection.expressions.swap(*$2); // 使用swap避免拷贝，提高效率
        delete $2; // 释放临时变量
      }

      // 处理FROM子句中的表列表
      if ($4 != nullptr) {
        $$->selection.relations.swap(*$4);
        delete $4; // 释放临时变量
      }

      // 处理WHERE子句中的条件列表（如果存在）
      if ($5 != nullptr) {
        $$->selection.conditions.swap(*$5);
        delete $5; // 释放临时变量
      }

      // 处理GROUP BY子句中的分组列列表（如果存在）
      if ($6 != nullptr) {
        $$->selection.group_by.swap(*$6);
        delete $6; // 释放临时变量
      }
    }
    ;
calc_stmt:
    CALC expression_list
    {
      $$ = new ParsedSqlNode(SCF_CALC);
      $$->calc.expressions.swap(*$2);
      delete $2;
    }
    ;

expression_list:
    expression
    {
      $$ = new vector<unique_ptr<Expression>>;
      $$->emplace_back($1);
    }
    | expression COMMA expression_list
    {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new vector<unique_ptr<Expression>>;
      }
      $$->emplace($$->begin(), $1);
    }
    ;
expression:
    expression '+' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::ADD, $1, $3, sql_string, &@$);
    }
    | expression '-' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::SUB, $1, $3, sql_string, &@$);
    }
    | expression '*' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::MUL, $1, $3, sql_string, &@$);
    }
    | expression '/' expression {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::DIV, $1, $3, sql_string, &@$);
    }
    | LBRACE expression RBRACE {
      $$ = $2;
      $$->set_name(token_name(sql_string, &@$));
    }
    | '-' expression %prec UMINUS {
      $$ = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, $2, nullptr, sql_string, &@$);
    }
    | value {
      $$ = new ValueExpr(*$1);
      $$->set_name(token_name(sql_string, &@$));
      delete $1;
    }
    | rel_attr {
      RelAttrSqlNode *node = $1;
      $$ = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      $$->set_name(token_name(sql_string, &@$));
      delete $1;
    }
    | '*' {
      $$ = new StarExpr();
    }
    // your code here
    ;

rel_attr:
    ID {
      $$ = new RelAttrSqlNode;
      $$->attribute_name = $1;
    }
    | ID DOT ID {
      $$ = new RelAttrSqlNode;
      $$->relation_name  = $1;
      $$->attribute_name = $3;
    }
    ;

relation:
    ID {
      $$ = $1;
    }
    ;
rel_list:
    relation {
      $$ = new vector<string>();
      $$->push_back($1);
    }
    | relation COMMA rel_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new vector<string>;
      }

      $$->insert($$->begin(), $1);
    }
    ;

/*
 * WHERE子句规则
 * 定义查询条件的语法结构：WHERE 条件列表
 * 支持空WHERE子句，表示查询所有记录
 */
where:
    /* empty */
    {
      $$ = nullptr;
    }
    | WHERE condition_list  // WHERE关键字后跟条件列表
      {
        $$ = $2;  // 返回解析得到的条件列表对象
      }
    ;
/*
 * 条件列表规则（condition_list）
 * 定义WHERE子句中条件的组合方式，支持单个条件或多个条件通过AND连接
 */
condition_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | condition {  // 单个条件
      $$ = new vector<ConditionSqlNode>;  // 创建条件列表容器
      $$->emplace_back(*$1);  // 将条件添加到列表中
      delete $1;  // 释放临时条件对象
    }
    | condition AND condition_list {  // 条件列表 AND 新条件
      $$ = $3;  // 使用现有的条件列表
      $$->emplace_back(*$1);  // 将新条件添加到列表末尾
      delete $1;  // 释放新条件的临时对象
    }
    ;
condition:
    rel_attr comp_op value
    {
      $$ = new ConditionSqlNode;
      $$->left_is_attr = 1;
      $$->left_attr = *$1;
      $$->right_is_attr = 0;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op value 
    {
      $$ = new ConditionSqlNode;
      $$->left_is_attr = 0;
      $$->left_value = *$1;
      $$->right_is_attr = 0;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | rel_attr comp_op rel_attr
    {
      $$ = new ConditionSqlNode;
      $$->left_is_attr = 1;
      $$->left_attr = *$1;
      $$->right_is_attr = 1;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op rel_attr
    {
      $$ = new ConditionSqlNode;
      $$->left_is_attr = 0;
      $$->left_value = *$1;
      $$->right_is_attr = 1;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    ;

comp_op:
      EQ { $$ = EQUAL_TO; }
    | LT { $$ = LESS_THAN; }
    | GT { $$ = GREAT_THAN; }
    | LE { $$ = LESS_EQUAL; }
    | GE { $$ = GREAT_EQUAL; }
    | NE { $$ = NOT_EQUAL; }
    ;

// your code here
group_by:
    /* empty */
    {
      $$ = nullptr;
    }
    ;
load_data_stmt:
    LOAD DATA INFILE SSS INTO TABLE ID 
    {
      char *tmp_file_name = common::substr($4, 1, strlen($4) - 2);
      
      $$ = new ParsedSqlNode(SCF_LOAD_DATA);
      $$->load_data.relation_name = $7;
      $$->load_data.file_name = tmp_file_name;
      free(tmp_file_name);
    }
    ;

explain_stmt:
    EXPLAIN command_wrapper
    {
      $$ = new ParsedSqlNode(SCF_EXPLAIN);
      $$->explain.sql_node = unique_ptr<ParsedSqlNode>($2);
    }
    ;

set_variable_stmt:
    SET ID EQ value
    {
      $$ = new ParsedSqlNode(SCF_SET_VARIABLE);
      $$->set_variable.name  = $2;
      $$->set_variable.value = *$4;
      delete $4;
    }
    ;

opt_semicolon: /*empty*/
    | SEMICOLON
    ;
%%
//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, ParsedSqlResult *sql_result) {
  yyscan_t scanner;
  std::vector<char *> allocated_strings;
  yylex_init_extra(static_cast<void*>(&allocated_strings),&scanner);
  scan_string(s, scanner);
  int result = yyparse(s, sql_result, scanner);

  for (char *ptr : allocated_strings) {
    free(ptr);
  }
  allocated_strings.clear();

  yylex_destroy(scanner);
  return result;
}

/*
 * ***********************************************************************
 * yacc_sql.y 文件功能详解
 * 该文件是miniob数据库SQL语法解析器的语法定义文件，使用yacc/bison工具生成
 * SQL解析器。以下是对该文件支持的SQL语法和功能的详细分析：
 * ***********************************************************************
 */

/*
 * *************************** 基本结构说明 *****************************
 * 1. 文件结构
 *   - %{ ... %}: C语言代码片段，会被原样复制到生成的C文件中
 *   - %define, %locations, %lex-param, %parse-param: yacc/bison配置选项
 *   - %token: 定义词法标记（从词法分析器lex_sql.l获取）
 *   - %union: 定义语法规则中使用的数据类型联合体
 *   - %type: 定义非终结符的类型
 *   - %left, %right: 定义操作符优先级和结合性
 *   - %% ... %%: 语法规则定义
 *   - %% ... : 额外的C代码
 *
 * 2. Yacc/Bison工作原理
 *   - 采用LALR(1)（Look-Ahead LR）算法进行语法分析
 *   - 读取词法分析器提供的token序列，构建语法分析树
 *   - 当匹配到语法规则时，执行对应的动作代码（花括号内的代码）
 *   - $$ 表示规则左部的值，$1, $2, ... 表示规则右部各元素的值
 *   - 最终生成一个或多个ParsedSqlNode对象，存储在ParsedSqlResult中
 *
 * 3. 错误处理
 *   - 使用yyerror函数处理语法错误，将错误信息存储在ParsedSqlResult中
 *   - 配置了verbose错误信息，提供更详细的错误提示
 * ***********************************************************************
 */

/*
 * *************************** 支持的SQL语句类型 ************************
 *
 * 1. 数据定义语言(DDL)
 *   创建表语句
 *   example: CREATE TABLE table_name (column_definition [, column_definition]... [, PRIMARY KEY (column_name)]...) [STORAGE FORMAT = format];
 *   support: 支持INT, FLOAT, STRING, VECTOR数据类型，支持PRIMARY KEY定义
 *
 *   删除表语句
 *   example: DROP TABLE table_name;
 *   support: 删除指定名称的表
 *
 *   创建索引语句
 *   example: CREATE INDEX index_name ON table_name (column_name);
 *   support: 在指定表的指定列上创建索引
 *
 *   删除索引语句
 *   example: DROP INDEX index_name ON table_name;
 *   support: 删除指定表上的指定索引
 *
 *   分析表语句
 *   example: ANALYZE TABLE table_name;
 *   support: 分析表的统计信息
 *
 * 2. 数据操作语言(DML)
 *   插入数据语句
 *   example: INSERT INTO table_name VALUES (value [, value]...);
 *   support: 向表中插入一行数据
 *
 *   更新数据语句
 *   example: UPDATE table_name SET column_name = value [WHERE condition];
 *   support: 更新表中满足条件的行
 *
 *   删除数据语句
 *   example: DELETE FROM table_name [WHERE condition];
 *   support: 删除表中满足条件的行
 *
 *   查询数据语句
 *   example: SELECT expression [, expression]... FROM table_name [, table_name]... [WHERE condition] [GROUP BY expression];
 *   support: 
 *     - 支持表达式计算
 *     - 支持WHERE条件过滤
 *     - 支持表连接
 *     - 支持GROUP BY子句（当前未实现具体功能）
 *
 *   计算表达式语句
 *   example: CALC expression [, expression]...;
 *   support: 计算并返回表达式的值
 *
 * 3. 数据控制语言(DCL)
 *   开始事务语句
 *   example: BEGIN;
 *   support: 开始一个新事务
 *
 *   提交事务语句
 *   example: COMMIT;
 *   support: 提交当前事务
 *
 *   回滚事务语句
 *   example: ROLLBACK;
 *   support: 回滚当前事务
 *
 * 4. 其他语句
 *   显示表列表语句
 *   example: SHOW TABLES;
 *   support: 显示数据库中所有表
 *
 *   查看表结构语句
 *   example: DESC table_name;
 *   support: 显示表的列定义信息
 *
 *   同步数据到磁盘语句
 *   example: SYNC;
 *   support: 将内存中的数据同步到磁盘
 *
 *   加载数据语句
 *   example: LOAD DATA INFILE 'filename' INTO TABLE table_name;
 *   support: 从文件加载数据到指定表
 *
 *   执行计划语句
 *   example: EXPLAIN sql_statement;
 *   support: 显示SQL语句的执行计划
 *
 *   设置变量语句
 *   example: SET variable_name = value;
 *   support: 设置系统变量
 *
 *   帮助语句
 *   example: HELP;
 *   support: 显示帮助信息
 *
 *   退出语句
 *   example: EXIT;
 *   support: 退出客户端
 */

/*
 * *************************** 表达式和条件支持 ************************
 *
 * 1. 算术表达式
 *   support: +, -, *, /四则运算，支持括号和负数
 *
 * 2. 比较操作符
 *   support: = (EQUAL_TO), < (LESS_THAN), > (GREAT_THAN), <= (LESS_EQUAL), >= (GREAT_EQUAL), != (NOT_EQUAL)
 *
 * 3. 条件连接
 *   support: AND连接多个条件
 *
 * 4. 字段引用
 *   support: 支持表名.字段名或直接字段名两种引用方式
 *
 * 5. 值类型
 *   support: 整数、浮点数、字符串常量
 *
 * 6. 聚合函数支持
 *   代码中定义了聚合函数的解析，但具体实现需要在后续阶段绑定
 *
 * 7. 星号表达式
 *   support: SELECT * 表示选择所有字段
 */

/*
 * *************************** 数据类型支持 ************************
 * INT_T: 整数类型
 * FLOAT_T: 浮点数类型
 * STRING_T: 字符串类型，可指定长度
 * VECTOR_T: 向量类型
 */

/*
 * *************************** 解析器工作流程 ************************
 * 1. 调用sql_parse函数开始解析SQL字符串
 * 2. 初始化词法分析器scanner
 * 3. 调用scan_string设置要扫描的字符串
 * 4. 调用yyparse开始语法分析
 * 5. 词法分析器lex_sql.l提供token流
 * 6. 语法分析器根据规则构建语法树并执行相应动作
 * 7. 生成的语法树节点存储在ParsedSqlResult对象中
 * 8. 清理资源并返回解析结果
 */

/*
 * *************************** 注意事项 ************************
 * 1. 该语法定义是miniob数据库SQL方言的一部分，可能与标准SQL有所不同
 * 2. 某些功能（如GROUP BY）虽然在语法上支持，但可能在后续处理中有限制
 * 3. 语法规则中的内存管理需要注意，大部分动态分配的对象需要正确释放
 * 4. 语法错误会被捕获并记录在ParsedSqlResult中，不会导致程序崩溃
 */
