#include "streamcodec.h"
#include "galois.h"
#include <assert.h>
#define WIN_ALLOC   10000

// pseudo-random number generator
extern void mt19937_seed(unsigned long s, unsigned long *mt);
extern unsigned long mt19937_randint(unsigned long *mt, int *mti);

static int  n_repair;       // number of repair packets received during decoder activation
static long n_row_sub_ops;  // number of row operations during subtraction, reset once decoder is deactivated

static long n_row_ops;  // number of row operations during subtraction, reset once decoder is deactivated
static long n_row_multiply_add;  // number of row operations during subtraction, reset once decoder is deactivated
static long convert_diagonal_to_1;

static long n_row_ops_A;
static long n_row_ops_inactive;

static long n_row_multiply_add_A;
static long n_row_multiply_add_inactive;

static long multiply_add_A_R;
static long multiply_add_A_L;
static long multiply_add_A_N;

static long zero_num;

static long Sum_A_L_Interval;
static long LastLost;

struct decoder *initialize_decoder(struct parameters *cp)
{
    struct decoder *dc = calloc(1, sizeof(struct decoder));
    dc->active = 0;
    dc->cp = cp;
    dc->inorder = -1;
    dc->win_s = -1;
    dc->win_e = -1;
    dc->dof = 0;
    dc->row = calloc(WIN_ALLOC, sizeof(ROW_VEC*));
    dc->message = calloc(WIN_ALLOC, sizeof(GF_ELEMENT*));
    dc->recovered = calloc(DEC_ALLOC, sizeof(GF_ELEMENT*));
    dc->prev_rep = -1;
    constructField(cp->gfpower);
    // allocate a single-packet buffer for packet desearilization
    int pktsize = dc->cp->pktsize;
    dc->pbuf = calloc(1, sizeof(struct packet));
    dc->pbuf->coes = NULL;
    dc->pbuf->syms = calloc(pktsize, sizeof(GF_ELEMENT));
    return dc;
}

struct packet *deserialize_packet(struct decoder *dc, unsigned char *pktstr)
{
    struct packet *pkt = dc->pbuf;
    memcpy(&pkt->sourceid, pktstr, sizeof(int));
    memcpy(&pkt->repairid, pktstr+sizeof(int), sizeof(int));
    memcpy(&pkt->win_s, pktstr+sizeof(int)*2, sizeof(int));
    memcpy(&pkt->win_e, pktstr+sizeof(int)*3, sizeof(int));
    memcpy(pkt->syms, pktstr+sizeof(int)*4, dc->cp->pktsize);
    if (pkt->sourceid == -1) {
        int curr_rep = pkt->repairid;
        int i, j;
        // recover coding coefficients using PRNG
        int width = pkt->win_e - pkt->win_s + 1;
        if (pkt->coes != NULL) {
            free(pkt->coes);
            pkt->coes = NULL;
        }
        pkt->coes = calloc(width, sizeof(GF_ELEMENT));
        // init prng using packet's repairid to synchronize encoding vector
        dc->prng.mti = N;
        mt19937_seed(pkt->repairid*EWIN, dc->prng.mt);
        for (j=0; j<width; j++) {
            GF_ELEMENT co = mt19937_randint(dc->prng.mt, &dc->prng.mti) % (1 << dc->cp->gfpower);
            pkt->coes[j] = co;
        }
    }
    return pkt;
}

int receive_packet(struct decoder *dc, struct packet *pkt)
{
    if (pkt->sourceid == -1) {
        dc->prev_rep = pkt->repairid;
    }
    int pktsize = dc->cp->pktsize;
    if (!dc->active) {
        // Decoder is inactive, in-order receiving
        if (pkt->sourceid >= 0) {
            // A source packet received, check whether it's consistent
            if (pkt->sourceid <= dc->inorder) {
                DEBUG_PRINT(("[Decoder] Receive out-dated source packet %d current inorder: %d\n", pkt->sourceid, dc->inorder));
                return 0;
            }
            if (pkt->sourceid == dc->inorder + 1) {
                // in-order delievery, store it
                if (dc->recovered[pkt->sourceid % DEC_ALLOC] != NULL) {
                    memset(dc->recovered[pkt->sourceid % DEC_ALLOC], 0, pktsize*sizeof(GF_ELEMENT));     // overwrite (i.e., "flush") previous older source packets
                } else {
                    dc->recovered[pkt->sourceid % DEC_ALLOC] = calloc(pktsize, sizeof(GF_ELEMENT));
                }
                memcpy(dc->recovered[pkt->sourceid % DEC_ALLOC], pkt->syms, pktsize*sizeof(GF_ELEMENT));
                DEBUG_PRINT(("[Decoder] Receive in-order source packet %d \n", pkt->sourceid));
                // printf("[Decoder] delay of source packet %d : 0 \n", pkt->sourceid);
                dc->inorder += 1;
                return 0;
            } else {
                // missing source packet
                DEBUG_PRINT(("[Decoder] Receives source packet %d but in-order is %d, activating decoder...\n", pkt->sourceid, dc->inorder));
                activate_decoder(dc, pkt);
                return 0;
            }
        } else {
            // A repair packet received 
            if (pkt->win_e <= dc->inorder) {
                // encoded from all the recovered packets: i.e., still in-order, just ignore
                DEBUG_PRINT(("[Decoder] Receives repair packet coded across [%d, %d], in-order is %d, just ignore...\n", pkt->win_s, pkt->win_e, dc->inorder));
                return 0;
            } else {
                // it contains some missing source packets, activate decoder
                DEBUG_PRINT(("[Decoder] Receives repair packet coded across [%d, %d], but in-order is %d, activating decoder...\n", pkt->win_s, pkt->win_e, dc->inorder));
                activate_decoder(dc, pkt);
                return 0;
            }
        }
    } else {
        // Decoder is active, i.e., in-order reception halted, so perform on-the-fly GE
        process_packet(dc, pkt);
        if (dc->dof == (dc->win_e - dc->win_s + 1)) {
            // ready to recover the whole window
            deactivate_decoder(dc);
        }
        return 0;
    }
    return 0;
}

int activate_decoder(struct decoder *dc, struct packet *pkt)
{
    n_repair = 0;
    n_row_sub_ops = 0;
    n_row_ops=0;
    n_row_multiply_add=0;
    convert_diagonal_to_1=0;
    
    n_row_ops_A=0;
    n_row_ops_inactive=0;
    n_row_multiply_add_A=0;
    n_row_multiply_add_inactive=0;
    
    multiply_add_A_L=0;
    multiply_add_A_R=0;
    multiply_add_A_N=0;

    zero_num=0;

    Sum_A_L_Interval=0;

    LastLost=0;

    assert(dc->win_e-dc->win_s+1 <= DEC_ALLOC);
    int pktsize = dc->cp->pktsize;
    if (pkt->sourceid >= 0) {
        // A source packet triggered decoder activation
        // Packets of inorder+1, inorder+2,..., sourceid-1 are missing
        dc->win_s = dc->inorder + 1;
        dc->win_e = pkt->sourceid;
        // save the packet to A*S=C
        int index = pkt->sourceid - dc->win_s;
        //assert(dc->row[index] == NULL);
        dc->row[index] = calloc(1, sizeof(ROW_VEC));
        dc->row[index]->len = 1;
        dc->row[index]->elem = calloc(dc->row[index]->len, sizeof(GF_ELEMENT));
        dc->row[index]->elem[0] = 1;
        dc->message[index] = calloc(pktsize, sizeof(GF_ELEMENT));
        memcpy(dc->message[index], pkt->syms, pktsize * sizeof(GF_ELEMENT));
        dc->dof = 1;
        DEBUG_PRINT(("[Decoder] Decoder activated by source packet %d \n", pkt->sourceid));
    } else {
        n_repair += 1;
        dc->win_s = dc->inorder + 1;
        dc->win_e = pkt->win_e;
        dc->dof = 0;
        // TODO: If some encoded source packets have been "flushed" from the decoder, the repair packet cannot be used
        
        // process the repair packet to eliminate those already in-order recovered packets from the packet
        // and then save to A*S=C
        for (int i=pkt->win_s; i<=dc->inorder; i++) {
            GF_ELEMENT co = pkt->coes[i-pkt->win_s];
            galois_multiply_add_region(pkt->syms, dc->recovered[i % DEC_ALLOC], co, pktsize);
            pkt->coes[i-pkt->win_s] = 0;
        }
        int offset = dc->win_s - pkt->win_s;
        GF_ELEMENT *coes = &pkt->coes[offset];    // coefficients corresponding to the rest of the cofficients
        int dww = dc->win_e - dc->win_s + 1;
        for (int i=dww-1; i>=0; i--) { //找到最后一个不为0的编码系数
            if (coes[i] != 0) {
                // save the coefficient vector and the corresponding message row to A*S=C
                dc->row[i] = calloc(1, sizeof(ROW_VEC));
                dc->row[i]->len = i + 1;
                dc->row[i]->elem = calloc(dc->row[i]->len, sizeof(GF_ELEMENT));
                memcpy(dc->row[i]->elem, &coes[0], dc->row[i]->len*sizeof(GF_ELEMENT));
                dc->message[i] = calloc(pktsize, sizeof(GF_ELEMENT));
                memcpy(dc->message[i], pkt->syms, pktsize * sizeof(GF_ELEMENT));
                dc->dof = 1;
                break;
            }
        }
        DEBUG_PRINT(("[Decoder] Decoder activated by repair packet %d with encoding window [ %d %d ] \n", pkt->repairid, pkt->win_s, pkt->win_e));
    }
    DEBUG_PRINT(("[Decoder] Decoder activated with decoding window [%d %d], DoF: %d\n", dc->win_s, dc->win_e, dc->dof));
    dc->active = 1;
    // In case decoding window lenght is only 1 and dof is 1 after activated,
    // decoding can complete immediately
    if (dc->dof == (dc->win_e - dc->win_s + 1)) {
        deactivate_decoder(dc);
    }
    return 0;
}

int process_packet(struct decoder *dc, struct packet *pkt)
{
    int n_row_ops_sub =0;

    int pktsize = dc->cp->pktsize;
    if ((pkt->sourceid >= 0 && pkt->sourceid < dc->win_s) || (pkt->repairid>=0 && pkt->win_e < dc->win_s)) {
        // Out-dated packets are discarded directly
        return 0;
    }
    if (pkt->sourceid >= 0 && pkt->sourceid > dc->win_e) {
        // A newer source packet beyond the current decoding window is received
        int index = pkt->sourceid - dc->win_s;
        dc->row[index] = calloc(1, sizeof(ROW_VEC));
        dc->row[index]->len = 1;
        dc->row[index]->elem = calloc(dc->row[index]->len, sizeof(GF_ELEMENT));
        dc->row[index]->elem[0] = 1;
        dc->message[index] = calloc(pktsize, sizeof(GF_ELEMENT));
        memcpy(dc->message[index], pkt->syms, pktsize * sizeof(GF_ELEMENT));
        dc->dof += 1;
        dc->win_e = pkt->sourceid;  // expand decoding window
        DEBUG_PRINT(("[Decoder] Processed source packet %d , decoding window [ %d %d ], DoF: %d\n", pkt->sourceid, dc->win_s, dc->win_e, dc->dof));
        return 0;
    }
    // An out-of-order source packet within the decoding window or a repair packet arrives
    if (pkt->repairid>=0) {
        // TODO: If some encoded source packets have been "flushed" from the decoder, the repair packet cannot be used

        // first of all, eliminate those already in-order recovered packets from the repair packets
        n_repair += 1;
        for (int i=pkt->win_s; i<=dc->inorder; i++) {
            GF_ELEMENT co = pkt->coes[i-pkt->win_s];
            galois_multiply_add_region(pkt->syms, dc->recovered[i % DEC_ALLOC], co, pktsize);
            pkt->coes[i-pkt->win_s] = 0;
            //n_row_sub_ops += 1;
        }
        if (pkt->win_e >= dc->win_e) {
            dc->win_e = pkt->win_e;  // expand decoding window if the repair packet covers newer source packets beyond the current window
        }
    }
    int dww = dc->win_e - dc->win_s + 1;
    // Find the index of the column from which the received source or repair packet gonna affect
    // as well as the offset of the coefficients in repair's encoding vector having the effect
    //      dc->win_s         dc->win_e
    //         |                 |
    //         v                 v
    // ........[.................]........
    // ...xxxxxxxxxxxx....
    // .............xxxxxxxxx.............
    // .................xxxxxxxxxx........
    //    ^         ^   ^
    //    |         |   |
    //   (a)       (b) (c)
    // Cases of repair packet's coeffcients (xxx's):
    int index, offset;
    if (pkt->sourceid>=0) {
        index = pkt->sourceid - dc->win_s;
        DEBUG_PRINT(("[Decoder] Processing \"lost\" source packet %d ...\n", pkt->sourceid));
    } else {
        index = pkt->win_s > dc->win_s ? (pkt->win_s - dc->win_s) : 0;
        offset = pkt->win_s > dc->win_s ? 0 : (dc->win_s - pkt->win_s);
    }
    // The effective encoding vector of the receive packet,length: (dww - index)
    int width = dww - index;
    GF_ELEMENT *coes = calloc(width, sizeof(GF_ELEMENT));
    if (pkt->sourceid>=0) {
        coes[0] = 1;
    } else {
        int ntocopy = (pkt->win_e - pkt->win_s + 1) - offset;
        memcpy(coes, &pkt->coes[offset], ntocopy * sizeof(GF_ELEMENT));
    }

    //aligned_coe与aligned_row是为了方便对齐编码系数向量而产生的
    GF_ELEMENT *aligned_coe = calloc(dww,sizeof(GF_ELEMENT));   
    memcpy(&aligned_coe[dww - width],coes,width*sizeof(GF_ELEMENT));
    
    // Process the effective EV to the appropriate row
    GF_ELEMENT quotient;
    int filled = -1;
    int CC;
    for (int i=dww-1; i>=0; i--) {
        if (aligned_coe[i] != 0) {
            if (dc->row[i] != NULL) {
                quotient = galois_divide(aligned_coe[i], dc->row[i]->elem[dc->row[i]->len - 1]);
                CC = dww-i < dc->row[i]->len ? dww-i:dc->row[i]->len; //CC为公共部分长度
                GF_ELEMENT *aligned_row = calloc(i+1,sizeof(GF_ELEMENT)); 
                memcpy(&aligned_row[i+1 - dc->row[i]->len],dc->row[i]->elem,dc->row[i]->len*sizeof(GF_ELEMENT));
                galois_multiply_add_region(aligned_coe, aligned_row, quotient,i + 1); //改动
                galois_multiply_add_region(pkt->syms, dc->message[i], quotient, pktsize);
                free(aligned_row);
                

                int RowWidth=dc->row[i+index]->len + pktsize;
                n_row_multiply_add=n_row_multiply_add+RowWidth;
                n_row_multiply_add_A=n_row_multiply_add_A+RowWidth;

                multiply_add_A_L = multiply_add_A_L+pktsize+1;
                if (multiply_add_A_R != multiply_add_A_R+RowWidth-pktsize-1){
                    DEBUG_PRINT(("[R] multiply_add_A_N: %d \n",RowWidth-pktsize-1));
                    multiply_add_A_N+=1;
                }
                multiply_add_A_R = multiply_add_A_R+RowWidth-pktsize-1;                

                n_row_ops = n_row_ops+1;
                n_row_ops_A=n_row_ops_A+1;
                n_row_ops_sub=n_row_ops_sub+1;
                DEBUG_PRINT(("[N_ROWOPTION], currentRow: %d , n_row_ops: %d RowWidth: %d\n",i+index, n_row_ops_sub,RowWidth));
            }else {
                Sum_A_L_Interval = i+index;
                DEBUG_PRINT(("[L] currentRow: %ld \n",i+index-LastLost));
                dc->row[i] = calloc(1, sizeof(ROW_VEC));
                dc->row[i]->len = i + 1;
                dc->row[i]->elem = calloc(dc->row[i]->len, sizeof(GF_ELEMENT));
                memcpy(dc->row[i]->elem, aligned_coe, (i + 1) * sizeof(GF_ELEMENT));
                dc->message[i] = calloc(pktsize, sizeof(GF_ELEMENT));
                memcpy(dc->message[i], pkt->syms, pktsize * sizeof(GF_ELEMENT));
                dc->dof += 1;
                filled = i;

                for (int j2=0;j2<(i + 1);j2++){
                    if (aligned_coe[j2] ==0){
                        zero_num = zero_num + 1;
                    }
                }

                break;
            }
        }else{
            zero_num =zero_num+1;
        }
    }
    free(coes);
    free(aligned_coe);
    if (pkt->sourceid >= 0) {
        DEBUG_PRINT(("[Decoder] Processed source packet %d , current decoding window: [ %d %d ], filled: %d DoF: %d\n", pkt->sourceid, dc->win_s, dc->win_e, filled+dc->win_s, dc->dof));
    } else {
        DEBUG_PRINT(("[Decoder] Processed repair packet %d , encoding window [%d, %d], current decoding window: [ %d %d ], filled: %d DoF: %d\n", pkt->repairid, pkt->win_s, pkt->win_e, dc->win_s, dc->win_e, filled+dc->win_s, dc->dof));
        DEBUG_PRINT(("current decoding window length: %d\n",dc->win_e - dc->win_s + 1));
        //DEBUG_PRINT(("tatal_length_coes: %ld\n",tatal_length_coes));
    }
    return 0;
}

int deactivate_decoder(struct decoder *dc)
{
    int pktsize = dc->cp->pktsize;
    int win_s = dc->win_s;
    int win_e = dc->win_e;
    int width = win_e - win_s + 1;
    // Eliminate nonzeros above diagonal elements
    int i, j, b ;
    int len;
    GF_ELEMENT quotient;
    // eliminate all nonzeros above diagonal elements from right to left
    for (i=0; i<width; i++) {//从第一列开始寻找非零非空的元素
        for (j=i+1; j<width; j++) {
            len = dc->row[j]->len;
            if (j>=i+len || dc->row[j]->elem[i] == 0)
                continue;
            assert(dc->row[i]->elem[dc->row[i]->len - 1]);
            quotient = galois_divide(dc->row[j]->elem[i], dc->row[i]->elem[dc->row[i]->len - 1]);
            galois_multiply_add_region(dc->message[j], dc->message[i], quotient, pktsize);
            dc->row[j]->elem[i] = 0;
            DEBUG_PRINT(("[N_ROWOPTION], InActive current[ %d , %d ] n_row_ops: %d multiply_add: %d\n",i,j,1,pktsize));

            n_row_ops = n_row_ops+1;
            n_row_ops_inactive=n_row_ops_inactive+1;
            n_row_multiply_add = n_row_multiply_add+pktsize;
            n_row_multiply_add_inactive = n_row_multiply_add_inactive+pktsize;

        }
        // convert diagonal to 1
        if (dc->row[i]->elem[dc->row[i]->len - 1] != 1) {
            galois_multiply_region(dc->message[i], galois_divide(1, dc->row[i]->elem[dc->row[i]->len - 1]), pktsize);
            dc->row[i]->elem[dc->row[i]->len - 1] = 1;

            convert_diagonal_to_1=convert_diagonal_to_1+1;

        }
        // save recovered packet
        int sourceid = win_s + i;
        if (dc->recovered[sourceid % DEC_ALLOC] != NULL) {
            memset(dc->recovered[sourceid % DEC_ALLOC], 0, pktsize*sizeof(GF_ELEMENT));     // overwrite (i.e., "flush") previous older source packets
        } else {
            dc->recovered[sourceid % DEC_ALLOC] = calloc(pktsize, sizeof(GF_ELEMENT));
        }
        memcpy(dc->recovered[sourceid % DEC_ALLOC], dc->message[i], pktsize*sizeof(GF_ELEMENT));
        free(dc->message[i]);
        dc->message[i] = NULL;
        free(dc->row[i]->elem);
        dc->row[i]->elem = NULL;
        free(dc->row[i]);
        dc->row[i] = NULL;
    }
    // Deactivate
    DEBUG_PRINT(("[Decoder] Inactivating decoder with DW window [%d, %d] of width: %d new in-order: %d n_repair: %d n_sub_row_ops: %ld\n", dc->win_s, dc->win_e, width, win_e, n_repair, n_row_sub_ops));
    DEBUG_PRINT(("[ROWOPTION] Inactivating decoder with DW window [%d, %d] of width: %d new in-order: %d n_repair: %d n_row_ops: %ld n_row_multiply_add: %ld convert_diagonal_to_1: %ld n_row_ops_A: %ld n_row_ops_inactive: %ld n_row_multiply_add_A: %ld n_row_multiply_add_inactive: %ld zero_num: %ld \n", dc->win_s, dc->win_e, width, win_e, n_repair, n_row_ops,n_row_multiply_add,convert_diagonal_to_1,n_row_ops_A,n_row_ops_inactive,n_row_multiply_add_A,n_row_multiply_add_inactive,zero_num));
    DEBUG_PRINT(("[ROWOPTION] A n_repair: %d n_row_multiply_add_A: %ld n_row_multiply_add_inactive: %ld multiply_add_A_L: %ld multiply_add_A_R: %ld Sum_A_L_Interval: %ld multiply_add_A_N: %ld\n", n_repair, n_row_multiply_add_A,n_row_multiply_add_inactive,multiply_add_A_L,multiply_add_A_R,Sum_A_L_Interval,multiply_add_A_N));

    n_repair = 0;
    n_row_sub_ops = 0;
    n_row_ops=0;
    convert_diagonal_to_1=0;
    n_row_multiply_add =0;
    n_row_ops_A=0;
    n_row_ops_inactive=0;
    n_row_multiply_add_A=0;
    n_row_multiply_add_inactive=0;
    zero_num =0;

    multiply_add_A_L=0;
    multiply_add_A_R=0;
    multiply_add_A_N=0;
    Sum_A_L_Interval =0;
    LastLost =0;

    dc->inorder = win_e;
    dc->dof = 0;
    dc->win_s = -1;
    dc->win_e = -1;
    dc->active = 0;

    // NOTE: The active time timer is triggered when the #win_s source packet is lost. The busy period time should not 
    // count in that time slot, because the analytical busy period counts from when the state is already in X=w-d=1, rather than 0.
    return 0;
}

void free_decoder(struct decoder *dc)
{
    assert(dc != NULL);
    //free(dc->cp);       // not malloced, no need to free
    //if (dc->pbuf != NULL) {
    //    free_packet(dc->pbuf);
    //}
    int i;
    for (i=0; i<WIN_ALLOC; i++) {
        if (dc->row[i] != NULL) {
            free(dc->row[i]->elem);
            free(dc->row[i]);
            dc->row[i] = NULL;
        }
        if (dc->message[i] != NULL) {
            free(dc->message[i]);
            dc->message[i] = NULL;
        }
    }
    free(dc->row);
    free(dc->message);
    for (i=0; i<DEC_ALLOC; i++) {
        if (dc->recovered[i] != NULL) {
            free(dc->recovered[i]);
            dc->recovered[i] = NULL;
        }
    }
    free(dc->recovered);
    free(dc);
    dc = NULL;
    return;
}
