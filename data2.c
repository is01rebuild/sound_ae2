#include "sound.h"

extern struct ioctl_count ReadWriteCount;

extern struct cmd_dump *dump;
extern struct ma_IoCtlReadRegWait *dumpRead;
extern struct ma_IoCtlWriteRegWait *dumpWrite;
extern unsigned char  *dumpWriteByte;
extern unsigned short *dumpWriteWord;
extern unsigned char  *dumpReadByte;
extern unsigned short *dumpReadWord;


void mem_data2() {
    //dump[617].cmd=

    // /cat/dev/ae2debug の結果の約1/3程度の2つ目を張り付けてください
    // 単なる代入ですので重複しても構いませんが、データ抜けが無いよう注意してください。

    //dumpWriteWord[20263]=
}
