
# Comannd Line Interface Editor

## How to start CLIEditor

In your c++ code, call `Fase::setEditor<CLIEditor>` like this;

```c++
#include <fase.h>

// ...

fase::Fase fase;
fase.setEditor<fase::editor::CLIEditor>();

// set your functions, and class ...

fase.startEditing();
```

The example is ready in `example/clieditor`.

## Commands

### print the information

* show all set function

This command shows all the functions that you set in your c++ code.

```
> show f(unction)
```

* show the state

This command shows all the nodes, and arguments in the state of now.

```
> show state
```

* show the links

This command shows all links between ths Node and ths Node or Argument.

```
> show link
```

### change the state

* add Argument

```
> add v [typename] [argument name] [value]
```

* add Constant Argument

```
> add c [typename] [argument name] [value]
```

* add Node

```
> add f [function name] [node name]
```

* delete Node or Argument

```
> del [name]
```

### build and run

* build pipeline and run

```
> run
```

* run without a building

```
> run nobuild
```

## Example

Build `example/clieditor` and run `./bin/clieditor`.
And type this commands;

```
> add v int a 1
> add v int b 5
> add f Add add
> add f Print print
> link add 0 a 0
> link add 1 b 0
> link print 0 add 2
> run
```

`6` will be outputted.  
Because a is 5 and b is 1, and the pipeline output a + b.  
If you type `> add v int a 10` and `> run` after that, `15` will be outputted.
