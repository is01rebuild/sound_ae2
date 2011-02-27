#include "sound.h"

extern struct ioctl_count ReadWriteCount;

extern struct cmd_dump *dump;
extern struct ma_IoCtlReadRegWait *dumpRead;
extern struct ma_IoCtlWriteRegWait *dumpWrite;
extern unsigned char  *dumpWriteByte;
extern unsigned short *dumpWriteWord;
extern unsigned char  *dumpReadByte;
extern unsigned short *dumpReadWord;

void mem_data1() {
    //dump[0].cmd=
    //

    // /cat/dev/ae2debug の結果の約1/3程度を張り付けてください
  
    //dumpWriteWord[12260]=

}
