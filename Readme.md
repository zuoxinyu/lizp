# Lizp -- A Toy Functional Language
---

Lizp is a functional language which based on lambda calculus.
 
## Features
- Currying
- High level functions
- Call by value
- Weak type deference


## Grammar
```
<expr> ::= <var> | (\<var>.<expr>) | (<expr> <expr>)
<def>  ::= def <var> = <expr>
<var>  ::= <num> | <str> | <idf>
<num>  ::= /-?[0-9]+/
<str>  ::= /"[^\"\0]*"/
<idf>  ::= /[^0-9][a-zA-Z0-9_-'~!@#$%^&*]+/
```


## Code Sample

- Lambda Defination
```
  (\x.x)
```
  
- Lambda Application
```
  ((\x.x) y) 
    -> y
```
  
- Currying
```
  (\x.(\y.x+y)) z 
    -> (\y.z+y) 
```
    
- Function Defination
```
  def add1 = (\x.x+1)
```
  
- Function Application
```
  (add1 2) 
    -> (\x.x+1) 2 
      -> 2+1 
        -> 3
```
        
- High level function defination
  - To apply  a lambda
```
    def apply = (\f.(\x.f x))
```
    
  - To reduce a lambda
```
    def addn = (\x.(\y.x+y))
```
    
- High level function application
  - Apply  a lambda
```
    apply add1 2 
      -> (\f.(\x.f x)) add1 2 
        -> (\x.add1 x) 2 
          -> (\x.(\y.y+1) x) 2 
            -> (\y.y+1) 2 
              -> 2+1 
                -> 3
```
                
  - Reduce a lambda
```
    addn 2 
      -> (\y.(\x.x+y)) 2 
        -> (\x.x+2)
```
        
- Variable Defination
```
  def var = 1
```
  
  
## Future Features

- Function Combinator 
```
  def add = apply addn 
```
  
- Recursion Trick
```
  def fact = (\x.x!=0 or 1?x*(self x-1):1) 
  ; <?:> is a builtin
  ; <self> is a keyword
```

> Actually, We can define everything as lambda(s). 

- Builtins:

| Element(s) | Symantic                     |
| ---------- | ---------------------------- |
|number      | nature numbers               | 
|string      | native strings               |
|[+,-,\*,/]  | algebra operators            |
|[&,\|,~]    | bitwise operators            |
|[&&,\|\|,!] | logic operators              |
|[>,<,==,!=] | compare operators            |
|cond        | symantic for goto            |
|self        | symantic for recursion       |
|cons        | symantic for list constuctor |



