/*
$Id: genpsi.c,v 1.1 2004/10/07 13:12:38 essu Exp $

 Copyright (c) 2004 gmo18t, Germany. All rights reserved.

 aktuelle Versionen gibt es hier:
 $Source: /home/xubuntu/berlios_backup/github/tmp-cvs/yadi/Repository/tools/genpsi/genpsi.c,v $

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 675 Mass Ave, Cambridge MA 02139, USA.

 Mit diesem Programm koennen Neutrino TS Streams f�r das Abspielen unter Enigma gepatched werden 
 */
//#define WIN32
#ifdef WIN32
#define O_LARGEFILE O_BINARY
#else
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#define SIZE_TS_PKT           188
#define SIZE_PROBE            (10000*SIZE_TS_PKT)

#define OFS_HDR_1             0
#define OFS_HDR_2             5
#define OFS_HDR 3             8

#define OFS_PAT_DATA	      13

#define OFS_PMT_DATA	      13
#define OFS_STREAM_TAB        17
#define SIZE_STREAM_TAB_ROW   5  

#define OFS_ENIGMA_DATA       13
#define OFS_ENIGMA_TAB        27
#define SIZE_ENIGMA_TAB_ROW   4  

#define ES_TYPE_MPEG12        0x02
#define ES_TYPE_MPA           0x03
#define ES_TYPE_AAC           0x0F
#define ES_TYPE_AC3           0x81

#define EN_TYPE_VIDEO	      0x00
#define EN_TYPE_AUDIO	      0x01
#define EN_TYPE_TELTEX        0x02
#define EN_TYPE_PCR	      0x03

#define SID_PRIVATE_STREAM1   0xBD
#define SID_VIDEO_STREAM_S    0xE0
#define SID_VIDEO_STREAM_E    0xEF
#define SID_AUDIO_STREAM_S    0xC0
#define SID_AUDIO_STREAM_E    0xDF


typedef struct 
{
  int      nbv;
  int      nba;
  uint16_t vpid;
  uint16_t apid[10];
  int      isAC3[10];
} T_AV_PIDS;

static const uint32_t crc_table[256] = 
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

//-- special enigma stream description packet for  --
//-- at least 1 video, 1 audo and 1 PCR-Pid stream --
//---------------------------------------------------
static uint8_t pkt_enigma[] =
{
  0x47, 0x40, 0x1F, 0x10, 0x00, // HEADER-1
  0x7F, 0x80, 0x24,             // HEADER-2, len=0x24 (1 video, 1 audio, 1 PCR)
  0x00, 0x00, 0x01, 0x00, 0x00, // HEADER-3
  
  0x00, 0x00, 0x6D, 0x66, 0x30, 0x19,        // ..., TSID(16), ...
  0x80, 0x13, 'E', 'N', 'I', 'G', 'M', 'A',  // tag(8), len(8), text(6) 
  
  0x00, 0x02, 0x00, 0x6e,          // cVPID(8), len(8), PID(16)
  0x01, 0x03, 0x00, 0x78, 0x00,    // cAPID(8), len(8), PID(16), ac3flag(8)
//  0x02, 0x02, 0x00, 0x82,        // cTPID(8), len(8), ... (not used)
  0x03, 0x02, 0x00, 0x6e           // cPCRPID(8), ...
};  

//-- PAT packet for at least 1 PMT --
//-----------------------------------
static uint8_t pkt_pat[] =
{
  0x47, 0x40, 0x00, 0x10, 0x00,  // HEADER-1
  0x00, 0xB0, 0x0D,              // HEADER-2, len=0x0D (1 PMT record)
  0x04, 0x37, 0xE9, 0x00, 0x00,  // HEADER-3
  0x6D, 0x66, 0xEF, 0xFF,        // PAT-DATA, 1 PMT record (with PID=0xFFF)
};

//-- PMT packet for at least 1 video and 1 audio stream --          
//--------------------------------------------------------
static uint8_t pkt_pmt[] =
{
  0x47, 0x4F, 0xFF, 0x10, 0x00,  // HEADER-1
  0x02, 0xB0, 0x17,              // HEADER-2, len=0x17 (1 video, 1 audio)
  0x04, 0x37, 0xE9, 0x00, 0x00,  // HEADER-3
  0xE0, 0x00, 0xF0, 0x00,        // PMT-DATA  
  0x02, 0xE0, 0x00, 0xF0, 0x00,  //   (video stream 1)
  0x03, 0xE0, 0x00, 0xF0, 0x00   //   (audio stream 1)
};  

//== calculates CRC value on bufferd data ==
//========================================== 
static uint32_t calc_crc32(uint8_t *dst, const uint8_t *src, uint32_t len)
{  
  uint32_t i;
  uint32_t crc = 0xffffffff;
    
  for (i=0; i<len; i++)
    crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *src++) & 0xff];
    
  if (dst)
  {
    dst[0] = (crc >> 24) & 0xff;
    dst[1] = (crc >> 16) & 0xff;
    dst[2] = (crc >> 8) & 0xff;
    dst[3] = (crc) & 0xff;
  }
    
  return crc;
}


//== sync to next TS packet frpm "pos"     ==
//== returns offset to first TS packet or  ==
//== (off_t)-1 on failure.                 ==
//== read position will be set on sync pos ==
//=========================================== 
static off_t sync_pkt(int fd, off_t pos)
{
  off_t   npos = pos;
  uint8_t pkt[SIZE_TS_PKT];
 
  lseek(fd, npos, SEEK_SET); 
  while ( read(fd, pkt, 1) > 0 )
  {
    //-- check every byte for sync word --
    npos++;
    if (*pkt == 0x47) 
    {
      //-- if found double check for next sync word --
      if ( read(fd, pkt, SIZE_TS_PKT) == SIZE_TS_PKT )
      {
        if (pkt[SIZE_TS_PKT-1] == 0x47) 
          return lseek(fd, npos-1, SEEK_SET); // assume sync ok
        else
          lseek(fd, npos, SEEK_SET);  // ups, next pkt doesn't start with sync 
      }
    }
    
    //-- check probe limits --
    if (npos > (pos+SIZE_PROBE) ) break; 
  }
  
  return lseek(fd, 0, SEEK_CUR);
}

//== decode PID from 188 byte TS packet ==
//========================================
static uint16_t decode_pid(uint8_t *pkt)
{
  uint16_t pp = 0;
  
  pp  = (pkt[1] & 0x1F)<<8;
  pp |= pkt[2];
  
  return pp;
}

//== scan for all A/V PIDs in TS stream ==
//== (limited by a certain range)       ==
//========================================
static void get_avpids(int fd, T_AV_PIDS *avPids)
{
  uint8_t pkt[SIZE_TS_PKT];
  off_t   pos;
  
  int      ac3flag  = 0;
  int      apid_cnt = 0;
  uint16_t apid     = 0;
  
  avPids->nbv     = 0;
  avPids->nba     = 0;
  avPids->apid[0] = 0;
  avPids->vpid    = 0;

  //-- sync to first valid TS packet --
  //-----------------------------------
  if ( (pos = sync_pkt(fd, 0)) == (off_t)-1 ) return; 

  //-- search inside of probe limits as long min 1 apid and 1 vpid detected --
  //-------------------------------------------------------------------------- 
  while ( avPids->nba == 0 || avPids->nbv == 0 || pos < SIZE_PROBE )
  {
    //-- try to read a complete TS packet --
    if ( read(fd, pkt, SIZE_TS_PKT) != SIZE_TS_PKT ) return;
    
    //-- on sync --
    //-------------
    if (pkt[0] == 0x47)
    {
      pos += SIZE_TS_PKT;
      
      if (pkt[1] & 0x40)
      {
        int i = 0;
        
        //-- check adaption field --
        int ofs = ( pkt[3] & 0x20 )? ofs = pkt[4] + 1 : 0;
          
        //-- select stream ID --
        switch(pkt[7+ofs])
        {
          //-- Video --
          case SID_VIDEO_STREAM_S ... SID_VIDEO_STREAM_E:
              avPids->nbv  = 1;
              avPids->vpid = decode_pid(pkt);
              break;
           
          //-- Audio AC3 --
          case SID_PRIVATE_STREAM1:
              ac3flag = 1;
              // no break;
              
          //-- Audio MPA --  
          case SID_AUDIO_STREAM_S ... SID_AUDIO_STREAM_E:
              apid = decode_pid(pkt);
              for (i=0; i<apid_cnt; i++) if (avPids->apid[i] == apid) break;
              
              //-- store new apid --
              if (i==apid_cnt)
              {
                avPids->apid[apid_cnt]  = apid;
                avPids->isAC3[apid_cnt] = ac3flag;
                avPids->nba++;
                apid_cnt++;
              }      
              ac3flag = 0;
              break;
        }
      }
    }
    //-- otherway do resync --
    //------------------------
    else
      pos = sync_pkt(fd, pos+1);
  }
}


//== setup a new TS packet with format ==
//== predefined with a template        ==
//=======================================
#define COPY_TEMPLATE(dst, src) copy_template(dst, src, sizeof(src))

static int copy_template(uint8_t *dst, uint8_t *src, int len)
{
  //-- reset buffer --
  memset(dst, 0xFF, SIZE_TS_PKT);

  //-- copy PMT template --
  memcpy(dst, src, len);

  return len;
}

//==========
//== MAIN ==
//==========
int main(int argc, char **argv)
{
  int   fd, bytes = 0;
  off_t pos;
  char  *fname;
    
  uint8_t   pkt[SIZE_TS_PKT];
  int       i, data_len, patch_len, ofs;
  T_AV_PIDS avPids;

  //-- parse command line --
  //------------------------
  if (argc<2)
  { 
    fprintf(stderr,"\nusage: genpsi <tsfile-to-patch>\n\n");
    return 0;
  }

  fname = argv[1];
  
  //-- open ts file r/w for analyzing/patching --
  //---------------------------------------------
  if ( (fd = open(fname, O_RDWR | O_LARGEFILE)) < 0 )
  {
    fprintf(stderr, "couldn't open (%s)\n", fname);
    perror("");
    return 0;
  }
  
  //-- scan a part to determine all A/V Pids --
  //-------------------------------------------
  get_avpids(fd, &avPids);
  if ( avPids.nbv==0 || avPids.nba==0 )
  {
    close (fd);
    fprintf(stderr, "not enough A/V PIDs found - Audio (%d), Video (%d)\n",
            avPids.nba, avPids.nbv);
    return 0;
  }

#if 0  
  // Test 
  avPids.nba      = 2;
  avPids.apid[1]  = 0x555;
  avPids.isAC3[1] = 1;
#endif

  //-- show PIDs found --
  //---------------------   
  fprintf
  (
    stderr, 
    "\nfile (%s) analyzed - detected PIDs:\n\n"
    " Video: (0x%x)\n" 
    " Audio: ", 
    fname, avPids.vpid
  );
                  
  for (i=0; i<avPids.nba; i++)                
    fprintf(stderr, "(0x%x)\n        ", avPids.apid[i]);
  fprintf(stderr,"\n");    


  //-- seek synced to file start ready to patch --
  //----------------------------------------------
  pos = sync_pkt(fd, 0);
  fprintf(stderr, "synced to pos (%ld) - start patching ...\n", pos);
  
  //-- (I) build "Enigma" --
  //------------------------
  
  //-- copy "Enigma"-template --
  data_len = COPY_TEMPLATE(pkt, pkt_enigma);
  
  //-- adjust len dependent to number of audio streams --
  data_len += ((SIZE_ENIGMA_TAB_ROW+1) * (avPids.nba-1));
  
  patch_len = data_len - OFS_HDR_2 + 1;
  pkt[OFS_HDR_2+1] |= (patch_len>>8);
  pkt[OFS_HDR_2+2]  = (patch_len & 0xFF); 

  //-- write row with desc. for video stream --  
  ofs = OFS_ENIGMA_TAB;
  pkt[ofs]   = EN_TYPE_VIDEO;
  pkt[ofs+1] = 0x02;
  pkt[ofs+2] = (avPids.vpid>>8);
  pkt[ofs+3] = (avPids.vpid & 0xFF);

  //-- for each audio stream, write row with desc. --
  ofs += SIZE_ENIGMA_TAB_ROW;  
  for (i=0; i<avPids.nba; i++)
  {
    pkt[ofs]   = EN_TYPE_AUDIO;
    pkt[ofs+1] = 0x03;
    pkt[ofs+2] = (avPids.apid[i]>>8);
    pkt[ofs+3] = (avPids.apid[i] & 0xFF);
    pkt[ofs+4] = (avPids.isAC3[i]==1)? 0x01 : 0x00;
  
    ofs += (SIZE_ENIGMA_TAB_ROW + 1);
  }
  
  //-- write row with desc. for pcr stream (eq. video) -- 
  pkt[ofs]   = EN_TYPE_PCR;
  pkt[ofs+1] = 0x02;
  pkt[ofs+2] = (avPids.vpid>>8);
  pkt[ofs+3] = (avPids.vpid & 0xFF);
   
  //-- calculate CRC --
  calc_crc32(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );
  
  //-- write TS packet --
  bytes += write(fd, pkt, SIZE_TS_PKT);
  
  //-- (II) build PAT --
  //--------------------
  data_len = COPY_TEMPLATE(pkt, pkt_pat);
  
  //-- calculate CRC --
  calc_crc32(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );

  //-- write TS packet --
  bytes += write(fd, pkt, SIZE_TS_PKT);
  
  //-- (III) build PMT --
  //---------------------
  data_len = COPY_TEMPLATE(pkt, pkt_pmt);
  
  //-- adjust len dependent to count of audio streams --
  data_len += (SIZE_STREAM_TAB_ROW * (avPids.nba-1));
  
  patch_len = data_len - OFS_HDR_2 + 1;
  pkt[OFS_HDR_2+1] |= (patch_len>>8);
  pkt[OFS_HDR_2+2]  = (patch_len & 0xFF); 

  //-- patch pcr PID --
  ofs = OFS_PMT_DATA;
  pkt[ofs]  |= (avPids.vpid>>8);
  pkt[ofs+1] = (avPids.vpid & 0xFF);
  
  //-- write row with desc. for ES video stream --
  ofs = OFS_STREAM_TAB;
  pkt[ofs]   = ES_TYPE_MPEG12;
  pkt[ofs+1] = 0xE0 | (avPids.vpid>>8);
  pkt[ofs+2] = (avPids.vpid & 0xFF);
  pkt[ofs+3] = 0xF0;
  pkt[ofs+4] = 0x00;
  
  //-- for each ES audio stream, write row with desc. --
  for (i=0; i<avPids.nba; i++)
  {
    ofs += SIZE_STREAM_TAB_ROW;
    pkt[ofs]   = (avPids.isAC3[i]==1)? ES_TYPE_AC3 : ES_TYPE_MPA;
    pkt[ofs+1] = 0xE0 | (avPids.apid[i]>>8);
    pkt[ofs+2] = (avPids.apid[i] & 0xFF);
    pkt[ofs+3] = 0xF0;
    pkt[ofs+4] = 0x00;
  }
   
  //-- calculate CRC --
  calc_crc32(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );
  
  //-- write TS packet --
  bytes += write(fd, pkt, SIZE_TS_PKT);
  
  //-- finish --
  close(fd);
  fprintf(stderr, "... EDT/PMT/PAT (%d bytes) write done !\n\n", bytes);
  
  return 1;
}
