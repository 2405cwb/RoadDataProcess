# Errors

Command failures and integration errors.

---

## [ERR-20260616-001] csharp_wrapper_compile_compat

**Logged**: 2026-06-16T15:48:00+08:00
**Priority**: medium
**Status**: resolved
**Area**: backend

### Summary
PowerShell Add-Type used an older C# compiler that rejected C# string interpolation in the PackSdk wrapper.

### Error
```text
Unexpected character "$" at the string interpolation expression in PackReader.cs.
Cannot convert null to System.IntPtr when compiling Pack_ExportAll wrapper call.
```

### Context
- Attempted to compile `code/PackSdk/csharp/PackReader.cs` with `Add-Type -Path`.
- The wrapper used `$"PackSdk failed: {errorCode}"`, which is not accepted by the local compiler.

### Suggested Fix
Keep SDK wrapper syntax compatible with older C# compilers used by Windows PowerShell and legacy customer tooling. Use `IntPtr.Zero` instead of `null` for native pointer arguments.

### Metadata
- Reproducible: yes
- Related Files: code/PackSdk/csharp/PackReader.cs

### Resolution
- **Resolved**: 2026-06-16T15:49:00+08:00
- **Notes**: Replaced string interpolation, get-only auto-property syntax, and null pointer arguments with older C# compatible forms.

---
