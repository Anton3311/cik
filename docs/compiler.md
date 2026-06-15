# Compiler

A compiler is where the parsed AST turns into a some sort of an executable form, either machine code or bytecode instructions. However a more advanced approach, that production grade compilers rely on, is to compile first into a machine-independent intermediate instruction format, which is usually called an Intermediate Representation. IR is used to perform all sorts of optimizations, before it is lowered to machine specific instructions, like x86_64 machine code.

**Cik** compiler is no different, as it relies on it's own IR format, which is based on Sea of Nodes. It is defined in `code_gen/src/code_gen/instr.h`.

Before going into more details about Sea of Nodes, it is first important to mention Single Static Assignment form, or SSA for short, which is a base for the Sea of Nodes.

## Single Static Assignment

SSA is a way to represent computations of a program in a form where each variable is only assigned once.

A value of a particular variable in any program changes over time, this complicates trivial optimizations such as **constant propagation**.

> [!NOTE]
> **Constant propagation** is an optimization which replaces usages of a variable with its value.
> 
> This, in turn, allows room for a different optimization called **constant folding**, which replaces operations on constants with their results at compile time, therefore completely avoiding these computations at runtime.

Let's look at a simple example with an assignement to a variable:

```c
int a = 10;
int c = a + 10;
a = 0;

int d = c + a;
```

The above example will produce the next expression tree for the variable `d`:

```
  +
 / \
c   a
```

Replacing usages of `c` and `a` with their values, gives the following tree:

```
      +
    /   \
  +       a
 / \      
a   10
```

Here we end up with two usages of a variable `a`, and we want to further replace them with its value. However, right as we try to do this, we hit a roadblock: we have two usages, but in between them the variable gets reassigned. A simple solution of tracking the current value of each variable, won't cut it here. If we were to use this approach, we would end up with the following incorrect expression tree:

```
      +
    /   \
  +       0
 / \      
0   10
```

And after evaluating this tree, the result would be `10`, instead of `20`.

Producing a correct tree, would require tracking all possible values that have ever been assigned to a particular variable, so that later when replacing the usages, we could decide, based on where this variable usage located in the expression, which value to use. This complicates things a lot.

However, luckily there is a clear solution, which involves creating a new variable version on every assignment. This has a one important property that drastically simplifies everything: every variable can only have a single value, throught the whole lifetime of the problem.

Once the time comes, to replace variable usages, all the above algorithms for deciding, which value of the variable to use, are completely gone, since a variable is only allowed to have a single value.

And that is the main idea behind single static assignment.

By applying the above solution, the initial example will look like this:

```c
int a_0 = 10;
int c_0 = a_0 + 10;
int a_1 = 0;

int d_0 = c_0 + a_1;
```

And this produces the next expression tree:

```
      +
    /   \
  +       a_1
 / \      
a_0 10
```

Since in SSA form a variable can have only one value, which doesn't change over time, we can simply replace usages with respective values and get a correct result:

```
      +
    /   \
  +       0
 / \      
10  10
```

Evaluating the above expression tree gives us a value of `20`, which is exactly what was expected.

## SSA Phi Nodes

The above approach works flawlessly on linear code, however once branches get instroduced, further roadblocks appear.

Let's say, a variable gets assigned inside one of the if statement branches, we create a new version of that variable, and use it in further expressions, everything works.

But then we come to the end of the if statement and return to the parent scope, that is where the problem is hiding. After returning to the parent scope, we face the same problem like the last time, when trying to do constant propogation before SSA: **we somehow need to decide which version of that variable to use**.

The if statement branch might run, or might not, therefore the new variable version might be created or might not be, which is fully based on the condition.

Unfortunately there are no an equally elegant solution to it.

This is where the phi nodes come into play. A `phi` node is usually its own instruction, whose purpose is to select different variable versions, based on which side of the branch has run.

In `Cik` phi nodes are defined by `INSTR_PHI` and `INSTR_SELECT` or the `phi` and `select` variants in the `Instr` struct.

```c
struct {
    InstrInputs variants;
} phi;

struct {
    InstrIndex value;
    InstrIndex region;
} select;
```

For example, we have the following code snippet, that conditionally modifies a variable:

```c
// parent
int b = 0;
if (condition == 0) {
    b = 10;
} else {
    b = 100;
}

printf("%d", b);
```

After turning it into SSA form and placing a phi node, we get the following result:

```c
int b_0 = 0;
if (condition == 0) {
    b_1 = 10;
} else {
    b_2 = 100;
}

int b_3 = phi(b_1, b_2);
printf("%d", b_3);
```

Value of `b_3` is a phi node, which selects between either `b_1` or `b_2`, based on the condition `condition == 0`.

With that part now covered, it is now possible to explore the Sea of Nodes form, before going deeper into the details of how the compiler is implemented.

## Sea of Nodes

Sea of Nodes is built upon SSA, however traditional implementations of SSA have a Control Flow Graph alongside them. Sea of Nodes, however, merges these two structures into a single graph, turning CFG nodes into their own instructions, which are called `regions`.

Sea of Nodes comes with an extra property, which is a lack of an explicitely defined order of execution, order is only defined by instruction dependencies, therefore allowing instructions to execute in any order.

However, the lack of an explicit order is both a blessing and a curse, in some cases the compiler can reorder operations to speedup execution, in others, however, it is neccessary to define an explicit order of operations to ensure correctness of the program.

One of such cases are function calls.

For instance, if the program has two sebsequent calls to `printf`, we want them to happen in the same order they were defined in the source code, or else the output wouldn't be the one that was expected. This is one of many examples when the explicit serialization of operations must be used. 

To mitigate this issue a new instruction, called `INSTR_IO_STATE`, was instroduced. It's injected into every single function call and every control instruction as an input, making it a dependecy. Every function call produces a new `io state`, which eventually gets consumed by another call or a control instruction.

For example, if we have a call followed by a control instruction, the call consumes the initial `io state` and produces a new one, the control then consumes a newly created `io state` forming a dependency on the function call.

Let's look at a simple example, with two function calls:

```c
void print();

int main(int argc, char* argv[]) {
    print();
    print();
    return 0;
}
```

This compiles down to the following IR instructions (compiled with ` --show-ir --keep-dead-instr` flags):

```
0          load_arg                index: 0
1          load_arg                index: 1
2          io_state                producer: 65535
3          region                  id: 0 last_instr: 9
4          call_internal           args: [] io_state: 2 function_index: 0
5          io_state                producer: 4
6          call_internal           args: [] io_state: 5 function_index: 0
7          io_state                producer: 6
8          const_32                u: 0 i: 0 f: 0.000000
9          return_value            value: 8 io_state: 7
```

Here we have a single region at index 3, and its last instruction is a return at index 9. Each instruction that dependends on an io state has an `io_state` field.

By traversing the chain of calls and io states, starting from the return instruction we get the following dependency chain:

```
return at 9 -> call at 6 -> call at 4
```

Finally, when lowering to machine code, the call at 4 will execute first, then the call at 6 and finally the return.
