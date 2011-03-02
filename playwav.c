/* Copyright (C) 2008 The Android Open Source Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/ioctl.h>
#include <linux/i2c.h>

#include "sound.h"
#include "msm_audio.h"
#include "msm8k_cad_devices.h"

#if 1
#define D(fmt, args...) printf("%s():"fmt, __FUNCTION__  ,##args)
#define DS(fmt, args...) printf(fmt, ##args)
#else
#define D(fmt, args...) do {} while (0)
#endif

#define RED      "\x1b[31m"
#define DEFAULT  "\x1b[39m"

#define DEBUG_DUMP_START                 0
#define DEBUG_DUMP_END               20000 // sizeof(struct cmd_dump)
#define DEBUG_DUMP_READ_MAX          20000 // sizeof(struct ma_IoCtlReadRegWait)
#define DEBUG_DUMP_WRITE_MAX         20000 // sizeof(struct ma_IoCtlWriteRegWait)

#define DEBUG_DATA_DUMP_WRITE_MAX   100000 // unsigned char u_int8 and unsigned short u_int16
#define DEBUG_DATA_DUMP_READ_MAX    100000 // unsigned char u_int8 and unsigned short u_int16

#define AE2_INIT_START_COUNT              1
#define AE2_INIT_END1_COUNT            3495 // I2C アクセス直前まで
#define AE2_INIT_END2_COUNT            4562 // 初期化完了まで

#define AE2_SERVICE_START_COUNT   4563
#define AE2_SERVICE_END_COUNT     5725

#define MA_IOC_MAGIC                     'x'
#define MA_IOCTL_WAIT                    _IOW(  MA_IOC_MAGIC, 0, unsigned int )
#define MA_IOCTL_SLEEP 	         _IOW(  MA_IOC_MAGIC, 1, unsigned int )
#define MA_IOCTL_WRITE_REG_WAIT          _IOW(  MA_IOC_MAGIC, 2, struct ma_IoCtlWriteRegWait )
#define MA_IOCTL_READ_REG_WAIT           _IOWR( MA_IOC_MAGIC, 3, struct ma_IoCtlReadRegWait )
#define MA_IOCTL_DISABLE_IRQ 	         _IO(   MA_IOC_MAGIC, 4 )
#define MA_IOCTL_ENABLE_IRQ 	         _IO(   MA_IOC_MAGIC, 5 )
#define MA_IOCTL_RESET_IRQ_MASK_COUNT    _IO(   MA_IOC_MAGIC, 6 )
#define MA_IOCTL_WAIT_IRQ 	         _IOR(  MA_IOC_MAGIC, 7, int )
#define MA_IOCTL_CANCEL_WAIT_IRQ         _IO(   MA_IOC_MAGIC, 8 )
#define MA_IOCTL_SET_GPIO 	         _IOW(  MA_IOC_MAGIC, 9, unsigned int )



/*// kernel ヘッダファイルをincludeしたのでコメントアウト
struct msm_audio_config {
    uint32_t buffer_size;
    uint32_t buffer_count;
    uint32_t channel_count;
    uint32_t sample_rate;
    uint32_t codec_type;
    uint32_t unused[3];
};

struct msm_audio_stats {
    uint32_t out_bytes;
    uint32_t unused[3];
};
*/

//-----------------------------------------------------------------------------------
// 型宣言
void mem_data();

struct ae2_driver_info {
    int fd; // /dev/ae2 の fd
    int dIrqCount;
    int init_flag; // 初期化完了フラグ
} ae2_info;

char* cmdToText[10] ={ 
    "MA_IOCTL_WAIT",
    "MA_IOCTL_SLEEP",
    "MA_IOCTL_WRITE_REG_WAIT",
    "MA_IOCTL_READ_REG_WAIT",
    "MA_IOCTL_DISABLE_IRQ",
    "MA_IOCTL_ENABLE_IRQ",
    "MA_IOCTL_RESET_IRQ_MASK_COUNT",
    "MA_IOCTL_WAIT_IRQ",
    "MA_IOCTL_CANCEL_WAIT_IRQ",
    "MA_IOCTL_SET_GPIO" };

struct cmd_dump *dump=NULL;
struct ma_IoCtlWriteRegWait *dumpWrite=NULL;
struct ma_IoCtlReadRegWait *dumpRead=NULL;
unsigned char  *dumpWriteByte = NULL;
unsigned short *dumpWriteWord = NULL;
unsigned char  *dumpReadByte = NULL;
unsigned short *dumpReadWord = NULL;

int readCount=0;
int readDataMode=0;
unsigned int readDataCount=0;

struct ioctl_count ReadWriteCount = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
                   ioctlCountNow  = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };



//--------------------------------------------------    
int pcm_play(unsigned rate, unsigned channels,
             int (*fill)(void *buf, unsigned sz, void *cookie),
             void *cookie)
{
    struct msm_audio_config config;
    struct msm_audio_stats stats;
    unsigned sz, n,count;
    char buf[8192],i2c_buf[4];
    int i,afd,i2c;
    int addr = 0x5b; /* I2C address */
    int i2c_flag=-1;

    afd = open("/dev/msm_pcm", O_RDWR);
    if (afd < 0) {
        perror("pcm_play: cannot open audio device");
        return -1;
    }

    if(ioctl(afd, AUDIO_GET_CONFIG, &config)) {
        perror("could not get config");
        return -1;
    }

    config.channel_count = channels;
    config.sample_rate = rate;
    if (ioctl(afd, AUDIO_SET_CONFIG, &config)) {
        perror("could not set config");
        return -1;
    }
    sz = config.buffer_size;
    if (sz > sizeof(buf)) {
        fprintf(stderr,"too big\n");
        return -1;
    }

    fprintf(stderr,"prefill\n");
    for (n = 0; n < config.buffer_count; n++) {
        if (fill(buf, sz, cookie))
            break;
	/*
        if (write(afd, buf, sz) != sz)
            break;
	*/
    }

    fprintf(stderr,"start\n");
    ioctl(afd, AUDIO_START, 0);
    //-------------------------------------------------------
    i2c = open("/dev/i2c-0", O_RDWR);
    if (i2c < 0) {
        perror("pcm_play: cannot open /dev/i2c-0 device");
    }
    else {
      if( (i2c_flag=ioctl( i2c , I2C_SLAVE, addr)) ==0 ) {
	count = read( i2c , i2c_buf , 3 );
	if( count == 3 ) {
	  printf("1 i2c-0 read success\n");
	  for(i=0;i<(int)count;i++) {
	    printf("0x%02X\n",i2c_buf[i] );
	  }
	  if( i2c_buf[0] == 0x08 && 
	      i2c_buf[1] == 0xCD &&
	      i2c_buf[2] == 0x00 )
	      {
		 i2c_buf[0]=0x00;
		 i2c_buf[1]=0x0A;
		 if( write( i2c , i2c_buf , 2 ) == 2 )
		   printf(" i2c-0 write success\n");
		 else
		   printf(" i2c-0 write failed\n");
		 count = 2U;
		 for(i=0;i<(int)count;i++) {
		   printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
		 }
	      }
	}
           count = read( i2c , i2c_buf , 3 );
           if( count == 3 ) {
	     printf("2 i2c-0 read success\n");
	     for(i=0;i<(int)count;i++) {
                         printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
	     }
	     if( i2c_buf[0] == 0x0A && 
	         i2c_buf[1] == 0xCD &&
	         i2c_buf[2] == 0x00 )
	       {
		 i2c_buf[0]=0x01;
		 i2c_buf[1]=0xCD;
		 if( write( i2c , i2c_buf , 2 ) == 2 )
		   printf(" i2c-0 write success\n");
		 else
		   printf(" i2c-0 write failed\n");
		 count = 2U;
		 for(i=0;i<(int)count;i++) {
		   printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
		 }
	       }
           }

           count = read( i2c , i2c_buf , 3 );
           if( count == 3 ) {
	     printf("3 i2c-0 read success\n");
	     for(i=0;i<(int)count;i++) {
                         printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
	     }
	     if( i2c_buf[0] == 0x0A && 
	         i2c_buf[1] == 0xCD &&
	         i2c_buf[2] == 0x00 )
	       {
		 i2c_buf[0]=0x02;
		 i2c_buf[1]=0x00;
		 if( write( i2c , i2c_buf , 2 ) == 2 )
		   printf(" i2c-0 write success\n");
		 else
		   printf(" i2c-0 write failed\n");
		 count = 2U;
		 for(i=0;i<(int)count;i++) {
		   printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
		 }
	       }
           }

           count = read( i2c , i2c_buf , 3 );
           if( count == 3 ) {
	     printf("4 i2c-0 read success\n");
	     for(i=0;i<(int)count;i++) {
                         printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
	     }
	     if( i2c_buf[0] == 0x0A && 
	         i2c_buf[1] == 0xCD &&
	         i2c_buf[2] == 0x00 )
	       {
		 i2c_buf[0]=0x00;
		 i2c_buf[1]=0x0E;
		 if( write( i2c , i2c_buf , 2 ) == 2 )
		   printf(" i2c-0 write success\n");
		 else
		   printf(" i2c-0 write failed\n");
		 count = 2U;
		 for(i=0;i<(int)count;i++) {
		   printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
		 }
	       }

		 
      }


    }

    //----------------------------------------------------

    for (;;) {
#if 0
        if (ioctl(afd, AUDIO_GET_STATS, &stats) == 0)
            fprintf(stderr,"%10d\n", stats.out_bytes);
#endif
        if (fill(buf, sz, cookie))
            break;
        if (write(afd, buf, sz) != sz)
            break;
    }

done:
    close(afd);

    //----------------------------------------------------
/*   if( i2c >= 0 ) {
     if( i2c_flag == 0 ) {
       count = read( i2c , i2c_buf , 3 );
       if( count == 3 ) printf(" i2c-0 read success\n");
       for(i=0;i<(int)count;i++) {
	 printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
       }
       i2c_buf[0]=0x00;
       i2c_buf[1]=0x0A;
       if( write( i2c , i2c_buf , 2 ) == 2 )
	 printf(" i2c-0 write success\n");
       else
	 printf(" i2c-0 write failed\n");
       count = 2U;
       for(i=0;i<(int)count;i++) {
	 printf("0x%02X\n",(unsigned int)i2c_buf[i]&0xff);
       }
     }*/
     close(i2c);
   }
    //----------------------------------------------------

    return 0;
}

/* http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t riff_fmt;
	uint32_t fmt_id;
	uint32_t fmt_sz;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
	uint16_t block_align;     /* num_channels * bps / 8 */
	uint16_t bits_per_sample;
	uint32_t data_id;
	uint32_t data_sz;
};


static char *next;
static unsigned avail;

int fill_buffer(void *buf, unsigned sz, void *cookie)
{
    if (sz > avail)
        return -1;
    memcpy(buf, next, sz);
    next += sz;
    avail -= sz;
    return 0;
}

void play_file(unsigned rate, unsigned channels,
               int fd, unsigned count)
{
    next = malloc(count);
    if (!next) {
        fprintf(stderr,"could not allocate %d bytes\n", count);
        return;
    }
    if (read(fd, next, count) != count) {
        fprintf(stderr,"could not read %d bytes\n", count);
        return;
    }
    avail = count;
    pcm_play(rate, channels, fill_buffer, 0);
}

int wav_play(const char *fn)
{
	struct wav_header hdr;
    unsigned rate, channels;
	int fd;
	fd = open(fn, O_RDONLY);
	if (fd < 0) {
        fprintf(stderr, "playwav: cannot open '%s'\n", fn);
		return -1;
	}
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "playwav: cannot read header\n");
		return -1;
	}
    fprintf(stderr,"playwav: %d ch, %d hz, %d bit, %s\n",
            hdr.num_channels, hdr.sample_rate, hdr.bits_per_sample,
            hdr.audio_format == FORMAT_PCM ? "PCM" : "unknown");
    
    if ((hdr.riff_id != ID_RIFF) ||
        (hdr.riff_fmt != ID_WAVE) ||
        (hdr.fmt_id != ID_FMT)) {
        fprintf(stderr, "playwav: '%s' is not a riff/wave file\n", fn);
        return -1;
    }
    if ((hdr.audio_format != FORMAT_PCM) ||
        (hdr.fmt_sz != 16)) {
        fprintf(stderr, "playwav: '%s' is not pcm format\n", fn);
        return -1;
    }
    if (hdr.bits_per_sample != 16) {
        fprintf(stderr, "playwav: '%s' is not 16bit per sample\n", fn);
        return -1;
    }

    play_file(hdr.sample_rate, hdr.num_channels,
              fd, hdr.data_sz);
    
    return 0;
}

int wav_rec(const char *fn, unsigned channels, unsigned rate)
{
    struct wav_header hdr;
    unsigned char buf[8192];
    struct msm_audio_config cfg;
    unsigned sz, n;
    int fd, afd;
    unsigned total = 0;
    unsigned char tmp;
    
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = channels;
    hdr.sample_rate = rate;
    hdr.byte_rate = hdr.sample_rate * hdr.num_channels * 2;
    hdr.block_align = hdr.num_channels * 2;
    hdr.bits_per_sample = 16;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;

    fd = open(fn, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("cannot open output file");
        return -1;
    }
    write(fd, &hdr, sizeof(hdr));

    afd = open("/dev/msm_pcm_in", O_RDWR);
    if (afd < 0) {
        perror("cannot open msm_pcm_in");
        close(fd);
        return -1;
    }

        /* config change should be a read-modify-write operation */
    if (ioctl(afd, AUDIO_GET_CONFIG, &cfg)) {
        perror("cannot read audio config");
        goto fail;
    }

    cfg.channel_count = hdr.num_channels;
    cfg.sample_rate = hdr.sample_rate;
    if (ioctl(afd, AUDIO_SET_CONFIG, &cfg)) {
        perror("cannot write audio config");
        goto fail;
    }

    if (ioctl(afd, AUDIO_GET_CONFIG, &cfg)) {
        perror("cannot read audio config");
        goto fail;
    }

    sz = cfg.buffer_size;
    fprintf(stderr,"buffer size %d x %d\n", sz, cfg.buffer_count);
    if (sz > sizeof(buf)) {
        fprintf(stderr,"buffer size %d too large\n", sz);
        goto fail;
    }

    if (ioctl(afd, AUDIO_START, 0)) {
        perror("cannot start audio");
        goto fail;
    }

    fcntl(0, F_SETFL, O_NONBLOCK);
    fprintf(stderr,"\n*** RECORDING * HIT ENTER TO STOP ***\n");

    for (;;) {
        while (read(0, &tmp, 1) == 1) {
            if ((tmp == 13) || (tmp == 10)) goto done;
        }
        if (read(afd, buf, sz) != sz) {
            perror("cannot read buffer");
            goto fail;
        }
        if (write(fd, buf, sz) != sz) {
            perror("cannot write buffer");
            goto fail;
        }
        total += sz;

    }
done:
    close(afd);

        /* update lengths in header */
    hdr.data_sz = total;
    hdr.riff_sz = total + 8 + 16 + 8;
    lseek(fd, 0, SEEK_SET);
    write(fd, &hdr, sizeof(hdr));
    close(fd);
    return 0;

fail:
    close(afd);
    close(fd);
    unlink(fn);
    return -1;
}

int mp3_play(const char *fn)
{
    char buf[64*1024];
    int r;
    int fd, afd;

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        perror("cannot open mp3 file");
        return -1;
    }

    afd = open("/dev/msm_mp3", O_RDWR);
    if (afd < 0) {
        close(fd);
        perror("cannot open mp3 output device");
        return -1;
    }

    fprintf(stderr,"MP3 PLAY\n");
    ioctl(afd, AUDIO_START, 0);

    for (;;) {
        r = read(fd, buf, 64*1024);
        if (r <= 0) break;
        r = write(afd, buf, r);
        if (r < 0) break;
    }

    close(fd);
    close(afd);
    return 0;
}
//-----------------------------------------------------------------------------------------------------
void ae2_ioctl_play(int i){
    int dCnt;
    struct cmd_dump cmd_dump;
    cmd_dump = dump[i];
    DS("dump[%d].cmd=%08x;//",i, cmd_dump.cmd );//, cmdToText[_IOC_NR(cmd)] );

    switch ( cmd_dump.cmd ) {
        case MA_IOCTL_WAIT://0
            ioctl(ae2_info.fd, MA_IOCTL_WAIT, cmd_dump.arg );
            DS("ioctl(ae2_info.fd, MA_IOCTL_WAIT, %u );\n",cmd_dump.arg);
            break;
            
        case MA_IOCTL_SLEEP://1
            ioctl(ae2_info.fd, MA_IOCTL_SLEEP, cmd_dump.arg );
            DS("ioctl(ae2_info.fd, MA_IOCTL_SLEEP, %u );\n",cmd_dump.arg);
          break;
          
        case MA_IOCTL_WRITE_REG_WAIT://2
            DS("ioctl(ae2_info.fd, MA_IOCTL_WRITE_REG_WAIT, dumpWrite[%u] );\n",cmd_dump.arg);
            unsigned int pDataNum;
            switch( dumpWrite[cmd_dump.arg].dSize) {
            case  sizeof( unsigned char ):
                if( ((unsigned char *)dumpWrite[cmd_dump.arg].pData-dumpWriteByte)==0 ) {
                    pDataNum=0;
                } else {
                    pDataNum=((unsigned int)((unsigned char *)dumpWrite[cmd_dump.arg].pData-dumpWriteByte))/dumpWrite[cmd_dump.arg].dSize;
                }
                break;
            case sizeof( unsigned short ):
                if( ((unsigned short *)dumpWrite[cmd_dump.arg].pData-dumpWriteWord)==0 ) {
                    pDataNum=0;
                } else {
                    pDataNum=((unsigned int)((unsigned short *)dumpWrite[cmd_dump.arg].pData-dumpWriteWord))/dumpWrite[cmd_dump.arg].dSize;
                }
                break;
            default:
                pDataNum=99999;
                break;
            }

            DS(  "dump[%d].arg=%d;\n"
                 "dumpWrite[%d].dAddress=%lu;\n"
                 "dumpWrite[%d].pData=&%s[%u];\n"
                 "dumpWrite[%d].dSize=%u;\n"
                 "dumpWrite[%d].dDataLen=%u;\n"
                 "dumpWrite[%d].dWait=%u;\n",
                 i,cmd_dump.arg,
                 cmd_dump.arg, dumpWrite[cmd_dump.arg].dAddress,        // I/F Address
                 cmd_dump.arg, 
                 (dumpWrite[cmd_dump.arg].dSize==sizeof( unsigned char ))?"dumpWriteByte":"dumpWriteWord",
                 pDataNum,                                           // Write Pointer 
                 cmd_dump.arg, dumpWrite[cmd_dump.arg].dSize,          // Write Size(data type size)
                 cmd_dump.arg, dumpWrite[cmd_dump.arg].dDataLen,       // Data Length
                 cmd_dump.arg, dumpWrite[cmd_dump.arg].dWait );        // Wait ns

            switch ( dumpWrite[cmd_dump.arg].dSize ) {
            case sizeof( unsigned char ):
                for(dCnt=0;dCnt< dumpWrite[cmd_dump.arg].dDataLen; ++dCnt) {
                    DS("dumpWrite[%d] dumpWriteByte[%d]=0x%02x;\n" ,cmd_dump.arg ,pDataNum+dCnt ,dumpWriteByte[pDataNum+dCnt] );
                }
                break;
            case sizeof( unsigned short ):
                for(dCnt=0; dCnt< dumpWrite[cmd_dump.arg].dDataLen; ++dCnt) {
                    DS("dumpWrite[%d] dumpWriteWord[%d]=0x%04x;\n" ,cmd_dump.arg ,pDataNum+dCnt ,dumpWriteWord[pDataNum+dCnt] );
                }
                break;
            default:
                break;
            }

            ioctl(ae2_info.fd, MA_IOCTL_WRITE_REG_WAIT, &dumpWrite[cmd_dump.arg] );

            break;
            
        case MA_IOCTL_READ_REG_WAIT://3
            ioctl(ae2_info.fd, MA_IOCTL_READ_REG_WAIT, &dumpRead[cmd_dump.arg] );
            DS("ioctl(ae2_info.fd, MA_IOCTL_READ_REG_WAIT, dumpRead[%u] );\n",cmd_dump.arg);
            break;
            
        case MA_IOCTL_DISABLE_IRQ://4
            ioctl(ae2_info.fd, MA_IOCTL_DISABLE_IRQ );
            DS("ioctl(ae2_info.fd, MA_IOCTL_DISABLE_IRQ );\n");
            break;
            
        case MA_IOCTL_ENABLE_IRQ://5
            ioctl(ae2_info.fd, MA_IOCTL_ENABLE_IRQ );
            DS("ioctl(ae2_info.fd, MA_IOCTL_ENABLE_IRQ );\n");
            break;
            
        case MA_IOCTL_RESET_IRQ_MASK_COUNT://6
            ioctl(ae2_info.fd, MA_IOCTL_RESET_IRQ_MASK_COUNT );
            DS("ioctl(ae2_info.fd, MA_IOCTL_RESET_IRQ_MASK_COUNT );\n");
            break;
            
        case MA_IOCTL_WAIT_IRQ://7
            ioctl(ae2_info.fd, MA_IOCTL_WAIT_IRQ, (int)cmd_dump.arg );
            DS("ioctl(ae2_info.fd, MA_IOCTL_WAIT_IRQ, %d );\n",(int)cmd_dump.arg);
            break;
            
        case MA_IOCTL_CANCEL_WAIT_IRQ://8
            ioctl(ae2_info.fd, MA_IOCTL_CANCEL_WAIT_IRQ );
            DS("ioctl(ae2_info.fd, MA_IOCTL_CANCEL_WAIT_IRQ );\n");
          break;
          
        case MA_IOCTL_SET_GPIO://9
            ioctl(ae2_info.fd, MA_IOCTL_SET_GPIO, cmd_dump.arg );
            DS("ioctl(ae2_info.fd, MA_IOCTL_SET_GPIO, %u );\n",cmd_dump.arg);
            break;
            
        default:
            DS(RED "!!!!!!!!!!! E R R O R !!!!!!!!!!!!!\n" DEFAULT "\n");
            break;
    }
}

//----------------------------------------------------------------------------------------------
void * ae2_ctrl_func( void * arg ) {
    (void)arg;
    pthread_detach( pthread_self() );
    
    usleep(200);
    
    int i;
    for(i=AE2_INIT_START_COUNT;i<=AE2_INIT_END1_COUNT;i++) {
    //for(i=AE2_INIT_START_COUNT; i<=50; i++) {
        ae2_ioctl_play( i );
    }

    // 初期化完了
    ae2_info.init_flag=1;
    D("complete: init_flag set \n");

    return 0;
}

//----------------------------------------------------------------------------------------------
void * ae2_ctrl_func_zero( void * arg ) {
    (void)arg;
    pthread_detach( pthread_self() );
    
    D("ioctl(ae2_info.fd, MA_IOCTL_WAIT_IRQ, &(ae2_info.dIrqCount) );\n");
    ioctl(ae2_info.fd, MA_IOCTL_WAIT_IRQ, &(ae2_info.dIrqCount) );
    D("complete Wait IRQ\n");
    
    return 0;
}

void mem_init(){
    dump = malloc( DEBUG_DUMP_END*sizeof(struct cmd_dump) );
    if( dump !=NULL )
        D("dump=malloc(%dKB) OK\n", DEBUG_DUMP_END*sizeof(struct cmd_dump)/1024);
    else
        D("dump=malloc() NG\n");

    dumpRead = malloc( DEBUG_DUMP_READ_MAX * sizeof(struct ma_IoCtlReadRegWait) );
    if( dumpRead !=NULL )
        D("dumpRead=malloc(%dKB) OK\n", DEBUG_DUMP_READ_MAX * sizeof(struct ma_IoCtlReadRegWait)/1024);
    else
        D("dumpRead=malloc() NG\n");

    dumpWrite = malloc(  DEBUG_DUMP_WRITE_MAX * sizeof(struct ma_IoCtlWriteRegWait) );
    if( dumpWrite !=NULL )
        D("dumpWrite=malloc(%dKB) OK\n", DEBUG_DUMP_WRITE_MAX * sizeof(struct ma_IoCtlWriteRegWait) /1024);
    else
        D("dumpWrite=malloc() NG\n");
    
    dumpWriteByte = malloc( DEBUG_DATA_DUMP_WRITE_MAX *sizeof(unsigned char) );
    if( dumpWriteByte  !=NULL )
        D("dumpWriteByte =malloc(%dKB) OK\n", DEBUG_DATA_DUMP_WRITE_MAX *sizeof(unsigned char) /1024);
    else
        D("dumpWriteByte =malloc() NG\n");
    
    dumpWriteWord = malloc( DEBUG_DATA_DUMP_WRITE_MAX *sizeof(unsigned short) );
    if( dumpWriteWord  !=NULL )
        D("dumpWriteWord =malloc(%dKB) OK\n", DEBUG_DATA_DUMP_WRITE_MAX *sizeof(unsigned short)/1024);
    else
        D("dumpWriteWord =malloc() NG\n");
    
    dumpReadByte = malloc( DEBUG_DATA_DUMP_READ_MAX *sizeof(unsigned char) );
    if( dumpReadByte  !=NULL )
        D("dumpReadByte =malloc(%dKB) OK\n",  DEBUG_DATA_DUMP_READ_MAX *sizeof(unsigned char)/1024);
    else
        D("dumpReadByte =malloc() NG\n");
    
    dumpReadWord = malloc( DEBUG_DATA_DUMP_READ_MAX *sizeof(unsigned short) );
    if( dumpReadWord  !=NULL )
        D("dumpReadWord =malloc(%dKB) OK\n",  DEBUG_DATA_DUMP_READ_MAX *sizeof(unsigned short)/1024);
    else
        D("dumpReadWord =vmalloc() NG\n");
    
    ReadWriteCount.allCount=0;
    ReadWriteCount.waitCount=0;
    ReadWriteCount.sleepCount=0;
    ReadWriteCount.writeCount=0;
    ReadWriteCount.writeByteCount=0;
    ReadWriteCount.writeWordCount=0;
    ReadWriteCount.readCount=0;
    ReadWriteCount.readByteCount=0;
    ReadWriteCount.readWordCount=0;
    ReadWriteCount.disableCount=0;
    ReadWriteCount.enableCount=0;
    ReadWriteCount.resetCount=0;
    ReadWriteCount.waitIRQCount=0;
    ReadWriteCount.cancelCount=0;
    ReadWriteCount.setCount=0;
    
}

void mem_free() {
    if( dump !=NULL ){
        free(dump);
        D("free(dump)\n");
    }
    else
        D("free(dump) NG\n");
    
    if( dumpRead !=NULL ){
        free(dumpRead);
        D("free(dumpRead)\n");
    }
    else
        D("free(dumpRead) NG\n");

    if( dumpWrite !=NULL ) {
        free(dumpWrite);
        D("free(dumpWrite)\n");
    }
    else
        D("free(dumpWrite) NG\n");
    
    if( dumpWriteByte != NULL ) {
        free(dumpWriteByte);
        D("free(dumpWriteByte)\n");
    }
    else
        D("free(dumpWriteByte) NG\n");    

    if( dumpWriteWord != NULL ) {
        free(dumpWriteWord);
        D("free(dumpWriteWord)\n");
    }
    else
        D("free(dumpWriteWord) NG\n");

    if( dumpReadByte != NULL ) {
        free(dumpReadByte);
        D("free(dumpReadByte)\n");
    }
    else
        D("free(dumpReadByte) NG\n");

    if( dumpReadWord != NULL ) {
        free(dumpReadWord);
        D("free(dumpReadWord)\n");
    }
    else
        D("free(dumpReadWord) NG\n");
}

int main(int argc, char **argv)
{
    const char *fn = 0;
    int play = 1;
    unsigned channels = 1;
    unsigned rate = 44100;
    
    // メモリ初期化
    mem_init();
    // データ代入
    mem_data1();
    mem_data2();
    mem_data3();
    mem_data4();
    mem_data5();
    mem_data6();
    mem_data7();
    mem_data8();

    argc--;
    argv++;
    while (argc > 0) {
        if (!strcmp(argv[0],"-rec")) {
            play = 0;
        } else if (!strcmp(argv[0],"-play")) {
            play = 1;
        } else if (!strcmp(argv[0],"-stereo")) {
            channels = 2;
        } else if (!strcmp(argv[0],"-mono")) {
            channels = 1;
        } else if (!strcmp(argv[0],"-rate")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr,"playwav: -rate requires a parameter\n");
                return -1;
            }
            rate = atoi(argv[0]);
        } else {
            fn = argv[0];
        }
        argc--;
        argv++;
    }
    
    if (fn == 0) {
        fn = play ? "/data/out.wav" : "/data/rec.wav";
    }
    
    if (play) {
        // msm_audio_dev_ctrl 初期化
        int fd_madc1,fd_madc2;
        fd_madc1 = open("/dev/msm_audio_dev_ctrl", O_RDWR);
        unsigned int vol=100;
        if( fd_madc1 != 0 ){
            ioctl( fd_madc1 , AUDIO_SET_VOLUME , &vol );
            close( fd_madc1 );
        }

        fd_madc1 = open("/dev/msm_audio_dev_ctrl", O_RDWR);
        unsigned int dev=0x30;//I2S_RX_SPKR
        if( fd_madc1 != 0 ){
            ioctl( fd_madc1 , AUDIO_SWITCH_DEVICE , &dev );

            fd_madc2 = open("/dev/msm_audio_dev_ctrl", O_RDWR);
            if( fd_madc2 != 0 ) {
                ioctl( fd_madc2 , AUDIO_SET_VOLUME , &vol );
                close( fd_madc2 );
            }
            dev=0x06;//AUDIO_SWITCH_DEVICE
            ioctl( fd_madc1 , AUDIO_SWITCH_DEVICE , &dev );
            struct msm_mute_info mute;
            mute.mute=SND_MUTE_UNMUTED; // #define SND_MUTE_UNMUTED 0
            mute.path=CAD_TX_DEVICE; // #define CAD_TX_DEVICE  0x01
            ioctl( fd_madc1 , AUDIO_SET_MUTE , &mute );

            close( fd_madc1 );
        }

        // ae2 ドライバー初期化開始
        ae2_info.init_flag=0;
        ae2_info.fd = open("/dev/ae2", O_RDWR);
        //pthread_t pt_adc,pt_adc1;
        pthread_t pt_zero,pt;
        // スレッド生成
        D("ae2 init start\n");
        pthread_create( &pt_zero, NULL, &ae2_ctrl_func_zero, 0 );
        // 初期化スレッド生成
        pthread_create( &pt, NULL, &ae2_ctrl_func, 0 );
        // 初期化完了待ち
        usleep(5000000);
        while( ae2_info.init_flag==0 ){
            usleep(1000000);
            D("init wait...\n");
        }
        D("complete ae2 init\n");
        
        D("dIrqCount=%d\n",ae2_info.dIrqCount );

        goto end;        
        {
            int fd;
            fd = open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER", O_WRONLY|O_LARGEFILE);
            write(fd,"1",1);
            close(fd);
            fd = open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER_LEV", O_WRONLY|O_LARGEFILE);
            write(fd,"1",1);
            close(fd);
            fd=open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER_LEV", O_WRONLY|O_LARGEFILE);
            write(fd,"2",1);
            close(fd);
        }
        
        const char *dot = strrchr(fn, '.');
        if (dot && !strcmp(dot,".mp3")) {
            return mp3_play(fn);
        } else {
            return wav_play(fn);
        }
        
        {
            int fd;
            fd = open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER_LEV", O_WRONLY|O_LARGEFILE);
            write(fd,"1",1);
            close(fd);
            
            fd=open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER_LEV", O_WRONLY|O_LARGEFILE);
            write(fd,"0",1);
            close(fd);
            
            fd = open("/sys/kernel/shterm/info/SHTERM_INFO_SPEAKER", O_WRONLY|O_LARGEFILE);
            write(fd,"0",1);
            close(fd);
        }
        
    } else {
        return wav_rec(fn, channels, rate);
    }
    

 end:
    // メモリ開放
    mem_free();
    // /dev/ae2 close
    if(ae2_info.fd>=0)
        close(ae2_info.fd);

    return 0;
}


