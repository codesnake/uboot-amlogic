

/*
 * E-FUSE char device driver.
 *
 * Author: Bo Yang <bo.yang@amlogic.com>
 *
 * Copyright (c) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the smems of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 */

#include <linux/types.h>
#include "aml_keys.h"
#include <amlogic/securitykey.h>
#define KEYS_MODULE_NAME    "aml_keys"
#define KEYS_DRIVER_NAME	"aml_keys"
#define KEYS_DEVICE_NAME    "aml_keys"
#define KEYS_CLASS_NAME     "aml_keys"

#define EFUSE_READ_ONLY
//static unsigned long efuse_status;
#define EFUSE_IS_OPEN           (0x01)

//#define TEST_NAND_KEY_WR

//#define KEY_NODE_CREATE

/*
typedef struct efuse_dev_s
{
    struct cdev cdev;
    unsigned int flags;
} efuse_dev_t;
*/
static uint8_t keys_version=0;
static uint8_t version_dirty = 0;

static aml_keys_schematic_t * key_schematic[256]={NULL};
static aml_key_t * curkey=NULL;

typedef int (*aes_algorithm_t)(void *dst,size_t * dst_len,const void *src,size_t src_len);
static aes_algorithm_t aes_algorithm_encrypt=NULL;
static aes_algorithm_t aes_algorithm_decrypt=NULL;

int32_t aml_keys_register(int32_t version, aml_keys_schematic_t * schematic)
{
    if (version < 1 || version > 255)
        return -1;
    if (key_schematic[version] != NULL)
        return -1;
    key_schematic[version] = schematic;
    return 0;
}
#define PROVIDERS_NUM	5
static aml_keybox_provider_t * providers[5]={NULL};

static void trigger_key_init(void);

int32_t aml_keybox_provider_register(aml_keybox_provider_t * provider)
{
    int i;
	for (i = 0; i < ARRAY_SIZE(providers); i++)
	{
		if (providers[i])
			continue;
		providers[i] = provider;
		printk("i=%d,register --- %s\n", i,provider->name);
		//trigger_key_init();
		break;
	}
	if(i<ARRAY_SIZE(providers)){
		return 0;
	}
	else{
		return -1;
	}
}
aml_keybox_provider_t * aml_keybox_provider_get(char * name)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(providers); i++)
    {
        if (providers[i] == NULL)
            continue;
//        printk("name=%s %s\n", providers[i]->name, name);
        if (strcmp(name, providers[i]->name))
            continue;
        return providers[i];
    }
    return NULL;

}

#ifdef CRYPTO_DEPEND_ON_KERENL
/**
 * Crypto API
 */
static struct crypto_blkcipher *aml_keybox_crypto_alloc_cipher(void)
{
    return crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
}

static int aml_keybox_aes_encrypt(const void *key, int key_len,
                                  const u8 *aes_iv, void *dst, size_t *dst_len,
                                  const void *src, size_t src_len)
{
    struct scatterlist sg_in[2], sg_out[1];
    struct crypto_blkcipher *tfm = aml_keybox_crypto_alloc_cipher();
    struct blkcipher_desc desc =
        { .tfm = tfm, .flags = 0 };
    int ret;
    void *iv;
    int ivsize;
    size_t zero_padding = (0x10 - (src_len & 0x0f));
    char pad[16];

    if (IS_ERR(tfm))
        return PTR_ERR(tfm);

    memset(pad, zero_padding, zero_padding);

    *dst_len = src_len + zero_padding;

    crypto_blkcipher_setkey((void *) tfm, key, key_len);
    sg_init_table(sg_in, 2);
    sg_set_buf(&sg_in[0], src, src_len);
    sg_set_buf(&sg_in[1], pad, zero_padding);
    sg_init_table(sg_out, 1);
    sg_set_buf(sg_out, dst, *dst_len);
    iv = crypto_blkcipher_crt(tfm)->iv;
    ivsize = crypto_blkcipher_ivsize(tfm);

    memcpy(iv, aes_iv, ivsize);
    /*
     print_hex_dump(KERN_ERR, "enc key: ", DUMP_PREFIX_NONE, 16, 1,
     key, key_len, 1);
     print_hex_dump(KERN_ERR, "enc src: ", DUMP_PREFIX_NONE, 16, 1,
     src, src_len, 1);
     print_hex_dump(KERN_ERR, "enc pad: ", DUMP_PREFIX_NONE, 16, 1,
     pad, zero_padding, 1);
     */
    ret = crypto_blkcipher_encrypt(&desc, sg_out, sg_in,
                                   src_len + zero_padding);
    crypto_free_blkcipher(tfm);
    if (ret < 0)
        pr_err("ceph_aes_crypt failed %d\n", ret);
    /*
     print_hex_dump(KERN_ERR, "enc out: ", DUMP_PREFIX_NONE, 16, 1,
     dst, *dst_len, 1);
     */
    return 0;
}
static int amlogic_keybox_aes_decrypt(const void *key, int key_len,
                                      const u8 *aes_iv, void *dst,
                                      size_t *dst_len, const void *src,
                                      size_t src_len)
{
    struct scatterlist sg_in[1], sg_out[2];
    struct crypto_blkcipher *tfm = aml_keybox_crypto_alloc_cipher();
    struct blkcipher_desc desc =
        { .tfm = tfm };
    char pad[16];
    void *iv;
    int ivsize;
    int ret;
    int last_byte;

    if (IS_ERR(tfm))
        return PTR_ERR(tfm);

    crypto_blkcipher_setkey((void *) tfm, key, key_len);
    sg_init_table(sg_in, 1);
    sg_init_table(sg_out, 2);
    sg_set_buf(sg_in, src, src_len);
    sg_set_buf(&sg_out[0], dst, *dst_len);
    sg_set_buf(&sg_out[1], pad, sizeof(pad));

    iv = crypto_blkcipher_crt(tfm)->iv;
    ivsize = crypto_blkcipher_ivsize(tfm);

    memcpy(iv, aes_iv, ivsize);

    /*
     print_hex_dump(KERN_ERR, "dec key: ", DUMP_PREFIX_NONE, 16, 1,
     key, key_len, 1);
     print_hex_dump(KERN_ERR, "dec  in: ", DUMP_PREFIX_NONE, 16, 1,
     src, src_len, 1);
     */

    ret = crypto_blkcipher_decrypt(&desc, sg_out, sg_in, src_len);
    crypto_free_blkcipher(tfm);
    if (ret < 0)
    {
        pr_err("ceph_aes_decrypt failed %d\n", ret);
        return ret;
    }

    if (src_len <= *dst_len)
        last_byte = ((char *) dst)[src_len - 1];
    else
        last_byte = pad[src_len - *dst_len - 1];
    if (last_byte <= 16 && src_len >= last_byte)
    {
        *dst_len = src_len - last_byte;
    } else
    {
        pr_err("ceph_aes_decrypt got bad padding %d on src len %d\n",
               last_byte, (int)src_len);
        return -EPERM; /* bad padding */
    }
    /*
     print_hex_dump(KERN_ERR, "dec out: ", DUMP_PREFIX_NONE, 16, 1,
     dst, *dst_len, 1);
     */
    return 0;
}
static void test_aes(void)
{
    static char *key = "1234567890abcdef1234567890abcdef";
    static char dec[600];
    static char enc[600];
    static char orig[300];
    int i;
    size_t dst_len = 256;
    size_t tlen;
    for (i = 0; i < sizeof(orig); i++)
        orig[i] = i;
    printk("=================================\n");
    aml_keybox_aes_encrypt(key, sizeof(key), "1234567890abcdef1234567890abcdef",
                           enc, &dst_len, orig, sizeof(orig));
    printk("%d\n", dst_len);
    tlen = dst_len;
    dst_len = sizeof(orig);
    amlogic_keybox_aes_decrypt(key, sizeof(key),
                               "1234567890abcdef1234567890abcdef", dec,
                               &dst_len, enc, tlen);
    printk("%d\n", dst_len);
    for (i = 0; i < sizeof(orig); i++)
        if (orig[i] != dec[i])
            printk("%d %d\n", orig[i], dec[i]);
    printk("=================================\n");
}
#endif //CRYPTO_DEPEND_ON_KERENL

#ifdef TEST_NAND_KEY_WR
#define PRINT_HASH(hash) {printk("%s:%d ",__func__,__LINE__);int __i;for(__i=0;__i<32;__i++)printk("%02x,",hash[__i]);printk("\n");}
#else
#define PRINT_HASH(hash)
#endif

#ifdef CRYPTO_DEPEND_ON_KERENL
static int aml_key_hash(unsigned char *hash, const char *data, unsigned int len)
{
    struct scatterlist sg;

    struct crypto_hash *tfm;
    struct hash_desc desc;

    tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
    PRINT_HASH(hash);
    if (IS_ERR(tfm))
        return -EINVAL;

    PRINT_HASH(hash);
    /* ... set up the scatterlists ... */
    sg_init_one(&sg, (u8 *) data, len);
    desc.tfm = tfm;
    desc.flags = 0;

    if (crypto_hash_digest(&desc, &sg,len, hash))
        return -EINVAL;
    PRINT_HASH(hash);
    crypto_free_hash(tfm);

    return 0;
}
#else
extern int hash_sha256(unsigned char *buf,int len,unsigned char *hash);
static int aml_key_hash(unsigned char *hash, const char *data, unsigned int len)
{
	int ret;
	ret = hash_sha256((unsigned char *)data,(int)len,hash);
	return ret;
}
static uint16_t aml_key_checksum(char *data,int lenth)
{
	uint16_t checksum;
	uint8_t *pdata;
	int i;
	checksum = 0;
	pdata = (uint8_t*)data;
	for(i=0;i<lenth;i++){
		checksum += pdata[i];
	}
	return checksum;
}	
#endif

#ifdef CRYPTO_DEPEND_ON_KERENL
static struct
{
    u8 *iv;
    u8 * key;
    int key_len;

} aml_root_key =
    { .iv = "amlogic_hello_iput_vector", .key =
              "1234567890abcdef1234567890abcdef",
      .key_len = 32 };

static int aml_key_encrypt(void *dst, uint16_t * dst_len, const void *src,
                           uint16_t src_len)
{
    int ret;
    size_t dstlen = (size_t) (*dst_len);
    size_t srclen = (size_t) (src_len);

    ret = aml_keybox_aes_encrypt(aml_root_key.key, aml_root_key.key_len,
                                 aml_root_key.iv, dst, &dstlen, src, srclen);
    if (ret)
    {
        pr_err("encrypt error ");
        return ret;
    }
    *dst_len = (uint16_t) dstlen;

    return ret;
}
static int aml_key_decrypt(void *dst, size_t *dst_len, const void *src,
                           size_t src_len)
{
    int ret;
    size_t dstlen = (size_t) (*dst_len);
    size_t srclen = (size_t) (src_len);

    ret = amlogic_keybox_aes_decrypt(aml_root_key.key, aml_root_key.key_len,
                                     aml_root_key.iv, dst, &dstlen, src,
                                     srclen);
    if (ret)
    {
        pr_err("encrypt error ");
        return ret;
    }
    *dst_len = (uint16_t) dstlen;
    return ret;
}
#else
extern int aml_algorithm_aes_enc_dec(int encFlag,unsigned char *out,int *outlen,unsigned char *in,int inlen);

static int aml_key_encrypt(void *dst, size_t * dst_len, const void *src,
                           size_t src_len)
{
	int ret=0;
	int keydatalen,buf_len;
	unsigned char *data;
	unsigned char bEncryptFlag = 1;
	size_t dstlen;
	keydatalen = src_len + 4;
	buf_len = ((keydatalen+15)>>4)<<4;
	dstlen = buf_len;
	
	data = kzalloc(buf_len, GFP_KERNEL);
	if(data == NULL){
		printk("malloc mem fail,%s:%d\n",__func__,__LINE__);
		return -ENOMEM;
	}
	memset(data,0,buf_len);
	memcpy(&data[0],&src_len,4);
	memcpy(&data[4],src,src_len);
	//the aml aes is used from 2013.12.19
	ret = aml_algorithm_aes_enc_dec(bEncryptFlag,(unsigned char *)dst,(int*)&dstlen,data,buf_len);
	*dst_len = dstlen;
	kfree(data);
	return ret;
}
static int aml_key_decrypt(void *dst, size_t *dst_len, const void *src,
                           size_t src_len)
{
	int ret=0;
	size_t srclen = src_len;
	size_t dstlen;
	size_t keydatalen;
	unsigned char *data;
	unsigned char bEncryptFlag = 0;
	srclen = ((src_len+15)>>4)<<4;
	if(src_len != ((src_len+15)>>4)<<4)
	{
		printf("data len is not 16 byte aligned error!\n");
		return -ENOMEM; 
	}
	data = kzalloc(srclen, GFP_KERNEL);
	if(data == NULL){
		printk("malloc mem fail,%s:%d\n",__func__,__LINE__);
		return -ENOMEM;
	}
	dstlen=srclen;
	memset(data,0,srclen);
	//the aml aes is used from 2013.12.19
	ret = aml_algorithm_aes_enc_dec(bEncryptFlag,data,(int*)&dstlen,(unsigned char *)src,srclen);
	memcpy(&keydatalen,data,4);
	memcpy(dst,&data[4],keydatalen);
	*dst_len = keydatalen;
	
	kfree(data);
	return ret;
}

extern int aml_aes_encrypt(unsigned char *output,unsigned char *input,int size);
extern int aml_aes_decrypt(unsigned char *output,unsigned char *input,int size);
static int aml_keysafety_encrypt(void *dst, size_t * dst_len, const void *src,
                           size_t src_len)
{
	int ret=0;
	size_t srclen = src_len;
	//size_t dstlen;
	size_t keydatalen;
	unsigned char *data;

	keydatalen = src_len+4;
	srclen = ((keydatalen+15)>>4)<<4;

	data = kzalloc(srclen, GFP_KERNEL);
	if(data == NULL){
		printk("malloc mem fail,%s:%d\n",__func__,__LINE__);
		return -ENOMEM;
	}
	memset(data,0,srclen);
	memcpy(&data[0],&src_len,4);
	memcpy(&data[4],src,src_len);
	ret = aml_aes_encrypt((unsigned char *)dst,data,srclen);
	*dst_len = srclen;
	kfree(data);
	return ret;
}
static int aml_keysafety_decrypt(void *dst, size_t *dst_len, const void *src,
                           size_t src_len)
{
	int ret=0;
	size_t srclen = src_len;
	size_t keydatalen;
	unsigned char *data;

	srclen = ((src_len+15)>>4)<<4;
	if(src_len != ((src_len+15)>>4)<<4)
	{
		printk("data len is not 16 byte aligned error!\n");
		return -ENOMEM; 
	}
	data = kzalloc(srclen, GFP_KERNEL);
	if(data == NULL){
		printk("malloc mem fail,%s:%d\n",__func__,__LINE__);
		return -ENOMEM;
	}
	memset(data,0,srclen);
	ret = aml_aes_decrypt(data,(unsigned char*)src,srclen);
	memcpy(&keydatalen,data,4);
	if(keydatalen <= srclen){
		// this decrypt is ok
		memcpy(dst,&data[4],keydatalen);
		*dst_len = keydatalen;
	}
	else{
		// this decrypt is err
		memcpy(dst,&data[4],srclen);
		*dst_len = srclen;
	}
	kfree(data);
	return ret;
}

int register_aes_algorithm(int storage_version)
{
	int ret=-1;
	if(storage_version == 1){
//		printk("%s:%d,old way\n",__func__,__LINE__);
		aes_algorithm_encrypt = aml_key_encrypt;
		aes_algorithm_decrypt = aml_key_decrypt;
		ret = 0;
	}
	else if(storage_version == 2){
//		printk("%s:%d,new way\n",__func__,__LINE__);
		aes_algorithm_encrypt = aml_keysafety_encrypt;
		aes_algorithm_decrypt = aml_keysafety_decrypt;
		ret = 0;
	}
	return ret;
}
#endif
/**
 ssize_t (*show)(struct device *dev, struct device_attribute *attr,
 char *buf);
 ssize_t (*store)(struct device *dev, struct device_attribute *attr,
 const char *buf, size_t count);
 };
 */
/**
 * @todo key process , identy the key version
 */
static int debug_mode = 1;//1:debug,0:normal
static int postpone_write = 0; //1:postpone, 0:normal once write
static int rewrite_nandkey=1;//1:disable,0:enable

static ssize_t mode_show(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
    return sprintf(buf, "debug=%s,postpone_write=%s",
                   debug_mode ? "enable" : "disable",
                   postpone_write ? "enable" : "disable");
}
static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
    if (strncmp(buf, "debug=", 6) == 0)
    {
        if (strncmp(&buf[6], "enable", 6) == 0)
        {
            debug_mode = 1;
        }
        if (strncmp(&buf[6], "disable", 7) == 0)
        {
            debug_mode = 0;
        } else
        {
            return -EINVAL;
        }

    }
    else if (strncmp(buf, "postpone_w=", 11) == 0)
    {

        if (strncmp(&buf[11], "enable", 6) == 0)
        {
            postpone_write = 1;
        }
        if (strncmp(&buf[11], "disable", 7) == 0)
        {
            postpone_write = 0;
        } else
        {
            return -EINVAL;
        }
    }
    else if(strncmp(buf, "rewrite_key=", 12) == 0)
    {
	if (strncmp(&buf[12], "enable", 6) == 0)
		{
			rewrite_nandkey = 0;
		}
		else if (strncmp(&buf[12], "disable", 7) == 0)
		{
			rewrite_nandkey = 1;
		}
		else
		{	return -EINVAL;
		}
    }
    else
    {
        return -EINVAL;
    }
    return count;
}
//static DEVICE_ATTR(mode, 0660, mode_show, mode_store);

#ifdef CRYPTO_DEPEND_ON_KERENL
int __v3_write_hash(int id,char *data)
{
}
int __v3_read_hash(int id,char *data)
{
}
#endif
#ifdef CRYPTO_DEPEND_ON_KERENL
static int aml_key_write_hash(aml_key_t * key, char * hash)
{
    struct aml_key_hash_s key_hash;
	int slot= AML_KEY_GETSLOT(key);

    key_hash.size = key->valid_size;
    memcpy(key_hash.hash, hash, sizeof(key_hash.hash));
    PRINT_HASH(hash);
    key_schematic[keys_version]->hash.write(key,slot,
                                            (char*) &key_hash);
    if (debug_mode == 0 && postpone_write == 0)
        __v3_write_hash(slot, (char*) &key_hash);
    else
        key->st |= AML_KEY_ST_EFUSE_DIRTY;

    return 0;
}
static int aml_key_read_hash(aml_key_t * key, char * hash)
{
    int i;
    struct aml_key_hash_s key_hash;
	int slot= AML_KEY_GETSLOT(key);

    if (debug_mode == 0 )
    {
        __v3_read_hash(slot, (char*) &key_hash);
        key_schematic[keys_version]->hash.write(key,slot,
                                                (char*) &key_hash);
    }
    else
        key_schematic[keys_version]->hash.read(key,slot,
												(char*) &key_hash);
    PRINT_HASH(key_hash.hash);
    printk("key_hash.size:%d,key->valid_size:%d,%s:%d\n",key_hash.size,key->valid_size,__func__,__LINE__);
    if (key_hash.size != key->valid_size && key_hash.size != 0){
		pr_err("key_hash.size != key->valid_size");
        return -EINVAL;
    }
    PRINT_HASH(key_hash.hash);
    if (key_hash.size == 0)
    {
        for (i = 0; i < 32; i++)
            if (key_hash.hash[i]){
				pr_err("key_hash.hash[i]!=0");
                return -EIO;
            }
        return 1;
    }
    PRINT_HASH(key_hash.hash);
    memcpy(hash, key_hash.hash, sizeof(key_hash.hash));
    return 0;
}
#else
static int aml_key_write_hash(aml_key_t * key, uint8_t * hash)
{
	struct aml_key_hash_s key_hash;
	int slot= 0;
	key_hash.size = key->valid_size;

	memcpy(key_hash.hash, hash, sizeof(key_hash.hash));
	if(key_schematic[keys_version]->hash.write){
	    key_schematic[keys_version]->hash.write(key,slot,(char*) &key_hash);
	}
	return 0;
}
static int aml_key_read_hash(aml_key_t * key, uint8_t * hash)
{
    int i;
    struct aml_key_hash_s key_hash;
    int slot=0;

	if(key_schematic[keys_version]->hash.read){
		key_schematic[keys_version]->hash.read(key,slot,(char*) &key_hash);
	}
	if (key_hash.size != key->valid_size && key_hash.size != 0){
		printk("%s:%d,key_hash.size != key->valid_size",__func__,__LINE__);
		return -EINVAL;
	}
	if (key_hash.size == 0)
	{
		for (i = 0; i < 32; i++){
			if (key_hash.hash[i]){
				printk("key_hash.hash[i]!=0");
				return -EIO;
			}
		}
		return 1;
    }
	memcpy(hash, key_hash.hash, sizeof(key_hash.hash));
	return 0;
}
#endif

static ssize_t key_core_show(struct device *dev, struct device_attribute *attr,
                             char *buf)
{
#define aml_key_show_error_return(error,label) {printk("%s:%d",__func__,__LINE__);n=error;goto label;}
    ssize_t n = 0;
    uint16_t checksum=0;
    size_t readbuff_validlen;
    int i;
    aml_key_t * key = (aml_key_t *) attr;
    size_t size;
    size_t out_size;
    char * data=NULL,* dec_data=NULL;

    i = CONFIG_MAX_STORAGE_KEYSIZE;
    i = ((i+15)>>4)<<4;

    data = kzalloc(i, GFP_KERNEL);
    dec_data = kzalloc(i, GFP_KERNEL);

    size=i;
#ifdef TEST_NAND_KEY_WR
	printk("key->name:%s,key->valid_size:%d,key->storage_size:%d,%s:%d\n",key->name,key->valid_size,key->storage_size,__func__,__LINE__);
#endif
    if (IS_ERR_OR_NULL(data) || IS_ERR_OR_NULL(dec_data))
        aml_key_show_error_return(-EINVAL, core_show_return);

    memset(data,0,i);
    memset(dec_data,0,i);

    if(key->read(key, (uint8_t *)data))
    {
        printk("can't get valid key,%s,%d\n",__func__,__LINE__);
        aml_key_show_error_return(-EINVAL, core_show_return);
    }

	if(aes_algorithm_decrypt){
		if (aes_algorithm_decrypt(dec_data, &size,data, key->storage_size))
			aml_key_show_error_return(-EINVAL, core_show_return);
	}
	else{
		aml_key_show_error_return(-EINVAL, core_show_return);
	}

	readbuff_validlen = ((key->valid_size+1)>>1);
	checksum = aml_key_checksum( dec_data,readbuff_validlen);
	if(checksum != key->checksum){
		#ifdef TEST_NAND_KEY_WR
		printk("checksum error: %d,%d,%s:%d\n",checksum,key->checksum,__func__,__LINE__);
		#endif
		aml_key_show_error_return(-EINVAL, core_show_return);
	}
#ifdef CRYPTO_DEPEND_ON_KERENL
    if(size!=key->valid_size)
        aml_key_show_error_return(-EINVAL, core_show_return);
    aml_key_hash(data, dec_data, key->valid_size);
    aml_key_read_hash(key, &data[64]);
    if (memcmp(data, &data[64], 32))
    {
        for(i=0;i<32;i++)
        {
            printk("%x %x |",data[i],data[64+i]);
        }
        printk("\n");
        aml_key_show_error_return(-EINVAL, core_show_return);
    }
#endif
#ifdef TEST_NAND_KEY_WR
	printk("key->valid_size:%d,key->storage_size:%d,%s:%d\n",key->valid_size,key->storage_size,__FILE__,__LINE__);
#endif

    out_size = key->valid_size;
    for (i = 0; i < out_size>>1; i++)
    {
        n += sprintf(&buf[n], "%02x", dec_data[i]);
        //printk("data[%d]:0x%x\n",i,dec_data[i]);
    }
    if(out_size%2)
    {
        n += sprintf(&buf[n], "%x", (dec_data[i]&0xf0)>>4);
        //printk("data[%d]:0x%x\n",i,(dec_data[i]&0xf0)>>4);
    }
#if 0
    // key hash valid don't output
    n += sprintf(&buf[n], "\n");
    for (i = 0; i < 32; i++)
    {
        n += sprintf(&buf[n], "%02x", data[i]);

    }
#endif
core_show_return:
    if (!IS_ERR_OR_NULL(data))
        kfree(data);
    if (!IS_ERR_OR_NULL(dec_data))
        kfree(dec_data);
    return n;
}
#ifdef CRYPTO_DEPEND_ON_KERENL
#define KEY_READ_ATTR  (S_IRUSR|S_IRGRP|S_IROTH)
#define KEY_WRITE_ATTR (S_IWUSR|S_IWGRP)
#endif

#define ENABLE_AML_KEY_DEBUG 0
#define aml_key_store_error_return(error,label) {printk("error:%s:%d\n",__func__,__LINE__);err=error;goto label;}
static ssize_t aml_key_store(aml_key_t * key, const char *buf, size_t count)
{
    int i, j, err;
    char * data = NULL;
    char * enc_data = NULL;
    //char * temp = NULL;
    size_t in_key_len=0;
    uint16_t checksum=0;
    size_t readbuff_validlen;

    err = count;
#if 0
    if (!(key_is_not_installed(*key) || key_slot_is_inval(*key)))
    {
        printk("Key has been initialled\n");
        aml_key_store_error_return(-EINVAL, store_error_return);
    }
#endif
    i = CONFIG_MAX_STORAGE_KEYSIZE;
    i = ((i+15)>>4)<<4;

    data = kzalloc(i, GFP_KERNEL);
    enc_data = kzalloc(i, GFP_KERNEL);
#if ENABLE_AML_KEY_DEBUG
    temp=kzalloc(i,GFP_KERNEL);
    if(IS_ERR_OR_NULL(temp))
        aml_key_store_error_return(-EINVAL,store_error_return);
#endif

    if (IS_ERR_OR_NULL(data) || IS_ERR_OR_NULL(enc_data))
        aml_key_store_error_return(-EINVAL, store_error_return);

    if(CONFIG_MAX_VALID_KEYSIZE<count)
    {
		printk("key size is too much, count:%d,valid:%d\n",count,CONFIG_MAX_VALID_KEYSIZE);
		aml_key_store_error_return(-EINVAL, store_error_return);
    }
    memset(data,0,i);
    memset(enc_data,0,i);
    /**
     * if key is not
     */
    for (i = 0; i * 2 < count; i++, buf += 2)
    {
        for (j = 0; j < 2; j++)
        {
            switch (buf[j])
            {
                case '0' ... '9': 
                    //data[i] |= (buf[j] - '0') << (j * 4);
                    data[i] |= (buf[j] - '0') << ((j?0:1) * 4);
                    in_key_len++;
                    break;
                case 'a' ... 'f':
                    //data[i] |= (buf[j] - 'a') << (j * 4);
                    data[i] |= (buf[j] - 'a' + 10) << ((j?0:1) * 4);
                    in_key_len++;
                    break;
                case 'A' ... 'F':
                    //data[i] |= (buf[j] - 'A') << (j * 4);
                    data[i] |= (buf[j] - 'A' + 10) << ((j?0:1) * 4);
                    in_key_len++;
                    break;
                case '\n':
                case '\r':
                case 0:
                    break;
                default:
                    aml_key_store_error_return(-EINVAL, store_error_return);
                    break;
            }
        }
        #if 0
        if (i >= key->valid_size)
        {
            printk("size is not legal %d %d\n", i, key->valid_size);
            aml_key_store_error_return(-EINVAL, store_error_return);
        }
        #endif
    }
    key->valid_size = in_key_len;
	
#ifdef CRYPTO_DEPEND_ON_KERENL
	PRINT_HASH(data);
    aml_key_hash(enc_data, data, key->valid_size);
	aml_key_read_hash(key, &enc_data[64]);
    if (key_is_not_installed(*key))
    {
        aml_key_write_hash(key, enc_data);
    }
	else
    { ///key is inval
    #ifndef TEST_NAND_KEY_WR
        if (memcmp(enc_data, &enc_data[64], 32))
        {
            printk("Hash does not equal\n");
            aml_key_store_error_return(-EINVAL, store_error_return);
        }
    #else
		// for test
		aml_key_write_hash(key, enc_data);
    #endif
    }
#endif
    ///printk("xxxxxxjjjddddddd %d\n",key->storage_size);
    readbuff_validlen = ((key->valid_size+1)>>1);
    checksum = aml_key_checksum( data,readbuff_validlen);
    key->checksum = checksum;

	if(aes_algorithm_encrypt){
		if(aes_algorithm_encrypt(enc_data, &key->storage_size, data, readbuff_validlen))
			aml_key_store_error_return(-EINVAL, store_error_return);
	}
	else{
		aml_key_store_error_return(-EINVAL, store_error_return);
	}

#ifdef TEST_NAND_KEY_WR
	printk("key:valid_size:%d,storage_size:%d,%s\n",key->valid_size,key->storage_size,__func__);
#endif
#if 0

    aml_key_decrypt(temp, &dec_size, enc_data, enc_size);
    printk("%s:%d enc_size=%d dec_size=%d\n", __FILE__, __LINE__, enc_size,
           dec_size);
    for (i = 0; i < dec_size; i++)
    {
        if (temp[i] != data[i])
        {
            printk("%s:%d %d %x %x\n", __FILE__, __LINE__, i, temp[i],
                   data[i]);
        }
    }

#endif
    key->write(key, (uint8_t *)enc_data);
    err = count;

	if(!postpone_write){
	 	key_schematic[keys_version]->flush(key_schematic[keys_version]);
	 }
store_error_return:
	if (!IS_ERR_OR_NULL(data))
        kfree(data);
    if (!IS_ERR_OR_NULL(enc_data))
        kfree(enc_data);
#if ENABLE_AML_KEY_DEBUG
    if (!IS_ERR_OR_NULL(temp))
        kfree(temp);
#endif
    return err;
}
static ssize_t key_core_store(struct device *dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
	int err;
	//dump_stack();
	//printk("%s\n\n\n",__func__);
    /**
     * @todo add sysfs node operation here;
     */
	err = aml_key_store((aml_key_t*) attr, buf, count);
    //if( aml_key_store((aml_key_t*) attr, buf, count)>=0)
    if(err >= 0)
    {
#ifdef KEY_NODE_CREATE
		#ifndef TEST_NAND_KEY_WR
		attr->attr.mode &= (~KEY_WRITE_ATTR);
		sysfs_chmod_file(&dev->kobj,attr,KEY_READ_ATTR);
		#endif
#endif
		//printk("%s,attr WR change to RD\n",__func__);
    }
    return err;
}
static ssize_t key_core_control(struct device *class,
                                struct device_attribute *attr, const char *buf,
                                size_t count)
{
    /**
     * @todo next version function
     */
    return -1;
}
static int key_check_inval(aml_key_t * key)
{
    uint8_t buf[32];
    int ret = aml_key_read_hash(key, buf);
    if (ret == -EINVAL)
    {
        key->st |= AML_KEY_ST_INVAL;
        return 1;
    }
    if (ret == 1)
    {
        key->st &= ~(AML_KEY_ST_INVAL | AML_KEY_ST_INSTALLED);
    }
    return 0;
}
static ssize_t key_node_set(struct device *dev);

#ifdef CRYPTO_DEPEND_ON_KERENL
int32_t aml_keys_set_version(struct device *dev, uint8_t version, int storer)
#else
int32_t aml_keys_set_version(char *dev, uint8_t version, int storer)
#endif
{
    aml_key_t * keys;
#ifdef CRYPTO_DEPEND_ON_KERENL
    char **keyfile = (char**) dev->platform_data;
#else
    char *keyfile = dev;
#endif
    int i, ret;
    int keyfile_index;
    //if (keys_version > 0 && keys_version < 256) ///has been initial
    //   return 0;
	if(version_dirty){
		return 0;
	}

//	printk("version:%d,%s\n",version,__func__);
    if (version < 1 || version > 255)
        return -EINVAL;

    if(storer < 0)
		return -EINVAL;
    keyfile_index = storer;
//    printk("keyfile:%s\n",keyfile[0]);
//    printk("keyfile:%s\n",keyfile[1]);
#ifdef CRYPTO_DEPEND_ON_KERENL
    if (key_schematic[version] == NULL
            || key_schematic[version]->init(key_schematic[version], keyfile[0])
                    < 0) ///@todo Platform Data
    {
        printk(KERN_ERR KEYS_DEVICE_NAME ": version %d can not be init %p\n",
               keys_version, key_schematic[version]);
        return -EINVAL;
    }
#else
    if (key_schematic[version] == NULL
            || key_schematic[version]->init(key_schematic[version], keyfile)
                    < 0) ///@todo Platform Data
    {
        printk(KERN_ERR KEYS_DEVICE_NAME ": version %d can not be init %p\n",
               keys_version, key_schematic[version]);
        return -EINVAL;
    }
#endif
    version_dirty = 1;
    keys_version = version;

    keys = key_schematic[keys_version]->keys;

    ret = key_node_set(dev);
    if (ret < 0){
		printk("creat key dev fail,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}

	//printk("%s,keys_version:%d,key_schematic[keys_version]->count:%d\n",__func__,keys_version,key_schematic[keys_version]->count);
#ifdef KEY_NODE_CREATE
    for (i = 0; i < key_schematic[keys_version]->count; i++)
    {
        mode_t mode;
        if (key_slot_is_inval(keys[i]))
            continue;
        keys[i].update_status(&keys[i]);

        mode = KEY_READ_ATTR;
        if (key_is_otp_key(keys[i]))
        {
            keys[i].attr.show = key_core_show;
			#ifdef TEST_NAND_KEY_WR  //for test
                mode |= KEY_WRITE_ATTR;
                keys[i].attr.store = key_core_store;
			#endif
            if (key_is_not_installed(keys[i]) || key_check_inval(&keys[i]))
            {
                mode |= KEY_WRITE_ATTR;
                keys[i].attr.store = key_core_store;
            } else if (key_is_readable(keys[i]))
            {
                mode |= KEY_READ_ATTR;
                keys[i].attr.show = key_core_show;
            } else if (key_is_control_node(keys[i]))
            {
                ///@todo in the future we must implement it
                printk("%s:%d\n", __FILE__, __LINE__);
                return -1;
                ///keys[i].attr.store = key_core_control;
            }
            keys[i].attr.attr.name = &keys[i].name[0];
            keys[i].attr.attr.mode = mode;
        } else
        {
            printk("%s:%d\n", __FILE__, __LINE__);
            return -EINVAL; ///@todo NO not OTP key support Now

        }
        ///type , r ,w , OTP,
        ret = device_create_file(
                dev, (const struct device_attribute *) &keys[i].attr);
        if (ret < 0)
        {
            printk("%s:%d\n", __FILE__, __LINE__);
            return -EINVAL;
        }
	printk("keys[%d].name:%s, device_create_file ok,%s:%d\n",i,keys[i].name,__FILE__,__LINE__);
    }
#endif
    /**
     * @todo remove version write interface
     */
    return 0;
}
/***
 * version show and setting rounte
 * @return
 */
static int32_t version_check(void)
{
    /**
     * @todo add real efuse operation
     */
#if 0
    return efuse_read_version();
#else

    return 3;
#endif
}

#define NAND_STORE_KEY_INDEX    0
#define EMMC_STORE_KEY_INDEX    1
#define NAND_STORE_KEY     "nand"
#define EMMC_STORE_KEY     "emmc"
//static int keyfileindex=-1;

#ifdef CRYPTO_DEPEND_ON_KERENL
static ssize_t version_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
    version_check();
    return sprintf(buf, "version=%d", keys_version);
}
static ssize_t version_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
    char *endp;
    unsigned long new ;
    int ret = -EINVAL;
    int storer;

    if (strncmp(buf, NAND_STORE_KEY, 4) == 0)
    {	storer = NAND_STORE_KEY_INDEX;
    }
    else if(strncmp(buf, EMMC_STORE_KEY, 4) == 0)
    {	storer = EMMC_STORE_KEY_INDEX;
    }
    else
    {
	printk("store memory not know\n");
	return ret;
    }

    new = simple_strtoul(buf+4, &endp, 0);

    if (endp == (buf+4))
    {
	printk("version NO error\n");
        return ret;
    }

    if (new < 1 || new > 255)
        return ret;
    //if (keys_version > 0 && keys_version < 256)
    //    return ret;

    if (aml_keys_set_version(dev, new,storer) < 0)
    {
        return ret;
    }
    /***
     * @todo remove write interface .
     */
    return count;
}
#endif

static ssize_t storer_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
    int ret = -EINVAL;
    if (keys_version == 0)
        return ret;

    if (key_schematic[keys_version]->read(key_schematic[keys_version]) < 0)
        return ret;

    //to do , need read hash to efuse
    if (debug_mode == 0 && postpone_write == 1)
    {
    }
	else{
		//key_schematic[keys_version]->dump(key_schematic[keys_version]);
	}

    return sprintf(buf, "storer=%d\n", keys_version);
}
static ssize_t storer_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
    int ret = -EINVAL;
    if (keys_version == 0)
        return ret;

    if(strncmp(buf, "write", 5) != 0)
    {
	printk("cmd error,%s:%d\n",__func__,__LINE__);
	return ret;
    }
    printk("cmd ok,%s:%d\n",__func__,__LINE__);

    if (key_schematic[keys_version]->flush(key_schematic[keys_version]) < 0)
        return -EINVAL;

    //to do , need write hash to efuse
    if (debug_mode == 0 && postpone_write == 1)
    {
    }
    printk("storer_store\n");
    return count;
}

static ssize_t key_list_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
	aml_key_t * keys;
	int i,n=0;
	keys = key_schematic[keys_version]->keys;
	for(i=0;i<key_schematic[keys_version]->count;i++)
	{
		//printk("%s,%s:%d\n",keys[i].name,__func__,__LINE__);
		if(keys[i].name[0] != 0){
			n += sprintf(&buf[n], keys[i].name);
			n += sprintf(&buf[n], "\n");
		}
	}
	buf[n] = 0;
	return n;
}
static ssize_t key_list_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
	return -1;
}

static ssize_t key_name_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
	int n=0;
	if(curkey == NULL){
		printk("please set cur key name,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	n += sprintf(&buf[n], curkey->name);
	n += sprintf(&buf[n], "\n");
	return n;
}
static char security_key_name[AML_KEY_NAMELEN];
static ssize_t key_name_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
	aml_key_t * keys;
	int i,cnt,suffix;
	char *cmd,*oldname=NULL,*newname;
	keys = key_schematic[keys_version]->keys;
	if (strncmp(buf, "rename", 6) == 0)
	{
		cmd = &buf[6];
		for(i=0;i<key_schematic[keys_version]->count;i++)
		{
			oldname = strstr(cmd,keys[i].name);
			if(oldname){
				suffix = i;
				break;
			}
		}
		if(oldname)
		{
			while(*oldname != ' '){
				oldname++;
			}
			newname = oldname;
			while(*newname == ' '){
				newname++;
			}
			oldname = newname;
			cnt=0;
			while((*oldname != ' ')&&(*oldname !='\n')&&(*oldname !='\r')&&(*oldname !=0)){
				cnt++;
				oldname++;
			}
			newname[cnt]=0;
			if(cnt >= AML_KEY_NAMELEN){
				newname[AML_KEY_NAMELEN-1]=0;
			}
			for(i=0;i<key_schematic[keys_version]->count;i++)
			{
				if(strcmp(newname,keys[i].name) == 0){
					printk("name exist,%s:%d\n",__func__,__LINE__);
					return -EINVAL;
				}
			}
			//printk("newname:%s,keys[%d].name:%s,%s:%d\n",newname,i,keys[i].name,__func__,__LINE__);
			strcpy(keys[suffix].name,newname);
			//printk("keys[%d].name:%s,%s:%d\n",i,keys[i].name,__func__,__LINE__);
		}
		else {
			printk("don't have old key name,%s:%d\n",__func__,__LINE__);
			return -EINVAL;
		}
		return count;
	}
	else if(strncmp(buf, "set", 3) == 0)
	{
		cmd = &buf[3];
		for(i=0;i<key_schematic[keys_version]->count;i++)
		{
			oldname = strstr(cmd,keys[i].name);
			if(oldname){
				curkey = &keys[i];
				break;
			}
		}
		if(oldname == NULL){
			printk("don't have the key name,%s:%d\n",__func__,__LINE__);
			return -EINVAL;
		}
		return count;
	}

	cmd = &buf[0];
	newname = cmd;
	oldname = cmd;
	cnt=0;
	while((*oldname != ' ')&&(*oldname !='\n')&&(*oldname !='\r')&&(*oldname !=0)){
		cnt++;
		oldname++;
	}
	newname[cnt]=0;
	if(cnt >= AML_KEY_NAMELEN){
		newname[AML_KEY_NAMELEN-1]=0;
	}
	curkey = NULL;
	security_key_name[0]=0;
	for(i=0;i<key_schematic[keys_version]->count;i++)
	{
		if(strcmp(newname,keys[i].name) == 0){
			curkey = &keys[i];
			break;
		}
	}
	if(curkey == NULL)
	{
		for(i=0;i<key_schematic[keys_version]->count;i++)
		{
			if(keys[i].name[0] == 0){
				curkey = &keys[i];
				break;
			}
		}
		if(curkey == NULL){
			printk("key count too much,%s:%d\n",__func__,__LINE__);
			return -EINVAL;
		}
		strcpy(security_key_name,newname);
	}

	return count;
}

static ssize_t key_write_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
	return -1;
}
static ssize_t key_write_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
	int err;
	if((security_key_name[0] != 0) &&(curkey->name[0] == 0))
	{
		strcpy(curkey->name,security_key_name);
	}
	err = aml_key_store(curkey, buf, count);
	return err;
}

static ssize_t key_read_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
	int err;
	if((curkey == NULL)||(curkey->name[0] == 0)){
		printk("unkown current key-name,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	//printk("curkey->valid_size:%d,curkey->storage_size:%d,%s:%d\n",curkey->valid_size,curkey->storage_size,__func__,__LINE__);
	err = key_core_show(dev, (struct device_attribute*)curkey,buf);
	return err;
}
static ssize_t key_read_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
	return -1;
}

#ifdef CRYPTO_DEPEND_ON_KERENL
struct key_new_node{
	struct device_attribute  attr;
	char name[16];
};
static struct key_new_node key_node_name[]={
	[0]={
		.name = "key_list",
	},
	[1]={
		.name = "key_name",
	},
	[2]={
		.name = "key_write",
	},
	[3]={
		.name = "key_read",
	},
};
static ssize_t key_node_set(struct device *dev)
{
	int ret,i;
	struct key_new_node *key_new;
	mode_t mode;
	mode = KEY_READ_ATTR;
	key_new = &key_node_name[0];
	i=0;
	key_new[i].attr.show = key_list_show;
	//key_new[i].attr.store = key_list_store;
	key_new[i].attr.attr.name = &key_new[i].name[0];
	key_new[i].attr.attr.mode = mode;
    ret = device_create_file(dev, (const struct device_attribute *) &key_new[i].attr);
    if (ret < 0)
    {
        printk("%s:%d\n", __FILE__, __LINE__);
        return -EINVAL;
    }
	i=1;
	mode = KEY_READ_ATTR | KEY_WRITE_ATTR;
	key_new[i].attr.show = key_name_show;
	key_new[i].attr.store = key_name_store;
	key_new[i].attr.attr.name = &key_new[i].name[0];
	key_new[i].attr.attr.mode = mode;
    ret = device_create_file(dev, (const struct device_attribute *) &key_new[i].attr);
    if (ret < 0)
    {
        printk("%s:%d\n", __FILE__, __LINE__);
        return -EINVAL;
    }
	i=2;
	mode =   KEY_WRITE_ATTR;
	//key_new[i].attr.show = key_write_show;
	key_new[i].attr.store = key_write_store;
	key_new[i].attr.attr.name = &key_new[i].name[0];
	key_new[i].attr.attr.mode = mode;
    ret = device_create_file(dev, (const struct device_attribute *) &key_new[i].attr);
    if (ret < 0)
    {
        printk("%s:%d\n", __FILE__, __LINE__);
        return -EINVAL;
    }
	i=3;
	mode = KEY_READ_ATTR ;
	key_new[i].attr.show = key_read_show;
	//key_new[i].attr.store = key_read_store;
	key_new[i].attr.attr.name = &key_new[i].name[0];
	key_new[i].attr.attr.mode = mode;
    ret = device_create_file(dev, (const struct device_attribute *) &key_new[i].attr);
    if (ret < 0)
    {
        printk("%s:%d\n", __FILE__, __LINE__);
        return -EINVAL;
    }
    return 0;
}
#else
static ssize_t key_node_set(struct device *dev)
{
	return 0;
}
#endif

#ifdef CRYPTO_DEPEND_ON_KERENL
static struct device * key_device= NULL;
void trigger_key_init(void)
{
    printk("amlkeys=%d\n", keys_version);
    if (key_device == NULL)
        return;
    aml_keys_set_version(key_device, version_check(), -1);
    register_reboot_notifier(&aml_keys_notify);
    version_dirty = 0;

}
#else
static struct device * key_device= NULL;
void trigger_key_init(void)
{
    printk("amlkeys=%d\n", keys_version);
    //if (key_device == NULL)
    //    return;
    //aml_keys_set_version(key_device, version_check(), -1);
    //register_reboot_notifier(&aml_keys_notify);
    //version_dirty = 0;
}
#endif

struct device_type_s{
	char devname[16];
	char storername[16];
	int  storer_index;
};
#define AUTO_IDENTIFY_STORE		0xff
#define DEVICE_TYPE_MAP_COUNT	3
static struct device_type_s device_type_map[DEVICE_TYPE_MAP_COUNT]={
	[0]={
		.devname="nand",
		.storername="nand_key",
		.storer_index = NAND_STORE_KEY_INDEX,
	},
	[1]={
		.devname="emmc",
		.storername="emmc_key",
		.storer_index = EMMC_STORE_KEY_INDEX,
	},
	[2]={
		.devname="auto",
		.storername="auto",
		.storer_index = AUTO_IDENTIFY_STORE,
	},
};

ssize_t uboot_key_init(void)
{
	int i,err = 0;
	static int uboot_key_init_flat=0;
	extern int v3_keys_init(void);
	if(uboot_key_init_flat){
		return 0;
	}
	for(i=0;i<256;i++){
		key_schematic[i]=NULL;
	}
	//for(i=0;i<PROVIDERS_NUM;i++){
		//providers[i] = NULL;
	//}
	err=v3_keys_init();
	if (err<0){
		printk("v3_keys_init error\n");
		return -1;
	}
	uboot_key_init_flat = 1;
	return 0;
}
int key_set_version(char *device)
{
	int i,err=-1;
	//char *pname = NULL;
	int flag;
	struct device_type_s *pdev = &device_type_map[0];
	flag = 0;
	for(i=0;i<DEVICE_TYPE_MAP_COUNT;i++,pdev++){
		if(strcmp(device,pdev->devname) == 0){
			//pname=pdev->platname;
			flag = 1;
			break;
		}
	}
	if(flag == 0){
		printk("device name error,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	if(pdev->storer_index != AUTO_IDENTIFY_STORE){
		err = aml_keys_set_version(pdev->storername, version_check(), pdev->storer_index);
		if(err < 0){
			printk("uboot key init fail,%s:%d\n",__func__,__LINE__);
		}
		return err;
	}
	else{
		pdev = &device_type_map[0];
		for(i=0;i<(DEVICE_TYPE_MAP_COUNT-1);i++,pdev++){
			err = aml_keys_set_version(pdev->storername, version_check(), pdev->storer_index);
			if(err < 0){
				//printk("uboot key init fail,%s:%d\n",__func__,__LINE__);
				continue;
			}
			else{
				printk("current storer:%s\n",pdev->storername);
				return err;
			}
		}
		if(i>=(DEVICE_TYPE_MAP_COUNT-1)){
			printf("%s:%d,not found storer device\n",__func__,__LINE__);
			return -1;
		}
	}
	return -1;
}

ssize_t uboot_key_write(char *keyname, char *keydata)
{
	ssize_t ret=0;
	struct device dev;
	struct device_attribute attr;
	ret = key_name_store(&dev, &attr,(const char *)keyname, strlen(keyname));
	if(ret >=0)
	{
		ret = key_write_store(&dev, &attr,(const char *)keydata, strlen(keydata));
		if(ret < 0){
			printk("keydata err,%s:%d\n",__func__,__LINE__);
		}
		return ret;
	}
	else{
		printk("keyname err,%s:%d\n",__func__,__LINE__);
		return ret;
	}
}
ssize_t uboot_key_read(char *keyname, char *keydata)
{
	aml_key_t * keys;
	ssize_t ret=0;
	struct device dev;
	struct device_attribute attr;
	int i;
	if(strlen(keyname) >= AML_KEY_NAMELEN){
		printk("keyname too lenth,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	keys = key_schematic[keys_version]->keys;
	curkey = NULL;
	security_key_name[0]=0;
	for(i=0;i<key_schematic[keys_version]->count;i++)
	{
		if(strcmp(keyname,keys[i].name) == 0){
			curkey = &keys[i];
			break;
		}
	}
	if(curkey == NULL){
		printk("don't found keyname,%s:%d\n",__func__,__LINE__);
		return -EINVAL;
	}
	ret = key_read_show(&dev, &attr,keydata);
	return ret;
}
ssize_t uboot_get_keylist(char *keylist)
{
	ssize_t ret=0;
	struct device dev;
	struct device_attribute attr;
	ret = key_list_show(&dev, &attr,keylist);
	return ret;
}


static int uboot_key_inited=0;

/*
 * device: nand,emmc,auto
 * */
int uboot_key_initial(char *device)
{
	int error=-1;
	if(uboot_key_inited )
	{
		return 0;
	}
	if ((!strcmp(device,"nand"))
		|| (!strcmp(device,"emmc"))
		|| (!strcmp(device,"auto"))){
		error=uboot_key_init();
		if(error>=0){
			error = key_set_version(device);
			if(error>=0){
//				printk("device:%s, init key ok!!\n", device);
				uboot_key_inited = 1;
				return 0;
			}
		}
	}
	return error;
}

static char hex_to_asc(char para)
{
	if(para>=0 && para<=9)
		para = para+'0';
	else if(para>=0xa && para<=0xf)
		para = para+'a'-0xa;
		
		return para;
}

static char asc_to_hex(char para)
{
	if(para>='0' && para<='9')
		para = para-'0';
	else if(para>='a' && para<='f')
		para = para-'a'+0xa;
	else if(para>='A' && para<='F')
		para = para-'A'+0xa;
		
		return para;
}


/*
 *  device: nand, emmc
 *  key_name: key name, such as: key1, hdcp
 *  key_data: key data, 
 *  key_data_len: key data lenth
 *  ascii_flag: 1,ASCII, 0: hex
 * */

ssize_t uboot_key_put(char *device,char *key_name, char *key_data,int key_data_len,int ascii_flag)
{
	char *data;
	ssize_t error=-1;
	int i,j;
	if(uboot_key_inited == 0){
		error=uboot_key_initial(device);
		if(error < 0){
			printk("%s:%d,uboot key init error\n",__func__,__LINE__);
			return -1;
		}
	}
	if (!strcmp(device,"nand") || !strcmp(device,"emmc")|| (!strcmp(device,"auto"))){
		if(ascii_flag ){
			error = uboot_key_write(key_name, key_data);
		}
		else{
			data = kzalloc(key_data_len*2+1, GFP_KERNEL);
			if(data == NULL){
				printk("%s:%d malloc mem fail\n",__func__,__LINE__);
				return -1;
			}
			memset(data,0,key_data_len*2+1);
			for(i=0,j=0;i<key_data_len;i++){
				data[j++]=hex_to_asc((key_data[i]>>4) & 0x0f);
				data[j++]=hex_to_asc((key_data[i]) & 0x0f);
			}
			error = uboot_key_write(key_name, data);
			kfree(data);
		}
	}
	return error;
}
/*
 *  device: nand, emmc
 *  key_name: key name, such as: key1, hdcp
 *  key_data: key data, 
 *  key_data_len: key data lenth
 *  ascii_flag: 1,ASCII, 0: hex
 * */
ssize_t uboot_key_get(char *device,char *key_name, char *key_data,int key_data_len,int ascii_flag)
{
	char *data;
	ssize_t error=-1;
	int i,j;
	if(uboot_key_inited == 0){
		error=uboot_key_initial(device);
		if(error < 0){
			printk("%s:%d,uboot key init error\n",__func__,__LINE__);
			return -1;
		}
	}
	if (!strcmp(device,"nand") || !strcmp(device,"emmc")|| (!strcmp(device,"auto"))){
		if(ascii_flag ){
			data = kzalloc(CONFIG_MAX_VALID_KEYSIZE, GFP_KERNEL);
			if(data == NULL){
				printk("%s:%d malloc mem fail\n",__func__,__LINE__);
				return -1;
			}
			memset(data,0,CONFIG_MAX_VALID_KEYSIZE);
			error = uboot_key_read(key_name, data);
			memcpy(key_data,data,key_data_len);
			kfree(data);
		}
		else{
			data = kzalloc(CONFIG_MAX_VALID_KEYSIZE, GFP_KERNEL);
			if(data == NULL){
				printk("%s:%d malloc mem fail\n",__func__,__LINE__);
				return -1;
			}
			memset(data,0,CONFIG_MAX_VALID_KEYSIZE);
			error = uboot_key_read(key_name, data);
			for(i=0,j=0;i<key_data_len;i++,j++){
				key_data[i]= (((asc_to_hex(data[j]))<<4) | (asc_to_hex(data[++j])));
			}
			kfree(data);
		}
	}
	return error;
}
int uboot_storer_read(char *buf,int len)
{
	int ret = -EINVAL;
	if (keys_version == 0){
		printk("%s:%d securitykey is not init\n",__func__,__LINE__);
		return ret;
	}

	if (key_schematic[keys_version]->read(key_schematic[keys_version]) < 0){
		printk("%s:%d securitykey read storer error\n",__func__,__LINE__);
		return ret;
	}

	return 0;
}
int uboot_storer_write(char *buf,int len)
{
	int ret = -EINVAL;
	if (keys_version == 0){
		printk("%s:%d securitykey is not init\n",__func__,__LINE__);
		return ret;
	}
	if (key_schematic[keys_version]->flush(key_schematic[keys_version]) < 0){
		printk("%s:%d securitykey write storer error\n",__func__,__LINE__);
        return -EINVAL;
	}
	return 0;
}

/* function name: uboot_key_query
 * device : nand,emmc,auto
 * key_name: key name
 * keystate: 0: key not exist, 1: key burned , other : reserve
 * return : <0: fail, >=0 ok
 * */
ssize_t uboot_key_query(char *device,char *key_name,unsigned int *keystate)
{
	aml_key_t * keys;
	int i;
	ssize_t error=-1;
	if(uboot_key_inited == 0){
		error=uboot_key_initial(device);
		if(error < 0){
			printk("%s:%d,uboot key init error\n",__func__,__LINE__);
			return -1;
		}
	}
	if (!strcmp(device,"nand") || !strcmp(device,"emmc")|| (!strcmp(device,"auto"))){
		error = 0;
		*keystate = 0;
		keys = key_schematic[keys_version]->keys;
		for(i=0;i<key_schematic[keys_version]->count;i++)
		{
			if(keys[i].name[0] != 0){
				if(strcmp(key_name,keys[i].name) == 0){
					*keystate = 1;
					break;
				}
			}
		}
	}
	return error;
}

