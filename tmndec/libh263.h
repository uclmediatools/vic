#ifdef __cplusplus
extern "C" {
#endif

struct H263_STATE {
    int            width,height;
    unsigned char  **frame;

    /* internal state */
    int            framenum;
    int            conf;
    char           conf_data[512];
};

struct H263_STATE* h263_start();
void h263_stop(struct H263_STATE *state);

int h263_decode_frame(struct H263_STATE *state, char *packet);

#ifdef __cplusplus
}
#endif
