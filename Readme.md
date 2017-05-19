# Lizp -- A Toy Functional Language
---

Lizp is a functional language which based on lambda calculus.
 
## Features
- Currying
- High level functions
- Call by value
- Dynamic typed


## Grammar
```
<expr> ::= <var> | <list> | \<sym>.<expr> | (<expr> <expr>) | <let-expr>
<let-expr>  ::= let <var> = <expr> in <expr>
<var>  ::= <num> | <str> | <sym>
<list> ::= /[<expr>(,<expr>)*]/
<num>  ::= /-?[0-9]+/
<str>  ::= /"[^\"\0]*"/
<sym>  ::= /[^0-9][a-zA-Z0-9_-'~!@#$%^&*]+/
```

## Code Sample

- Lambda Definition
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
  (\x.\y.(+ x y) z)
    -> \y.(+ z y) 
```
    
- Function Definition
```
  let add1 = \x.(+ x 1) in (add1 2)
    -> 3
```
  
- Function Application
```
  (add1 2) 
    -> (\x.(+ x 1) 2)
      -> 2+1 
        -> 3
```
        
- High level function definition

  - To apply  a lambda
```
    let apply = \f.\x.(f x) in ...
```
    
  - To reduce a lambda
```
    let addn = \x.\y.(+ x y) in ...
```
    
- High level function application

  - Apply  a lambda
```
    apply add1 2 
      -> ((\f.\x.(f x) add1) 2) 
        -> (\x.(add1 x) 2)
          -> (\x.\y.((+ y 1) x) 2)
            -> (\y.(+ y 1) 2) 
              -> (+ 2 1) 
                -> 3
```
                
  - Reduce a lambda
```
    addn 2 
      -> (\y.\x.(+ x y) 2) 
        -> \x.(+ x 2)
```
        
- Variable Definition
```
  let var = 1 in ...
```
  
  
## Future Features

- Recursion Trick
```
  let fact = \n.(if (== n 0)
                    1
                    (self n))
  in ...
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
|[>,<,==,/=] | compare operators            |
|if          | symantic for branch          |
|self        | symantic for recursion       |
|cons        | symantic for list constuctor |



