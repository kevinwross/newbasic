# U

<a id="unlock-statement"></a>
## UNLOCK Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Unlocks a range of records previously locked with `LOCK`.

### Syntax

```basic
UNLOCK [#file number] [, [record] [TO record]]
```

### Newbasic status

Not implemented. Newbasic has no record-locking model.

<a id="usr-function"></a>
## USR Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Calls a machine-language routine defined by `DEF USR`.

### Syntax

```basic
USR[n](argument)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no machine-code interface.
