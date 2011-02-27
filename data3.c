#include "sound.h"

extern struct ioctl_count ReadWriteCount;

extern struct cmd_dump *dump;
extern struct ma_IoCtlReadRegWait *dumpRead;
extern struct ma_IoCtlWriteRegWait *dumpWrite;
extern unsigned char  *dumpWriteByte;
extern unsigned short *dumpWriteWord;
extern unsigned char  *dumpReadByte;
extern unsigned short *dumpReadWord;


void mem_data3() {
    //dump[2750].cmd=

    // /cat/dev/ae2debug の結果の約1/3程度の3つ目を張り付けてください
    // 単なる代入ですので重複しても構いませんが、データ抜けが無いよう注意してください。
    // 一応、ここまでで、サービスを start させただけの dump 結果を張り付けました。

    //dump[4560].cmd=

    /*
    ReadWriteCount.allCount=4561;
    ReadWriteCount.waitCount=43;
    ReadWriteCount.sleepCount=174;
    ReadWriteCount.writeCount=2373;
    ReadWriteCount.writeByteCount=1655;
    ReadWriteCount.writeWordCount=20476;
    ReadWriteCount.readCount=678;
    ReadWriteCount.readByteCount=678;
    ReadWriteCount.readWordCount=0;
    ReadWriteCount.disableCount=645;
    ReadWriteCount.enableCount=645;
    ReadWriteCount.resetCount=1;
    ReadWriteCount.waitIRQCount=1;
    ReadWriteCount.cancelCount=0;
    ReadWriteCount.setCount=1;
    */
}
