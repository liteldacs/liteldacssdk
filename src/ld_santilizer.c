//
// Created by jiaxv on 23-6-4.
//
#include "ld_santilizer.h"

inline void append_bits(int *to_shift, const uint64_t *n, uint8_t **cur) {
    while (*to_shift > (~BITS_PER_BYTE + 1)) {
        if (*to_shift >= 0) {
            **cur |= (*n >> *to_shift) & COMPLEMENT_8;
            (*cur)++;
        } else {
            **cur |= (*n << (~(*to_shift) + 1)) & COMPLEMENT_8;
        }
        *to_shift -= BITS_PER_BYTE;
    }
}

inline void detach_bits(int *to_shift, uint64_t *n, uint8_t **cur, size_t pre_unavil) {
    uint8_t specific;

    /* if 'to_shift' smaller than -8, meaning that all bits of this field has been processed */
    while (*to_shift > (~BITS_PER_BYTE + 1)) {
        if (pre_unavil) {
            specific = **cur & (0xFF >> pre_unavil); /* set the bits of last processed fields into 0 */
            pre_unavil = 0;
        } else {
            specific = **cur;
        }

        if (*to_shift >= 0) {
            *n |= (specific << *to_shift) & COMPLEMENT_64;
            (*cur)++;
        } else {
            *n |= (specific >> (~(*to_shift) + 1)) & COMPLEMENT_64;
        }
        *to_shift -= BITS_PER_BYTE;
    }
}

void append_str(uint8_t **cur, size_t *bit_size, const uint8_t *src, size_t size) {
    while (size--) {
        int to_shift = *bit_size % BITS_PER_BYTE;
        uint64_t n = *(const uint8_t *) src++;
        append_bits(&to_shift, &n, cur);
        *bit_size += BITS_PER_BYTE;
    }
}

void detach_str(uint8_t **cur, size_t *bit_size, uint8_t *dst, size_t size) {
    uint64_t n;
    while (size--) {
        n = 0;
        size_t pre_octect_unavil = *bit_size % BITS_PER_BYTE;
        /* the previous bit length of last processed field in current byte */
        int to_shift = pre_octect_unavil; /* the beginning 'to_shift' is the remaining length of the very octect */
        detach_bits(&to_shift, &n, cur, pre_octect_unavil);

        *dst++ = n;
        *bit_size += BITS_PER_BYTE;
    }
}


void int2byte(uint32_t val, uint8_t *bytes, size_t size) {
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (uint8_t) (val >> (BITS_PER_BYTE * (size - i - 1)));
    }
}

void byte2int(uint32_t *val, const uint8_t *bytes, size_t size) {
    for (size_t i = 0; i < size; i++) {
        *val += bytes[i] << (BITS_PER_BYTE * (size - i - 1));
    }
}

inline bool is_equal(const uint8_t *src, const uint8_t *contrast, size_t size) {
    for (int i = 0; i < size; i++) {
        if (src[i] != contrast[i]) return FALSE;
    }
    return TRUE;
}

/**
 * init the bit stream with a certain uchar array
 * @param pbs packet_bit_stream entity
 * @param start the first address of uchar array
 * @param len length of the whole buffer / input stream
 * @param name the description of this pbs
 */
void init_pbs(pb_stream *pbs, uint8_t *start, size_t len, const char *name) {
    pbs->container = NULL;
    pbs->desc = NULL;
    pbs->name = name;
    pbs->start = pbs->cur = start;
    pbs->roof = start + len;
    pbs->slen_fld = pbs->plen_fld = pbs->crc_fld = pbs->pld_fld = NULL;
    pbs->slento_shift = pbs->plento_shift = 0;
    pbs->bitsize = 0;
}

void copy_pbs(pb_stream *src, pb_stream *dst) {
    memcpy(dst, src, sizeof(pb_stream));
}

/**
 * parse the input stream into data struct
 *
 * If obj_pbs is supplied, a new pb_stream is created for the
 * variable part of the structure (this depends on their
 * being one length field in the structure).  The cursor of this
 * new PBS is set to after the parsed part of the struct.
 *
 *
 * @param struct_ptr :data struct
 * @param sd :description of data organization method
 * @param ins : input stream
 * @param obj_pbs :subflow
 * @return :if successfully parse
 */
bool in_struct(void *struct_ptr, struct_desc_t *sd, pb_stream *ins, pb_stream *obj_pbs) {
    err_t ugh = NULL;
    uint8_t *cur = ins->cur;
    size_t *bit_size = &ins->bitsize;
    size_t sdu_len = 0;
    size_t pdu_len = 0;
    uint8_t *payload_start;

    if (ins->roof - cur < sizeof_struct(sd->fields)) {
        ugh = builddiag("not enough room left in input packet for %s",
                        sd->name);
    } else {
        uint8_t *roof = cur + sizeof_struct(sd->fields);
        uint8_t *outp = struct_ptr;
        field_desc *fp;

        for (fp = sd->fields; ugh == NULL; fp++) {
            size_t i = fp->size;
            switch (fp->field_type) {
                case ft_mbz:
                case ft_enum:
                case ft_slen:
                case ft_plen:
                case ft_fix:
                case ft_time:
                case ft_sqn:
                case ft_set: {
                    uint64_t n = 0;

                    size_t pre_octect_unavil = *bit_size % BITS_PER_BYTE;
                    /* the previous bit length of last processed field in current byte */
                    int to_shift = i + pre_octect_unavil - BITS_PER_BYTE;
                    /* the beginning 'to_shift' is the remaining length of the very octect */

                    detach_bits(&to_shift, &n, &cur, pre_octect_unavil);

                    /* Determin type based on bit length */
                    switch (bytes_len(i)) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            *(uint8_t *) outp = n;
                            break;
                        case 4:
                            *(uint16_t *) outp = n;
                            break;
                        case 5:
                            *(uint32_t *) outp = n;
                            break;
                        case 6:
                            *(uint64_t *) outp = n;
                            break;
                        default:
                            //TODO: 错误处理
                            break;
                    }

                    switch (fp->field_type) {
                        case ft_enum:
                            if (enum_name(fp->desc, n) == NULL) {
                                ugh = builddiag("%s of %s has an unknown out value: %lu",
                                                fp->name, sd->name, (unsigned long) n);
                            }
                            break;
                        case ft_slen: {
                            sdu_len = n;
                            break;
                        }
                        case ft_plen: {
                            pdu_len = n;
                            break;
                        }
                        //case ft_sqn: {
                        //
                        //    break;
                        //}
                        case ft_fix: {
                            const fix_desc_t *fix_desc = fp->desc;
                            if (n != fix_desc->fix_num) {
                                ugh = builddiag("%s value of %s is %lu, but it is not equals to %lu",
                                                fp->name, sd->name, (unsigned long) n, fix_desc->fix_num);
                            }
                            break;
                        }
                        default:
                            break;
                    }

                    outp += ((fp->size - 1) / BITS_PER_BYTE) + 1;
                    *bit_size += i;
                    break;
                }
                case ft_pad:
                    if (*bit_size % BITS_PER_BYTE) {
                        cur++; /* jump to the next octect */
                        *bit_size = 0;
                        /* now all the previous bits have been organized into bytes, bit_size should be set to 0 */
                    } /* if not, multiple of 8, no need to padding, break */

                    payload_start = cur;
                    break;
                case ft_crc: {
                    ins->crc_fld = cur;
                    switch (fp->size) {
                        case CRC_8_SIZE: {
                            uint8_t crc = cal_crc_8bits(ins->start, cur);
                            if (crc != *cur++) {
                                ugh = builddiag("%s Wrong CRC", fp->name);
                            }
                            break;
                        }
                        case CRC_32_SIZE: {
                            uint32_t crc = cal_crc_32bits(ins->start, cur);
                            uint32_t in = 0;
                            for (int p = 0; p < CRC_32_SIZE / BITS_PER_BYTE; p++) {
                                in += *cur++ << ((CRC_32_SIZE / BITS_PER_BYTE - p - 1) * BITS_PER_BYTE);
                            }
                            if (crc != in) {
                                ugh = builddiag("%s Wrong CRC", fp->name);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case ft_fl_str: {
                    buffer_t *b = init_buffer_ptr(((pk_fix_length_t *) fp->desc)->len);
                    *(buffer_t **) (uint64_t *) outp = b;

                    uint8_t temp[MAX_INPUT_BUFFER_SIZE] = {0};

                    detach_str(&cur, bit_size, temp, b->free);
                    cat_to_buffer(b, temp, b->free);

                    outp += sizeof(void *);
                    break;
                }
                case ft_dl_str: {
                    buffer_t *b = (buffer_t *) *(uint64_t *) outp;
                    if (b) {
                        uint8_t temp[MAX_INPUT_BUFFER_SIZE] = {0};

                        detach_str(&cur, bit_size, temp, b->free);
                        cat_to_buffer(b, temp, b->free);
                    }

                    outp += sizeof(void *);
                    break;
                }
                //case ft_sqn:
                //    byte2int((uint32_t *)outp, cur, fp->size);
                //    cur += fp->size;
                //    break;
                case ft_p_raw: {
                    ins->pld_fld = cur;
                    size_t slen = pdu_len - (cur - ins->start) - (*(size_t *) fp->desc >> 3);

                    buffer_t *b = (buffer_t *) *(uint64_t *) outp;
                    uint8_t temp[MAX_INPUT_BUFFER_SIZE] = {0};

                    detach_str(&cur, bit_size, temp, slen);
                    CLONE_TO_CHUNK(*b, temp, slen);

                    outp += sizeof(void *);
                    break;
                }
                case ft_end:
                    if (obj_pbs != NULL) {
                        /* the room of obj_pbs is the len in the parent pb_stream */

                        size_t payload_len = sdu_len;
                        init_pbs(obj_pbs, payload_start, payload_len, sd->name);

                        obj_pbs->container = ins;
                        obj_pbs->desc = sd;
                        obj_pbs->cur = cur;
                        ins->cur = cur + payload_len;
                    } else {
                        ins->cur = cur;
                    }

                //DBG(DBG_EMITTING, DBG_print_struct("emit",struct_ptr, sd));
                    return TRUE;
                default:
                    break;
            }
        }
    }

    log_warn("%s warnning: %s", sd->name, ugh);
    return FALSE;
}


/**
 * 验证输入MAC
 * @param ins 输入报文组装工具
 * @param mac_len MAC长度
 * @param key 密钥
 * @return 验证结果
 */
bool pb_in_mac(pb_stream *ins, size_t mac_len, KEY_HANDLE key_med, verify_hmac_func verify_hmac) {
    uint8_t *initial = ins->start;
    size_t len = ins->cur - initial;

    /* 验证结果 */
    if (!verify_hmac(key_med, ins->cur, initial, len, mac_len)) {
        log_buf(LOG_DEBUG, "to_veri", ins->cur, 32);
        log_error("MAC verify failed!");
        return FALSE;
    } else {
        log_debug("MAC verify succeed!");
        ins->cur += mac_len;
        return TRUE;
    }
}

/**
 * construct packet based on description
 *
 * If obj_pbs is non-NULL, its pbs describes a new output stream set up
 * to contain the object.  The cursor will be left at the variable part.
 * This new stream must subsequently be finalized by close_output_pbs().
 *
 * The value of any field of type ft_len is computed, not taken
 * from the input struct.  The length is actually filled in when
 * the object's output stream is finalized.  If obj_pbs is NULL,
 * finalization is done by out_struct before it returns.
 *
 * @param struct_ptr :address of data struct
 * @param sd :description of data organization method
 * @param outs :output stream
 * @param obj_pbs :subflow
 * @return :if successfully constructed
 */
bool out_struct(const void *struct_ptr, struct_desc_t *sd, pb_stream *outs, pb_stream *obj_pbs) {
    err_t ugh = NULL;
    const uint8_t *inp = struct_ptr;
    uint8_t *cur = outs->cur;
    size_t *bit_size = &outs->bitsize;
    uint8_t *payload_start;


    if (outs->roof - cur < sizeof_struct(sd->fields)) {
        ugh = builddiag("not enough room left in output packet to place %s",
                        sd->name);
    } else {
        //DBG(DBG_EMITTING, DBG_print_struct("emit",struct_ptr, sd));
        field_desc *fp;
        pb_stream obj;
        obj.slen_fld = obj.plen_fld = obj.crc_fld = NULL;

        for (fp = sd->fields; ugh == NULL; fp++) {
            size_t i = fp->size;
            switch (fp->field_type) {
                case ft_time:
                case ft_mbz:
                case ft_enum:
                case ft_slen:
                case ft_plen:
                case ft_fix:
                case ft_sqn:
                case ft_set: {
                    uint64_t n = 0;

                    /* Determin type based on bit length */
                    switch (bytes_len(i)) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            n = *(const uint8_t *) inp;
                            break;
                        case 4:
                            n = *(const uint16_t *) inp;
                            break;
                        case 5:
                            n = *(const uint32_t *) inp;
                            break;
                        case 6:
                            n = *(const uint64_t *) inp;
                            break;
                        default:
                            //应有错误处理
                            break;
                    }

                    /* to_shift is the length that needs to be shifted right
                    * if to_shift < 0, means that the remaining bits are not enough to fill one byte,
                    * need to shift the absolute value of to_shift to the left.
                    * Otherwise, shift right with to_shift bits
                    */
                    int to_shift = i - (BITS_PER_BYTE - (*bit_size % BITS_PER_BYTE));

                    switch (fp->field_type) {
                        case ft_enum:
                            if (enum_name(fp->desc, n) == NULL) {
                                ugh = builddiag("%s of %s has an unknown out value: %lu",
                                                fp->name, sd->name, (unsigned long) n);
                            }
                            break;
                        case ft_slen:
                            obj.slen_fld = cur;
                            obj.slento_shift = to_shift;
                            break;
                        case ft_plen:
                            outs->plen_fld = cur;
                            outs->plento_shift = to_shift;
                            break;
                        case ft_fix: {
                            const fix_desc_t *fix_desc = fp->desc;
                            n = fix_desc->fix_num;
                            break;
                        }
                        case ft_time: {
                            time_t t = time(NULL);
                            n = t;
                            break;
                        }
                        default:
                            break;
                    }

                    append_bits(&to_shift, &n, &cur);
                    inp += ((fp->size - 1) / BITS_PER_BYTE) + 1;
                    *bit_size += i;
                    break;
                }
                case ft_pad: {
                    /* if not, bit_size is multiple of 8, no need to padding */
                    if (*bit_size % BITS_PER_BYTE) cur++; /* jump to the next octect */
                    outs->bitsize = *bit_size = 0;
                    /* now all the previous bits have been organized into bytes, bit_size should be set to 0 */
                    payload_start = cur;
                    break;
                }
                //case ft_sqn:
                //    int2byte(*(uint32_t *)inp, cur, fp->size);
                //    cur += fp->size;
                //    break;
                case ft_crc: {
                    outs->crc_fld = cur;
                    outs->crc_size = fp->size;
                    cur += fp->size >> 3; // BITS_PER_SIZE 2^3 == 8

                    break;
                }
                case ft_fl_str:
                case ft_dl_str:
                case ft_p_raw: {
                    buffer_t *b = (buffer_t *) *(uint64_t *) inp;
                    if (b) {
                        append_str(&cur, bit_size, b->ptr, b->len);
                    }
                    inp += sizeof(void *);
                    break;
                }
                case ft_end:
                    passert(payload_start != NULL);

                    obj.container = outs;
                    obj.desc = sd;
                    obj.name = sd->name;
                /* It is different from IPSec, the length of IPSec is the length which is containing the whole struct
                 * but the length here is to record the payload length, the beginning of which is the very first byte after 'ft_pad'*/
                    obj.start = payload_start;
                    obj.cur = cur;
                    obj.roof = outs->roof; /* limit of possible */
                    obj.bitsize = 0;
                /* obj.lenfld and obj.lenfld_desc already set */

                    if (obj_pbs == NULL) {
                        close_output_pbs(&obj); /* fill in length field, if any */
                    } else {
                        /* We set outs->cur to outs->roof so that
                         * any attempt to output something into outs
                         * before obj is closed will trigger an error.
                         */
                        outs->cur = outs->roof;
                        *obj_pbs = obj;
                    }
                    return TRUE;
                default:
                    bad_case(fp->field_type);
            }
        }
    }
    /* some failure got us here: report it */
    loglog(RC_LOG_SERIOUS, "%s", ugh); /* ??? serious, but errno not relevant */
    return FALSE;
}


/**
 * 计算MAC
 * @param outs 报文组装工具
 * @param mac_len MAC长度
 * @param key 密钥
 */
void pb_out_mac(pb_stream *outs, size_t mac_len, KEY_HANDLE key_med, calc_hmac_func calc_hmac) {
    uint8_t *initial = outs->start;
    size_t len = outs->cur - initial;

    calc_hmac(initial, len, key_med, outs->cur, mac_len);

    /** should cut mac into a certain length, depending on mac_len */
    outs->cur += mac_len;
}

/* Record current length.
 * Note: currently, this may be repeated any number of times;
 * the last one wins.
 */
void close_output_pbs(pb_stream *pbs) {
    if (pbs->slen_fld != NULL) {
        uint64_t slen = pbs_offset(pbs);
        append_bits(&pbs->slento_shift, &slen, &pbs->slen_fld); /* fill the length field */
    }
    if (pbs->container != NULL) {
        pbs->container->cur = pbs->cur; /* pass space utilization up, skip the subflow */
    }
    if (pbs->plen_fld != NULL) {
        uint64_t plen = pbs_offset(pbs);
        append_bits(&pbs->plento_shift, &plen, &pbs->plen_fld); /* fill the length field */
    }
    /* generate crc as the last procedure */
    if (pbs->crc_fld != NULL) {
        switch (pbs->crc_size) {
            case CRC_8_SIZE: {
                uint8_t crc = cal_crc_8bits(pbs->start, pbs->crc_fld);
                *pbs->crc_fld++ = crc;
                break;
            }
            case CRC_32_SIZE: {
                uint32_t crc = cal_crc_32bits(pbs->start, pbs->crc_fld);
                for (int p = 0; p < CRC_32_SIZE / BITS_PER_BYTE; p++) {
                    *pbs->crc_fld++ = (crc >> ((CRC_32_SIZE / BITS_PER_BYTE - 1 - p) * BITS_PER_BYTE)) & 0xFF;
                }
                break;
            }
            default:
                break;
        }
    }
}

int ret_len(field_desc *fd) {
    int size = 0;
    for (int i = 0; fd[i].field_type != ft_end; i++) {
        size += fd[i].size;
    }
    return size;
}

const char *enum_name_default(const enum_names *ed, unsigned long val, const char *def) {
    const enum_names *p;

    for (p = ed; p != NULL; p = p->en_next_range)
        if (p->en_first <= val && val <= p->en_last)
            return p->en_names[val - p->en_first];
    return def;
}

const char *enum_name(const enum_names *ed, unsigned long val) {
    return enum_name_default(ed, val, NULL);
}

const char *enum_show(enum_names *ed, unsigned long val) {
    const char *p = enum_name(ed, val);

    if (p == NULL) {
        static char buf[12]; /* only one!  I hope that it is big enough */
        snprintf(buf, sizeof(buf), "%lu??", val);
        p = buf;
    }
    return p;
}


size_t sizeof_struct(field_desc *fd) {
    size_t ret = 0;
    for (field_desc *f = fd; f->field_type != ft_end; f++) {
        ret += f->size;
    }
    return ret % 8 == 0 ? ret / 8 : (ret / 8) + 1;
}

buffer_t *gen_pdu(void *objs, struct_desc_t *desc, const char *name) {
    uint8_t out_stream[512] = {0};
    pb_stream pbs;
    init_pbs(&pbs, out_stream, sizeof(out_stream), name);
    out_struct(objs, desc, &pbs, NULL);
    close_output_pbs(&pbs);

    buffer_t *buf = init_buffer_unptr();
    CLONE_TO_CHUNK(*buf, pbs.start, pbs_offset(&pbs))
    return buf;
}


void *parse_sdu(buffer_t *buf, struct_desc_t *desc, size_t size) {
    pb_stream pbs;
    init_pbs(&pbs, buf->ptr, buf->len, "parse sdu");

    void *data_upload = malloc(size);
    if (data_upload == NULL) return NULL;
    if (in_struct(data_upload, desc, &pbs, NULL) == FALSE) {
        free(data_upload);
        return NULL;
    }

    // char *dd = data_upload;
    // log_info("%s", desc->name);
    // log_buf(LOG_WARN, "AAA", dd, size);
    return data_upload;
}
