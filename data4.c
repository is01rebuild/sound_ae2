#include "sound.h"

extern struct ioctl_count ReadWriteCount;

extern struct cmd_dump *dump;
extern struct ma_IoCtlReadRegWait *dumpRead;
extern struct ma_IoCtlWriteRegWait *dumpWrite;
extern unsigned char  *dumpWriteByte;
extern unsigned short *dumpWriteWord;
extern unsigned char  *dumpReadByte;
extern unsigned short *dumpReadWord;

void mem_data4() {
    // 本ファイル内のデータは playwav.c にてまだ利用していません。

    //dump[4561].cmd=

    // サービスを start させた後、
    // kill -9  `ps | grep mediaserver | awk '{print $2}'`
    // を行ってふえた部分を追加してください。
    // 

    //dumpWriteByte[2438]=0x01;

    //dump[5725].cmd=

    // サービスを stop させ
    // kill -9  `ps | grep mediayamaha | awk '{print $2}'`
    // を行った結果を追加してください


    //dump[5729].cmd=

    // 下記は手元で実行した結果です。
    // cat /dev/ae2debug の最後の部分をそのまま張り付けてください。
    /*
    ReadWriteCount.allCount=5730;
    ReadWriteCount.waitCount=77;
    ReadWriteCount.sleepCount=202;
    ReadWriteCount.writeCount=3157;
    ReadWriteCount.writeByteCount=2439;
    ReadWriteCount.writeWordCount=20476;
    ReadWriteCount.readCount=893;
    ReadWriteCount.readByteCount=893;
    ReadWriteCount.readWordCount=0;
    ReadWriteCount.disableCount=699;
    ReadWriteCount.enableCount=699;
    ReadWriteCount.resetCount=1;
    ReadWriteCount.waitIRQCount=1;
    ReadWriteCount.cancelCount=0;
    ReadWriteCount.setCount=1;
    */
}
