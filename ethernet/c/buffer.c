/**
 Copyright (C) 2015 Hui Chen <huichen AT ieee DOT org>
 
 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Description */
/* 
 * dump the buffer in a human-readable form to stdout
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "buffer.h"

#define PRINTF printf

void dumpbuf(char *buf, int nrecv)
{
#define LINE_WIDTH        74
#define INDEX_LEN        4
#define INDEX_GAP_LEN    2
#define COLUMN_GAP_LEN    4
#define HEX_COLUMN_START    (INDEX_LEN + INDEX_GAP_LEN)
#define HEX_COLUMN_WIDTH    \
                ((LINE_WIDTH - HEX_COLUMN_START - COLUMN_GAP_LEN)/4*3)
#define PRINT_COLUMN_START    \
                (HEX_COLUMN_START + HEX_COLUMN_WIDTH + COLUMN_GAP_LEN)
#define PRINT_COLUMN_WIDTH    (HEX_COLUMN_WIDTH/3)
#define    CHAR_PER_LINE    (PRINT_COLUMN_WIDTH)
            
    int i = 0, hexpos = 0, printpos = 0;
    char idxbuf[INDEX_LEN+1], 
         hexbuf[HEX_COLUMN_WIDTH+1], 
         printbuf[PRINT_COLUMN_WIDTH+1];

    sprintf(idxbuf, "%0*d", INDEX_LEN, i);
    while (nrecv > 0 && i < nrecv) {
        if (i % CHAR_PER_LINE == 0) {
            printbuf[printpos] = '\0';
            hexpos = 0;
            printpos = 0;
            if (i > 0)
                PRINTF("%s%*c%-*s%*c%s\n", idxbuf, INDEX_GAP_LEN, ' ', 
                        HEX_COLUMN_WIDTH, hexbuf, COLUMN_GAP_LEN,
                        ' ', printbuf);
            sprintf(idxbuf, "%0*x", INDEX_LEN, i);
        }
        hexpos += sprintf(hexbuf+hexpos, "%02x ", (unsigned char)buf[i]); 
        if (isprint(buf[i]))
            printbuf[printpos ++] = buf[i];
        else
            printbuf[printpos ++] = '.';
        i ++;
    }

    if (nrecv > 0) {
        printbuf[printpos] = '\0';
        PRINTF("%s%*c%-*s%*c%s\n", idxbuf, INDEX_GAP_LEN, ' ', 
                HEX_COLUMN_WIDTH, hexbuf, COLUMN_GAP_LEN, ' ', printbuf);
    }
}


char *safe_strncpy(char *dst, const char *src, size_t size)
{
    dst[size-1] = '\0';
    return strncpy(dst, src, size-1);
}




