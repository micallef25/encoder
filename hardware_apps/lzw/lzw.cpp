#include "lzw.h"
#include <ap_int.h>
#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// ref code
// ap_uint<216> key_low[512]; ap_uint<216> key_high[512]; int value[216];

// match_low=key_low[key%512]; match_high=key_high[(key>>9)%512]; match=match_low & match_high; addr=binary_encode(match);
// res=value[addr];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/* Change to have a verbose output of the algorithm, as well as supplementary
 * checks of the intermediate stages of the algorithm
 */
#define DEBUG_CODE 0
#define DEBUG_CAM_HASH 0
#define DEBUG_DMA 0
#define VERBOSE 0

#define ENABLE_CAM 1

static uint16_t ctr;
static uint16_t lzw_chunk_bit_count;
static uint32_t lzw_chunk_bit_buffer;

/**
 * hash function
 *	Takes a 21 bit key and generates a 12 bit hash from the key, by doing random
 *	xors of the bits of the key
 */
ap_uint<LZW_HASH_BITS> hash_lzw(ap_uint<LZW_KEY_BITS> key)
{

	#if (LZW_KEY_BITS == 20 && LZW_HASH_BITS == 13)
	ap_uint<LZW_HASH_BITS> hash_out;
	hash_out[0x00] = key[0x10] ^ key[0x07] ^ key[0x0a] ^ key[0x04] ^ key[0x05] ^ key[0x06];
	hash_out[0x01] = key[0x10] ^ key[0x08] ^ key[0x0c] ^ key[0x06] ^ key[0x0b] ^ key[0x09];
	hash_out[0x02] = key[0x07] ^ key[0x0c] ^ key[0x00] ^ key[0x01] ^ key[0x0f] ^ key[0x08];
	hash_out[0x03] = key[0x11] ^ key[0x13] ^ key[0x06] ^ key[0x0c] ^ key[0x10] ^ key[0x0d];
	hash_out[0x04] = key[0x02] ^ key[0x0b] ^ key[0x03] ^ key[0x06] ^ key[0x08] ^ key[0x00];
	hash_out[0x05] = key[0x08] ^ key[0x01] ^ key[0x0b] ^ key[0x10] ^ key[0x03] ^ key[0x0c];
	hash_out[0x06] = key[0x0c] ^ key[0x13] ^ key[0x0b] ^ key[0x07] ^ key[0x04] ^ key[0x10];
	hash_out[0x07] = key[0x04] ^ key[0x10] ^ key[0x08] ^ key[0x02] ^ key[0x0c] ^ key[0x11];
	hash_out[0x08] = key[0x12] ^ key[0x10] ^ key[0x0c] ^ key[0x11] ^ key[0x00] ^ key[0x08];
	hash_out[0x09] = key[0x05] ^ key[0x0d] ^ key[0x06] ^ key[0x0c] ^ key[0x00] ^ key[0x08];
	hash_out[0x0a] = key[0x0c] ^ key[0x0d] ^ key[0x13] ^ key[0x08] ^ key[0x0f] ^ key[0x06];
	hash_out[0x0b] = key[0x02] ^ key[0x03] ^ key[0x12] ^ key[0x10] ^ key[0x0b] ^ key[0x0c];
	hash_out[0x0c] = key[0x08] ^ key[0x0a] ^ key[0x10] ^ key[0x0d] ^ key[0x02] ^ key[0x0b];
	#endif
	return hash_out;
}

void encode_hash_table(ap_uint<HASH_TABLE_ROW_WIDTH> *row,
				  	  	  	  ap_uint<HASH_TABLE_ITEM_WIDTH> item,
							  ap_uint<HASH_TABLE_COUNTER_BITS> pos)
{
	(*row).range((pos+1) * HASH_TABLE_ITEM_WIDTH - 1,pos * HASH_TABLE_ITEM_WIDTH) = item;
}

ap_uint<HASH_TABLE_COUNTER_BITS> buckets_used_row(ap_uint<HASH_TABLE_ROW_WIDTH> row)
{
	ap_uint<NUM_BUCKETS_PER_HASH> tmp;
	ap_uint<HASH_TABLE_COUNTER_BITS> ret_used;
	for(int i = 0; i < NUM_BUCKETS_PER_HASH; i++) {
		#pragma HLS unroll
		tmp.set_bit(i, row.get_bit((i + 1)*HASH_TABLE_ITEM_WIDTH - 1));
	}
	switch (tmp){
	case 0x0:
		ret_used = 0;
		break;
	case 0x1:
		ret_used = 1;
		break;
	case 0x3:
		ret_used = 2;
		break;
	case 0x7:
		ret_used = 3;
		break;
	case 0xF:
		ret_used = 4;
		break;
	}
	return ret_used;
}

ap_uint<1>  match_hash_table_row(ap_uint<HASH_TABLE_ROW_WIDTH> row,
						  ap_uint<LZW_KEY_BITS> key,
						  ap_uint<LZW_CODE_BITS> *value_found)
{
	ap_uint<1> ret_value = 1;
	for (int i = 0; i < NUM_BUCKETS_PER_HASH; i++) {
		#pragma HLS unroll
		if ((row.range(i * HASH_TABLE_ITEM_WIDTH + LZW_KEY_BITS - 1,i * HASH_TABLE_ITEM_WIDTH) == key) &&
				row.get_bit((i+1)*HASH_TABLE_ITEM_WIDTH -1)) 
		{
			*value_found = row.range((i + 1) * HASH_TABLE_ITEM_WIDTH - 2,i * HASH_TABLE_ITEM_WIDTH + LZW_KEY_BITS);
			ret_value= 0;
		}
	}
	return ret_value;
}

/**
 * decode_match
 *	Decodes ANDed matchbits (1-hot)
 *	which should be power-of-2 (only one '1')
 *	into the result-th element of value array.
 */
ap_uint<CAM_VALUES_BITS> decode_match(ap_uint<CAM_BRAM_BITS> matchbits)
{
	ap_uint<CAM_VALUES_BITS> retval = 0;

	ap_uint<32> tmp_0;
	tmp_0.set_bit(0,matchbits.get_bit(1));
	tmp_0.set_bit(1,matchbits.get_bit(3));
	tmp_0.set_bit(2,matchbits.get_bit(5));
	tmp_0.set_bit(3,matchbits.get_bit(7));
	tmp_0.set_bit(4,matchbits.get_bit(9));
	tmp_0.set_bit(5,matchbits.get_bit(11));
	tmp_0.set_bit(6,matchbits.get_bit(13));
	tmp_0.set_bit(7,matchbits.get_bit(15));
	tmp_0.set_bit(8,matchbits.get_bit(17));
	tmp_0.set_bit(9,matchbits.get_bit(19));
	tmp_0.set_bit(10,matchbits.get_bit(21));
	tmp_0.set_bit(11,matchbits.get_bit(23));
	tmp_0.set_bit(12,matchbits.get_bit(25));
	tmp_0.set_bit(13,matchbits.get_bit(27));
	tmp_0.set_bit(14,matchbits.get_bit(29));
	tmp_0.set_bit(15,matchbits.get_bit(31));
	tmp_0.set_bit(16,matchbits.get_bit(33));
	tmp_0.set_bit(17,matchbits.get_bit(35));
	tmp_0.set_bit(18,matchbits.get_bit(37));
	tmp_0.set_bit(19,matchbits.get_bit(39));
	tmp_0.set_bit(20,matchbits.get_bit(41));
	tmp_0.set_bit(21,matchbits.get_bit(43));
	tmp_0.set_bit(22,matchbits.get_bit(45));
	tmp_0.set_bit(23,matchbits.get_bit(47));
	tmp_0.set_bit(24,matchbits.get_bit(49));
	tmp_0.set_bit(25,matchbits.get_bit(51));
	tmp_0.set_bit(26,matchbits.get_bit(53));
	tmp_0.set_bit(27,matchbits.get_bit(55));
	tmp_0.set_bit(28,matchbits.get_bit(57));
	tmp_0.set_bit(29,matchbits.get_bit(59));
	tmp_0.set_bit(30,matchbits.get_bit(61));
	tmp_0.set_bit(31,matchbits.get_bit(63));
	ap_uint<16> tmp_1;
	ap_uint<2> ttmp_3_2 = matchbits.range(3, 2);
	tmp_1.set_bit(0, ttmp_3_2.or_reduce());
	ap_uint<2> ttmp_7_6 = matchbits.range(7, 6);
	tmp_1.set_bit(1, ttmp_7_6.or_reduce());
	ap_uint<2> ttmp_11_10 = matchbits.range(11, 10);
	tmp_1.set_bit(2, ttmp_11_10.or_reduce());
	ap_uint<2> ttmp_15_14 = matchbits.range(15, 14);
	tmp_1.set_bit(3, ttmp_15_14.or_reduce());
	ap_uint<2> ttmp_19_18 = matchbits.range(19, 18);
	tmp_1.set_bit(4, ttmp_19_18.or_reduce());
	ap_uint<2> ttmp_23_22 = matchbits.range(23, 22);
	tmp_1.set_bit(5, ttmp_23_22.or_reduce());
	ap_uint<2> ttmp_27_26 = matchbits.range(27, 26);
	tmp_1.set_bit(6, ttmp_27_26.or_reduce());
	ap_uint<2> ttmp_31_30 = matchbits.range(31, 30);
	tmp_1.set_bit(7, ttmp_31_30.or_reduce());
	ap_uint<2> ttmp_35_34 = matchbits.range(35, 34);
	tmp_1.set_bit(8, ttmp_35_34.or_reduce());
	ap_uint<2> ttmp_39_38 = matchbits.range(39, 38);
	tmp_1.set_bit(9, ttmp_39_38.or_reduce());
	ap_uint<2> ttmp_43_42 = matchbits.range(43, 42);
	tmp_1.set_bit(10, ttmp_43_42.or_reduce());
	ap_uint<2> ttmp_47_46 = matchbits.range(47, 46);
	tmp_1.set_bit(11, ttmp_47_46.or_reduce());
	ap_uint<2> ttmp_51_50 = matchbits.range(51, 50);
	tmp_1.set_bit(12, ttmp_51_50.or_reduce());
	ap_uint<2> ttmp_55_54 = matchbits.range(55, 54);
	tmp_1.set_bit(13, ttmp_55_54.or_reduce());
	ap_uint<2> ttmp_59_58 = matchbits.range(59, 58);
	tmp_1.set_bit(14, ttmp_59_58.or_reduce());
	ap_uint<2> ttmp_63_62 = matchbits.range(63, 62);
	tmp_1.set_bit(15, ttmp_63_62.or_reduce());
	ap_uint<8> tmp_2;
	ap_uint<4> ttmp_7_4 = matchbits.range(7, 4);
	tmp_2.set_bit(0, ttmp_7_4.or_reduce());
	ap_uint<4> ttmp_15_12 = matchbits.range(15, 12);
	tmp_2.set_bit(1, ttmp_15_12.or_reduce());
	ap_uint<4> ttmp_23_20 = matchbits.range(23, 20);
	tmp_2.set_bit(2, ttmp_23_20.or_reduce());
	ap_uint<4> ttmp_31_28 = matchbits.range(31, 28);
	tmp_2.set_bit(3, ttmp_31_28.or_reduce());
	ap_uint<4> ttmp_39_36 = matchbits.range(39, 36);
	tmp_2.set_bit(4, ttmp_39_36.or_reduce());
	ap_uint<4> ttmp_47_44 = matchbits.range(47, 44);
	tmp_2.set_bit(5, ttmp_47_44.or_reduce());
	ap_uint<4> ttmp_55_52 = matchbits.range(55, 52);
	tmp_2.set_bit(6, ttmp_55_52.or_reduce());
	ap_uint<4> ttmp_63_60 = matchbits.range(63, 60);
	tmp_2.set_bit(7, ttmp_63_60.or_reduce());
	ap_uint<4> tmp_3;
	ap_uint<8> ttmp_15_8 = matchbits.range(15, 8);
	tmp_3.set_bit(0, ttmp_15_8.or_reduce());
	ap_uint<8> ttmp_31_24 = matchbits.range(31, 24);
	tmp_3.set_bit(1, ttmp_31_24.or_reduce());
	ap_uint<8> ttmp_47_40 = matchbits.range(47, 40);
	tmp_3.set_bit(2, ttmp_47_40.or_reduce());
	ap_uint<8> ttmp_63_56 = matchbits.range(63, 56);
	tmp_3.set_bit(3, ttmp_63_56.or_reduce());
	ap_uint<2> tmp_4;
	ap_uint<16> ttmp_31_16 = matchbits.range(31, 16);
	tmp_4.set_bit(0, ttmp_31_16.or_reduce());
	ap_uint<16> ttmp_63_48 = matchbits.range(63, 48);
	tmp_4.set_bit(1, ttmp_63_48.or_reduce());
	ap_uint<1> tmp_5;
	ap_uint<32> ttmp_63_32 = matchbits.range(63, 32);
	tmp_5.set_bit(0, ttmp_63_32.or_reduce());
	retval.set_bit(0, tmp_0.or_reduce());
	retval.set_bit(1, tmp_1.or_reduce());
	retval.set_bit(2, tmp_2.or_reduce());
	retval.set_bit(3, tmp_3.or_reduce());
	retval.set_bit(4, tmp_4.or_reduce());
	retval.set_bit(5, tmp_5.or_reduce());

	return retval;
}


void output_code(ap_uint<LZW_CODE_BITS> code,
				 ap_uint<1> is_last_word,
				 unsigned char *buffer)
{
	lzw_chunk_bit_buffer |= (uint32_t)code << (32 - OUTPUT_BITS - lzw_chunk_bit_count);
	lzw_chunk_bit_count += OUTPUT_BITS;
	if (lzw_chunk_bit_count >= 8) {
		buffer[ctr] = (unsigned char)(lzw_chunk_bit_buffer >> 24);
		lzw_chunk_bit_buffer <<= 8;
		lzw_chunk_bit_count -= 8;
		ctr++;
	}
	if (lzw_chunk_bit_count >= 8) {
		buffer[ctr] = (unsigned char)(lzw_chunk_bit_buffer >> 24);
		lzw_chunk_bit_buffer <<= 8;
		lzw_chunk_bit_count -= 8;
		ctr++;
	}
	if (lzw_chunk_bit_count > 0 && lzw_chunk_bit_count < 8 && is_last_word) {
		buffer[ctr] = (unsigned char)(lzw_chunk_bit_buffer >> 24);
		ctr++;
	}
}

ap_uint<LZW_CODE_BITS> get_code(ap_uint<LZW_KEY_BITS> entry)
{
	return entry.range(LZW_KEY_BITS - 1, 8);
}

ap_uint<LZW_KEY_BITS> set_code(ap_uint<LZW_CODE_BITS> code, unsigned char c)
{
	ap_uint<LZW_KEY_BITS> retval = 0;
	retval.range(LZW_KEY_BITS - 1, 8) = code;
	retval.range(7, 0) = c;
	return retval;
}

/**
 * do_lzw
 *  Performs compression on chunk
 * Updates
 *  lzw_chunk: potentially larger post-lzw chunk payload
 *  lzw_size: size of lzw_chunk
 */
#pragma SDS data copy(cur_chunk[0:cur_chunk_size])
#pragma SDS data copy(lzw_chunk[0:2 * cur_chunk_size])
#pragma SDS data access_pattern(cur_chunk:SEQUENTIAL, lzw_chunk:SEQUENTIAL)
#pragma SDS data mem_attribute(cur_chunk:PHYSICAL_CONTIGUOUS, lzw_chunk:PHYSICAL_CONTIGUOUS)
//#pragma SDS data buffer_depth(cur_chunk:20, lzw_chunk: 20)
void do_lzw_hw(const unsigned char cur_chunk[MAX_CHUNK_SIZE],
			   const int cur_chunk_size,
			   unsigned char lzw_chunk[MAX_LZW_SIZE],
			   unsigned int *lzw_size)
{
	int chunk_left = cur_chunk_size;
	/* The dictionary will contain the codes using the tree notation
	 * <previous-code> + next char
	 */
	uint16_t keys_ind = 256;
	const unsigned char *inp = cur_chunk;
	unsigned char c0 = 0;
	unsigned char c1 = 0;

	#if DEBUG_DMA
	int ctr_cr = 0;
	int ctr_sr = 0;
	int ctr_lw = 0;
	#endif

	// BRAMs for CAM
	ap_uint<CAM_BRAM_BITS> key_lo[KEY_DEPTH];
	ap_uint<CAM_BRAM_BITS> key_mid[KEY_DEPTH];
	ap_uint<CAM_BRAM_BITS> key_hi[KEY_DEPTH];
	ap_uint<LZW_CODE_BITS> values[CAM_VALUES_SIZE]; // 20b used

	// BRAMs for hash table
	ap_uint<HASH_TABLE_ROW_WIDTH> hash_table[HASH_TABLE_SIZE];
	#pragma HLS array_partition variable=hash_table cyclic factor=4 dim=1

	// Other vars
	ap_uint<LZW_KEY_BITS> cur_key;
	uint16_t cur_val_idx = 0;

	// 9-bit matches
	ap_uint<CAM_BRAM_BITS> match_lo;
	ap_uint<CAM_BRAM_BITS> match_hi;
	ap_uint<CAM_BRAM_BITS> match_mid;
	// Init match with all '1', so that we can and against it
	ap_uint<CAM_BRAM_BITS> match;

	ctr = 0;
	lzw_chunk_bit_count = 0;
	lzw_chunk_bit_buffer = 0;

	// Initialize key and value BRAMs to empty
	init_keys:for (int i = 0; i < KEY_DEPTH; ++i) {
		key_lo[i] = 0;
		key_mid[i] = 0;
		key_hi[i] = 0;
	}

	init_table:for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
		#pragma HLS unroll factor=32
		hash_table[i] = 0;
	}

	/* Compress until there's no more data */
	c0 = *inp; // assumes no zero-size input is passed

	bool updated_c0 = false;
	compress:while (chunk_left > 1) {
		#pragma HLS loop_tripcount min=1 avg=3000 max=8191 // worst case input: never find in dict


		int ii = 1;
		c1 = *(inp + 1);
		ap_uint<LZW_CODE_BITS> code = c0; // preserve value from previous loop iter, or initial
		ap_uint<LZW_KEY_BITS> key_to_search = set_code(code, c1);


		ap_uint<9> lo_idx;
		ap_uint<9> mid_idx;
		ap_uint<LZW_KEY_BITS % 9> hi_idx;

		updated_c0 = false;
		compress_inner:while (true) {
			#pragma HLS loop_tripcount min=1 avg=2 max=8064 // worst case input: constant input

			cur_key = key_to_search;
			// hash our current key to see where we are
			ap_uint<LZW_HASH_BITS> cur_key_hash = hash_lzw(cur_key);

			// Look up into hash table
			ap_uint<LZW_CODE_BITS> val_found;
			// look in the hash table
			ap_uint<1> hash_retval = match_hash_table_row(hash_table[cur_key_hash], cur_key,&val_found);

			if (!hash_retval) {
				// We found it in the hash table
				code = val_found;

				ii++;

				if (ii < chunk_left) {
					c0 = *(inp + ii); // store for posterity
					key_to_search = set_code(code, c0);

					updated_c0 = true;
				}
				else {
					break;
				}

			} else {
				ap_uint<HASH_TABLE_COUNTER_BITS> slot = buckets_used_row(hash_table[cur_key_hash]);
				if (slot < NUM_BUCKETS_PER_HASH) {

					// We still have place, store this key in the hash table and break
					ap_uint<HASH_TABLE_ITEM_WIDTH> hash_table_item;
					hash_table_item.range(LZW_KEY_BITS-1, 0) = cur_key;
					hash_table_item.range(HASH_TABLE_ITEM_WIDTH-2, LZW_KEY_BITS) = keys_ind++;
					hash_table_item.set_bit(HASH_TABLE_ITEM_WIDTH-1, 1);
					encode_hash_table(&hash_table[cur_key_hash], hash_table_item, slot);
					break;
				}
				#if ENABLE_CAM
				else {

					// The hash table is full and the item was not found. We have to look into the CAM
					lo_idx = cur_key.range(8, 0);
					mid_idx = cur_key.range(17, 9);
					hi_idx = cur_key.range(LZW_KEY_BITS-1, 18);
					match_lo = key_lo[lo_idx];
					match_mid = key_mid[mid_idx];
					match_hi = key_hi[hi_idx];
					match = match_lo & match_mid & match_hi;

					// Match encode (1-hot)
					if (!match) {
						// Miss!
						if (cur_val_idx == CAM_VALUES_SIZE) {
							// Tough luck!

						} else {


							// Update match BRAMs
							int cur_val_adj = cur_val_idx % CAM_BRAM_BITS;
							key_lo[lo_idx].set(cur_val_adj);
							key_mid[mid_idx].set(cur_val_adj);
							key_hi[hi_idx].set(cur_val_adj);
							// Write to empty values BRAM slot
							values[cur_val_idx++] = keys_ind++;
						}
						break;
					} else {
						// Hit! Read the value
						// match == 1 => values[0] == log 1
						// match == 2 => values[1] == log 2
						// match == 4 => values[2] == log 4
						// match == 8 => values[3] == log 8, etc...
						ap_uint<LZW_CODE_BITS> dm = decode_match(match);
						code = values[dm];

						ii++;

						if (ii < chunk_left) {
							c0 = *(inp + ii); // save for posterity
							key_to_search = set_code(code, c0);

							updated_c0 = true;
						}
						else {
							break;
						}
					}
				}
				#endif
			}
		}


		inp += ii;
		chunk_left -= ii;

		// If we had a hit, then c0 was already updated
		// If we did not record a hit, then c0 should be updated to c1
		if (!updated_c0) {
			c0 = c1;

		}


		if (chunk_left)
			output_code(code, false, lzw_chunk);
		else
			output_code(code, true, lzw_chunk);


	}

	if (chunk_left) {
		if (!updated_c0 && cur_chunk_size > 1) {
			// Note if only 1 byte in chunk, it's still c0
			c0 = c1;

		} else {

		}

		//output_code(c0, true, lzw_chunk);
	}

	*lzw_size = ctr;

}
