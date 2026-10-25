/* Test helper: inject pointer clicks and raw keys via input.device.
 * uaeinput click X Y      absolute screen position, left click
 * uaeinput key HEX...     raw key codes (down+up)                   */
#include <stdlib.h>
#include <string.h>
#include <exec/io.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/dos.h>

static struct IOStdReq *io;

static void send(UBYTE cls, UWORD code, WORD x, WORD y)
{
    struct InputEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.ie_Class = cls;
    ev.ie_Code = code;
    ev.ie_X = x;
    ev.ie_Y = y;
    io->io_Command = IND_WRITEEVENT;
    io->io_Data = &ev;
    io->io_Length = sizeof(ev);
    DoIO((struct IORequest *)io);
}

int main(int argc, char **argv)
{
    struct MsgPort *port;
    int i;
    if (argc < 2) return 10;
    port = CreateMsgPort();
    io = port ? (struct IOStdReq *)CreateIORequest(port, sizeof(*io)) : NULL;
    if (!io || OpenDevice((STRPTR)"input.device", 0, (struct IORequest *)io, 0)) return 20;
    if (!strcmp(argv[1], "click") && argc >= 4) {
        WORD x = atoi(argv[2]), y = atoi(argv[3]);
        send(IECLASS_POINTERPOS, IECODE_NOBUTTON, x, y);
        Delay(5);
        send(IECLASS_RAWMOUSE, IECODE_LBUTTON, 0, 0);
        Delay(3);
        send(IECLASS_RAWMOUSE, IECODE_LBUTTON | IECODE_UP_PREFIX, 0, 0);
    } else if (!strcmp(argv[1], "key")) {
        for (i = 2; i < argc; ++i) {
            UWORD code = (UWORD)strtol(argv[i], NULL, 16);
            send(IECLASS_RAWKEY, code, 0, 0);
            Delay(2);
            send(IECLASS_RAWKEY, code | IECODE_UP_PREFIX, 0, 0);
            Delay(2);
        }
    }
    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    return 0;
}
