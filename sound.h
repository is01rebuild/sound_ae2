struct ma_IoCtlWriteRegWait
{
	unsigned long	dAddress;		/* I/F Address */
	const void	*pData;		/* Write Pointer */
	unsigned int	dSize;			/* Write Size(data type size) */
	unsigned int	dDataLen;		/* Data Length */
	unsigned int	dWait;			/* Wait ns */
};

struct ma_IoCtlReadRegWait
{
	unsigned long	dAddress;		/* I/F Address */
	void		*pData;		/* Read Data Store Pointer*/
	unsigned int	dSize;			/* Read Size(data type size) */
	unsigned int	dDataLen;		/* Data Length */
	unsigned int	dWait;			/* Wait ns */
};

struct ioctl_count {
    unsigned int allCount;

    unsigned int waitCount;//pos uint
    unsigned int sleepCount;//pos uint
    unsigned int writeCount,writeByteCount,writeWordCount;//pos struct ma_IoCtlWriteRegWait
    unsigned int readCount,readByteCount,readWordCount;//pos struct ma_IoCtlReadRegWait
    unsigned int disableCount;//-
    unsigned int enableCount;//-
    unsigned int resetCount;//-
    unsigned int waitIRQCount;//pos int
    unsigned int cancelCount;//-
    unsigned int setCount;//pos unsigned int

};

struct cmd_dump {
    unsigned int cmd;
    unsigned int arg;
};

// 関数プロトタイプ宣言
void mem_data1();
void mem_data2();
void mem_data3();
void mem_data4();
