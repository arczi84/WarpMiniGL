"""Public-only view of supplied V27 headers; never define private GLcontext.

The supplied gl.h pulls in renderer-private backend headers missing from the
include distribution. Dispatch clients need its exact types/enums and callback
declarations, but neither the renderer layout nor its inline implementation.
The dispatch table itself is included unchanged from sdk/v27/include.
"""
from pathlib import Path
import sys

root = Path(__file__).resolve().parent.parent
sdk = root / 'sdk' / (sys.argv[2] if len(sys.argv) > 2 else 'v27') / 'include/mgl'
out = Path(sys.argv[1]) / 'mgl'
out.mkdir(parents=True, exist_ok=True)
gl = (sdk / 'gl.h').read_text()
gl = gl[:gl.index('#include "mgl/vertexbuffer.h"')]
gl += '\n#include "mgl/context.h"\n#include "mgl/modes.h"\n'
gl += '\n#ifdef __cplusplus\n}\n#endif\n#endif /* GL_H_ */\n'
context = (sdk / 'context.h').read_text()
context = context[context.index('typedef struct GLcontext_t * GLcontext;'):
                  context.index('typedef void (*IdleFn)(void);') + len('typedef void (*IdleFn)(void);')]
context = '#ifndef WM_V27_CLIENT_CONTEXT_H\n#define WM_V27_CLIENT_CONTEXT_H\nstruct Window;\nstruct BitMap;\n' + context + '\n#endif\n'
(out / 'gl.h').write_text(gl)
(out / 'context.h').write_text(context)
