# SICP 读书笔记

## Abstract

### 名字与抽象 Naming and Abstract

抽象即从模块中抽离出公共的部分。名字是实现抽象的手段。

### 名字与环境 Name and Environment

将一个名字与一个值绑定的过程可视作一次计算。其计算对象 (oprand) 为 ([Name:Value]) 和 (Env)。
实现手段是 (define)

### 值与过程 Values and Procedures

一个值可以为常量 (constant)，亦可为一个过程 (procedure)。

### 计算 Calculation / Evaluation

一个程序本质上是一次计算。

### Lambda演算 Lambda Calculus

Everything is lambda.
Lambda 本身没有意义，如果给其赋予一个名字，即可模拟出所有。

### 值 Values

一个值所必须支持的操作有：
1. Eval()

### 作用域 Scoping

一个名字的作用域取决于其所处的环境。一个环境由一个符号表和一个用于遍历的指针组成。本质上是一个容器，实现方式可以是一个线性表或者 HashTable。

其必须支持的操作有：

1. AddBinding(name, value)
2. DelBinding(name)
3. FindBinding(name)

若要支持块级作用域，还必须提供：

4. ParentScope()
5. ChildrenScope()

此外，在查找操作中，如果支持块级作用域，其查找范围可以上溯而不可下游。
