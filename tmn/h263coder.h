#ifdef __cplusplus
extern "C" {
#endif
#include "sim.h"
    
struct ENCODER_STATE {
    PictImage      *prev_image;
    PictImage      *curr_image;
    PictImage      *curr_recon;
    PictImage      *prev_recon;

    /* To be used for temporal scalability. */
    PictImage      *next_P_image;
    PictImage      *next_P_recon;
    PictImage      *prev_P_image;
    PictImage      *prev_P_recon;

    /* To be used for SNR/spatial scalability. */
    PictImage      *prev_enhancement_image;
    PictImage      *prev_enhancement_recon;
    PictImage      *curr_reference_recon;
    PictImage      *curr_enhancement_recon;
    DiffImage      *diff_image;
    DiffImage      *diff_recon;

    /* PB-frame specific */
    PictImage      *B_recon;
    PictImage      *B_image;

    Pict           *pic;
    unsigned char  *image;

    int             prev_I_P_quant;

    int             fixed_frameskip, True_B_frameskip, P_frameskip;

    int             TR_new, TR_old;

    float           DelayBetweenFramesInSeconds;

    PictImage      *stored_image;
};

struct ENCODER_STATE *h263_init_encoder_1();
void h263_init_encoder_2(struct ENCODER_STATE *s);
int h263_encode_one_frame(struct ENCODER_STATE *s, int i, int tr);
void h263_cleanup(struct ENCODER_STATE *s);

#ifdef __cplusplus
}
#endif

