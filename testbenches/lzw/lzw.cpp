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
	#if (LZW_KEY_BITS == 21 && LZW_HASH_BITS == 12)
	ap_uint<LZW_HASH_BITS> hash_out;
	hash_out[0x00] = key[0x00] ^ key[0x0e] ^ key[0x0f] ^ key[0x14] ^ key[0x0b] ^
		key[0x06];
	hash_out[0x01] = key[0x14] ^ key[0x08] ^ key[0x09] ^ key[0x12] ^ key[0x0b] ^
		key[0x03];
	hash_out[0x02] = key[0x0b] ^ key[0x01] ^ key[0x02] ^ key[0x13] ^ key[0x0f] ^
		key[0x08];
	hash_out[0x03] = key[0x0b] ^ key[0x02] ^ key[0x07] ^ key[0x03] ^ key[0x0a] ^
		key[0x06];
	hash_out[0x04] = key[0x0f] ^ key[0x0b] ^ key[0x0e] ^ key[0x14] ^ key[0x08] ^
		key[0x02];
	hash_out[0x05] = key[0x05] ^ key[0x0b] ^ key[0x09] ^ key[0x0e] ^ key[0x12] ^
		key[0x00];
	hash_out[0x06] = key[0x06] ^ key[0x08] ^ key[0x05] ^ key[0x09] ^ key[0x03] ^
		key[0x13];
	hash_out[0x07] = key[0x0f] ^ key[0x0b] ^ key[0x10] ^ key[0x0d] ^ key[0x0a] ^
		key[0x05];
	hash_out[0x08] = key[0x03] ^ key[0x10] ^ key[0x04] ^ key[0x0c] ^ key[0x13] ^
		key[0x14];
	hash_out[0x09] = key[0x0d] ^ key[0x0c] ^ key[0x0b] ^ key[0x14] ^ key[0x10] ^
		key[0x06];
	hash_out[0x0a] = key[0x06] ^ key[0x0b] ^ key[0x08] ^ key[0x04] ^ key[0x0f] ^
		key[0x10];
	hash_out[0x0b] = key[0x10] ^ key[0x06] ^ key[0x0a] ^ key[0x0b] ^ key[0x09] ^
		key[0x03];
	#elif (LZW_KEY_BITS == 20 && LZW_HASH_BITS == 12)
	ap_uint<LZW_HASH_BITS> hash_out;
	hash_out[0x00] = key[0x0b] ^ key[0x0e] ^ key[0x0f] ^ key[0x04] ^ key[0x02] ^
		key[0x13];
	hash_out[0x01] = key[0x0a] ^ key[0x10] ^ key[0x0c] ^ key[0x07] ^ key[0x0b] ^
		key[0x04];
	hash_out[0x02] = key[0x07] ^ key[0x12] ^ key[0x0a] ^ key[0x0b] ^ key[0x00] ^
		key[0x11];
	hash_out[0x03] = key[0x06] ^ key[0x0b] ^ key[0x02] ^ key[0x0e] ^ key[0x05] ^
		key[0x13];
	hash_out[0x04] = key[0x08] ^ key[0x0d] ^ key[0x10] ^ key[0x0b] ^ key[0x09] ^
		key[0x13];
	hash_out[0x05] = key[0x0b] ^ key[0x03] ^ key[0x0f] ^ key[0x06] ^ key[0x02] ^
		key[0x00];
	hash_out[0x06] = key[0x0c] ^ key[0x04] ^ key[0x0d] ^ key[0x05] ^ key[0x0b] ^
		key[0x0e];
	hash_out[0x07] = key[0x0c] ^ key[0x10] ^ key[0x09] ^ key[0x12] ^ key[0x0e] ^
		key[0x01];
	hash_out[0x08] = key[0x09] ^ key[0x08] ^ key[0x0c] ^ key[0x06] ^ key[0x12] ^
		key[0x0d];
	hash_out[0x09] = key[0x0b] ^ key[0x01] ^ key[0x03] ^ key[0x13] ^ key[0x06] ^
		key[0x02];
	hash_out[0x0a] = key[0x08] ^ key[0x09] ^ key[0x00] ^ key[0x04] ^ key[0x0d] ^
		key[0x0b];
	hash_out[0x0b] = key[0x0b] ^ key[0x00] ^ key[0x0e] ^ key[0x05] ^ key[0x04] ^
		key[0x0c];
	#elif (LZW_KEY_BITS == 21 && LZW_HASH_BITS == 15)
	ap_uint<LZW_HASH_BITS> hash_out;
	hash_out[0x00] = key[0x0e] ^ key[0x06] ^ key[0x00] ^ key[0x10] ^ key[0x09] ^ key[0x08];
	hash_out[0x01] = key[0x0e] ^ key[0x0d] ^ key[0x11] ^ key[0x09] ^ key[0x01] ^ key[0x13];
	hash_out[0x02] = key[0x06] ^ key[0x08] ^ key[0x0c] ^ key[0x0d] ^ key[0x10] ^ key[0x05];
	hash_out[0x03] = key[0x14] ^ key[0x0a] ^ key[0x07] ^ key[0x02] ^ key[0x09] ^ key[0x05];
	hash_out[0x04] = key[0x06] ^ key[0x14] ^ key[0x07] ^ key[0x11] ^ key[0x0e] ^ key[0x0c];
	hash_out[0x05] = key[0x10] ^ key[0x02] ^ key[0x12] ^ key[0x0b] ^ key[0x03] ^ key[0x05];
	hash_out[0x06] = key[0x0b] ^ key[0x09] ^ key[0x01] ^ key[0x0e] ^ key[0x06] ^ key[0x04];
	hash_out[0x07] = key[0x0c] ^ key[0x00] ^ key[0x08] ^ key[0x0e] ^ key[0x0a] ^ key[0x01];
	hash_out[0x08] = key[0x05] ^ key[0x04] ^ key[0x0b] ^ key[0x0f] ^ key[0x11] ^ key[0x0e];
	hash_out[0x09] = key[0x07] ^ key[0x11] ^ key[0x00] ^ key[0x06] ^ key[0x0c] ^ key[0x13];
	hash_out[0x0a] = key[0x13] ^ key[0x00] ^ key[0x01] ^ key[0x06] ^ key[0x02] ^ key[0x09];
	hash_out[0x0b] = key[0x0b] ^ key[0x04] ^ key[0x0e] ^ key[0x06] ^ key[0x07] ^ key[0x0a];
	hash_out[0x0c] = key[0x00] ^ key[0x12] ^ key[0x0e] ^ key[0x13] ^ key[0x0a] ^ key[0x04];
	hash_out[0x0d] = key[0x0b] ^ key[0x00] ^ key[0x12] ^ key[0x13] ^ key[0x0e] ^ key[0x0c];
	hash_out[0x0e] = key[0x05] ^ key[0x0d] ^ key[0x0e] ^ key[0x02] ^ key[0x08] ^ key[0x13];
	#elif (LZW_KEY_BITS == 21 && LZW_HASH_BITS == 14)
	ap_uint<LZW_HASH_BITS> hash_out;
	hash_out[0x00] = key[0x0e] ^ key[0x11] ^ key[0x01] ^ key[0x04] ^ key[0x00] ^ key[0x12];
	hash_out[0x01] = key[0x07] ^ key[0x04] ^ key[0x00] ^ key[0x0b] ^ key[0x08] ^ key[0x12];
	hash_out[0x02] = key[0x10] ^ key[0x04] ^ key[0x12] ^ key[0x0d] ^ key[0x06] ^ key[0x0f];
	hash_out[0x03] = key[0x0d] ^ key[0x11] ^ key[0x09] ^ key[0x14] ^ key[0x0a] ^ key[0x0c];
	hash_out[0x04] = key[0x10] ^ key[0x0c] ^ key[0x13] ^ key[0x01] ^ key[0x0e] ^ key[0x0d];
	hash_out[0x05] = key[0x06] ^ key[0x01] ^ key[0x03] ^ key[0x10] ^ key[0x0c] ^ key[0x05];
	hash_out[0x06] = key[0x10] ^ key[0x02] ^ key[0x01] ^ key[0x07] ^ key[0x03] ^ key[0x0e];
	hash_out[0x07] = key[0x07] ^ key[0x0d] ^ key[0x0c] ^ key[0x01] ^ key[0x0e] ^ key[0x0b];
	hash_out[0x08] = key[0x09] ^ key[0x0e] ^ key[0x0d] ^ key[0x03] ^ key[0x01] ^ key[0x06];
	hash_out[0x09] = key[0x0d] ^ key[0x12] ^ key[0x0f] ^ key[0x03] ^ key[0x0a] ^ key[0x13];
	hash_out[0x0a] = key[0x0c] ^ key[0x0f] ^ key[0x0d] ^ key[0x09] ^ key[0x12] ^ key[0x14];
	hash_out[0x0b] = key[0x09] ^ key[0x06] ^ key[0x0e] ^ key[0x04] ^ key[0x0d] ^ key[0x10];
	hash_out[0x0c] = key[0x09] ^ key[0x03] ^ key[0x0a] ^ key[0x13] ^ key[0x0d] ^ key[0x10];
	hash_out[0x0d] = key[0x0e] ^ key[0x03] ^ key[0x0d] ^ key[0x11] ^ key[0x07] ^ key[0x0f];
	#elif (LZW_KEY_BITS == 20 && LZW_HASH_BITS == 13)
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
	(*row).range((pos+1) * HASH_TABLE_ITEM_WIDTH - 1,
			  pos * HASH_TABLE_ITEM_WIDTH) = item;
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
		if ((row.range(i * HASH_TABLE_ITEM_WIDTH + LZW_KEY_BITS - 1,
					  i * HASH_TABLE_ITEM_WIDTH) == key) &&
				row.get_bit((i+1)*HASH_TABLE_ITEM_WIDTH -1)) {
			*value_found = row.range((i + 1) * HASH_TABLE_ITEM_WIDTH - 2,
									  i * HASH_TABLE_ITEM_WIDTH + LZW_KEY_BITS);
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
	#if (CAM_VALUES_SIZE == 256)
	ap_uint<128> tmp_0;
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
	tmp_0.set_bit(32,matchbits.get_bit(65));
	tmp_0.set_bit(33,matchbits.get_bit(67));
	tmp_0.set_bit(34,matchbits.get_bit(69));
	tmp_0.set_bit(35,matchbits.get_bit(71));
	tmp_0.set_bit(36,matchbits.get_bit(73));
	tmp_0.set_bit(37,matchbits.get_bit(75));
	tmp_0.set_bit(38,matchbits.get_bit(77));
	tmp_0.set_bit(39,matchbits.get_bit(79));
	tmp_0.set_bit(40,matchbits.get_bit(81));
	tmp_0.set_bit(41,matchbits.get_bit(83));
	tmp_0.set_bit(42,matchbits.get_bit(85));
	tmp_0.set_bit(43,matchbits.get_bit(87));
	tmp_0.set_bit(44,matchbits.get_bit(89));
	tmp_0.set_bit(45,matchbits.get_bit(91));
	tmp_0.set_bit(46,matchbits.get_bit(93));
	tmp_0.set_bit(47,matchbits.get_bit(95));
	tmp_0.set_bit(48,matchbits.get_bit(97));
	tmp_0.set_bit(49,matchbits.get_bit(99));
	tmp_0.set_bit(50,matchbits.get_bit(101));
	tmp_0.set_bit(51,matchbits.get_bit(103));
	tmp_0.set_bit(52,matchbits.get_bit(105));
	tmp_0.set_bit(53,matchbits.get_bit(107));
	tmp_0.set_bit(54,matchbits.get_bit(109));
	tmp_0.set_bit(55,matchbits.get_bit(111));
	tmp_0.set_bit(56,matchbits.get_bit(113));
	tmp_0.set_bit(57,matchbits.get_bit(115));
	tmp_0.set_bit(58,matchbits.get_bit(117));
	tmp_0.set_bit(59,matchbits.get_bit(119));
	tmp_0.set_bit(60,matchbits.get_bit(121));
	tmp_0.set_bit(61,matchbits.get_bit(123));
	tmp_0.set_bit(62,matchbits.get_bit(125));
	tmp_0.set_bit(63,matchbits.get_bit(127));
	tmp_0.set_bit(64,matchbits.get_bit(129));
	tmp_0.set_bit(65,matchbits.get_bit(131));
	tmp_0.set_bit(66,matchbits.get_bit(133));
	tmp_0.set_bit(67,matchbits.get_bit(135));
	tmp_0.set_bit(68,matchbits.get_bit(137));
	tmp_0.set_bit(69,matchbits.get_bit(139));
	tmp_0.set_bit(70,matchbits.get_bit(141));
	tmp_0.set_bit(71,matchbits.get_bit(143));
	tmp_0.set_bit(72,matchbits.get_bit(145));
	tmp_0.set_bit(73,matchbits.get_bit(147));
	tmp_0.set_bit(74,matchbits.get_bit(149));
	tmp_0.set_bit(75,matchbits.get_bit(151));
	tmp_0.set_bit(76,matchbits.get_bit(153));
	tmp_0.set_bit(77,matchbits.get_bit(155));
	tmp_0.set_bit(78,matchbits.get_bit(157));
	tmp_0.set_bit(79,matchbits.get_bit(159));
	tmp_0.set_bit(80,matchbits.get_bit(161));
	tmp_0.set_bit(81,matchbits.get_bit(163));
	tmp_0.set_bit(82,matchbits.get_bit(165));
	tmp_0.set_bit(83,matchbits.get_bit(167));
	tmp_0.set_bit(84,matchbits.get_bit(169));
	tmp_0.set_bit(85,matchbits.get_bit(171));
	tmp_0.set_bit(86,matchbits.get_bit(173));
	tmp_0.set_bit(87,matchbits.get_bit(175));
	tmp_0.set_bit(88,matchbits.get_bit(177));
	tmp_0.set_bit(89,matchbits.get_bit(179));
	tmp_0.set_bit(90,matchbits.get_bit(181));
	tmp_0.set_bit(91,matchbits.get_bit(183));
	tmp_0.set_bit(92,matchbits.get_bit(185));
	tmp_0.set_bit(93,matchbits.get_bit(187));
	tmp_0.set_bit(94,matchbits.get_bit(189));
	tmp_0.set_bit(95,matchbits.get_bit(191));
	tmp_0.set_bit(96,matchbits.get_bit(193));
	tmp_0.set_bit(97,matchbits.get_bit(195));
	tmp_0.set_bit(98,matchbits.get_bit(197));
	tmp_0.set_bit(99,matchbits.get_bit(199));
	tmp_0.set_bit(100,matchbits.get_bit(201));
	tmp_0.set_bit(101,matchbits.get_bit(203));
	tmp_0.set_bit(102,matchbits.get_bit(205));
	tmp_0.set_bit(103,matchbits.get_bit(207));
	tmp_0.set_bit(104,matchbits.get_bit(209));
	tmp_0.set_bit(105,matchbits.get_bit(211));
	tmp_0.set_bit(106,matchbits.get_bit(213));
	tmp_0.set_bit(107,matchbits.get_bit(215));
	tmp_0.set_bit(108,matchbits.get_bit(217));
	tmp_0.set_bit(109,matchbits.get_bit(219));
	tmp_0.set_bit(110,matchbits.get_bit(221));
	tmp_0.set_bit(111,matchbits.get_bit(223));
	tmp_0.set_bit(112,matchbits.get_bit(225));
	tmp_0.set_bit(113,matchbits.get_bit(227));
	tmp_0.set_bit(114,matchbits.get_bit(229));
	tmp_0.set_bit(115,matchbits.get_bit(231));
	tmp_0.set_bit(116,matchbits.get_bit(233));
	tmp_0.set_bit(117,matchbits.get_bit(235));
	tmp_0.set_bit(118,matchbits.get_bit(237));
	tmp_0.set_bit(119,matchbits.get_bit(239));
	tmp_0.set_bit(120,matchbits.get_bit(241));
	tmp_0.set_bit(121,matchbits.get_bit(243));
	tmp_0.set_bit(122,matchbits.get_bit(245));
	tmp_0.set_bit(123,matchbits.get_bit(247));
	tmp_0.set_bit(124,matchbits.get_bit(249));
	tmp_0.set_bit(125,matchbits.get_bit(251));
	tmp_0.set_bit(126,matchbits.get_bit(253));
	tmp_0.set_bit(127,matchbits.get_bit(255));
	ap_uint<64> tmp_1;
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
	ap_uint<2> ttmp_67_66 = matchbits.range(67, 66);
	tmp_1.set_bit(16, ttmp_67_66.or_reduce());
	ap_uint<2> ttmp_71_70 = matchbits.range(71, 70);
	tmp_1.set_bit(17, ttmp_71_70.or_reduce());
	ap_uint<2> ttmp_75_74 = matchbits.range(75, 74);
	tmp_1.set_bit(18, ttmp_75_74.or_reduce());
	ap_uint<2> ttmp_79_78 = matchbits.range(79, 78);
	tmp_1.set_bit(19, ttmp_79_78.or_reduce());
	ap_uint<2> ttmp_83_82 = matchbits.range(83, 82);
	tmp_1.set_bit(20, ttmp_83_82.or_reduce());
	ap_uint<2> ttmp_87_86 = matchbits.range(87, 86);
	tmp_1.set_bit(21, ttmp_87_86.or_reduce());
	ap_uint<2> ttmp_91_90 = matchbits.range(91, 90);
	tmp_1.set_bit(22, ttmp_91_90.or_reduce());
	ap_uint<2> ttmp_95_94 = matchbits.range(95, 94);
	tmp_1.set_bit(23, ttmp_95_94.or_reduce());
	ap_uint<2> ttmp_99_98 = matchbits.range(99, 98);
	tmp_1.set_bit(24, ttmp_99_98.or_reduce());
	ap_uint<2> ttmp_103_102 = matchbits.range(103, 102);
	tmp_1.set_bit(25, ttmp_103_102.or_reduce());
	ap_uint<2> ttmp_107_106 = matchbits.range(107, 106);
	tmp_1.set_bit(26, ttmp_107_106.or_reduce());
	ap_uint<2> ttmp_111_110 = matchbits.range(111, 110);
	tmp_1.set_bit(27, ttmp_111_110.or_reduce());
	ap_uint<2> ttmp_115_114 = matchbits.range(115, 114);
	tmp_1.set_bit(28, ttmp_115_114.or_reduce());
	ap_uint<2> ttmp_119_118 = matchbits.range(119, 118);
	tmp_1.set_bit(29, ttmp_119_118.or_reduce());
	ap_uint<2> ttmp_123_122 = matchbits.range(123, 122);
	tmp_1.set_bit(30, ttmp_123_122.or_reduce());
	ap_uint<2> ttmp_127_126 = matchbits.range(127, 126);
	tmp_1.set_bit(31, ttmp_127_126.or_reduce());
	ap_uint<2> ttmp_131_130 = matchbits.range(131, 130);
	tmp_1.set_bit(32, ttmp_131_130.or_reduce());
	ap_uint<2> ttmp_135_134 = matchbits.range(135, 134);
	tmp_1.set_bit(33, ttmp_135_134.or_reduce());
	ap_uint<2> ttmp_139_138 = matchbits.range(139, 138);
	tmp_1.set_bit(34, ttmp_139_138.or_reduce());
	ap_uint<2> ttmp_143_142 = matchbits.range(143, 142);
	tmp_1.set_bit(35, ttmp_143_142.or_reduce());
	ap_uint<2> ttmp_147_146 = matchbits.range(147, 146);
	tmp_1.set_bit(36, ttmp_147_146.or_reduce());
	ap_uint<2> ttmp_151_150 = matchbits.range(151, 150);
	tmp_1.set_bit(37, ttmp_151_150.or_reduce());
	ap_uint<2> ttmp_155_154 = matchbits.range(155, 154);
	tmp_1.set_bit(38, ttmp_155_154.or_reduce());
	ap_uint<2> ttmp_159_158 = matchbits.range(159, 158);
	tmp_1.set_bit(39, ttmp_159_158.or_reduce());
	ap_uint<2> ttmp_163_162 = matchbits.range(163, 162);
	tmp_1.set_bit(40, ttmp_163_162.or_reduce());
	ap_uint<2> ttmp_167_166 = matchbits.range(167, 166);
	tmp_1.set_bit(41, ttmp_167_166.or_reduce());
	ap_uint<2> ttmp_171_170 = matchbits.range(171, 170);
	tmp_1.set_bit(42, ttmp_171_170.or_reduce());
	ap_uint<2> ttmp_175_174 = matchbits.range(175, 174);
	tmp_1.set_bit(43, ttmp_175_174.or_reduce());
	ap_uint<2> ttmp_179_178 = matchbits.range(179, 178);
	tmp_1.set_bit(44, ttmp_179_178.or_reduce());
	ap_uint<2> ttmp_183_182 = matchbits.range(183, 182);
	tmp_1.set_bit(45, ttmp_183_182.or_reduce());
	ap_uint<2> ttmp_187_186 = matchbits.range(187, 186);
	tmp_1.set_bit(46, ttmp_187_186.or_reduce());
	ap_uint<2> ttmp_191_190 = matchbits.range(191, 190);
	tmp_1.set_bit(47, ttmp_191_190.or_reduce());
	ap_uint<2> ttmp_195_194 = matchbits.range(195, 194);
	tmp_1.set_bit(48, ttmp_195_194.or_reduce());
	ap_uint<2> ttmp_199_198 = matchbits.range(199, 198);
	tmp_1.set_bit(49, ttmp_199_198.or_reduce());
	ap_uint<2> ttmp_203_202 = matchbits.range(203, 202);
	tmp_1.set_bit(50, ttmp_203_202.or_reduce());
	ap_uint<2> ttmp_207_206 = matchbits.range(207, 206);
	tmp_1.set_bit(51, ttmp_207_206.or_reduce());
	ap_uint<2> ttmp_211_210 = matchbits.range(211, 210);
	tmp_1.set_bit(52, ttmp_211_210.or_reduce());
	ap_uint<2> ttmp_215_214 = matchbits.range(215, 214);
	tmp_1.set_bit(53, ttmp_215_214.or_reduce());
	ap_uint<2> ttmp_219_218 = matchbits.range(219, 218);
	tmp_1.set_bit(54, ttmp_219_218.or_reduce());
	ap_uint<2> ttmp_223_222 = matchbits.range(223, 222);
	tmp_1.set_bit(55, ttmp_223_222.or_reduce());
	ap_uint<2> ttmp_227_226 = matchbits.range(227, 226);
	tmp_1.set_bit(56, ttmp_227_226.or_reduce());
	ap_uint<2> ttmp_231_230 = matchbits.range(231, 230);
	tmp_1.set_bit(57, ttmp_231_230.or_reduce());
	ap_uint<2> ttmp_235_234 = matchbits.range(235, 234);
	tmp_1.set_bit(58, ttmp_235_234.or_reduce());
	ap_uint<2> ttmp_239_238 = matchbits.range(239, 238);
	tmp_1.set_bit(59, ttmp_239_238.or_reduce());
	ap_uint<2> ttmp_243_242 = matchbits.range(243, 242);
	tmp_1.set_bit(60, ttmp_243_242.or_reduce());
	ap_uint<2> ttmp_247_246 = matchbits.range(247, 246);
	tmp_1.set_bit(61, ttmp_247_246.or_reduce());
	ap_uint<2> ttmp_251_250 = matchbits.range(251, 250);
	tmp_1.set_bit(62, ttmp_251_250.or_reduce());
	ap_uint<2> ttmp_255_254 = matchbits.range(255, 254);
	tmp_1.set_bit(63, ttmp_255_254.or_reduce());
	ap_uint<32> tmp_2;
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
	ap_uint<4> ttmp_71_68 = matchbits.range(71, 68);
	tmp_2.set_bit(8, ttmp_71_68.or_reduce());
	ap_uint<4> ttmp_79_76 = matchbits.range(79, 76);
	tmp_2.set_bit(9, ttmp_79_76.or_reduce());
	ap_uint<4> ttmp_87_84 = matchbits.range(87, 84);
	tmp_2.set_bit(10, ttmp_87_84.or_reduce());
	ap_uint<4> ttmp_95_92 = matchbits.range(95, 92);
	tmp_2.set_bit(11, ttmp_95_92.or_reduce());
	ap_uint<4> ttmp_103_100 = matchbits.range(103, 100);
	tmp_2.set_bit(12, ttmp_103_100.or_reduce());
	ap_uint<4> ttmp_111_108 = matchbits.range(111, 108);
	tmp_2.set_bit(13, ttmp_111_108.or_reduce());
	ap_uint<4> ttmp_119_116 = matchbits.range(119, 116);
	tmp_2.set_bit(14, ttmp_119_116.or_reduce());
	ap_uint<4> ttmp_127_124 = matchbits.range(127, 124);
	tmp_2.set_bit(15, ttmp_127_124.or_reduce());
	ap_uint<4> ttmp_135_132 = matchbits.range(135, 132);
	tmp_2.set_bit(16, ttmp_135_132.or_reduce());
	ap_uint<4> ttmp_143_140 = matchbits.range(143, 140);
	tmp_2.set_bit(17, ttmp_143_140.or_reduce());
	ap_uint<4> ttmp_151_148 = matchbits.range(151, 148);
	tmp_2.set_bit(18, ttmp_151_148.or_reduce());
	ap_uint<4> ttmp_159_156 = matchbits.range(159, 156);
	tmp_2.set_bit(19, ttmp_159_156.or_reduce());
	ap_uint<4> ttmp_167_164 = matchbits.range(167, 164);
	tmp_2.set_bit(20, ttmp_167_164.or_reduce());
	ap_uint<4> ttmp_175_172 = matchbits.range(175, 172);
	tmp_2.set_bit(21, ttmp_175_172.or_reduce());
	ap_uint<4> ttmp_183_180 = matchbits.range(183, 180);
	tmp_2.set_bit(22, ttmp_183_180.or_reduce());
	ap_uint<4> ttmp_191_188 = matchbits.range(191, 188);
	tmp_2.set_bit(23, ttmp_191_188.or_reduce());
	ap_uint<4> ttmp_199_196 = matchbits.range(199, 196);
	tmp_2.set_bit(24, ttmp_199_196.or_reduce());
	ap_uint<4> ttmp_207_204 = matchbits.range(207, 204);
	tmp_2.set_bit(25, ttmp_207_204.or_reduce());
	ap_uint<4> ttmp_215_212 = matchbits.range(215, 212);
	tmp_2.set_bit(26, ttmp_215_212.or_reduce());
	ap_uint<4> ttmp_223_220 = matchbits.range(223, 220);
	tmp_2.set_bit(27, ttmp_223_220.or_reduce());
	ap_uint<4> ttmp_231_228 = matchbits.range(231, 228);
	tmp_2.set_bit(28, ttmp_231_228.or_reduce());
	ap_uint<4> ttmp_239_236 = matchbits.range(239, 236);
	tmp_2.set_bit(29, ttmp_239_236.or_reduce());
	ap_uint<4> ttmp_247_244 = matchbits.range(247, 244);
	tmp_2.set_bit(30, ttmp_247_244.or_reduce());
	ap_uint<4> ttmp_255_252 = matchbits.range(255, 252);
	tmp_2.set_bit(31, ttmp_255_252.or_reduce());
	ap_uint<16> tmp_3;
	ap_uint<8> ttmp_15_8 = matchbits.range(15, 8);
	tmp_3.set_bit(0, ttmp_15_8.or_reduce());
	ap_uint<8> ttmp_31_24 = matchbits.range(31, 24);
	tmp_3.set_bit(1, ttmp_31_24.or_reduce());
	ap_uint<8> ttmp_47_40 = matchbits.range(47, 40);
	tmp_3.set_bit(2, ttmp_47_40.or_reduce());
	ap_uint<8> ttmp_63_56 = matchbits.range(63, 56);
	tmp_3.set_bit(3, ttmp_63_56.or_reduce());
	ap_uint<8> ttmp_79_72 = matchbits.range(79, 72);
	tmp_3.set_bit(4, ttmp_79_72.or_reduce());
	ap_uint<8> ttmp_95_88 = matchbits.range(95, 88);
	tmp_3.set_bit(5, ttmp_95_88.or_reduce());
	ap_uint<8> ttmp_111_104 = matchbits.range(111, 104);
	tmp_3.set_bit(6, ttmp_111_104.or_reduce());
	ap_uint<8> ttmp_127_120 = matchbits.range(127, 120);
	tmp_3.set_bit(7, ttmp_127_120.or_reduce());
	ap_uint<8> ttmp_143_136 = matchbits.range(143, 136);
	tmp_3.set_bit(8, ttmp_143_136.or_reduce());
	ap_uint<8> ttmp_159_152 = matchbits.range(159, 152);
	tmp_3.set_bit(9, ttmp_159_152.or_reduce());
	ap_uint<8> ttmp_175_168 = matchbits.range(175, 168);
	tmp_3.set_bit(10, ttmp_175_168.or_reduce());
	ap_uint<8> ttmp_191_184 = matchbits.range(191, 184);
	tmp_3.set_bit(11, ttmp_191_184.or_reduce());
	ap_uint<8> ttmp_207_200 = matchbits.range(207, 200);
	tmp_3.set_bit(12, ttmp_207_200.or_reduce());
	ap_uint<8> ttmp_223_216 = matchbits.range(223, 216);
	tmp_3.set_bit(13, ttmp_223_216.or_reduce());
	ap_uint<8> ttmp_239_232 = matchbits.range(239, 232);
	tmp_3.set_bit(14, ttmp_239_232.or_reduce());
	ap_uint<8> ttmp_255_248 = matchbits.range(255, 248);
	tmp_3.set_bit(15, ttmp_255_248.or_reduce());
	ap_uint<8> tmp_4;
	ap_uint<16> ttmp_31_16 = matchbits.range(31, 16);
	tmp_4.set_bit(0, ttmp_31_16.or_reduce());
	ap_uint<16> ttmp_63_48 = matchbits.range(63, 48);
	tmp_4.set_bit(1, ttmp_63_48.or_reduce());
	ap_uint<16> ttmp_95_80 = matchbits.range(95, 80);
	tmp_4.set_bit(2, ttmp_95_80.or_reduce());
	ap_uint<16> ttmp_127_112 = matchbits.range(127, 112);
	tmp_4.set_bit(3, ttmp_127_112.or_reduce());
	ap_uint<16> ttmp_159_144 = matchbits.range(159, 144);
	tmp_4.set_bit(4, ttmp_159_144.or_reduce());
	ap_uint<16> ttmp_191_176 = matchbits.range(191, 176);
	tmp_4.set_bit(5, ttmp_191_176.or_reduce());
	ap_uint<16> ttmp_223_208 = matchbits.range(223, 208);
	tmp_4.set_bit(6, ttmp_223_208.or_reduce());
	ap_uint<16> ttmp_255_240 = matchbits.range(255, 240);
	tmp_4.set_bit(7, ttmp_255_240.or_reduce());
	ap_uint<4> tmp_5;
	ap_uint<32> ttmp_63_32 = matchbits.range(63, 32);
	tmp_5.set_bit(0, ttmp_63_32.or_reduce());
	ap_uint<32> ttmp_127_96 = matchbits.range(127, 96);
	tmp_5.set_bit(1, ttmp_127_96.or_reduce());
	ap_uint<32> ttmp_191_160 = matchbits.range(191, 160);
	tmp_5.set_bit(2, ttmp_191_160.or_reduce());
	ap_uint<32> ttmp_255_224 = matchbits.range(255, 224);
	tmp_5.set_bit(3, ttmp_255_224.or_reduce());
	ap_uint<2> tmp_6;
	ap_uint<64> ttmp_127_64 = matchbits.range(127, 64);
	tmp_6.set_bit(0, ttmp_127_64.or_reduce());
	ap_uint<64> ttmp_255_192 = matchbits.range(255, 192);
	tmp_6.set_bit(1, ttmp_255_192.or_reduce());
	ap_uint<1> tmp_7;
	ap_uint<128> ttmp_255_128 = matchbits.range(255, 128);
	tmp_7.set_bit(0, ttmp_255_128.or_reduce());
	retval.set_bit(0, tmp_0.or_reduce());
	retval.set_bit(1, tmp_1.or_reduce());
	retval.set_bit(2, tmp_2.or_reduce());
	retval.set_bit(3, tmp_3.or_reduce());
	retval.set_bit(4, tmp_4.or_reduce());
	retval.set_bit(5, tmp_5.or_reduce());
	retval.set_bit(6, tmp_6.or_reduce());
	retval.set_bit(7, tmp_7.or_reduce());
#elif (CAM_VALUES_SIZE == 64)
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
	#else
	for (int i = 0; i < 252; i += 6)
		#pragma HLS unroll
		switch (matchbits.range(i+5, i)) {
		case 0x01:
			retval = i;
			break;
		case 0x2:
			retval = i + 1;
			break;
		case 0x04:
			retval = i + 2;
			break;
		case 0x08:
			retval = i + 3;
			break;
		case 0x10:
			retval = i + 4;
			break;
		case 0x20:
			retval = i + 5;
			break;
		}
	switch (matchbits.range(255, 252)) {
		case 0x01:
			retval = 252;
			break;
		case 0x2:
			retval = 253;
			break;
		case 0x04:
			retval = 254;
			break;
		case 0x08:
			retval = 255;
			break;
	}
	#endif
	return retval;
}


void output_code(ap_uint<LZW_CODE_BITS> code,
				 ap_uint<1> is_last_word,
				 unsigned char *buffer)
{
	lzw_chunk_bit_buffer |= (uint32_t)code <<
		(32 - OUTPUT_BITS - lzw_chunk_bit_count);
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
	// uint32_t keys[MAX_CHUNK_SIZE - 1] = {0};
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
	for (int i = 0; i < KEY_DEPTH; ++i) {
		key_lo[i] = 0;
		key_mid[i] = 0;
		key_hi[i] = 0;
	}

	for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
		#pragma HLS unroll factor=32
		hash_table[i] = 0;
	}

	/* Compress until there's no more data */
	c0 = *inp; // assumes no zero-size input is passed
	#if DEBUG_DMA
	#if VERBOSE
	printf("init/ c0 = %d\n", inp);
	#endif
	ctr_cr++;
	#endif
	bool updated_c0 = false;
	while (chunk_left > 1) {
		#pragma HLS loop_tripcount min=1 avg=3000 max=8191 // worst case input: never find in dict

		#if DEBUG_CODE
		printf("--------------------\n");
		printf("chunk_left: %d\n", chunk_left);
		printf("REMAINING: ");
		printf("%s\n", inp);
		#endif
		#if DEBUG_DMA & VERBOSE
		printf("(tow): c0 should be at %d\n", inp);
		#endif
		int ii = 1;
		c1 = *(inp + 1);
		ap_uint<LZW_CODE_BITS> code = c0; // preserve value from previous loop iter, or initial
		ap_uint<LZW_KEY_BITS> key_to_search = set_code(code, c1);
		#if DEBUG_DMA
		assert ((c0 == *inp) && (c1 == *(inp + 1)));
		#if VERBOSE
		printf("0/ c1 = %d\n", inp + 1);
		#endif
		ctr_cr++;
		#endif

		ap_uint<9> lo_idx;
		ap_uint<9> mid_idx;
		ap_uint<LZW_KEY_BITS % 9> hi_idx;

		updated_c0 = false;
		while (true) {
			#pragma HLS loop_tripcount min=1 avg=2 max=8064 // worst case input: constant input

			cur_key = key_to_search;
			ap_uint<LZW_HASH_BITS> cur_key_hash = hash_lzw(cur_key);

			// Look up into hash table
			ap_uint<LZW_CODE_BITS> val_found;
			ap_uint<1> hash_retval = match_hash_table_row(hash_table[cur_key_hash], cur_key,
								 	 	 	 	 	 	 &val_found);

			if (!hash_retval) {
				// We found it in the hash table
				code = val_found;
				#if DEBUG_CAM_HASH
				printf("Hit in hash table. Retrieved code %u\n", (unsigned int)code);
				#endif
				ii++;

				if (ii < chunk_left) {
					c0 = *(inp + ii); // store for posterity
					key_to_search = set_code(code, c0);
					#if DEBUG_DMA
					#if VERBOSE
					printf("HH / c0 = %d\n", inp + ii);
					#endif
					ctr_cr++;
					#endif
					updated_c0 = true;
				}
				else {
					break;
				}

			} else {
				ap_uint<HASH_TABLE_COUNTER_BITS> slot = buckets_used_row(hash_table[cur_key_hash]);
				if (slot < NUM_BUCKETS_PER_HASH) {
					#if DEBUG_CAM_HASH
					printf("Key not found. Filling hash 0x%x - spot %d - value %d\n",
						   (unsigned long)cur_key_hash, (int)slot, keys_ind);
					#endif
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
					#if DEBUG_CAM_HASH
					printf("Hash table full, CAM lookup\n");
					#endif

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
							#if DEBUG_CAM_HASH
							printf("CAM full. This should not happen\n");
							exit(1);
							#endif
						} else {
							#if DEBUG_CAM_HASH
							printf("CAM Miss. Filling code 0x%04x: 0x%04x - 0x%02x\n",
									keys_ind, get_code(cur_key),
									cur_key & 0xFF);
							#endif

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
						#if DEBUG_CAM_HASH
						printf("CAM hit. Retrieved code %d from dm %d\n", code, dm);
						#endif
						ii++;

						if (ii < chunk_left) {
							c0 = *(inp + ii); // save for posterity
							key_to_search = set_code(code, c0);
							#if DEBUG_DMA
							#if VERBOSE
							printf("CH / c0 = %d\n", inp + ii);
							#endif
							ctr_cr++;
							#endif
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

		#if DEBUG_CODE
		printf("OUPTUT: 0x%02x %d\n", (unsigned int)code, (unsigned int)code);
		#endif

		inp += ii;
		chunk_left -= ii;

		// If we had a hit, then c0 was already updated
		// If we did not record a hit, then c0 should be updated to c1
		if (!updated_c0) {
			c0 = c1;
			#if DEBUG_DMA && VERBOSE
			printf("(eow): c0 updated to c1; i.e. ii should be 1: %d\n", ii);
			#endif
		}
		#if DEBUG_DMA
		if (chunk_left > 1 && c0 != *inp) {
			// if chunk_left == 0, no input left to read
			assert(c0 == *inp);
		}
		#endif

		if (chunk_left)
			output_code(code, false, lzw_chunk);
		else
			output_code(code, true, lzw_chunk);

		#if DEBUG_CODE && 0
		printf("DICT:\n");
		for (int i = 0; i < keys_ind; i++)
			printf("0x%04x: 0x%04x - 0x%02x (%c)\n", values[i],
				   get_code(keys[i]), keys[i] & 0xFF, keys[i] & 0xFF);
		printf("\n");
		#endif
	}

	if (chunk_left) {
		if (!updated_c0 && cur_chunk_size > 1) {
			// Note if only 1 byte in chunk, it's still c0
			c0 = c1;
			#if DEBUG_DMA & VERBOSE
			printf("End w/leftover; already c1 read final at %d\n", inp);
			#endif
		} else {
			#if DEBUG_DMA & VERBOSE
			printf("End w/leftover; already c0 read final at %d\n", inp);
			#endif
		}
		#if DEBUG_DMA
		assert(c0 == *inp);
		// if (c0 != *inp) {
		// 	printf("~~~err EOF c0 != *inp; inp is %d\n", inp);
		// }
		#endif
		//output_code(c0, true, lzw_chunk);
	}

	*lzw_size = ctr;

	#if DEBUG_DMA
	printf("****************\nctr_cr / size: %d, %d\n**********\n",
		   ctr_cr, cur_chunk_size);
	#endif
}
