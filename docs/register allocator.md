# Register Allocator

Register allocator is an integral part of any compiler that compiles source code down to machine instructions.

The reason register allocators even exist, is that CPUs have a limited number of general purpose registers (x86_64 has 16 general purpose registers).

Allowing the program to store its state only in registers, would imply a serious limitation on the number of possible variables a program is allowed to use. Furthermore, that limitation would cover not just variables, but also constants and interpediate results of expressions, leaving the programmer with even less number of usable variables. In case of x86_64 it would be a total of 16 possible variables, constants and interemediate expression results together.

Therefore such a limitation is not a feasible solution. We need to store some important values in the registers for quick access, and other less important on the stack.

And the decision process behind choosing which values go in registers and which go onto the stack, is what register allocator is resposible for.

# The process of register allocation

The implementation of the register allocator is the part of the `x64` backend and is located in `code_gen/src/code_gen/backends/x64/x64.c`.

The API for it is kept rather simple. It is a single function, that accepts the backend state, and a bit mask of allowed registers, and produces a list of storage locations for each instruction that requires one.

```c
void x64_alloc_registers(X64CodeGenerator* gen, uint16_t allowed_registers);
```

The result gets stored in the `instr_storage` field on the backend state (`X64CodeGenerator` struct):

```c
typedef struct {
    InstrBuffer instr_buffer;
    InstrUsageRange* usage_ranges;
    InstrStorageLocation* instr_storage;
    
    Arena* allocator;
    Arena* temp_allocator;
  
    CodeBuffer* per_region_code_buffer;
  
    const FunctionRefTable* ref_table;
  
    uint16_t* phi_variant_counts_per_region;
    InstrIndexArray* phi_variants_per_region;
  
    // A per region array of phi instructions that select a variant from that region.
    // Size of the array is in `phi_variant_counts_per_region`
    InstrIndex** phi_node_of_variant;
} X64CodeGenerator;
```

The result is an array of `InstrStorageLocation`, which can be `NONE` if the instruction doesn't need storage, `REG` if the value must be stored in the register or `STACK` if the value is stored on the stack:

```c
typedef enum {
    INSTR_STORAGE_NONE,
    INSTR_STORAGE_REG,
    INSTR_STORAGE_STACK,
} InstrStorageKind;

typedef struct {
    InstrStorageKind kind;
    union {
        X64Register reg;
    };
} InstrStorageLocation;
```

> [!IMPORTANT]
> The current version of the register allocator doesn't yet support allocating stack locations. So the resulting store locations can ony be registers.

Considering the above notice the current implementation of the register allocator, is relatively simple. However, a full register allocator is easly one of the most complicated parts of the compiler.

The current implementation can be split into 3 steps, visible in the following code snippet:
```c
InstrIndexArray instr_with_storage_requirement = _x64_gather_instr_with_storage_requirement(
    gen->instr_buffer,
    gen->usage_ranges,
    gen->temp_allocator);

UInt16Array* interference_graph = _x64_build_interference_graph(gen->instr_buffer, 
    instr_with_storage_requirement,
    gen->usage_ranges,
    gen->temp_allocator);

gen->instr_storage = arena_alloc_array_zeroed(gen->allocator,
    InstrStorageLocation,
    gen->instr_buffer.count);

_x64_run_graph_coloring(gen->instr_buffer,
    instr_with_storage_requirement,
    interference_graph,
    gen->instr_storage,
    allowed_registers,
    gen->temp_allocator);
```

The first step is to gather all instructions that require a store locations.

The second step is to build an inerference graph.
An interference graph is a graph where each vertex corresponds to an instruction (that requires a storage location), and edges are created between instructions whose live ranges overlap. A further important property is that vertices that share an edge, aren't allowed to share the same storage location.

A **live range** is a range of program points where the instruction result must be kept alive (stored somewhere). 

In **Cik** the indices of instructions in the instruction buffer act as program points.

Let's look at an example, on how we can determine live ranges for a compiled code snippet and then build an interference graph:
```c
int main(int argc, char* argv[]) {
    int a = 10;
    int b = 11;
    return a + b;
}
```

This snippet produces the next set of instructions:
```
0          no_op
1          no_op
2          io_state                producer: 65535
3          region                  id: 0 last_instr: 7
4          const_32                u: 10 i: 10 f: 0.000000
5          const_32                u: 11 i: 11 f: 0.000000
6          bin_op_32               kind: add left: 4 right: 5
7          return_value            value: 6 io_state: 2
```

Now let's build live ranges for these instructions:

```
  io_state region const_32 const_32 bin_op_32 return_value
0    
1
2     |
3     |       |                                    |
4     |              |                             |
5     |              |        |                    |
6     |              |        |         |          |
7     |                                 |          |
```

On the left we have a column of program points, and live ranges are marked with `|`. For example, a live range for `io_state` is `[2; 7]`.

Now that we have an array of live ranges of each instruction, we can build an interference graph. Out of all instructions we only have 3 that require a storage locations: `const_32` (at index 4), `const_32` (at index 5) and `bin_op_32`.

Their corresponding live ranges are:
1. `const_32` - `[4; 6]`
2. `const_32` - `[5; 6]`
3. `bin_op_32` - `[6; 7]`

By forming edges beween instructions whose live ranges overlap, we get the following interference graph:

```
 4 --- 5

    6
```

Here we've created an edge between, `const_32` instructions at indices 4 and 5 have an edge, since their live ranges overlap, thus they aren't allowed to share a store location.

> [!NOTE]
> `bin_op_32` overlaps with both `const_32` instructions, however it still is not connected to any of them.
> 
> This is a little optimization case, to reduce register pressure.
> 
> Here both `const_32`, are only consumed by the `bin_op_32`, and no longer needed afterwards. So starting from `bin_op_32` the register allocator, can reuse storage locations of inputs. And it does, by using of them as an output location for `bin_op_32`.

Now that we have a ready intereference graph, we can run the final step of the process, which is graph coloring. The idea is that storage locations act as colors, and we are not allowed to assign the same color (storage location) to vertices that share an edge.

> [!NOTE]
> Let's return back to the previous note, where it was stated that `bin_op_32` is not connected to the `const_32` inputs, in order to reuse one of the registers.
> 
> Let's say, we didn't apply that optimization, and were instead to create an edge between `bin_op_32` and its inputs, we would get the next interference graph with 3 edges:
>
> ```
> 4 --- 5
>  \   /
>    6
> ```
> 
> The previous graph we were able to color with just 2 colors (registers), but this one we can only color with 3 colors. Thus increasing register pressure.

Here is the implementation of the graph coloring phase, minus handling of the function arguments that must be allocated in specific registers defined by the calling convetion:

```c
// Assign locations to the rest of the instructions
for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
    InstrIndex instr_index = instr_with_storage_requirement.instr[i];

    // Skip `INSTR_LOAD_ARG` instructions, that have already received their storage
    // locations defined by the calling convention
    if (instr_storage[instr_index.value].kind != INSTR_STORAGE_NONE) {
        continue;
    }

    uint16_t potential_registers = potential_instr_registers[instr_index.value];
    assert_msg(potential_registers != 0,
        "This instruction must be spilled, but spilling is not yet implemented");
    
    // Select the first suitable register
    uint16_t first_potential_register = count_trailing_zeros(potential_registers);
    assert(first_potential_register < 16);
    
    instr_storage[instr_index.value].kind = INSTR_STORAGE_REG;
    instr_storage[instr_index.value].reg = first_potential_register;
    
    // Disallow allocations of the selected register,
    // for the instructions that share an edge with the current instruction.
    UInt16Array edges = interference_graph[i];
    for (size_t j = 0; j < edges.count; j += 1) {
        InstrIndex interfering_instr = instr_with_storage_requirement.instr[edges.values[j]];
        potential_instr_registers[interfering_instr.value] &= ~(1 << first_potential_register);
    }
}
```

Well that's the end of register allocator implementation.

Let's look at the result of register allocation on the same little example:
```c
int main(int argc, char* argv[]) {
    int a = 10;
    int b = 11;
    return a + b;
}
```

This snippet produces the next set of instructions:
```
0          no_op
1          no_op
2          io_state                producer: 65535
3          region                  id: 0 last_instr: 7
4          const_32                u: 10 i: 10 f: 0.000000
5          const_32                u: 11 i: 11 f: 0.000000
6          bin_op_32               kind: add left: 4 right: 5
7          return_value            value: 6 io_state: 2
```

Out of all the instructions we only 3, that require a storage location: two `const_32` and a `bin_op_32`.

After running all the steps of the register allocator (as it was described above), we arrive at the final result:

```
Assigned storage locations:
0: none
1: none
2: none
3: none
4: EAX
5: ECX
6: EAX
7: none
```

Total number of used register is only 2, because the output of `bin_op_32` shares the same register as one of it's inputs.
