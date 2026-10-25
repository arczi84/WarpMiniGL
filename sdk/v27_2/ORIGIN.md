# V27_2 SDK provenance

include/ is an unchanged copy of D:\dev\Pistorm3D\include_v27_2\include.
MGLCreateContextFromBitMap is appended after the V27 window constructor.
The renderer borrows a CGX-lockable four-byte-per-pixel bitmap; MGLSwitchDisplay
finishes rendering without presentation. No retarget API is published.
client/ uses the existing single-LVO import and checks table size and required
function pointers. Public-only generated headers keep GLcontext opaque.
