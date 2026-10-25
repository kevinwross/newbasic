# Enhanced SUB/FUNCTION and lexical locals — implementation plan

**Status:** Proposal only. No implementation has started.  
**Date:** 2026-10-24  
**Feature:** "Idea #1" — scoped procedures, parameters, local variables, and functions  
**Primary motivation:** remove the frame protocol and shared-scratch workarounds in
`baslib`, while giving ordinary newbasic programs a safer way to write larger
programs.  
**Approval gate:** do not write code until this plan and the decisions in
[§11](#11-decisions-needed) are approved.

---

## 0. Recommendation in one page

Implement a newbasic extension with:

- `SUB Name[(params)] ... END SUB`
- `FUNCTION Name%(params) ... END FUNCTION`
- `LOCAL name[, name]...` at the top of a procedure
- `CALL Name` / `CALL Name(args)` for subs
- `Name(args)` in expressions for functions
- `EXIT SUB` / `EXIT FUNCTION`
- parameters by value, with exact count checking
- per-call local storage, recursion, and lexical visibility
- undeclared names still resolve to globals

The recommended storage model is **per-call named local scopes**, not
save/restore of globals and not compiler-allocated frame slots in v1.

That choice is the important one:

- **Saved globals** are the smallest implementation and are how `DEF FN`
  currently works, but they are dynamically scoped. A caller's local `x%`
  would be visible to a callee that has no local `x%`. That is not the feature
  we want.
- **Compiler-allocated slots** are the cleanest and fastest, but every
  instruction that names a variable would need a local-aware variant. That is a
  much larger change than the first version needs.
- **Named per-call scopes** keep the existing variable-access instructions and
  `LoadVariable`/`StoreVariable` seams, while making each procedure call own a
  private name map. A scope stack supplies lexical visibility. The remaining
  work is to route the handful of direct `vars_` writes through scoped accessors.

The public syntax is the same whichever storage model is used, so a later
optimization to slots or frame arenas does not change programs.

V1 deliberately excludes `BYREF`, local arrays, optional/default parameters,
overloading, closures, nested procedure definitions, and modules. Those are
phase-2 candidates after the core is stable.

**Effort:** large. It touches the lexer keyword table, front-end pre-scan and
compiler, `Program` metadata, VM variable access and call frames, `FOR`/`SELECT`
cleanup, `ON ERROR`/`RESUME`, snapshots, the debugger, DAP scopes, the statement
inventory, tests, and the reference manual. It should be one milestone series,
not one commit.

---

## 1. Goals and non-goals

### 1.1 Goals

1. A procedure call gets its own parameters and `LOCAL` variables.
2. Recursion works without extra user-written save/restore code.
3. A callee cannot accidentally read a caller's local variables.
4. Undeclared names remain global, so existing programs and the host API keep
   working.
5. `CALL`/`SUB` and `FUNCTION` calls are checked at compile time for existence
   and argument count.
6. `FOR`, `INPUT`, `READ`, `SWAP`, `LSET`/`RSET`, `FIELD`, and `STACK` targets
   keep working when they name a local variable.
7. `ON ERROR`/`RESUME` has defined behavior in the presence of procedure frames.
8. Debugger call stacks, locals, stepping, snapshots, and DAP display understand
   procedures.
9. Existing programs, `GOSUB`, `DEF FN`, `RENUM`, `INCLUDE`, and the baslib
   public API continue to work.
10. The design leaves room for a later hidden-slot optimization without changing
    the language.

### 1.2 Non-goals for v1

- `BYREF` parameters (true references / copy-out).
- Whole-array parameters.
- Local arrays, local `DIM`, or local file `FIELD` buffers.
- Optional parameters, default values, variadic parameters.
- `STATIC` locals.
- Local `ON ERROR`/`ON TIMER` handlers; v1 requires handlers in main.
- Procedure overloading or type-generic procedures.
- Nested procedure definitions or closures.
- Modules, namespaces, or `PRIVATE`/`PUBLIC`.
- `OPTION EXPLICIT` or a `GLOBAL` escape for shadowed names.
- Full OOP, classes, or multiple windows.
- Source maps for `INCLUDE` (separate tooling project).
- Rewriting baslib in the same change. Baslib migration is a separate,
  follow-on phase after the language feature is stable.

---

## 2. Proposed user-facing syntax

These examples are illustrative; exact keyword choices are in §11.

Compact grammar:

```text
SUB name [(param[, param]...)]
    [LOCAL name[, name]...]...
    statements
END SUB

FUNCTION name% [(param[, param]...)]      ' % may be % & ! # or $
    [LOCAL name[, name]...]...
    statements
    name% = expression                    ' function result
END FUNCTION

CALL name
CALL name(argument[, argument]...)
name(argument[, argument]...)             ' function call in an expression
EXIT SUB
EXIT FUNCTION
```

### 2.1 A sub with parameters and locals

```basic
100 SUB DrawButton(h%, x%, y%, w%, hgt%)
110   LOCAL face%, label$, tx%, ty%
120   face% = blUser#(h%, 8)
130   label$ = blCtlText$(h%)
140   tx% = x% + 2
150   ty% = y% + 2
160   LINE (x%, y%)-(x% + w% - 1, y% + hgt% - 1), face%, BF
170   DRAWTEXT (tx%, ty%), label$, blUser#(h%, 19), 1
180 END SUB
```

```basic
300 CALL DrawButton(ButtonH%, 10, 20, 180, 28)
```

### 2.2 A function

```basic
100 FUNCTION Clamp%(v%, lo%, hi%)
110   LOCAL r%
120   r% = v%
130   IF r% < lo% THEN r% = lo%
140   IF r% > hi% THEN r% = hi%
150   Clamp% = r%
160 END FUNCTION

200 PRINT Clamp%(150, 0, 100)
```

The function result is the function-named local variable. `Clamp% = r%` assigns
it. Falling into `END FUNCTION`, or executing `EXIT FUNCTION`, returns the
current value.

### 2.3 Locals shadow globals

```basic
10 X% = 99
20 CALL Show
30 PRINT X%
40 END
100 SUB Show
110   LOCAL X%
120   X% = 42
130   PRINT X%
140 END SUB
```

Output:

```text
 42
 99
```

### 2.4 Recursion

```basic
100 FUNCTION Fact#(n%)
110   IF n% <= 1 THEN Fact# = 1 : EXIT FUNCTION
120   Fact# = n% * Fact#(n% - 1)
130 END FUNCTION
```

Each activation has its own `n%` and result slot.

### 2.5 Early exit

```basic
100 SUB Validate(ok%)
110   IF ok% = 0 THEN EXIT SUB
120   PRINT "valid"
130 END SUB
```

### 2.6 No parentheses for a zero-argument sub

```basic
CALL Reset
```

`CALL Reset()` is also acceptable. A function with no parameters is called as
`Name()`; a bare `Name` stays a scalar reference.

### 2.7 What is deliberately not in the syntax

- No `SUB` inside a `SUB` or `FUNCTION`.
- No `LOCAL` outside a procedure.
- No `LOCAL A%(10)` in v1.
- No `BYREF` in v1.
- No `CALL` to a `FUNCTION`; use the function in an expression.
- No assignment to a `FUNCTION` name outside that function.
- No `RETURN value`; `RETURN` remains the `GOSUB` return statement. Use
  `EXIT FUNCTION`/`END FUNCTION` and assign to the function name.

---

## 3. Semantics

### 3.1 Procedure definitions

- A procedure is defined by a numbered line whose first token is `SUB` or
  `FUNCTION`. The header line must contain only the header and an optional
  comment; body statements start on later lines.
- The body ends at the matching `END SUB` or `END FUNCTION`. The `END` line
  must contain only `END SUB`/`END FUNCTION` and an optional comment.
- Definitions are executed inertly: the header line emits a jump over the body,
  so ordinary sequential execution never falls into a procedure.
- Procedures are global and may be called before their definition (the compiler
  pre-scans headers).
- Procedure names are case-insensitive, like all BASIC names.
- A `SUB` name has no type suffix. A `FUNCTION` name must have a type suffix
  (`%`, `&`, `!`, `#`, or `$`) so its result type and call name are unambiguous
  during the pre-scan.
- Duplicate procedure names are a compile error.
- A procedure name may not also be used as a scalar or array anywhere in the
  program; see §3.10.
- Procedures may not be nested. A `SUB`/`FUNCTION` header inside another
  procedure is a compile error.
- An `END SUB`/`END FUNCTION` without an open procedure is a compile error.
- A procedure with no body is legal; a function with no assigned result returns
  its type's default (0 or "").

### 3.2 Parameters and locals

- Syntax: `SUB Name(a%, b$, c#)`; types come from the name suffix or the
  current `DEFINT`/`DEFSNG`/`DEFDBL`/`DEFSTR` default.
- `LOCAL a%, b$` declares additional locals.
- Parameters and `LOCAL` names share one namespace inside the procedure.
  Duplicate names are a compile error.
- A parameter/local name must not collide with a `CONST` name.
- `LOCAL` declarations must appear before the first executable statement in the
  procedure. Multiple `LOCAL` lines are allowed. A `LOCAL` after executable code
  is a compile error.
- Locals are initialized on every call: numeric 0 of the declared kind, strings
  to `""`.
- Parameters are assigned from the call arguments after initialization.
- Locals shadow globals for the duration of the call.
- Undeclared names inside a procedure are global, exactly as they are today.
- A callee does **not** see the caller's locals. Only its own locals and globals
  are visible.
- `DEF FN`, timer handlers, and global error handlers execute with a global
  scope barrier so they do not accidentally read a procedure's locals; see
  §5.1.

### 3.3 Calls and argument evaluation

- `CALL Name` calls a zero-argument sub.
- `CALL Name(arg1, arg2, ...)` calls a sub with arguments.
- `Name(arg1, ...)` calls a function in an expression.
- A function call always requires parentheses, even with zero arguments:
  `Now%()`.
- Arguments are evaluated left to right in the caller's scope.
- Arguments are passed by value in v1.
- Each argument is converted to the parameter's declared type using normal
  assignment rules: rounding for integer kinds, `Type mismatch` for string vs
  numeric mismatches, `Overflow` for out-of-range integers.
- The argument count must match exactly; mismatch is a compile error.
- A call to an undefined procedure is a compile error. Forward calls are
  supported.
- `CALL` on a `FUNCTION` is a compile error.
- A `FUNCTION` name used as a value without parentheses is a compile error
  outside that function; inside the function it reads the result local.
- Direct-mode `CALL` is illegal.

### 3.4 Return and early exit

- `END SUB` returns from a sub.
- `END FUNCTION` returns the current function result.
- `EXIT SUB` returns immediately.
- `EXIT FUNCTION` returns the current function result immediately.
- `RETURN` is still the `GOSUB` return statement. It is not a procedure return.
- If a procedure body executes `RETURN` without an active `GOSUB` at or above
  the procedure's entry depth, the VM reports `RETURN without GOSUB` rather than
  popping a caller's GOSUB frame.
- A procedure may `GOSUB` to lines in its own body; the `RETURN` goes back to
  the procedure.
- `GOTO`/`GOSUB` may not enter or leave a procedure body; see §3.6.
- `END` inside a procedure ends the program, not the procedure.
- `STOP` inside a procedure breaks to the debugger; the procedure frame remains
  for inspection and can be resumed or rewound.

### 3.5 Recursion and call depth

- Each call gets a fresh scope and fresh locals.
- Recursion is allowed.
- A configurable maximum call depth protects the host (proposed 256; see §11).
- Exceeding it reports a new extension error, proposed
  `Call stack overflow` (code 79), without partially constructing a frame.
- `DEF FN` calls and procedure calls share the depth budget, or at least both
  are bounded; exact accounting is in §5.5.

### 3.6 Control-flow boundaries

A procedure body is a lexical region. Jumping into or out of it is unsafe
because the frame would be missing or would outlive its locals.

| From | To | Allowed? |
|---|---|---|
| main | main label/line | yes |
| main | line inside a `SUB`/`FUNCTION` body | no |
| procedure | line in the same procedure body | yes |
| procedure | main line | no |
| procedure | line in another procedure body | no |
| procedure | its own header or `END SUB`/`END FUNCTION` line | no |
| `GOSUB` in procedure | label in same procedure | yes |
| `GOSUB` in procedure | main or another procedure | no |
| `ON ERROR GOTO` handler | main only in v1 | yes (error path is special; local handlers are phase 2) |
| `ON TIMER` handler | main only in v1 | yes (event path is special) |
| `RESUME` | current procedure or main | yes, with frame reconciliation |

Rules:

- Literal line numbers, labels, `IF ... THEN line`, and `ON ... GOTO/GOSUB`
  targets are checked at compile time.
- Computed `GOTO`/`GOSUB` targets are checked at run time against the current
  procedure range.
- `CALL` is the supported way to cross from one procedure to another.
- `GOSUB` remains the supported way to share a helper inside one procedure.
- `ON ERROR` and `ON TIMER` are exempt because the VM, not the user, controls
  the jump. Their scope rules are in §5.7 and §5.1.
- `RESTORE <line>` is a DATA pointer, not a code jump; it is exempt.
- `RUN <line>` restarts the program; procedure frames are discarded.

### 3.7 `FOR`, `SELECT`, and `GOSUB` interaction

A procedure frame records:

- the `FOR` stack depth at entry,
- the `SELECT CASE` stack depth at entry,
- the `GOSUB` stack depth at entry.

On `END SUB`, `END FUNCTION`, `EXIT SUB`, or `EXIT FUNCTION`:

- `FOR` loops opened by the procedure are discarded by trimming `for_stack_` to
  the entry depth. This mirrors the existing `RETURN` behavior.
- If the procedure still has an open `SELECT CASE` relative to entry, the
  return reports `EXIT SUB would leave a SELECT CASE` (or the function
  equivalent). The procedure must close its own `SELECT`.
- If the procedure still has an open `GOSUB` relative to entry, the return
  reports an error rather than silently discarding the GOSUB frame. A procedure
  should finish its own `GOSUB` helpers before returning.
- A `GOTO` out of a `SELECT CASE` remains a runtime error, as it is today.

### 3.8 Error handling and `RESUME`

This is the most delicate part and is deliberately staged.

**V1 restriction:** `ON ERROR GOTO` handlers and `ON TIMER` handlers must be in
**main**, not inside a procedure. Local handlers are a phase-2 extension. This
keeps the scope and unwinding rules tractable.

V1 semantics:

- A handler in main runs with a global scope barrier: it sees globals, not any
  procedure's locals.
- `RESUME` with no target retries the failing statement. If the failing
  statement is inside a procedure, its frame is still alive and the barrier is
  removed; execution continues with the procedure's locals.
- `RESUME NEXT` continues at the next statement. It stays in the current
  procedure if the next statement is still in that procedure; otherwise it
  pops the procedure frame(s) it leaves.
- `RESUME <line>` jumps to a line. If the line is in the current procedure, the
  frame stays. If it is in main, procedure frames are popped. If it is in an
  outer active procedure, inner frames are popped. If it is in a different,
  inactive procedure, that is an error (`RESUME across a procedure boundary`).
- An error raised inside a procedure with no handler exits the program; frames
  remain available to the debugger until the next run/reset.
- `ON ERROR GOTO 0` disables trapping as today.

V1 does not attempt to make `RESUME` a general exception-unwinding mechanism.
The rule is: resume inside the same procedure, or resume outward by popping
frames.

### 3.9 Direct mode, `RENUM`, `INCLUDE`, and program editing

- `SUB`, `FUNCTION`, `END SUB`, `END FUNCTION`, `LOCAL`, `EXIT`, and `CALL` are
  illegal in direct mode. They need a stored program.
- `LIST`/`SAVE` show the source as written; procedure headers and `END`
  statements are ordinary lines.
- `RENUM` moves procedure lines but does not touch `CALL`/function names.
  Procedure ranges are recomputed at compile time. A `RENUM` that reorders
  procedure headers relative to their `END` lines is rejected by the existing
  reorder check or fails to compile.
- `INCLUDE` expands before compilation, so procedures may be defined in an
  included library. The included lines share the global line-number space.
  Cross-file procedure bodies are legal but not recommended; a header and its
  `END` should normally live in the same file.
- `DELETE` that removes an `END SUB`/`END FUNCTION` produces the normal
  "SUB without END SUB" compile error.
- `CHAIN`/`RUN` discard procedure frames.
- `CLEAR` inside a procedure is rejected. `CLEAR` in main still clears
  variables, arrays, execution stacks, and procedure frames as it clears the
  other execution state today.
- `COMMON` variables are globals and survive `CHAIN`; locals do not. A
  `COMMON` statement inside a procedure names globals, not its locals.
- `STOP`/breakpoint leaves frames in place for debugging.

### 3.10 Names and collisions

- Procedure names occupy a program-wide procedure namespace.
- A procedure name may not also be used as a scalar, array, or `CONST`.
  `SUB Foo` plus `Foo = 1` is a compile error. This avoids ambiguous calls and
  function-result reads.
- A `SUB` and a `FUNCTION` may not share a name.
- A `FUNCTION` name and a `DEF FN` name may not share a name.
- Labels and procedures are separate namespaces: `CALL Foo` and `GOSUB Foo`
  can coexist, but this is discouraged and may be rejected later.
- Parameter/local names may match a procedure name? No: reject, to avoid
  confusing `CALL`/function resolution inside the body.
- Parameter/local names may match a label? Labels are a separate namespace;
  allow, but do not recommend.
- `DEF FN` definitions are global and are not allowed inside a procedure in
  v1. This avoids a second, differently scoped expression body inside a
  procedure range. A procedure may call a `DEF FN` defined in main; that
  function sees globals, not procedure locals.
- `DEFINT`/`DEFSNG`/`DEFDBL`/`DEFSTR` remain program-order global declarations.
  A `DEF*` line inside a procedure is not scoped to that procedure; do not use
  one to change local types. Declare local types with suffixes.
- `ON ERROR GOTO` and `ON TIMER` handler lines must be in main in v1; see §3.8.

### 3.11 Debugger-visible semantics

- The top frame's locals are inspectable and editable.
- Globals are inspectable as today.
- Call frames show `SUB Name`/`FUNCTION Name` with call-site line numbers.
- Snapshots include procedure frames and local scopes.
- Rewind restores the same frames and locals.

---

## 4. Front-end and compiler design

### 4.1 Lexer changes

Add reserved words:

- `SUB`
- `FUNCTION`
- `LOCAL`
- `EXIT`

`CALL` is already reserved but currently in the unimplemented statement list.
`END` is already reserved. `BYREF`/`BYVAL` are not needed in v1; when added,
they can be contextual words after a parameter name rather than global
keywords, to reduce compatibility impact.

Compatibility note: exact identifiers `SUB`, `FUNCTION`, `LOCAL`, and `EXIT`
become reserved. Whole-word lexing means `SUBTOTAL`, `FUNCTIONAL`, and
`LOCALNAME` remain ordinary names. This is the same trade-off as `LABEL`,
`CONST`, `STACK`, `SELECT`, and `SLEEP`.

Update:

- `Lexer` keyword table (strictly sorted).
- `tests/test_lexer.cpp`, `tests/test_names.cpp`, `tests/test_robustness.cpp`.
- `tests/test_inventory.cpp` (`CALL` moves from stub to implemented; new
  keywords are added to the appropriate inventory checks).
- `RefManual/README.md` index and new `RefManual` entries.

### 4.2 Pre-scan pass: `BuildProcedures`

Run after `BuildConsts` (for `CONST` names) and before/alongside `BuildLabels`.

For each stored line:

1. Tokenize.
2. If the first token is `SUB` or `FUNCTION`, parse:
   - name (identifier; `SUB` must be suffix-free, `FUNCTION` must have a type
     suffix),
   - optional parameter list (names only; types resolved later),
   - header line number,
   - procedure kind.
3. Record a `ProcedureDecl` in a pre-scan table:
   - raw name,
   - kind,
   - header line,
   - raw parameter names,
   - end line (filled when the matching `END` is seen),
   - body/end pc (filled during compilation).
4. Track nesting:
   - a second header before the matching `END` is an error,
   - `END SUB`/`END FUNCTION` with no open procedure is an error,
   - kind mismatch (`SUB` closed by `END FUNCTION`) is an error,
   - missing `END` at end of program is an error.
5. Reject duplicate procedure names.
6. Build a line-range map: every line from header through `END` belongs to that
   procedure. This map drives boundary checks and runtime scope checks.

The pre-scan needs only parameter **count** and names for forward calls.
Parameter types and `LOCAL` declarations are finalized when the header/body is
compiled, before the program runs. The VM reads the finalized `Procedure`
metadata at call time.

Open detail: `DEFINT`/`DEFSNG`/`DEFDBL`/`DEFSTR` apply in program order. The
pre-scan should not depend on default types; therefore a `FUNCTION` name must
carry an explicit type suffix. Parameter and `LOCAL` types may use the default
at the point where the header/body is compiled.

### 4.3 Program metadata

Add to `Program`:

```cpp
enum class ProcKind { Sub, Function };

struct ProcParam {
  std::string canonical_name; // e.g. "X%" or "S$"
  Value::Kind kind = Value::Kind::Sgl;
  bool byref = false;         // always false in v1; reserved
};

struct Procedure {
  std::string name;           // canonical name; "FOO" for SUB, "FOO%" for FUNCTION
  ProcKind kind = ProcKind::Sub;
  std::vector<ProcParam> params;
  std::vector<ProcParam> locals; // not including params
  int header_line = 0;
  int end_line = 0;
  int body_pc = -1;           // first instruction after the header jump
  int end_pc = -1;            // SubReturn instruction
  std::string result_name;    // FUNCTION only; same as name
  bool has_code = false;      // for LOCAL placement checks
};
```

`Program` also gets:

- `std::vector<Procedure> procedures_;`
- `std::map<std::string, int> procedure_index_;`
- helper methods:
  - `std::optional<int> ProcedureForLine(int line) const;`
  - `std::optional<int> ProcedureForPc(int pc) const;`
  - `const Procedure* FindProcedure(const std::string& name) const;`

The line-range query can use the existing `LineTable()` and a small binary
search, or a cached `line -> proc` vector built at `Finish()`.

### 4.4 Compiler state

Add to `Compiler`:

- `std::vector<ProcedureDecl> proc_decls_;` (pre-scan)
- `int current_proc_ = -1;` (index into `out_->procedures_`)
- `std::set<std::string> local_names_;` (for duplicate checks)
- `bool proc_has_executable_ = false;`
- `std::map<std::string, int> proc_name_to_index_;`
- `std::vector<Fixup>` extended for procedure-boundary checks; each fixup
  records the `current_proc_` at the time it was emitted.

Name resolution:

- In a procedure, parameter/local names are still emitted as ordinary
  `LoadVar`/`StoreVar` instructions. The VM decides whether the name is local
  by consulting the active scope. This is deliberate: it keeps `INPUT`, `READ`,
  `FOR`, `SWAP`, `LSET`, `FIELD`, `STACK` targets, and future statements working
  without new local-specific opcodes.
- The compiler still needs the declared-name list for:
  - duplicate parameter/local checks,
  - `LOCAL` placement,
  - function result handling,
  - rejecting a procedure name used as a variable,
  - debugger metadata and frame initialization.

### 4.5 Compiling a procedure definition

At a `SUB`/`FUNCTION` header line:

1. Look up the pre-scanned declaration; set `current_proc_`.
2. Resolve parameter names/types with `TypeName`.
3. Emit a placeholder `Jump` over the body.
4. Set `Procedure.body_pc = Pc()`.
5. Compile the body statements until `END SUB`/`END FUNCTION`.
6. `LOCAL` lines register names/types; they emit no runtime code.
7. `EXIT SUB`/`EXIT FUNCTION` emit `SubReturn` with the appropriate result flag.
8. The `END` line emits `SubReturn`.
9. Set `Procedure.end_pc`, `end_line`; patch the header `Jump` to `Pc()`.
10. Clear `current_proc_`.

The header jump makes the definition inert during normal sequential execution.
`CALL` jumps directly to `body_pc`, not to the header line, so the jump is never
re-executed on entry.

`END SUB`/`END FUNCTION` are parsed in the `END` statement branch, alongside
`END SELECT`. `END` alone remains program termination.

### 4.6 Compiling calls

Statement `CALL`:

1. Parse the procedure name.
2. Look it up in `procedure_index_`; undefined is a compile error.
3. Require `ProcKind::Sub`; `CALL` on a function is a compile error.
4. Parse optional parentheses/arguments; emit arguments left to right.
5. Check argument count against the pre-scanned parameter count.
6. Emit `CallProc` with `a = procedure index`, `b = argc`.

Function call in `ParsePrimary`:

1. When an identifier is followed by `(` and its canonical name is a declared
   `FUNCTION`, parse arguments and emit `CallProc` with the function's index.
2. Require exact arguments.
3. A function call always leaves one value on the expression stack.
4. A `FUNCTION` name without `(`:
   - inside its own body, read the result local (`LoadVar` works because the
     frame has a local with that name);
   - outside, compile error "Function name requires arguments".
5. A `FUNCTION` name used in `DIM`, as an array, or as an ordinary assignment
   target outside its own body is a compile error.

`SUB` names are not expressions. A `SUB` name used as a scalar/array is a
compile error.

### 4.7 Boundary checks at compile time

Extend `Fixup` with the emitting procedure:

```cpp
int owner_proc = -1;
```

At `Finish()` or when the target line is known:

- Resolve the target line to `target_proc` via `ProcedureForLine`.
- If `owner_proc != target_proc` and either is a procedure, report
  `GOTO/GOSUB would cross a procedure boundary`, except for `OnError`/`OnTimer`.
- If the target line is the header or `END` line of any procedure, reject it
  even from inside that procedure (the header jump/return is not a legal
  label target).
- `OnError` and `OnTimer` fixups require a main-program target (no procedure)
  in v1; they skip the normal boundary check but still reject a missing line.
  A handler line inside a procedure is a compile error.
- `RESTORE` fixups skip the boundary check.

Computed targets (`GotoIndirect`, `GosubIndirect`, `OnJump`) are checked in the
VM at run time because the line is not known at compile time. The VM uses the
current procedure (or none) and the target line's procedure to apply the same
rule.

### 4.8 Compile errors and messages

Use `kErrSyntax` with specific messages for:

- `SUB without END SUB`
- `FUNCTION without END FUNCTION`
- `END SUB without SUB`
- `END FUNCTION without FUNCTION`
- `SUB inside SUB`
- `Duplicate procedure Name`
- `LOCAL outside a procedure`
- `LOCAL after executable code`
- `Duplicate parameter/local Name`
- `Parameter name collides with CONST Name`
- `CALL requires a SUB`
- `Undefined procedure Name`
- `Name takes N argument(s)`
- `Function name requires arguments`
- `Procedure name used as a variable`
- `GOTO/GOSUB would cross a procedure boundary`
- `GOTO to a procedure header/END line`
- `ON ERROR/ON TIMER handler must be in main`
- `CLEAR is not allowed inside a procedure`
- `DEF FN is not allowed inside a procedure`

For runtime errors, add extension codes (proposed; see §11):

- `78` `Undefined procedure` (if a computed/edge case reaches the VM),
- `79` `Call stack overflow`,
- `80` `Procedure boundary` (computed jump / resume),
- reuse `3` `RETURN without GOSUB` with a detail message for the guarded case,
- reuse `5` `Illegal function call` with a detail message for
  `EXIT SUB would leave a SELECT CASE` and open-`GOSUB` returns.

Update `errors.h`, `errors.cpp`, and `tests/test_errors.cpp`.

---

## 5. VM design

### 5.1 Recommended storage: per-call named scopes

Add a scope stack:

```cpp
struct SubFrame {
  int proc_index = -1;
  int return_pc = 0;
  std::map<std::string, Value> locals; // canonical name -> value
  std::size_t for_depth = 0;
  std::size_t select_depth = 0;
  std::size_t gosub_depth = 0;
  std::uint64_t sequence = 0;          // debugger ordering
};

enum class ScopeKind { Sub, GlobalBarrier };

struct ScopeRef {
  ScopeKind kind = ScopeKind::GlobalBarrier;
  std::size_t sub_index = 0;
};

std::vector<SubFrame> sub_stack_;
std::vector<ScopeRef> scope_stack_;
```

`SubFrame.locals` is pre-populated on call with every parameter and `LOCAL`
name, initialized to the correct default. This means a declared local is always
found; an undeclared name falls through to globals.

### 5.2 `LoadVariable` and `StoreVariable`

Change both to consult only the **top scope**:

```text
LoadVariable(name):
  if scope_stack_ is not empty:
    top = scope_stack_.back()
    if top.kind == Sub:
      frame = sub_stack_[top.sub_index]
      if frame.locals contains canonical(name):
        return frame.locals[canonical(name)]
  return global LoadVariable(name)
```

`StoreVariable` mirrors this: store into the top `Sub` scope if the name is a
declared local; otherwise store the global.

Why only the top scope:

- A callee must not see a caller's locals.
- A `GOSUB` helper inside the same procedure does not push a scope, so it sees
  that procedure's locals — which is correct because the helper is textually
  inside the procedure and cross-boundary `GOSUB` is forbidden.
- A `DEF FN` call pushes a `GlobalBarrier`, so its body sees globals and its
  saved parameters, not the caller's procedure locals.
- A timer handler pushes a `GlobalBarrier`, so it sees globals, not the
  interrupted procedure's locals.
- A main-program `ON ERROR` handler pushes a `GlobalBarrier` when the error
  occurred in a procedure. An error raised in main needs no barrier.
- A timer handler is entered through the existing GOSUB stack, so `GosubFrame`
  must record the scope-stack depth and a barrier flag; `RETURN` pops the
  barrier before returning to the interrupted procedure.
- A `DEF FN` activation must similarly record/pop its `GlobalBarrier` on
  `UserReturn`, including on error paths.

This is the key to making the feature lexical rather than dynamic.

### 5.3 Direct `vars_` accesses to audit and route

Most variable reads/writes already go through `LoadVariable`/`StoreVariable`.
The implementation must fix the direct `vars_` sites so locals work everywhere:

- `Vm::DoForNext` writes `vars_[frame.name]` directly (two places). Use a scoped
  store helper.
- `LSET`/`RSET` writes `vars_[in.name]` directly.
- `FIELD` bindings and random-file `GET`/`PUT` read/write `vars_[field.name]`.
- `SWAP` writes `vars_[item.target->name]`/reads directly.
- `STACK` scalar/array-element targets use direct `vars_` in at least one path.
- `GetVar`/`SetVar` are debugger/global APIs; leave them global and add
  `GetLocal`/`SetLocal` for frame locals.
- `COMMON`/`CHAIN`, `InitConstants`, `ClearState`, and `RUN` are global by
  design; do not route them through scopes.

Add a single `StoreCanonical`/`LoadCanonical` pair that both `LoadVariable` and
the direct sites call, so there is one definition of scope lookup.

### 5.4 `CallProc` and `SubReturn`

New ops:

```cpp
CallProc,  // a = procedure index, b = argc
SubReturn, // a = 1 for function result, 0 for sub
```

`CallProc`:

1. Validate the procedure index and argc.
2. If `sub_stack_.size() + func_stack_.size()` is at the limit, fail with
   `Call stack overflow` before pushing anything.
3. Pop arguments right-to-left into a temporary vector (arguments were pushed
   left-to-right).
4. Create a `SubFrame`:
   - `proc_index`,
   - `return_pc = pc_`,
   - `for_depth/select_depth/gosub_depth`,
   - `locals` initialized for every param/local from `Procedure` metadata,
   - `sequence = ++call_sequence_`.
5. Convert and store each argument into the corresponding parameter local.
6. Push the frame and a `ScopeRef{Sub, index}`.
7. Set `pc_ = procedure.body_pc`.

`SubReturn`:

1. If `sub_stack_` is empty, fail `Internal error` (or `RETURN without GOSUB`
   if reached via a stray `RETURN`).
2. Take the top frame.
3. Check open `SELECT CASE`: if `select_stack_.size() > frame.select_depth`,
   fail `EXIT SUB would leave a SELECT CASE` (function equivalent).
4. Check open `GOSUB`: if `gosub_stack_.size() > frame.gosub_depth`, fail
   `EXIT SUB inside a GOSUB` (function equivalent). This avoids silently
   skipping a `RETURN`.
5. Trim `for_stack_` to `frame.for_depth`.
6. For a function, read the result from `frame.locals[result_name]` before
   destroying the frame.
7. Pop the scope and frame.
8. Set `pc_ = frame.return_pc`.
9. For a function, push the result value.

`EXIT SUB`/`END SUB` and `EXIT FUNCTION`/`END FUNCTION` all use `SubReturn`
with the appropriate flag. A function that falls off `END FUNCTION` without
assigning the result returns the default initialized value.

### 5.5 Recursion and call depth

- The depth limit should count procedure and `DEF FN` activations so a mixed
  recursion cannot exhaust host memory.
- Proposed limit: 256 total activations. This is well above baslib's current
  `blMaxFrames% = 32` and is safe for a host stack.
- Add `Vm::CallDepth()` = `sub_stack_.size() + func_stack_.size()` (plus
  perhaps GOSUB depth for debugger purposes; decide separately).
- On overflow, fail before creating the frame; expression stack cleanup follows
  the normal `Fail` path.

### 5.6 `GOTO`/`GOSUB`/`RETURN` guards

- `Goto`/`Gosub` literal targets are checked at compile time.
- `GotoIndirect`/`GosubIndirect`/`OnJump` check the target line at run time:
  - `target_proc = ProcedureForLine(target_line)`,
  - `current_proc = current procedure from the top Sub scope, or -1`,
  - if both differ and either is a procedure, fail `Procedure boundary`.
- `Return`:
  - If a Sub frame is active and `gosub_stack_.size() <= frame.gosub_depth`,
    fail `RETURN without GOSUB` with a detail message. This prevents a stray
    `RETURN` in a procedure from popping a caller's GOSUB frame.
  - Otherwise behave as today, including FOR-depth trimming.
- `GOSUB` within the same procedure is allowed and increments the GOSUB stack.

### 5.7 Error handling and `RESUME`

**V1 restriction:** handlers are in main; local handlers are phase 2. A handler
therefore always runs with a global scope barrier when a procedure frame is
active.

Add an error-scope marker:

```cpp
bool error_scope_barrier_ = false;
std::size_t error_scope_depth_ = 0; // scope stack depth before the barrier
```

When `Fail` traps to the main handler:

1. If a procedure scope is active, push a `GlobalBarrier` and record
   `error_scope_barrier_ = true`.
2. Jump to the handler with the normal `trap_pending_` mechanism.

`Resume`:

- `RESUME` (retry) / `RESUME NEXT`: pop the error barrier if present; the
  procedure frame stays if the target pc is still in that procedure.
- `RESUME <line>`:
  - Resolve the target procedure.
  - Pop the error barrier.
  - If the target is in the current procedure, keep its frame.
  - If the target is in an outer active procedure, pop inner frames and their
    scopes.
  - If the target is in main, pop all procedure frames/scopes.
  - If the target is in a different, inactive procedure, fail
    `RESUME across a procedure boundary`.
- `ON ERROR GOTO 0` clears the handler and any barrier.

`ON ERROR GOTO`/`ON TIMER` compile-time checks reject a handler line inside a
procedure. This keeps handler scope and frame reconciliation simple in v1.

### 5.8 Snapshots and state reset

Extend `Vm::Snapshot` with:

- `std::vector<SubFrame> sub_stack;`
- `std::vector<ScopeRef> scope_stack;`
- `std::uint64_t call_sequence;`
- error-scope barrier state.

`SaveState`/`RestoreState` copy them. `Load`, `ClearState`, and `Reset` clear
them. `CloseAllFiles` remains independent.

`Debugger` history snapshots then rewind procedure frames and locals correctly.
Add a test that rewinds into a recursive call and continues.

### 5.9 Performance

The named-scope model adds a top-scope check to every variable access and a map
lookup for local names. That is acceptable for a first implementation but must
be measured:

- Benchmark a program with a tight loop calling a sub with several locals
  against the same logic using globals.
- Benchmark baslib-like paint dispatches (many small calls).
- If local access is too slow, the migration path is:
  1. replace the per-frame `std::map` with a `std::vector<Value>` and a
     compile-time slot map (still no syntax change),
  2. or add `LoadLocal`/`StoreLocal` ops as a later optimization.
- Do not add slots to v1 unless measurements justify the extra surface area.
- Frame allocation: reserve/pool `SubFrame` storage if profiling shows
  allocation cost. Avoid `DIM` or string allocation per call.

### 5.10 Rejected alternatives

**Saved globals** (the `DEF FN` model, extended):

- Pros: minimal VM change; all existing ops work.
- Cons: dynamic scoping. A caller's local would be visible to a callee that
  has no local of that name. Example:

  ```basic
  100 SUB A
  110   LOCAL X%
  120   X% = 42
  130   CALL B
  140 END SUB
  200 SUB B
  210   PRINT X%   ' must be 0/global, not 42
  220 END SUB
  ```

  Saved globals would print 42. That violates the lexical goal.

**Hidden slots with new local ops:**

- Pros: fastest, cleanest frame layout, no name maps.
- Cons: every instruction that names a variable must learn whether the name is
  local; `FOR`, `INPUT`, `READ`, `SWAP`, `LSET`, `FIELD`, `STACK`, `DIM`, and
  future statements all need local-aware variants. This is the right end state
  if performance demands it, but it is not the smallest safe first step.

**Compiler name mangling per call:**

- Cons: runtime call identity is not a compile-time constant; recursion makes
  unique names impossible without dynamic rewriting. Rejected.

---

## 6. Debugger and DAP

### 6.1 VM inspection

Add:

- `Vm::CallDepth()` (procedure + `DEF FN` activations; optionally GOSUB).
- `Vm::SubDepth()`.
- `Vm::CurrentLocals()` returning ordered `name -> Value` for the top procedure
  frame, or empty.
- `Vm::GetLocal(name)`, `Vm::SetLocal(name, value)` for the top frame.
- `CallFrame::Kind::Sub` (use one kind for both SUB and FUNCTION, with the
  procedure name and kind in the label).
- A debugger ordering field (`sequence`) on GOSUB, SUB, and FOR frames so
  `CallStack()` can interleave them correctly. Current `CallStack()` lists all
  GOSUB then all FOR; with SUB that ordering is no longer adequate.

`CallFrame` additions:

```cpp
enum class Kind { Gosub, For_, Sub };
std::string name;      // SUB/FUNCTION name, or FOR variable
int line = 0;          // call site/return line for the caller frame
int current_line = 0;  // current body line for the active frame (optional)
```

### 6.2 Stepping

- Step Into: already steps into GOSUB; it steps into `CALL` because the body is
  just instructions.
- Step Over: should treat `CALL` like `GOSUB`. Replace `GosubDepth()` with a
  combined `CallDepth()` or add `SubDepth()` and update
  `Debugger::StepOver()`.
- Step Out: should return from a `SUB`/`FUNCTION` as well as a GOSUB. Use the
  combined depth.
- Step Back/Reverse Continue: snapshots include procedure frames; verify
  rewind into a recursive call.

### 6.3 DAP

- `stackTrace`: add `SUB Name`/`FUNCTION Name` frames with call-site line
  numbers. Keep the top frame's current line.
- `scopes`:
  - Preserve `variablesReference: 1` as the global scope to keep existing DAP
    tests and clients working.
  - Add a new scope for the current procedure's locals (for example
    `variablesReference: 2`), with its own reference range.
  - If this is too disruptive, an alternative is one Locals scope that contains
    frame locals plus globals; that is less clear and is not recommended.
- `variables`: list frame locals with type/value formatting consistent with
  globals; arrays remain global.
- `setVariable`: allow editing a frame local through the local scope reference;
  keep reference 1 editing globals.
- `stackTrace` frame IDs and `scopes` need per-frame mapping, not only the top
  frame. Minimum v1: top frame locals + global; later all frames.
- Add DAP tests for a stopped `CALL`: stack frames, locals, setVariable, step
  over/out, and snapshot.

### 6.4 Source mapping

`INCLUDE` still reports merged line numbers. Procedure frames should report the
merged line for now. Source maps for included files are a separate tooling
project and out of scope here.

---

## 7. Compatibility and migration

### 7.1 Existing programs

- Programs that do not use `SUB`, `FUNCTION`, `LOCAL`, `EXIT`, or `CALL` are
  unaffected except for the new reserved words.
- `GOSUB`/`RETURN`, `DEF FN`, `FOR`, `SELECT`, `ON ERROR`, `TIMER`, `STACK`,
  `INCLUDE`, and `RENUM` keep their current behavior.
- `CALL` changes from `Advanced Feature` to a real procedure call. Programs
  that relied on the refusal now behave differently; this is the intended
  feature.
- `DEF FN` remains expression-bodied and global-scoped. A `DEF FN` called from
  a procedure sees globals, not procedure locals.

### 7.2 baslib

After the language feature is stable, migrate baslib in a separate phase:

1. Add a small proof-of-concept: convert `blDrawText` or `blLayoutChildren` to a
   `SUB` with parameters/locals in a branch/copy.
2. Verify the nested-panel layout bug disappears when the layout cursor and mode
   are locals.
3. Decide whether to replace the `blFramePush`/`blFrameLoad`/`blFrameClose`/
   `blFrameDrop` protocol with direct `CALL`s.
4. Keep the public `blApi...` labels and the host ABI unchanged.
5. Preserve the line-number contract; do not renumber the library in the same
   change.
6. Re-run the baslib test suite and the windowed demos.
7. Update `baslib-prog-ref.md` and `baslib-plan.md` after the migration, not
   before.

The language feature should land and be tested before baslib is rewritten. A
failed baslib refactor must not block the language feature.

### 7.3 Samples and docs

- Add `samples/procedures.bas` or `samples/subs.bas` demonstrating:
  - a sub with a `LOCAL`,
  - a function with a result,
  - recursion,
  - shadowing,
  - `EXIT SUB`,
  - a `FOR` loop inside a procedure.
- Add `RefManual` entries:
  - `SUB Statement`
  - `FUNCTION Statement`
  - `LOCAL Statement`
  - `CALL Statement` (replace the unsupported entry)
  - `EXIT Statement`
  - `END SUB` / `END FUNCTION` cross-references
- Update `RefManual/README.md` index.
- Update `plan.md` / `docs/newbasic-spec-plan.md` status and divergence notes.
- Add a "Procedures and locals" section to the newbasic user documentation, not
  the baslib guide (baslib adoption comes later).

---

## 8. Test plan

### 8.1 New test file

Add `tests/test_subs.cpp` to `tests/CMakeLists.txt`. Use `test_helpers.h`.

Suggested sections:

| Area | Cases |
|---|---|
| Lexing | `SUB`, `FUNCTION`, `LOCAL`, `EXIT` are keywords; `SUBTOTAL`, `FUNCTIONAL`, `LOCALNAME` are names |
| Definition | basic sub, empty sub, basic function, forward call, duplicate name, nested sub, missing END, mismatched END, END without SUB |
| Parameters | zero/one/many, type conversion, string/numeric mismatch, too few/too many, expression arguments, left-to-right side effects |
| Locals | initialized each call, shadow global, restore global, multiple LOCAL lines, duplicate param/local, LOCAL after code, LOCAL outside proc, CONST collision |
| Scope | callee cannot see caller local; callee sees global; GOSUB helper inside same proc sees local; `DEF FN` sees global not local; timer handler sees global (if practical to test) |
| Recursion | factorial/fibonacci, mutual recursion, depth limit, nested calls |
| Return | END SUB, EXIT SUB, END FUNCTION, EXIT FUNCTION, default function result, assignment to result, recursion result |
| Control flow | GOTO within proc, GOTO out, GOTO in, GOSUB within proc, GOSUB out, computed GOTO/GOSUB boundary, IF THEN line boundary, ON ... GOTO/GOSUB boundary |
| FOR/SELECT | FOR loop inside proc, EXIT SUB from FOR, EXIT SUB with open SELECT, GOSUB open at return |
| Errors | ON ERROR/RESUME with a main handler, RESUME NEXT, RESUME main, RESUME different proc, local handler rejected, error with no handler, RETURN in proc |
| Definitions | `DEF FN` inside a procedure rejected; `CLEAR` inside a procedure rejected; `ON TIMER` handler in a procedure rejected |
| Runtime | CALL undefined (should be compile error), CALL function, function as variable, function name as array |
| Snapshots | save/restore inside recursion, rewind into sub, locals restored |
| Debugger | CallStack order, top locals, step over CALL, step out of SUB, step back |
| DAP | stackTrace labels, scopes, variables, setVariable local, step over/out |
| Regression | full existing suite, baslib compiles, DEF FN tests, GOSUB tests, RENUM/INCLUDE tests |

### 8.2 Specific acceptance criteria

- `SUB`/`FUNCTION` definitions compile and are inert when execution passes over
  them.
- A call before the definition works.
- A local defined in a caller is not visible in a callee.
- A recursive function returns the correct value at depth 10 and reaches a
  controlled error at the depth limit.
- `EXIT SUB` from inside a `FOR` discards the loop frame.
- `EXIT SUB` with an open `SELECT` reports a clear error.
- `GOTO` into/out of a procedure is rejected at compile time for literal
  targets and at run time for computed targets.
- `ON ERROR` + `RESUME` inside a procedure works; a global handler sees globals.
- Snapshot/rewind inside a recursive call restores locals and continuation.
- `test_inventory` passes with `CALL` moved out of the stub list.
- All pre-existing tests pass without modification except DAP scope tests that
  are intentionally extended.
- `baslib/demo.bas`, `demo-controls.bas`, and the baslib test suite still pass
  before any baslib refactor.

### 8.3 Performance check

Add a benchmark-style test or a manual benchmark:

- tight loop calling a sub with 4 locals, 1,000,000 iterations;
- same logic with globals;
- report the ratio.

Do not gate the feature on an arbitrary number; record the baseline in the
implementation notes. If the ratio is unacceptable, decide between map
optimization and slots before baslib migration.

### 8.4 File-by-file work map

| File | Work |
|---|---|
| `src/newbasic-core/src/lexer.cpp` | Add `SUB`, `FUNCTION`, `LOCAL`, `EXIT`; keep the keyword table sorted |
| `src/newbasic-core/src/frontend.cpp` | Pre-scan, procedure metadata, headers, `LOCAL`, `CALL`, function calls, `EXIT`, boundary fixups, direct-mode checks |
| `src/newbasic-core/include/newbasic/vm.h` | `Procedure`, `SubFrame`, scope stack, new ops, snapshot fields, `CallFrame::Kind` |
| `src/newbasic-core/src/vm.cpp` | `CallProc`/`SubReturn`, scoped variable access, direct `vars_` routing, return guards, error-scope handling, snapshots |
| `src/newbasic-core/include/newbasic/errors.h`, `src/newbasic-core/src/errors.cpp` | New extension error codes/messages |
| `src/newbasic-debug/src/debugger.cpp` | Call depth, step over/out, locals, stack frames |
| `src/newbasic-debug/include/newbasic/debugger.h` | Locals/stack accessors |
| `src/newbasic-debug/include/newbasic/dap/handler.h` | stackTrace/scopes/variables/setVariable |
| `tests/CMakeLists.txt` | Add `test_subs.cpp` |
| `tests/test_subs.cpp` | New test matrix |
| `tests/test_inventory.cpp` | Move `CALL` from stub; new keyword checks |
| `tests/test_debugger.cpp`, `tests/test_dap_handler.cpp` | Procedure frames, locals, stepping |
| `RefManual/*.md`, `RefManual/README.md` | Entries and index |
| `samples/subs.bas` (new) | Runnable example |
| `plan.md`, `docs/newbasic-spec-plan.md` | Feature status/divergence notes |

---

## 9. Implementation milestones

Each milestone ends with a runnable test and a short note. Do not start a
milestone until the previous gate passes.

### M0 — Design approval

- Confirm the decisions in §11.
- Freeze the v1 scope.
- Add this plan to the repo (this document).
- No code.

**Gate:** user approval.

### M1 — Lexer, metadata, pre-scan, compile-only tests

- Add keywords.
- Add `Procedure` metadata to `Program`.
- Implement `BuildProcedures` and line-range lookup.
- Parse `SUB`/`FUNCTION` headers and `END SUB`/`END FUNCTION`.
- Compile definitions inertly with a header jump and `SubReturn`.
- Compile `CALL` and function calls; check names and argument counts.
- Add compile-error tests.
- Do not execute procedure bodies yet (or make `CallProc` a stub that reports
  `Internal error`); tests focus on compile acceptance/rejection.

**Gate:** definitions and calls compile; errors are clear; no existing test
regressions.

### M2 — VM frames, locals, parameters, recursion

- Add `SubFrame`, scope stack, `CallProc`, `SubReturn`.
- Implement scoped `LoadVariable`/`StoreVariable`.
- Route direct `vars_` sites through scoped accessors.
- Implement parameter conversion and local initialization.
- Implement `EXIT SUB`/`END SUB` and open-FOR cleanup.
- Add tests for parameters, locals, shadowing, recursion, `FOR`, `INPUT`,
  `READ`, `SWAP`, `LSET`, `FIELD`, `STACK` targets.
- Benchmark local access.

**Gate:** subs run; recursion works; locals are correct; no global leakage into
callees.

### M3 — Functions, result, boundaries, errors

- Implement `FUNCTION` call/result and `EXIT FUNCTION`/`END FUNCTION`.
- Implement compile-time boundary checks.
- Implement runtime boundary checks for computed targets.
- Implement `RETURN` guard, open-`SELECT`/open-`GOSUB` checks.
- Implement `ON ERROR`/`RESUME` scope barrier and frame reconciliation.
- Add call-depth limit.
- Add tests for all of the above.

**Gate:** functions, boundaries, and error handling are stable.

### M4 — Debugger, snapshots, DAP

- Add `CallFrame::Kind::Sub` and sequence ordering.
- Add `CallDepth`/`SubDepth`, top-frame locals, `GetLocal`/`SetLocal`.
- Update Step Over/Step Out.
- Extend `Snapshot` and `RestoreState`.
- Update DAP stackTrace/scopes/variables/setVariable.
- Add debugger and DAP tests.

**Gate:** debugger can step into/over/out of a `CALL`, inspect locals, and
rewind through recursion.

### M5 — Documentation, inventory, samples, full regression

- Update `RefManual` and index.
- Add sample(s).
- Update `test_inventory`.
- Update project status docs.
- Run the full suite on Windows and Linux.
- Run baslib demos/tests (no baslib source changes yet).
- Final performance notes.

**Gate:** all tests pass; docs are consistent; no known blocking issues.

### M6 — baslib proof-of-concept and migration (separate)

- Convert one or two baslib internals to `SUB`/`LOCAL` on a branch.
- Verify nested layout and frame reduction.
- Decide whether to migrate the full library.
- Keep the public API and line contract stable.

**Gate:** a deliberate go/no-go on full baslib migration, separate from the
language feature.

---

## 10. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| New reserved words break existing programs | Compatibility | Whole-word lexing limits the damage; document the new words; test names that start with them |
| Scope leaks (caller local visible in callee) | Wrong results | Named scopes with top-only lookup; explicit tests for the A/B example |
| `DEF FN`/timer/error handlers see locals | Wrong results | Push `GlobalBarrier` scopes for those paths |
| Direct `vars_` writes bypass locals | Inconsistent behavior | Audit every `vars_[` site; route through scoped helpers; add tests for FOR/INPUT/READ/SWAP/LSET/FIELD/STACK |
| Computed `GOTO`/`GOSUB` enters a procedure | Corrupt frame | Runtime `ProcedureForLine` check; test computed targets |
| `EXIT SUB` leaves `SELECT`/`GOSUB` open | Corrupt stacks | Entry-depth checks; clear errors; tests |
| `ON ERROR`/`RESUME` across frames | Corrupt frames | Scope barrier + frame reconciliation; keep v1 rules narrow; test |
| Call depth overflow | Host crash | 256-depth limit; fail before pushing frame |
| Snapshot misses frame state | Broken rewind | Include sub/scope stacks and sequence; rewind tests |
| Debugger/DAP changes break existing tests | Regression | Preserve reference 1 for globals; add new local scope; update tests intentionally |
| Performance of named scopes | Slow UI | Benchmark; optimize map/vector; slots as later fallback |
| Function/array name ambiguity | Confusing errors | Require function type suffix; reject procedure names as variables/arrays |
| `CLEAR` inside a procedure | Frame loss | Reject it at compile time; test the rejection |
| `INCLUDE`/`RENUM` interactions | Broken line ranges | Recompute ranges at compile; test include/renum fixtures |
| Large frame maps in snapshots | Memory | Snapshot only active frames; measure history limits |
| `CALL` divergence from GW-BASIC machine `CALL` | Porting confusion | Document `CALL` as a BASIC procedure call; `USR` remains unsupported |

---

## 11. Decisions needed

The recommended defaults are below. The first four are the ones that most affect
the implementation; the rest can be settled during M1.

| # | Decision | Recommended | Alternatives | Why |
|---:|---|---|---|---|
| 1 | Local storage model | Per-call named scopes + scope stack | Saved globals; hidden slots | Lexical, moderate effort, reuses variable instructions |
| 2 | Function naming/call | Any identifier with mandatory type suffix; call `Name(args)`; reject collisions | Require `FN` prefix; `CALL Name(...) RETURN var` | Natural syntax; pre-scan resolves forward calls |
| 3 | Function return | Assign to function name; `EXIT FUNCTION`/`END FUNCTION` | `RETURN expr`; `EXIT FUNCTION(expr)` | Classic BASIC; `RETURN` stays GOSUB-only |
| 4 | `LOCAL` placement | Top of procedure, before executable code | Anywhere/hoisted | Simple, predictable, matches most BASICs |
| 5 | `BYREF`/array params | Defer to phase 2 | Scalar `BYREF` in v1; arrays in v1 | Keeps v1 small; references need lvalue/array work |
| 6 | Cross-procedure `GOTO`/`GOSUB` | Forbid enter/leave; allow within same procedure | Allow `GOSUB` out; runtime only | Prevents missing frames and dynamic scope leaks |
| 7 | Error handling | Handlers in main; global barrier; `RESUME` same proc or outward with frame pops; cross-proc is an error | Local handlers; unwind all frames on trap | Keeps v1 scope/unwinding tractable; local handlers can come later |
| 8 | Call depth | 256 total activations | 1024; configurable; unlimited | Host-safe and above current baslib needs |
| 9 | New error codes | Add 78 `Undefined procedure`, 79 `Call stack overflow`, 80 `Procedure boundary` | Reuse `Illegal function call`/`Out of memory` | Clear diagnostics; extension codes already exist |
| 10 | `DIM` inside procedure | Leave global; local arrays phase 2 | Forbid `DIM` in procedures until local arrays | Avoids silent semantic change; document |
| 11 | Procedure/label namespaces | Separate; reject duplicate procedure names only | Single namespace; reject cross-collisions | `CALL` vs `GOSUB` syntax distinguishes them |
| 12 | Direct mode | Definitions, `LOCAL`, `EXIT`, `CALL` illegal direct | Allow `CALL` to a loaded program | No stored program/frame context in direct mode |
| 13 | `CALL` parentheses | `CALL Name` for zero args; `CALL Name(args)` with args | Always require parentheses | Matches BASIC statement style |
| 14 | `GLOBAL` escape for shadowed names | Not in v1 | Add `GLOBAL name`/`SHARED name` | Locals shadow globals; escape can come later |
| 15 | `CLEAR` inside procedure | Reject `CLEAR` in a procedure; allow it in main | Clear frames and abort; preserve frames | Avoids a frame-less continuation and a confusing partial reset |
| 16 | Procedure name vs label | Separate namespaces | Reject any collision | Keeps `CALL`/`GOSUB` independent |

---

## 12. What I would not do in v1

- Do not add hidden slot opcodes until the named-scope version is measured.
- Do not add `BYREF` at the same time as the core; references interact with
  aliasing, early returns, and arrays.
- Do not add local arrays or local `DIM`; array storage is global today and
  needs a separate design.
- Do not add modules, `PRIVATE`, or qualified names; that is a packaging feature
  and would distract from the calling convention.
- Do not rewrite baslib in the same change.
- Do not change the existing `GOSUB`/`RETURN` semantics for existing programs.
- Do not make all variables local by default; undeclared names stay global.
- Do not add exception-style `TRY`/`CATCH`; `ON ERROR` plus these rules is
  enough for v1.

---

## 13. Deliverables and definition of done

### Code

- [ ] Lexer keyword entries.
- [ ] `Program::Procedure` metadata and line-range queries.
- [ ] `BuildProcedures` pre-scan.
- [ ] Compile `SUB`/`FUNCTION`/`LOCAL`/`CALL`/`EXIT`/`END SUB`/`END FUNCTION`.
- [ ] `CallProc`/`SubReturn` ops.
- [ ] Scoped `LoadVariable`/`StoreVariable` + direct `vars_` audit.
- [ ] Boundary checks (compile and runtime).
- [ ] Function results.
- [ ] `ON ERROR`/`RESUME` scope handling.
- [ ] Call-depth limit.
- [ ] Snapshot/restore.
- [ ] Debugger/DAP support.

### Tests

- [ ] `tests/test_subs.cpp` with the matrix in §8.
- [ ] Updated `test_inventory.cpp`, `test_lexer.cpp`, `test_names.cpp`,
      `test_errors.cpp`, debugger/DAP tests.
- [ ] Full suite green on Windows and Linux.
- [ ] baslib suite and demos green before baslib migration.

### Documentation

- [ ] `RefManual` entries and index.
- [ ] `samples/subs.bas` (or equivalent).
- [ ] Project plan/status note.
- [ ] No baslib guide rewrite until the language feature is stable.

### Definition of done

A program can define a `SUB` and a `FUNCTION`, call them with parameters, use
`LOCAL` variables, recurse, exit early, and observe lexical scoping; the
debugger can show the call stack and locals; snapshots rewind correctly; and all
existing tests and baslib demos still pass.

---

## 14. Immediate next step after approval

1. Record the chosen decisions in this file.
2. Create `tests/test_subs.cpp` with the compile-error cases from M1.
3. Implement M1 only: keywords, metadata, pre-scan, definitions, calls, and
   compile-time checks.
4. Stop at the M1 gate for review before touching VM execution.

No implementation begins until the decision table is answered.
