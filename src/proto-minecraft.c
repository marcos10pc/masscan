#include "proto-minecraft.h"
#include "unusedparm.h"

#define STATE_READ_PACKETLEN  1
#define STATE_READ_PACKET_ID  2
#define STATE_READ_DATALEN    3
#define STATE_READ_DATA       4

static const char minecraft_hello[] = {
    0x11, // length
    0x00, // packet id
    0xff, 0xff, 0xff, 0xff, 0x0f, // protocol version number (-1)
    0x07, 0x6d, 0x61, 0x73, 0x73, 0x63, 0x61, 0x6e, /// hostname (masscan)
    0x00 0x0, // port (0)
    0x01, // next state (status)

    0x01, // length
    0x00 // packet id (request status)
};

static void minecraft_parse(const struct Banner1 *banner1,
                            void *banner1_private,
                            struct ProtocolState *stream_state,
                            const unsigned char *px, size_t length,
                            struct BannerOutput *banout,
                            struct InteractiveData *more) {

    unsigned state = stream_state->state; // assuming this starts out at zero

    UNUSEDPARM(banner1_private);
    UNUSEDPARM(banner1);

    // beware: the `bytes_read` field is reused
    for(size_t i = 0; i < length; i++) {
        switch(state) {
            
            // initial: set up some stuff
            case 0:
                state = STATE_READ_PACKETLEN;
                // !!! fall through !!!

            // read varint
            // reuse same code for both varints
            case STATE_READ_PACKETLEN:
            case STATE_READ_DATALEN:
                // MSB indicates whether there are more bytes in the varint
                // varints shouldn't be longer than 5 bytes, but we don't enforce this restriction
                if(!(px[i] & 0x80)) {
                    if(state == STATE_READ_PACKETLEN)
                        state = STATE_READ_PACKET_ID;
                    else
                        state = STATE_READ_DATA;
                }
                break;

            case STATE_READ_PACKET_ID:
                if(px[i] == 0x00) {
                    state = STATE_READ_DATALEN;
                } else {
                    state = 0xffffffff;
                }
                break;

            case STATE_READ_DATA:
                banout_append_char(banout, PROTO_MINECRAFT, px[i]);
                break;
                
            // skip to the end if something went wrong
            default:
                i = (unsigned)length;

        }
    }

    stream_state->state = state;

}

static void *minecraft_init(struct Banner1 *banner1) {
    UNUSEDPARM(banner1);
    return 0;
}

// TODO
static int minecraft_selftest() {
    return 0; // TODO
}

const struct ProtocolParserStream banner_minecraft = {
    "minecraft", 25565, minecraft_hello, sizeof(minecraft_hello), 0, 
    minecraft_selftest,
    minecraft_init,
    minecraft_parse
};
