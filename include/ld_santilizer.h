//
// Created by 邹嘉旭 on 2025/4/1.
//

#ifndef LD_SANTILIZER_H
#define LD_SANTILIZER_H

#include "ldacs_sim.h"
#include "ld_util_def.h"
#include "passert.h"
#include "ld_buffer.h"
#include "ld_log.h"
#include "ld_crc.h"
#include "cipher_util.h"


#define elemsof(array) (sizeof(array) / sizeof(*(array)))	/* number of elements in an array */



#define CRC_8_SIZE 8
#define CRC_32_SIZE 32

#define pbs_offset(pbs) ((size_t)((pbs)->cur - (pbs)->start))
#define pbs_room(pbs) ((size_t)((pbs)->roof - (pbs)->start))
#define pbs_left(pbs) ((size_t)((pbs)->roof - (pbs)->cur))

typedef struct struct_desc_t_s {
    const char *name;
    struct field_desc *fields;
} struct_desc_t;

/* structure for use by constant->name array. This is private. */
typedef struct enum_names {
    unsigned long en_first; /* first value in range */
    unsigned long en_last; /* last value in range (inclusive) */
    const char *const *en_names;
    struct enum_names *en_next_range; /* descriptor of next range for multiple ranges */
} enum_names;

typedef struct pk_fix_length_s {
    size_t len;
} pk_fix_length_t;


/*
 * 需要考虑更具体的数据类型
 */
enum field_type {
    ft_mbz, /* must be zero, abort */
    ft_set, /* bits representing set */
    ft_enum, /* value from an enumeration*/
    ft_plen, /* record pdu length */
    /* when len field is plen(a.k.a record whole pdu length), use this tag.
     * When using this tag, make sure the desc fields of fp is the pointer of size_t
     * which identifying the length after this field  */
    ft_p_raw,
    ft_slen, /* record sdu length */
    ft_fix,
    ft_sqn,
    ft_mac,
    ft_time,
    /* only can be used when str has a dynamic length */
    ft_dl_str,
    /* only can be used when str has a fix length */
    ft_fl_str,
    ft_pad, /* bits padding to octet*/
    ft_crc,
    ft_end, /* end of field list */
};

enum fix_type {
    NUMBER = 0,
    STRING,
};

typedef struct fix_desc_s {
    enum fix_type type;
    unsigned long fix_num;
    uint8_t *fix_str;
} fix_desc_t;

typedef struct field_desc {
    enum field_type field_type;
    const int size; /* size, in bits, of field */
    const char *name;
    const void *desc; /* enum_names for enum or char *[] for bits */
} field_desc;

typedef struct packet_bit_stream {
    struct packet_bit_stream *container; /* PBS of which we are part */
    struct_desc_t *desc;
    const char *name; /* what does this PBS represent? */
    uint8_t *start,
            *cur, /* current position in stream */
            *roof; /* byte after last in PBS (actually just a limit on output) */

    /* For an output PBS, the length field will be filled in later so
     * we need to record its particulars.  Note: it may not be aligned.
     */
    uint8_t *slen_fld,
            *plen_fld,
            *crc_fld,
            *pld_fld;

    int slento_shift;
    int plento_shift;
    size_t crc_size;

    size_t bitsize;
} pb_stream;


int ret_len(field_desc *fd);

void init_pbs(pb_stream *pbs, uint8_t *start, size_t len, const char *name);

/*
 * outs in the out_struct() is the output stream, obj_pbs is used to record fields (length fields) that have not been determined by the module currently being processed,
 * the undetermined length fields recorded in obj_pbs will be filled in the closed_output_pbs() function.
 *
 * So as the in_struct().
 */

/*
 *  Copy the 'struct_ptr' to the outs as described in SD.
 *  Meanwhile, if 'obj_pbs' exists, it will point to the payload section
 *  and updates 'cur' pointer of obj_pbs to the newly filled position,
 *  and then the 'cur' of outs will be set to the maximum.
 *  So that other functions cannot operate on 'outs' anymore
 *  unless obj_pbs is closed in close_outpub_pbs().
 *
 *  So as the in_struct().
 */

//#define KEY_HANDLE void *

bool in_struct(void *struct_ptr, struct_desc_t *sd, pb_stream *ins, pb_stream *obj_pbs);


bool pb_in_mac(pb_stream *ins, size_t mac_len, KEY_HANDLE key_med);

bool out_struct(const void *struct_ptr, struct_desc_t *sd, pb_stream *outs, pb_stream *obj_pbs);


void pb_out_mac(pb_stream *outs, size_t mac_len, KEY_HANDLE key_med);

void close_output_pbs(pb_stream *pbs);


uint8_t cal_crc_8bits(uint8_t *start, const uint8_t *cur);

static inline void append_bits(int *to_shift, const uint64_t *n, uint8_t **cur);

static inline void detach_bits(int *to_shift, uint64_t *n, uint8_t **cur, size_t unanvil);

//static inline uint8_t * detach_bits(int argc, const int * to_shift, uint64_t * n, uint8_t * cur, size_t unanvil);
//#define detach_bits(...) detach_bits_l(ARGC(__VA_ARGS__), __VA_ARGS__)

static inline bool is_equal(const uint8_t *src, const uint8_t *contrast, size_t size);

void int2byte(uint32_t val, uint8_t *bytes, size_t size);

void byte2int(uint32_t *val, const uint8_t *bytes, size_t size);


void copy_pbs(pb_stream *src, pb_stream *dst);

size_t sizeof_struct(field_desc *fd);

const char *enum_name(const enum_names *ed, unsigned long val);

const char *enum_show(enum_names *ed, unsigned long val);


#endif //LD_SANTILIZER_H
