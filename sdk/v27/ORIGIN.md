# V27 client SDK provenance

`include/` is an unchanged copy of the user-supplied
`D:\dev\Pistorm3D\includev27\include` (20 September 2026).
The published MGLCreateContextFromWindow entry is appended after GLReadBuffer.
The SDK still declares ABI 3; clients check structSize before reading that slot.

`client/` reuses the existing V23 single-LVO import client; minigl_open.c additionally
checks the borrowed-window function/current-context pointers and refuses Classic
or stub backends for WINDOW mode. It does not guess an experimental V18 offset.

The supplied gl.h/context.h include private V3D backend headers not present in
this include-only distribution. gcc/generate_v27_client.py extracts the exact
public GL types/enums and callback declarations into a build-local include view.
GLcontext remains an opaque pointer; no private GLcontext layout is compiled or
accessed. The complete dispatch table is taken unchanged from this SDK.
