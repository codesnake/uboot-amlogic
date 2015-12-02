#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>

#ifndef FIRMWARE_IN_ONE_FILE
#define STATIC_PREFIX
#else
#define STATIC_PREFIX static inline
#endif

#ifndef memcpy
#define memcpy ipl_memcpy:
#endif

#ifdef CONFIG_SPI_NOR_SECURE_STORAGE
#define SPI_SECURESTORAGE_MAGIC		"spi_sskey"
#define SPI_SECURESTORAGE_AREA_SIZE	(256*1024)
#define SPI_SECURESTORAGE_AREA_COUNT 2
#define SPI_SECURESTORAGE_OFFSET	(0x100000+64*1024)

#define SPI_SECURESTORAGE_MAGIC_SIZE	16
#define SPI_SECURESTORAGE_AREA_HEAD_SIZE	(SPI_SECURESTORAGE_MAGIC_SIZE+4*6)
#define SPI_SECURESTORAGE_AREA_VALID_SIZE	(SPI_SECURESTORAGE_AREA_SIZE-SPI_SECURESTORAGE_AREA_HEAD_SIZE)

#define SECUREOS_KEY_DEFAULT_ADDR	(SECUREOS_KEY_BASE_ADDR+0xe0000)
#define SECUREOS_KEY_DEFAULT_SIZE	(128*1024)

#define SECURE_STORAGE_MEM_ADDR		SECUREOS_KEY_DEFAULT_ADDR //0x86000000
#define SECURE_STORAGE_MEM_SIZE		SECUREOS_KEY_DEFAULT_SIZE
struct spi_securestorage_head_t{
	unsigned char magic[SPI_SECURESTORAGE_MAGIC_SIZE];
	unsigned int magic_checksum;
	unsigned int version;
	unsigned int tag;
	unsigned int checksum; //data checksum
	unsigned int timestamp;
	unsigned int reserve;
};
STATIC_PREFIX unsigned int emmckey_calculate_checksum(unsigned char *buf,unsigned int lenth)
{
	unsigned int checksum = 0;
	unsigned int cnt;
#if 0
	for(cnt=0;cnt<lenth;cnt++){
		checksum += buf[cnt];
	}
#else
	cnt = lenth-1;
	do{
		checksum += buf[cnt];
	}while(cnt-- != 0);
#endif
	return checksum;
}
STATIC_PREFIX int spi_secure_storage_get(int nor_addr,int mem_addr,int size)
{
	int i;
	unsigned valid_addr=0;
	struct spi_securestorage_head_t part1_head,part2_head;
	char part1_flag,part2_flag;
	unsigned char magic[16];
	unsigned char *pm = SPI_SECURESTORAGE_MAGIC;
	mem_addr = SECURE_STORAGE_MEM_ADDR;
	size = SECURE_STORAGE_MEM_SIZE;
#if 0
	for(i=0;i<9;i++){
		magic[i] = pm[i];
	}
#else
	i = 8;
	do{
		magic[i] = pm[i];
	}while(i--!=0);
#endif
	memcpy(&part1_head,(unsigned *)(nor_addr+SPI_SECURESTORAGE_OFFSET),sizeof(struct spi_securestorage_head_t));
	memcpy(&part2_head,(unsigned *)(nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_SIZE),sizeof(struct spi_securestorage_head_t));
	part1_flag = 0;
	part2_flag = 0;
	for(i=0;i<9;i++){
		if(part1_head.magic[i] != magic[i]){
			part1_flag = 1;
			//serial_puts("\nspi secure storage read part1 1 error\n");
		}
		if(part2_head.magic[i] != magic[i]){
			part2_flag = 1;
			//serial_puts("\nspi secure storage read part2 1 error\n");
		}
	}

	if((part1_head.magic_checksum != emmckey_calculate_checksum(&part1_head.magic[0],SPI_SECURESTORAGE_MAGIC_SIZE))){
		part1_flag = 2;
		//serial_puts("\nspi secure storage read part1 2 error\n");
	}
	if((part2_head.magic_checksum != emmckey_calculate_checksum(&part2_head.magic[0],SPI_SECURESTORAGE_MAGIC_SIZE))){
		part2_flag = 2;
		//serial_puts("\nspi secure storage read part2 2 error\n");
	}
	if(!part1_flag){
		if(part1_head.checksum != emmckey_calculate_checksum((unsigned char*)(nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_HEAD_SIZE),SPI_SECURESTORAGE_AREA_VALID_SIZE)){
			part1_flag = 3;
			//serial_puts("\nspi secure storage read part1 3 error\n");
		}
	}
	if(!part2_flag){
		if(part2_head.checksum != emmckey_calculate_checksum((unsigned char*)(nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_SIZE+SPI_SECURESTORAGE_AREA_HEAD_SIZE),SPI_SECURESTORAGE_AREA_VALID_SIZE)){
			part2_flag = 3;
			//serial_puts("\nspi secure storage read part2 3 error\n");
		}
	}
	if((!part1_flag) &&(!part2_flag)){
		if(part1_head.timestamp > part2_head.timestamp){
			valid_addr = nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_HEAD_SIZE;
		}
		else{
			valid_addr = nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_SIZE+SPI_SECURESTORAGE_AREA_HEAD_SIZE;
		}
	}
	if(valid_addr == 0){
		if(!part1_flag){
			valid_addr = nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_HEAD_SIZE;
		}
		if(!part2_flag){
			valid_addr = nor_addr+SPI_SECURESTORAGE_OFFSET+SPI_SECURESTORAGE_AREA_SIZE+SPI_SECURESTORAGE_AREA_HEAD_SIZE;
		}
	}
	if(valid_addr){
		memcpy((unsigned *)mem_addr,(unsigned *)valid_addr,size);
		//serial_puts("\nspi secure storage read ok\n");
		serial_puts("spi sskey R ok\n");
		return 0;
	}
	//serial_puts("\nspi secure storage read error\n");
	serial_puts("spi sskey R err\n");
	return -1;
}
#endif



