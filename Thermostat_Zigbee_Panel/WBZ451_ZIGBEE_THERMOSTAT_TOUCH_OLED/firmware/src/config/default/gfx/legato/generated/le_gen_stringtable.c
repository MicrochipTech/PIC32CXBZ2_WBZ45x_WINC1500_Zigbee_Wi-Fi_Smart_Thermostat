#include "gfx/legato/generated/le_gen_assets.h"

/*****************************************************************************
 * Legato String Table
 * Encoding        ASCII
 * Language Count: 1
 * String Count:   2
 *****************************************************************************/

/*****************************************************************************
 * string table data
 * 
 * this table contains the raw character data for each string
 * 
 * unsigned short - number of indices in the table
 * unsigned short - number of languages in the table
 * 
 * index array (size = number of indices * number of languages
 * 
 * for each index in the array:
 *   unsigned byte - the font ID for the index
 *   unsigned byte[3] - the offset of the string codepoint data in
 *                      the table
 * 
 * string data is found by jumping to the index offset from the start
 * of the table
 * 
 * string data entry:
 *     unsigned short - length of the string in bytes (encoding dependent)
 *     codepoint data - the string data
 ****************************************************************************/

const uint8_t stringTable_data[34] =
{
    0x02,0x00,0x01,0x00,0x01,0x0C,0x00,0x00,0x00,0x14,0x00,0x00,0x06,0x00,0x57,0x42,
    0x5A,0x34,0x35,0x31,0x0B,0x00,0x5A,0x49,0x47,0x42,0x45,0x45,0x20,0x44,0x65,0x6D,
    0x6F,0x00,
};

/* font asset pointer list */
leFont* fontList[2] =
{
    (leFont*)&NotoSans_Bold,
    (leFont*)&NotoMono_Regular,
};

const leStringTable stringTable =
{
    {
        LE_STREAM_LOCATION_ID_INTERNAL, // data location id
        (void*)stringTable_data, // data address pointer
        34, // data size
    },
    (void*)stringTable_data, // string table data
    fontList, // font lookup table
    LE_STRING_ENCODING_ASCII // encoding standard
};


// string list
leTableString string_txtString2;
leTableString string_txtString1;

void initializeStrings(void)
{
    leTableString_Constructor(&string_txtString2, stringID_txtString2);
    leTableString_Constructor(&string_txtString1, stringID_txtString1);
}
